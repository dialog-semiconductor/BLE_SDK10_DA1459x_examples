/**
 ****************************************************************************************
 *
 * @file knob_G_click_led.h
 *
 * @brief Header file to support the PCA9956B LED driver of KNOB G CLICK.
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

#ifndef KNOB_G_CLICK_LED_H_
#define KNOB_G_CLICK_LED_H_

#include <stdlib.h>
#include <stdio.h>

/* Turn on a LED with max. brightness.  */
#define KNOB_G_CLICK_LED_ON    ( 0xFF )
/* Turn off a LED completely */
#define KNOB_G_CLICK_LED_OFF   ( 0x00 )

/*
 * Helper enumerator to select an individual or all LEDs
 * on the KNOB board.
 */
typedef enum {
        KNOB_G_CLICK_LED_1 = 0x01,
        KNOB_G_CLICK_LED_2,
        KNOB_G_CLICK_LED_3,
        KNOB_G_CLICK_LED_4,
        KNOB_G_CLICK_LED_5,
        KNOB_G_CLICK_LED_6,
        KNOB_G_CLICK_LED_7,
        KNOB_G_CLICK_LED_8,
        KNOB_G_CLICK_LED_9,
        KNOB_G_CLICK_LED_10,
        KNOB_G_CLICK_LED_11,
        KNOB_G_CLICK_LED_12,
        KNOB_G_CLICK_LED_13,
        KNOB_G_CLICK_LED_14,
        KNOB_G_CLICK_LED_15,
        KNOB_G_CLICK_LED_16,
        KNOB_G_CLICK_LED_17,
        KNOB_G_CLICK_LED_18,
        KNOB_G_CLICK_LED_19,
        KNOB_G_CLICK_LED_20,
        KNOB_G_CLICK_LED_21,
        KNOB_G_CLICK_LED_22,
        KNOB_G_CLICK_LED_23,
        KNOB_G_CLICK_LED_24,
        KNOB_G_CLICK_LED_MAX = KNOB_G_CLICK_LED_24,
        KNOB_G_CLICK_LED_ALL
} KNOB_G_CLICK_LED;

/* Pointer function which should perform the actual I2C write transaction */
typedef int (*knob_G_click_led_write_t)(void *, uint8_t, const uint8_t *, size_t);
/* Pointer function which should perform the actual I2C read transaction */
typedef int (*knob_G_click_led_read_t)(void *, uint8_t, uint8_t *, size_t);

/* KNOB LED configuration structure */
typedef struct {
        /* Application-defined pointer to function for writing to the PCA9956B LED driver. */
        knob_G_click_led_write_t write;
        /* Application-defined pointer to function for reading from the PCA9956B LED driver. */
        knob_G_click_led_read_t read;
        /* PCA9956B slave device handler */
        void *handle;
} knob_G_click_led_cfg_t;

/*
 * @brief API to select the brightness of an individual or all LEDs on the KNOB board.
 *
 * \param [in] ctx         Pointer to application-defined LED driver's context used to establish the
 *                         I2C connection with the underlying LED driver.
 *
 * \param [in] led         Enumerator to select an individual or all LEDs on the KNOB board.
 *
 * \param [in] duty_cycle  The brightness that should be applied to the selected LED(s).
 *                         Permitted values should be [0..255] where 0 should reflect a closed LED
 *                         and 255 a fully bright LED.
 *
 */
void knob_G_click_led_set_brightness(knob_G_click_led_cfg_t *ctx, KNOB_G_CLICK_LED led, uint8_t duty_cycle);

/*
 * @brief API to select the output current gain of an individual or all LEDs on the KNOB board.
 *
 * \param [in] ctx         Pointer to application-defined LED driver's context used to establish the
 *                         I2C connection with the underlying LED driver.
 *
 * \param [in] led         Enumerator to select an individual or all LEDs on the KNOB board.
 *
 * \param [in] duty_cycle  The output current gain that should be applied to the selected LED(s).
 *                         Permitted values should be [0..255] where 0 should reflect a closed LED
 *                         and 255 a fully bright LED.
 *
 */
void knob_G_click_led_set_gain(knob_G_click_led_cfg_t *ctx, KNOB_G_CLICK_LED led, uint8_t gain);

/*
 * @brief API to turn off an individual or all LEDs on the KNOB board.
 *
 * \param [in] ctx         Pointer to application-defined LED driver's context used to establish the
 *                         I2C connection with the underlying LED driver.
 *
 * \param [in] led         Enumerator to select an individual or all LEDs on the KNOB board.
 *
 */
void knob_G_click_led_turn_off(knob_G_click_led_cfg_t *ctx, KNOB_G_CLICK_LED led);

/*
 * @brief API to turn off fully turn on an individual or all LEDs on the KNOB board.
 *
 * \param [in] ctx         Pointer to application-defined LED driver's context used to establish the
 *                         I2C connection with the underlying LED driver.
 *
 * \param [in] led         Enumerator to select an individual or all LEDs on the KNOB board.
 *
 */
void knob_G_click_led_turn_on(knob_G_click_led_cfg_t *ctx, KNOB_G_CLICK_LED led);

/*
 * @brief API to HW reset the PCA9956B LED driver.
 *
 * \param [in] ctx  Pointer to application-defined LED driver's context used to establish the
 *                  I2C connection with the underlying LED driver.
 *
 * \note The underlying routine toggles the reset line with the help of the I/O debug macros.
 *       Therefore, calling this routine has a meaning only if \sa DBG_IO_ENABLE is enabled.
 */
void knob_G_click_led_hw_reset(knob_G_click_led_cfg_t *ctx);

#endif /* KNOB_G_CLICK_LED_H_ */
