/**
 ****************************************************************************************
 *
 * @file custom_config_eflash.h
 *
 * @brief Board Support Package. User Configuration file for cached eFLASH mode.
 *
 * Copyright (C) 2020-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */
#ifndef CUSTOM_CONFIG_EFLASH_H_
#define CUSTOM_CONFIG_EFLASH_H_

#include "bsp_definitions.h"

#define CONFIG_USE_BLE

#define CONFIG_RETARGET
#define DBG_PRINT_ENABLE                        ( 1 )

/*************************************************************************************************\
 * System configuration
 */
#define dg_configEXEC_MODE                      ( MODE_IS_CACHED )
#define dg_configCODE_LOCATION                  ( NON_VOLATILE_IS_EMBEDDED_FLASH )

#define dg_configUSE_WDOG                       ( 1 )

#define dg_configUSE_SW_CURSOR                  ( 1 )

/*************************************************************************************************\
 * FreeRTOS configuration
 */
#define OS_FREERTOS                             /* Define this to use FreeRTOS */
#define configTOTAL_HEAP_SIZE                   ( 15852 )   /* FreeRTOS Total Heap Size */

/*************************************************************************************************\
 * Peripherals configuration
 */
#define dg_configFLASH_ADAPTER                  ( 1 )
#define dg_configNVMS_ADAPTER                   ( 1 )
#define dg_configNVMS_VES                       ( 1 )
#define dg_configNVPARAM_ADAPTER                ( 1 )

/*************************************************************************************************\
 * BLE configuration
 */
#define CONFIG_USE_BLE_SERVICES

#define dg_configBLE_CENTRAL                    ( 0 )
#define dg_configBLE_GATT_CLIENT                ( 0 )
#define dg_configBLE_OBSERVER                   ( 0 )
#define dg_configBLE_BROADCASTER                ( 0 )
#define dg_configBLE_L2CAP_COC                  ( 0 )

#define defaultBLE_ATT_DB_CONFIGURATION         ( 0x10 )  // Peripheral Pref. Conn. Param. attribute
#define defaultBLE_PPCP_INTERVAL_MIN            ( BLE_CONN_INTERVAL_FROM_MS( 500 ) )    // 500 ms
#define defaultBLE_PPCP_INTERVAL_MAX            ( BLE_CONN_INTERVAL_FROM_MS( 750 ) )    // 750 ms
#define defaultBLE_PPCP_SLAVE_LATENCY           ( 0 )                                   // 0 events
#define defaultBLE_PPCP_SUP_TIMEOUT             ( BLE_SUPERVISION_TMO_FROM_MS( 6000 ) ) // 6000 ms

/* Include bsp default values */
#include "bsp_defaults.h"
/* Include middleware default values */
#include "middleware_defaults.h"

#endif /* CUSTOM_CONFIG_EFLASH_H_ */
