/**
 ****************************************************************************************
 *
 * @file rtc.h
 *
 * @brief RCT block demonstration
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

#ifndef RTC_H_
#define RTC_H_

#include "hw_rtc.h"

/*********************************************************************
 *
 *       Typedefs
 *
 *********************************************************************
 */

/* RTC second event callback function */
typedef void (* _rtc_sec_event_callback)(void);

/* RTC alarm event callback function */
typedef void (* _rtc_alarm_event_callback)(void);

/* RTC status enumeration */
typedef enum {
        RTC_STATUS_FAIL   = -1,
        RTC_STATUS_OK
} RTC_STATUS;

/*********************************************************************
 *
 *       Defines
 *
 *********************************************************************
 */

/*
 * '1' to set the 24H display mode,
 * '0' to set the 12H display mode.
 */
#define RTC_DISPLAY_MODE_24H    1

/* Initial time values */
#define RTC_SECOND              0
#define RTC_MINUTE             58
#define RTC_HOUR               14

/* Initial date values */
#define RTC_MONTH               6
#define RTC_MONTH_DAY          26
#define RTC_WEEK_DAY            3
#define RTC_YEAR             2023

/*********************************************************************
 *
 *       API
 *
 *********************************************************************
 */

/*
 * \brief RTC Initialization
 *
 * This routine should be called at the very beginning and before performing
 * any RTC operations.
 *
 * \return The status of the target operation
 *
 * \sa RTC_STATUS_OK
 * \sa RTC_STATUS_FAIL
 */
int _rtc_init(void);

/**
 * \brief Get current date/calendar
 *
 * Wrapper function to get the current RTC counter values (time/date).
 *
 * \param[out]  time_cfg       Pointer to time structure to be filled
 * \param[out]  calendar_cfg   Pointer to calendar structure to be filled
 *
 * \sa hw_rtc_get_time_clndr()
 *
 */
void _rtc_get_value(hw_rtc_time_t * time_cfg, hw_rtc_calendar_t * calendar_cfg);

/*
 * \brief Set alarm event
 *
 * Wrapper function to set alarm event easier. To get notification, and once the alarm hits,
 * a callback routine should first be registered.
 *
 * \param[in] alarm_time_cfg      Pointer to alarm time structure
 * \param[in] alarm_calendar_cfg  Pointer to alarm calendar structure
 *
 * \return Status of the target operation
 *
 * \sa RTC_STATUS_OK
 * \sa RTC_STATUS_FAIL
 *
 * \sa _rtc_register_alarm_event_cb()
 * \sa _rtc_unregister_alarm_event_cb()
 * \sa hw_rtc_set_alarm()
 *
 */
int _rtc_set_alarm(hw_rtc_time_t * alarm_time_cfg, hw_rtc_alarm_calendar_t * alarm_calendar_cfg);

/**
 * \brief Callback function registration for second events
 *
 * The callback routine registered will be called following an RTC event.
 * Currently, every second event.
 *
 * \param[in] cb  Callback function to register.
 *
 * \note The task can get the current RTC counter values by calling
 *       \sa _rtc_get_log()
 *
 */
void _rtc_register_sec_event_cb(_rtc_sec_event_callback cb);

/**
 * \brief Unregister callback function for second events
 *
 * Calling this routine will result in getting no second events notifications.
 *
 */
void _rtc_unregister_sec_event_cb( void );

/**
 * \brief Callback function registration for alarm events
 *
 * The callback routine registered will be called following an alarm event
 * (as defined by the application).
 *
 * \param[in] cb  Callback function to register
 */
void _rtc_register_alarm_event_cb(_rtc_alarm_event_callback cb);

/**
 * \brief Unregister callback function for alarm events
 *
 * Calling this routine will result in getting no alarm events notifications.
 *
 */
void _rtc_unregister_alarm_event_cb(void);

#endif /* RTC_H_ */
