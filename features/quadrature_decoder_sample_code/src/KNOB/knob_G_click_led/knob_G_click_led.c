/**
 ****************************************************************************************
 *
 * @file knob_G_click_led.c
 *
 * @brief Source file to support the PCA9956B LED driver of KNOB G CLICK.
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
#include "osal.h"
#include "knob_G_click_led.h"
#include "knob_G_click_config.h"

#if (KNOB_G_CLICK_LED_ENABLE == 1)

#define KNOB_REG_BRIGHTNESS_CONTROL_FOR_ALL_LED   0x3F
#define KNOB_REG_GAIN_CONTROL_FOR_ALL_LED         0x40

static int knob_G_click_led_write_reg(knob_G_click_led_cfg_t *ctx, uint8_t reg, const uint8_t *wbuf, size_t len)
{
        int ret;

        ret = ctx->write(ctx->handle, reg, wbuf, len);

        return ret;
}

#if ( 0 )
static int knob_G_click_led_read_reg(knob_G_click_led_cfg_t *ctx, uint8_t reg, uint8_t *rbuf, size_t len)
{
        int ret;

        ret = ctx->read(ctx->handle, reg, rbuf, len);

        return ret;
}
#endif

void knob_G_click_led_turn_on(knob_G_click_led_cfg_t *ctx, KNOB_G_CLICK_LED led)
{
        ASSERT_WARNING(led <= KNOB_G_CLICK_LED_MAX);

        uint8_t value = KNOB_G_CLICK_LED_ON;

        knob_G_click_led_write_reg(ctx, 0x09 + led, &value, 1);
}

void knob_G_click_led_turn_off(knob_G_click_led_cfg_t *ctx, KNOB_G_CLICK_LED led)
{
        ASSERT_WARNING(led <= KNOB_G_CLICK_LED_MAX);

        uint8_t value = KNOB_G_CLICK_LED_OFF;

        knob_G_click_led_write_reg(ctx, 0x09 + led, &value, 1);
}

void knob_G_click_led_set_brightness(knob_G_click_led_cfg_t *ctx, KNOB_G_CLICK_LED led, uint8_t duty_cycle)
{
        if (led == KNOB_G_CLICK_LED_ALL) {
                knob_G_click_led_write_reg(ctx, KNOB_REG_BRIGHTNESS_CONTROL_FOR_ALL_LED, &duty_cycle, 1);
        } else {
                ASSERT_WARNING(led <= KNOB_G_CLICK_LED_MAX);

                knob_G_click_led_write_reg(ctx, 0x09 + led, &duty_cycle, 1);
        }
}

void knob_G_click_led_set_gain(knob_G_click_led_cfg_t *ctx, KNOB_G_CLICK_LED led, uint8_t gain)
{
        if (led == KNOB_G_CLICK_LED_ALL) {
                knob_G_click_led_write_reg(ctx, KNOB_REG_GAIN_CONTROL_FOR_ALL_LED, &gain, 1);
        } else {
                ASSERT_WARNING(led <= KNOB_G_CLICK_LED_MAX);

                knob_G_click_led_write_reg(ctx, 0x21 + led, &gain, 1);
        }
}

void knob_G_click_led_hw_reset(knob_G_click_led_cfg_t *ctx)
{
        DBG_IO_OFF(KNOB_G_CLICK_RST_PORT, KNOB_G_CLICK_RST_PIN);
        OS_DELAY_MS(10);
        DBG_IO_ON(KNOB_G_CLICK_RST_PORT, KNOB_G_CLICK_RST_PIN);
        OS_DELAY_MS(10);

        knob_G_click_led_set_gain(ctx, KNOB_G_CLICK_LED_ALL, 0xFF);
}

#endif

