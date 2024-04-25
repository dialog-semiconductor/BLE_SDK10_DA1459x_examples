/**
 ****************************************************************************************
 *
 * @file timer_pwm.c
 *
 * @brief HW timer demonstration task
 *
 * Copyright (C) 2015-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */
#include <stdbool.h>
#include "osal.h"
#include "sys_watchdog.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"
#include "hw_timer.h"
#include "hw_sys.h"
#include "platform_devices.h"
#include "misc.h"
#include "hw_pdc.h"
#include "periph_setup.h"

__RETAINED static OS_TASK task_h;

#define APP_TIMER_COUNTER_INT_NOTIF  ( 1 << 0 )
#define APP_TIMER_CAPTURE_INT_NOTIF  ( 1 << 1 )

#if APP_TIMER_CAPTURE_EN
#define APP_GPIO1_CAPTURE_STATUS_Msk   0x01
#define APP_GPIO2_CAPTURE_STATUS_Msk   0x02
#define APP_GPIO3_CAPTURE_STATUS_Msk   0x04
#define APP_GPIO4_CAPTURE_STATUS_Msk   0x08

#define APP_GPIO1_CAPTURE_INT_Msk   0x01
#define APP_GPIO2_CAPTURE_INT_Msk   0x02
#define APP_GPIO3_CAPTURE_INT_Msk   0x04
#define APP_GPIO4_CAPTURE_INT_Msk   0x08

#define WORD_MAX_VALUE         0xFFFFFFFFUL

static volatile uint32_t app_gpio1_capture_val;
static volatile uint32_t app_gpio2_capture_val;
static volatile uint32_t app_timer_overflow_cnt;
static volatile bool app_is_timer_running;

static uint32_t app_timer_get_frequency(HW_TIMER_ID id);

/* Compute the distance in ticks between GPIO1 and GPIO2 capture events. */
uint32_t app_timer_get_events_period(HW_TIMER_ID id)
{
        uint32_t diff = 0;
        uint32_t on_reload = hw_timer_get_reload(id);
        uint32_t direction = HW_TIMER_REG_GETF(id, TIMER_CTRL_REG, TIM_COUNT_DOWN_EN);

        if (((direction == HW_TIMER_DIR_UP) && on_reload) /*not supported*/ ||
                app_is_timer_running /*another capture cycle is already in progress and so symbols might have been updated*/) {
                ASSERT_WARNING(0);
        }

        /* Disable clocking to avoid updating symbols mid-calculation. */
        hw_timer_disable_clk(id);
        if (direction == HW_TIMER_DIR_UP) {
                /*
                 * Mathematical formula: (TIMER_MAX_VALUE * R) + (GPIO2_EVENT - GPIO1_EVENT),
                 * where R = number of TIMERx overflow events.
                 */
                if (app_gpio2_capture_val >= app_gpio1_capture_val) {
                        /* Check if there is any overflow (to avoid the multiplication) */
                        if (app_timer_overflow_cnt) {
                                diff = ((TIMER_MAX_RELOAD_VAL * app_timer_overflow_cnt) +
                                                        (app_gpio2_capture_val - app_gpio1_capture_val));
                        } else {
                                diff = (app_gpio2_capture_val - app_gpio1_capture_val);
                        }
                /*
                 * Mathematical formula: ( (TIMER_MAX_VALUE * (R-1)) + (TIMER_MAX_VALUE - GPIO1_EVENT) + GPIO2_EVENT),
                 * where R = number of timer overflow events.
                 */
                } else if (app_gpio2_capture_val < app_gpio1_capture_val) {

                        /* Just to avoid the multiplication */
                        if (app_timer_overflow_cnt == 1) {
                                diff = ((TIMER_MAX_RELOAD_VAL - app_gpio1_capture_val) + app_gpio2_capture_val);
                        } else {
                                diff = ((TIMER_MAX_RELOAD_VAL * (--app_timer_overflow_cnt)) +
                                        (TIMER_MAX_RELOAD_VAL - app_gpio1_capture_val) + app_gpio2_capture_val);
                        }
                }
        } else if (direction == HW_TIMER_DIR_DOWN) {
                /*
                 * Mathematical formula: (ON_RELOAD * R) + (GPIO1_EVENT - GPIO2_EVENT),
                 * where R = number of timer overflapp_gpio2_capture_val     */
               if (app_gpio1_capture_val >= app_gpio2_capture_val) {
                       /* Check if there is any overflow (to avoid the multiplication) */
                       if (app_timer_overflow_cnt) {
                               diff = ((on_reload * app_timer_overflow_cnt) +
                                                       (app_gpio1_capture_val - app_gpio2_capture_val));
                       } else {
                               diff = (app_gpio1_capture_val - app_gpio2_capture_val);
                       }
              /*
               * Mathematical formula: ( (ON_RELOAD * (R-1)) + (ON_RELOAD - GPIO2_EVENT) + GPIO1_EVENT),
               * where R = number of timer overflow events.
               */
               } else if (app_gpio1_capture_val < app_gpio2_capture_val) {

                       /* Just to avoid the multiplication */
                       if (app_timer_overflow_cnt == 1) {
                               diff = ((on_reload - app_gpio2_capture_val) + app_gpio1_capture_val);
                       } else {
                               diff = ((on_reload * (--app_timer_overflow_cnt)) +
                                       (on_reload - app_gpio2_capture_val) +  app_gpio1_capture_val);
                       }
               }
        }
        hw_timer_enable_clk(id);

        ASSERT_WARNING(diff < WORD_MAX_VALUE);

        return diff;
}

/* Compute timer period in microseconds. To increase accuracy period is multiplied by 1024. */
static uint64_t app_timer_get_period_us(HW_TIMER_ID id)
{
        uint64_t freq;
        uint64_t period_us;

        /* Multiplied by 1024 to increase accuracy */
        freq = (uint64_t)app_timer_get_frequency(id) << 10;

        /* Counter period in microseconds; multiplied by 1024 to increase accuracy */
        period_us = (((uint64_t)1000000 << 20) / freq);

        return period_us;
}

/* Convert microseconds to timer ticks */
uint32_t us_to_ticks(HW_TIMER_ID id, uint32_t time_in_us)
{
        uint64_t period_us = app_timer_get_period_us(id);

        uint32_t timer_ticks = (uint32_t)(((uint64_t)period_us << 10) / time_in_us);

        return timer_ticks;
}

/* Convert timer ticks to microseconds */
uint32_t ticks_to_us(HW_TIMER_ID id, uint32_t ticks)
{
        uint64_t period_us = app_timer_get_period_us(id);

        uint32_t time_us = (uint32_t)(((uint64_t)ticks * period_us) >> 10);

        return time_us;
}

/*
 * One more capture channels should be triggered. Use masks to check
 * which channels should be read. Capture channels are mapped to bit
 * positions as follows:
 *
 *  bit position   |   3   |  2    |  1    |   0   |
 *                 +-------+-------+-------+-------+
 * capture channel | GPIO4 | GPIO3 | GPIO2 | GPIO1 |
 *                 +-------+-------+-------+-------+
 */
static void app_timer_capture_cb(uint8_t gpio_event)
{
        /* GPIO1 capture channel has been triggered  */
        if (gpio_event & APP_GPIO1_CAPTURE_STATUS_Msk) {
                app_is_timer_running = true;
                app_timer_overflow_cnt = 0;

                /* Get a snapshot of counter's value when the event took place */
                app_gpio1_capture_val = hw_timer_get_capture1(APP_TIMER_ID);
        }

        if (gpio_event & APP_GPIO2_CAPTURE_STATUS_Msk) {
                /* Make sure GPIO is already tracked! */
                if (app_is_timer_running) {
                        /* Get a snapshot of counter's value when the event took place */
                        app_gpio2_capture_val = hw_timer_get_capture2(APP_TIMER_ID);

                        app_is_timer_running = false;

                        /* Notify task so it can compute the period of the incoming pulse. */
                        OS_TASK_NOTIFY_FROM_ISR(task_h, APP_TIMER_CAPTURE_INT_NOTIF, OS_NOTIFY_SET_BITS);
                }
        }
}
#endif

static uint32_t app_timer_get_frequency(HW_TIMER_ID id)
{
        uint32_t freq;
        uint32_t prescaler;
        HW_TIMER_CLK_SRC clock_src;

        prescaler = hw_timer_get_prescaler(id);
        clock_src = hw_timer_get_clk(id);

        /* LP clock is the selected clock source */
        if (clock_src == HW_TIMER_CLK_SRC_INT) {
                freq = configSYSTICK_CLOCK_HZ / (prescaler + 1);
        /* DIVN (XTAL32M) is the selected timer clock source */
        } else {
                freq = dg_configDIVN_FREQ / (prescaler + 1);
        }

        return freq;
}

/* User callback function to be called when timer reaches zero value. */
static void app_timer_counter_cb(void)
{
#if (dg_configUSE_LP_CLK == LP_CLK_RCX)
        /*
         * Recalculate reload value as RCX might have been drifted slightly from
         * its nominal value, that is 15kHz. */
        if (APP_TIMER_DEVICE->clk_src == HW_TIMER_CLK_SRC_INT) {
                hw_timer_disable_clk(APP_TIMER_ID);
                hw_timer_set_reload(APP_TIMER_ID, app_timer_get_frequency(APP_TIMER_ID) / APP_TIMER_COUNTER_RELOAD_HZ);
                hw_timer_enable_clk(APP_TIMER_ID);
        }
#endif

#if APP_TIMER_CAPTURE_EN
        /*
         * Counter should be counting is free mode. It might happen that counter has overflowed one or multiple times
         * before an event is captured. Make sure overflow events are tracked once the first input event is captured.
         */
        if (app_is_timer_running) {
                app_timer_overflow_cnt++;
        }
#endif

        OS_TASK_NOTIFY_FROM_ISR(task_h, APP_TIMER_COUNTER_INT_NOTIF, OS_NOTIFY_SET_BITS);
}

/**
 * @brief TIMER PWM application task
 */
void app_timer_task( void *pvParameters )
{
        DBG_PRINTF("*** HW TIMER Demonstration ***\n\r");

        uint32_t notif;
        OS_BASE_TYPE ret __UNUSED;
        int8_t wdog_id __UNUSED;

        wdog_id = sys_watchdog_register(false);

        task_h = OS_GET_CURRENT_TASK();

        /*
         * If the sleep mode is enabled then a PDC entry should be registered so that the system
         * is notified (and so it can exit the sleep state) following timer's events. Keep in mind
         * that only counters' events are supported by PDC. That is:
         *
         *   - up counter mode --> counter has reached the reload value
         *   - down counter mode --> counter has reached zero
         *
         * Capture events are not supported by PDC.
         *
         */
        if (pm_sleep_mode_get() == pm_mode_extended_sleep) {
                uint32_t pdc_entry;
                HW_PDC_PERIPH_TRIG_ID trig_id = APP_TIMER_ID == HW_TIMER ? HW_PDC_PERIPH_TRIG_ID_TIMER :
                                                APP_TIMER_ID == HW_TIMER2 ? HW_PDC_PERIPH_TRIG_ID_TIMER2 :
                                                APP_TIMER_ID == HW_TIMER3 ? HW_PDC_PERIPH_TRIG_ID_TIMER3_QDEC :
                                                                            HW_PDC_PERIPH_TRIG_ID_TIMER;

                /* Check if a relevant entry has already been registered. */
                pdc_entry = hw_pdc_find_entry(HW_PDC_TRIG_SELECT_PERIPHERAL,
                                              trig_id,
                                              HW_PDC_MASTER_CM33,
                                              (dg_configENABLE_XTAL32M_ON_WAKEUP ? HW_PDC_LUT_ENTRY_EN_XTAL : 0),
                                              0);

                if (pdc_entry == HW_PDC_INVALID_LUT_INDEX) {
                        pdc_entry = hw_pdc_add_entry(HW_PDC_LUT_ENTRY_VAL(HW_PDC_TRIG_SELECT_PERIPHERAL,
                                                                          trig_id,
                                                                          HW_PDC_MASTER_CM33,
                                                                          (dg_configENABLE_XTAL32M_ON_WAKEUP ? HW_PDC_LUT_ENTRY_EN_XTAL : 0)));
                        ASSERT_WARNING(pdc_entry != HW_PDC_INVALID_LUT_INDEX);

                        /* Recommended! */
                        hw_pdc_set_pending(pdc_entry);
                        hw_pdc_acknowledge(pdc_entry);
                }
        }

        /* Setup timer so it's ready to run. */
        hw_timer_init(APP_TIMER_ID, APP_TIMER_DEVICE);
        /*
         * Define counter's reload value. Initialization is done here as the low power clock frequency is not fixed
         * when RCX is the selected sleep clock. */
        hw_timer_set_reload(APP_TIMER_ID, app_timer_get_frequency(APP_TIMER_ID) / APP_TIMER_COUNTER_RELOAD_HZ);

        /*
         * Register a user callback so users can get notified following counter's events.
         * That is, either counter has reached zero (down counting) or counter has reached
         * the reload value (up counting).
         */
        hw_timer_register_int(APP_TIMER_ID, app_timer_counter_cb);

#if APP_TIMER_CAPTURE_EN
        /*
         * Register a user callback so users can get notified following capture events on any of the capture channels
         * (GPIO1, GPIO2, GPIO3 and GPIO4).
         * It is essential that one or multiple capture channels are registered to assert the capture interrupt line.
         * In this example GPIO1 and GPIO2 are registered. This API is valid only when HW_TIMER is utilized!
         */
        hw_timer_register_capture_int(app_timer_capture_cb,
                                APP_GPIO1_CAPTURE_INT_Msk | APP_GPIO2_CAPTURE_INT_Msk);
#endif

        /* Timer should be running in free mode from this point onwards! */
        hw_timer_enable(APP_TIMER_ID);

        for (;;) {

                sys_watchdog_notify(wdog_id);
                /* Remove task id from the monitoring list as long as a task remains in the blocking state. */
                sys_watchdog_suspend(wdog_id);

                ret = OS_TASK_NOTIFY_WAIT(0x0, OS_TASK_NOTIFY_ALL_BITS, &notif,
                                                                OS_TASK_NOTIFY_FOREVER);
                ASSERT_WARNING(ret == OS_TASK_NOTIFY_SUCCESS);

                sys_watchdog_notify_and_resume(wdog_id);

#if APP_TIMER_CAPTURE_EN
                /* A capture event has take place */
                if (notif & APP_TIMER_CAPTURE_INT_NOTIF) {
                        uint32_t pulse_width = app_timer_get_events_period(APP_TIMER_ID);

                        DBG_PRINTF("Pulse width: %lu.%02lu ms\n\r",
                                ticks_to_us(APP_TIMER_ID, pulse_width) / 1000,
                                ticks_to_us(APP_TIMER_ID, pulse_width) % 1000);
                }
#endif
                /* Counter's interrupt has been fired. */
                if (notif & APP_TIMER_COUNTER_INT_NOTIF) {
                        static bool pin_status = false;

                        if (pin_status) {
                                DBG_IO_ON(LED1_PORT, LED1_PIN);
                        } else {
                                DBG_IO_OFF(LED1_PORT, LED1_PIN);
                        }

                        /* Toggle LED status */
                        pin_status ^= true;
                }
        }
}
