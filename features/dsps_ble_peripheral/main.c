/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief BLE peripheral application
 *
 * Copyright (C) 2015-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */
#include "osal.h"
#include "ble_mgr.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"
#include "sys_watchdog.h"
#if dg_configUART_ADAPTER
   #include "ad_uart.h"
#endif
#include "platform_devices.h"


/*
 * Perform any application specific hardware configuration.  The clocks,
 * memory, etc. are configured before main() is called.
 */
static void prvSetupHardware( void );

/*
 * Task functions .
 */
OS_TASK_FUNCTION(dsps_peripheral_task, pvParameters);
OS_TASK_FUNCTION(dsps_rx_task, pvParameters);
OS_TASK_FUNCTION(dsps_tx_task, pvParameters);

/**
 * @brief System Initialization and creation of the BLE task
 */
static OS_TASK_FUNCTION(system_init, pvParameters)
{
        OS_TASK handle;
        OS_BASE_TYPE status;

#if defined CONFIG_RETARGET
        extern void retarget_init(void);
#endif

        cm_sys_clk_init(CUSTOM_SYS_CLK);
        cm_apb_set_clock_divider(apb_div1);
        cm_ahb_set_clock_divider(ahb_div1);
        cm_lp_clk_init();

        /* Prepare the hardware to run this demo. */
        prvSetupHardware();

        /* Set the desired sleep mode. */
        pm_set_wakeup_mode(true);
        pm_sleep_mode_set(pm_mode_extended_sleep);

#if defined CONFIG_RETARGET
        retarget_init();
#endif

        /* Initialize BLE Manager */
        ble_mgr_init();

        /* Create a task responsible for handling the BLE related events */
        status = OS_TASK_CREATE("DSPS BLE",        /* The text name assigned to the task, for
                                                      debug only; not used by the kernel. */
                       dsps_peripheral_task,       /* The function that implements the task. */
                       NULL,                       /* The parameter passed to the task. */
                       1024,                       /* The number of bytes to allocate to the
                                                      stack of the task. */
                       OS_TASK_PRIORITY_NORMAL,    /* The priority assigned to the task. */
                       handle);                    /* The task handle. */
        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);

        /* Create a task responsible for handling incoming data from the selected interface. */
         status = OS_TASK_CREATE("DSPS RX",        /* The text name assigned to the task, for
                                                      debug only; not used by the kernel. */
                       dsps_rx_task,               /* The function that implements the task. */
                       NULL,                       /* The parameter passed to the task. */
                       768,                        /* The number of bytes to allocate to the
                                                      stack of the task. */
                       OS_TASK_PRIORITY_NORMAL,    /* The priority assigned to the task. */
                       handle);                    /* The task handle. */
         OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);

        /* Create a task responsible for handling output data to the selected interface. */
        status = OS_TASK_CREATE("DSPS TX",         /* The text name assigned to the task, for
                                                      debug only; not used by the kernel. */
                       dsps_tx_task,               /* The function that implements the task. */
                       NULL,                       /* The parameter passed to the task. */
                       768,                        /* The number of bytes to allocate to the
                                                      stack of the task. */
                       OS_TASK_PRIORITY_NORMAL,    /* The priority assigned to the task. */
                       handle);                    /* The task handle. */
        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);

        /* the work of the SysInit task is done */
        OS_TASK_DELETE(OS_GET_CURRENT_TASK());
}
/*-----------------------------------------------------------*/

/**
 * @brief Basic initialization and creation of the system initialization task.
 */
int main( void )
{
        OS_BASE_TYPE status;
        OS_TASK handle;

        /* Start SysInit task. */
        status = OS_TASK_CREATE("SysInit",                /* The text name assigned to the task, for
                                                             debug only; not used by the kernel. */
                                system_init,              /* The System Initialization task. */
                                ( void * ) 0,             /* The parameter passed to the task. */
                                1200,                     /* The number of bytes to allocate to the
                                                             stack of the task. */
                                OS_TASK_PRIORITY_HIGHEST, /* The priority assigned to the task. */
                                handle );                 /* The task handle */
        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);

        /* Start the tasks and timer running. */
        vTaskStartScheduler();

        /* If all is well, the scheduler will now be running, and the following
        line will never be reached.  If the following line does execute, then
        there was insufficient FreeRTOS heap memory available for the idle and/or
        timer tasks     to be created.  See the memory management section on the
        FreeRTOS web site for more details. */
        for ( ;; );
}

/**
 * \brief Initialize the peripherals domain after power-up.
 *
 */
static void periph_init(void)
{
        /*
         * Recommended place to support GPIO not controlled directly by some kind of adapter.
         * Keep in mind that this routine is called every time the device exits the extended
         * sleep state and before the OS scheduler executes again. Typical case is to configure
         * the mode, functionality and state of a pin.
         */
}

static void prvSetupHardware( void )
{
        /* Init hardware */
        pm_system_init(periph_init);

        /* Recommended place to configure I/O pins (controlled by adapters) in the OFF state. */
#if dg_configUART_ADAPTER
        ad_uart_io_config(UART_DSPS_DEVICE->id, UART_DSPS_DEVICE->io, AD_IO_CONF_OFF);
#endif
}

/**
 * @brief Malloc fail hook
 *
 * This function will be called only if it is enabled in the configuration of the OS
 * or in the OS abstraction layer header osal.h, by a relevant macro definition.
 * It is a hook function that will execute when a call to OS_MALLOC() returns error.
 * OS_MALLOC() is called internally by the kernel whenever a task, queue,
 * timer or semaphore is created. It can be also called by the application.
 * The size of the available heap is defined by OS_TOTAL_HEAP_SIZE in osal.h.
 * The OS_GET_FREE_HEAP_SIZE() API function can be used to query the size of
 * free heap space that remains, although it does not provide information on
 * whether the remaining heap is fragmented.
 */
OS_APP_MALLOC_FAILED( void )
{
        ASSERT_ERROR(0);
}

/**
 * @brief Application idle task hook
 *
 * This function will be called only if it is enabled in the configuration of the OS
 * or in the OS abstraction layer header osal.h, by a relevant macro definition.
 * It will be called on each iteration of the idle task.
 * It is essential that code added to this hook function never attempts
 * to block in any way (for example, call OS_QUEUE_GET() with a block time
 * specified, or call OS_TASK_DELAY()). If the application makes use of the
 * OS_TASK_DELETE() API function (as this demo application does) then it is also
 * important that OS_APP_IDLE() is permitted to return to its calling
 * function, because it is the responsibility of the idle task to clean up
 * memory allocated by the kernel to any task that has since been deleted.
 */
OS_APP_IDLE( void )
{
        /* Needed if the idle task is configured to be tracked by the system WDG service. */
        sys_watchdog_idle_task_notify();
}

/**
 * @brief Application stack overflow hook
 *
 * Run-time stack overflow checking is performed only if it is enabled in the configuration of the OS
 * or in the OS abstraction layer header osal.h, by a relevant macro definition.
 * This hook function is called if a stack overflow is detected.
 */
OS_APP_STACK_OVERFLOW( OS_TASK pxTask, char *pcTaskName )
{
        ( void ) pcTaskName;
        ( void ) pxTask;

        ASSERT_ERROR(0);
}

/**
 * @brief Application tick hook
 *
 * This function will be called only if it is enabled in the configuration of the OS
 * or in the OS abstraction layer header osal.h, by a relevant macro definition.
 * This hook function is executed each time a tick interrupt occurs.
 */
OS_APP_TICK( void )
{
}
