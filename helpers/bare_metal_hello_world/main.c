/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Bare metal (no OS) retarget application.
 *
 * Copyright (C) 2015-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include <stdio.h>
#include "hw_watchdog.h"
#include "hw_clk.h"

int main(void)
{
        /* Watchdog should be enabled by ROM booter */
        hw_watchdog_freeze();

        for ( ;; ) {

                hw_clk_delay_usec(1000000);

                printf("Hello world!\n\r");
                fflush(stdout);
        }

        return 0;
}
