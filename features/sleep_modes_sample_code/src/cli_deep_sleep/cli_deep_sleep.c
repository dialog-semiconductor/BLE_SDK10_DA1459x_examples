/**
 ****************************************************************************************
 *
 * @file cli_deep_sleep.c
 *
 * @brief Deep sleep shell operations
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "hw_pdc.h"
#include "cli_deep_sleep.h"
#include "cli_app.h"
#include "cli_app_config.h"
#include "sys_power_mgr.h"
#include "hw_wkup.h"

#if APP_CLI_DEEP_SLEEP_EN
__RETAINED static bool deep_sleep_status;
__RETAINED_RW static uint32_t deep_sleep_2d_pdc_idx[HW_GPIO_PORT_MAX][HW_GPIO_PIN_MAX];

bool cli_deep_sleep_get_status(void)
{
        return deep_sleep_status;
}

void cli_deep_sleep_get_info_handler(int argc, const char *argv[], void *user_data)
{
        for (int i = 0; i < HW_GPIO_PORT_MAX; i++) {
                for (int j = 0; j < hw_gpio_port_num_pins[i]; j++) {
                        if (deep_sleep_2d_pdc_idx[i][j] != HW_PDC_INVALID_LUT_INDEX) {
                                DBG_PRINTF(FG_GREEN "@ P%d.%d is used as wake-up source" DBG_NEW_LINE, i, j);
                        }
                }
        }
        DBG_PRINTF(FG_GREEN "Status: %s" DBG_NEW_LINE, deep_sleep_status ? "enabled" : "disabled");
}

void cli_deep_sleep_set_status_handler(int argc, const char *argv[], void *user_data)
{
        if (argc != 2) {
                DBG_PRINTF(FG_LRED "Invalid syntax - deep_sleep_set_status <status>, where"
                        DBG_NEW_LINE FG_GREEN " * <status>: <enable> or <disable>");
                return;
        }

        if (!strcmp("enable", argv[1])) {
                deep_sleep_status = true;
        } else if (!strcmp("disable", argv[1])) {
                deep_sleep_status = false;
        } else {
                DBG_PRINTF(FG_RED "Invalid status selection...");
        }
        DBG_PRINTF(DBG_NEW_LINE);
}

void cli_deep_sleep_select_port_handler(int argc, const char *argv[], void *user_data)
{
        HW_GPIO_PORT port;
        HW_GPIO_PIN pin;
        bool pol;

        if (argc != 4) {
                DBG_PRINTF(FG_LRED "Invalid syntax - deep_sleep_sel_port <port> <pin> <pol>, where "
                        DBG_NEW_LINE FG_GREEN " * <port>: <p0> or <p1>"
                        DBG_NEW_LINE FG_GREEN " * <pin>: A valid pin number from [0..15]"
                        DBG_NEW_LINE FG_GREEN " * <pol>: <active_low> or <active_high>" DBG_NEW_LINE);
                return;
        }

        if (!strcmp("p0", argv[1])) {
                port = 0;
        } else if (!strcmp("p1", argv[1])) {
                port = 1;
        } else {
                port = HW_GPIO_PORT_MAX;
        }

        pin = atoi(argv[2]);

        if (!strcmp("active_low", argv[3])) {
                pol = false;
        } else if (!strcmp("active_high", argv[3])) {
                pol = true;
        } else {
                DBG_PRINTF(FG_RED "Invalid polarity selection..." DBG_NEW_LINE);
                return;
        }

        if (port >= HW_GPIO_PORT_MAX || pin >= hw_gpio_port_num_pins[port]) {
                DBG_PRINTF(FG_CYAN "Invalid GPIO selection..." DBG_NEW_LINE);
                return;
        }

        /* First, try to acquire a valid PDC entry TODO: XXX Check if this is required for deep sleep */
        if (deep_sleep_2d_pdc_idx[port][pin] == HW_PDC_INVALID_LUT_INDEX) {
                deep_sleep_2d_pdc_idx[port][pin] = cli_app_gpio_pdc_entry_add(port, pin);

                if (deep_sleep_2d_pdc_idx[port][pin] == HW_PDC_INVALID_LUT_INDEX) {
                        DBG_PRINTF(FG_RED "Unable to get a PDC entry. "
                                "The selected port cannot be used to exit the deep slate" DBG_NEW_LINE);
                        return;
                }
        }

        /* One or multiple pins can be selected as wake-up sources */
        /* Multiplex the selected pin a GPIO input */
        hw_gpio_set_pin_function(port, pin,
                pol ? HW_GPIO_MODE_INPUT_PULLDOWN : HW_GPIO_MODE_INPUT_PULLUP, HW_GPIO_FUNC_GPIO);
        hw_gpio_pad_latch_enable(port, pin);
        hw_gpio_pad_latch_disable(port, pin);

        hw_wkup_set_trigger(port, pin, pol ? HW_WKUP_TRIG_EDGE_HI : HW_WKUP_TRIG_EDGE_LO);

        DBG_PRINTF(DBG_NEW_LINE);
}

void cli_deep_sleep_reset_port_handler(int argc, const char *argv[], void *user_data)
{
        HW_GPIO_PORT port;
        HW_GPIO_PIN pin;

        if (argc != 3) {
                DBG_PRINTF(FG_LRED "Invalid syntax - deep_sleep_reset_port <port> <pin>, where "
                        DBG_NEW_LINE FG_GREEN " * <port>: <p0> or <p1>"
                        DBG_NEW_LINE FG_GREEN " * <pin>: A valid pin number from [0..16]" DBG_NEW_LINE);
                return;
        }

        if (!strcmp("p0", argv[1])) {
                port = 0;
        } else if (!strcmp("p1", argv[1])) {
                port = 1;
        } else {
                port = HW_GPIO_PORT_MAX;
        }

        pin = atoi(argv[2]);

        if (port >= HW_GPIO_PORT_MAX || pin >= hw_gpio_port_num_pins[port]) {
                DBG_PRINTF(FG_CYAN "Invalid GPIO selection..." DBG_NEW_LINE);
                return;
        }

        /* First, remove the associated PDC entry, if any available TODO: XXX Check if this is required for deep sleep */
        if (deep_sleep_2d_pdc_idx[port][pin] != HW_PDC_INVALID_LUT_INDEX) {
                cli_app_gpio_pdc_entry_remove(deep_sleep_2d_pdc_idx[port][pin]);

                deep_sleep_2d_pdc_idx[port][pin] = HW_PDC_INVALID_LUT_INDEX;

                /* Invalidate port selection */
                hw_gpio_set_pin_function(port, pin, HW_GPIO_MODE_INVALID, HW_GPIO_FUNC_LAST);
                hw_gpio_pad_latch_enable(port, pin);
                hw_gpio_pad_latch_disable(port, pin);
        } else {
                DBG_PRINTF(FG_RED "P%d.%d was never set before - aborting..." DBG_NEW_LINE, port, pin);
        }
        DBG_PRINTF(DBG_NEW_LINE);
}

void cli_deep_sleep_init(void)
{
        for (int i = 0; i < HW_GPIO_PORT_MAX; i++) {
                for (int j = 0; j < HW_GPIO_PIN_MAX; j++) {
                        deep_sleep_2d_pdc_idx[i][j] = HW_PDC_INVALID_LUT_INDEX;
                }
        }
}

DEVICE_INIT(cli_deep_sleep, cli_deep_sleep_init, NULL);

#endif /* APP_CLI_DEEP_SLEEP_EN */
