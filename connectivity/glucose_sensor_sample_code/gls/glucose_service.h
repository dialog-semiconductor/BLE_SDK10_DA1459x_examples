/**
 ****************************************************************************************
 *
 * @file glucose_service.h
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

#ifndef GLUCOSE_SERVICE_H_

#define GLUCOSE_SERVICE_H_

#include "ble_service.h"
#include "glucose_service_default.h"

/* Officially defined 16-bit UUID numbers */
#define UUID_GLUCOSE_SERVICE                  ( 0x1808 )
#define UUID_GLUCOSE_MEASUREMENT              ( 0x2A18 )
#define UUID_GLUCOSE_MEASUREMENT_CONTEXT      ( 0x2A34 )
#define UUID_GLUCOSE_FEATURE                  ( 0x2A51 )
#define UUID_GLUCOSE_RACP                     ( 0x2A52 )

#define ARG_UNUSED(arg)  (void)(arg)

typedef struct __packed {
        uint8_t operator;
        uint8_t filter_type;
        const uint8_t *filter_param;
        size_t filter_param_len;
} gls_racp_t;

typedef uint16_t sfloat;

/*
 * Packed version of \sa svc_data_time_t structure. This structure should be used to format
 * base and user facing time fields.
 *
 * \note Zero value, which otherwise would mean unknown, for the year, month or day fields
 *       shall not be used for the Glucose Service.
 */
typedef struct __packed {
        uint16_t year;          /**< 1582 .. 9999 */
        uint8_t  month;         /**< 1..12 */
        uint8_t  day;           /**< 1..31*/
        uint8_t  hours;         /**< 0..23 */
        uint8_t  minutes;       /**< 0..59 */
        uint8_t  seconds;       /**< 0..59 */
} gls_base_user_facing_time_t;

/* Structure of the \p status field of the measurement packet. */
typedef struct {
        union {
                struct {
                        uint16_t battery_low         : 1; /* Device battery low at time of measurement. */
                        uint16_t malfunction         : 1; /* Sensor malfunction or faulting at time of measurement. */
                        uint16_t sample_insufficient : 1; /* Sample size for blood or control solution insufficient at time of measurement.  */
                        uint16_t strip_insertion     : 1; /* Strip insertion error. */
                        uint16_t strip_type          : 1; /* Strip type incorrect for device. */
                        uint16_t result_higher       : 1; /* Sensor result higher than the device can process. */
                        uint16_t result_lower        : 1; /* Sensor result lower than the device can process. */
                        uint16_t temperature_high    : 1; /* Sensor temperature too high for valid test/result at time of measurement. */
                        uint16_t temperature_low     : 1; /* Sensor temperature too low for valid test/result at time of measurement. */
                        uint16_t read_interruption   : 1; /* Sensor read interrupted because strip was pulled too soon at the time of measurement. */
                        uint16_t general_error       : 1; /* General device fault has occurred in the sensor. */
                        uint16_t time_error          : 1; /* Time fault has occurred in the sensor and time may be inaccurate. */
                };
                uint16_t status;
        };
} gls_status_t;

/* Packet structure of the GLS measurement characteristic value. */
typedef struct __packed {
        uint8_t flags;
        uint16_t seq_number;
        gls_base_user_facing_time_t base_time;
        int16_t time_offset;
#if GLS_FLAGS_CONCENTRATION_TYPE_SAMPLE_LOCATION
        sfloat concentration;
        union {
                struct {
                        uint8_t type            : 4;
                        uint8_t sample_location : 4;
                };
                uint8_t type_sample_location;
        };
#endif
#if GLS_FLAGS_STATUS_ANNUNCIATION
        gls_status_t status;
#endif
} gls_measurement_t;

/* Packet structure of the GLS measurement context characteristic value. */
typedef struct __packed {
        uint8_t flags;
        uint16_t seq_number;
#if GLS_CONTEXT_FLAGS_EXTENDED_FLAGS
        uint8_t extended_flags;
#endif
#if GLS_CONTEXT_FLAGS_CARBOHYDRATE_ID
        uint8_t carbohydrate_id;
        sfloat carbohydrate; /* In units of kilograms */
#endif
#if GLS_CONTEXT_FLAGS_MEAL
        uint8_t meal;
#endif
#if GLS_CONTEXT_FLAGS_TESTER_HEALTH
        union {
                uint8_t tester_health;
                struct {
                        uint8_t tester : 4;
                        uint8_t health : 4;
                };
        };
#endif
#if GLS_CONTEXT_FLAGS_EXERCISE_DURATION_INTENSITY
        uint16_t exercise_duration; /* In seconds units. [0, 65534], 65535 is translated as "overrun". */
        uint8_t exercise_intensity; /* In percentage units */
#endif
#if GLS_CONTEXT_FLAGS_MEDICATION_ID
        uint8_t medication_id;
        sfloat medication;
#endif
#if GLS_CONTEXT_FLAGS_HbA1c
        sfloat HbA1c;
#endif
} gls_context_t;

typedef struct __packed {
        gls_measurement_t measurement;
#if GLS_FLAGS_CONTEXT_INFORMATION
        gls_context_t context;
#endif
} gls_record_t;

/* Valid values for the Type nibble field of the GLS measurement characteristic value. */
typedef enum {
        GLS_TYPE_RFU = 0x0, /* Reserved for future use */
        GLS_TYPE_CAPILLARY_WHOLE_BLOOD,
        GLS_TYPE_CAPILLARY_PLASMA,
        GLS_TYPE_VENOUS_WHOLE_BLOOD,
        GLS_TYPE_VENOUS_PLASMA,
        GLS_TYPE_ARTERIAL_WHOLE_BLOOD,
        GLS_TYPE_ARTERIAL_PLASMA,
        GLS_TYPE_UNDETERMINED_WHOLE_BOOLD,
        GLS_TYPE_UNDETERMINED_PLASMA,
        GLS_TYPE_INTERSTISIAL_FLUID,
        GLS_TYPE_CONTROL_SOLUTION,
        GLS_TYPE_MAX
} GLS_TYPE;

/* Valid values for the Sample Location nibble field of the GLS measurement characteristic value.  */
typedef enum {
        GLS_SAMPLE_LOCATION_RFU = 0x0,
        GLS_SAMPLE_LOCATION_FINGER,
        GLS_SAMPLE_LOCATION_ALTERNATE_SITE_TEST,
        GLS_SAMPLE_LOCATION_EARLOBE,
        GLS_SAMPLE_LOCATION_CONTROL_SOLUTION,
        GLS_SAMPLE_CONTROL_SOLUTION_NOT_AVAILABLE = 0xF
} GLS_SAMPLE_LOCATION;

/* Valid values for the Carbohydrate ID field of the GLS measurement context characteristic value. */
typedef enum {
        GLS_CARBOHYDRATE_ID_RFU = 0x0,
        GLS_CARBOHYDRATE_ID_BREAKFAST,
        GLS_CARBOHYDRATE_ID_LUNCH,
        GLS_CARBOHYDRATE_ID_DINNER,
        GLS_CARBOHYDRATE_ID_SNACK,
        GLS_CARBOHYDRATE_ID_DRINK,
        GLS_CARBOHYDRATE_ID_SUPPER,
        GLS_CARBOHYDRATE_ID_BRUNCH,
        GLS_CARBOHYDRATE_ID_MAX
} GLS_CARBOHYDRATE_ID;

/* Valid values for the Meal field of the of GLS measurement context characteristic value.  */
typedef enum {
        GLS_MEAL_RFU = 0x0,
        GLS_MEAL_PREPRANDIAL,    /* Before meal */
        GLS_MEAL_POSTPRANDIAL,   /* After meal */
        GLS_MEAL_FASTING,
        GLS_MEAL_CASUAL,         /* snacks, drinks etc. */
        GLS_MEAL_BEDTIME,
        GLS_MEAL_MAX
} GLS_MEAL;

/* Valid values for the Tester nibble field of the of GLS measurement context characteristic value. */
typedef enum {
        GLS_TESTER_RFU = 0x0,
        GLS_TESTER_SELF,
        GLS_TESTER_HEALTH_CARE_PROFESSIONAL,
        GLS_TESTER_LAB_TEST,
        GLS_TESTER_NOT_AVAILABLE = 0xF
} GLS_TESTER;

/* Valid values for the Health nibble field of the of GLS measurement context characteristic value. */
typedef enum {
        GLS_HEALTH_RFU = 0x0,
        GLS_HEALTH_MINOR_ISSUES,
        GLS_HEALTH_MAJOR_ISSUES,
        GLS_HEALTH_DURING_MENSES,
        GLS_HEALTH_UNDER_STRESS,
        GLS_HEALTH_NO_ISSUES,
        GLS_HEALTH_NOT_AVAILABLE = 0xF
} GLS_HEALTH;

/* Valid values for the Medication ID field of the of GLS measurement context characteristic value. */
typedef enum {
        GLS_MEDICATION_ID_RFU = 0x0,
        GLS_MEDICATION_ID_RAPID_ACTING_INSULIN,
        GLS_MEDICATION_ID_SHORT_ACTING_INSULIN,
        GLS_MEDICATION_ID_INTERMEDIATE_ACTING_INSULIN,
        GLS_MEDICATION_ID_LONG_ACTING_INSULIN,
        GLS_MEDICATION_ID_PREMIXED_INSULIN,
        GLS_MEDICATION_ID_MAX
} GLS_MEDICATION_ID;

/* Valid opcode values of a RACP packet */
typedef enum {
        GLS_RACP_COMMAND_RFU = 0x0,
        GLS_RACP_COMMAND_REPORT_RECORDS,
        GLS_RACP_COMMAND_DELETE_RECORDS,
        GLS_RACP_COMMAND_ABORT_OPERATION,
        GLS_RACP_COMMAND_NUMBER_OF_RECORDS,
        GLS_RACP_COMMAND_NUMBER_OF_RECORDS_RESPONSE,
        GLS_RACP_COMMAND_RESPONSE,
        GLS_RACP_COMMAND_MAX
} GLS_RACP_COMMAD;

/* Valid Operator values of a RACP packet */
typedef enum {
        GLS_RACP_OPERATOR_NULL = 0x0,
        GLS_RACP_OPERATOR_ALL_RECORDS,
        GLS_RACP_OPERATOR_LESS_EQUAL,
        GLS_RACP_OPERATOR_GREATER_EQUAL,
        GLS_RACP_OPERATOR_WITHIN_RANGE,
        GLS_RACP_OPERATOR_FIRST_RECORD,  /* Oldest record */
        GLS_RACP_OPERATOR_LAST_RECORD,   /* Most recent record */
        GLS_RACP_OPERATOR_MAX
} GLS_RACP_OPERATOR;

/* Valid Filter Type values (sub-value of Operand) of a RACP packet */
typedef enum {
        GLS_RACP_FILTER_TYPE_RFU = 0x0,
        GLS_RACP_FILTER_TYPE_SN,         /* Sequence number */
        GLS_RACP_FILTER_TYPE_UFT,        /* User facing time (base time + offset time) */
        GLS_RACP_FILTER_TYPE_MAX
} GLS_RACP_FILTER_TYPE;

/* Valid Response Code values of a RACP packet with Response Code opcode value. */
typedef enum {
        GLS_RACP_RESPONSE_RFU = 0x0,
        GLS_RACP_RESPONSE_SUCCESS,               /* Normal response for successful operation. */
        GLS_RACP_RESPONSE_UNSUPPORTED_COMMAND,   /* Normal response if unsupported command/opcode is received. */
        GLS_RACP_RESPONSE_INVALID_OPERATOR,      /* Normal response if operator received does not meet the requirements of the service (e.g. NULL was expected). */
        GLS_RACP_RESPONSE_UNSUPPORTED_OPERATOR,  /* Normal response if unsupported operator is received. */
        GLS_RACP_RESPONSE_INVALID_OPERAND,       /* Normal response if operand received does not meet the requirements of the service (e.g. less operand bytes than expected). */
        GLS_RACP_RESPONSE_NO_RECORDS,            /* Normal response if request to report or delete stored records resulted in no records meeting criteria. */
        GLS_RACP_RESPONSE_ABORT_UNSUCCESSFUL,    /* Normal response if request for Abort cannot be completed. */
        GLS_RACP_RESPONSE_NOT_COMPLETED,         /* Normal response if data transfer should be interrupted for any reason. */
        GLS_RACP_RESPONSE_UNSUPPORTED_OPERAND,   /* Normal response if unsupported Operand is received. */
        GLS_RACP_RESPONSE_MAX
} GLS_RACP_RESPONSE;

/*
 * @brief Get feature callback function signature.
 *
 * This callback will be invoked by the service following read requests for the supported features.
 * This callback is mandatory for the application to register if the feature value can be changed
 * over the lifetime of the sensor (indicated via GLS_FEATURE_INDICATION_PROPERTY). Otherwise, the
 * callback is ignored and read requests are handled transparently by the Glucose service.
 *
 * \param [in]  svc      pointer to the instantiated Glucose server
 * \param [in] conn_idx  connection index associated to the peer that issued the request
 *
 */
typedef void (*gls_get_features_cb_t)(ble_service_t *svc, uint16_t conn_idx);

/*
 * @brief Get number of stored records callback function signature.
 *
 * This callback will be invoked by the service following a write request to RACP with opcode value
 * set to \sa GLS_RACP_COMMAND_NUMBER_OF_RECORDS. Application should provide the server with
 * the requested number of records via \sa gls_indicate_number_of_stored_records. If the request cannot
 * be serviced, for any reason, application should call \sa gls_indicate_status instead indicating
 * the reason of failure.
 *
 * \note This callback is mandatory for the application to register.
 *
 * \param [in] svc           pointer to the instantiated Glucose server
 * \param [in] conn_idx      connection index associated to the peer that issued the request
 * \param [in] record        pointer to record data (should be used to further decode operand
 *                           value based on filter type and operator)
 *
 */
typedef void (*gls_racp_report_num_of_records_cb_t)(ble_service_t *svc, uint16_t conn_idx, gls_racp_t *record);

/*
 * @brief Get stored records callback function signature.
 *
 * This callback will be invoked by the service following a write request to RACP with opcode value
 * set to \sa GLS_RACP_COMMAND_REPORT_RECORDS. Application should provide the server with
 * the requested records via \sa gls_notify_record. Once all records are transfered application should
 * call \sa gls_indicate_report_records_status to indicate the status of request.
 *
 * \note This callback is mandatory for the application to register.
 *
 * \param [in] svc           pointer to the instantiated Glucose server
 * \param [in] conn_idx      connection index associated to the peer that issued the request
 * \param [in] record        pointer to record data (should be used to further decode operand
 *                           value based on filter type and operator)
 *
 */
typedef void (*gls_racp_report_records_cb_t)(ble_service_t *svc, uint16_t conn_idx, gls_racp_t *record);

/*
 * @brief Get stored records callback function signature.
 *
 * This callback will be invoked by the service following a write request to RACP with opcode value
 * set to \sa GLS_RACP_COMMAND_DELETE_RECORDS. Application should permanently delete the records
 * that meet the requested criteria. Once all records are deleted, application should
 * call \sa gls_indicate_delete_records_status to indicate the status of request.
 *
 * \note This callback is mandatory for the application to register if
 *       \sa GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT is set.
 *
 * \param [in] svc           pointer to the instantiated Glucose server
 * \param [in] conn_idx      connection index associated to the peer that issued the request
 * \param [in] record        pointer to record data (should be used to further decode operand
 *                           value based on filter type and operator)
 *
 */
typedef void (*gls_racp_delete_records_cb_t)(ble_service_t *svc, uint16_t conn_idx, gls_racp_t *record);

/*
 * @brief Abort operation callback function signature
 *
 * This callback will be invoked by the service following a write request to RACP with opcode value
 * set to \sa GLS_RACP_COMMAND_ABORT_OPERATION. Application shall make a best effort to stop sending
 * any further data. Once all RACP procedures have been stopped, \sa gls_indicate_abort_operation_status
 * should be called to indicate the success of failure of the requested operation.
 *
 * \param [in] svc           pointer to the instantiated Glucose server
 * \param [in] conn_idx      connection index associated to the peer that issued the request
 *
 * \note This callback is mandatory for the application to register.
 *
 */
typedef void (*gls_racp_abort_operation_cb_t)(ble_service_t *svc, uint16_t conn_idx);

/*
 * @brief CCC descriptor update callback function signature.
 *
 * This callback will be invoked by the service following a write request to a valid CCC descriptor
 * of type "notifications" and given that the written value indicates that notifications are enabled.
 * As per GLS specifications, if CCC descriptor has been configured to enable notifications and
 * the peer collector does not perform a RACP procedure, then after waiting for an idle time of 5'',
 * the sensor device should perform the GAP Terminate Connection procedure. So, once this callback is
 * triggered an OS timer can be started with timeout of 5'' so that if the timer expires, reaches zero
 * value, the currently established connection is terminated.
 *
 * \note This callback is optional for the application to register.
 */
typedef void (*gls_ccc_enabled_cb_t)(ble_service_t *svc, const ble_evt_gatts_write_req_t *evt);


/*
 * @brief Send notifications/indications callback function signature
 *
 * This callback will be invoked by the service following notifications/indications to peer devices.
 * This callback is optional and could be used by application to make sure that previously sent
 * events were executed successfully or not.
 *
 * \param [in] svc  pointer to the instantiated Glucose server
 * \param [in] evt  pointer to the event as reported by the server
 *
 */
typedef void (*gls_event_sent_cb_t)(ble_service_t *svc, const ble_evt_gatts_event_sent_t *evt);

/* Structure holding user-defined callback functions to be called following RACP write requests. */
typedef struct {
        gls_racp_report_num_of_records_cb_t report_num_of_records;
        gls_racp_report_records_cb_t report_records;
#if GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT
        gls_racp_delete_records_cb_t delete_records;
#endif
        gls_racp_abort_operation_cb_t abort_operation;
} racp_callbacks_t;

/* Structure holding all the user-defined callback functions required by Glucose Service. */
typedef struct {
        /*
         * If glucose sensor's capabilities can be changed runtime it's the application
         * that should service Feature read requests. Otherwise, the Glucose service
         * should automatically handle read requests.
         */
#if GLS_FEATURE_INDICATION_PROPERTY
        gls_get_features_cb_t get_features;
#endif
        racp_callbacks_t racp_callbacks;
        gls_event_sent_cb_t event_sent;
        gls_ccc_enabled_cb_t ccc_enabled;
} gls_callbacks_t;

/*
 * Glucose Service database instantiation. In theory, this API can be called multiple times, but typically
 * only one service instantiation is required for a sensor application.
 *
 * \param [in] cb  pointer to user callback functions to be called upon specific events. Keep in mind that
 *                 some callback functions are mandatory. If any mandatory callback function is missed
 *                 then an assertion is expected to be thrown.
 *
 * \return Pointer to the service structure instantiation.
 *
 */
ble_service_t *gls_init(const gls_callbacks_t *cb);

#if GLS_FEATURE_INDICATION_PROPERTY
/*
 * Function to be called by application following Feature characteristic read requests.
 * If a feature read request has been raised, the registered \sa get_features callback function
 * will be invoked so application can provide the server with the requested data using that API.
 *
 * \param [in] svc        pointer to the instantiated Glucose server.
 * \param [in] conn_idx   connection index that reflects the peer device that raised the read request.
 *                        The value should be provided by \sa get_features callback function.
 * \param [in] status     attribute error code indicating the validity of the data provided
 * \param [in] feature    data to be provided as response to the read request
 *
 * \note This API is available only if the feature value can be changed over the lifetime of the platform
 *       \sa GLS_FEATURE_INDICATION_PROPERTY.
 *
 * \note Returning the feature value should not take much time and so this API is typically called
 *       within \sa get_features. That is, there is no reason to defer is handling at a later point
 *       using notifications to the application task.
 */
void gls_get_feature_cfm(ble_service_t *svc, uint16_t conn_idx, att_error_t status, uint16_t feature);

/*
 * Function to be called by application to indicate that sensor's capabilities have been changed
 * since the last connection. The function will first check is the CCC descriptor of the associated
 * peer device is enabled and if the selected peer device (connection index) is bonded.
 * If either of the mentioned criteria are not met, the function will return immediately. This function
 * can be called at any point in time.
 *
 * \param [in] svc        pointer to the instantiated Glucose server
 * \param [in] conn_idx   connection index that reflects the peer device to whom indications will be sent to
 * \param [in] feature    value to be indicated through the Feature characteristic handle
 *
 * \note This API is available only if the feature value can be changed over the lifetime of the platform
 *       \sa GLS_FEATURE_INDICATION_PROPERTY.
 */
void gls_indicate_feature(ble_service_t *svc, uint16_t conn_idx, uint16_t feature);

/*
 * Helper function to send feature indications to all connected devices. See description of
 * \sa gls_indicate_feature.
 *
 * \param [in] svc        pointer to the instantiated Glucose server
 * \param [in] feature    value to be indicated to the selected peer device
 *
 * \note This API is available only if the feature value can be changed over the lifetime of the platform
 *       \sa GLS_FEATURE_INDICATION_PROPERTY.
 */
void gls_indicate_feature_all(ble_service_t *svc, uint16_t feature);
#endif /* GLS_FEATURE_INDICATION_PROPERTY */

/*
 * Function to be called by application following "get number of stored records" RACP write requests.
 * If a relevant request has been raised, the registered \sa report_num_of_records callback function
 * will be invoked so application can provide the server with the requested data using that API.
 * A zero value should be provided if no data are found based to meet the requested criteria.
 *
 * \param [in] svc          pointer to the instantiated Glucose server
 * \param [in] conn_idx     connection index that reflects the peer device that raised the read request.
 *                          The value should be provided by \sa set_racp callback
 * \param [in] num_records  data to be provided as response to the request
 */
void gls_indicate_number_of_stored_records(ble_service_t *svc, uint16_t conn_idx, uint16_t num_records);

/*
 * General function to be called by application to indicate the success of failure of a request.
 * Although this function can be called directly, it's preferably to use the corresponding helper APIs:
 *
 * \sa gls_indicate_abort_operation_status
 * \sa gls_indicate_delete_records_status
 * \sa gls_indicate_report_records_status
 *
 * This API should be called at the end of the request handling.
 *
 *
 * \param [in] svc          pointer to the instantiated Glucose server
 * \param [in] conn_idx     connection index that reflects the peer device that raised the read request.
 *                          The value should be provided by the register callback associated to the request
 * \param [in] command      the opcode/command to which the status corresponds to.
 *                          Valid values can be selected from \sa GLS_RACP_COMMAD and should reflect the
 *                          request value that is currently serviced
 * \param [in] status       The status of the request. Valid values can be selected from \sa GLS_RACP_RESPONSE.
 */
void gls_indicate_status(ble_service_t *svc, uint16_t conn_idx, uint8_t command, uint8_t status);

/*
 * Helper function to be called by application to indicate the success or failure of a previously raised
 * abort request. This API should be called at the end of the request handling.
 *
 * \param [in] svc          pointer to the instantiated Glucose server
 * \param [in] conn_idx     connection index that reflects the peer device that raised the read request.
 *                          The value should be provided by \sa abort_operation callback function
 * \param [in] status       The status of the abort request. According to GLS specifications valid values
 *                          are:
 *                             - GLS_RACP_RESPONSE_SUCCESS
 *                             - GLS_RACP_RESPONSE_ABORT_UNSUCCESSFUL
 */
void gls_indicate_abort_operation_status(ble_service_t *svc, uint16_t conn_idx, uint8_t status);

#if GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT
/*
 * Helper function to be called by application to indicate the success or failure of a previously raised
 * "delete stored records" RACP write request. This API should be called at the end of the request handling.
 *
 * \param [in] svc          pointer to the instantiated Glucose server
 * \param [in] conn_idx     connection index that reflects the peer device that raised the read request.
 *                          The value should be provided by \sa delete_records callback function
 * \param [in] status       The status of the deletion request. According to GLS specifications valid values
 *                          are:
 *                             - GLS_RACP_RESPONSE_SUCCESS
 *                             - GLS_RACP_RESPONSE_NO_RECORDS
 */
void gls_indicate_delete_records_status(ble_service_t *svc, uint16_t conn_idx, uint8_t status);
#endif

/*
 * Helper function to be called by application to indicate the success or failure of a previously raised
 * "report stored records" RACP write request. This API should be called at the end of the request handling.
 *
 * \param [in] svc          pointer to the instantiated Glucose server
 * \param [in] conn_idx     connection index that reflects the peer device that raised the read request.
 *                          The value should be provided by \sa report_records callback function
 * \param [in] status       The status of the report request. According to GLS specifications valid values
 *                          are:
 *                             - GLS_RACP_RESPONSE_SUCCESS
 *                             - GLS_RACP_RESPONSE_NO_RECORDS
 *                             - GLS_RACP_RESPONSE_NOT_COMPLETED
 */
void gls_indicate_report_records_status(ble_service_t *svc, uint16_t conn_idx, uint8_t status);

/*
 * Function to be called by application following "get stored records" RACP write requests.
 * If a request has been raised, the registered \sa report_records callback function will be invoked
 * so application can provide the server with the requested data using that API. It is expected that
 * the number of function calls should match the number of records that meet the requested criteria.
 * The function will first check is the CCC descriptor of the associated peer device is enabled and
 * if the selected peer device (connection index) is bonded. If either of the mentioned criteria are
 * not met, the function will return immediately. The function will first notify the RACP characteristic
 * value and if context information is enabled then a notification to the Glucose Measurement Context
 * characteristic value will follow.
 * This API should also be used if new records are available since the last connection with a bonded
 * peer device (collector). In the latter case it is handier for the application to call
 * \sa gls_notify_record_all instead.
 *
 * \param [in] svc          pointer to the instantiated Glucose server
 * \param [in] conn_idx     connection index that reflects the peer device that raised the read request.
 * \param [in] record       data/record to be provided as response to the request
 *
 * \return True if notifications was sent over the air, false otherwise.
 *
 * \note The application should retry calling this API until true is returned as it might happen that
 *       Bluetooth server is busy handling other requests.
 *
 */
bool gls_notify_record(ble_service_t *svc, uint16_t conn_idx, gls_record_t *record);

/*
 * Helper function to send record notifications to all bonded devices. See description of
 * \sa gls_notify_record.
 *
 * \param [in] svc          pointer to the instantiated Glucose server
 * \param [in] record       data/record to be provided as response to the request
 *
 */
void gls_notify_record_all(ble_service_t *svc, gls_record_t *record);

#endif /* GLUCOSE_SERVICE_H_ */
