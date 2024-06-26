/**
 ****************************************************************************************
 *
 * @file qdec_task.c
 *
 * @brief Quadrature decoder application task
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sys_power_mgr.h"
#include "sys_clock_mgr.h"
#include "sys_watchdog.h"
#include "misc.h"
#include "hw_wkup.h"
#include "hw_pdc.h"
#include "hw_quad.h"
#include "ad_i2c.h"
#include "sys_timer.h"
#include "platform_devices.h"
#include "knob_G_click_key.h"
#include "knob_G_click_led.h"
#include "knob_G_click_led_port.h"
#include "knob_G_click_config.h"

#define APP_QDEC_EVT_NOTIF                    ( 1 << 1 )
#define APP_KNOB_G_CLICK_KEY_PRESS_NOTIF      ( 1 << 2 )
#define APP_KNOB_G_CLICK_KEY_RELEASE_NOTIF    ( 1 << 3 )

/* Should reflect the number of different LED visual effects */
#define APP_KNOB_G_CLICK_MAX_KEY_STATES       ( 3 )

typedef hw_quad_handler_cb app_qdec_callback;

typedef struct {
        volatile hw_quad_values_t values;
        volatile uint8_t key_state;
} app_qdec_data_t;

typedef struct {
        hw_quad_config_t hw_qaud_cfg;
        uint8_t int_threshold;
} app_qdec_cfg_t;

__RETAINED_RW static app_qdec_cfg_t app_qdec_cfg = {
        .hw_qaud_cfg = {
                .clk_prescaler = HW_QUAD_REG_CLOCKDIV_PRESCALER_OFF,
                .clk_div = 0,
                .chz_event_mode = HW_QUAD_REG_CTRL2_CHANNEL_EVENT_MODE_NORMAL_CNT,
                .chy_event_mode = HW_QUAD_REG_CTRL2_CHANNEL_EVENT_MODE_NORMAL_CNT,
                .chx_event_mode = HW_QUAD_REG_CTRL2_CHANNEL_EVENT_MODE_NORMAL_CNT,
                .chz_port_sel = HW_QUAD_REG_INPUT_NONE,
                .chy_port_sel = HW_QUAD_REG_INPUT_NONE,
                .chx_port_sel = HW_QUAD_REG_INPUT_A_P008_B_P009
        },
        .int_threshold = 1 /* +1 */
};

__RETAINED static OS_TASK app_qdec_task_h;
__RETAINED static app_qdec_data_t app_qdec_data;

#if (KNOB_G_CLICK_LED_ENABLE == 1)
/* Context used to drive the PCA9956B LED driver */
__RETAINED_RW static knob_G_click_led_cfg_t app_knob_G_click_led_cfg = {
        .write = app_knob_G_click_led_write_reg,
#if ( 0 )
        .read = app_knob_G_click_led_read_reg,
#endif
        .handle = NULL
};
#endif

#if (KNOB_G_CLICK_KEY_ENABLE == 1)
static void app_kbob_G_click_key_callback(HW_GPIO_PORT port, uint32_t status)
{
        if ((port == KNOB_G_CLICK_KEY_PORT) && (status & (1 << KNOB_G_CLICK_KEY_PIN))) {
                HW_WKUP_TRIGGER trigger;

                trigger = hw_wkup_get_trigger(KNOB_G_CLICK_KEY_PORT, KNOB_G_CLICK_KEY_PIN);

                if (KNOB_G_CLICK_KEY_ACTIVE_LOW) {
                        OS_TASK_NOTIFY_FROM_ISR(app_qdec_task_h, trigger == HW_WKUP_TRIG_EDGE_LO?
                                APP_KNOB_G_CLICK_KEY_PRESS_NOTIF :
                                APP_KNOB_G_CLICK_KEY_RELEASE_NOTIF, OS_NOTIFY_SET_BITS);
                } else {
                        OS_TASK_NOTIFY_FROM_ISR(app_qdec_task_h, trigger == HW_WKUP_TRIG_EDGE_HI?
                                APP_KNOB_G_CLICK_KEY_PRESS_NOTIF :
                                APP_KNOB_G_CLICK_KEY_RELEASE_NOTIF, OS_NOTIFY_SET_BITS);
                }

                /* Invert trigger logic */
                hw_wkup_set_trigger(KNOB_G_CLICK_KEY_PORT, KNOB_G_CLICK_KEY_PIN, trigger == HW_WKUP_TRIG_EDGE_LO ?
                        HW_WKUP_TRIG_EDGE_HI : HW_WKUP_TRIG_EDGE_LO);
        }
}
#endif

/* Add a PDC entry so that the device can exit the sleep state upon QDEC events! */
static void app_qdec_pdc_register(void)
{
        uint32_t pdc_idx;

        pdc_idx = hw_pdc_add_entry(HW_PDC_LUT_ENTRY_VAL(HW_PDC_TRIG_SELECT_PERIPHERAL,
                                                        HW_PDC_PERIPH_TRIG_ID_TIMER3_QDEC,
                                                        HW_PDC_MASTER_CM33, 0));
        ASSERT_WARNING(pdc_idx != HW_PDC_INVALID_LUT_INDEX);

        hw_pdc_set_pending(pdc_idx);
        hw_pdc_acknowledge(pdc_idx);
}


static void app_qdec_cb(const volatile hw_quad_values_t *values, void *user_data)
{
        OS_TASK task = (OS_TASK)user_data;

        /* Copy the info provided by QDEC so it can be processed by the task upon exiting the ISR */
        app_qdec_data.values = *values;

        OS_TASK_NOTIFY_FROM_ISR(task, APP_QDEC_EVT_NOTIF, OS_NOTIFY_SET_BITS);
}

static void app_qdec_init(const app_qdec_cfg_t *cfg, app_qdec_callback cb, void *user_data)
{
        hw_quad_init(&cfg->hw_qaud_cfg);
        hw_quad_interrupt_enable_and_register(cb, cfg->int_threshold, user_data);
        hw_quad_start();
        app_qdec_pdc_register();
}

/* Main application task */
OS_TASK_FUNCTION(prvQdecTask, pvParameters)
{
        OS_BASE_TYPE ret;
        uint32_t notif;

        app_qdec_task_h = OS_GET_CURRENT_TASK();

        int8_t wdog_id = sys_watchdog_register(false);

#if (KNOB_G_CLICK_KEY_ENABLE == 1)
        /* Callback registration so that the task can be notified upon QDEC's KEY events */
        knob_G_click_key_register_cb(app_kbob_G_click_key_callback);
#endif

#if (KNOB_G_CLICK_LED_ENABLE == 1)
        ad_i2c_handle_t app_click_G_click_led_h = ad_i2c_open(KNOB_G_CLICK_LED_DEVICE);
        ASSERT_WARNING(app_click_G_click_led_h);

        app_knob_G_click_led_cfg.handle = app_click_G_click_led_h;
        knob_G_click_led_hw_reset(&app_knob_G_click_led_cfg);
#endif

        app_qdec_init(&app_qdec_cfg, app_qdec_cb, (void *)app_qdec_task_h);

        DBG_PRINTF(FG_LMAGENTA "\n\rEnter a valid command and press enter..." DBG_NEW_LINE);

        for (;;) {

                sys_watchdog_notify(wdog_id);
                /* Remove task id from the monitoring list as long as a task remains in the blocking state. */
                sys_watchdog_suspend(wdog_id);

                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                OS_ASSERT(ret == OS_OK);

                sys_watchdog_notify_and_resume(wdog_id);

#if (KNOB_G_CLICK_LED_ENABLE == 1)
                if (notif & APP_QDEC_EVT_NOTIF) {
                        bool direction = app_qdec_data.values.hw_quad_xcnt_delta < 0 ? false : true;

                        /* A QDEC event should have been taken place. Update LEDs positions based on the
                         * current LED state. */
                        app_knob_G_click_led_update(&app_knob_G_click_led_cfg,
                                                        app_qdec_data.key_state, direction);

                        DBG_PRINTF(FG_LMAGENTA DBG_NEW_LINE
                                "QDEC->X_RAW = %d, QDEC->X_DELTA = %d, EVENT COUNTER = %d" DBG_NEW_LINE,
                                app_qdec_data.values.hw_quad_xcnt_raw,
                                app_qdec_data.values.hw_quad_xcnt_delta,
                                app_qdec_data.values.hw_quad_event_cnt_raw);
                }
#endif

#if (KNOB_G_CLICK_KEY_ENABLE == 1)
                if (notif & APP_KNOB_G_CLICK_KEY_PRESS_NOTIF) {
                        /* Move onto the next state and wrap around when the max. num. of states is reached. */
                        app_qdec_data.key_state = (app_qdec_data.key_state + 1) % APP_KNOB_G_CLICK_MAX_KEY_STATES;

#if (KNOB_G_CLICK_LED_ENABLE == 1)
                        /* LED state should have been changed. Turn off all LEDs. */
                        knob_G_click_led_set_brightness(&app_knob_G_click_led_cfg, KNOB_G_CLICK_LED_ALL, KNOB_G_CLICK_LED_OFF);
#endif

                        DBG_PRINTF(FG_LCYAN DBG_NEW_LINE "QDEC KEY press event. Key state = %d" DBG_NEW_LINE,
                                app_qdec_data.key_state);
                }

                if (notif & APP_KNOB_G_CLICK_KEY_RELEASE_NOTIF) {
                        DBG_PRINTF(FG_LCYAN DBG_NEW_LINE "QDEC KEY release event..." DBG_NEW_LINE);
                }
#endif
        }
}
