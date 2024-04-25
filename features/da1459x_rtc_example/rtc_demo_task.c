/**
 ****************************************************************************************
 *
 * @file rtc_demo_task.c
 *
 * @brief RTC Task
 *
* Copyright (C) 2020-2023 Renesas Electronics Corporation and/or its affiliates.
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

#include <stdio.h>
#include "osal.h"
#include "rtc.h"
#include "sys_watchdog.h"

/*********************************************************************
 *
 *       Defines/macros
 *
 *********************************************************************
 */

/* Notification bit-masks */
#define NOTIF_RTC_SEC_EVENT     ( 1 << 1 )
#define NOTIF_RTC_ALARM_EVENT   ( 1 << 2 )
#define NOTIF_SET_ALARM_EVENT   ( 1 << 3 )

/* Alarm timer configuration timeout */
#define SET_ALARM_EVENT_TIMEOUT_MS    ( 5000 )

/*
 * '1' to enable printing debugging info on the serial console,
 * '0' otherwise.
 */
#define DBG_SERIAL_CONSOLE      ( 1 )

#define _NEWLINE_    "\n\r"
#define _VERBOSE_(state, format, args...)                      \
                                                               \
        if (state) {                                           \
                printf(_NEWLINE_ #format _NEWLINE_, ##args);   \
        }

/*********************************************************************
 *
 *       Retained symbols
 *
 *********************************************************************
 */

__RETAINED_RW static OS_TASK task_handle = NULL;

/*********************************************************************
 *
 *       Static code
 *
 *********************************************************************
 */

/* Alarm timer callback function */
static void _alarm_timer_cb(OS_TIMER timer)
{
        /* Notify task to further process the event */
        OS_TASK_NOTIFY(task_handle, NOTIF_SET_ALARM_EVENT, OS_NOTIFY_SET_BITS);
}

/* Callback function called following a second RTC event */
static void _rtc_sec_event_cb(void)
{
        (in_interrupt()) ?
                OS_TASK_NOTIFY_FROM_ISR(task_handle, NOTIF_RTC_SEC_EVENT, OS_NOTIFY_SET_BITS) :
                                        OS_TASK_NOTIFY(task_handle, NOTIF_RTC_SEC_EVENT, OS_NOTIFY_SET_BITS);
}

static void _rtc_alarm_event_cb(void)
{
        (in_interrupt()) ?
                OS_TASK_NOTIFY_FROM_ISR(task_handle, NOTIF_RTC_ALARM_EVENT, OS_NOTIFY_SET_BITS) :
                                        OS_TASK_NOTIFY(task_handle, NOTIF_RTC_ALARM_EVENT, OS_NOTIFY_SET_BITS);
}

/*********************************************************************
 *
 *       Non static code
 *
 *********************************************************************
 */

/* RTC demo task */
OS_TASK_FUNCTION(prvRTCTask, params)
{
        OS_BASE_TYPE ret;
        OS_TIMER alarm_timer_handle;
        uint32_t notif;
        hw_rtc_time_t time_info;
        hw_rtc_calendar_t calendar_info;

#if dg_configUSE_WDOG
        int8_t wdog_id;
        /* Register rtc_task to be monitored by watchdog */
        wdog_id = sys_watchdog_register(false);
#endif

        _VERBOSE_(DBG_SERIAL_CONSOLE, ***** RTC Demonstration ***)

        task_handle = OS_GET_CURRENT_TASK();

        ret = _rtc_init();
        ASSERT_WARNING(ret == RTC_STATUS_OK);

        /* Register callback function to be called following RTC second events. */
        _rtc_register_sec_event_cb(_rtc_sec_event_cb);
        /* Register callback function to be called following RTC alarm events. */
        _rtc_register_alarm_event_cb(_rtc_alarm_event_cb);

        /* Create an one-shot OS timer used to set alarm events */
        alarm_timer_handle = OS_TIMER_CREATE("ALARM EVENT", OS_MS_2_TICKS(SET_ALARM_EVENT_TIMEOUT_MS),
                                                                                OS_TIMER_FAIL, NULL, _alarm_timer_cb);
        ASSERT_WARNING(alarm_timer_handle != NULL);
        OS_TIMER_START(alarm_timer_handle, OS_TIMER_FOREVER);

        for (;;) {

#if dg_configUSE_WDOG
                /* Notify watchdog on each loop */
                sys_watchdog_notify(wdog_id);

                /* Suspend watchdog while blocking on OS_TASK_NOTIFY_WAIT() */
                sys_watchdog_suspend(wdog_id);
#endif
                /* Block task as long as events are not present */
                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                ASSERT_WARNING(ret == OS_TASK_NOTIFY_SUCCESS);

#if dg_configUSE_WDOG
                /* Resume watchdog */
                sys_watchdog_notify_and_resume(wdog_id);
#endif

                if (notif & NOTIF_RTC_SEC_EVENT) {

                        /* Get the current RTC contents */
                        _rtc_get_value(&time_info, &calendar_info);

                        _VERBOSE_(DBG_SERIAL_CONSOLE, Weekday:%02d Date:%02d-%02d-%02d Time:%02d:%02d:%02d,
                                                        calendar_info.wday, calendar_info.mday,
                                                        calendar_info.month, calendar_info.year,
                                                        time_info.hour, time_info.minute, time_info.sec);
                }
                if (notif & NOTIF_RTC_ALARM_EVENT) {
                        _VERBOSE_(DBG_SERIAL_CONSOLE, Alarm event hit!)
                }
                if (notif & NOTIF_SET_ALARM_EVENT) {
                        int status;

                        /* Get the current RTC contents */
                        _rtc_get_value(&time_info, &calendar_info);

                        /*
                         * Set alarm event based on the current RTC contents.
                         * Here, an alarm event is set in one minute.
                         */
                        time_info.minute += 1;

                        /* Set the alarm event */
                        status = _rtc_set_alarm(&time_info, NULL);
                        ASSERT_WARNING(status == RTC_STATUS_OK);

                        _VERBOSE_(DBG_SERIAL_CONSOLE, Setting alarm event in one minute...)
                }
        }
}
