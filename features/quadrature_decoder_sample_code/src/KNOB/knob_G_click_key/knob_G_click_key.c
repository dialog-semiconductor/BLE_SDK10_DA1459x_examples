/**
 ****************************************************************************************
 *
 * @file knob_G_click_key.c
 *
 * @brief Source file to support KEY (push button) events of KNOB G CLICK.
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

#include "misc.h"
#include "hw_wkup.h"
#include "hw_pdc.h"
#include "sys_power_mgr.h"
#include "knob_G_click_key.h"
#include "knob_G_click_config.h"

#if (KNOB_G_CLICK_KEY_ENABLE == 1)

__RETAINED static knob_G_click_key_callback_t callback;

void knob_G_click_key_register_cb(knob_G_click_key_callback_t user_cb)
{
        callback = user_cb;
}

static void knob_G_click_key_cb(HW_GPIO_PORT port)
{
        uint32_t status;

        /* Get the status of the last wake-up event */
        status = hw_wkup_get_gpio_status(port);

        /* User should process the KEY event */
        if (callback) {
                callback(port, status);
        }

        /* This function must be called so the status register is cleared!!! */
        hw_wkup_clear_gpio_status(port, status);
}

#if (QDEC_KEY_PORT == HW_GPIO_PORT_0)
static void knob_G_click_key_p0_cb(void)
{
        knob_G_click_key_cb(HW_GPIO_PORT_0);
}
#else
static void knob_G_click_key_p1_cb(void)
{
        knob_G_click_key_cb(HW_GPIO_PORT_1);
}
#endif

static uint32_t knob_G_click_key_add_pdc_entry(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
        uint32_t pdc_idx;

        pdc_idx = hw_pdc_add_entry(HW_PDC_LUT_ENTRY_VAL(port, pin, HW_PDC_MASTER_CM33, 0));
        ASSERT_WARNING(pdc_idx != HW_PDC_INVALID_LUT_INDEX);

        hw_pdc_set_pending(pdc_idx);
        hw_pdc_acknowledge(pdc_idx);

        return pdc_idx;
}

static void knob_G_click_key_init(void *data)
{
        (void)data;

        /* Initialize the WKUP controller */
        hw_wkup_init(NULL);

        /*
         * The WKUP should initially be configured based on the selected active edge.
         * User can then toggle the active edge within the registered callback.
         */
        hw_wkup_set_trigger(KNOB_G_CLICK_KEY_PORT, KNOB_G_CLICK_KEY_PIN,
                KNOB_G_CLICK_KEY_ACTIVE_LOW ? HW_WKUP_TRIG_EDGE_LO : HW_WKUP_TRIG_EDGE_HI);

#if (QDEC_KEY_PORT == HW_GPIO_PORT_0)
        hw_wkup_register_gpio_p0_interrupt(knob_G_click_key_p0_cb, 1);
#else
        hw_wkup_register_gpio_p1_interrupt(knob_G_click_key_p1_cb, 1);
#endif

        /* Enable interrupts on WKUP controller */
        hw_wkup_enable_key_irq();

        knob_G_click_key_add_pdc_entry(KNOB_G_CLICK_KEY_PORT, KNOB_G_CLICK_KEY_PIN);
}

DEVICE_INIT(app_qdec_key, knob_G_click_key_init, NULL);

#endif
