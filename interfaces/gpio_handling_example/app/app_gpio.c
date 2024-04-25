/**
 ****************************************************************************************
 *
 * @file app_gpio.c
 *
 * @brief Basic framework to handle I/O pins outside adapter context.
 *
 * Copyright (C) 2017-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include "hw_sys.h"
#include "app_gpio.h"

void app_gpio_set_active(const ad_io_conf_t *io, uint8_t size)
{
#if !dg_configPM_ENABLES_PD_COM_WHILE_ACTIVE
        /* GPIO controller resides in PD_COM and so it should be enabled before
         * accessing any GPIO. */
        hw_sys_pd_com_enable();
#endif

        ad_io_configure(io, size, AD_IO_CONF_ON);

        ad_io_set_pad_latch(io, size, APP_GPIO_DYNAMIC_MODE ? AD_IO_PAD_LATCHES_OP_TOGGLE :
                                                              AD_IO_PAD_LATCHES_OP_ENABLE);

#if !dg_configPM_ENABLES_PD_COM_WHILE_ACTIVE && APP_GPIO_DYNAMIC_MODE
        /* When PD_COM is not needed it should be disabled to minimize power consumption
         * when in the sleep state. */
        hw_sys_pd_com_disable();
#endif /* dg_configPM_ENABLES_PD_COM_WHILE_ACTIVE */
}

void app_gpio_set_inactive(const ad_io_conf_t *io, uint8_t size)
{
#if !dg_configPM_ENABLES_PD_COM_WHILE_ACTIVE && APP_GPIO_DYNAMIC_MODE
        /* GPIO controller resides in PD_COM and so it should be enabled before
         * accessing any GPIO. */
        hw_sys_pd_com_enable();
#else
        ASSERT_WARNING(hw_pd_check_com_status());
#endif

        ad_io_configure(io, size, AD_IO_CONF_OFF);

        ad_io_set_pad_latch(io, size, APP_GPIO_DYNAMIC_MODE ? AD_IO_PAD_LATCHES_OP_TOGGLE :
                                                              AD_IO_PAD_LATCHES_OP_DISABLE);

#if !dg_configPM_ENABLES_PD_COM_WHILE_ACTIVE
        /* When PD_COM is not needed it should be disabled to minimize power consumption
         * when in the sleep state. */
        hw_sys_pd_com_disable();
#endif
}
