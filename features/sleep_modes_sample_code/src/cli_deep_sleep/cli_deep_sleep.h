/**
 ****************************************************************************************
 *
 * @file cli_deep_sleep.h
 *
 * @brief Deep sleep shell operations
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

#ifndef CLI_DEEP_SLEEP_H_
#define CLI_DEEP_SLEEP_H_

void cli_deep_sleep_set_status_handler(int argc, const char *argv[], void *user_data);
void cli_deep_sleep_select_port_handler(int argc, const char *argv[], void *user_data);
void cli_deep_sleep_reset_port_handler(int argc, const char *argv[], void *user_data);
void cli_deep_sleep_get_info_handler(int argc, const char *argv[], void *user_data);

/*
 * Get deep sleep status
 *
 * \return True is deep sleep is selected by users, false otherwise.
 *
 * \note The deep sleep mode should be enabled explicitly by application
 *       and not by the handler once users requested to do so.
 *       The reason for this is that, a sleeping mode of higher priority
 *       (typically the active or idle state) might be requested at that time
 *       which should prevent the device from entering the deep sleep mode.
 *       Application should make sure that the device is ready to enter the
 *       deep sleep mode.
 */
bool cli_deep_sleep_get_status(void);

#endif /* CLI_DEEP_SLEEP_H_ */
