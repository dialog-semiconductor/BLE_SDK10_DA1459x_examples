/**
 ****************************************************************************************
 *
 * @file glucose_sensor_task.c
 *
 * @brief Glucose sensor application implementation
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

#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include "osal.h"
#include "ble_att.h"
#include "ble_common.h"
#include "ble_gap.h"
#include "ble_gatts.h"
#include "ble_l2cap.h"
#include "sdk_list.h"
#include "sys_power_mgr.h"
#include "ad_nvparam.h"
#include "sys_watchdog.h"
#include "app_nvparam.h"

#if dg_configSUOTA_SUPPORT
#include "dlg_suota.h"
#include "sw_version.h"
#endif

#include "dis.h"
#include "glucose_service.h"
#include "svc_types.h"
#include "misc.h"
#include "gap.h"
#include "glucose_sensor_database.h"

/*
 * Notification bits reservation
 *
 * Bit #0 is always assigned to BLE event queue notification.
 */
#define ADV_TMO_NOTIF                   (1 << 1)
#define UPDATE_CONN_PARAM_NOTIF         (1 << 2)
#define FEATURE_TMO_NOTIF               (1 << 3)
#define RECORD_UPDATE_TMO_NOTIF         (1 << 4)
#define RACP_DELETE_RECORDS_NOTIF       (1 << 5)
#define RACP_REPORT_RECORDS_NOTIF       (1 << 6)
#define RACP_REPORT_NUM_NOTIF           (1 << 7)

/*
 * The maximum length of name in scan response
 */
#define MAX_NAME_LEN         (BLE_SCAN_RSP_LEN_MAX - 2)

#define UUID_SUOTA_LSB       (dg_configBLE_UUID_SUOTA_SERVICE & 0xFF)
#define UUID_SUOTA_MSB       ((dg_configBLE_UUID_SUOTA_SERVICE & 0xFF00) >> 8)

#define UUID_GLS_LSB          (UUID_GLUCOSE_SERVICE & 0xFF)
#define UUID_GLS_MSB          ((UUID_GLUCOSE_SERVICE & 0xFF00) >> 8)

/* Name can not be longer than 29 bytes (BLE_SCAN_RSP_LEN_MAX - 2 bytes)*/
#define GLUCOSE_SENSOR_DEFAULT_NAME        "Renesas Glucose Sensor"

/*
 * When set to non-zero, SUOTA build will request longer connection interval in order to reduce
 * power consumption when connection is active and change back to short connection interval before
 * starting SUOTA process. This however seem to cause interoperability problems with some devices.
 *
 * This parameter is not applicable for non-SUOTA builds since they will always use longer connection
 * interval when possible.
 */
#define GLUCOSE_SENSOR_SUOTA_POWER_SAVING  ( 0 )

#define GLS_DATABASE_UPDATE_MS    ( 10000 )

/*
 * Recommended advertising interval values as defined by specifications. By default,
 * "fast connection" is used.
 */
static const struct {
        uint16_t min;
        uint16_t max;
} adv_intervals[2] = {
        // "fast connection" interval values
        {
                .min = BLE_ADV_INTERVAL_FROM_MS(20),      // 20ms
                .max = BLE_ADV_INTERVAL_FROM_MS(30),      // 30ms
        },
        // "reduced power" interval values
        {
                .min = BLE_ADV_INTERVAL_FROM_MS(1000),    // 1000ms
                .max = BLE_ADV_INTERVAL_FROM_MS(2500),    // 2500ms
        }
};

typedef struct {
        /* Should always be the first element; will be used by the list framework internally. */
        void           *next;

        bool            expired;
        uint16_t        conn_idx;
        OS_TIMER        param_timer;
        OS_TASK         current_task;
} conn_dev_t;

typedef enum {
        ADV_INTERVAL_FAST = 0,
        ADV_INTERVAL_POWER = 1,
} adv_setting_t;

/*
 * The Glucose Profile specifications suggests that GLS UUID should be part of the advertising data.
 * Device name is set in scan response to make it easily recognizable as suggested by GLP.
 */
static const gap_adv_ad_struct_t adv_data[] = {
#if dg_configSUOTA_SUPPORT
        GAP_ADV_AD_STRUCT_BYTES(GAP_DATA_TYPE_UUID16_LIST_INC,
                                UUID_GLS_LSB, UUID_GLS_MSB,  // = 0x1808 (GLS UUID)
                                UUID_SUOTA_LSB, UUID_SUOTA_MSB)
#else
        GAP_ADV_AD_STRUCT_BYTES(GAP_DATA_TYPE_UUID16_LIST_INC,
                                UUID_GLS_LSB, UUID_GLS_MSB)  // = 0x1808 (GLS UUID)
#endif
};

#if dg_configSUOTA_SUPPORT
/*
 * Store information about ongoing SUOTA.
 */
__RETAINED_RW static bool suota_ongoing = false;
#endif

__RETAINED static OS_TASK gls_task_h;

/* Current advertising setting */
__RETAINED_RW static adv_setting_t adv_setting = ADV_INTERVAL_FAST;

/* Timer used to switch from "fast connection" to "reduced power" advertising intervals */
__RETAINED static OS_TIMER adv_tim;

/* List of devices waiting for connection parameters update */
__RETAINED static void *param_connections;

/* Buffer must have length at least max_len + 1 */
static uint16_t read_name(uint16_t max_len, char *name_buf)
{
        uint16_t read_len = 0;

#if dg_configNVPARAM_ADAPTER
        nvparam_t param;
        param = ad_nvparam_open("ble_app");
        read_len = ad_nvparam_read(param, TAG_BLE_APP_NAME, max_len, name_buf);
        ad_nvparam_close(param);
#endif /* dg_configNVPARAM_ADAPTER */

        if (read_len == 0) {
                strcpy(name_buf, GLUCOSE_SENSOR_DEFAULT_NAME);
                return strlen(GLUCOSE_SENSOR_DEFAULT_NAME);
        }

        name_buf[read_len] = '\0';

        return read_len;
}

/* Advertising intervals change timeout timer callback */
static void adv_tim_cb(OS_TIMER timer)
{
        OS_TASK task = (OS_TASK) OS_TIMER_GET_TIMER_ID(timer);

        OS_TASK_NOTIFY(task, ADV_TMO_NOTIF, OS_NOTIFY_SET_BITS);
}

#if GLS_FEATURE_INDICATION_PROPERTY
static void feature_tim_cb(OS_TIMER timer)
{
        OS_TASK task = (OS_TASK) OS_TIMER_GET_TIMER_ID(timer);

        OS_TASK_NOTIFY(task, FEATURE_TMO_NOTIF, OS_NOTIFY_SET_BITS);
}
#endif

static void record_tim_cb(OS_TIMER timer)
{
        OS_TASK task = (OS_TASK) OS_TIMER_GET_TIMER_ID(timer);

        OS_TASK_NOTIFY(task, RECORD_UPDATE_TMO_NOTIF, OS_NOTIFY_SET_BITS);
}

static void set_advertising_interval(adv_setting_t setting)
{
        uint16_t min = adv_intervals[setting].min;
        uint16_t max = adv_intervals[setting].max;

        /* Save current advertising setting */
        adv_setting = setting;

        ble_gap_adv_intv_set(min, max);
}

static void start_adv(void)
{
        gap_device_t devices[BLE_GAP_MAX_CONNECTED];
        size_t length = ARRAY_LENGTH(devices);

        ble_gap_get_devices(GAP_DEVICE_FILTER_CONNECTED, NULL, &length, devices);
        if (length == ARRAY_LENGTH(devices)) {
                /*
                 * We reached maximum number of connected devices, don't start advertising
                 * now. Advertising will start again once any device will disconnect.
                 */
                return;
        }

        ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);
        if (adv_setting == ADV_INTERVAL_FAST) {
                /* Start timer to switch to ADV_INTERVAL_POWER after 30 s */
                OS_TIMER_START(adv_tim, OS_TIMER_FOREVER);
        }
}

/* Match connection by connection index */
static bool conn_params_match(const void *elem, const void *ud)
{
        conn_dev_t *conn_dev = (conn_dev_t *) elem;
        uint16_t conn_idx = (uint16_t) (uint32_t) ud;

        return conn_dev->conn_idx == conn_idx;
}

/*
 * This timer callback notifies the task that time for discovery, bonding and encryption
 * elapsed, and connection parameters can be changed to the preferred ones.
 */
static void conn_params_timer_cb(OS_TIMER timer)
{
        conn_dev_t *conn_dev = (conn_dev_t *) OS_TIMER_GET_TIMER_ID(timer);;

        conn_dev = list_find(param_connections, conn_params_match,
                                                (const void *) (uint32_t) conn_dev->conn_idx);
        if (conn_dev) {
                conn_dev->expired = true;
                OS_TASK_NOTIFY(conn_dev->current_task, UPDATE_CONN_PARAM_NOTIF,
                                                                        OS_NOTIFY_SET_BITS);
        }
}

#if GLUCOSE_SENSOR_SUOTA_POWER_SAVING
/* Update connection parameters */
static void conn_param_update(uint16_t conn_idx)
{
        gap_conn_params_t cp;

        cp.interval_min = defaultBLE_PPCP_INTERVAL_MIN;
        cp.interval_max = defaultBLE_PPCP_INTERVAL_MAX;
        cp.slave_latency = defaultBLE_PPCP_SLAVE_LATENCY;
        cp.sup_timeout = defaultBLE_PPCP_SUP_TIMEOUT;

        ble_gap_conn_param_update(conn_idx, &cp);
}
#endif

#if dg_configSUOTA_SUPPORT && GLUCOSE_SENSOR_SUOTA_POWER_SAVING
/* Update connection parameters for SUOTA */
static void conn_param_update_for_suota(uint16_t conn_idx)
{
        gap_conn_params_t cp;

        cp.interval_min = BLE_CONN_INTERVAL_FROM_MS(20);    // 20ms
        cp.interval_max = BLE_CONN_INTERVAL_FROM_MS(60);    // 60ms
        cp.slave_latency = 0;
        cp.sup_timeout = BLE_SUPERVISION_TMO_FROM_MS(2000); // 2000ms

        ble_gap_conn_param_update(conn_idx, &cp);
}
#endif

static void handle_evt_gap_connected(ble_evt_gap_connected_t *evt)
{
        conn_dev_t *conn_dev;

        /* Add timer that when expired will re-negotiate connection parameters */
        conn_dev = OS_MALLOC(sizeof(*conn_dev));
        if (conn_dev) {
                conn_dev->conn_idx = evt->conn_idx;
                conn_dev->expired = false;
                conn_dev->current_task = OS_GET_CURRENT_TASK();
                conn_dev->param_timer = OS_TIMER_CREATE("conn_param", OS_MS_2_TICKS(5000),
                                        OS_TIMER_FAIL, (uint32_t) conn_dev, conn_params_timer_cb);
                list_append(&param_connections, conn_dev);
                OS_TIMER_START(conn_dev->param_timer, OS_TIMER_FOREVER);
        }
}

static void handle_evt_gap_disconnected(ble_evt_gap_disconnected_t *evt)
{
        ble_error_t ret;
        conn_dev_t *conn_dev = list_unlink(&param_connections, conn_params_match,
                                                        (const void *) (uint32_t) evt->conn_idx);

        /*
         * The device is still in the list if the disconnection happened before the timer expiration.
         * In this case stop the timer and free the associated memory.
         */
        if (conn_dev) {
                OS_TIMER_DELETE(conn_dev->param_timer, OS_TIMER_FOREVER);
                OS_FREE(conn_dev);
        }

        /* Switch back to fast advertising interval */
        set_advertising_interval(ADV_INTERVAL_FAST);
        ret = ble_gap_adv_stop();
        /* Advertising isn't started, start it now */
        if (ret == BLE_ERROR_NOT_ALLOWED) {
                start_adv();
        }
}

static void handle_evt_gap_adv_completed(ble_evt_gap_adv_completed_t *evt)
{
        /*
         * If advertising is completed, just restart it. It's either because a new client connected
         * or it was cancelled in order to change the interval values.
         */
#if dg_configSUOTA_SUPPORT
        /* If SUOTA is ongoing, don't restart advertising */
        if (suota_ongoing) {
                return;
        }
#endif
        start_adv();
}

static void handle_evt_gap_pair_req(ble_evt_gap_pair_req_t *evt)
{
        /* Just accept the pairing request, set bond flag to what peer requested */
        ble_gap_pair_reply(evt->conn_idx, true, evt->bond);
}

static void print_bonded_devices(void)
{
        size_t bonded_length = defaultBLE_MAX_BONDED;
        static gap_device_t devices[defaultBLE_MAX_BONDED];
        uint8_t i;

        ble_gap_get_devices(GAP_DEVICE_FILTER_BONDED, NULL, &bonded_length, devices);

        printf("\r\n");
        printf("Bonded devices:\r\n");

        if (!bonded_length) {
                printf("(no bonded devices)\r\n");
                printf("\r\n");
                return;
        }

        printf("Nr | Address \r\n");

        for (i = 0; i < bonded_length; ++i) {
                printf("%2d | %17s\r\n", (i + 1), ble_address_to_string(&devices[i].address));
        }

        printf("\r\n");
}

static void handle_evt_gap_pair_completed(ble_evt_gap_pair_completed_t *evt)
{
        if (evt->status) {
                DBG_PRINTF("Pair failed\r\n");
                DBG_PRINTF("\tConnection Index: %d\r\n", evt->conn_idx);
                DBG_PRINTF("\tStatus: 0x%02X\r\n", evt->status);
        } else {
                DBG_PRINTF("Pair completed\r\n");
                DBG_PRINTF("\tConnection index: %u\r\n", evt->conn_idx);
                DBG_PRINTF("\tStatus: 0x%02u\r\n", evt->status);
                DBG_PRINTF("\tBond: %s\r\n", evt->bond ? "true" : "false");
                DBG_PRINTF("\tMITM: %s\r\n", evt->mitm ? "true" : "false");
        }

        if (evt->bond) {
                print_bonded_devices();
        }
}

/* Display the passkey as provided by the peer device, part of the pairing procedure.
 * This value will be displayed on the serial console and then should be typed in
 * the pop-up window of the master device. */
static void handle_evt_gap_passkey_notify(ble_evt_gap_passkey_notify_t * evt)
{
        DBG_PRINTF("Passkey notify\r\n");
        DBG_PRINTF("\tConnection index: %d\r\n", evt->conn_idx);
        DBG_PRINTF("\tPasskey=%06" PRIu32 "\r\n", evt->passkey);
}

#if dg_configSUOTA_SUPPORT
#if (dg_configBLE_2MBIT_PHY == 1)
static void handle_ble_evt_gap_phy_set_completed(ble_evt_gap_phy_set_completed_t* evt)
{
        /*
         * Add code to handle the case where the preferred PHY settings
         * failed to be set (evt->status != BLE_STATUS_OK).
         */
}

static void handle_ble_evt_gap_phy_changed(ble_evt_gap_phy_changed_t* evt)
{
        /*
         * Add code to handle the PHY changed event.
         */
}
#endif /* (dg_configBLE_2MBIT_PHY == 1) */

static bool suota_ready_cb(uint16_t conn_idx)
{
        /*
         * This callback is used so that the application can accept or block SUOTA.
         * Also, before SUOTA starts, the user might want to do some actions,
         * e.g. disable sleep mode.
         *
         * If `true` is returned, then advertising is stopped and SUOTA is started.
         * Otherwise SUOTA is canceled.
         */
        suota_ongoing = true;

#if GLUCOSE_SENSOR_SUOTA_POWER_SAVING
        /*
         * We need to decrease the connection interval for SUOTA so that data can be transferred
         * quickly.
         */
        conn_param_update_for_suota(conn_idx);
#endif

#if (dg_configBLE_2MBIT_PHY == 1)
        /* Switch to 2Mbit PHY during SUOTA */
        ble_gap_phy_set(conn_idx, BLE_GAP_PHY_PREF_2M, BLE_GAP_PHY_PREF_2M);
#endif /* (dg_configBLE_2MBIT_PHY == 1) */

        return true;
}

static void suota_status_changed_cb(uint16_t conn_idx, uint8_t status, uint8_t error_code)
{
        /* In case SUOTA finished with an error, we just restore default connection parameters. */
        if (status != SUOTA_ERROR) {
                return;
        }

#if GLUCOSE_SENSOR_SUOTA_POWER_SAVING
        conn_param_update(conn_idx);
#endif

#if (dg_configBLE_2MBIT_PHY == 1)
        /* Switch to Auto PHY when SUOTA is completed */
        ble_gap_phy_set(conn_idx, BLE_GAP_PHY_PREF_AUTO, BLE_GAP_PHY_PREF_AUTO);
#endif /* (dg_configBLE_2MBIT_PHY == 1) */
}

static const suota_callbacks_t suota_cb = {
        .suota_ready = suota_ready_cb,
        .suota_status = suota_status_changed_cb,
};
#endif /* dg_configSUOTA_SUPPORT */

/* In order to encapsulate a Bluetooth Device Address as System ID, the Company Identifier (OUI)
 * is concatenated with 0xFFFE followed by the Company Assigned Identifier
 * (manufacturer assigned identifier). */
static const dis_system_id_t system_id = {
        .manufacturer = {0xA5, 0xA5, 0xA5, 0xFE, 0xFF}, // The first 24 bits are arbitrary assigned
        .oui = {0x50, 0x90, 0x74},    // Defined by IEEE, 0x749050
};

/* Device Information Service data */
static const dis_device_info_t dis_info = {
        .manufacturer  = defaultBLE_DIS_MANUFACTURER,   // Mandatory as per GLP
        .model_number  = defaultBLE_DIS_MODEL_NUMBER,   // Mandatory as per GLP
        .serial_number = defaultBLE_DIS_SERIAL_NUMBER,
        .hw_revision   = defaultBLE_DIS_HW_REVISION,
        .fw_revision   = defaultBLE_DIS_FW_REVISION,
#ifdef SW_VERSION
        .sw_revision   = SW_VERSION,
#else
        .sw_revision   = defaultBLE_DIS_SW_REVISION,
#endif /* SW_VERSION */
        .system_id     = &system_id, // Mandatory as per GLP
};

static void update_device_address(void)
{
        own_address_t addr;
        ble_gap_address_get(&addr);

        static const uint8_t default_addr[] = defaultBLE_STATIC_ADDRESS;

        if (memcmp(addr.addr, default_addr, BD_ADDR_LEN) == 0) {

                uint8_t offset = NVPARAM_OFFSET_BLE_PLATFORM_BD_ADDRESS;
                nvms_t nvms;

                /* Generate a random address */
                /* Generate a random address */
                for (int i = 0; i < BD_ADDR_LEN; i++) {
                        addr.addr[i] = (uint8_t)rand();
                }

                /* The two MSBs of a random static address have to be 1 */
                addr.addr[BD_ADDR_LEN - 1] |= GAP_STATIC_ADDR; // Need to check if the same enumeration is available in sdk and not in sdk

                nvms = ad_nvms_open(NVMS_PARAM_PART);
                if (!nvms) {
                        DBG_PRINTF("\r\nCannot open NVMS_PARAM_PART, storing device address failed.\r\n");
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
}

static void print_device_address(void)
{
        own_address_t addr;
        ble_gap_address_get(&addr);

        DBG_PRINTF("Device address: ");

        /* Address should be displayed inverted */
        for (int i = BD_ADDR_LEN - 1; i >= 0; --i) {
                DBG_PRINTF("%02X", addr.addr[i]);
                if (i != 0) {
                        DBG_PRINTF(":");
                }
        }
        DBG_PRINTF("\n\rAddress type = %d\n\r", addr.addr_type);
}

#if GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT
static void delete_records_cb(ble_service_t *svc, uint16_t conn_idx, gls_racp_t *record)
{
        /* Get the RACP request as after exiting this function the record will be lost */
        app_db_update_racp_request(svc, conn_idx, GLS_RACP_COMMAND_DELETE_RECORDS, record);

        DBG_PRINTF("%s, Operator: %d, filter type: %d, operand size: %d\n\r",
                                __func__, record->operator, record->filter_type, record->filter_param_len);

        /* Parsing should be done in the application's task context so BLE Manager's task can process other
         * BLE events. Defer further processing by sending notifications. */
        OS_TASK_NOTIFY(gls_task_h, RACP_DELETE_RECORDS_NOTIF, OS_NOTIFY_SET_BITS);
}
#endif

static void report_num_of_records_cb(ble_service_t *svc, uint16_t conn_idx, gls_racp_t *record)
{
        /* Get the RACP request as after exiting this function the record will be lost */
        app_db_update_racp_request(svc, conn_idx, GLS_RACP_COMMAND_NUMBER_OF_RECORDS, record);

        DBG_PRINTF("%s, Operator: %d, filter type: %d, operand size: %d\n\r",
                                __func__, record->operator, record->filter_type, record->filter_param_len);

        /* Parsing should be done in the application's task context so BLE Manager's task can process other
         * BLE events. Defer further processing by sending notifications. */
        OS_TASK_NOTIFY(gls_task_h, RACP_REPORT_NUM_NOTIF, OS_NOTIFY_SET_BITS);
}

static void report_records_cb(ble_service_t *svc, uint16_t conn_idx, gls_racp_t *record)
{
        /* Get the RACP request as after exiting this function the record will be lost */
        app_db_update_racp_request(svc, conn_idx, GLS_RACP_COMMAND_REPORT_RECORDS, record);

        DBG_PRINTF("%s, Operator: %d, filter type: %d, operand size: %d\n\r",
                                __func__, record->operator, record->filter_type, record->filter_param_len);

        /* Parsing should be done in the application's task context so BLE Manager's task can process other
         * BLE events. Defer further processing by sending notifications. */
        OS_TASK_NOTIFY(gls_task_h, RACP_REPORT_RECORDS_NOTIF, OS_NOTIFY_SET_BITS);
}

static void abort_operation_cb(ble_service_t *svc, uint16_t conn_idx)
{
        DBG_PRINTF("%s\n", __func__);
        gls_indicate_abort_operation_status(svc, conn_idx, GLS_RACP_RESPONSE_SUCCESS);
}

static void event_sent_cb(ble_service_t *svc, const ble_evt_gatts_event_sent_t *evt)
{
}

#if GLS_FEATURE_INDICATION_PROPERTY
static void get_features_cb(ble_service_t *svc, uint16_t conn_idx)
{
        gls_get_feature_cfm(svc, conn_idx, ATT_ERROR_OK, GLS_FEATURE_CHARACTERISTIC_VALUE);
}
#endif

__RETAINED_RW static gls_callbacks_t gls_callbacks = {
        .event_sent = event_sent_cb,
#if GLS_FEATURE_INDICATION_PROPERTY
        .get_features = get_features_cb,
#endif
        .racp_callbacks = {
                .report_num_of_records = report_num_of_records_cb,
                .report_records = report_records_cb,
                .abort_operation = abort_operation_cb,
#if GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT
                .delete_records = delete_records_cb,
#endif
        },
};

/* Helper function to initialize a record entry */
static void init_record_entry_cb(gls_record_t *record)
{
        ASSERT_WARNING(record);

        __unused svc_ieee11073_float_t ieee_float = {
                .mantissa = 5,
        };

        record->measurement.flags = GLS_FLAGS_FIELD_VALUE;

        record->measurement.seq_number = app_db_get_sequence_number();

        /* Typically this value is fixed and does not change over the lifetime of the device unless a serious
         * malfunctions occurs. */
        record->measurement.base_time.year = APP_DB_BASE_TIME_YEAR;
        record->measurement.base_time.month = APP_DB_BASE_TIME_MONTH;
        record->measurement.base_time.day = APP_DB_BASE_TIME_DAY;
        record->measurement.base_time.hours = APP_DB_BASE_TIME_HOURS;
        record->measurement.base_time.minutes = APP_DB_BASE_TIME_MINUTES;
        record->measurement.base_time.seconds = APP_DB_BASE_TIME_SECONDS;

        /* In practice the value should reflect the "current time - base time"
         * translated in minutes. For demonstrated purposes this value matches
         * SN. */
        record->measurement.time_offset = record->measurement.seq_number;

#if GLS_FLAGS_CONCENTRATION_TYPE_SAMPLE_LOCATION
        /* Add arbitrary values */

# if GLS_FLAGS_CONCENTRATION_UNITS
        ieee_float.exp = -4;
# else
        ieee_float.exp = -5;
# endif

        record->measurement.concentration = pack_ieee11703_sfloat(&ieee_float);
        record->measurement.type = GLS_TYPE_ARTERIAL_PLASMA;
        record->measurement.sample_location = GLS_SAMPLE_LOCATION_FINGER;
#endif

#if GLS_FLAGS_STATUS_ANNUNCIATION
        /* Since there is no real sensor to report its status assign the values of the feature characteristic. */
        record->measurement.status.battery_low = GLS_FEATURE_DEVICE_LOW_BATTERY;
        record->measurement.status.malfunction = GLS_FEATURE_SENSOR_MALFUNCTION;
        record->measurement.status.sample_insufficient = GLS_FEATURE_SAMPLE_SIZE;
        record->measurement.status.strip_insertion = GLS_FEATURE_STRIP_INSERTION_ERROR;
        record->measurement.status.strip_type = GLS_FEATURE_STRIP_TYPE_ERROR;
        record->measurement.status.result_higher = GLS_FEATURE_RESULT_HIGH_LOW;
        record->measurement.status.result_lower = !GLS_FEATURE_RESULT_HIGH_LOW;
        record->measurement.status.temperature_high = GLS_FEATURE_TEMPERATURE_HIGH_LOW;
        record->measurement.status.temperature_low = !GLS_FEATURE_TEMPERATURE_HIGH_LOW;
        record->measurement.status.read_interruption = GLS_FEATURE_READ_INTERRUPT;
        record->measurement.status.general_error = GLS_FEATURE_GENERAL_DEVICE_ERROR;
        record->measurement.status.time_error = GLS_FEATURE_TIME_ERROR;
#endif

#if GLS_FLAGS_CONTEXT_INFORMATION
        record->context.flags = GLS_CONTEXT_FLAGS_FIELD_VALUE;
        /* Shall be the same as the value of SN of its corresponding Glucose
         * measurement characteristic. */
        record->context.seq_number = record->measurement.seq_number;

#if GLS_CONTEXT_FLAGS_EXTENDED_FLAGS
        record->context.extended_flags = 0;
#endif

#if GLS_CONTEXT_FLAGS_CARBOHYDRATE_ID
        record->context.carbohydrate_id = GLS_CARBOHYDRATE_ID_BREAKFAST;
        ieee_float.exp = -3;
        record->context.carbohydrate = pack_ieee11703_sfloat(&ieee_float);
#endif

#if GLS_CONTEXT_FLAGS_MEAL
        record->context.meal = GLS_MEAL_CASUAL;
#endif

#if GLS_CONTEXT_FLAGS_TESTER_HEALTH
        record->context.tester = GLS_TESTER_LAB_TEST;
        record->context.health = GLS_HEALTH_NO_ISSUES;
#endif

#if GLS_CONTEXT_FLAGS_EXERCISE_DURATION_INTENSITY
        record->context.exercise_duration = 0;
        record->context.exercise_intensity = 0;
#endif

#if GLS_CONTEXT_FLAGS_MEDICATION_ID
        record->context.medication_id = GLS_MEDICATION_ID_RAPID_ACTING_INSULIN;

# if GLS_CONTEXT_FLAGS_MEDICATION_UNITS
        ieee_float.exp = -3;
# else
        ieee_float.exp = -6;
# endif

        record->context.medication = pack_ieee11703_sfloat(&ieee_float);
#endif

#if GLS_CONTEXT_FLAGS_HbA1c
        ieee_float.exp = 0;
        record->context.HbA1c = pack_ieee11703_sfloat(&ieee_float);
#endif

#endif /* GLS_FLAGS_CONTEXT_INFORMATION */
}

OS_TASK_FUNCTION(glucose_sensor_task, params)
{
        int8_t wdog_id;
#if dg_configSUOTA_SUPPORT
        ble_service_t *suota;
#endif
        ble_service_t *gls;

        uint16_t name_len;
        char name_buf[MAX_NAME_LEN + 1];        /* 1 byte for '\0' character */

        /* Scan Response object to be populated with <Complete Local Name> AD type */
        gap_adv_ad_struct_t *scan_rsp;

        gls_task_h = OS_GET_CURRENT_TASK();

        /* Register task to be monitored by watchdog */
        wdog_id = sys_watchdog_register(false);

        /*************************************************************************************************\
         * Initialize BLE
         */
        /* Start BLE device as peripheral */
        ble_peripheral_start();

        /* Register task to BLE framework to receive BLE event notifications */
        ble_register_app();

#if dg_configSUOTA_SUPPORT
        /* Set maximum allowed MTU to increase SUOTA throughput */
        ble_gap_mtu_size_set(512);
#endif

        update_device_address();
        print_device_address();

        /* Get device name from NVPARAM if valid or use default otherwise */
        name_len = read_name(MAX_NAME_LEN, name_buf);

        /* Set device name with write properties as suggested by GLP. */
        ble_gap_device_name_set(name_buf, ATT_PERM_RW);

        /* I/O capabilities should be defined otherwise authentication will fail and so accessing
         * RACP will not be possible. ATT write with authentication properties means that security
         * level should be raised to 3 level or greater. */
        ble_gap_set_io_cap(GAP_IO_CAP_DISP_ONLY);

        /* Define Scan Response object internals dealing with retrieved name */
        scan_rsp = GAP_ADV_AD_STRUCT_DECLARE(GAP_DATA_TYPE_LOCAL_NAME, name_len, name_buf);

        /*************************************************************************************************\
         * Initialize BLE services
         */
#if dg_configSUOTA_SUPPORT
        /* Add SUOTA Service */
        suota = suota_init(&suota_cb);
        OS_ASSERT(suota != NULL);
#endif

        gls = gls_init((const gls_callbacks_t *)&gls_callbacks);
        OS_ASSERT(gls != NULL);

        /* Add Device Information Service as mandated by the Glucose Profile specifications. */
        dis_init(NULL, &dis_info);

        /*************************************************************************************************\
         * Initialize timers
         */

        /*
         * Create timer for switching from "fast connection" to "reduced power" advertising
         * intervals after 30 seconds.
         */
        adv_tim = OS_TIMER_CREATE("adv", OS_MS_2_TICKS(30000), OS_TIMER_FAIL,
                                                (void *) OS_GET_CURRENT_TASK(), adv_tim_cb);

#if GLS_FEATURE_INDICATION_PROPERTY
        OS_TIMER feature_tim = OS_TIMER_CREATE("gls_f", OS_MS_2_TICKS(5000), OS_TIMER_SUCCESS,
                                                (void *) OS_GET_CURRENT_TASK(), feature_tim_cb);
        OS_TIMER_START(feature_tim, OS_TIMER_FOREVER);
#endif

        app_db_init();

        OS_TIMER record_tim = OS_TIMER_CREATE("record", OS_MS_2_TICKS(GLS_DATABASE_UPDATE_MS),
                                                OS_TIMER_SUCCESS, (void *) OS_GET_CURRENT_TASK(),
                                                record_tim_cb);
        OS_TIMER_START(record_tim, OS_TIMER_FOREVER);

        /*************************************************************************************************\
         * Start advertising
         *
         * Set advertising data and scan response, then start advertising.
         *
         * By default, advertising interval is set to "fast connect" and a timer is started to
         * switch to "reduced power" interval afterwards.
         */
        set_advertising_interval(ADV_INTERVAL_FAST);
        ble_gap_adv_ad_struct_set(ARRAY_LENGTH(adv_data), adv_data, 1 , scan_rsp);

        ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);
        OS_TIMER_START(adv_tim, OS_TIMER_FOREVER);

        for (;;) {
                OS_BASE_TYPE ret __UNUSED;
                uint32_t notif;

                /* Notify watchdog on each loop */
                sys_watchdog_notify(wdog_id);

                /* Suspend watchdog while blocking on OS_TASK_NOTIFY_WAIT() */
                sys_watchdog_suspend(wdog_id);

                /*
                 * Wait on any of the notification bits, then clear them all
                 */
                ret = OS_TASK_NOTIFY_WAIT(OS_TASK_NOTIFY_NONE, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                /* Blocks forever waiting for the task notification. Therefore, the return value must
                 * always be OS_OK
                 */
                OS_ASSERT(ret == OS_OK);

                /* Resume watchdog */
                sys_watchdog_notify_and_resume(wdog_id);

                /* Notified from BLE manager? */
                if (notif & BLE_APP_NOTIFY_MASK) {
                        ble_evt_hdr_t *hdr;

                        hdr = ble_get_event(false);
                        if (!hdr) {
                                goto no_event;
                        }

                        /*
                         * First, the application needs to check if the event is handled by the
                         * ble_service framework. If it is not handled, the application may handle
                         * it by defining a case for it in the `switch ()` statement below. If the
                         * event is not handled by the application either, it is handled by the
                         * default event handler.
                         */
                        if (!ble_service_handle_event(hdr)) {
                                switch (hdr->evt_code) {
                                case BLE_EVT_GAP_CONNECTED:
                                        handle_evt_gap_connected((ble_evt_gap_connected_t *) hdr);
                                        break;
                                case BLE_EVT_GAP_DISCONNECTED:
                                        handle_evt_gap_disconnected((ble_evt_gap_disconnected_t *) hdr);
                                        break;
                                case BLE_EVT_GAP_ADV_COMPLETED:
                                        handle_evt_gap_adv_completed((ble_evt_gap_adv_completed_t *) hdr);
                                        break;
                                case BLE_EVT_GAP_PAIR_REQ:
                                        handle_evt_gap_pair_req((ble_evt_gap_pair_req_t *) hdr);
                                        break;
                                case BLE_EVT_GAP_PASSKEY_NOTIFY:
                                        handle_evt_gap_passkey_notify((ble_evt_gap_passkey_notify_t *) hdr);
                                        break;
                                case BLE_EVT_GAP_PAIR_COMPLETED:
                                        handle_evt_gap_pair_completed((ble_evt_gap_pair_completed_t *) hdr);
                                        break;
#if (dg_configSUOTA_SUPPORT == 1)
#if (dg_configBLE_2MBIT_PHY == 1)
                                case BLE_EVT_GAP_PHY_SET_COMPLETED:
                                        handle_ble_evt_gap_phy_set_completed((ble_evt_gap_phy_set_completed_t *) hdr);
                                        break;
                                case BLE_EVT_GAP_PHY_CHANGED:
                                        handle_ble_evt_gap_phy_changed((ble_evt_gap_phy_changed_t *) hdr);
                                        break;
#endif /* (dg_configBLE_2MBIT_PHY == 1) */
#endif /* (dg_configSUOTA_SUPPORT == 1) */
#if dg_configSUOTA_SUPPORT && defined(SUOTA_PSM)
                                case BLE_EVT_L2CAP_CONNECTED:
                                case BLE_EVT_L2CAP_DISCONNECTED:
                                case BLE_EVT_L2CAP_DATA_IND:
                                        suota_l2cap_event(suota, hdr);
                                        break;
#endif
                                default:
                                        ble_handle_event_default(hdr);
                                        break;
                                }
                        }

                        /* Free event buffer (it's not needed anymore) */
                        OS_FREE(hdr);

no_event:
                        /*
                         * If there are more events waiting in queue, application should process
                         * them now.
                         */
                        if (ble_has_event()) {
                                OS_TASK_NOTIFY(OS_GET_CURRENT_TASK(), BLE_APP_NOTIFY_MASK,
                                                                                OS_NOTIFY_SET_BITS);
                        }
                }

#if GLS_FEATURE_INDICATION_PROPERTY
                if (notif & FEATURE_TMO_NOTIF) {
                        static uint16_t feature_val = 0;

                        /* Send dummy indication values */
                        gls_indicate_feature_all(gls, feature_val++);
                }
#endif

                if (notif & RECORD_UPDATE_TMO_NOTIF) {
                        /*
                         * No callback is used here; record entries will be initialized
                         * internally using arbitrary values.
                         */
                        app_db_add_record_entry(init_record_entry_cb);
                }

                /* The number of records might be long enough to timeout WDOG. */
                sys_watchdog_suspend(wdog_id);

                if (notif & RACP_REPORT_RECORDS_NOTIF) {
                        app_db_report_records_handle();
                }

                if (notif & RACP_DELETE_RECORDS_NOTIF) {
                        app_db_delete_records_handle();
                }

                sys_watchdog_notify_and_resume(wdog_id);

                if (notif & RACP_REPORT_NUM_NOTIF) {
                        app_db_report_num_of_records_handle();
                }

                /* Notified from advertising timer? */
                if (notif & ADV_TMO_NOTIF) {
                        ble_error_t ret;
                        /*
                         * Change interval values and stop advertising. Once it's stopped, it will
                         * be started again with the new parameters.
                         */
                        set_advertising_interval(ADV_INTERVAL_POWER);
                        ret = ble_gap_adv_stop();
                        /* Advertising isn't started, start it now */
                        if (ret == BLE_ERROR_NOT_ALLOWED) {
                                start_adv();
                        }
                }

                /* Fast connection timer expired, try to set reduced power connection parameters */
                if (notif & UPDATE_CONN_PARAM_NOTIF) {
                        conn_dev_t *conn_dev = param_connections;

                        if (conn_dev && conn_dev->expired) {
                                param_connections = conn_dev->next;

#if dg_configSUOTA_SUPPORT
                                /*
                                 * Ignore this if SUOTA is ongoing - it's possible to start SUOTA
                                 * before reduced power parameters are applied so this would switch
                                 * to a long connection interval.
                                 */
                                if (!suota_ongoing) {
#endif
#if GLUCOSE_SENSOR_SUOTA_POWER_SAVING
                                        conn_param_update(conn_dev->conn_idx);
#endif
#if dg_configSUOTA_SUPPORT
                                }
#endif

                                OS_TIMER_DELETE(conn_dev->param_timer, OS_TIMER_FOREVER);
                                OS_FREE(conn_dev);

                                /*
                                 * If the queue is not empty, reset bit and check if timer expired
                                 * next time.
                                 */
                                if (param_connections) {
                                        OS_TASK_NOTIFY(OS_GET_CURRENT_TASK(),
                                                UPDATE_CONN_PARAM_NOTIF, OS_NOTIFY_SET_BITS);
                                }
                        }
                }
        }
}

