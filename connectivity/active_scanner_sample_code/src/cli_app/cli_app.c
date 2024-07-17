/**
 ****************************************************************************************
 *
 * @file cli_app.c
 *
 * @brief CLI APP operations
 *
 * Copyright (C) 2015-2024 Renesas Electronics Corporation and/or its affiliates.
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

#include "cli_app.h"
#include "sys_power_mgr.h"
#include "cli.h"

__RETAINED static cli_app_gpio0_callback cli_gpio0_cb;

static void app_wkup_gpio_p0_cb(void)
{
        uint32_t status;

        /* Get the status of the last wake-up event */
        status = hw_wkup_get_gpio_status(HW_GPIO_PORT_0);

        if (cli_gpio0_cb) {
                cli_gpio0_cb(status);
        }

        /* This function must be called so the status register is cleared!!! */
        hw_wkup_clear_gpio_status(HW_GPIO_PORT_0, status);
}

void cli_app_gpio0_register_cb(cli_app_gpio0_callback cb)
{
        cli_gpio0_cb = cb;
}

static uint32_t cli_app_gpio_pdc_entry_add(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
        uint32_t pdc_idx;

        pdc_idx = hw_pdc_add_entry(HW_PDC_LUT_ENTRY_VAL(port, pin, HW_PDC_MASTER_CM33, 0));
        ASSERT_WARNING(pdc_idx != HW_PDC_INVALID_LUT_INDEX);

        hw_pdc_set_pending(pdc_idx);
        hw_pdc_acknowledge(pdc_idx);

        return pdc_idx;
}

static void cli_wkup_init(void)
{
        /* Initialize the WKUP controller */
        hw_wkup_init(NULL);

#if dg_configUSE_CONSOLE
        /*
         * Configure CTS line's trigger polarity and mode based on the current state
         * as it might happen that the serial console is not enabled by the time
         * this routine is called.
         */
        hw_wkup_set_trigger(SER1_CTS_PORT, SER1_CTS_PIN,
                hw_gpio_get_pin_status(SER1_CTS_PORT, SER1_CTS_PIN) ?
                HW_WKUP_TRIG_EDGE_LO : HW_WKUP_TRIG_EDGE_HI);
#endif

        /* CTS_N is connected to P0.11 */
        hw_wkup_register_gpio_p0_interrupt(app_wkup_gpio_p0_cb, 1);

        /* Enable interrupts on WKUP controller */
        hw_wkup_enable_key_irq();

#if dg_configUSE_CONSOLE
        cli_app_gpio_pdc_entry_add(SER1_CTS_PORT, SER1_CTS_PIN);
#endif
}

static void cli_app_init(void)
{
        /* When the CONSOLE/CLI modules are enabled and because of the UART adapter invocation,
         * the device is only allowed to enter the idle sleep state (low-power WFI ARM state). */
        cli_init();
        cli_wkup_init();
}

DEVICE_INIT(cli_app, cli_app_init, NULL);
