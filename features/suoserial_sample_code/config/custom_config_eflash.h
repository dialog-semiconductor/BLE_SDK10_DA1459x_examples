/**
 ****************************************************************************************
 *
 * @file custom_config_eflash.h
 *
 * @brief Board Support Package. User Configuration file for cached eFLASH mode.
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
#ifndef CUSTOM_CONFIG_EFLASH_H_
#define CUSTOM_CONFIG_EFLASH_H_

#include "bsp_definitions.h"

#define USE_PARTITION_TABLE_EFLASH_WITH_SUOTA

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
#define configTOTAL_HEAP_SIZE                   ( 20 * 1024 )   /* FreeRTOS Total Heap Size */

/*************************************************************************************************\
 * Peripherals configuration
 */
#define dg_configNVMS_VES                       ( 0 )
#define dg_configNVPARAM_ADAPTER                ( 1 )

#define dg_configUART_ADAPTER                   ( 1 )     /* Enable the UART Adapter abstraction layer and API */

/* Enable circular DMA so no data are overwritten in the RX HW FIFOs upon file transfers. */
#define dg_configUART_RX_CIRCULAR_DMA           ( 1 )
/* This value has been adjusted so that no data loss is encountered in file transfer operations. */
#define dg_configUART2_RX_CIRCULAR_DMA_BUF_SIZE ( 1024 )

#if (dg_configUART_ADAPTER == 1)
#undef CONFIG_RETARGET
/*
 * Use the RTT mechanism to interact with the outside world e.g. log messages.
 * RETARGET operations employ the serial port which should raise conflicts
 * with the SUOSERIAL service.
 */
#define CONFIG_RTT
#endif

#define dg_configSUOUART_SUPPORT                 ( 1 )

#if (dg_configSUOUART_SUPPORT == 1)
/*
 * Once the SUOUART task is executed there will be no time for lower
 * priority tasks (typically that should be the idle task) to execute
 * and thus denoting its presence to the WDOG service. Therefore, it's
 * imperative that WDOG monitoring for these tasks be disabled since
 * there is no API to get the IDLE task's WDOG ID. Otherwise, the
 * idle task's monitoring would just be suspended as long as SUOUART
 * was up and running.
 */
#define dg_configWDOG_GUARD_IDLE_TASK            ( 0 )
#endif

/* Include bsp default values */
#include "bsp_defaults.h"
/* Include middleware default values */
#include "middleware_defaults.h"

#endif /* CUSTOM_CONFIG_EFLASH_H_ */
