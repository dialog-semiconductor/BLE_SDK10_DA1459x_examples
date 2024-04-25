/**
 ****************************************************************************************
 *
 * @file platform_devices.h
 *
 * @brief Configuration of devices connected to board.
 *
 * Copyright (C) 2016-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef PLATFORM_DEVICES_H_
#define PLATFORM_DEVICES_H_

#include "ad_gpadc.h"

#if (dg_configGPADC_ADAPTER == 1)

extern const ad_gpadc_controller_conf_t die_temp_ctrl;
#define DIE_TEMPERATURE die_temp_ctrl

#endif

#endif /* PLATFORM_DEVICES_H_ */
