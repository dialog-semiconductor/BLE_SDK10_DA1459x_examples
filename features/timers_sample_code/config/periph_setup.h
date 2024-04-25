/**
 ****************************************************************************************
 *
 * @file periph_setup.h
 *
 * @brief Configuration of devices connected to board.
 *
 * Copyright (C) 2016-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef PERIPH_SETUP_H
#define PERIPH_SETUP_H

#include "platform_devices.h"

#define APP_TIMER_CAPTURE_EN  1

/*
 * PWM functionality is available in sleep mode on the following two pins:
 *      - P0_12 if the selected timer id is HW_TIMER
 *      - P0_10 if the selected timer id is HW_TIMER2
 */
#define APP_TIMER_PWM_PORT  HW_GPIO_PORT_0
#define APP_TIMER_PWM_PIN   HW_GPIO_PIN_12

/* Counter's interrupt frequency. A callback function should be called following interrupt assertion.  */
#define APP_TIMER_COUNTER_RELOAD_HZ    10

/*
 * Timer ID for the HW timer instance used. The device integrates four block instances in total.
 * Each instance consists of a counter and a PWM sub-blocks.
 * Note that HW_TIMER2 is reserved by SDK for the OS tick generation using the low power clock (RCX).
 * Therefore it's counter sub-block cannot be modified. Only the PWM sub-block is allowed to be
 * utilized by developers.
 * It's worth mentioning that TIMER4 resides in PD_COM in contrast to the rest of the instances that
 * reside in PD_TIM which is kept enabled even in the sleep state. as HW_TIMER2 is used to support the
 * clock-less sleep mode. Please note that only HW_TIMER can generate interrupts upon capture events.
 */
#define APP_TIMER_ID    HW_TIMER

/* Select the port/pin used to capture events. Here the duty cycle of a PWM pulse is measured
 * and so a single port is selected. */
#define APP_TIMER_CAPTURE_PORT   HW_TIMER_GPIO_PIN_P0_10

/*
 * HW_TIMER timer can support up to four input events can be captured on a single or multiple GPIO whilst
 * the rest instances can support up to two capture channels. The capture channels are refereed to as
 * GPIO1, GPIO2, GPIO3 and GPIO4. As mentioned above, only the capture channels of HW_TIMER can assert a
 * dedicated interrupt line upon an event of any of the four capture channels.
 * In this example two of the four capture channels are used to measure the duty-cycle of a PWM pulse.
 *
 * A value of 1 will result in capturing GPIO1 on the rising edge of the incoming input pulse
 * whilst GPIO2 on the falling edge. Thus, measuring the duty cycle ON of the triggering pulse.
 *
 *  GPIO1                  GPIO2
 *      _____________________               __________________
 *     |                     |             |
 *  ___|          ON         |_____OFF_____|
 *
 *
 * A value of 0 will result in capturing GPIO1 on the falling edge of the incoming input pulse
 * whilst GPIO2 on the rising edge. Thus, measuring the duty cycle OFF of the triggering pulse.
 *
 *                        GPIO1         GPIO2
 *      _____________________               __________________
 *     |                     |             |
 *  ___|         ON          |_____OFF_____|
 *
 */
#define APP_TIMER_CAPTURE_EDGE     1

#endif /*PERIPH_SETUP_H*/
