
/**
 ****************************************************************************************
 *
 * @file ble_peripheral_task.c
 *
 * @brief Demonstrate using the custom service framework
 *
 * Copyright (c) 2023 Dialog Semiconductor. All rights reserved.
 *
 * This software ("Software") is owned by Dialog Semiconductor. By using this Software
 * you agree that Dialog Semiconductor retains all intellectual property and proprietary
 * rights in and to this Software and any use, reproduction, disclosure or distribution
 * of the Software without express written permission or a license agreement from Dialog
 * Semiconductor is strictly prohibited. This Software is solely for use on or in
 * conjunction with Dialog Semiconductor products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES OR AS
 * REQUIRED BY LAW, THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE PROVIDED
 * IN A LICENSE AGREEMENT BETWEEN THE PARTIES OR BY LAW, IN NO EVENT SHALL DIALOG
 * SEMICONDUCTOR BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE SOFTWARE.
 *
 ****************************************************************************************
 */

#include <stdbool.h>
#include <stdlib.h>
#include "osal.h"
#include "sys_watchdog.h"
#include "ble_common.h"
#include "ble_gap.h"
#include "ble_gattc.h"
#include "misc.h"
#include "ble_custom_service.h"
#if dg_configSUOTA_SUPPORT
  #include "ble_l2cap.h"
  #include "dis.h"
  #include "dlg_suota.h"
  #include "sw_version.h"
#endif
#include "sys_trng.h"
#include "ad_nvms.h"
#include "platform_nvparam.h"
#include "gap.h"

/* Just to demonstrate how notifications can be sent explicitly for dedicated attribute handles */
#define APP_NOTIF_DEMONSTRATION            ( 0 )

/**
 * Attribute values maximum size expressed in bytes
 *
 * \warning The remote device must not exceed that number. Otherwise, the system will crash!!!
 *
 */
#define ATTR_MAX_SIZE                      ( 50 )

#if APP_NOTIF_DEMONSTRATION
  /* Characteristic attributes value update frequency */
  #define NOTIF_TIMER_PERIOD_MS            ( 5000 )

  /*
   * Notification bits reservation
   * bit #0 is always assigned to BLE event queue notification
   */
  #define NOTIF_TIMER_EVENT                ( 1 << 1 )
#endif

/* Dummy 128-bit UUIDs */
#define CHARACTERISTIC_ATTR_1_128_UUID     STRINGIFY(11111111-0000-0000-0000-000000000001)
#define CHARACTERISTIC_ATTR_2_128_UUID     STRINGIFY(22222222-0000-0000-0000-000000000001)
#define CHARACTERISTIC_ATTR_3_128_UUID     STRINGIFY(22222222-0000-0000-0000-000000000002)
#define CHARACTERISTIC_ATTR_4_128_UUID     STRINGIFY(22222222-0000-0000-0000-000000000003)

#define SERVICE_ATTR_1_128_UUID            STRINGIFY(11111111-0000-0000-0000-111111111111)
#define SERVICE_ATTR_2_128_UUID            STRINGIFY(22222222-0000-0000-0000-222222222222)

#define CHARACTERISTIC_ATTR_1_DESCRIPTOR   STRINGIFY(Service #1 Characteristic #1)
#define CHARACTERISTIC_ATTR_2_DESCRIPTOR   STRINGIFY(NULL)
#define CHARACTERISTIC_ATTR_3_DESCRIPTOR   STRINGIFY(NULL)
#define CHARACTERISTIC_ATTR_4_DESCRIPTOR   STRINGIFY(Service #2 Characteristic #3)

#if dg_configSUOTA_SUPPORT
/*
 * Store information about ongoing SUOTA.
 */
__RETAINED_RW static bool suota_ongoing = false;
#endif

/**
 *
 * \note Characteristic attribute values should be handled on application level by users.
 *
 */
__RETAINED uint8_t attribute_value[ATTR_MAX_SIZE];

#if APP_NOTIF_DEMONSTRATION
/* OS timer handler used to update characteristic attributes value */
__RETAINED static OS_TIMER notif_timer_h;
#endif

/* Task handle */
__RETAINED_RW static OS_TASK ble_task_handle = NULL;

/**
 * BLE peripheral advertising data
 */
static const gap_adv_ad_struct_t adv_data[] = {
#if dg_configSUOTA_SUPPORT
#define UUID_SUOTA_LSB     (dg_configBLE_UUID_SUOTA_SERVICE & 0xFF)
#define UUID_SUOTA_MSB     ((dg_configBLE_UUID_SUOTA_SERVICE & 0xFF00) >> 8)
        GAP_ADV_AD_STRUCT_BYTES(GAP_DATA_TYPE_LOCAL_NAME,
                                'M', 'y', ' ', 'C', 'u', 's', 't', 'o', 'm', ' ', 'S', 'e', 'r', 'v', 'i', 'c', 'e'),
        GAP_ADV_AD_STRUCT_BYTES(GAP_DATA_TYPE_UUID16_LIST_INC,
                                UUID_SUOTA_LSB, UUID_SUOTA_MSB),
#else
        GAP_ADV_AD_STRUCT_BYTES(GAP_DATA_TYPE_LOCAL_NAME,
                                'M', 'y', ' ', 'C', 'u', 's', 't', 'o', 'm', ' ', 'S', 'e', 'r', 'v', 'i', 'c', 'e')
#endif
};

static void device_set_random_address(void)
{
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

static void device_get_random_address(void)
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

/**
 * This callback is fired when a peer device issues a read request for a specific attribute value.
 *
 * \param [out] data: The value returned back to the peer device
 *
 * \param [out] length: The number of bytes returned back to the peer device
 *
 * \warning: The BLE stack will not proceed with the next BLE event until the
 *           callback returns.
 */
static void characteristic_attr_read_cb(uint8_t **data, uint16_t *length)
{
        *data  = attribute_value;
        *length = sizeof(attribute_value);

        DBG_PRINTF("\nRead callback function hit! - Returned value: %s\n\r", (char *)attribute_value);
}

/**
 * This callback is fired when a peer device issues a write request for a specific attribute value.
 *
 * \param [in] value:  The data written by the peer device.
 *
 * \param [in] length: Number of bytes written by the peer device
 *
 * \warning: The BLE stack will not proceed with the next BLE event until the
 *           callback returns.
 *
 */
static void characteristic_attr_write_cb(const uint8_t *data, uint16_t length)
{
        memset((void *)attribute_value, '\0', sizeof(attribute_value));
        memcpy((void *)attribute_value, (void *)data, length);

        DBG_PRINTF("\nWrite callback function hit! - Written value: %s, length: %d\n\r",
                                                                        attribute_value, length);
}

/**
 * This callback is fired for each connected peer device that has enabled
 * notifications/indications explicitly (for a specific attribute handle).
 *
 * \param [in] conn_idx: Connection index
 *
 * \param [in] status: True when notifications are sent successfully, otherwise false.
 *
 * \param [in] type: Type of notification
 *
 * \warning: The BLE stack will not proceed with the next BLE event until the
 *           callback returns.
 */
static void characteristic_attr_notification_cb(uint16_t conn_idx, bool status, gatt_event_t type)
{
        DBG_PRINTF("\nNotification callback - Connection IDX: %d, Status: %d\n\r", conn_idx, status);
}

static void characteristic_attr_indication_cb(uint16_t conn_idx, bool status, gatt_event_t type)
{
        DBG_PRINTF("\nIndication callback - Connection IDX: %d, Status: %d\n\r", conn_idx, status);
}

/***************************** BLE event handlers *******************************/
static void handle_evt_gattc_mtu_changed(ble_evt_gattc_mtu_changed_t *evt)
{
        DBG_PRINTF("\n\rMTU CHANGED - CON IDX[%d] - MTU: %d\n\r", evt->conn_idx, evt->mtu);
}

static void handle_evt_gap_connected(ble_evt_gap_connected_t *evt)
{
        /*
         * Manage connection information
         */
        ble_gattc_exchange_mtu(evt->conn_idx);
}

static void handle_evt_gap_disconnected(ble_evt_gap_disconnected_t *evt)
{
        /*
         * Manage disconnection information
         */
}

static void handle_evt_gap_adv_completed(ble_evt_gap_adv_completed_t *evt)
{
#if dg_configSUOTA_SUPPORT
        /* If SUOTA is ongoing, don't restart advertising */
        if (suota_ongoing) {
                return;
        }
#endif

        /* Restart advertising so we can connect again */
        ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);
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

#if (dg_configBLE_2MBIT_PHY == 1)
        /* Switch to Auto PHY when SUOTA is completed */
        ble_gap_phy_set(conn_idx, BLE_GAP_PHY_PREF_AUTO, BLE_GAP_PHY_PREF_AUTO);
#endif /* (dg_configBLE_2MBIT_PHY == 1) */
}

static const suota_callbacks_t suota_cb = {
        .suota_ready = suota_ready_cb,
        .suota_status = suota_status_changed_cb,
};

/* Device Information Service data */
static const dis_device_info_t dis_info = {
        .manufacturer  = defaultBLE_DIS_MANUFACTURER,
        .model_number  = defaultBLE_DIS_MODEL_NUMBER,
        .serial_number = defaultBLE_DIS_SERIAL_NUMBER,
        .hw_revision   = defaultBLE_DIS_HW_REVISION,
        .fw_revision   = defaultBLE_DIS_FW_REVISION,
#ifdef SW_VERSION
        .sw_revision   = SW_VERSION,
#else
        .sw_revision   = defaultBLE_DIS_SW_REVISION,
#endif /* SW_VERSION */
};
#endif

#if APP_NOTIF_DEMONSTRATION
/**
 *  Notification OS timer callback function.
 *
 *  \note Used to notify the peripheral task that the OS timer has been expired so,
 *        notifications are sent to the peer devices connected.
 *
 */
static void notif_timer_cb(OS_TIMER timer)
{
        OS_TASK task = (OS_TASK) OS_TIMER_GET_TIMER_ID(timer);

        if (task) {
                OS_TASK_NOTIFY(task, NOTIF_TIMER_EVENT, OS_NOTIFY_SET_BITS);
        }
}
#endif

/*
 * Define a task that utilizes the BLE custom service service framework, initiates the BLE controller in the slave role
 * and handles the various incoming BLE events.
 */
OS_TASK_FUNCTION(ble_peripheral_task, pvParameters)
{
        int8_t wdog_id;
#if dg_configSUOTA_SUPPORT
        ble_service_t *suota;
#endif

        /* register ble_peripheral task to be monitored by watchdog */
        wdog_id = sys_watchdog_register(false);

        /* Get task's handler */
        ble_task_handle = OS_GET_CURRENT_TASK();

        /* Initiate the BLE controller in the slave role */
        ble_peripheral_start();

        /* Register the current task to the BLE manager so the first can be notified for incoming BLE events. */
        ble_register_app();

        /* \warning This API should be called prior to creating the DB! */
        device_set_random_address();
        device_get_random_address();

        /* \warning This API should be called prior to creating the DB! */
        ble_gap_device_name_set("BLE Custom Service", ATT_PERM_READ);

        /* \warning This API should be called prior to creating the DB! */
        ble_gap_mtu_size_set(defaultBLE_MAX_MTU_SIZE);


        //************ BLE custom service data base definition  *************

        const mcs_characteristic_config_t custom_service_1[] = {

                /* Characteristic attributes declarations */
                CHARACTERISTIC_DECLARATION(CHARACTERISTIC_ATTR_1_128_UUID,
                                           ATTR_MAX_SIZE,
                                           GATT_PROP_WRITE | GATT_PROP_READ | GATT_PROP_NOTIFY,
                                           ATT_PERM_RW,
                                           CHARACTERISTIC_ATTR_1_DESCRIPTOR,
                                           characteristic_attr_read_cb,
                                           characteristic_attr_write_cb,
                                           characteristic_attr_notification_cb),


                 // -----------------------------------------------------------------
                 // -- Here you can add more Characteristic Attributes --
                 // -----------------------------------------------------------------


        };
        // ***************** BLE service declaration *****************
        SERVICE_DECLARATION(custom_service_1, SERVICE_ATTR_1_128_UUID)


        const mcs_characteristic_config_t custom_service_2[] = {

                /* Characteristic attributes declarations. You can define your preferred settings. */
                CHARACTERISTIC_DECLARATION(CHARACTERISTIC_ATTR_2_128_UUID, 0,
                                           GATT_PROP_NONE, ATT_PERM_NONE,
                                           CHARACTERISTIC_ATTR_2_DESCRIPTOR,
                                           NULL, NULL, NULL),


                /* Characteristic attributes declarations. You can define your preferred settings. */
                CHARACTERISTIC_DECLARATION(CHARACTERISTIC_ATTR_3_128_UUID, 0,
                                           GATT_PROP_NONE, ATT_PERM_NONE,
                                           CHARACTERISTIC_ATTR_3_DESCRIPTOR,
                                           NULL, NULL, NULL),


                /* Characteristic attributes declarations. You can define your preferred settings. */
                CHARACTERISTIC_DECLARATION(CHARACTERISTIC_ATTR_4_128_UUID,
                                           ATTR_MAX_SIZE,
                                           GATT_PROP_INDICATE, ATT_PERM_NONE,
                                           CHARACTERISTIC_ATTR_4_DESCRIPTOR,
                                           NULL, NULL, characteristic_attr_indication_cb),


                // -----------------------------------------------------------------
                // --------- Here you can add more Characteristic Attributes -------
                // -----------------------------------------------------------------

        };
        // ***************** BLE service declaration *****************
        SERVICE_DECLARATION(custom_service_2, SERVICE_ATTR_2_128_UUID)

#if dg_configSUOTA_SUPPORT
        /* Add SUOTA Service */
        suota = suota_init(&suota_cb);
        OS_ASSERT(suota != NULL);

        /* Add Device Information Service */
        dis_init(NULL, &dis_info);
#endif

        strcpy((char *)attribute_value, "Some dummy text.");

        ble_gap_adv_ad_struct_set(ARRAY_LENGTH(adv_data), adv_data, 0 , NULL);
        ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);

#if APP_NOTIF_DEMONSTRATION
        /* Define an OS timer in reload mode which will be used for updating characteristic attributes values.  */
        notif_timer_h = OS_TIMER_CREATE("APP NOTIF",
                                        OS_MS_2_TICKS(NOTIF_TIMER_PERIOD_MS),
                                        true,
                                        (void *) OS_GET_CURRENT_TASK(),
                                        notif_timer_cb);
        ASSERT_WARNING(notif_timer_h);

        OS_TIMER_START(notif_timer_h, OS_TIMER_FOREVER);
#endif

        for (;;) {
                OS_BASE_TYPE ret;
                uint32_t notif;

                /* notify watchdog on each loop */
                sys_watchdog_notify(wdog_id);

                /* suspend watchdog while blocking on OS_TASK_NOTIFY_WAIT() */
                sys_watchdog_suspend(wdog_id);

                /*
                 * Wait on any of the notification bits, then clear them all
                 */
                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                /* Blocks forever waiting for task notification. The return value must be OS_OK */
                OS_ASSERT(ret == OS_OK);

                /* resume watchdog */
                sys_watchdog_notify_and_resume(wdog_id);

                /* notified from BLE manager, can get event */
                if (notif & BLE_APP_NOTIFY_MASK) {
                        ble_evt_hdr_t *hdr;

                        hdr = ble_get_event(false);
                        if (!hdr) {
                                goto no_event;
                        }

                        if (!ble_service_handle_event(hdr)) {
                                switch (hdr->evt_code) {
                                case BLE_EVT_GAP_CONNECTED:
                                        handle_evt_gap_connected((ble_evt_gap_connected_t *) hdr);
                                        break;
                                case BLE_EVT_GAP_ADV_COMPLETED:
                                        handle_evt_gap_adv_completed((ble_evt_gap_adv_completed_t *) hdr);
                                        break;
                                case BLE_EVT_GAP_DISCONNECTED:
                                        handle_evt_gap_disconnected((ble_evt_gap_disconnected_t *) hdr);
                                        break;
                                case BLE_EVT_GAP_PAIR_REQ:
                                {
                                        ble_evt_gap_pair_req_t *evt = (ble_evt_gap_pair_req_t *) hdr;
                                        ble_gap_pair_reply(evt->conn_idx, true, evt->bond);
                                        break;
                                }
                                case BLE_EVT_GATTC_MTU_CHANGED:
                                        handle_evt_gattc_mtu_changed((ble_evt_gattc_mtu_changed_t *) hdr);
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
                                OS_TASK_NOTIFY(OS_GET_CURRENT_TASK(), BLE_APP_NOTIFY_MASK, OS_NOTIFY_SET_BITS);
                        }
                }

#if APP_NOTIF_DEMONSTRATION
                /*
                 * The following code is just for demonstration purposes to illustrate how notifications
                 * can be sent explicitly for a specific attribute handle.
                 * A connected peer device will receive notifications/indications only if has them enabled
                 * explicitly. The notification type is defined automatically based on its associated
                 * attribute declaration.
                 */
                if (notif & NOTIF_TIMER_EVENT) {
                        /* Generate a random number with the help of the TRNG engine */
                        int arbitrary_value = rand();

                        /* Send notifications for a specific attribute value (referenced by its UUID) */
                        mcs_send_notifications(CHARACTERISTIC_ATTR_1_128_UUID,
                             (const uint8_t *)&arbitrary_value, sizeof(arbitrary_value));

                        mcs_send_notifications(CHARACTERISTIC_ATTR_4_128_UUID,
                             (const uint8_t *)&arbitrary_value, sizeof(arbitrary_value));
                }
#endif
        }
}
