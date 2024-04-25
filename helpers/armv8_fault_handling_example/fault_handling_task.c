/**
 ****************************************************************************************
 *
 * @file fault_handling_task.c
 *
 * @brief ARMv8 Fault Handling Task
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
#include <stdlib.h>
#include <stdio.h>
#include "sys_power_mgr.h"
#include "sys_watchdog.h"
#include "cli.h"
#include "misc.h"
#include "hw_mpu.h"
#include "user_application.h"

/* Bit mask notifications */
#define FAULT_HANDLING_TASK_CLI_NOTIF   (1 << 0)

/* Handler to trigger a BusFault exception. */
static void busfault_handler(int argc, const char *argv[], void *user_data)
{
        if (argc > 1) {
                DBG_PRINTF("\n\rNo arguments are required!\n\r");
        }

        /* Attempt to access a memory location outside memory boundaries */
        volatile uint8_t *p = (volatile uint8_t *)0x99999999UL;
        *p = 0xFF;
}

/* Handler to trigger a HardFault exception. */
static void hardfault_handler(int argc, const char *argv[], void *user_data)
{
        if (argc > 1) {
                DBG_PRINTF("\n\rNo arguments are required!\n\r");
        }

#if (dg_configCODE_LOCATION == NON_VOLATILE_IS_NONE)
#pragma message "HardFault should not work when debugger is attached!"
#endif
        /*
         * Execution of a breakpoint instruction and while the CPU
         * not in debug mode, will trigger a HardFault.
         */
        __BKPT(0);
}

/* Handler to trigger a MemManagement fault exception */
static void memmanage_fault_handler(int argc, const char *argv[], void *user_data)
{
        if (argc > 1) {
                DBG_PRINTF("\n\rNo arguments are required!\n\r");
        }

        /* MPU Settings */
        mpu_region_config region_cfg = {
                .access_permissions = HW_MPU_AP_RO,
                .attributes         = HW_MPU_ATTR_NORMAL,
                .execute_never      = HW_MPU_XN_FALSE,
                .start_addr         = MEMORY_SYSRAM_BASE & 0xFFFFFFE0,         // The appropriate mask should be applied when writing to
                .end_addr           = (MEMORY_SYSRAM_BASE + 0x200 - 1) | 0x1F  // The appropriate mask should be applied when writing to
         };

        hw_mpu_disable();
        hw_mpu_config_region(HW_MPU_REGION_0, &region_cfg);
        hw_mpu_enable(true);

        /* Attempt to perform a write operation in a Read-only SRAM region */
        volatile uint32_t *ptr = (volatile uint32_t *)MEMORY_SYSRAM_BASE;
        *ptr = 0xFFFFFFFF;
}

/* Handler to trigger a UsageFault exception. */
static void usagefault_handler(int argc, const char *argv[], void *user_data)
{
        if (argc > 1) {
                DBG_PRINTF("\n\rNo arguments are required!\n\r");
        }

        /* Perform an unaligned memory access */
        volatile uint32_t *ptr = (volatile uint32_t *)0x50030E01;
        *ptr = 0xFFFFFFFF;
}

/* Handler to trigger an escalated BusFault exception. */
static void escalated_fault_handler(int argc, const char *argv[], void *user_data)
{
        if (argc > 1) {
                DBG_PRINTF("\n\rNo arguments are required!\n\r");
        }

        fault_escalation_status_set(true);

        /* Attempt to access a memory location outside memory boundaries */
        volatile uint8_t *p = (volatile uint8_t *)0x99999999UL;
        *p = 0xFF;
}

/* Recursive function to trigger stack overflow. */
static void test_overflow_func(void)
{
        test_overflow_func();
}

/* Handler to trigger task overflow (PSP) and thus, raising a UsageFault exception. */
static void task_overflow_handler(int argc, const char *argv[], void *user_data)
{
        ASSERT_WARNING(!in_interrupt());

        if (argc > 1) {
                DBG_PRINTF("\n\rNo arguments are required!\n\r");
        }

        test_overflow_func();
}

__ALWAYS_RETAINED_CODE
void SysTick_Handler(void)
{
        /* Stop timer */
        SysTick->CTRL = 0;
        test_overflow_func();
}

/* Handler to trigger stack overflow (PSP) and thus, raising a UsageFault exception. */
static void stack_overflow_handler(int argc, const char *argv[], void *user_data)
{
        uint32_t control = __get_CONTROL();

        /* Make sure SysTick register file is accessible (only in privilege mode). */
        ASSERT_WARNING((control & CONTROL_nPRIV_Pos) == 0);
        /* Make sure MSPLIM is configured */
        ASSERT_WARNING(__get_MSPLIM());

        /* Start SysTick timer so its ISR is fired. */
        SysTick_Config(0xFF);
}

/* Registered commands along with their handlers */
static const cli_command_t cmd_handlers[] = {
        {"busfault",        busfault_handler,        NULL},
        {"hardfault",       hardfault_handler,       NULL},
        {"memmanage_fault", memmanage_fault_handler, NULL},
        {"usagefault",      usagefault_handler,      NULL},
        {"escalated_fault", escalated_fault_handler, NULL},
        {"task_overflow",   task_overflow_handler,   NULL},
        {"stack_overflow",  stack_overflow_handler,  NULL},
        {} /* A null entry is required.*/
};

/* CLI handler to be called when no handler has been matched for the requested task. */
static void cli_default_handler(int argc, const char *argv[], void *user_data)
{
        DBG_PRINTF("\n\rInvalid command. Retry with: "
                                                "busfault, "
                                                "hardfault, "
                                                "memmanage_fault, "
                                                "usagefault, "
                                                "escalated_fault, "
                                                "task_overflow, "
                                                "stack_overflow "
                                                "\n\r");
}

/* Main application task */
OS_TASK_FUNCTION(prvFaultHandlingTask, pvParameters)
{
        OS_BASE_TYPE ret;
        uint32_t notif;
        int8_t wdog_id;
        cli_t cli;

        system_boot_status_check_and_analyze();

        wdog_id = sys_watchdog_register(false);

        /* When the CONSOLE/CLI modules are enabled and because of the UART adapter invocation,
         * the device is only allowed to enter the idle sleep state (low-power WFI ARM state). */
        cli_init();
        cli = cli_register(FAULT_HANDLING_TASK_CLI_NOTIF, cmd_handlers, cli_default_handler);

        DBG_PRINTF("\n\rEnter a valid command and press enter...\n\r");

        for (;;) {

                sys_watchdog_notify(wdog_id);
                /* Remove task id from the monitoring list as long as a task remains in the blocking state. */
                sys_watchdog_suspend(wdog_id);

                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                OS_ASSERT(ret == OS_OK);

                sys_watchdog_notify_and_resume(wdog_id);

                if (notif & FAULT_HANDLING_TASK_CLI_NOTIF) {
                      cli_handle_notified(cli);
                }
        }
}
