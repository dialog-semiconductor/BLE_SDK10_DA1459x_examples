/**
 ****************************************************************************************
 *
 * @file active_scanner_task.c
 *
 * @brief Active scanner demonstration example
 *
 * Copyright (C) 2015-2024 Renesas Electronics Corporation and/or its affiliates.
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

#include "osal.h"
#include "sys_watchdog.h"
#include "ble_att.h"
#include "ble_common.h"
#include "ble_config.h"
#include "ble_gap.h"
#include "ble_uuid.h"
#include "misc.h"
#if GENERATE_RANDOM_DEVICE_ADDRESS
# include "sys_trng.h"
# include "ad_nvms.h"
# include "platform_nvparam.h"
# include "gap.h"
#endif
#include "cli.h"
#include "console.h"
#include "cli_app.h"

#define APP_BLE_GAP_CALL_FUNC_UNTIL_NO_ERR(_func, args...) \
        {                                                  \
                ble_error_t ret;                           \
                uint32_t loop = 0;                         \
                                                           \
                while (true) {                             \
                        ret = (_func)(args);               \
                                                           \
                        if (ret == BLE_STATUS_OK) {        \
                               break;                      \
                        }                                  \
                        OS_DELAY_MS(5);                    \
                        loop++;                            \
                                                           \
                        DBG_LOG("%s failed with status = %d (%lu)", #_func, ret, loop); \
                }                                          \
        }

#define CONN_TIMEOUT_MS        ( 15000 )

/**
 * Task notifications and handles
 */
#define BLE_CLI_NOTIF                   (1 << 1)
#define BLE_SCAN_START_NOTIF            (1 << 2)
#define BLE_CONN_TIMEOUT_NOTIF          (1 << 3)
#define BLE_CTS_N_INACTIVE_NOTIF        (1 << 4)
#define BLE_CTS_N_ACTIVE_NOTIF          (1 << 5)

#define BLE_SCAN_INTERVAL               (BLE_SCAN_INTERVAL_FROM_MS(30))
#define BLE_SCAN_WINDOW                 (BLE_SCAN_WINDOW_FROM_MS(15))

__RETAINED static OS_TASK active_scanner_handle;
__RETAINED static bd_address_t peer_addr;
__RETAINED static OS_TIMER conn_timeout_h;
__RETAINED_RW static bool is_scan_allowed = true;

__RETAINED_RW static gap_conn_params_t cp = {
        .interval_min  = defaultBLE_PPCP_INTERVAL_MIN,   // in unit of 1.25ms
        .interval_max  = defaultBLE_PPCP_INTERVAL_MAX,   // in unit of 1.25ms
        .slave_latency = defaultBLE_PPCP_SLAVE_LATENCY,
        .sup_timeout   = defaultBLE_PPCP_SUP_TIMEOUT,    // in unit of 10ms
};

/*  Current connection index */
__RETAINED_RW static uint16_t conn_idx = BLE_CONN_IDX_INVALID;

static void device_set_random_address(void)
{
#if GENERATE_RANDOM_DEVICE_ADDRESS
        own_address_t addr;
        ble_gap_address_get(&addr);

        const uint8_t default_addr[] = defaultBLE_STATIC_ADDRESS;

        if (memcmp(addr.addr, default_addr, BD_ADDR_LEN) == 0) {

                uint8_t offset = NVPARAM_OFFSET_BLE_PLATFORM_BD_ADDRESS;
                nvms_t nvms;

                /* Generate a random address */
                for (int i = 0; i < BD_ADDR_LEN; i++) {
                        addr.addr[i] = (uint8_t)rand();
                }

                /* The two MSBs of a random static address have to be 1 */
                addr.addr[BD_ADDR_LEN - 1] |= GAP_STATIC_ADDR;

                nvms = ad_nvms_open(NVMS_PARAM_PART);
                if (!nvms) {
                        DBG_LOG("\r\nCannot open NVMS_PARAM_PART, storing device address failed.\r\n");
                        return;
                }

                ad_nvms_write(nvms, offset, addr.addr, BD_ADDR_LEN);
                ad_nvms_write(nvms, BD_ADDR_LEN, &offset, sizeof(offset));

                /*
                 * SDK assumes that a stored address is public so we need to
                 * check for random static address and set the correct type.
                 */
                if ((addr.addr[BD_ADDR_LEN - 1] & 0xC0) == GAP_STATIC_ADDR) {
                        addr.addr_type = PRIVATE_STATIC_ADDRESS;
                        ble_gap_address_set(&addr, 0);
                }
        }
#endif /* GENERATE_RANDOM_DEVICE_ADDRESS */
}

static void device_get_random_address(void)
{
        own_address_t addr;
        ble_gap_address_get(&addr);

        DBG_LOG("Device address: ");

        /* Address should be displayed inverted */
        for (int i = BD_ADDR_LEN - 1; i >= 0; --i) {
                DBG_LOG("%02X", addr.addr[i]);
                if (i != 0) {
                        DBG_LOG(":");
                }
        }
        DBG_LOG("\n\rAddress type = %d\n\r", addr.addr_type);
}

static void handle_evt_gap_adv_report(ble_evt_gap_adv_report_t *evt)
{
        int tlv_idx = 0;
        int data_idx = 0;

        DBG_LOG("\n\r****** BD address = %s *******\n\r", ble_address_to_string(&evt->address));

        /* Process TLVs */
        while (tlv_idx + 2 < evt->length) {
                /*
                 * The 1st byte should reflect the number of bytes that follow for the specific data type.
                 * The 2nd byte should reflect the data type (should be interpreted using the gap_data_type_t enumerator).
                 */
                DBG_LOG("\n\r[%d]. Length = %d, Type = %d, Data = ", data_idx, evt->data[tlv_idx], evt->data[tlv_idx + 1]);

                for (int i = 0; i < evt->data[tlv_idx]; i++) {
                        DBG_LOG("0x%02X ", evt->data[tlv_idx + 2 + i]);
                }
                DBG_LOG("\n\r");

                data_idx++;
                tlv_idx += (evt->data[tlv_idx] + 1); // go to the next TLV
        }
}

static void handle_evt_gap_connection_completed(ble_evt_gap_connection_completed_t *evt)
{
        DBG_LOG("%s, Status = %d\n\r", __func__, evt->status);

        /*
         * Should reach here if a connection request is aborted by the user
         * explicitly by calling ble_gap_connect_cancel().
         */
        if (evt->status == BLE_ERROR_CANCELED) {

        }
}

static void handle_evt_gap_scan_completed(ble_evt_gap_scan_completed_t *evt)
{
        DBG_LOG("%s, status = %d\n\r", __func__, evt->status);

        /* Process only if scanner was canceled by user. */
        if (evt->status == BLE_ERROR_CANCELED) {
                ble_error_t ret = ble_gap_connect_ce((const bd_address_t *)&peer_addr, &cp,
                                                                defaultBLE_CONN_EVENT_LENGTH_MIN, 0);

                if (ret == BLE_ERROR_BUSY) {
                        DBG_LOG("%s failed with status BLE_ERROR_CANCELED.\n\r", __func__);

                        /*
                         * ble_gap_connect_ce() will return a BLE_ERROR_BUSY status when another connection
                         * procedure is already ongoing. To be able to connect to another device, cancel
                         * the last connection request to retry connecting with the new device.
                         */
                        ble_gap_connect_cancel();
                } else {
                        ASSERT_WARNING(ret == BLE_STATUS_OK);

                        /*
                         * A connection establishment should now be initiated. Start a timer
                         * which will be in charge of making sure that the connection request
                         * is canceled after some time has elapsed and the connection has yet
                         * to be established. It might happen that the requested peer address
                         * is no longer available.
                         */
                        OS_TIMER_START(conn_timeout_h, OS_TIMER_FOREVER);
                }
        } else {
                ASSERT_WARNING(evt->status == BLE_ERROR_TIMEOUT);

                /* Scanning operations should timeout, now. */
                is_scan_allowed = true;
        }
}

/*
 * BLE event handlers
 */
static void handle_evt_gap_connected(ble_evt_gap_connected_t *evt)
{
        DBG_LOG("%s: conn_idx=%04x address=%s CI max is %u. \r\n", __func__, evt->conn_idx, \
                        ble_address_to_string(&evt->peer_address), evt->conn_params.interval_max);

        conn_idx = evt->conn_idx;

        ASSERT_WARNING(OS_TIMER_IS_ACTIVE(conn_timeout_h));

        /* Connection has been established; stop connection timer. */
        OS_TIMER_STOP(conn_timeout_h, OS_TIMER_FOREVER);
}

static void handle_evt_gap_conn_param_updated_req(ble_evt_gap_conn_param_update_req_t * evt)
{
        DBG_LOG("Central rejected connection parameter update.\r\n");

        /* Reject the connection parameter update */
        ble_gap_conn_param_update_reply(evt->conn_idx, false);
}

static void handle_evt_gap_disconnected(ble_evt_gap_disconnected_t *evt)
{
        DBG_LOG("%s: conn_idx=%04x address=%s reason=%d\r\n", __func__,
                                        evt->conn_idx, ble_address_to_string(&evt->address), evt->reason);

        conn_idx = BLE_CONN_IDX_INVALID;
}

static void handle_evt_gap_security_request(ble_evt_gap_security_request_t *evt)
{
        DBG_LOG("%s: conn_idx=%04x bond=%d\r\n", __func__, evt->conn_idx, evt->bond);

        /* Trigger pairing */
        ble_gap_pair(evt->conn_idx, evt->bond);
}

static void handle_evt_gap_pair_completed(ble_evt_gap_pair_completed_t *evt)
{
        DBG_LOG("%s: conn_idx=%04x status=%d bond=%d mitm=%d\r\n",
                                __func__, evt->conn_idx, evt->status, evt->bond, evt->mitm);
}

static void scan_start(void)
{
        if (is_scan_allowed) {
                is_scan_allowed = false;

                APP_BLE_GAP_CALL_FUNC_UNTIL_NO_ERR(ble_gap_scan_start,
                        GAP_SCAN_ACTIVE, GAP_SCAN_GEN_DISC_MODE, BLE_SCAN_INTERVAL, BLE_SCAN_WINDOW, false, false);
        }
}

/* Connection timer callback */
static void conn_timeout_cb(OS_TIMER xTimer)
{
        OS_TASK task = (OS_TASK) OS_TIMER_GET_TIMER_ID(xTimer);

        OS_TASK_NOTIFY(task, BLE_CONN_TIMEOUT_NOTIF, OS_NOTIFY_SET_BITS);
}

static void cli_active_scanner_start_handler(int argc, const char *argv[], void *user_data)
{
        if (argc != 1) {
                DBG_LOG("Command does not support arguments. Discarding following arguments...\n\r");
        }

        OS_TASK_NOTIFY(active_scanner_handle, BLE_SCAN_START_NOTIF, OS_NOTIFY_SET_BITS);
}

static const cli_command_t cli_cmd_handlers[] = {
        {"active_scanner_start", cli_active_scanner_start_handler, NULL},
        {} //! A null entry is required to indicate the ending point
};

/* CLI handler to be called when no handler has been matched for the requested task. */
static void cli_default_handler(int argc, const char *argv[], void *user_data)
{
        DBG_LOG("Invalid command - retry...\n\r");
}

static void app_gpio0_cb(uint32_t status)
{
#if dg_configUSE_CONSOLE
        if (status & (1 << SER1_CTS_PIN)) {
                HW_WKUP_TRIGGER trigger;

                /* Invert trigger logic */
                trigger = ((hw_wkup_get_trigger(SER1_CTS_PORT, SER1_CTS_PIN) == HW_WKUP_TRIG_EDGE_LO) ?
                        HW_WKUP_TRIG_EDGE_HI : HW_WKUP_TRIG_EDGE_LO);
                hw_wkup_set_trigger(SER1_CTS_PORT, SER1_CTS_PIN, trigger);

                OS_TASK_NOTIFY_FROM_ISR(active_scanner_handle, trigger == HW_WKUP_TRIG_EDGE_LO ?
                        BLE_CTS_N_INACTIVE_NOTIF :
                        BLE_CTS_N_ACTIVE_NOTIF, OS_NOTIFY_SET_BITS);
        }
#endif
}

OS_TASK_FUNCTION(active_scanner_task, pvParameters)
{
        int8_t wdog_id;
        cli_t cli;

        active_scanner_handle = OS_GET_CURRENT_TASK();

        wdog_id = sys_watchdog_register(false);

        cli = cli_register(BLE_CLI_NOTIF, cli_cmd_handlers, cli_default_handler);
        cli_app_gpio0_register_cb(app_gpio0_cb);

        /* Initiate the BLE controller in the master role */
        ble_central_start();
        /* Register the current task to the BLE manager so the first can be notified for incoming BLE events. */
        ble_register_app();

        /* \warning This API should be called prior to creating the DB! */
        ble_gap_device_name_set("RENESAS Active Scanner", ATT_PERM_READ);

        /* \warning This API should be called prior to creating the DB! */
        device_set_random_address();
        device_get_random_address();

        conn_timeout_h = OS_TIMER_CREATE("CONN_TIMEOUT", OS_MS_2_TICKS(CONN_TIMEOUT_MS), OS_TIMER_FAIL,
                                                                        OS_GET_CURRENT_TASK(), conn_timeout_cb);
        ASSERT_WARNING(conn_timeout_h);

        for (;;) {
                OS_BASE_TYPE ret;
                uint32_t notif;

                /* Notify watchdog on each loop */
                sys_watchdog_notify(wdog_id);

                /* Suspend watchdog while blocking on OS_TASK_NOTIFY_WAIT() */
                sys_watchdog_suspend(wdog_id);

                /*
                 * Wait on any of the notification bits, then clear them all
                 */
                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                /* Blocks forever waiting for task notification. The return value must be OS_OK */
                OS_ASSERT(ret == OS_OK);

                /* Resume watchdog */
                sys_watchdog_notify_and_resume(wdog_id);

                /* notified from BLE manager, can get event */
                if (notif & BLE_APP_NOTIFY_MASK) {
                        ble_evt_hdr_t *hdr;

                        hdr = ble_get_event(false);
                        if (!hdr) {
                                goto no_event;
                        }

                        switch (hdr->evt_code) {
                        case BLE_EVT_GAP_CONNECTED:
                                handle_evt_gap_connected((ble_evt_gap_connected_t *) hdr);
                                break;
                        case BLE_EVT_GAP_ADV_REPORT:
                                handle_evt_gap_adv_report((ble_evt_gap_adv_report_t *) hdr);
                                break;
                        case BLE_EVT_GAP_SCAN_COMPLETED:
                                handle_evt_gap_scan_completed((ble_evt_gap_scan_completed_t *) hdr);
                                break;
                        case BLE_EVT_GAP_CONNECTION_COMPLETED:
                                handle_evt_gap_connection_completed((ble_evt_gap_connection_completed_t *) hdr);
                                break;
                        case BLE_EVT_GAP_CONN_PARAM_UPDATE_REQ:
                                handle_evt_gap_conn_param_updated_req((ble_evt_gap_conn_param_update_req_t *) hdr);
                                break;
                        case BLE_EVT_GAP_DISCONNECTED:
                                handle_evt_gap_disconnected((ble_evt_gap_disconnected_t *) hdr);
                                break;
                        case BLE_EVT_GAP_SECURITY_REQUEST:
                                handle_evt_gap_security_request((ble_evt_gap_security_request_t *) hdr);
                                break;
                        case BLE_EVT_GAP_PAIR_COMPLETED:
                                handle_evt_gap_pair_completed((ble_evt_gap_pair_completed_t *) hdr);
                                break;
                        default:
                                ble_handle_event_default(hdr);
                                break;
                        }

                        OS_FREE(hdr);

no_event:
                        /* Notify again if there are more events to process in queue */
                        if (ble_has_event()) {
                                OS_TASK_NOTIFY(active_scanner_handle, BLE_APP_NOTIFY_MASK, OS_NOTIFY_SET_BITS);
                        }
                }

                if (notif & BLE_CTS_N_ACTIVE_NOTIF) {
                        DBG_LOG("CTS line asserted. Notifying the console task...\n\r");

                        console_wkup_handler();
                }

                if (notif & BLE_CLI_NOTIF) {
                        cli_handle_notified(cli);
                }

                if (notif & BLE_SCAN_START_NOTIF) {
                        DBG_LOG("Scan started...\n\r");
                        scan_start();
                }

                if (notif & BLE_CONN_TIMEOUT_NOTIF) {
                        ble_error_t status __UNUSED;
                        status = ble_gap_connect_cancel();

                        ASSERT_WARNING(status == BLE_STATUS_OK);
                }
        }
}
