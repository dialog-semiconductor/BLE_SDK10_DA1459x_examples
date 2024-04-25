/**
 ****************************************************************************************
 *
 * @file cli_rtc.h
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

#ifndef CLI_RTC_H_
#define CLI_RTC_H_

#include "hw_rtc.h"

#ifndef APP_RTC_MODE_24H
#define APP_RTC_MODE_24H    ( 1 )
#endif

#ifndef APP_RTC_12H_PM
#define APP_RTC_12H_PM  ( 0 )
#endif

#ifndef APP_RTC_HSEC
#define APP_RTC_HSEC    ( 0 )
#endif

#ifndef APP_RTC_SEC
#define APP_RTC_SEC     ( 0 )
#endif

#ifndef APP_RTC_MIN
#define APP_RTC_MIN     ( 0 )
#endif

#ifndef APP_RTC_HOUR
#define APP_RTC_HOUR    ( 0 )
#endif

#ifndef APP_RTC_MONTH
#define APP_RTC_MONTH   ( 1 )
#endif

#ifndef APP_RTC_MDAY
#define APP_RTC_MDAY    ( 1 )
#endif

#ifndef APP_RTC_YEAR
#define APP_RTC_YEAR    ( 2024 )
#endif

#ifndef APP_RTC_WDAY
#define APP_RTC_WDAY    ( 1 )
#endif

typedef void (*cli_rtc_alarm_callback)(void);

void cli_rtc_time_get_handler(int argc, const char *argv[], void *user_data);
void cli_rtc_time_set_handler(int argc, const char *argv[], void *user_data);
void cli_rtc_alarm_set_handler(int argc, const char *argv[], void *user_data);
void cli_rtc_alarm_get_handler(int argc, const char *argv[], void *user_data);

/*
 * User callback function to be called upon alarm event
 *
 * \param [in] cb User callback function to be called upon alarm event.
 *                Setting to NULL will result in unregistering any
 *                previous callback function.
 *
 * \note The callback function will be called within ISR interrupt and so
 *       callback should be as fast as possible. In case of tasks, these
 *       should be deferred by sending notifications to the application
 *       task.
 */
void cli_rtc_alarm_register_cb(cli_rtc_alarm_callback cb);

#endif /* CLI_RTC_H_ */
