/**
 ****************************************************************************************
 *
 * @file platform_device.c
 *
 * @brief Platform devices source file. This is the recommended place to define peripheral drivers.
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

#include "ad_i2c.h"
#include "platform_devices.h"
#include "knob_G_click_config.h"

/*
 * PLATFORM PERIPHERALS GPIO CONFIGURATION
 *****************************************************************************************
 */

#if KNOB_G_CLICK_LED_ENABLE

/* I2C I/O configuration */
static const ad_i2c_io_conf_t pca9956b_led_io = {
        .scl = {
                .port = KNOB_G_CLICK_SCL_PORT, .pin = KNOB_G_CLICK_SCL_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT_OPEN_DRAIN, HW_GPIO_FUNC_I2C_SCL, false },
                .off = { HW_GPIO_MODE_INPUT,            HW_GPIO_FUNC_GPIO,    true  }
        },
        .sda = {
                .port = KNOB_G_CLICK_SDA_PORT, .pin = KNOB_G_CLICK_SDA_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT_OPEN_DRAIN, HW_GPIO_FUNC_I2C_SDA, false },
                .off = { HW_GPIO_MODE_INPUT,            HW_GPIO_FUNC_GPIO,    true  }
        },
};


/*
 * PLATFORM PERIPHERALS CONTROLLER CONFIGURATION
 *****************************************************************************************
 */

static const ad_i2c_driver_conf_t pca9956b_led_drv = {
                I2C_DEFAULT_CLK_CFG,
                .i2c.speed = HW_I2C_SPEED_FAST,
                .i2c.mode = HW_I2C_MODE_MASTER,
                .i2c.addr_mode = HW_I2C_ADDRESSING_7B,
                .i2c.address = KNOB_G_CLICK_I2C_ADDR,
#if (HW_I2C_DMA_SUPPORT == 1)
                .dma_channel = HW_DMA_CHANNEL_0
#endif
};


/* I2C controller configuration */
const ad_i2c_controller_conf_t pca9956b_led_cfg = {
        .id = HW_I2C1,
        .io = &pca9956b_led_io,
        .drv = &pca9956b_led_drv
};

#endif /* dg_configI2C_ADAPTER */

