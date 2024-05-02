/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief SPI template application with MCP4921 sensor.
 *
 * Copyright (C) 2015-2023 Renesas Electronics Corporation and/or its affiliates.
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

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "peripheral_setup.h"
#include "platform_devices.h"
#include "osal.h"
#include "resmgmt.h"
#include "hw_cpm.h"
#include "hw_gpio.h"
#include "hw_watchdog.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"
#include "hw_pdc.h"
#include "ad_spi.h"
#include "hw_wkup.h"
#include "hw_sys.h"
#include "sys_watchdog.h"
#include "misc.h"
#include "mcp4921.h"

/* Task priorities */
#define mainSPI_TASK_PRIORITY              ( OS_TASK_PRIORITY_NORMAL )

/* Enable/disable asynchronous SPI operations */
#define SPI_ASYNC_EN                 ( 0 )

#define SPI_DEVICE_CLOSE_TIMEOUT_MS  ( 30 )

/* Retained symbols */
__RETAINED static OS_EVENT WKUP_EVENT;
__RETAINED static OS_EVENT MCP4921_ASYNC_EVENT;

/* SPI task handle */
__RETAINED static OS_TASK prvMCP4921_h;

#if dg_configUSE_WDOG
__RETAINED_RW int8_t idle_task_wdog_id = -1;
#endif

/* Functions prototypes */
static void prvMCP4921_Task(void *pvParameters);
int _dev_spi_close(ad_spi_handle_t dev, uint32_t timeout_ms);

/* The rate at which data is template task counter is incremented. */
#define mainCOUNTER_FREQUENCY_MS                OS_MS_2_TICKS(200)

/*
 * Perform any application specific hardware configuration.  The clocks,
 * memory, etc. are configured before main() is called.
 */
static void prvSetupHardware( void );

static OS_TASK xHandle;

/*
 * Task functions.
 */
static OS_TASK_FUNCTION(prvMCP4921_Task, pvParameters);

static OS_TASK_FUNCTION(system_init, pvParameters)
{

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

        pm_set_wakeup_mode(true);
        /* Set the desired sleep mode. */
        pm_sleep_mode_set(pm_mode_extended_sleep);

        OS_EVENT_CREATE(WKUP_EVENT);
        OS_EVENT_CREATE(MCP4921_ASYNC_EVENT);

        /* SPI Task */
        OS_TASK_CREATE("MCP4921",                      /* The text name assigned to the task, for
                                                          debug only; not used by the kernel. */
                       prvMCP4921_Task,                /* The function that implements the task. */
                       NULL,                           /* The parameter passed to the task. */
                       1024 * OS_STACK_WORD_SIZE,      /* Stack size allocated for the task in bytes. */
                       mainSPI_TASK_PRIORITY,          /* The priority assigned to the task. */
                       prvMCP4921_h );                 /* The task handle. */
        OS_ASSERT(prvMCP4921_h);

        /* the work of the SysInit task is done */
        OS_TASK_DELETE( xHandle );
}

/**
 * @brief Template main creates a SysInit task, which creates a MCP4921 task
 */
int main( void )
{
        OS_BASE_TYPE status;


        /* Start the two tasks as described in the comments at the top of this
        file. */
        status = OS_TASK_CREATE("SysInit",              /* The text name assigned to the task, for
                                                           debug only; not used by the kernel. */
                        system_init,                    /* The System Initialization task. */
                        ( void * ) 0,                   /* The parameter passed to the task. */
                        OS_MINIMAL_TASK_STACK_SIZE,
                                                        /* The number of bytes to allocate to the
                                                           stack of the task. */
                        OS_TASK_PRIORITY_HIGHEST,       /* The priority assigned to the task. */
                        xHandle );                      /* The task handle */
        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);



        /* Start the tasks and timer running. */
        OS_TASK_SCHEDULER_RUN();

        /* If all is well, the scheduler will now be running, and the following
        line will never be reached.  If the following line does execute, then
        there was insufficient FreeRTOS heap memory available for the idle and/or
        timer tasks to be created.  See the memory management section on the
        FreeRTOS web site for more details. */
        for ( ;; );

}

#if (SPI_ASYNC_EN == 1)
/*
 * User-defined callback function called once SPI async. write operations are complete.
 *
 * \param[in] ud            Pointer to user data passed to the callback
 * \param[in] num_of_bytes  Data transferred through the SPI bus
 */
void MCP4921_write_async_cb(void *ud, uint16_t num_of_bytes)
{
       (void)ud;
       ASSERT_WARNING(num_of_bytes == sizeof(uint16_t));

       if (MCP4921_ASYNC_EVENT) {
               OS_EVENT_SIGNAL_FROM_ISR(MCP4921_ASYNC_EVENT);
       }
}
#endif /* SPI_ASYNC_EN */

/* Helper function to access the MCP4921 DAC module */
static void MCP4921_write_helper(const void *dev, const uint8_t *buf, size_t size)
{
        ASSERT_WARNING(buf != NULL);
        ASSERT_WARNING(size > 0);
        int ret;

        /* Open the SPI device */
        ad_spi_handle_t MCP4921_h = ad_spi_open((ad_spi_controller_conf_t *)dev);

        /* Enable the target SPI device */
        ad_spi_activate_cs(MCP4921_h, 0);

#if (SPI_ASYNC_EN == 0)
        /*
         * Perform asynchronous SPI write operations, that is, the task is blocking
         * until the transaction is complete.
         */
        ret = ad_spi_write(MCP4921_h, buf, size);
#else
        /*
         * Perform asynchronous SPI write operations, that is, the task does not
         * block waiting for the transaction to finish. Once the transaction is
         * complete the registered callback function is called indicating the
         * completion of underlying operation.
         */
        ret = ad_spi_write_async(MCP4921_h, buf, size, MCP4921_write_async_cb, NULL);

        /*
         * In the meantime and while SPI transactions are performed in the background,
         * application task can proceed to other operations/calculations.
         * It is essential that, the new operations do not involve SPI transactions
         * on the already occupied bus!!!
         */
         OS_EVENT_WAIT(MCP4921_ASYNC_EVENT, OS_EVENT_FOREVER);
#endif /* SPI_ASYNC_EN */

         /* Disable the target SPI device */
         ad_spi_deactivate_cs(MCP4921_h);

         /* Close the SPI device */
         _dev_spi_close(MCP4921_h, SPI_DEVICE_CLOSE_TIMEOUT_MS);

         if (ret == 0) {
                 DBG_PRINTF("Value written to MCP4921 DAC : 0x%04X\n\r", ((uint16_t *)buf)[0]);
         } else {
                 DBG_PRINTF("\n\rUnsuccessful SPI write operation: %d\n\r", ret);
         }
}

/* Recommended routine to close an SPI device with timeout */
int _dev_spi_close(ad_spi_handle_t dev, uint32_t timeout_ms)
{
        OS_TICK_TIME timeout = OS_GET_TICK_COUNT() + OS_MS_2_TICKS(timeout_ms);

        while (ad_spi_close(dev, false) != AD_SPI_ERROR_NONE) {
                if (timeout <= OS_GET_TICK_COUNT()) {
                        ad_spi_close(dev, true);
                        return AD_SPI_ERROR_NONE;
                }
                OS_DELAY_MS(1);
        }
        return AD_SPI_ERROR_NONE;
}

/**
 * @brief Task to control the MCP4921 DAC module
 */
static void prvMCP4921_Task(void *pvParameters)
{
        uint16_t reg_val;
        uint16_t dig_val;

        DBG_PRINTF("\n\r*** MCP4921 SPI Demonstration Example ***\n\r\n");

        for (;;) {
                /*
                 * Suspend task execution - As soon as WKUP ISR
                 * is triggered, the task resumes its execution.
                 */
                OS_EVENT_WAIT(WKUP_EVENT, OS_EVENT_FOREVER);

                DBG_PRINTF("\n\r*** Button Press ***\n\r\n");

                /* Select an arbitrary value */
                dig_val = (uint16_t)rand();

                reg_val = MCP4921_SET_REG(dig_val, MCP4921_AB_CONTROL_BIT_SET | MCP4921_GA_CONTROL_BIT_SET |
                                                                                MCP4921_SHDN_CONTROL_BIT_SET);
                /* Perform an SPI write operation */
                MCP4921_write_helper(MCP4921_DEVICE, (const uint8_t *)&reg_val, sizeof(uint16_t));
        }
}

/* WKUP KEY interrupt handler */
static void wkup_cb(void)
{
        if (WKUP_EVENT) {
                OS_EVENT_SIGNAL_FROM_ISR(WKUP_EVENT);
        }
}

/* WKUP KEY block initializer */
static void wkup_init(void)
{
        /* Initialize the WKUP controller */
        hw_wkup_init(NULL);

        /*
         * Set debounce time expressed in ms. Maximum allowable value is 63 ms.
         * A value set to 0 disables the debounce functionality.
         */
        hw_wkup_set_key_debounce_time(10);

        /*
         * Enable interrupts produced by the KEY block of the wakeup controller (debounce
         * circuitry) and register a callback function to hit following a KEY event.
         */
        hw_wkup_register_key_interrupt(wkup_cb, 1);

        /*
         * Set the polarity (rising/falling edge) that triggers the WKUP controller.
         *
         * \note The polarity is applied both to KEY and GPIO blocks of the controller
         *
         */
        hw_wkup_set_trigger(KEY1_PORT, KEY1_PIN,HW_WKUP_TRIG_LEVEL_LO_DEB);

        /* Enable interrupts of WKUP controller */
        hw_wkup_enable_key_irq();
}

/**
 * @brief Initialize the peripherals domain after power-up.
 *
 */
static void periph_init(void)
{
}

/**
 * @brief Hardware Initialization
 */
static void prvSetupHardware( void )
{

    /*
     * The IRQ produced by the KEY sub block of the wakeup controller (debounced IO IRQ)
     * is multiplexed with other trigger sources (VBUS IRQ, SYS2CMAC IRQ, JTAG present)
     * in a single PDC peripheral trigger ID (HW_PDC_PERIPH_TRIG_ID_COMBO).
     */
#if !defined(CONFIG_USE_BLE) && (!dg_configENABLE_DEBUGGER)

    uint32_t pdc_wkup_combo_id = hw_pdc_add_entry(HW_PDC_LUT_ENTRY_VAL(HW_PDC_TRIG_SELECT_PERIPHERAL,
                                                                       HW_PDC_PERIPH_TRIG_ID_COMBO,
                                                                       HW_PDC_MASTER_CM33, 0));
    OS_ASSERT(pdc_wkup_combo_id != HW_PDC_INVALID_LUT_INDEX);

    hw_pdc_set_pending(pdc_wkup_combo_id);
    hw_pdc_acknowledge(pdc_wkup_combo_id);
#endif

    wkup_init();

    /* Init hardware */
    pm_system_init(periph_init);

    /* Enable the COM power domain before handling GPIO */
    hw_sys_pd_com_enable();

    /* Default SPI pins state (off) */
    ad_spi_io_config(((ad_spi_controller_conf_t *)MCP4921_DEVICE)->id,
            ((ad_spi_controller_conf_t *)MCP4921_DEVICE)->io, AD_IO_CONF_OFF);

    /* Configure the KEY1 push button on Pro DevKit */
    HW_GPIO_SET_PIN_FUNCTION(KEY1);
    HW_GPIO_PAD_LATCH_ENABLE(KEY1);
    /* Latch pad status */
    HW_GPIO_PAD_LATCH_DISABLE(KEY1);

    /* Disable the COM power domain after handling GPIOs */
    hw_sys_pd_com_disable();

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
