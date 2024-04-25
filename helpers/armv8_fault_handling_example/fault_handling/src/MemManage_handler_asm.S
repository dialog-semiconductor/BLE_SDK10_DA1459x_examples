/*
 * Copyright (C) 2019-2023 Renesas Electronics Corporation and/or its affiliates.
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
 */

 		.syntax unified
 		.arch armv8-m.main

/*
 * User-defined UsageFault handler. Programmers can define their own handlers
 * and override the SKD ones on the premise that existing handlers are marked
 * as "weak".
 * This is the 1st part of the handler written in pure assembly. Here we locate
 * the position of the stacked frame saved on exception entrance and we check
 * if FP context has been stacked. That is when a fault exception takes place mid
 * FPU instruction execution.
 *
 * Since the state of the system is unknown after a fault, an attempt to access
 * a peripheral device (e.g. FLASH) could lead to failure or even to system collapse.
 * Hence, it's a good practice to place the handlers in SYSRAM.
 * Moreover, accessing code from SRAM is much faster compared to a non volatile device
 * (e.g. FLASH or EEPROM) and quite often is critical for the system to react in a
 * fault event as fast as possible.
 *
 */
#if (dg_configCODE_LOCATION == NON_VOLATILE_IS_NONE) || (dg_configREDUCE_RETAINED_CODE)
		.section .text
#else
		.section text_retained
#endif

		.thumb
		.thumb_func
		.align 2
		.globl MemManage_Handler
		.type  MemManage_Handler, %function
MemManage_Handler:

		/* Check for the location of the stacked frame. LR should contain the EXC_RETURN special value. */
		tst lr, #4
		ite eq
		/* If EXC_RETURN[2] == 0, stacked frame is stored in MSP. */
		mrseq r0, msp
		/* If EXC_RETURN[2] == 1, stacked frame is stored in PSP. */
		mrsne r0, psp

		/* Check if the stacked frame contains FP context. */
		tst lr, #16
		ite eq
		/* If EXC_RETURN[4] == 0, FP context is present (extended stack frame). */
		moveq r1, #1
		/* If EXC_RETURN[4] == 1, FP context is not present (integer only stack frame). */
		movne r1, #0

		/* Branch to C fault handler */
		ldr r2, =MemManage_HandlerC
		bx r2

		.size MemManage_Handler, . - MemManage_Handler
