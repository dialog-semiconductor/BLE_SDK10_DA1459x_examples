/**
 ****************************************************************************************
 *
 * @file i2c_task.h
 *
 * @brief I2C task
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

#ifndef _I2C_TASK_H_
#define _I2C_TASK_H_

#define I2C_REQUEST_SIZE		16
#define I2C_RESPONSE_SIZE		4

/* The rate at which transaction test counter is incremented. */
#define TASK_FREQUENCY_MS		(1000)

#define I2C_ASYNC_EN 			(1)

/**
 * @brief I2C master task: write bytes, reads bytes and print read bytes from EEPROM.
 */
OS_TASK_FUNCTION (i2c_master_task, pvParameters );

#endif /* _I2C_TASK_H_ */
