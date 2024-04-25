/**
 ****************************************************************************************
 *
 * @file platform_devices.h
 *
 * @brief Configuration of devices connected to board
 *
 * Copyright (C) 2017-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef PLATFORM_DEVICES_H_
#define PLATFORM_DEVICES_H_

#include "ad.h"

extern ad_io_conf_t led1_cfg[];
extern size_t led1_size;

#define LED1_DEVICE  led1_cfg
#define LED1_SIZE led1_size

#endif /* PLATFORM_DEVICES_H_ */

