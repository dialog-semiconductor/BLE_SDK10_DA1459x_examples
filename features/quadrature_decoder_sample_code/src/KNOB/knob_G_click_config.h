/**
 ****************************************************************************************
 *
 * @file knob_G_click_config.h
 *
 * @brief Quadrature decoder default configuration values. Values can be overwritten by re-definig
 *        the corresponding macro into the customer configuration file i.e. custom_config_xxx.h.
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

#ifndef KNOB_G_CLICK_CONFIG_H_
#define KNOB_G_CLICK_CONFIG_H_

#ifndef KNOB_G_CLICK_SDA_PORT
#define KNOB_G_CLICK_SDA_PORT  ( HW_GPIO_PORT_0 )
#endif

#ifndef KNOB_G_CLICK_SDA_PIN
#define KNOB_G_CLICK_SDA_PIN   ( HW_GPIO_PIN_0 )
#endif

#ifndef KNOB_G_CLICK_SCL_PORT
#define KNOB_G_CLICK_SCL_PORT  ( HW_GPIO_PORT_0 )
#endif

#ifndef KNOB_G_CLICK_SCL_PIN
#define KNOB_G_CLICK_SCL_PIN   ( HW_GPIO_PIN_1 )
#endif

#ifndef KNOB_G_CLICK_RST_PORT
#define KNOB_G_CLICK_RST_PORT  ( HW_GPIO_PORT_0 )
#endif

#ifndef KNOB_G_CLICK_RST_PIN
#define KNOB_G_CLICK_RST_PIN   ( HW_GPIO_PIN_2 )
#endif

#ifndef KNOB_G_CLICK_I2C_ADDR
#define KNOB_G_CLICK_I2C_ADDR   ( 0x70 )
#endif

#ifndef KNOB_G_CLICK_KEY_PORT
#define KNOB_G_CLICK_KEY_PORT   ( HW_GPIO_PORT_0 )
#endif

#ifndef KNOB_G_CLICK_KEY_PIN
#define KNOB_G_CLICK_KEY_PIN    ( HW_GPIO_PIN_3 )
#endif

/*
 * KNOB G Click should connect its SW signal to logic high
 * and thus, KEY press events occur when the SW signal is
 * grounded. The triggering edge should be used to properly
 * configure the WKUP controller.
 */
#ifndef KNOB_G_CLICK_KEY_ACTIVE_LOW
#define KNOB_G_CLICK_KEY_ACTIVE_LOW  ( 1 )
#endif

/* Configuration pre-processor to include/exclude KNOB KEY support. */
#ifndef KNOB_G_CLICK_KEY_ENABLE
#define KNOB_G_CLICK_KEY_ENABLE      ( 1 )
#endif

/* Configuration pre-processor to include/exclude KNOB LED support. */
#ifndef KNOB_G_CLICK_LED_ENABLE
#define KNOB_G_CLICK_LED_ENABLE      ( 1 )
#endif

#endif /* KNOB_G_CLICK_CONFIG_H_ */
