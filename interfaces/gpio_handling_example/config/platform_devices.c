/**
 ****************************************************************************************
 *
 * @file platform_devices.c
 *
 * @brief Configuration of devices connected to board.
 *
 * Copyright (C) 2017-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include "ad.h"
#include "hw_gpio.h"
#include "platform_devices.h"

ad_io_conf_t led1_cfg[] = {
        {
                .port = LED1_PORT, .pin = LED1_PIN,
                .on  = {HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false},
                .off = {HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false},
        },

        /* Here you can add more I/O pins definitions */
};


size_t led1_size = ARRAY_LENGTH(led1_cfg);
