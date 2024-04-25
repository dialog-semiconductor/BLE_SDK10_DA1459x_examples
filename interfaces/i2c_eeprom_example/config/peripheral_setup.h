/**
 ****************************************************************************************
 *
 * @file periph_setup.h
 *
 * @brief Configuration of devices connected to board
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

#ifndef _PERIPH_SETUP_H_
#define _PERIPH_SETUP_H_

#include "hw_gpio.h"

/* I2X Interface */
#define I2C_GPIO_LEVEL HW_GPIO_POWER_V33

/* I2C */
#define I2C_MASTER_SCL_PIN  HW_GPIO_PIN_14
#define I2C_MASTER_SCL_PORT HW_GPIO_PORT_0

#define I2C_MASTER_SDA_PIN  HW_GPIO_PIN_2
#define I2C_MASTER_SDA_PORT HW_GPIO_PORT_1

#define I2C_SLAVE_ADDRESS    ( 0x50 )

#define I2C_AD_CONFIG_MASTER_I2C_CTRL       (HW_I2C1)
#define I2C_AD_CONFIG_MASTER_DMA_CH         (HW_DMA_CHANNEL_2)

#endif /* _PERIPH_SETUP_H_ */
