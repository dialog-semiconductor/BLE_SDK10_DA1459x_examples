/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief GPADC adapter demonstration example.
 *
 * Copyright (C) 2015-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "osal.h"
#include "resmgmt.h"
#include "hw_cpm.h"
#include "sys_watchdog.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"
#include "platform_devices.h"
#include "misc.h"

/*
 * Task functions .
 */
static OS_TASK_FUNCTION(gpadc_task, pvParameters);

#if dg_configUSE_WDOG
__RETAINED_RW int8_t idle_task_wdog_id = -1;
#endif

#define GPADC_MEASUREMENT_MS       OS_MS_2_TICKS(1000)

/*
 * Perform any application specific hardware configuration.  The clocks,
 * memory, etc. are configured before main() is called.
 */
static void prvSetupHardware( void );

/**
 * @brief system_init() initialize the system
 */
static OS_TASK_FUNCTION(system_init, pvParameters)
{
        OS_TASK handle;
        OS_BASE_TYPE status;

#if defined CONFIG_RETARGET
        extern void retarget_init(void);
#endif

        cm_sys_clk_init(sysclk_XTAL32M);

        cm_apb_set_clock_divider(apb_div1);
        cm_ahb_set_clock_divider(ahb_div1);
        cm_lp_clk_init();

        /* Prepare the hardware to run this demo. */
        prvSetupHardware();

#if defined CONFIG_RETARGET
        retarget_init();
#endif

        /* true - wait for the XTAL32M to be ready before run the app code on wake-up */
        pm_set_wakeup_mode(true);

        /* Set the desired sleep mode. */
        pm_sleep_mode_set(pm_mode_extended_sleep);

        /* Start a task that performs DIE temperature measurements */
        status = OS_TASK_CREATE("ADCTASK",                             /* The text name assigned to the task, for
                                                                           debug only; not used by the kernel. */
                        gpadc_task,                                     /* The System Initialization task. */
                        ( void * ) 0,                                   /* The parameter passed to the task. */
                        1024,                                           /* The number of bytes to allocate to the
                                                                           stack of the task. */
                        OS_TASK_PRIORITY_NORMAL,                        /* The priority assigned to the task.
                                                                           we need this task to run first with
                                                                           no interruption, that is why choose
                                                                           HIGHEST priority*/
                        handle );                                         /* The task handle */

        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);                    /* Check that the task created OK */

        /* the work of the SysInit task is done
         * The task will be terminated */
        OS_TASK_DELETE( OS_GET_CURRENT_TASK() );
}

/**
 * @brief main() creates a 'system_init' task and starts the FreeRTOS scheduler.
 */
int main( void )
{
        OS_BASE_TYPE status;
        OS_TASK handle;

        status = OS_TASK_CREATE("SysInit",                              /* The text name assigned to the task, for
                                                                           debug only; not used by the kernel. */
                        system_init,                                    /* The System Initialization task. */
                        ( void * ) 0,                                   /* The parameter passed to the task. */
                        configMINIMAL_STACK_SIZE * OS_STACK_WORD_SIZE,  /* The number of bytes to allocate to the
                                                                           stack of the task. */
                        OS_TASK_PRIORITY_HIGHEST,                       /* The priority assigned to the task.
                                                                           we need this task to run first with
                                                                           no interruption, that is why choose
                                                                           HIGHEST priority*/
                        handle );                                       /* The task handle */

        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);                    /* Check that the task created OK */

        /* Start the tasks and timer running. */
        vTaskStartScheduler();

        /* If all is well, the scheduler will now be running, and the following
        line will never be reached.  If the following line does execute, then
        there was insufficient FreeRTOS heap memory available for the idle and/or
        timer tasks to be created.  See the memory management section on the
        FreeRTOS web site for more details. */
        for ( ;; );

}

#define GPADC_ADAPTER_CLOSE_TIMEOUT_MS    ( 1000 )

static void app_ad_gpadc_close(ad_gpadc_handle_t handle)
{
        OS_TICK_TIME timeout = OS_GET_TICK_COUNT() + OS_MS_2_TICKS(GPADC_ADAPTER_CLOSE_TIMEOUT_MS);

        while (ad_gpadc_close(handle, false) != AD_GPADC_ERROR_NONE) {

                OS_TICK_TIME current_time = OS_GET_TICK_COUNT();
                /*
                 * If time is not passed the timeout calculated, give some time for the peripheral to finish its
                 * operations (by blocking for some short period of time). Otherwise, if the time has reached the
                 * expiration time, close the adapter forcefully, that is even if the target peripheral has yet to
                 * finish its tasks.
                 */
                if (current_time < timeout) {
                        ad_gpadc_close(handle, true);
                        break;
                }
        }
}

/**
 * @brief Task reposnsible for performing DIE temperature measurements via the GPADC peripheral.
 *        The GPADC block access is achieved with the help of the GPADC adapter layer.
 */
static OS_TASK_FUNCTION(gpadc_task, pvParameters)
{
        OS_TICK_TIME xNextWakeTime;
        int8_t wdog_id;

        /* Just remove compiler warnings about the unused parameter */
        ( void ) pvParameters;

        ad_gpadc_handle_t die_temp_handle;
        uint16_t die_temp_raw_value;
        int ret;
        int die_temp_raw_to_celsius;

        /* Register ble_adv_demo_task to be monitored by watchdog */
        wdog_id = sys_watchdog_register(false);

        /* Initialise xNextWakeTime - this only needs to be done once. */
        xNextWakeTime = OS_GET_TICK_COUNT();

        for ( ;; ) {

                /* Notify watchdog on each loop */
                 sys_watchdog_notify(wdog_id);

                /* Suspend watchdog while blocking on ble_get_event() */
                sys_watchdog_suspend(wdog_id);

                /*
                 * Place this task in the blocked state until it is time to run again.
                 * The block time is specified in ticks, the constant used converts ticks
                 * to ms. While in the Blocked state this task will not consume any CPU
                 * time.
                 */
                xNextWakeTime += GPADC_MEASUREMENT_MS;
                OS_DELAY_UNTIL(xNextWakeTime);

                /* Resume watchdog */
                sys_watchdog_notify_and_resume(wdog_id);

                /*
                 * Open the target GPADC controller and get a handler that should be
                 * used for subsequent GPADC conversions. Keep in mind that as long
                 * as an adapter instance is opened the device cannot enter the extended
                 * sleep state. Instead the ARM WFI state is used when possible.
                 */
                die_temp_handle = ad_gpadc_open(&DIE_TEMPERATURE);
                ASSERT_WARNING(die_temp_handle != NULL);

                /*
                 * Perform a single blocking GPADC operation (that is, the task that
                 * calls the API is blocked until the requested GPADC conversions are
                 * complete).
                 */
                ret = ad_gpadc_read_nof_conv(die_temp_handle, 1, &die_temp_raw_value);
                ASSERT_WARNING(ret == AD_GPADC_ERROR_NONE);

                /*
                 * This API should be used to translate a retrieved raw value (after conversion)
                 * to temperature in Celsius (computed value is multiplied x100).
                 */
                die_temp_raw_to_celsius = ad_gpadc_conv_to_temp_x100(DIE_TEMPERATURE.drv, die_temp_raw_value);

                /*
                 * Once an adapter instance is no longer used, it should be closed so the target
                 * peripheral block is de-initialized and the device can return to the extended
                 * sleep mode. Here is a recommended way how and when an adapter should be closed
                 * forcefully.
                 */
                app_ad_gpadc_close(die_temp_handle);

                DBG_PRINTF("Die temperature: %d oC\n\r", die_temp_raw_to_celsius / 100);
        }

        OS_TASK_DELETE( OS_GET_CURRENT_TASK() );                                /* Delete the task before exiting. It is not allowed in
                                                                                 * FreeRTOS a task to exit without being deleted from
                                                                                 * the OS's queues */
}

/**
 * @brief Initialize the peripherals domain after power-up.
 *
 */
static void periph_init(void)
{
        /*
         * This is the place to control any I/O pin that is not controlled directly by some kind of adapter.
         * Keep in mind that this routine is called every time the device exits the extended sleep state and
         * before the OS scheduler is started.
         *
         */
}

/**
 * @brief Hardware Initialization
 */
static void prvSetupHardware( void )
{
        /* Init hardware */
        pm_system_init(periph_init);

        /*
         * Recommended place to configure I/O pins (controlled by adapters) in the OFF state.
         * In this example, and since no I/O pins are employed, this routine has no effect.
         */
        ad_gpadc_io_config(DIE_TEMPERATURE.id, DIE_TEMPERATURE.io, AD_IO_CONF_OFF);
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
#if dg_configUSE_WDOG
        sys_watchdog_idle_task_notify();
#endif
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

