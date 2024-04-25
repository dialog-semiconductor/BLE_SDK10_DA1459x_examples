/**
 ****************************************************************************************
 *
 * @file sleep_modes_task.c
 *
 * @brief System sleep modes demonstration example
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
#include <string.h>
#include "sys_power_mgr.h"
#include "sys_clock_mgr.h"
#include "sys_watchdog.h"
#include "cli.h"
#include "console.h"
#include "misc.h"

#include "cli_app.h"
#include "cli_deep_sleep.h"
#include "cli_hibernation.h"
#include "cli_por_gpio.h"
#include "cli_rtc.h"
#include "cli_app_config.h"

#include "hw_wkup.h"

__RETAINED static OS_TASK app_task_h;

#define APP_WKUP_CTS_N_ACTIVE_NOTIF    (1 << 1)
#define APP_WKUP_CTS_N_INACTIVE_NOTIF  (1 << 2)
#define APP_CLI_EVT_NOTIF              (1 << 3)
#define APP_RTC_ALARM_EVT_NOTIF        (1 << 4)

#if APP_CLI_RTC_EN
static void app_rtc_alarm_cb(void)
{
        if (in_interrupt()) {
                OS_TASK_NOTIFY_FROM_ISR(app_task_h, APP_RTC_ALARM_EVT_NOTIF, OS_NOTIFY_SET_BITS);
        } else {
                OS_TASK_NOTIFY(app_task_h, APP_RTC_ALARM_EVT_NOTIF, OS_NOTIFY_SET_BITS);
        }
}
#endif

static void app_gpio0_cb(uint32_t status)
{
#if dg_configUSE_CONSOLE
        if (status & (1 << SER1_CTS_PIN)) {
                HW_WKUP_TRIGGER trigger;

                /* Invert trigger logic */
                trigger = ((hw_wkup_get_trigger(SER1_CTS_PORT, SER1_CTS_PIN) == HW_WKUP_TRIG_EDGE_LO) ?
                        HW_WKUP_TRIG_EDGE_HI : HW_WKUP_TRIG_EDGE_LO);
                hw_wkup_set_trigger(SER1_CTS_PORT, SER1_CTS_PIN, trigger);

                OS_TASK_NOTIFY_FROM_ISR(app_task_h, trigger == HW_WKUP_TRIG_EDGE_LO ?
                                APP_WKUP_CTS_N_INACTIVE_NOTIF :
                                APP_WKUP_CTS_N_ACTIVE_NOTIF, OS_NOTIFY_SET_BITS);
        }
#endif
}

static const cli_command_t cmd_handlers[] = {
#if APP_CLI_DEEP_SLEEP_EN
        { "deep_sleep_set_status", cli_deep_sleep_set_status_handler,   NULL },
        { "deep_sleep_sel_port",   cli_deep_sleep_select_port_handler,  NULL },
        { "deep_sleep_reset_port", cli_deep_sleep_reset_port_handler,   NULL },
        { "deep_sleep_get_info",   cli_deep_sleep_get_info_handler,     NULL },
#endif
#if APP_CLI_HIBERNATION_EN
        { "hibernation_set_status", cli_hibernation_set_status_handler,  NULL },
        { "hibernation_sel_port",   cli_hibernation_select_port_handler, NULL },
        { "hibernation_get_info",   cli_hibernation_get_info_handler,    NULL },
#endif
#if APP_CLI_POR_GPIO_EN
        { "por_gpio_sel_port",     cli_por_gpio_select_port_handler,    NULL },
        { "por_gpio_set_timeout",  cli_por_gpio_set_timeout_handler,    NULL },
        { "por_gpio_get_info",     cli_por_gpio_get_info_handler,       NULL },
#endif
#if APP_CLI_RTC_EN
        { "rtc_time_get",          cli_rtc_time_get_handler,            NULL },
        { "rtc_alarm_get",         cli_rtc_alarm_get_handler,           NULL },
        { "rtc_time_set",          cli_rtc_time_set_handler,            NULL },
        { "rtc_alarm_set",         cli_rtc_alarm_set_handler,           NULL },
#endif
        {} //! A null entry is required to indicate the ending point
};

/* CLI handler to be called when no handler has been matched for the requested task. */
static void cli_default_handler(int argc, const char *argv[], void *user_data)
{
        DBG_PRINTF(DBG_NEW_LINE FG_LBLUE "Invalid command - retry..." DBG_NEW_LINE);
}

/* Main application task */
OS_TASK_FUNCTION(prvSleepModesTask, pvParameters)
{
        OS_BASE_TYPE ret;
        uint32_t notif;
        int8_t wdog_id;
        cli_t cli;

        app_task_h = OS_GET_CURRENT_TASK();

        wdog_id = sys_watchdog_register(false);

        /* This API should be called within the task of interest */
        cli = cli_register(APP_CLI_EVT_NOTIF, cmd_handlers, cli_default_handler);

        DBG_PRINTF(FG_LMAGENTA "\n\rEnter a valid command and press enter..." DBG_NEW_LINE);

#if APP_CLI_RTC_EN
        cli_rtc_alarm_register_cb(app_rtc_alarm_cb);
#endif
        cli_app_gpio0_register_cb(app_gpio0_cb);

        for (;;) {

                sys_watchdog_notify(wdog_id);
                /* Remove task id from the monitoring list as long as a task remains in the blocking state. */
                sys_watchdog_suspend(wdog_id);

                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                OS_ASSERT(ret == OS_OK);

                sys_watchdog_notify_and_resume(wdog_id);

                if (notif & APP_CLI_EVT_NOTIF) {
                      cli_handle_notified(cli);
                }

                if (notif & APP_WKUP_CTS_N_ACTIVE_NOTIF) {
                        DBG_PRINTF(FG_DEFAULT "CTS line asserted. Notifying the console task...");

                        console_wkup_handler();
                }

                if (notif & APP_RTC_ALARM_EVT_NOTIF) {
                        DBG_PRINTF(FG_DEFAULT "Alarm event hit...");
                }

                if (notif & APP_WKUP_CTS_N_INACTIVE_NOTIF) {
                        DBG_PRINTF(FG_DEFAULT "CTS line is de-asserted. Checking for deferred sleep states...");

#if APP_CLI_HIBERNATION_EN
                        if (cli_hibernation_get_status()) {
                                /*
                                 * Verify that no other processes prevent the device from entering
                                 * the selected sleep mode.
                                 */
                                if (pm_sleep_mode_get() == APP_DEFAULT_SLEEP_MODE) {
                                        pm_sleep_mode_set(pm_mode_hibernation);
                                } else {
                                        /*
                                         * This should be the case where the console task has lower
                                         * or equal priority to the application task and it has not
                                         * reached the point where CTS_N is read high (de-asserted)
                                         * so the UART adapter is closed and thus releasing the idle
                                         * state required when a serial bus is opened. Make sure that
                                         * CLI task has always higher priority by setting:
                                         *
                                         * #define CONSOLE_TASK_PRIORITY ( OS_TASK_PRIORITY_NORMAL + 1 )
                                         *
                                         */
                                        ASSERT_WARNING(0);
                                }
                        }
#endif /* APP_CLI_HIBERNATION_EN */
#if APP_CLI_DEEP_SLEEP_EN && APP_CLI_HIBERNATION_EN
                        else
#endif
#if APP_CLI_DEEP_SLEEP_EN
                        if (cli_deep_sleep_get_status()) {
                                /*
                                 * Verify that no other processes prevent the device from entering
                                 * the selected sleep mode.
                                 */
                                if (pm_sleep_mode_get() == APP_DEFAULT_SLEEP_MODE) {
                                        pm_sleep_mode_set(pm_mode_deep_sleep);
                                } else {
                                        /*
                                         * This should be the case where the console task has lower
                                         * or equal priority to the application task and it has not
                                         * reached the point where CTS_N is read high (de-asserted)
                                         * so the UART adapter is closed and thus releasing the idle
                                         * state required when a serial bus is opened. Make sure that
                                         * CLI task has always higher priority by setting:
                                         *
                                         * #define CONSOLE_TASK_PRIORITY ( OS_TASK_PRIORITY_NORMAL + 1 )
                                         *
                                         */
                                        ASSERT_WARNING(0);
                                }
                        }
#endif /* APP_CLI_DEEP_SLEEP_EN */
                }
        }
}
