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

#include "ad_gpadc.h"
#include "platform_devices.h"

#if (dg_configGPADC_ADAPTER == 1)
/*
 * Define sources connected to GPADC
 */

#if dg_configGPADC_DMA_SUPPORT
gpadc_dma_cfg die_temp_dma_cfg = {
        .channel= HW_DMA_CHANNEL_0, // Preferably use an even channel number to keep consistency with peripheral blocks that require a pair or RX/TX DMA channels.
        .circular = false,          // The selected DMA channel will be disabled automatically by the GPADC block once the requested bytes are transfered.
        .prio = HW_DMA_PRIO_0,      // Give it the lowest priority
        .irq_nr_of_trans = 0        // A zero value should trigger a DMA interrupt once all the requested bytes are transferred.
};
#endif

const ad_gpadc_io_conf_t die_temp_io = {};

const ad_gpadc_driver_conf_t die_temp_drv =  {
        .input_mode       = HW_GPADC_INPUT_MODE_SINGLE_ENDED,
        .positive         = HW_GPADC_INP_DIE_TEMP,
        .sample_time      = HW_GPADC_DIE_TEMP_SMPL_TIME,       // Recommended sample time (otherwise an assertion is thrown)
        .continuous       = false,
        .interval         = 0,
        .input_attenuator = HW_GPADC_INPUT_VOLTAGE_UP_TO_0V9,  // Should not exceed 0V9 (otherwise an assertion is thrown)
        .chopping         = true,                              // Recommended setting (otherwise an assertion is thrown)
        .oversampling     = HW_GPADC_OVERSAMPLING_16_SAMPLES,  // Minimum oversampling (otherwise an assertion is thrown)
#if dg_configGPADC_DMA_SUPPORT
        .dma_setup        = &die_temp_dma_cfg
#endif
};

const ad_gpadc_controller_conf_t die_temp_ctrl = {
        .id = HW_GPADC_1,
        .io = &die_temp_io,
        .drv = &die_temp_drv,
};

#endif /* dg_configGPADC_ADAPTER */
