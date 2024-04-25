/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief iBeacon demo application
 *
 * Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************************
 */
#include "osal.h"
#include "ble_common.h"
#include "ble_gap.h"
#include "ble_mgr.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"
#include "sys_watchdog.h"

/* iBeacon advertising data */
#define IBEACON_LENGTH              0x1a
#define IBEACON_TYPE                0xff
#define IBEACON_COMPANY_ID          0xd2, 0x00 /* Company ID: Renesas Electronics Corporation, Vendor ID bytes must be inverted */
#define IBEACON_SUBTYPE             0x02
#define IBEACON_SUBTYPE_LENGTH      0x15
#define IBEACON_128_UUID            0xf0, 0x23, 0x1f, 0x82, 0x2d, 0x3b, 0xfd, 0x12, 0x85, 0x02, 0x98, 0x9b, 0xe0, 0xfd, 0xd9, 0xda
#define IBEACON_MAJOR_NUM           0x00, 0x01
#define IBEACON_MINOR_NUM           0x00, 0x01
#define IBEACON_TX_POWER            0x3c

/* iBeacon advertising data */
static const uint8_t adv_data[] = {
/* Standard flags must not be included in the advertising data. These will be included automatically by stack.*/
IBEACON_LENGTH,
IBEACON_TYPE,
IBEACON_COMPANY_ID,
IBEACON_SUBTYPE,
IBEACON_SUBTYPE_LENGTH,
IBEACON_128_UUID,
IBEACON_MAJOR_NUM,
IBEACON_MINOR_NUM,
IBEACON_TX_POWER };

/* Task priorities */
#define mainIBEACON_DEMO_TASK_PRIORITY              ( OS_TASK_PRIORITY_NORMAL )

#if (dg_configTRACK_OS_HEAP == 1)
/*
 * ConstantsVariables used for Tasks Stack and OS Heap tracking
 * Declared global to avoid IDLE stack Overflows
 */
#define mainMAX_NB_OF_TASKS           10
#define mainMIN_STACK_GUARD_SIZE      8 /* words */
#define mainTOTAL_HEAP_SIZE_GUARD     64 /*bytes */

TaskStatus_t pxTaskStatusArray[mainMAX_NB_OF_TASKS];
uint32_t ulTotalRunTime;
#endif /* (dg_configTRACK_OS_HEAP == 1) */

/*
 * Perform any application specific hardware configuration.  The clocks,
 * memory, etc. are configured before main() is called.
 */
static void prvSetupHardware(void);
/*
 * Task functions .
 */
static OS_TASK_FUNCTION(ibeacon_demo_task, pvParameters);

static OS_TASK handle = NULL;

/**
 * @brief System Initialization and creation of the BLE task
 */
static OS_TASK_FUNCTION(system_init, pvParameters)
{
#if defined CONFIG_RETARGET
        extern void retarget_init(void);
#endif

        /* Use appropriate XTAL for each device */
        cm_sys_clk_init(sysclk_XTAL32M);
        cm_apb_set_clock_divider(apb_div1);
        cm_ahb_set_clock_divider(ahb_div1);
        cm_lp_clk_init();


        /* Prepare the hardware to run this demo */
        prvSetupHardware();

#if defined CONFIG_RETARGET
        retarget_init();
#endif

        /* Set the desired sleep mode */
        pm_set_wakeup_mode(true);

        pm_sleep_mode_set(pm_mode_extended_sleep);

        /* Initialize BLE Manager */
        ble_mgr_init();

        /* Start the BLE adv demo application task */
        OS_TASK_CREATE("iBeacon_Demo",                  /* The text name assigned to the task, for
                                                           debug only; not used by the kernel. */
                       ibeacon_demo_task,               /* The function that implements the task. */
                       NULL,                            /* The parameter passed to the task. */
                       768,                             /* The number of bytes to allocate to the
                                                           stack of the task. */
                       mainIBEACON_DEMO_TASK_PRIORITY,  /* The priority assigned to the task. */
                       handle);                         /* The task handle. */
        OS_ASSERT(handle);

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
        OS_TASK_SCHEDULER_RUN();

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

static void periph_init(void) {

}

static void handle_evt_gap_connected(ble_evt_gap_connected_t *evt) {
	/**
	 * Manage behavior upon connection
	 */
}

static void handle_evt_gap_disconnected(ble_evt_gap_disconnected_t *evt) {
	/**
	 * Manage behavior upon disconnection
	 */

	// Restart advertising
	ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);
}

static void handle_evt_gap_pair_req(ble_evt_gap_pair_req_t *evt) {
	ble_gap_pair_reply(evt->conn_idx, true, evt->bond);
}

static OS_TASK_FUNCTION(ibeacon_demo_task, pvParameters) {
	int8_t wdog_id;

	/* Just remove compiler warnings about the unused parameter */
	(void ) pvParameters;

	/* Register to be monitored by watchdog */
	wdog_id = sys_watchdog_register(false);

	/* Start BLE device as a peripheral */
	ble_peripheral_start();

	/* Set advertising data */
	ble_gap_adv_data_set(sizeof(adv_data), adv_data, 0, NULL);

	/* Set Advertising interval to 100ms */
	ble_gap_adv_intv_set(0xA0, 0xA0);

	/* Start advertising */
	ble_gap_adv_start(GAP_CONN_MODE_NON_CONN);

	for (;;) {
		ble_evt_hdr_t *hdr;

		/* Notify watchdog on each loop */
		sys_watchdog_notify(wdog_id);

		/* Suspend watchdog while blocking on ble_get_event() */
		sys_watchdog_suspend(wdog_id);

		/*
		 * Wait for a BLE event - this task will block
		 * indefinitely since nothing will be received,
		 * because it is not possible to connect to the
		 * iBeacon (GAP_CONN_MODE_NON_CONN).
		 */
		hdr = ble_get_event(true);

		/* Resume watchdog */
		sys_watchdog_notify_and_resume(wdog_id);

		if (!hdr) {
			continue;
		}

		switch (hdr->evt_code) {
		case BLE_EVT_GAP_CONNECTED:
			handle_evt_gap_connected((ble_evt_gap_connected_t*) hdr);
			break;
		case BLE_EVT_GAP_DISCONNECTED:
			handle_evt_gap_disconnected((ble_evt_gap_disconnected_t*) hdr);
			break;
		case BLE_EVT_GAP_PAIR_REQ:
			handle_evt_gap_pair_req((ble_evt_gap_pair_req_t*) hdr);
			break;
		default:
			ble_handle_event_default(hdr);
			break;
		}

		/* Free event buffer (it's not needed anymore) */
		OS_FREE(hdr);
	}
}

static void prvSetupHardware(void) {

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
OS_APP_MALLOC_FAILED( void ) {
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
OS_APP_IDLE( void ) {
#if (dg_configTRACK_OS_HEAP == 1)
        OS_BASE_TYPE i = 0;
        OS_BASE_TYPE uxMinimumEverFreeHeapSize;

        // Generate raw status information about each task.
        OS_UBASE_TYPE uxNbOfTaskEntries = OS_GET_TASKS_STATUS(pxTaskStatusArray, mainMAX_NB_OF_TASKS);

        for (i = 0; i < uxNbOfTaskEntries; i++) {
                /* Check Free Stack*/
                OS_BASE_TYPE uxStackHighWaterMark;

                uxStackHighWaterMark = OS_GET_TASK_STACK_WATERMARK(pxTaskStatusArray[i].xHandle);
                OS_ASSERT(uxStackHighWaterMark >= mainMIN_STACK_GUARD_SIZE);
        }

        /* Check Minimum Ever Free Heap against defined guard. */
        uxMinimumEverFreeHeapSize = OS_GET_HEAP_WATERMARK();
        OS_ASSERT(uxMinimumEverFreeHeapSize >= mainTOTAL_HEAP_SIZE_GUARD);
#endif /* (dg_configTRACK_OS_HEAP == 1) */

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
OS_APP_STACK_OVERFLOW( OS_TASK pxTask, char *pcTaskName ) {
	(void ) pcTaskName;
	(void ) pxTask;

	ASSERT_ERROR(0);
}

/**
 * @brief Application tick hook
 *
 * This function will be called only if it is enabled in the configuration of the OS
 * or in the OS abstraction layer header osal.h, by a relevant macro definition.
 * This hook function is executed each time a tick interrupt occurs.
 */
OS_APP_TICK( void ) {
}
