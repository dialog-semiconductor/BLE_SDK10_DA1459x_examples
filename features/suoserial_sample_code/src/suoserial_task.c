/**
 ****************************************************************************************
 *
 * @file suoserial_task.c
 *
 * @brief Software Update over UART (SUOUART) application implementation
 *
 * Copyright (C) 2016-2024 Renesas Electronics Corporation and/or its affiliates
 * The MIT License (MIT)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 ****************************************************************************************
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include "sys_power_mgr.h"
#include "osal.h"
#include "suota.h"
#include "ad_nvms.h"
#include "sys_watchdog.h"
#include "platform_devices.h"
#include "suoserial_task.h"

#include "suoserial_service.h"

/*********************************************************************
 *
 *       Defines, configurable
 *
 **********************************************************************
 */


/*
 * Calculate CLI overhead:
 * "SUOUART_PATCH_DATA 0 %d %s\n", suotabuffsz, hexstringbuff
 * xxxxxxxxxxxxxxxxxxx x          20 chars for text
 *                       yy       4 chars for length
 *                            zz  2 chars for \n and CR
 *                                6 chars to round it to 32 bytes
 */
#define CLI_PATCH_DATA_CMD_SZ   ( 32 )

/* This is the size of the binary data we can shift in one go */
#define SUOSERIAL_CHUNK_SIZE      ( 2048 )
/*
 * The host APP will probe for this figure (CMD:"getsuoserialbuffsz")
 * to decide what size chunk of code to transmit i.e. half or it.
 */
#define SUOSERIAL_BUFFER_SIZE     (SUOSERIAL_CHUNK_SIZE * 2)
/*
 * This is the size of the CLI command buffer; it can be less for some commands,
 * but we expect to have to receive and work on whole RX buffer when pulling in
 * data for SUOUART.
 */
#define CLI_BUFF_SIZE   (SUOSERIAL_BUFFER_SIZE + CLI_PATCH_DATA_CMD_SZ)

#define SUOSERIAL_TASK_PRIORITY   ( OS_TASK_PRIORITY_NORMAL )

__RETAINED volatile static uint8 run_task;

static ad_uart_handle_t uart_handle;
static uint8_t cli_buffer[CLI_BUFF_SIZE];

#define SUOSERIAL_WRITE_STATUS_STR     "SUOSERIAL_WRITE_STATUS"
#define SUOSERIAL_MEMDEV_STR           "SUOSERIAL_MEM_DEV"
#define SUOSERIAL_GPIO_MAP_STR         "SUOSERIAL_GPIO_MAP"
#define SUOSERIAL_PATCH_LEN_STR        "SUOSERIAL_PATCH_LEN"
#define SUOSERIAL_PATCH_DATA_STR       "SUOSERIAL_PATCH_DATA"
#define SUOSERIAL_READ_STATUS_STR      "SUOSERIAL_READ_STATUS"
#define SUOSERIAL_READ_MEMINFO_STR     "SUOSERIAL_READ_MEMINFO"
#define SUOSERIAL_GET_BUF_SIZE_STR     "getsuoserialbuffsz"

static uint8_t asciibyte2nibble(uint8_t *src);
static uint32_t suoserial_alloc_execution(char *argv, uint8_t **buf);
__STATIC_INLINE void suoserial_callback(const char *status);
static char *suoserial_err_str(suoserial_error_t err);
static void suoserial_fwupdate_execution(int32_t pkt_length, uint8_t *buf, uint32_t buf_size);
static void suoserial_task(void *params);
static void suoserial_printfln(const char *fmt, ...);
static int32_t suoserial_readline(uint8_t *buf, size_t size, bool echo);

/**
 * Brief:  Given an ASCII character, returns hex nibble value
 * Param:  pointer to ASCII value
 * Return: 0x00 to 0x0F, or 0xFF if invalid hex
 *         (i.e. not '0'-'9', 'a'-'f' or 'A'-'F')
 */
static uint8_t asciibyte2nibble(uint8_t *src)
{
        return ((src[0] >= '0') ?
                ((src[0] > '9') ?
                        ((src[0] >= 'A') ?
                                ((src[0] > 'F') ?
                                        ((src[0] >= 'a') ?
                                                (src[0] > 'f') ? 0xFF : (src[0] - 'a' + 10)
                                                : 0xFF)
                                        :
                                        (src[0] - 'A' + 10))
                                :
                                0xFF)
                        :
                        (src[0] - '0'))
                :
                0xFF);
}

static uint32_t suoserial_alloc_execution(char *argv, uint8_t **buf)
{
        uint32_t size = atoi(argv);

        if (size) {
                if (*buf != NULL) {
                        OS_FREE(*buf);
                        *buf = 0;
                }
                *buf = OS_MALLOC(size);
                if (*buf != NULL) {
                        suoserial_printfln("OK");
                        return size;
                } else {
                        suoserial_printfln("ERROR fail to allocate [%s]", argv[1]);
                }
        } else {
                suoserial_printfln("ERROR [%s]=INVALID", argv[1]);
        }

        return 0;
}

__STATIC_INLINE void suoserial_callback(const char *status)
{
        suoserial_printfln("INFO %s", status);
}

static char *suoserial_err_str(suoserial_error_t err)
{
        switch (err) {
        case SUOSERIAL_ERROR_OK:
                return "OK";
        case SUOSERIAL_ERROR_READ_NOT_PERMITTED:
                return "READ_NOT_PERMITTED";
        case SUOSERIAL_ERROR_REQUEST_NOT_SUPPORTED:
                return "REQUEST_NOT_SUPPORTED";
         case SUOSERIAL_ERROR_ATTRIBUTE_NOT_FOUND:
                return "ATTRIBUTE_NOT_FOUND";
        case SUOSERIAL_ERROR_ATTRIBUTE_NOT_LONG:
                return "ATTRIBUTE_NOT_LONG";
        case SUOSERIAL_ERROR_APPLICATION_ERROR:
                return "APPLICATION_ERROR";
        default:
                return "UNKNOWN";
        }
}

static void suoserial_fwupdate_execution(int32_t pkt_length, uint8_t *buf, uint32_t buf_size)
{
        uint16_t offs;
        uint16_t size;
        uint32_t slen;
        uint8_t *src;
        uint8_t *dst;
        suoserial_error_t err;
        uint32_t value;
        bool read;

        /* Keep going until get an empty line */
        while (pkt_length > 0) {
                pkt_length = suoserial_readline(cli_buffer, sizeof(cli_buffer), false) ? true : false;

                if (pkt_length > 0) {
                        uint32_t len = strlen((char *)cli_buffer);

                        /* Strip CRs and LFs from end */
                        while (len && ((cli_buffer[len - 1] == '\r') || (cli_buffer[len - 1] == '\n'))) {
                                len--;
                        }
                        cli_buffer[len] = 0;
                }

                uint32_t my_len = 0;

                /* Work as a CLI */
                if ((pkt_length > 0) && (cli_buffer[0] != 0)) {
                        uint32_t len = strlen((char *)cli_buffer);
                        char *argv[10];
                        uint32_t argc = 0;
                        uint32_t n = 0;
                        uint8_t *p = cli_buffer;

                        my_len = len;

                        /* Pseudo-strtok */
                        while ((n < len) && (argc < 10)) {
                                argv[argc++] = (char *)&p[n];

                                while ((n < len) && (p[n] != ' ')) {
                                        n++;
                                }

                                p[n] = 0;
                                n++;

                                while ((n < len) && (p[n] == ' ')) {
                                        n++;
                                }
                        }

                        /* Process valid requests */
                        if (argc != 4) {
                                suoserial_printfln("ERROR wrong number of parameters! argc=%d. len=%d", argc, my_len);
                                return;
                        }

                        offs = atoi(argv[1]);
                        size = atoi(argv[2]);
                        slen = strlen(argv[3]);

                        /* Ensure we have enough working buffer allocated and message is consistent */
                        if (buf == NULL) {
                                suoserial_printfln("ERROR no buffer!");
                                return;
                        }

                        if (size > buf_size) {
                                suoserial_printfln("ERROR out of bounds! (%d > %d buffer)", size, buf_size);
                                return;
                        }

                        if ((size * 2) != slen) {
                                suoserial_printfln("ERROR size[%d] != string given[slen=%d]", size, slen);
                                return;
                        }

                        src = (uint8_t *)argv[3];
                        dst = buf;
                        err = SUOSERIAL_ERROR_REQUEST_NOT_SUPPORTED;
                        read = false;

                        /* Convert hex string back to data */
                        for (n = 0; n < size; n++) {
                                uint8_t hi, lo, hex;
                                hi = asciibyte2nibble(src++);
                                lo = asciibyte2nibble(src++);
                                hex = ((hi << 4) | lo);
                                dst[n] = hex;
                        }

                        if (0 == strcmp(argv[0], SUOSERIAL_WRITE_STATUS_STR)) {
                                printf("fwupdate: " SUOSERIAL_WRITE_STATUS_STR "[%s]\r\n", argv[3]);
                                err = suoserial_write_req(SUOSERIAL_WRITE_STATUS, offs, size, buf);
                        }
                        else if (0 == strcmp(argv[0], SUOSERIAL_MEMDEV_STR)) {
                                printf("fwupdate: " SUOSERIAL_MEMDEV_STR "[%s]\n", argv[3]);
                                err = suoserial_write_req(SUOSERIAL_WRITE_MEMDEV, offs, size, buf);
                        }
                        else if (0 == strcmp(argv[0], SUOSERIAL_PATCH_LEN_STR)) {
                                printf("fwupdate: " SUOSERIAL_PATCH_LEN_STR "[%s]\r\n", argv[3]);
                                err = suoserial_write_req(SUOSERIAL_WRITE_PATCH_LEN, offs, size, buf);
                        }
                        else if (0 == strcmp(argv[0], SUOSERIAL_PATCH_DATA_STR)) {
                                err = suoserial_write_req(SUOSERIAL_WRITE_PATCH_DATA, offs, size, buf);
                        }
                        else if (0 == strcmp(argv[0], SUOSERIAL_READ_STATUS_STR)) {
                                read = true;
                                err = suoserial_read_req(SUOSERIAL_READ_STATUS, &value);
                                printf("fwupdate: " SUOSERIAL_READ_STATUS_STR "[%04lx]\r\n", value);
                        }
                        else if (0 == strcmp(argv[0], SUOSERIAL_READ_MEMINFO_STR)) {
                                read = true;
                                err = suoserial_read_req(SUOSERIAL_READ_MEMINFO, &value);
                                printf("fwupdate: " SUOSERIAL_READ_MEMINFO_STR "[%04lx]\r\n", value);
                        }
                        else {
                                err = SUOSERIAL_ERROR_REQUEST_NOT_SUPPORTED;
                                printf("fwupdate: what? [%s]\r\n", argv[0]);
                        }

                        if (err == SUOSERIAL_ERROR_OK) {
                                if (read) {
                                        suoserial_printfln("OK %d", value);
                                } else {
                                        if (0 != strcmp(argv[0], SUOSERIAL_PATCH_DATA_STR))
                                                suoserial_printfln("OK");
                                }
                        } else {
                                suoserial_printfln("ERROR %s", suoserial_err_str(err));
                        }
                }
        }

        printf(("fwupdate: done"));
}

static OS_TASK_FUNCTION(suoserial_task, pvParams)
{
        (void)pvParams;
        int32_t length;
        uint8_t *qspibuf = NULL;
        uint32_t qspibufsz = 0;

        /*
         * Register callback for status notifications from software update process
         * also get active firmware image and product header.
         */
        suoserial_init(suoserial_callback);

        run_task = 1;

        uart_handle = ad_uart_open(&suouart_conf);
        ASSERT_ERROR(uart_handle != NULL);

        while (run_task == 1) {
                /* Indicate CLI prompt */
                ad_uart_write(uart_handle, (char *)">", 1);

                length = suoserial_readline(cli_buffer, sizeof(cli_buffer), true);

                if (length > 0) {
                        uint32_t len = strlen((char *)cli_buffer);
                        /* Strip CRs and LFs from end */
                        while (len && ((cli_buffer[len - 1] == '\r') ||
                                        (cli_buffer[len - 1] == '\n'))) {
                                len--;
                        }
                        cli_buffer[len] = 0;

                        /* Proper CRLF before new prompt */
                        ad_uart_write(uart_handle, (char *)"\r\n", 2);
                }

                /* Work as a CLI */
                if ((length > 0) && (cli_buffer[0] != 0)) {
                        uint32_t len = strlen((char *)cli_buffer);
                        char *argv[10];
                        uint32_t argc = 0;
                        uint32_t n = 0;
                        uint8_t *p = cli_buffer;

                        /* Pseudo-strtok */
                        while ((n < len) && (argc < 10)) {
                                /* Store arguments */
                                argv[argc++] = (char *)&p[n];

                                /* Reach end of current argument */
                                while ((n < len) && (p[n] != ' ')) {
                                        n++;
                                }
                                p[n] = 0;
                                n++;

                                /* Reach the next argument */
                                while ((n < len) && (p[n] == ' ')) {
                                        n++;
                                }
                        }

                        /* Command interpretation */
                        if ((0 == strcmp(argv[0], "alloc")) && (argc == 2)) {
                                qspibufsz = suoserial_alloc_execution(argv[1], &qspibuf);
                        }
                        else if ((0 == strcmp(argv[0], SUOSERIAL_GET_BUF_SIZE_STR)) && (argc == 1)) {
                                suoserial_printfln("OK %d", SUOSERIAL_BUFFER_SIZE);
                        }
                        else if ((0 == strcmp(argv[0], "fwupdate")) && (argc == 1)) {

                                if ((!qspibuf) || (qspibufsz > (CLI_BUFF_SIZE / 2))) {
                                        suoserial_printfln("ERROR use 'alloc' to define buffer with size <= %d",
                                                (CLI_BUFF_SIZE / 2));
                                }
                                suoserial_printfln("OK");

                                suoserial_fwupdate_execution(length, qspibuf, qspibufsz);
                        }
                        else if ((0 == strcmp(argv[0], "readsdtparam")) && (argc == 1)) {

                                nvms_t nvms_h;
                                nvms_h = ad_nvms_open(NVMS_PARAM_PART);
                                uint8_t param_data[84];
                                ad_nvms_read(nvms_h, 0, param_data, 84);

                                for (uint8_t i = 0; i < 84; i++) {
                                        suoserial_printfln("%02x", param_data[i]);
                                }
                        }
                        else {
                                suoserial_printfln("ERROR unrecognised command [%s]", argv[0]);
                                for (n = 1; n < argc; n++) {
                                        suoserial_printfln("ERROR argument %d = [%s]", n, argv[n]);
                                }
                        }
                }
        }
        run_task = 0;

        OS_TASK_DELETE(NULL);
}

void suoserial_start_task(void)
{
        if (run_task == 0) {
                run_task = 1;

                OS_BASE_TYPE status;
                OS_TASK task_handle __UNUSED;

                /* Start the SUOUART application task. */
                status = OS_TASK_CREATE("SUOSerialTask",          /* The text name assigned to the task, for
                                                                     debug only; not used by the kernel. */
                                suoserial_task,                   /* The function that implements the task. */
                                NULL,                             /* The parameter passed to the task. */
                                (8 * 1024),                       /* The number of bytes to allocate to the
                                                                     stack of the task. */
                                SUOSERIAL_TASK_PRIORITY,          /* The priority assigned to the task. */
                                task_handle);                     /* The task handle. */
                OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);
        }
}

static void suoserial_printfln(const char *fmt, ...)
{
        char buf[127 + 2];
        va_list ap;
        int len;

        /*
         * Format line of text into buffer and save space for newline.
         * vsnprintf() reserves 1 byte for \0, but we'll overwrite it
         * with newline anyway.
         */
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf) - 2 + 1, fmt, ap);
        va_end(ap);

        /*
         * Append newline characters at the end of string. It's OK to overwrite \0
         * since string does not need to be null-terminated for sending over UART
         * (it's handled as binary buffer).
         */
        len = strlen(buf);
        memcpy(&buf[len], "\n\r", 2);

        len += 2;
        ad_uart_write(uart_handle, buf, len);

}

/**
 * Brief:   Readline for UART device
 *          Will cancel and flush reception if buffer size exceeds
 *
 * Param 1: buffer pointer
 * Param 2: buffer size
 * return:  confirm if line is still active
 */
static int32_t suoserial_readline(uint8_t *buf, size_t size, bool echo)
{
        char c = 0;
        uint8_t *start = buf;
        int32_t len;
        int ret = -1;

        *start = 0; // Ensure to invalidate old buffer

        do {
                ret = ad_uart_read(uart_handle, &c, 1, 0);

                if (run_task == 0) {
                        return 0;
                }

                if (ret != 1) {
                        continue;
                }

                if (echo) {
                        if ((c != '\n') && (c != '\r')) {
                                ret = ad_uart_write(uart_handle, &c, 1);
                                if (ret < 0) {
                                        continue;
                                }
                        }
                }

                if (c == 8) { // backspace
                        buf--;
                        size++;
                        /* Overwrite old character */
                        if (echo) {
                                ret = ad_uart_write(uart_handle, (char *)" ", 1);
                                if (ret < 0) {
                                        return 0;
                                }
                                ret = ad_uart_write(uart_handle, &c, 1);
                                if (ret < 0) {
                                        return 0;
                                }
                        }
                }
                else {
                        *(buf++) = c;
                        size--;
                }
        } while ((c != '\n') && (c != '\r') && (size > 1)); // Wait for CR or reserve 1 char for \0

        /* Reject overly long lines, flush remainder, return NULL string */
        if (size == 1) {
                *start = 0; // empty string
                do {
                        ret = ad_uart_read(uart_handle, &c, 1, 0);
                        if (ret != 1) {
                                return 0;
                        }
                } while ((c != '\n') && (c != '\r')); // wait for CR/LF
        }

        /* Make sure it's null-terminated */
        *buf = '\0';

        len = strlen((char *)start);
        if (!len) {
                ad_uart_write(uart_handle, &c, 1);
        }

        return len;
}

