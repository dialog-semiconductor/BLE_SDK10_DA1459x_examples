/**
 ****************************************************************************************
 *
 * @file glucose_sensor_database.h
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

#ifndef SRC_GLUCOSE_SENSOR_DATABASE_H_

#define SRC_GLUCOSE_SENSOR_DATABASE_H_

#include "glucose_service.h"

/* Structure holding the record element in the list database. */
typedef struct {
        /* Should always be the first element; will be used by the list framework internally. */
        void *next;
        gls_record_t record;
} app_db_record_entry_t;

/* Structure holding the operand value as provided by a RACP write request */
typedef union {
        uint16_t seq_number[2];
        gls_base_user_facing_time_t data_time[2];
} app_db_operand_value_t;

/* Structure holding data elements used to handle RACP write requests. */
typedef struct {
        ble_service_t *svc;
        uint16_t conn_idx;
        uint8_t operator;
        uint8_t filter_type;
        app_db_operand_value_t filter_param;
        uint8_t status;
        uint8_t command;
        uint16_t num_of_records;
} app_db_data_t;

/*
 * Max number of records supported by database. As per GLS specifications,
 * if the max. storage capacity is reached, the oldest record should
 * be overwritten.
 */
#ifndef APP_DB_MAX_LIST_LEN
#define APP_DB_MAX_LIST_LEN      10
#endif

/*
 * Base time is typically set at the time of manufacturing or upon first use
 * by the end user and is not intended to be updated by the user or with any
 * other means except in the case of an unrecoverable loss of time.
 */
#ifndef APP_DB_BASE_TIME_YEAR
#define APP_DB_BASE_TIME_YEAR      2023
#endif

#ifndef APP_DB_BASE_TIME_MONTH
#define APP_DB_BASE_TIME_MONTH     11
#endif

#ifndef APP_DB_BASE_TIME_DAY
#define APP_DB_BASE_TIME_DAY       10
#endif

#ifndef APP_DB_BASE_TIME_HOURS
#define APP_DB_BASE_TIME_HOURS     10
#endif

#ifndef APP_DB_BASE_TIME_MINUTES
#define APP_DB_BASE_TIME_MINUTES   10
#endif

#ifndef APP_DB_BASE_TIME_SECONDS
#define APP_DB_BASE_TIME_SECONDS   10
#endif

/*
 * Function signature for the callback function that should be provided by users when a new entry
 * is requested to be reserved. Via this callback function application should be able to
 * initialize the various entries of the newly generated record entry. This callback function should
 * be used in \sa app_db_add_record_entry.
 */
typedef void (*app_db_add_record_entry_cb_t)(gls_record_t * const record);

/*
 * Database initialization routine. This function should be called once to initialize the
 * various parameters and perform the necessary checks required by the application GLS
 * database.
 */
void app_db_init(void);

/*
 * Request a new record area to be reserved. By calling this function, a new record memory area
 * will be allocated and will be linked in the application GLS database. Users should provide a
 * callback function which will be called by the database so application can assign the necessary
 * values of the various record fields.
 *
 * \param [in] cb  callback function so application can initialize the reserved record memory area.
 *
 */
void app_db_add_record_entry(app_db_add_record_entry_cb_t cb);

/*
 * Function to be called by application when a RACP user callback function is called. This
 * function will update a structure that reflects the current RACP request. Keep in
 * mind that as per GLS specifications a ATT error will be sent to the peer device if
 * a RACP request handling is in progress. Valid contexts to be called within are:
 *
 * \sa report_num_of_records
 * \sa report_records
 * \sa delete_records
 *
 * \param [in] svc         glucose database instance as provided by the service through the
 *                         callback function
 * \param [in] conn_idx    connection index indicating the peer device that raised the RACP write request;
 *                         should be provided by the service through the callback function
 * \param [in] command     Requested op code/command value; should be provided by application based on
 *                         the callback function within which the API is called.
 * \param [in] record      pointer to the RACP packet; should be provided by the service through
 *                         the callback function.
 *
 */
void app_db_update_racp_request(ble_service_t *svc, uint16_t conn_idx, uint8_t command, gls_racp_t *record);

/*
 * Function to be called by application when a RACP request to get the number of stored records
 * has been received through the \sa report_num_of_records registered callback function.
 * Typical usage is that, a notification is sent from within the registered callback function
 * so eventually the handle of the request is differed to be handled by application task which
 * should have lower priority compared to the BLE manager task. In doing so, the BLE manager
 * is freed to service other BLE requests as long as the application is tasked to service the
 * current RACP request.
 * This function will count the number of elements that match the request criteria. Once all records
 * are parsed the function will call \sa gls_indicate_number_of_stored_records as mandated by GLS
 * specifications. If no records are found to meet the request criteria then a zero value is returned
 * as response.
 */
void app_db_report_num_of_records_handle(void);

/*
 * Function to be called by application when a report RACP request has been received
 * through the \sa report_records registered callback function.
 * Typical usage is that, a notification is sent from within the registered callback function
 * so eventually the handle of the request is differed to be handled by application task which
 * should have lower priority compared to the BLE manager task. In doing so, the BLE manager
 * is freed to service other BLE requests as long as the application is tasked to service the
 * current RACP request.
 * This function will parse the report request by traversing all records entries in the
 * database. Elements that match the report criteria will be notified in the RACP characteristic.
 * Once all records are parsed the function will call \sa gls_indicate_report_records_status as mandated
 * by GLS specifications. If no records are found to meet the deletion criteria
 * \sa GLS_RACP_RESPONSE_NO_RECORDS is returned to the collector.
 */
void app_db_report_records_handle(void);

/*
 * Function to be called by application when a delete RACP request has been received
 * through the \sa delete_records registered callback function.
 * Typical usage is that, a notification is sent from within the registered callback function
 * so eventually the handle of the request is differed to be handled by application task which
 * should have lower priority compared to the BLE manager task. In doing so, the BLE manager
 * is freed to service other BLE requests as long as the application is tasked to service the
 * current RACP request.
 * This function will parse the delete request by traversing all records entries in the
 * database. Elements that match the deletion criteria will be unlinked from the database
 * and the reserved spaced will be freed. Once all records are parsed the function will
 * call \sa gls_indicate_delete_records_status as mandated by GLS specifications. If
 * no records are found to meet the deletion criteria \sa GLS_RACP_RESPONSE_NO_RECORDS
 * is returned to the collector.
 */
void app_db_delete_records_handle(void);

/*
 * Function to get a unique sequence number. This function should be called by application to initialize
 * the SN entry of a newly reserved record via \sa app_db_add_record_entry. If NVMS support is in place,
 * the SN value is stored so continuum is preserved across device reboots as suggested by GLS
 * specifications.
 */
uint16_t app_db_get_sequence_number(void);

#endif /* SRC_GLUCOSE_SENSOR_DATABASE_H_ */
