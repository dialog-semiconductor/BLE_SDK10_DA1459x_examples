/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Basic framework to handle I/O pins outside adapter context.
 *
 * Copyright (C) 2015-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include <stdio.h>
#include "osal.h"
#include "sys_watchdog.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"
#include "platform_devices.h"
#include "app_gpio.h"

#define APP_LED1_TOGGLE_MS  ( 1000 )

/*
 * Perform any application specific hardware configuration.  The clocks,
 * memory, etc. are configured before main() is called.
 */
static void prvSetupHardware( void );
/*
 * Task functions .
 */
static OS_TASK_FUNCTION(gpio_task, pvParameters);

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

        /* Task responsible for handling GPIO pins */
        status = OS_TASK_CREATE("GPIO",                /* The text name assigned to the task, for
                                                           debug only; not used by the kernel. */
                        gpio_task,                      /* The function that implements the task. */
                        NULL,                           /* The parameter passed to the task. */
                        1024,
                                                        /* The number of bytes to allocate to the
                                                           stack of the task. */
                        OS_TASK_PRIORITY_NORMAL,        /* The priority assigned to the task. */
                        handle );                       /* The task handle */
        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);

        /* the work of the SysInit task is done
         * The task will be terminated */
        OS_TASK_DELETE( OS_GET_CURRENT_TASK() );
}

/**
 * @brief Template main creates a SysInit task, which creates a GPIO handling task
 */
int main( void )
{
        OS_BASE_TYPE status;
        OS_TASK task;

        /* Start the two tasks as described in the comments at the top of this
        file. */
        status = OS_TASK_CREATE("SysInit",              /* The text name assigned to the task, for
                                                           debug only; not used by the kernel. */
                        system_init,                    /* The System Initialization task. */
                        ( void * ) 0,                   /* The parameter passed to the task. */
                        1024,
                                                        /* The number of bytes to allocate to the
                                                           stack of the task. */
                        OS_TASK_PRIORITY_HIGHEST,       /* The priority assigned to the task. */
                        task );                         /* The task handle */
        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);

        /* Start the tasks and timer running. */
        vTaskStartScheduler();

        /* If all is well, the scheduler will now be running, and the following
        line will never be reached.  If the following line does execute, then
        there was insufficient FreeRTOS heap memory available for the idle and/or
        timer tasks to be created.  See the memory management section on the
        FreeRTOS web site for more details. */
        for ( ;; );

}

#if !APP_GPIO_DYNAMIC_MODE
static bool ad_prepare_for_sleep_cb(void)
{
        app_gpio_set_inactive(LED1_DEVICE, LED1_SIZE);

        return true; // Allow the system to enter the sleep state. Otherwise, return false.
}

static void ad_sleep_canceled_cb(void)
{
        app_gpio_set_active(LED1_DEVICE, LED1_SIZE);
}
#endif

/**
 * @brief GPIO handling task
 */
static OS_TASK_FUNCTION(gpio_task, pvParameters)
{

#if !APP_GPIO_DYNAMIC_MODE
        const adapter_call_backs_t ad_pm_call_backs = {
                /* Should be called during sleep preparation. If time-consuming tasks take
                 * place within this callback, the corresponding delay should be reflected
                 * in ad_sleep_preparation_time (in LP clock ticks). */
                .ad_prepare_for_sleep = ad_prepare_for_sleep_cb,

                /* This is equivalent to periph_init which is called upon system wake-up. */
                .ad_wake_up_ind       = NULL,

                /* Should be called if the device was about to enter the sleep state, but sleep was
                 * aborted explicitly upon exit from ad_prepare_for_sleep (return false). */
                .ad_sleep_canceled    = ad_sleep_canceled_cb,

                /* Should triggered when XTAL32M settles (after a wake-up cycle). */
                .ad_xtalm_ready_ind   = NULL,

                /* Define a number of LP clock ticks that should be subtracted from the scheduled sleep
                 * period (as defined by OS) and will be needed by the running application to execute
                 * some specific tasks just before the device is about to enter the sleep state.
                 * (that is in ad_prepare_for_sleep_cb). */
                .ad_sleep_preparation_time = 0,
        };

        pm_register_adapter(&ad_pm_call_backs);
#endif /* GPIO_HANDLING_DYNAMICALLY */

        static bool led1_state = 0;

        for ( ;; ) {

                /* Block task execution for 1 second (just to allow the system to enter sleep). */
                OS_DELAY_MS(APP_LED1_TOGGLE_MS);

                /* Toggle LED1 and updates its state. In this example ON/OFF states should match. */
                led1_state ^= 1;
                LED1_DEVICE->on.high = LED1_DEVICE->off.high = led1_state;

#if APP_GPIO_DYNAMIC_MODE
                /* In this demonstration calling app_gpio_set_inctive would have the same results. */
                app_gpio_set_active(LED1_DEVICE, LED1_SIZE);
#else
                hw_gpio_toggle(LED1_DEVICE->port, LED1_DEVICE->pin);
#endif
        }
}

/**
 * @brief Initialize the peripherals domain after power-up.
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

#if !APP_GPIO_DYNAMIC_MODE
        app_gpio_set_active(LED1_DEVICE, LED1_SIZE);
#endif
}

/**
 * @brief Hardware Initialization
 */
static void prvSetupHardware( void )
{
        /* Init hardware */
        pm_system_init(periph_init);
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
