/**
 ****************************************************************************************
 *
 * @file glucose_service_config.h
 *
 * @brief Glucose service application configurations
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

#ifndef GLUCOSE_SERVICE_CONFIG_H_
#define GLUCOSE_SERVICE_CONFIG_H_

/* Arbitrary application configurations settings. Normally, the following settings
 * should be adjusted based on application needs/capabilities. */

#define GLS_FLAGS_CONCENTRATION_TYPE_SAMPLE_LOCATION     ( 1 )
#define GLS_FLAGS_STATUS_ANNUNCIATION                    ( 1 )
#define GLS_FLAGS_CONTEXT_INFORMATION                    ( 1 )

#define GLS_FEATURE_INDICATION_PROPERTY                  ( 1 )
#define GLS_FEATURE_DEVICE_LOW_BATTERY                   ( 1 )
#define GLS_FEATURE_MULTIPLE_BOND                        ( 1 )

#define GLS_CONTEXT_FLAGS_CARBOHYDRATE_ID                ( 1 )

#define GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT   ( 1 )

#define GLS_RACP_OPERATOR_FIRST_RECORD_SUPPORT           ( 1 )
#define GLS_RACP_OPERATOR_LAST_RECORD_SUPPORT            ( 1 )

#define APP_DB_MAX_LIST_LEN                              50

#endif /* GLUCOSE_SERVICE_CONFIG_H_ */
