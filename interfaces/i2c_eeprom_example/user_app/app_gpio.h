/**
 ****************************************************************************************
 *
 * @file app_gpio.h
 *
 * @brief Basic framework to handle I/O pins outside adapter context.
 *
 * Copyright (C) 2017-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef APP_GPIO_H_
#define APP_GPIO_H_

#include "ad.h"

/*
 * There are two approaches to directly handle GPIO pins in application context:
 *
 * Statically: In this approach certain actions are performed regardless of whether or not
 *             one or multiple pins should be driven to a specific state. in specific:
 *
 * When the system exits the sleep state `app_gpio_set_active` is called and the following
 * operations are executed:
 *
 * 1. `PD_COM` is enabled (if not handled transparently by SDK).
 * 2. GPIO pins of interest are configured and driven to their ON state.
 * 3. Associated pads are un-locked so their current state can be reflected on the output.
 * 4. The state of the previously configured pins can now be driven by accessing the low
 *    level driver API.
 *
 * When the system is about to enter the sleep state `app_gpio_set_inactive` is called and
 * the following operations are executed:
 *
 * 1. GPIO pins of interest are configured and driven to their OFF state.
 * 2. Associated pads are locked so their current state can be retained while in the sleep state.
 * 3. `PD_COM` is disabled (if not handled transparently by SDK).
 *
 * For this to work, user callback functions should be registered to the power manager (PM) by calling
 * `pm_register_adapter`. In doing so, the running application can get notified upon certain events
 * (e.g. exiting sleep, entering sleep, canceling sleep)
 *
 * Dynamically: The following actions take place every a time one or multiple GPIO pins should be driven
 * to a specific state by calling either `app_gpio_set_active`or `app_gpio_set_inactive`. In specific:
 *
 *   1. `PD_COM` is enabled (if not handled transparently by SDK)
 *   2. GPIO pins are configured and driven to a specific state (ON/OFF)
 *   3. Associated pads are enabled and then locked so their state can be retained when the device enters
 *      the sleep state.
 *   4. `PD_COM` is disabled (if not handled transparently by SDK).
 *
 **/
#ifndef APP_GPIO_DYNAMIC_MODE
#define APP_GPIO_DYNAMIC_MODE       ( 1 )
#endif

/*
 * Configure a list of GPIO pins to the ON state. The following actions are performed:
 *
 * 1. If not activated by SDK, the communication power domain (PD_COM), where the GPIO controller
 *    resides in, is enabled.
 * 2. GPIO pins are configured to their ON state.
 * 3. Their associated pads are latched/un-locked so their new state can be reflected.
 *    In dynamic mode, pads are also un-latched/locked so their state is retained when device enters
 *    the sleep state (otherwise pins should be reset to input pull-down mode).
 * 4. In dynamic mode, we make sure that PD_COM is disabled and given that SDK does not handle
 *    that domain transparently.
 *
 * \param [in] io    An array of GPIO pins definitions
 *
 * \param [in] size  Number of entries of of \p io.
 *
 */
void app_gpio_set_active(const ad_io_conf_t *io, uint8_t size);

/*
 * Configure a list of GPIO pins to the OFF state. The following actions are performed:
 *
 * 1. If not activated by SDK and when in dynamic mode, we make sure that PD_COM is enabled.
 * 2. GPIO pins are configured to their OFF state.
 * 3. Their associated pads are un-latched/locked so their state can be retained when the device enters
 *    the sleep state.
 *    In dynamic mode, pads are first latched/un-locked and then un-latched/locked.
 * 4. If not handled by SDK, PDC_COM is disabled.
 *
 * \param [in] io    An array of GPIO pins definitions
 *
 * \param [in] size  Number of entries of of \p io.
 *
 */
void app_gpio_set_inactive(const ad_io_conf_t *io, uint8_t size);

#endif /* APP_GPIO_H_ */
