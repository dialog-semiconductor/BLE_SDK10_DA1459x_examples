/**
 ****************************************************************************************
 *
 * @file glucose_service_default.h
 *
 * @brief Glucose Service default configurations
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

#ifndef GLUCOSE_SERVICE_DEFAULT_H_
#define GLUCOSE_SERVICE_DEFAULT_H_

#include "glucose_service_config.h"

/*
 * If set, the Time Offset field should be present in glucose measurement values. As per GLS
 * specifications, this value should be present when it's changed from the last reported measurement.
 * Otherwise, it may be present if even it has not been changed from record to record. By setting
 * this macro time offset will always be present in glucose measurements.
 *
 * \note Currently this macro does not have any effect as time offset is always present in records.
 *       This value should not be changed!
 */
#define GLS_FLAGS_TIME_OFFSET           ( 1 )

/*
 * If set, the Glucose Concentration field along with the Type-Sample Location field should be present
 * in glucose measurements.
 */
#ifndef GLS_FLAGS_CONCENTRATION_TYPE_SAMPLE_LOCATION
#define GLS_FLAGS_CONCENTRATION_TYPE_SAMPLE_LOCATION     ( 0 )
#endif

/* If set, glucose measurements are in base units of mol/L (typically displayed in units of mmol/L).
 * Otherwise, glucose measurements are in base units of kg/L (typically displayed in units of mg/dL).
 */
#ifndef GLS_FLAGS_CONCENTRATION_UNITS
#define GLS_FLAGS_CONCENTRATION_UNITS    ( 0 )
#endif

/*
 * If set, the Status Annunciation field should be present in glucose measurement values.
 * This field is optional and depends on the capabilities of the target sensor.
 */
#ifndef GLS_FLAGS_STATUS_ANNUNCIATION
#define GLS_FLAGS_STATUS_ANNUNCIATION    ( 0 )
#endif

/*
 * If set, the context information characteristic is enabled along with its descriptor.
 * If present, a (patient) record comprises of a glucose measurement value followed by a
 * glucose measurement context value. As per GLS specifications contextual info
 * is optional.
 */
#ifndef GLS_FLAGS_CONTEXT_INFORMATION
#define GLS_FLAGS_CONTEXT_INFORMATION    ( 0 )
#endif

/* Helper macro to construct the Flags field of Glucose Measurement characteristic value. */
#define GLS_FLAGS_FIELD_VALUE \
        ((GLS_FLAGS_TIME_OFFSET << 0)                        | \
         (GLS_FLAGS_CONCENTRATION_TYPE_SAMPLE_LOCATION << 1) | \
         (GLS_FLAGS_CONCENTRATION_UNITS << 2)                | \
         (GLS_FLAGS_STATUS_ANNUNCIATION << 3)                | \
         (GLS_FLAGS_CONTEXT_INFORMATION << 4))                 \


/* Low Battery Detection During Measurement Supported */
#ifndef GLS_FEATURE_DEVICE_LOW_BATTERY
#define GLS_FEATURE_DEVICE_LOW_BATTERY        ( 0 )
#endif

/*
 * Sensor Malfunction Detection Supported. If set, the the corresponding
 * bit in the status annunciation field should be valid. Otherwise, it
 * should be ignored.
 */
#ifndef GLS_FEATURE_SENSOR_MALFUNCTION
#define GLS_FEATURE_SENSOR_MALFUNCTION        ( 0 )
#endif

/*
 * Sensor Sample Size Supported. If set, the the corresponding
 * bit in the status annunciation field should be valid.
 * Otherwise, it should be ignored.
 */
#ifndef GLS_FEATURE_SAMPLE_SIZE
#define GLS_FEATURE_SAMPLE_SIZE               ( 0 )
#endif

/*
 * Sensor Strip Insertion Error Detection Supported. If set, the the corresponding
 * bit in the status annunciation field should be valid.
 * Otherwise, it should be ignored.
 */
#ifndef GLS_FEATURE_STRIP_INSERTION_ERROR
#define GLS_FEATURE_STRIP_INSERTION_ERROR     ( 0 )
#endif

/*
 * Sensor Strip Type Error Detection Supported. If set, the the corresponding
 * bit in the status annunciation field should be valid. Otherwise, it
 * should be ignored.
 */
#ifndef GLS_FEATURE_STRIP_TYPE_ERROR
#define GLS_FEATURE_STRIP_TYPE_ERROR          ( 0 )
#endif

/*
 * Sensor Result High-Low Detection Supported. If set, the the corresponding
 * bit in the status annunciation field should be valid. Otherwise, it
 * should be ignored.
 */
#ifndef GLS_FEATURE_RESULT_HIGH_LOW
#define GLS_FEATURE_RESULT_HIGH_LOW           ( 0 )
#endif

/*
 * Sensor Temperature High-Low Detection Supported. If set, the the corresponding
 * bit in the status annunciation field should be valid. Otherwise, it
 * should be ignored.
 */
#ifndef GLS_FEATURE_TEMPERATURE_HIGH_LOW
#define GLS_FEATURE_TEMPERATURE_HIGH_LOW      ( 0 )
#endif

/*
 * Sensor Read Interrupt Detection Supported. If set, the the corresponding
 * bit in the status annunciation field should be valid. Otherwise, it
 * should be ignored.
 */
#ifndef GLS_FEATURE_READ_INTERRUPT
#define GLS_FEATURE_READ_INTERRUPT            ( 0 )
#endif

/*
 * General Device Fault Supported. If set, the the corresponding bit in the status
 * annunciation field should be valid. Otherwise, it should be ignored.
 */
#ifndef GLS_FEATURE_GENERAL_DEVICE_ERROR
#define GLS_FEATURE_GENERAL_DEVICE_ERROR      ( 0 )
#endif

/*
 * Time Fault Supported. If set, the the corresponding bit in the status annunciation
 * field should be valid. Otherwise, it should be ignored.
 */
#ifndef GLS_FEATURE_TIME_ERROR
#define GLS_FEATURE_TIME_ERROR                ( 0 )
#endif

/*
 * Multiple Bond Supported. As per GLP specifications this field is required so a collector
 * can determine the recipient of sensor's advertising data. This info can also be used as a
 * device can for instance delete some records that another device would otherwise not delete.
 */
#ifndef GLS_FEATURE_MULTIPLE_BOND
#define GLS_FEATURE_MULTIPLE_BOND             ( 0 )
#endif

/* Helper macro to construct the Feature characteristic value. */
#define GLS_FEATURE_CHARACTERISTIC_VALUE \
        ((GLS_FEATURE_DEVICE_LOW_BATTERY << 0)    |\
         (GLS_FEATURE_SENSOR_MALFUNCTION << 1)    |\
         (GLS_FEATURE_SAMPLE_SIZE << 2)           |\
         (GLS_FEATURE_STRIP_INSERTION_ERROR << 3) |\
         (GLS_FEATURE_STRIP_TYPE_ERROR << 4)      |\
         (GLS_FEATURE_RESULT_HIGH_LOW << 5)       |\
         (GLS_FEATURE_TEMPERATURE_HIGH_LOW << 6)  |\
         (GLS_FEATURE_READ_INTERRUPT << 7)        |\
         (GLS_FEATURE_GENERAL_DEVICE_ERROR << 8)  |\
         (GLS_FEATURE_TIME_ERROR << 9)            |\
         (GLS_FEATURE_MULTIPLE_BOND << 10))

/*
 * If set, the indication property will be added to the glucose Feature characteristic
 * so that its value can be changed at runtime. If set, users should register a callback
 * function, \sa get_features,  to provide the supported capabilities of the device when
 * a Feature read request is received.
 */
#ifndef GLS_FEATURE_INDICATION_PROPERTY
#define GLS_FEATURE_INDICATION_PROPERTY               ( 0 )
#endif

/*
 * If set, Carbohydrate ID And Carbohydrate fields should be present in
 * Glucose Measurement Context characteristic value.
 */
#ifndef GLS_CONTEXT_FLAGS_CARBOHYDRATE_ID
#define GLS_CONTEXT_FLAGS_CARBOHYDRATE_ID             ( 0 )
#endif

/* If set, Meal field should be present in Glucose Measurement Context characteristic value. */
#ifndef GLS_CONTEXT_FLAGS_MEAL
#define GLS_CONTEXT_FLAGS_MEAL                         ( 0 )
#endif

/* If set, Tester and Health nibbles should be present in Glucose Measurement Context characteristic value. */
#ifndef GLS_CONTEXT_FLAGS_TESTER_HEALTH
#define GLS_CONTEXT_FLAGS_TESTER_HEALTH                ( 0 )
#endif

/* If set, Exercise Duration and Exercise Intensity fields should be present in
 * Glucose Measurement Context characteristic value.
 */
#ifndef GLS_CONTEXT_FLAGS_EXERCISE_DURATION_INTENSITY
#define GLS_CONTEXT_FLAGS_EXERCISE_DURATION_INTENSITY  ( 0 )
#endif

/*
 * If set, Medication ID and Medication fields should be present in
 * Glucose Measurement Context characteristic value.
 */
#ifndef GLS_CONTEXT_FLAGS_MEDICATION_ID
#define GLS_CONTEXT_FLAGS_MEDICATION_ID                ( 0 )
#endif

/*
 * If set, medication value is in units of liters. Otherwise, medication value
 * is in units of kilograms.
 */
#ifndef GLS_CONTEXT_FLAGS_MEDICATION_UNITS
#define GLS_CONTEXT_FLAGS_MEDICATION_UNITS             ( 0 )
#endif

/* If set, HbA1c field should be present in Glucose Measurement Context characteristic value. */
#ifndef GLS_CONTEXT_FLAGS_HbA1c
#define GLS_CONTEXT_FLAGS_HbA1c                        ( 0 )
#endif

/* If set, Extended Flags field should be present in Glucose Measurement Context characteristic value. */
#ifndef GLS_CONTEXT_FLAGS_EXTENDED_FLAGS
#define GLS_CONTEXT_FLAGS_EXTENDED_FLAGS               ( 0 )
#endif

/* Helper macro to construct the Flags field of Glucose Measurement Context characteristic value. */
#define GLS_CONTEXT_FLAGS_FIELD_VALUE \
        ((GLS_CONTEXT_FLAGS_CARBOHYDRATE_ID << 0)       |\
         (GLS_CONTEXT_FLAGS_MEAL << 1)                  |\
         (GLS_CONTEXT_FLAGS_TESTER_HEALTH << 2)         |\
         (GLS_CONTEXT_FLAGS_EXERCISE_DURATION_INTENSITY << 3) |\
         (GLS_CONTEXT_FLAGS_MEDICATION_ID << 4)               |\
         (GLS_CONTEXT_FLAGS_MEDICATION_UNITS << 5)            |\
         (GLS_CONTEXT_FLAGS_HbA1c << 6)                       |\
         (GLS_CONTEXT_FLAGS_EXTENDED_FLAGS << 7))

/*
 * As per GLS specifications the following should be taken into consideration:
 *
 * 1. Support for a given Operand for one opcode and Operator combination does not imply support
 *    of that Operand for other opcode and Operator combinations.
 *
 * 2. Support for a given Operator for one opcode does not imply support of that Operator for other
 *    opcodes.
 *
 * However, in this implementation, the following macros will globally enable/disable support for a
 * specific opcode, operand or operator. If the application database cannot support this scheme
 * then an error code should be returned as status code immediately after a registered callback
 * function is invoked following a RACP write request.
 */


/* If set, requests (opcodes) to delete stored records should be supported by the application database. */
#ifndef GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT
#define GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT       ( 0 )
#endif

/*
 * If set, 'less than or equal to' operators should be supported by the application
 * database. As per GLS specifications this operator is optional.
 */
#ifndef GLS_RACP_OPERATOR_LESS_EQUAL_SUPPORT
#define GLS_RACP_OPERATOR_LESS_EQUAL_SUPPORT                 ( 0 )
#endif

/*
 * If set, requests to delete the first (very first) record should be supported by the application
 * database. As per GLS specifications this operator is optional.
 */
#ifndef GLS_RACP_OPERATOR_FIRST_RECORD_SUPPORT
#define GLS_RACP_OPERATOR_FIRST_RECORD_SUPPORT               ( 0 )
#endif

/*
 * If set, requests to delete the last (most recent) record should be supported by the application
 * database. As per GLS specifications this operator is optional.
 */
#ifndef GLS_RACP_OPERATOR_LAST_RECORD_SUPPORT
#define GLS_RACP_OPERATOR_LAST_RECORD_SUPPORT                ( 0 )
#endif

/*
 * If set, 'within range of (inclusive)' operators should be supported by the application
 * database. As per GLS specifications this operator is optional.
 */
#ifndef GLS_RACP_OPERATOR_WITHIN_RANGE_SUPPORT
#define GLS_RACP_OPERATOR_WITHIN_RANGE_SUPPORT               ( 0 )
#endif

/*
 * If set, user facing type filtering (part of operand) should be supported by the application
 * database. As per GLS specifications filter type is optional.
 *
 * \warning The current database framework does not support UFT. If enabled, and without
 *          modifying the database source code, an assertion will be raised at build time.
 */
#ifndef GLS_RACP_FILTER_USER_FACING_TIME_SUPPORT
#define GLS_RACP_FILTER_USER_FACING_TIME_SUPPORT             ( 0 )
#endif

#endif /* GLUCOSE_SERVICE_DEFAULT_H_ */
