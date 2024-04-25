/**
 ****************************************************************************************
 *
 * @file fault_handling.c
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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "osal.h"
#include "fault_handling.h"
#include "misc.h"


#define PRINT_NULL                 DBG_PRINTF("\n\r")
#define PRINT_HEADER               DBG_PRINTF("\n\rBit Position |   Bit-Field Name |  Description\n\n\r")

/*
 * Structure used for storing the CPU status following a fault event. User can define
 * their own retained memory location in SRAM by changing the linker configurations
 * accordingly.
 */
volatile Fault_t _fault_exception_info __attribute__((section("hard_fault_info")));

static const char* armv8_irqn_def[] = { ARMv8_IRQn_DEFINITION };

static const char* mmfsr_bit_field_def[] = { MMFSR_BIT_FIELD_DESCRITION };

static const char* bfsr_bit_field_def[] = { BFSR_BIT_FIELD_DESCRIPTION };

static const char* ufsr_bit_field_def[] = { UFSR_BIT_FIELD_DESCRIPTION };


/* Flag used for triggering escalated fault exceptions */
__RETAINED_RW volatile bool fault_escalation_flag = false;

static void _analyze_xFSR(uint32_t reg_val, uint8_t reg_size_in_bits, const char * report[])
{
        ASSERT_WARNING(reg_size_in_bits <= 31);

        bool flag = false;
        /*
         * Check each bit of the status register individually starting from LSB.
         * If set, then print the corresponding description.
         */
        for (size_t i = 0; i < reg_size_in_bits; i++) {
               if (reg_val & (uint32_t)(1 << i)) {
                       DBG_PRINTF("%s\n\r", report[i]);
                       flag = true;
               }
        }

        if (flag) {
                PRINT_NULL;
                PRINT_HEADER;
        }
}

/* Analyze the contents of the Configurable Fault Status Register */
static void _analyze_CFSR(void)
{
        /* Analyze the CFSR */
        uint8_t memmanage_fault_status = _FLD2VAL(SCB_CFSR_MEMFAULTSR, _fault_exception_info._CFSR._CFSR);
        uint8_t bus_fault_status       = _FLD2VAL(SCB_CFSR_BUSFAULTSR, _fault_exception_info._CFSR._CFSR);
        uint16_t usage_fault_status    = _FLD2VAL(SCB_CFSR_USGFAULTSR, _fault_exception_info._CFSR._CFSR);

        PRINT_NULL;
        PRINT_HEADER;
        _analyze_xFSR((uint32_t)memmanage_fault_status,
                      sizeof(memmanage_fault_status) == 1 ? 8 : sizeof(memmanage_fault_status) == 2 ? 16 : 32,
                      mmfsr_bit_field_def);
        _analyze_xFSR((uint32_t)bus_fault_status,
                      sizeof(bus_fault_status) == 1 ? 8 : sizeof(bus_fault_status) == 2 ? 16 : 32,
                      bfsr_bit_field_def );
        _analyze_xFSR((uint32_t)usage_fault_status,
                      sizeof(usage_fault_status) == 1 ? 8 : sizeof(usage_fault_status) == 2 ? 16 : 32,
                      ufsr_bit_field_def);
}


/*
 * Given the acquired CPU status after a fault triggering, this routine performs a
 * basic analysis of the causes of a fault exception.
 */
static void _analyze_fault_exception(void)
{

        DBG_PRINTF("\n\n\r---- Extensive Fault Exception Analysis ----\n\r");

        DBG_PRINTF("\n\r---- Stacked Core Registers ----\n\n\r");
        DBG_PRINTF("  - R0    = 0x%08lx\r\n", _fault_exception_info._r0  );
        DBG_PRINTF("  - R1    = 0x%08lx\r\n", _fault_exception_info._r1  );
        DBG_PRINTF("  - R2    = 0x%08lx\r\n", _fault_exception_info._r2  );
        DBG_PRINTF("  - R3    = 0x%08lx\r\n", _fault_exception_info._r3  );

        DBG_PRINTF("  - R12   = 0x%08lx\r\n", _fault_exception_info._r12 );

        DBG_PRINTF("  - LR    = 0x%08lx\r\n", _fault_exception_info._lr  );
        DBG_PRINTF("  - PC    = 0x%08lx\r\n", _fault_exception_info._pc  );
        DBG_PRINTF("  - xPSR  = 0x%08lx\r\n", _fault_exception_info._xPSR);

        /* Check if FP context was stacked during on exception entrance */
        if (_fault_exception_info.is_fp_valid) {
                for (int i = 0; i < STACKED_FRAME_FP_MAX_ENTRIES; i++) {
                        DBG_PRINTF("  - S[%d]  = 0x%08lx\n\r", i, *((uint32_t *)(&_fault_exception_info._s0 + (i * 4))));
                }
                DBG_PRINTF("  - FPSCR = 0x%08lx\r\n", _fault_exception_info._FPSCR);
        }
        else {
                DBG_PRINTF("\n\n\r**** Floating Point (FP) context is not present in the Stack Frame ****\n\r");
        }

        DBG_PRINTF("\n\n\r---- Fault Status Related Registers ----\n\n\r");
        DBG_PRINTF("  - CFSR  = 0x%08lx\n\r", _fault_exception_info._CFSR._CFSR );
        DBG_PRINTF("  - HFSR  = 0x%08lx\n\r", _fault_exception_info._HFSR       );
        DBG_PRINTF("  - DFSR  = 0x%08lx\n\r", _fault_exception_info._DFSR       );
        DBG_PRINTF("  - AFSR  = 0x%08lx\n\r", _fault_exception_info._AFSR       );

        DBG_PRINTF("\n\n\r---- Fault Address Related Registers ----\n\n\r");
        DBG_PRINTF("  - MMFAR = 0x%08lx\n\r", _fault_exception_info._MMFAR      );
        DBG_PRINTF("  - BFAR  = 0x%08lx\n\r", _fault_exception_info._BFAR       );

        if (_FLD2VAL(SCB_HFSR_FORCED, _fault_exception_info._HFSR)) {
                DBG_PRINTF("\n\n\r***** A forced HardFault took place - Escalated by a configurable fault ******\r\n");
        }
        if (_FLD2VAL(SCB_HFSR_VECTTBL, _fault_exception_info._HFSR)) {
                DBG_PRINTF("\n\n\r***** HardFault on a vector table read during exception processing - The stacked PC value points to the instruction that was preempted by the exception ******\r\n");
        }

        DBG_PRINTF("\n\n\r******* Exception Type: %s *******\n\n\r", armv8_irqn_def[_fault_exception_info.irq_num]);

        if (_fault_exception_info.irq_num == USAGEFAULT_EXCEPTION_TYPE &&
                                _fault_exception_info.stacklimit_context != STACK_OVERFLOW_IS_NONE) {
                DBG_PRINTF("Stack overflow triggered due to %s overflow\n\r",
                                _fault_exception_info.stacklimit_context == STACK_OVERFLOW_IS_MSP ? "MSP" : "PSP");
        }

        /* Analyze the contents of the shadowed Configurable Fault Status Register  */
        _analyze_CFSR();
}


/* Check whether the system booted normally or due to a fault exception */
void system_boot_status_check_and_analyze(void)
{
        if (_fault_exception_info.magic_val == FAULT_HANDLING_MAGIC_VALUE) {
                DBG_PRINTF("\n\n\r******** System Rebooted Due to Fault Exception *******\n\r");
                /* Perform a basic analysis of the fault exception */
                _analyze_fault_exception();

                /* After fault analysis, invalidate the magic value */
                _fault_exception_info.magic_val = 0x00000000;
        }
        else {
                DBG_PRINTF("\n\n\r********** System Booted Normally! ***********\n\r");
        }
}

/* Enable/disable escalated fault exceptions */
void fault_escalation_status_set(bool status)
{
        fault_escalation_flag = status;
}
