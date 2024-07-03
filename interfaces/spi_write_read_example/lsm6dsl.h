/*
 ****************************************************************************************
 * @file lsm6dsl.h
 *
 * @brief Register addresses of LSM6DSL
 *
 * Copyright (C) 2020-2024 Renesas Electronics Corporation and/or its affiliates.
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

#ifndef LSM6DSL_H_
#define LSM6DSL_H_

#include <stdio.h>

// LSM6DSL Register Definition
const uint16_t LSM6DSL_CMD_READ     = 0x80;
const uint16_t LSM6DSL_WHO_AM_I     = 0x0F;
const uint16_t LSM6DSL_CTRL1_XL     = 0x10;
const uint16_t LSM6DSL_CTRL2_G      = 0x11;
const uint16_t LSM6DSL_OUTX_L_G_REG = 0x22;    // Gyroscope X-axis low byte register
const uint16_t LSM6DSL_OUTY_L_G_REG = 0x24;    // Gyroscope Y-axis low byte register
const uint16_t LSM6DSL_OUTZ_L_G_REG = 0x26;    // Gyroscope Z-axis low byte register
const uint16_t LSM6DSL_OUTX_L_XL    = 0x28;    // Accelerometer X-axis low byte register
const uint16_t LSM6DSL_OUTY_L_XL    = 0x2A;    // Accelerometer Y-axis low byte register
const uint16_t LSM6DSL_OUTZ_L_XL    = 0x2C;    // Accelerometer Z-axis low byte register
const uint16_t LSM6DSL_STATUS_REG   = 0x1E;    // Status register
const uint16_t LSM6DSL_CTRL3_C      = 0x12;    // Control register 3
const uint16_t LSM6DSL_CTRL4_C      = 0x13;    // Control register 4
const uint16_t LSM6DSL_CTRL5_C      = 0x14;    // Control register 5
const uint16_t LSM6DSL_FIFO_CTRL1   = 0x06;    // FIFO control register 1
const uint16_t LSM6DSL_FIFO_CTRL2   = 0x07;    // FIFO control register 2
const uint16_t LSM6DSL_FIFO_CTRL3   = 0x08;    // FIFO control register 3
const uint16_t LSM6DSL_INT1_CTRL    = 0x0D;    // Interrupt control register 1
const uint16_t LSM6DSL_INT2_CTRL    = 0x0E;    // Interrupt control register 2

#endif /* LSM6DSL_H_ */
