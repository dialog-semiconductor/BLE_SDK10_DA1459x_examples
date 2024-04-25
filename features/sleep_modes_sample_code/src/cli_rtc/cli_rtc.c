/**
 ****************************************************************************************
 *
 * @file cli_rtc.c
 *
 * @brief RTC shell operations
 *
 * Copyright (C) 2015-2023 Renesas Electronics Corporation and/or its affiliates.
 * All rights reserved. Confidential Information.
 *
 * This software ("Software") is supplied by Renesas Electronics Corporation and/or its
 * affiliates ("Renesas"). Renesas grants you a personal, non-exclusive, non-transferable,
 * revocable, non-sub-licensable right and license to use the Software, solely if used in
 * or together with Renesas products. You may make copies of this Software, provided this
 * copyright notice and disclaimer ("Notice") is included in all such copies. Renesas
 * reserves the right to change or discontinue the Software at any time without notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS". RENESAS DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. TO THE
 * MAXIMUM EXTENT PERMITTED UNDER LAW, IN NO EVENT SHALL RENESAS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE, EVEN IF RENESAS HAS BEEN ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES. USE OF THIS SOFTWARE MAY BE SUBJECT TO TERMS AND CONDITIONS CONTAINED IN
 * AN ADDITIONAL AGREEMENT BETWEEN YOU AND RENESAS. IN CASE OF CONFLICT BETWEEN THE TERMS
 * OF THIS NOTICE AND ANY SUCH ADDITIONAL LICENSE AGREEMENT, THE TERMS OF THE AGREEMENT
 * SHALL TAKE PRECEDENCE. BY CONTINUING TO USE THIS SOFTWARE, YOU AGREE TO THE TERMS OF
 * THIS NOTICE.IF YOU DO NOT AGREE TO THESE TERMS, YOU ARE NOT PERMITTED TO USE THIS
 * SOFTWARE.
 *
 ****************************************************************************************
 */

#include <string.h>
#include <stdlib.h>
#include "misc.h"
#include "hw_rtc.h"
#include "cli_rtc.h"
#include "hw_pdc.h"
#include "osal.h"
#include "sys_power_mgr.h"
#include "cli_app_config.h"

#if APP_CLI_RTC_EN
/*
 * Magic value indicating the RTC has been initialized.
 *
 * \note Applicable as long as the device is powered!!!
 */
#define APP_RTC_MAGIC_VALUE  0xAABBCCDD

__RETAINED_RW static struct rtc_cfg_t {
        hw_rtc_time_t time;
        hw_rtc_calendar_t calendar;
} rtc_config = {
        { APP_RTC_MODE_24H ? RTC_24H_CLK : RTC_12H_CLK,
          APP_RTC_12H_PM ? true : false,
          APP_RTC_HOUR, APP_RTC_MIN, APP_RTC_SEC, APP_RTC_HSEC },
        { APP_RTC_YEAR, APP_RTC_MONTH, APP_RTC_MDAY, APP_RTC_WDAY }
};

__RETAINED_UNINIT static uint32_t app_rtc_magic_value;

__RETAINED static cli_rtc_alarm_callback alarm_cb;

/* Formats accepted when setting date and/or time */
static const char format_iso8601[] = "%FT%T";
static const char format_time[] = "%T";
static const char format_date[] = " %F";

static const char *consume_chars(const char *s, char *dest, unsigned int cnt)
{
        if (strlen(s) < cnt) {
                return NULL;
        }

        memcpy(dest, s, cnt);
        dest[cnt] = '\0';

        return s + cnt;
}

static const char *consume_char(const char *s, char ch)
{
        if (*s != ch) {
                return NULL;
        }
        return ++s;
}

static const char *consume_date(const char *s, hw_rtc_calendar_t *calendar)
{
        char year[4 + 1];
        char month[2 + 1];
        char day[2 + 1];

        s = consume_chars(s, year, 4);
        if (!s) {
                return NULL;
        }

        s = consume_char(s, '-');
        if (!s) {
                return NULL;
        }

        s = consume_chars(s, month, 2);
        if (!s) {
                return NULL;
        }

        s = consume_char(s, '-');
        if (!s) {
                return NULL;
        }

        s = consume_chars(s, day, 2);
        if (!s) {
                return NULL;
        }

        calendar->year = atoi(year);
        calendar->month = atoi(month);
        calendar->mday = atoi(day);

        return s;
}

static const char *consume_time(const char *s, hw_rtc_time_t *time)
{
        char hour[2 + 1];
        char minute[2 + 1];
        char second[2 + 1];

        s = consume_chars(s, hour, 2);
        if (!s) {
                return NULL;
        }

        s = consume_char(s, ':');
        if (!s) {
                return NULL;
        }

        s = consume_chars(s, minute, 2);
        if (!s) {
                return NULL;
        }

        s = consume_char(s, ':');
        if (!s) {
                return NULL;
        }

        s = consume_chars(s, second, 2);
        if (!s) {
                return NULL;
        }

        time->hour = atoi(hour);
        time->minute = atoi(minute);
        time->sec = atoi(second);

        return s;
}

static char *strptime(const char *s, const char *format, hw_rtc_time_t *time, hw_rtc_calendar_t *calendar)
{
        /* Reduced implementation of strptime -
         * accepting only the 3 different format strings
         */
        if (!strcmp(format, format_iso8601)) {
                s = consume_date(s, calendar);
                if (!s) {
                        return NULL;
                }

                s = consume_char(s, 'T');
                if (!s) {
                        return NULL;
                }

                s = consume_time(s, time);
                if (!s) {
                        return NULL;
                }

                return (char *)s;

        } else if (!strcmp(format, format_time)) {
                return (char *)consume_time(s, time);

        } else if (!strcmp(format, format_date)) {
                return (char *)consume_date(s, calendar);

        } else {
                return NULL;
        }
}

void cli_rtc_time_get_handler(int argc, const char *argv[], void *user_data)
{
        hw_rtc_time_t time;
        hw_rtc_calendar_t calendar;

        hw_rtc_get_time_clndr(&time, &calendar);

        DBG_PRINTF(FG_GREEN "Weekday:%02d Date:%04d-%02d-%02d - Time:%02d:%02d:%02d" DBG_NEW_LINE,
                calendar.wday, calendar.year, calendar.month, calendar.mday,
                time.hour, time.minute, time.sec);
}

void cli_rtc_time_set_handler(int argc, const char *argv[], void *user_data)
{
        hw_rtc_time_t time;
        hw_rtc_calendar_t calendar;
        const char *format;
        HW_RTC_SET_REG_STATUS status __unused;

        hw_rtc_get_time_clndr(&time, &calendar);

        if (strchr(argv[1], 'T')) {
                format = format_iso8601;
        } else if (strchr(argv[1], '-')) {
                format = format_date;
        } else {
                format = format_time;
        }

        char *parseRes = strptime(argv[1], format, &time, &calendar);

        if (!parseRes || *parseRes != '\0') {
                DBG_PRINTF(FG_RED "Invalid RTC timer & calendar" DBG_NEW_LINE);
                return;
        }

        status = hw_rtc_set_time_clndr(&time, &calendar);
        /* Timer should be valid if this line is reached */
        OS_ASSERT(status == HW_RTC_VALID_ENTRY);
}

void cli_rtc_alarm_set_handler(int argc, const char *argv[], void *user_data)
{
        hw_rtc_time_t time;
        hw_rtc_calendar_t calendar;
        hw_rtc_alarm_calendar_t alarm_calendar;
        const char *format;
        HW_RTC_SET_REG_STATUS status __unused;
        uint8_t alarm_mask = 0xff;

        hw_rtc_get_time_clndr(NULL, &calendar);
        hw_rtc_get_alarm(&time, &alarm_calendar, NULL);

        if (strchr(argv[1], 'T')) {
                format = format_iso8601;
        } else if (strchr(argv[1], '-')) {
                format = format_date;
                alarm_mask &= ~(HW_RTC_ALARM_HOUR | HW_RTC_ALARM_MIN | HW_RTC_ALARM_SEC | HW_RTC_ALARM_HSEC);
        } else {
                format = format_time;
                alarm_mask &= ~(HW_RTC_ALARM_MONTH | HW_RTC_ALARM_MDAY);
        }

        char *parseRes = strptime(argv[1], format, &time, &calendar);

        if (!parseRes || *parseRes != '\0') {
                DBG_PRINTF(FG_RED "Invalid RTC alarm" DBG_NEW_LINE);
                return;
        }

        alarm_calendar.mday = calendar.mday;
        alarm_calendar.month = calendar.month;

        status = hw_rtc_set_alarm(&time, &alarm_calendar, alarm_mask);
        /* Timer should be valid if this line is reached */
        OS_ASSERT(status == HW_RTC_VALID_ENTRY);
}

void cli_rtc_alarm_get_handler(int argc, const char *argv[], void *user_data)
{
        hw_rtc_time_t time;
        hw_rtc_alarm_calendar_t calendar;
        uint8_t int_mask;

        hw_rtc_get_alarm(&time, &calendar, &int_mask);

        DBG_PRINTF(FG_GREEN "Mask:%02d Alarm:%02d-%02d - Time:%02d:%02d:%02d" DBG_NEW_LINE,
                int_mask, calendar.month, calendar.mday,
                time.hour, time.minute, time.sec);
}

static void cli_rtc_pdc_entry_add(void)
{
        uint32_t pdc_idx;

        pdc_idx = hw_pdc_add_entry(HW_PDC_LUT_ENTRY_VAL(HW_PDC_TRIG_SELECT_PERIPHERAL,
                                HW_PDC_PERIPH_TRIG_ID_RTC_ALARM,
                                HW_PDC_MASTER_CM33, 0));
        ASSERT_WARNING(pdc_idx != HW_PDC_INVALID_LUT_INDEX);

        hw_pdc_set_pending(pdc_idx);
        hw_pdc_acknowledge(pdc_idx);
}

static void cli_rtc_event_cb(uint8_t event)
{
        uint8_t int_mask = hw_rtc_get_interrupt_mask();

        /* Exercise the event and check if the corresponding interrupt mask is cleared (enabled). */
        if ((event & HW_RTC_EVENT_ALRM) && !(int_mask & HW_RTC_INT_ALRM)) {
                if (alarm_cb) {
                        alarm_cb();
                }
        }
}

static void cli_rtc_init(void)
{
        hw_rtc_set_hour_clk_mode(APP_RTC_MODE_24H ? RTC_24H_CLK : RTC_12H_CLK);

        /* RTC counters can be retained following a HW/SW reset */
        if (app_rtc_magic_value != APP_RTC_MAGIC_VALUE) {
                HW_RTC_SET_REG_STATUS ret;

                ret = hw_rtc_set_time_clndr(&rtc_config.time, &rtc_config.calendar);
                if (ret != HW_RTC_VALID_ENTRY) {
                        ASSERT_WARNING(0);
                }
                app_rtc_magic_value = APP_RTC_MAGIC_VALUE;
        }

        /* Clear the RTC event status register */
        hw_rtc_get_event_flags();

        /*
         * Enable RTC interrupts. One or more interrupt sources can be unmasked (enabled)
         * The selected interrupt sources should be bitwise-ORed.
         */
        hw_rtc_register_intr(cli_rtc_event_cb, 0x00);

        hw_rtc_clock_enable();
        hw_rtc_start();

        /* Register RTC to PDC so the main core can be notified following RTC events */
        cli_rtc_pdc_entry_add();
}

void cli_rtc_alarm_register_cb(cli_rtc_alarm_callback cb)
{
        alarm_cb = cb;
}

DEVICE_INIT(cli_rtc, cli_rtc_init, NULL);

#endif /* APP_CLI_RTC_EN */
