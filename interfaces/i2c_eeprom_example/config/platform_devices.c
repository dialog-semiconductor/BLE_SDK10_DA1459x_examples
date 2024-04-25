/**
 ****************************************************************************************
 *
 * @file platform_devices.c
 *
 * @brief Configuration of devices connected to board data structures
 *
 * Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************************
 */

#include <ad_i2c.h>
#include "peripheral_setup.h"
#include "platform_devices.h"

/*
 * PLATFORM PERIPHERALS GPIO CONFIGURATION
 *****************************************************************************************
 */

#if dg_configI2C_ADAPTER

/* I2C I/O configuration */
const ad_i2c_io_conf_t i2c_master_io = {
        .scl = {
                .port = I2C_MASTER_SCL_PORT, .pin = I2C_MASTER_SCL_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT_PUSH_PULL, HW_GPIO_FUNC_I2C_SCL, false },
                .off = { HW_GPIO_MODE_INPUT,             HW_GPIO_FUNC_GPIO,    true  }
        },
        .sda = {
                .port = I2C_MASTER_SDA_PORT, .pin = I2C_MASTER_SDA_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT_PUSH_PULL, HW_GPIO_FUNC_I2C_SDA, false },
                .off = { HW_GPIO_MODE_INPUT,             HW_GPIO_FUNC_GPIO,    true  }
        }
};
const ad_io_conf_t i2c_write_protect_io = {
		.port = HW_GPIO_PORT_0, .pin = HW_GPIO_PIN_12,
		.on  = {HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true},
		.off = {HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false},
};

/*
 * PLATFORM PERIPHERALS CONTROLLER CONFIGURATION
 *****************************************************************************************
 */

const ad_i2c_driver_conf_t master_driver_config = {
                I2C_DEFAULT_CLK_CFG,
                .i2c.speed = HW_I2C_SPEED_STANDARD,
                .i2c.mode = HW_I2C_MODE_MASTER,
                .i2c.addr_mode = HW_I2C_ADDRESSING_7B,
                .i2c.address = I2C_SLAVE_ADDRESS,
                .dma_channel = I2C_AD_CONFIG_MASTER_DMA_CH
};

/* I2C controller configuration */
const ad_i2c_controller_conf_t master_config = {
        .id = I2C_AD_CONFIG_MASTER_I2C_CTRL,
        .io = &i2c_master_io,
        .drv = &master_driver_config
};

const i2c_device I2C_MASTER_DEVICE = &master_config;
const ad_io_conf_t* I2C_WRITE_PROTECT = &i2c_write_protect_io;

#endif /* dg_configI2C_ADAPTER */

