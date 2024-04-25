/**
 ****************************************************************************************
 *
 * @file glucose_sensor_database.c
 *
 * @brief Glucose sensor database simple framework
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
#include "ble_gap.h"
#include "ble_gatts.h"
#include "glucose_sensor_database.h"
#include "glucose_service.h"
#include "svc_types.h"
#include "sdk_list.h"
#include "ad_nvms.h"

__RETAINED static app_db_data_t db_data;

/* List used to maintain records for the GLS ATT database */
__RETAINED static void *gls_database_records;

/* Synchronization semaphore required as database can be updated at any time when
 * a RACP parsing request is in progress. */
__RETAINED static OS_MUTEX app_db_sync;

/*
 * Helper macro to call a function until success is returned
 *
 * \param [in] _f     function to be called
 * \param [in] _s     value that should indicate success
 * \param [in] args   function arguments
 */
#define APP_CALL_FUNCTION_UNTILL_SUCCESS(_f, _s, args...) \
        typeof(_s) status;                                \
        do {                                              \
                status = _f(args);                        \
        } while (status != _s);                           \


/* Header structure appended at the start of the flash storage. */
typedef struct __packed {
        uint16_t current_seq_number;
        size_t num_of_records;
} storage_header_t;

__RETAINED static uint8_t current_sn;

// compile-time assertion
#define C_ASSERT(cond) typedef char __c_assert[(cond) ? 1 : -1] __attribute__((unused))

void app_db_init(void)
{
        C_ASSERT(APP_DB_MAX_LIST_LEN);

        OS_MUTEX_CREATE(app_db_sync);
}

uint16_t app_db_get_sequence_number(void)
{
        /* SN is not permitted to roll over, although a reset might occur in case of non-volatile storage
         * failure. It is also recommended that SN be stored in non-volatile memory to ensure consistency.  */
        if (current_sn == 0xFFFF) {
                return current_sn;
        } else {
                return current_sn++;
        }
}

static bool racp_records_compare(const void *elem, const void *ud)
{
        app_db_record_entry_t *record = (app_db_record_entry_t *)elem;
        app_db_record_entry_t *data = (app_db_record_entry_t *)ud;

        return (record->record.measurement.seq_number == data->record.measurement.seq_number);
}

void app_db_add_record_entry(app_db_add_record_entry_cb_t cb)
{
        ASSERT_WARNING(cb);

        app_db_record_entry_t *record;
        uint8_t size;

        OS_MUTEX_GET(app_db_sync, OS_MUTEX_FOREVER);

        size = list_size(gls_database_records);

        /* As per GLS specifications, if the max. storage capacity has been reached the oldest
         * value should be overwritten. List header should point to the oldest entry */
        if (size == APP_DB_MAX_LIST_LEN) {
                list_remove(&gls_database_records, racp_records_compare, (const void *)gls_database_records);
        }

        record = OS_MALLOC(sizeof(*record));
        if (record) {
                memset(record, 0, sizeof(*record));

                /* Call user's callback to initialize the record */
                cb(&record->record);

                /* Elements are generated in chronological order */
                list_append(&gls_database_records, record);
        }

        OS_MUTEX_PUT(app_db_sync);
}

void app_db_update_racp_request(ble_service_t *svc, uint16_t conn_idx, uint8_t command, gls_racp_t *record)
{
        if (record->filter_type == GLS_RACP_FILTER_TYPE_SN) {
                OPT_MEMCPY(&db_data.filter_param.seq_number,
                                        record->filter_param, record->filter_param_len);
        } else if (record->filter_type == GLS_RACP_FILTER_TYPE_UFT) {
                OPT_MEMCPY(&db_data.filter_param.data_time,
                                        record->filter_param, record->filter_param_len);
        }

        db_data.operator = record->operator;
        db_data.filter_type = record->filter_type;
        db_data.conn_idx = conn_idx;
        db_data.svc = svc;
        db_data.command = command;
}

static void racp_report_record_foreach_cb(const void *elem, const void *ud)
{
        app_db_record_entry_t *record = (app_db_record_entry_t *)elem;
        app_db_data_t *racp = (app_db_data_t *)ud;

        switch (racp->operator) {
        case GLS_RACP_OPERATOR_ALL_RECORDS:
                if (racp->command == GLS_RACP_COMMAND_REPORT_RECORDS) {
                        /* No operands are used; just notify collector */

                        APP_CALL_FUNCTION_UNTILL_SUCCESS(gls_notify_record, (bool)true,
                                                        racp->svc, racp->conn_idx, &record->record);

                        /* Success if at least one record matches criteria */
                        racp->status = GLS_RACP_RESPONSE_SUCCESS;
                } else {
                        racp->num_of_records++;
                }
                break;
        case GLS_RACP_OPERATOR_GREATER_EQUAL:
                if (racp->filter_type == GLS_RACP_FILTER_TYPE_SN) {
                        if (record->record.measurement.seq_number >= racp->filter_param.seq_number[0]) {
                                if (racp->command == GLS_RACP_COMMAND_REPORT_RECORDS) {

                                        APP_CALL_FUNCTION_UNTILL_SUCCESS(gls_notify_record, (bool)true,
                                                                        racp->svc, racp->conn_idx, &record->record);

                                        /* Success if at least one record matches criteria */
                                        racp->status = GLS_RACP_RESPONSE_SUCCESS;
                                } else {
                                        racp->num_of_records++;
                                }
                        }
                }
#if GLS_RACP_FILTER_USER_FACING_TIME_SUPPORT
                else {
                        /* Not supported; just indicate that no records matched criteria. */
                        C_ASSERT(0);
                }
#endif
                break;
#if GLS_RACP_OPERATOR_LESS_EQUAL_SUPPORT
        case GLS_RACP_OPERATOR_LESS_EQUAL:
                if (racp->filter_type == GLS_RACP_FILTER_TYPE_SN) {
                        if (record->record.measurement.seq_number <= racp->filter_param.seq_number[0]) {
                                if (racp->command == GLS_RACP_COMMAND_REPORT_RECORDS) {

                                        APP_CALL_FUNCTION_UNTILL_SUCCESS(gls_notify_record, (bool)true,
                                                                        racp->svc, racp->conn_idx, &record->record);

                                        /* Success if at least one record matches criteria */
                                        racp->status = GLS_RACP_RESPONSE_SUCCESS;
                                } else {
                                        racp->num_of_records++;
                                }
                        }
                }
#if GLS_RACP_FILTER_USER_FACING_TIME_SUPPORT
                else {
                        /* Not supported; just indicate that no records matched criteria. */
                        C_ASSERT(0);
                }
#endif
                break;
#endif /* GLS_RACP_OPERATOR_LESS_EQUAL_SUPPORT */
#if GLS_RACP_OPERATOR_WITHIN_RANGE_SUPPORT
        case GLS_RACP_OPERATOR_WITHIN_RANGE:
                if (racp->filter_type == GLS_RACP_FILTER_TYPE_SN) {
                        if (record->record.measurement.seq_number >= racp->filter_param.seq_number[0] &&
                                record->record.measurement.seq_number <= racp->filter_param.seq_number[1]) {
                                if (racp->command == GLS_RACP_COMMAND_REPORT_RECORDS) {

                                        APP_CALL_FUNCTION_UNTILL_SUCCESS(gls_notify_record, (bool)true,
                                                                        racp->svc, racp->conn_idx, &record->record);

                                        /* Success if at least one record matches criteria */
                                        racp->status = GLS_RACP_RESPONSE_SUCCESS;
                                } else {
                                        racp->num_of_records++;
                                }
                        }
                }
#if GLS_RACP_FILTER_USER_FACING_TIME_SUPPORT
                else {
                        /* Not supported; just indicate that no records matched criteria. */
                        C_ASSERT(0);
                }
#endif
                break;
#endif /* GLS_RACP_OPERATOR_WITHIN_RANGE_SUPPORT */
        default:
                /* Should not enter here */
                ASSERT_WARNING(0);
        }
}

static bool racp_delete_records_foreach_cb(const void *elem, const void *ud)
{
        app_db_record_entry_t *record = (app_db_record_entry_t *)elem;
        app_db_data_t *racp = (app_db_data_t *)ud;
        bool ret = false;

        switch (racp->operator) {
        case GLS_RACP_OPERATOR_ALL_RECORDS:
                /* No operands are used; just delete the entry  */
                ret = true;

                /* Success if at least one record matches criteria */
                racp->status = GLS_RACP_RESPONSE_SUCCESS;
                racp->num_of_records--;
                break;
        case GLS_RACP_OPERATOR_GREATER_EQUAL:
                if (racp->filter_type == GLS_RACP_FILTER_TYPE_SN) {
                        if (record->record.measurement.seq_number >= racp->filter_param.seq_number[0]) {
                                ret = true;

                                /* Success if at least one record matches criteria */
                                racp->status = GLS_RACP_RESPONSE_SUCCESS;
                                racp->num_of_records--;
                        }
                }
#if GLS_RACP_FILTER_USER_FACING_TIME_SUPPORT
                else {
                        /* Not supported; just indicate that no records matched criteria. */
                        C_ASSERT(0);
                }
#endif
                break;
#if GLS_RACP_OPERATOR_LESS_EQUAL_SUPPORT
        case GLS_RACP_OPERATOR_LESS_EQUAL:
                if (racp->filter_type == GLS_RACP_FILTER_TYPE_SN) {
                        if (record->record.measurement.seq_number <= racp->filter_param.seq_number[0]) {
                                ret = true;

                                /* Success if at least one record matches criteria */
                                racp->status = GLS_RACP_RESPONSE_SUCCESS;
                                racp->num_of_records--;
                        }
                }
#if GLS_RACP_FILTER_USER_FACING_TIME_SUPPORT
                else {
                        /* Not supported; just indicate that no records matched criteria. */
                        C_ASSERT(0);
                }
#endif
                break;
#endif /* GLS_RACP_OPERATOR_LESS_EQUAL_SUPPORT */
#if GLS_RACP_OPERATOR_WITHIN_RANGE_SUPPORT
        case GLS_RACP_OPERATOR_WITHIN_RANGE:
                if (racp->filter_type == GLS_RACP_FILTER_TYPE_SN) {
                        if (record->record.measurement.seq_number >= racp->filter_param.seq_number[0] &&
                                record->record.measurement.seq_number <= racp->filter_param.seq_number[1]) {
                                ret = true;
                                /* Success if at least one record matches criteria */
                                racp->status = GLS_RACP_RESPONSE_SUCCESS;
                                racp->num_of_records--;
                        }
                }
#if GLS_RACP_FILTER_USER_FACING_TIME_SUPPORT
                else {
                        /* Not supported; just indicate that no records matched criteria. */
                        C_ASSERT(0);
                }
#endif
                break;
#endif /* GLS_RACP_OPERATOR_WITHIN_RANGE_SUPPORT */
        default:
                /* Should not enter here */
                ASSERT_WARNING(0);
        }

        return ret;
}

void app_db_report_num_of_records_handle(void)
{
        OS_MUTEX_GET(app_db_sync, OS_MUTEX_FOREVER);

        db_data.num_of_records = 0;

        /* It is assumed that RACP requests sanity checks are performed by the service */
        switch (db_data.operator) {
#if GLS_RACP_OPERATOR_FIRST_RECORD_SUPPORT
        case GLS_RACP_OPERATOR_FIRST_RECORD:
        {
                app_db_record_entry_t *record = gls_database_records;

                if (record) {
                        db_data.num_of_records = 1;
                }
        }
                break;
#endif /* GLS_RACP_OPERATOR_FIRST_RECORD_SUPPORT */
#if GLS_RACP_OPERATOR_LAST_RECORD_SUPPORT
        case GLS_RACP_OPERATOR_LAST_RECORD:
        {
                app_db_record_entry_t *record = list_peek_back(&gls_database_records);
                if (record) {
                        db_data.num_of_records = 1;
                }
        }
                break;
#endif /* GLS_RACP_OPERATOR_LAST_RECORD_SUPPORT */
        default:
                /* The rest operators require parsing the database records */
                list_foreach(gls_database_records, racp_report_record_foreach_cb,
                                                                (const void *)&db_data);
                break;
        }

        OS_MUTEX_PUT(app_db_sync);

        /* If no entries are found zero value should be indicated */
        gls_indicate_number_of_stored_records(db_data.svc, db_data.conn_idx,
                                                        db_data.num_of_records);
}

void app_db_report_records_handle(void)
{
        OS_MUTEX_GET(app_db_sync, OS_MUTEX_FOREVER);

        db_data.status = GLS_RACP_RESPONSE_NO_RECORDS;

        /* It is assumed that RACP requests sanity checks are performed by the service */
        switch (db_data.operator) {
#if GLS_RACP_OPERATOR_FIRST_RECORD_SUPPORT
        case GLS_RACP_OPERATOR_FIRST_RECORD:
        {
                app_db_record_entry_t *record = gls_database_records;

                if (record) {
                        APP_CALL_FUNCTION_UNTILL_SUCCESS(gls_notify_record, (bool)true,
                                                db_data.svc, db_data.conn_idx, &record->record);
                        db_data.status = GLS_RACP_RESPONSE_SUCCESS;
                }
        }
                break;
#endif /* GLS_RACP_OPERATOR_FIRST_RECORD_SUPPORT */
#if GLS_RACP_OPERATOR_LAST_RECORD_SUPPORT
        case GLS_RACP_OPERATOR_LAST_RECORD:
        {
                app_db_record_entry_t *record = list_peek_back(&gls_database_records);
                if (record) {
                        APP_CALL_FUNCTION_UNTILL_SUCCESS(gls_notify_record, (bool)true,
                                                db_data.svc, db_data.conn_idx, &record->record);
                        db_data.status = GLS_RACP_RESPONSE_SUCCESS;
                }
        }
                break;
#endif /* GLS_RACP_OPERATOR_LAST_RECORD_SUPPORT */
        default:
                /* The rest operators require parsing the database records */
                list_foreach(gls_database_records, racp_report_record_foreach_cb,
                                                                (const void *)&db_data);
                break;
        }
        OS_MUTEX_PUT(app_db_sync);

        /* Last step is to indicate collector */
        gls_indicate_report_records_status(db_data.svc, db_data.conn_idx, db_data.status);
}

void app_db_delete_records_handle(void)
{
        OS_MUTEX_GET(app_db_sync, OS_MUTEX_FOREVER);

        db_data.status = GLS_RACP_RESPONSE_NO_RECORDS;

        /* It is assumed that RACP requests sanity checks are performed by the service */
        switch (db_data.operator) {
#if GLS_RACP_OPERATOR_FIRST_RECORD_SUPPORT
        case GLS_RACP_OPERATOR_FIRST_RECORD:
        {
                app_db_record_entry_t *first_entry;

                first_entry = gls_database_records;
                if (first_entry) {
                        list_remove(&gls_database_records, racp_records_compare, (const void *)first_entry);
                        db_data.status = GLS_RACP_RESPONSE_SUCCESS;
                }
        }
                break;
#endif
#if GLS_RACP_OPERATOR_LAST_RECORD_SUPPORT
        case GLS_RACP_OPERATOR_LAST_RECORD:
        {
                app_db_record_entry_t *last_entry;

                last_entry = list_peek_back(&gls_database_records);
                if (last_entry) {
                        list_remove(&gls_database_records, racp_records_compare, (const void *)last_entry);
                        db_data.status = GLS_RACP_RESPONSE_SUCCESS;
                }
        }
                break;
#endif
        default:
                /* The rest operators require parsing the database records */
                list_filter(&gls_database_records, racp_delete_records_foreach_cb,
                                                        (const void *)&db_data);
                break;
        }

        OS_MUTEX_PUT(app_db_sync);

#if GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT
        /* Last step is to indicate collector */
        gls_indicate_delete_records_status(db_data.svc, db_data.conn_idx,
                                                                        db_data.status);
#endif
}
