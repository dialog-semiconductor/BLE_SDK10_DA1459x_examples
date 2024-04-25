/**
 ****************************************************************************************
 *
 * @file platform_devices.h
 *
 * @brief Configuration of devices connected to board
 *
 * Copyright (C) 2016-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef PLATFORM_DEVICES_H_
#define PLATFORM_DEVICES_H_

#if (dg_configUART_ADAPTER == 1)
#include "ad_uart.h"

/*
 * Define sources connected to UART
 */
extern const ad_uart_controller_conf_t uart_dsps_ctrl;

#define UART_DSPS_DEVICE  (&uart_dsps_ctrl)

#endif /* dg_configUART_ADAPTER */

#endif /* PLATFORM_DEVICES_H_ */
