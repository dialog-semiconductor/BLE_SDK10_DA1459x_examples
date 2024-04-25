/**
 ****************************************************************************************
 *
 * @file glucose_service.c
 *
 * @brief Glucose Service
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
#include "osal.h"
#include "ble_att.h"
#include "ble_common.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "ble_storage.h"
#include "ble_bufops.h"
#include "ble_uuid.h"
#include "glucose_service.h"

#define UUID_CCC         ( 0x2902 )
#define GLS_ATT_PROCEDURE_ALREADY_IN_PROGRESS   0x80
#define GLS_ATT_CCC_IMPROPERLY_CONFIGURED       0x81

typedef struct {
        uint8_t command;       /* Current command being processed */
        bool racp_in_progress; /* Flag to indicate whether or not a record is currently being processed
                                  by users (via registered callback functions) and so no further RACP
                                  operations, except for abort operations, can be processed, if received.
                                  If this flag is never reset to zero then this should be an indication
                                  that application has not terminated the RACP procedure by sending
                                  RACP indications as response to the previously received RACP command. */
} g_service_data_t;

typedef struct {
        ble_service_t svc;

        /* User callback functions */
        const gls_callbacks_t *cb;

        g_service_data_t data;

        /* Data base handles */
        uint16_t gm_val_h;      /* Glucose measurement value handle */
        uint16_t gm_ccc_h;      /* Glucose measurement client characteristic configuration (CCC) descriptor handle */
#if GLS_FLAGS_CONTEXT_INFORMATION
        uint16_t gmc_val_h;     /* Glucose measurement context value handle */
        uint16_t gmc_ccc_h;     /* Glucose measurement context CCC descriptor handle */
#endif
        uint16_t gf_val_h;      /* Glucose feature value handle */
#if GLS_FEATURE_INDICATION_PROPERTY
        uint16_t gf_ccc_h;      /* Though not illustrated, section 3.3.1 of specifications clearly states the existence of CCC */
#endif
        uint16_t racp_val_h;    /* Record access control point value handle */
        uint16_t racp_ccc_h;    /* Record access control point CCC descriptor handle */
} g_service_t;

bool gls_notify_record(ble_service_t *svc, uint16_t conn_idx, gls_record_t *record)
{
        ASSERT_WARNING(svc);

        g_service_t *gls = (g_service_t *)svc;
        uint16_t ccc;
        gap_device_t dev;
        ble_error_t status;

        /*
         * Since this API can also be called to send notifications when new records are
         * available since the last connection with a bonded device, we should first
         * check if notifications are enabled for the specified peer device.
         * Note that if this API is called as response to RACP requests, CCC and bond status
         * checks should have already been done.
         */
        ble_storage_get_u16(conn_idx, gls->gm_ccc_h, &ccc);
        ble_gap_get_device_by_conn_idx(conn_idx, &dev);

        if (!(ccc & GATT_CCC_NOTIFICATIONS) || !dev.bonded) {
                return false;
        }

#if GLS_FLAGS_CONTEXT_INFORMATION
        ble_storage_get_u16(conn_idx, gls->gmc_ccc_h, &ccc);

        if (!(ccc & GATT_CCC_NOTIFICATIONS)) {
                return false;
        }
#endif

        /* Record data should already be packed! */
        status = ble_gatts_send_event(conn_idx, gls->gm_val_h, GATT_EVENT_NOTIFICATION,
                                                sizeof(record->measurement), &record->measurement);
        /*
         * If enabled, the notification of the Glucose Measurement characteristic shall be followed
         * by a notification of the Glucose Measurement Context characteristic.
         */
#if GLS_FLAGS_CONTEXT_INFORMATION
        status |= ble_gatts_send_event(conn_idx, gls->gmc_val_h, GATT_EVENT_NOTIFICATION,
                                                sizeof(record->context), &record->context);
#endif

        return (status == BLE_STATUS_OK);
}

void gls_notify_record_all(ble_service_t *svc, gls_record_t *record)
{
        uint8_t num_conn;
        uint16_t *conn_idx;

        ble_gap_get_connected(&num_conn, &conn_idx);

        while (num_conn--) {
                gls_notify_record(svc, conn_idx[num_conn], record);
        }

        /* Consumer should free the allocated buffer when is no longer needed */
        if (conn_idx) {
                OS_FREE(conn_idx);
        }
}

void gls_indicate_status(ble_service_t *svc, uint16_t conn_idx, uint8_t command, uint8_t status)
{
        ASSERT_WARNING(svc);

        g_service_t *gls = (g_service_t *)svc;
        uint8_t pdu[4];
        uint8_t *ptr = pdu;

        /* Pack the response as dictated by GLS specifications:
         * 1-byte opcode + 1-byte Operator + 2-byte Operand */
        put_u8_inc(&ptr, GLS_RACP_COMMAND_RESPONSE);
        put_u8_inc(&ptr, GLS_RACP_OPERATOR_NULL);
        put_u8_inc(&ptr, command);
        put_u8_inc(&ptr, status);

        ble_gatts_send_event(conn_idx, gls->racp_val_h, GATT_EVENT_INDICATION, sizeof(pdu), pdu);

        /* Mark the end of a previously requested RACP operation */
        gls->data.racp_in_progress = false;
}

void inline gls_indicate_abort_operation_status(ble_service_t *svc, uint16_t conn_idx, uint8_t status)
{
        gls_indicate_status(svc, conn_idx, GLS_RACP_COMMAND_ABORT_OPERATION, status);
}

#if GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT
void inline gls_indicate_delete_records_status(ble_service_t *svc, uint16_t conn_idx, uint8_t status)
{
        gls_indicate_status(svc, conn_idx, GLS_RACP_COMMAND_DELETE_RECORDS, status);
}
#endif

void inline gls_indicate_report_records_status(ble_service_t *svc, uint16_t conn_idx, uint8_t status)
{
        gls_indicate_status(svc, conn_idx, GLS_RACP_COMMAND_REPORT_RECORDS, status);
}

void gls_indicate_number_of_stored_records(ble_service_t *svc, uint16_t conn_idx, uint16_t num_records)
{
        ASSERT_WARNING(svc);

        g_service_t *gls = (g_service_t *)svc;
        uint8_t pdu[4];
        uint8_t *ptr = pdu;

        /* Pack the response as dictated by GLS specifications:
         * 1-byte opcode + 1-byte Operator + 2-byte Operand */
        put_u8_inc(&ptr, GLS_RACP_COMMAND_NUMBER_OF_RECORDS_RESPONSE);
        put_u8_inc(&ptr, GLS_RACP_OPERATOR_NULL);
        put_u16_inc(&ptr, num_records);

        ble_gatts_send_event(conn_idx, gls->racp_val_h, GATT_EVENT_INDICATION, sizeof(pdu), pdu);

        /* Mark the end of a previously requested RACP operation */
        gls->data.racp_in_progress = false;
}

#if GLS_FEATURE_INDICATION_PROPERTY
/* Should be called by application when get_features callback is called. */
void gls_get_feature_cfm(ble_service_t *svc, uint16_t conn_idx, att_error_t status, uint16_t feature)
{
        ASSERT_WARNING(svc);

        g_service_t *gls = (g_service_t *)svc;

        ble_gatts_read_cfm(conn_idx, gls->gf_val_h, status, sizeof(feature), &feature);
}

void gls_indicate_feature(ble_service_t *svc, uint16_t conn_idx, uint16_t feature)
{
        ASSERT_WARNING(svc);

        g_service_t *gls = (g_service_t *)svc;
        uint16_t ccc;
        gap_device_t device;

        ble_storage_get_u16(conn_idx, gls->gf_ccc_h, &ccc);
        ble_gap_get_device_by_conn_idx(conn_idx, &device);

        /* Indication should be sent only if CCC is enabled by the peer device and if the device
         * has been marked as bonded. */
        if (!(ccc & GATT_CCC_INDICATIONS) || !device.bonded) {
                return;
        }

        ble_gatts_send_event(conn_idx, gls->gf_val_h, GATT_EVENT_INDICATION, sizeof(feature), &feature);
}

void gls_indicate_feature_all(ble_service_t *svc, uint16_t feature)
{
        ASSERT_WARNING(svc);

        uint8_t num_conn;
        uint16_t *conn_idx;

        ble_gap_get_connected(&num_conn, &conn_idx);

        while (num_conn--) {
                gls_indicate_feature(svc, conn_idx[num_conn], feature);
        }

        /* Consumer should free the allocated buffer when is no longer needed */
        if (conn_idx) {
                OS_FREE(conn_idx);
        }
}
#endif

static att_error_t do_feature_read(g_service_t *gls, const ble_evt_gatts_read_req_t *evt)
{
        ARG_UNUSED(gls);

        if (evt->offset) {
                return ATT_ERROR_ATTRIBUTE_NOT_LONG;
        }

#if GLS_FEATURE_INDICATION_PROPERTY
        if (gls->cb->get_features == NULL) {
                return ATT_ERROR_READ_NOT_PERMITTED;
        }

        /* Application should use gls_get_feature_cfm to provide with the requested data asynchronously. */
        gls->cb->get_features(&gls->svc, evt->conn_idx);
#else
        uint16_t feature = GLS_FEATURE_CHARACTERISTIC_VALUE;
        ble_gatts_read_cfm(evt->conn_idx, evt->handle, ATT_ERROR_OK, 2, &feature);
#endif

        /* Indicate that read confirmation is handled */
        return (att_error_t) -1;
}

static att_error_t do_generic_ccc_read(const ble_evt_gatts_read_req_t *evt)
{
        uint16_t ccc;

        ble_storage_get_u16(evt->conn_idx, evt->handle, &ccc);
        ble_gatts_read_cfm(evt->conn_idx, evt->handle, ATT_ERROR_OK, sizeof(ccc), &ccc);

        return (att_error_t) -1;
}

static void handle_read_req(ble_service_t *svc, const ble_evt_gatts_read_req_t *evt)
{
        g_service_t *gls = (g_service_t *)svc;
        att_error_t status = ATT_ERROR_READ_NOT_PERMITTED;

        if (evt->handle == gls->gm_ccc_h ||
#if GLS_FLAGS_CONTEXT_INFORMATION
                evt->handle == gls->gmc_ccc_h ||
#endif
#if GLS_FEATURE_INDICATION_PROPERTY
                evt->handle == gls->gf_ccc_h ||
#endif
                evt->handle == gls->racp_ccc_h) {
                status = do_generic_ccc_read(evt);
        } else if (evt->handle == gls->gf_val_h) {
                status = do_feature_read(gls, evt);
        }

        if (status != (att_error_t) -1) {
                ble_gatts_read_cfm(evt->conn_idx, evt->handle, ATT_ERROR_READ_NOT_PERMITTED, 0, NULL);
        }
}

static att_error_t racp_parse_command(g_service_t *gls, const ble_evt_gatts_write_req_t *evt, uint8_t command, const uint8_t *ptr)
{
        gls_racp_t record;

        if (evt->length < 2) {
                return ATT_ERROR_INVALID_VALUE_LENGTH;
        }

        /* Next byte should reflect the operator */
        record.operator = get_u8_inc(&ptr);

        if (command == GLS_RACP_COMMAND_ABORT_OPERATION) {
                /* Operator should be NULL */
                if (record.operator != GLS_RACP_OPERATOR_NULL) {
                        gls_indicate_status(&gls->svc, evt->conn_idx, command, GLS_RACP_RESPONSE_INVALID_OPERATOR);
                } else {
                        gls->cb->racp_callbacks.abort_operation(&gls->svc, evt->conn_idx);
                }
                return ATT_ERROR_OK;
        }

        switch (record.operator) {
        case GLS_RACP_OPERATOR_ALL_RECORDS:
#if GLS_RACP_OPERATOR_FIRST_RECORD_SUPPORT
        case GLS_RACP_OPERATOR_FIRST_RECORD:
#endif
#if GLS_RACP_OPERATOR_LAST_RECORD_SUPPORT
        case GLS_RACP_OPERATOR_LAST_RECORD:
#endif
                /* Registered callback can be invoked immediately as no operand is expected next */
                record.filter_type = GLS_RACP_FILTER_TYPE_RFU;
                record.filter_param = NULL;
                record.filter_param_len = 0;
                break;
#if GLS_RACP_OPERATOR_LESS_EQUAL_SUPPORT
        case GLS_RACP_OPERATOR_LESS_EQUAL:
#endif
        case GLS_RACP_OPERATOR_GREATER_EQUAL:
#if GLS_RACP_OPERATOR_WITHIN_RANGE_SUPPORT
        case GLS_RACP_OPERATOR_WITHIN_RANGE:
#endif
                /* At least one more byte is expected and that should reflect the filter type
                 * upon which records will be filtered. */
                if (evt->length < 3) {
                        return ATT_ERROR_INVALID_VALUE_LENGTH;
                }

                record.filter_type = get_u8_inc(&ptr);

                switch (record.filter_type) {
                case GLS_RACP_FILTER_TYPE_SN:
#if GLS_RACP_FILTER_USER_FACING_TIME_SUPPORT
                case GLS_RACP_FILTER_TYPE_UFT:
#endif
                        /* Based on the filter type and operator compute the number of the next expected
                         * bytes that should reflect the actual operand value */
                        record.filter_param_len = (record.filter_type == GLS_RACP_FILTER_TYPE_SN) ?
                                                                        2 : sizeof(gls_base_user_facing_time_t);
                        record.filter_param_len *= (record.operator == GLS_RACP_OPERATOR_WITHIN_RANGE) ? 2 : 1;

                        /* Make sure operand has the expected size */
                        if (evt->length < (3 + record.filter_param_len) ||
                                evt->length > (3 + record.filter_param_len)) {
                                return ATT_ERROR_INVALID_VALUE_LENGTH;
                        }

                        record.filter_param = ptr;
                        break;
                default:
                        gls_indicate_status(&gls->svc, evt->conn_idx, command, GLS_RACP_RESPONSE_UNSUPPORTED_OPERAND);
                        return ATT_ERROR_OK;
                }
                break;
        default:
                gls_indicate_status(&gls->svc, evt->conn_idx, command, GLS_RACP_RESPONSE_UNSUPPORTED_OPERATOR);
                return ATT_ERROR_OK;
        }

        /*
         * If we reach this point then command can be processed by application via
         * the registered callback functions.
         * Mark that record processing is in progress. As per GLS specifications multiple
         * abort operations can be processed at the same time without returning ATT error.
         */
        gls->data.racp_in_progress = true;

        switch (command) {
        case GLS_RACP_COMMAND_NUMBER_OF_RECORDS:
                gls->cb->racp_callbacks.report_num_of_records(&gls->svc, evt->conn_idx, &record);
                break;
        case GLS_RACP_COMMAND_REPORT_RECORDS:
                gls->cb->racp_callbacks.report_records(&gls->svc, evt->conn_idx, &record);
                break;
#if GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT
        case GLS_RACP_COMMAND_DELETE_RECORDS:
                gls->cb->racp_callbacks.delete_records(&gls->svc, evt->conn_idx, &record);
                break;
#endif
        default:
                /* Should not enter here */
                ASSERT_WARNING(0);
                break;
        }

        return ATT_ERROR_OK;
}

static att_error_t do_racp_write(g_service_t *gls, const ble_evt_gatts_write_req_t *evt)
{
        uint16_t ccc;
        const uint8_t *ptr = (uint8_t *)evt->value;
        uint8_t command;

        /*
         * The data received should be of variable size and so parsing should become gradually.
         * The first byte should represent the requested command.
         */
        if (evt->length < 1) {
                /* As per GLS specifications, if requested command cannot be serviced
                 * RACP should be indicated sending response. However, the command
                 * here cannot be parsed and so a zero value is sent part of the requested
                 * opcode. */
                gls->data.command = GLS_RACP_COMMAND_RFU;
                return ATT_ERROR_INVALID_VALUE_LENGTH;
        }

        /* The first byte should reflect the requested command (opcode) */
        command = get_u8_inc(&ptr);
        gls->data.command = command;

        if (evt->offset) {
                return ATT_ERROR_ATTRIBUTE_NOT_LONG;
        }

        ble_storage_get_u16(evt->conn_idx, gls->racp_ccc_h, &ccc);

        /* CCC descriptor for RACP must be enabled, otherwise RACP cannot be used */
        if (!(ccc & GATT_CCC_INDICATIONS)) {
                return GLS_ATT_CCC_IMPROPERLY_CONFIGURED;
        }

        switch (command) {
        case GLS_RACP_COMMAND_ABORT_OPERATION:
                /* Though not clearly stated in GLS specifications there is no
                 * reason to continue processing if no RACP procedure is in progress. */
                if (!gls->data.racp_in_progress) {
                        gls_indicate_status(&gls->svc, evt->conn_idx, command, GLS_RACP_RESPONSE_SUCCESS);
                        return ATT_ERROR_OK;
                }
                break;
        case GLS_RACP_COMMAND_NUMBER_OF_RECORDS:
        case GLS_RACP_COMMAND_REPORT_RECORDS:
#if GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT
        case GLS_RACP_COMMAND_DELETE_RECORDS:
#endif
                /* Notifications for glucose measurements and glucose measurements contexts optionally
                 * should be enabled. */
                ble_storage_get_u16(evt->conn_idx, gls->gm_ccc_h, &ccc);

                if (!(ccc & GATT_CCC_NOTIFICATIONS)) {
                        return GLS_ATT_CCC_IMPROPERLY_CONFIGURED;
                }

#if GLS_FLAGS_CONTEXT_INFORMATION
                ble_storage_get_u16(evt->conn_idx, gls->gmc_ccc_h, &ccc);

                if (!(ccc & GATT_CCC_NOTIFICATIONS)) {
                        return GLS_ATT_CCC_IMPROPERLY_CONFIGURED;
                }
#endif

                /* Mandated by GLS specifications */
                if (gls->data.racp_in_progress) {
                        return GLS_ATT_PROCEDURE_ALREADY_IN_PROGRESS;
                }
                break;
        default:
                gls_indicate_status(&gls->svc, evt->conn_idx, GLS_RACP_COMMAND_RFU/*Though not clear, we indicate invalid opcode*/, GLS_RACP_RESPONSE_UNSUPPORTED_COMMAND);
                return ATT_ERROR_OK;
        }

        return racp_parse_command(gls, evt, command, ptr);
}

static att_error_t do_generic_ccc_write(g_service_t *gls, const ble_evt_gatts_write_req_t *evt)
{
        uint16_t ccc;

        if (evt->offset) {
                return ATT_ERROR_ATTRIBUTE_NOT_LONG;
        }

        if (evt->length != sizeof(ccc)) {
                return ATT_ERROR_INVALID_VALUE_LENGTH;
        }

        ccc = get_u16(evt->value);

        /* Value should be persistent for bonded devices */
        ble_storage_put_u32(evt->conn_idx, evt->handle, ccc, true);

        if (ccc == GATT_CCC_NOTIFICATIONS &&
                        gls->cb->ccc_enabled) {
                gls->cb->ccc_enabled(&gls->svc, evt);
        }

        return ATT_ERROR_OK;
}

static void handle_write_req(ble_service_t *svc, const ble_evt_gatts_write_req_t *evt)
{
        g_service_t *gls = (g_service_t *)svc;
        att_error_t status = ATT_ERROR_WRITE_NOT_PERMITTED;

        if (evt->handle == gls->gm_ccc_h ||
#if GLS_FLAGS_CONTEXT_INFORMATION
                evt->handle == gls->gmc_ccc_h ||
#endif
#if GLS_FEATURE_INDICATION_PROPERTY
                evt->handle == gls->gf_ccc_h ||
#endif
                evt->handle == gls->racp_ccc_h) {
                status = do_generic_ccc_write(gls, evt);
        } else if (evt->handle == gls->racp_val_h) {
                status = do_racp_write(gls, evt);
        }

        /*
         * Negative status indicates that confirmation will be sent later on on application level via
         * the appropriate registered callback by calling the appropriate confirmation helper API.
         */
        if (status != (att_error_t) -1) {
                ble_gatts_write_cfm(evt->conn_idx, evt->handle, status);
        }

        /* If status is other than OK then record parsing was interrupted due to ATT error.
         * Notify the peer device accordingly. */
        if (status != ATT_ERROR_OK) {
                gls_indicate_status(&gls->svc, evt->conn_idx, gls->data.command, GLS_RACP_RESPONSE_NOT_COMPLETED);
        }
}

static void handle_event_sent(ble_service_t *svc, const ble_evt_gatts_event_sent_t *evt)
{
        g_service_t *gls = (g_service_t *)svc;

        if (gls->gm_val_h == evt->handle ||
#if GLS_FLAGS_CONTEXT_INFORMATION
                gls->gmc_val_h == evt->handle ||
#endif
#if GLS_FEATURE_INDICATION_PROPERTY
                gls->gf_val_h == evt->handle ||
#endif
                gls->racp_val_h == evt->handle) {
                if (gls->cb->event_sent) {
                        gls->cb->event_sent(svc, evt);
                }
        }
}

static void handle_cleanup(ble_service_t *svc)
{
        g_service_t *gls = (g_service_t *)svc;

        ble_storage_remove_all(gls->gm_ccc_h);
#if GLS_FLAGS_CONTEXT_INFORMATION
        ble_storage_remove_all(gls->gmc_ccc_h);
#endif
        ble_storage_remove_all(gls->racp_ccc_h);

        /* Remove space allocated at service instantiation. */
        OS_FREE(gls);
}

#if GLS_FEATURE_INDICATION_PROPERTY
#define GLS_FEATURE_CCC_REGISTER(gls)  (&((g_service_t *)(gls))->gf_ccc_h) ,
#else
#define GLS_FEATURE_CCC_REGISTER(gls)
#endif

#if GLS_FLAGS_CONTEXT_INFORMATION
#define GLS_CONTEXT_VALUE_REGISTER(gls) (&((g_service_t *)(gls))->gmc_val_h) ,
#else
#define GLS_CONTEXT_VALUE_REGISTER(gls)
#endif

#if GLS_FLAGS_CONTEXT_INFORMATION
#define GLS_CONTEXT_CCC_REGISTER(gls) (&((g_service_t *)(gls))->gmc_ccc_h) ,
#else
#define GLS_CONTEXT_CCC_REGISTER(gls)
#endif

ble_service_t *gls_init(const gls_callbacks_t *cb)
{
        g_service_t *gls;
        att_uuid_t uuid;
        uint16_t num_attr;

        ASSERT_WARNING(cb &&
                cb->racp_callbacks.abort_operation &&
                cb->racp_callbacks.report_num_of_records &&
#if GLS_FEATURE_INDICATION_PROPERTY
                cb->get_features &&
#endif
#if GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT
                cb->racp_callbacks.delete_records &&
#endif
                cb->racp_callbacks.report_records);

        gls = OS_MALLOC(sizeof(*gls));
        OS_ASSERT(gls);
        memset(gls, 0, sizeof(*gls));

        gls->svc.read_req = handle_read_req;
        gls->svc.write_req = handle_write_req;
        gls->svc.cleanup = handle_cleanup;
        gls->svc.event_sent = handle_event_sent;
        gls->cb = cb;

        num_attr = ble_gatts_get_num_attr(0, 3 + (GLS_FLAGS_CONTEXT_INFORMATION ? 1 : 0),
                                             2 + (GLS_FLAGS_CONTEXT_INFORMATION ? 1 : 0) +
                                                 (GLS_FEATURE_INDICATION_PROPERTY ? 1 : 0));

        ble_uuid_create16(UUID_GLUCOSE_SERVICE, &uuid);
        ble_gatts_add_service(&uuid, GATT_SERVICE_PRIMARY, num_attr);

        ble_uuid_create16(UUID_GLUCOSE_MEASUREMENT, &uuid);
        ble_gatts_add_characteristic(&uuid, GATT_PROP_NOTIFY, ATT_PERM_NONE, 0, 0, NULL, &gls->gm_val_h);

        ble_uuid_create16(UUID_CCC, &uuid);
        /* When the flags is zero, and for notifications, read requests can still be processed. */
        ble_gatts_add_descriptor(&uuid, ATT_PERM_RW, 2, 0, &gls->gm_ccc_h);

#if GLS_FLAGS_CONTEXT_INFORMATION
        ble_uuid_create16(UUID_GLUCOSE_MEASUREMENT_CONTEXT, &uuid);
        ble_gatts_add_characteristic(&uuid, GATT_PROP_NOTIFY, ATT_PERM_NONE, 0, 0, NULL, &gls->gmc_val_h);

        ble_uuid_create16(UUID_CCC, &uuid);
        /* When the flags is zero, and for notifications, read requests can still be processed. */
        ble_gatts_add_descriptor(&uuid, ATT_PERM_RW, 2, 0, &gls->gmc_ccc_h);
#endif

        ble_uuid_create16(UUID_GLUCOSE_FEATURE, &uuid);
        ble_gatts_add_characteristic(&uuid,
                GATT_PROP_READ | (GLS_FEATURE_INDICATION_PROPERTY ? GATT_PROP_INDICATE : 0),
                ATT_PERM_READ, 0, GATTS_FLAG_CHAR_READ_REQ, NULL, &gls->gf_val_h);

#if GLS_FEATURE_INDICATION_PROPERTY
        ble_uuid_create16(UUID_CCC, &uuid);
        ble_gatts_add_descriptor(&uuid, ATT_PERM_RW, 2, 0, &gls->gf_ccc_h);
#endif

        ble_uuid_create16(UUID_GLUCOSE_RACP, &uuid);
        ble_gatts_add_characteristic(&uuid, GATT_PROP_WRITE | GATT_PROP_INDICATE, ATT_PERM_WRITE_AUTH,
                3 + (sizeof(gls_base_user_facing_time_t) << 1), 0, NULL, &gls->racp_val_h);

        ble_uuid_create16(UUID_CCC, &uuid);
        /* When the flags is zero, and for notifications, read requests can still be processed. */
        ble_gatts_add_descriptor(&uuid, ATT_PERM_RW, 2, 0, &gls->racp_ccc_h);

        ble_gatts_register_service(&gls->svc.start_h, &gls->gm_val_h, &gls->gm_ccc_h, &gls->gf_val_h,
                &gls->racp_val_h, &gls->racp_ccc_h,
                GLS_CONTEXT_VALUE_REGISTER(gls) GLS_CONTEXT_CCC_REGISTER(gls) GLS_FEATURE_CCC_REGISTER(gls) 0);

        gls->svc.end_h = gls->svc.start_h + num_attr;

        ble_service_add(&gls->svc);

        return &gls->svc;
}
