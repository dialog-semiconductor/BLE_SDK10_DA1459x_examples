/**
 ****************************************************************************************
 *
 * @file platform_devices.c
 *
 * @brief Configuration of devices connected to board data structures
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
 ***************************************************************************************
 */

#include <ad_spi.h>
#include "peripheral_setup.h"
#include "platform_devices.h"

/*
 * PLATFORM PERIPHERALS GPIO CONFIGURATION
 *****************************************************************************************
 */

/* SPI chip-select pins */
static ad_io_conf_t cs_LSM6DSL[] = {{

        .port = LSM6DSL_CS_PORT,
        .pin  = LSM6DSL_CS_PIN,
        .on = {
                .mode     = HW_GPIO_MODE_OUTPUT,
                .function = HW_GPIO_FUNC_GPIO,
                .high     = true
        },
        .off = {
                .mode     = HW_GPIO_MODE_OUTPUT,
                .function = HW_GPIO_FUNC_GPIO,
                .high     = true
               },}
};

/* SPI1 IO */
static const ad_spi_io_conf_t bus_LSM6DSL = {

        .spi_do = {
                .port = LSM6DSL_DO_PORT,
                .pin  = LSM6DSL_DO_PIN,
                .on   = {HW_GPIO_MODE_OUTPUT_PUSH_PULL, HW_GPIO_FUNC_SPI_DO, false},
                .off  = {HW_GPIO_MODE_INPUT,            HW_GPIO_FUNC_GPIO,   true},
        },
        .spi_di = { // Not required
                .port = LSM6DSL_DI_PORT,
                .pin  = LSM6DSL_DI_PIN,
                .on   = {HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_SPI_DI, false},
                .off  = {HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO,   true},
        },
        .spi_clk = {
                .port = LSM6DSL_CLK_PORT,
                .pin  = LSM6DSL_CLK_PIN,
                .on   = {HW_GPIO_MODE_OUTPUT_PUSH_PULL, HW_GPIO_FUNC_SPI_CLK, false},
                .off  = {HW_GPIO_MODE_INPUT,            HW_GPIO_FUNC_GPIO,    true},
        },

        /*
         * The number of pins in spi_master_cs array.
         *
         */
        .cs_cnt = 1,
        .spi_cs = cs_LSM6DSL,
};

/* External sensor/module SPI driver */
static const ad_spi_driver_conf_t drv_LSM6DSL = {
        .spi = {
                .cs_pad         = { LSM6DSL_CS_PORT, LSM6DSL_CS_PIN },
                .word_mode      = HW_SPI_WORD_16BIT,    /* Required by the module used */
                .smn_role       = HW_SPI_MODE_MASTER,
                .cpol_cpha_mode = HW_SPI_CP_MODE_3,
                .xtal_freq      = 16,    /* Operating freq. @ DIVN / xtal_freq = 2MHz */
                .fifo_mode      = HW_SPI_FIFO_RX_TX,
                .disabled       = 0,                    /* Should be disabled during initialization phase */
                .spi_cs         = HW_SPI_CS_GPIO,
                .rx_tl = HW_SPI_FIFO_LEVEL0,
                .tx_tl = HW_SPI_FIFO_LEVEL0,
                .swap_bytes = false,
                .select_divn = true,
                .use_dma = true,
                .rx_dma_channel = HW_DMA_CHANNEL_0,
                .tx_dma_channel = HW_DMA_CHANNEL_0 + 1,
        }
};
/* Sensor/module device configuration */
const ad_spi_controller_conf_t dev_LSM6DSL = {
        .id  = HW_SPI1,
        .io  = &bus_LSM6DSL,
        .drv = &drv_LSM6DSL,
};

spi_device LSM6DSL_DEVICE = &dev_LSM6DSL;
