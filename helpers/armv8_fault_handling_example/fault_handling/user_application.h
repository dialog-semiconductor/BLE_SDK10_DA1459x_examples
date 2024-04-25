/**
 ****************************************************************************************
 *
 * @file user_application.h
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

#ifndef USER_APPLICATION_H_
#define USER_APPLICATION_H_

#include <stdbool.h>

/*
 * There are flavors for handling fault exceptions:
 *
 * 1 --> CPU execution is trapped and/or handled waiting for the user to attach a debugger
 *       and perform a debugging session.
 *
 * 2 --> System's state is stored in a dedicated area in SRAM and a HW reset is triggered
 *       for the system to recover. Upon next boot the stored info is exercised and various
 *       debug info is printed on the console (if enabled). Please note that this mode is
 *       invalid when images are built for RAM execution. This is because RAM images require
 *       that debugger be attached and so rebooting the device is not allowed.
 *
 **/
#ifndef FAULT_HANDLING_MODE
#define FAULT_HANDLING_MODE           ( 1 )
#endif

/*
 * It should be called at the beginning of the application (e.g. inside application tasks)
 * to check if system booted normally or because of a fault exception.
 */
void system_boot_status_check_and_analyze(void);

/* If set an escalated fault will be triggered on purposed inside BusFault handler to demonstrate
 * the ARM exception escalation mechanism. */
void fault_escalation_status_set(bool status);

#endif /* USER_APPLICATION_H_ */
