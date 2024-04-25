/**
 ****************************************************************************************
 *
 * @file export_clocks_task.c
 *
 * @brief Mapping system clocks to GPIO
 *
 * Copyright (C) 2015-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */
#include <stdlib.h>
#include <stdio.h>
#include "hw_gpio.h"
#include "sys_power_mgr.h"
#include "sys_watchdog.h"
#include "cli.h"
#include "misc.h"

/* Bit mask notifications */
#define CLOCK_TASK_CLI_NOTIF   (1 << 0)

/* Must not be changed! */
#define CLOCK_SOURCE_XTAL32M_PORT  HW_GPIO_PORT_0
#define CLOCK_SOURCE_XTAL32M_PIN   HW_GPIO_PIN_11

#define CLOCK_SOURCE_RC32M_PORT    HW_GPIO_PORT_1
#define CLOCK_SOURCE_RC32M_PIN     HW_GPIO_PIN_2

#define CLOCK_SOURCE_DIVN_PORT     HW_GPIO_PORT_0
#define CLOCK_SOURCE_DIVN_PIN      HW_GPIO_PIN_8

/* Should be changed to meet application's requirements. */
#define CLOCK_MODE_DEFAULT      CLOCK_MODE_MANUAL
#define CLOCK_SOURCE_DEFAULT    CLOCK_SOURCE_RCX
#define CLOCK_PORT_DEFAULT      HW_GPIO_PORT_0
#define CLOCK_PIN_DEFAULT       HW_GPIO_PIN_10
#define CLOCK_STATUS_DEFAULT    true

#ifndef CLOCK_TASK_CLI_ENABLE
#define CLOCK_TASK_CLI_ENABLE   ( 0 )
#endif

/*****************************************************************************************
 * Clock mode
 */
typedef enum {
        CLOCK_MODE_MANUAL  = 0x0,
        CLOCK_MODE_AUTO         ,
        CLOCK_MODE_INVALID
} CLOCK_MODE;

/*****************************************************************************************
 * Clock source (must not be changed as it reflects bit field values!)
 */
typedef enum {
        CLOCK_SOURCE_XTAL32K = 0x0  ,
        CLOCK_SOURCE_RCLP           ,
        CLOCK_SOURCE_RCX            ,
        CLOCK_SOURCE_XTAL32M        ,
        CLOCK_SOURCE_RC32M          ,
        CLOCK_SOURCE_DIVN           ,
        CLOCK_SOURCE_INVALID
} CLOCK_SOURCE;

/*****************************************************************************************
 * User's clock configuration settings.
 */
__RETAINED_RW static struct {
        HW_GPIO_PORT port;
        HW_GPIO_PIN pin;
        CLOCK_SOURCE src;
        CLOCK_MODE mode;
        bool is_enabled;
} clock_cfg = {
        .port = CLOCK_PORT_DEFAULT,
        .pin = CLOCK_PIN_DEFAULT,
        .src = CLOCK_SOURCE_DEFAULT,
        .mode = CLOCK_MODE_DEFAULT,
        .is_enabled = true
};


__RETAINED static HW_GPIO_PORT clock_mode_manual_port;
__RETAINED static HW_GPIO_PIN clock_mode_manual_pin;
__RETAINED static bool auto_mode_entry_status;

/* Helper function to invalidate the currently selected port to input pull down. This
 * routine should be called prior to selecting a new mapped port/pin. */
__STATIC_FORCEINLINE void clock_invalidate_port(void)
{
        hw_gpio_set_pin_function(clock_cfg.port, clock_cfg.pin, HW_GPIO_MODE_INPUT,
                                                                          HW_GPIO_FUNC_GPIO);
}

/* Should be called prior to switching to the auto mode or updating the clock source
 * while in the auto mode as it might happen that currently selected pins are overwritten.
 * Previous port settings will be restored when switching back to the manual mode. */
static void clock_port_store_and_update(HW_GPIO_PORT port, HW_GPIO_PIN pin)
{
        clock_invalidate_port();

        /* Keep track of the very first auto mode entry. It might happen that clock sources
         * are being updated while in auto mode. */
        if (!auto_mode_entry_status) {
                /* Port selection might be overwritten as in auto mode clocks can be
                 * mapped to dedicated pins. Store the current port so it can be
                 * re-used when switching back to manual mode. */
                clock_mode_manual_port = clock_cfg.port;
                clock_mode_manual_pin = clock_cfg.pin;

                auto_mode_entry_status = true;
        }

        clock_cfg.port = port;
        clock_cfg.pin = pin;
}

/* This function should be used to perform the requested system clock mapping. */
static bool clock_mapping_update(void)
{
#if !dg_configPM_ENABLES_PD_COM_WHILE_ACTIVE
        hw_sys_pd_com_enable();
#endif

        if (clock_cfg.is_enabled) {
                uint32_t reg_val = 0;

               if (clock_cfg.mode == CLOCK_MODE_MANUAL) {
                       REG_SET_FIELD(GPIO, GPIO_CLK_SEL_REG, FUNC_CLOCK_SEL, reg_val, clock_cfg.src);
                       REG_SET_FIELD(GPIO, GPIO_CLK_SEL_REG, FUNC_CLOCK_EN, reg_val, 1);

               } else if (clock_cfg.mode == CLOCK_MODE_AUTO) {

                       /* Overwrite port settings as clock is exposed on dedicated ports */
                       switch (clock_cfg.src) {
                       case CLOCK_SOURCE_XTAL32M:
                               clock_port_store_and_update(CLOCK_SOURCE_XTAL32M_PORT, CLOCK_SOURCE_XTAL32M_PIN);
                               REG_SET_FIELD(GPIO, GPIO_CLK_SEL_REG, XTAL32M_OUTPUT_EN, reg_val, 1);
                               break;
                       case CLOCK_SOURCE_RC32M:
                               clock_port_store_and_update(CLOCK_SOURCE_RC32M_PORT, CLOCK_SOURCE_RC32M_PIN);
                               REG_SET_FIELD(GPIO, GPIO_CLK_SEL_REG, RC32M_OUTPUT_EN, reg_val, 1);
                               break;
                       case CLOCK_SOURCE_DIVN:
                               clock_port_store_and_update(CLOCK_SOURCE_DIVN_PORT, CLOCK_SOURCE_DIVN_PIN);
                               REG_SET_FIELD(GPIO, GPIO_CLK_SEL_REG, DIVN_OUTPUT_EN, reg_val, 1);
                               break;
                       default:
                               return false;
                       }
               }
               /* Apply new clock settings */
               GPIO->GPIO_CLK_SEL_REG = reg_val;

               hw_gpio_set_pin_function(clock_cfg.port, clock_cfg.pin, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_CLOCK);
               hw_gpio_pad_latch_enable(clock_cfg.port, clock_cfg.pin);
        } else {
                GPIO->GPIO_CLK_SEL_REG = 0;
                clock_invalidate_port();
        }

#if !dg_configPM_ENABLES_PD_COM_WHILE_ACTIVE
        hw_sys_pd_com_disable();
#endif
        return true;
}

#if CLOCK_TASK_CLI_ENABLE
/* Handler to be called when the mapped clock port/in is requested to be updated via CLI. */
static void clock_port_update_handler(int argc, const char *argv[], void *user_data)
{
        HW_GPIO_PORT port;
        HW_GPIO_PIN pin;

        if (argc != 3) {
                DBG_PRINTF("\n\rMissing arguments: 'clock_port <port> <pin>'\n\r");
                return;
        }

        /* Convert strings to integers */
        port = atoi(argv[1]);
        pin  = atoi(argv[2]);

        /* Perform sanity checks and make sure that a valid port is declared */
        if (port >= HW_GPIO_NUM_PORTS || pin >= hw_gpio_port_num_pins[port]) {
                DBG_PRINTF("\n\rInvalid clock port selection\n\r");
                return;
        }

        /* Update only if a new port is selected */
        if (port != clock_cfg.port || pin != clock_cfg.pin) {

                if (clock_cfg.mode == CLOCK_MODE_AUTO) {
                        DBG_PRINTF("\n\rChanging ports while in auto mode, is not allowed.\n\r");
                        return;
                }
                /* Explicitly reset the previously selected port */
                clock_invalidate_port();

                /* Update the GPIO used to output the selected clock. */
                clock_cfg.port = port;
                clock_cfg.pin  = pin;


                ASSERT_WARNING(clock_mapping_update() == true);
        }

        DBG_PRINTF("\n\rPORT%d.%d\n\r", port, pin);
}

/* CLI handler to be called when no handler has been matched for the requested task. */
static void cli_default_handler(int argc, const char *argv[], void *user_data)
{
        DBG_PRINTF("\n\rInvalid command. Retry...\n\r");
}

/* Handler to be called when the mapped clock source is requested to be updated via CLI. */
static void clock_source_update_handler(int argc, const char *argv[], void *user_data)
{
        CLOCK_SOURCE source;

        if (argc != 2) {
                DBG_PRINTF("\n\rMissing arguments: 'clock_source <arg1>', where <arg1> is 0 --> XTAL32K, "
                                           "1 --> RCLP, 2 --> RCX, 3 --> XTAL32M, 4 --> RC32M, 5 --> DIVN\n\r");
                return;
        }

        /* Convert string to integer */
        source = atoi(argv[1]);

        if (source < CLOCK_SOURCE_INVALID && source != clock_cfg.src) {

                switch (source) {
                case CLOCK_SOURCE_XTAL32K:
                        if (!(dg_configUSE_LP_CLK == LP_CLK_32768)) {
                                DBG_PRINTF("\n\rXTAL32K is not enabled. Please select another clock source or enable rXTAL32K.\n\r");
                                return;
                        } else {
                                DBG_PRINTF("\n\rClock source will be set to XTAL32K.\n\r");
                        }
                        break;
                case CLOCK_SOURCE_RCLP:
                        DBG_PRINTF("\n\rClock source will be set to RCLP.\n\r");
                        break;
                case CLOCK_SOURCE_RCX:
                        if (!(dg_configUSE_LP_CLK == LP_CLK_RCX)) {
                                DBG_PRINTF("\n\rRCX is not enabled. Please select another clock source or enable RCX.\n\r");
                                return;
                        } else {
                                DBG_PRINTF("\n\rClock source will be set to RCX.\n\r");
                        }
                        break;
                case CLOCK_SOURCE_XTAL32M:
                        DBG_PRINTF("\n\rClock source will be set to XTAL32M.\n\r");
                        break;
                case CLOCK_SOURCE_RC32M:
                        DBG_PRINTF("\n\rClock source will set to RC32M.\n\r");
                        break;
                case CLOCK_SOURCE_DIVN:
                        DBG_PRINTF("\n\rClock source will set to DIVN.\n\r");
                        break;
                default:
                        ASSERT_WARNING(0);
                        break;
                }

                /* Update clock settings */
                CLOCK_SOURCE old_src = clock_cfg.src;
                clock_cfg.src = source;

                if (clock_mapping_update()) {
                        DBG_PRINTF("\n\rClock source has been set successfully\n\r");
                } else {
                        ASSERT_WARNING(clock_cfg.mode == CLOCK_MODE_AUTO);

                        /* Updating the clock source failed. Restore its previous value. */
                        clock_cfg.src = old_src;
                        DBG_PRINTF("\n\rThe selected clock source is not supported in auto mode.\n\r");
                }
        } else if (source >= CLOCK_SOURCE_INVALID) {
                DBG_PRINTF("\n\rInvalid clock source. Retry...\n\r");
        } else {
                DBG_PRINTF("\n\rThe selected clock source is already applied.\n\r");
        }
}

/* Handler to be called when the mapped clock mode is requested to be updated via CLI. */
static void clock_mode_update_handler(int argc, const char *argv[], void *user_data)
{
        CLOCK_MODE mode;

        if (argc != 2) {
                DBG_PRINTF("\n\rMissing arguments: 'clock_mode <arg1>', where <arg1> is 0 --> manual, 1 --> auto\n\r");
                return;
        }

        /* Convert string to integer */
        mode = atoi(argv[1]);

        if (mode < CLOCK_MODE_INVALID && mode != clock_cfg.mode) {

                switch (mode) {
                case CLOCK_MODE_MANUAL:
                        /* Explicitly reset the previously selected port */
                        clock_invalidate_port();

                        /* Restore the port that might have changed when switching to
                         * auto mode or when changing the clock source while in auto mode. */
                        clock_cfg.port = clock_mode_manual_port;
                        clock_cfg.pin = clock_mode_manual_pin;

                        auto_mode_entry_status = false;
                        DBG_PRINTF("\n\rClock mode is set to manual.\n\r");
                        break;
                case CLOCK_MODE_AUTO:
                default:
                        break;
                }

                /* Update clock settings */
                CLOCK_MODE old_mode = clock_cfg.mode;
                clock_cfg.mode = mode;

                if (!clock_mapping_update()) {
                        ASSERT_WARNING(clock_cfg.mode == CLOCK_MODE_AUTO);

                        /* Updating the clock mode failed. Restore its previous value. */
                        clock_cfg.mode = old_mode;
                        DBG_PRINTF("\n\rThe selected clock source is not supported in auto mode.\n\r");
                } else {
                        DBG_PRINTF("\n\rClock mode is set to auto.\n\r");
                }
        } else if (mode >= CLOCK_MODE_INVALID) {
                DBG_PRINTF("\n\rInvalid clock mode...\r\n");
        } else {
                DBG_PRINTF("\n\rThe selected clock mode is already applied.\n\r");
        }
}

/* Handler to be called when the mapped clock is requested to be enabled/disabled via CLI. */
static void clock_enable_handler(int argc, const char *argv[], void *user_data)
{
        bool is_enabled;

        if (argc != 2) {
                DBG_PRINTF("\n\rMissing arguments: 'clock_enable <arg1>', where <arg1> is 0 --> disable clock output, 1 --> enable\n\r");
                return;
        }

        /* Convert string to integer */
        is_enabled = !!atoi(argv[1]);

        if (is_enabled != clock_cfg.is_enabled) {
                /* Enable clock settings */
                clock_cfg.is_enabled = is_enabled;

                ASSERT_WARNING(clock_mapping_update());
        }

        DBG_PRINTF("\n\rMapped clock is %s\n\r", clock_cfg.is_enabled ? "enabled" : "disabled");
}

/* Handler to be called when the mapped clock status is requested via CLI. */
static void clock_status_handler(int argc, const char *argv[], void *user_data)
{
        DBG_PRINTF("\n\rClock mode: %d\n\rClock PORT%d.%d\n\rClock source: %d\n\rClock status: %d\n\r",
                                                                                        clock_cfg.mode,
                                                                                        clock_cfg.port,
                                                                                        clock_cfg.pin,
                                                                                        clock_cfg.src,
                                                                                        clock_cfg.is_enabled);
}

/* Registered commands along with their handlers */
static const cli_command_t cmd_handlers[] = {
        {"clock_port",   clock_port_update_handler,   NULL},
        {"clock_mode",   clock_mode_update_handler,   NULL},
        {"clock_source", clock_source_update_handler, NULL},
        {"clock_enable", clock_enable_handler, NULL},
        {"clock_status", clock_status_handler, NULL},
        /* More handlers can be added here. */
        {} /* A null entry is required.*/
};
#else
static bool ad_prepare_for_sleep_cb(void)
{
        /* Prevent sleeping if the mapped clock is up and running. Mapping clocks while
         * in the sleep state is not allowed. */
        return !clock_cfg.is_enabled;
}
#endif /* CLOCK_TASK_CLI_ENABLE */

/* Main application task */
OS_TASK_FUNCTION(prvExportClocksTask, pvParameters)
{
        OS_BASE_TYPE ret;
        uint32_t notif;
        int8_t wdog_id;
#if CLOCK_TASK_CLI_ENABLE
        cli_t cli;
#endif

        wdog_id = sys_watchdog_register(false);

#if CLOCK_TASK_CLI_ENABLE
        /* When the CONSOLE/CLI modules are enabled and because of the UART adapter invocation,
         * the device is only allowed to enter the idle sleep state (low-power WFI ARM state). */
        cli_init();
        cli = cli_register(CLOCK_TASK_CLI_NOTIF, cmd_handlers, cli_default_handler);

        DBG_PRINTF("\n\rEnter a valid command and press enter...\n\r");
#else
        const adapter_call_backs_t ad_pm_call_backs = {
                /* Should be called during sleep preparation. If time-consuming tasks take
                 * place within this callback, the corresponding delay should be reflected
                 * in ad_sleep_preparation_time (in LP clock ticks). */
                .ad_prepare_for_sleep = ad_prepare_for_sleep_cb,

                /* This is equivalent to periph_init which is called upon system wake-up. */
                .ad_wake_up_ind       = NULL,

                /* Should be called if the device was about to enter the sleep state, but sleep was
                 * aborted explicitly upon exit from ad_prepare_for_sleep (return false). */
                .ad_sleep_canceled    = NULL,

                /* Should triggered when XTAL32M settles (after a wake-up cycle). */
                .ad_xtalm_ready_ind   = NULL,

                /* Define a number of LP clock ticks that should be subtracted from the scheduled sleep
                 * period (as defined by OS) and will be needed by the running application to execute
                 * some specific tasks just before the device is about to enter the sleep state.
                 * (that is in ad_prepare_for_sleep_cb). */
                .ad_sleep_preparation_time = 0,
        };

        /* If the CLI/CONSOLE module is not enabled then the device will enter the sleep state.
         * Register user callback functions so the task can get notified when the device is
         * about to enter the sleep state and prevent it if the mapped clock should be up and
         * running. */
        pm_register_adapter(&ad_pm_call_backs);
#endif /* CLOCK_TASK_CLI_ENABLE */

        /* Do the trick! */
        clock_mapping_update();

        for (;;) {

                sys_watchdog_notify(wdog_id);
                /* Remove task id from the monitoring list as long as a task remains in the blocking state. */
                sys_watchdog_suspend(wdog_id);

                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                OS_ASSERT(ret == OS_OK);

                sys_watchdog_notify_and_resume(wdog_id);

#if CLOCK_TASK_CLI_ENABLE
                if (notif & CLOCK_TASK_CLI_NOTIF) {
                      cli_handle_notified(cli);
                }
#endif
        }
}
