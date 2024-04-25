/**
 ****************************************************************************************
 *
 * @file fault_handling.h
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

#ifndef FAULT_HANDLING_H_
#define FAULT_HANDLING_H_

#include <stdbool.h>
#include "osal.h"
#include "hw_gpio.h"
#include "user_application.h"

#if (FAULT_HANDLING_MODE < 1) || (FAULT_HANDLING_MODE > 2)
#error "Select a valid handling mode (1, 2)"
#endif

#if (FAULT_HANDLING_MODE == 2) && (dg_configCODE_LOCATION == NON_VOLATILE_IS_NONE)
#error "When build in RAM the handling mode cannot be 2"
#endif

#define PX_SET_DATA_REG_ADDR(_port)     ((volatile uint32_t *)(GPIO_BASE + offsetof(GPIO_Type, P0_SET_DATA_REG)) + _port)
#define PX_SET_DATA_REG(_port)          *PX_SET_DATA_REG_ADDR(_port)
#define PX_RESET_DATA_REG_ADDR(_port)   ((volatile uint32_t *)(GPIO_BASE + offsetof(GPIO_Type, P0_RESET_DATA_REG)) + _port)
#define PX_RESET_DATA_REG(_port)        *PX_RESET_DATA_REG_ADDR(_port)
#define PXX_MODE_REG_ADDR(_port, _pin)  ((volatile uint32_t *)(GPIO_BASE + offsetof(GPIO_Type, P0_00_MODE_REG)) + (_port * 16)  + _pin)
#define PXX_MODE_REG(_port, _pin)       *PXX_MODE_REG_ADDR(_port, _pin)

__STATIC_FORCEINLINE void turn_on_led1(void)
{
        REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, COM_SLEEP);
        while ((CRG_TOP->SYS_STAT_REG & REG_MSK(CRG_TOP, SYS_STAT_REG, COM_IS_UP)) == 0);
        CRG_TOP->P1_SET_PAD_LATCH_REG = 1 << LED1_PIN;
        PXX_MODE_REG(LED1_PORT, LED1_PIN) = HW_GPIO_FUNC_GPIO | HW_GPIO_MODE_OUTPUT;
        PX_SET_DATA_REG(LED1_PORT) = 1 << LED1_PIN;
}

/* Magic value indicating that system rebooted due to system exception event. */
#define FAULT_HANDLING_MAGIC_VALUE     (0xAABBCCDD)

#define STACKED_FRAME_FP_IDX_START     ( 8 )
#define STACKED_FRAME_FP_IDX_END       ( 23 )
#define STACKED_FRAME_FP_MAX_ENTRIES   ( 16 )

#define HARDFAULT_EXCEPTION_TYPE       ( 3 )
#define MEMMANGE_FAULT_EXCEPTION_TYPE  ( 4 )
#define BUSFAULT_EXCEPTION_TYPE        ( 5 )
#define USAGEFAULT_EXCEPTION_TYPE      ( 6 )

typedef enum {
        STACK_OVERFLOW_IS_NONE  = 0x0,
        STACK_OVERFLOW_IS_MSP        ,
        STACK_OVERFLOW_IS_PSP
} STACK_OVERFLOW;


/* Configurable Fault Status Register */
typedef union {
        struct {
           uint32_t _MMFSR : 8;
           uint32_t _BFSR  : 8;
           uint32_t _UFSR  : 16;
        } _xFSR;

        uint32_t _CFSR;
} CFSR_t;


/* Structure that holds the CPU status following a fault exception */
typedef struct {
     /* Magic value */
     uint32_t magic_val;

     /* IRQ number */
     uint32_t irq_num;

     /* FP content validity */
     bool is_fp_valid;
     uint8_t stacklimit_context;

     /* CPU registers stacked on exception entrance. */
     uint32_t _r0;
     uint32_t _r1;
     uint32_t _r2;
     uint32_t _r3;
     uint32_t _r12;
     uint32_t _lr;
     uint32_t _pc;
     uint32_t _xPSR;

     /* Floating Point (FP) stacked registers */
     uint32_t _s0;
     uint32_t _s1;
     uint32_t _s2;
     uint32_t _s3;
     uint32_t _s4;
     uint32_t _s5;
     uint32_t _s6;
     uint32_t _s7;
     uint32_t _s8;
     uint32_t _s9;
     uint32_t _s10;
     uint32_t _s11;
     uint32_t _s12;
     uint32_t _s13;
     uint32_t _s14;
     uint32_t _s15;
     uint32_t _FPSCR;

     /* CPU status registers (related to fault exceptions). */
     CFSR_t   _CFSR;
     uint32_t _HFSR;
     uint32_t _DFSR;
     uint32_t _AFSR;

     /* System address registers (related to fault exceptions).  */
     uint32_t _MMFAR;
     uint32_t _BFAR;
} Fault_t;


/* Bit fields definitions and descriptions */
#define UFSR_BIT_FIELD_DESCRIPTION                                                                                                                                      \
        "UFSR[0]          UNDEFINSTR        An undefined instruction error has occurred."                                                                                  , \
        "UFSR[1]          INVSTATE          An invalid core state took place. EPSR.T or EPSR.IT validity error has occurred (e.g. switching from Thumb to ARM state)."     , \
        "UFSR[2]          INVPC             Invalid PC flag - Attempt to do an exception with a bad EXC_RETURN value."                                                     , \
        "UFSR[3]          NOCP              Attempt to execute a co-processor instruction while not supported or enabled."                                                 , \
        "UFSR[4]          STKOF             A stack overflow error has occurred."                                                                                          , \
        "UFSR[5]          RESERVED"                                                                                                                                        , \
        "UFSR[6]          RESERVED"                                                                                                                                        , \
        "UFSR[7]          RESERVED"                                                                                                                                        , \
        "UFSR[8]          UNALIGNED         An unaligned access took place."                                                                                               , \
        "UFSR[9]          DIVBYZERO         An integer division by zero error has occurred."                                                                               , \
        "UFSR[10]         RESERVED"                                                                                                                                        , \
        "UFSR[11]         RESERVED"                                                                                                                                        , \
        "UFSR[12]         RESERVED"                                                                                                                                        , \
        "UFSR[13]         RESERVED"                                                                                                                                        , \
        "UFSR[14]         RESERVED"                                                                                                                                        , \
        "UFSR[15]         RESERVED"                                                                                                                                          \

#define BFSR_BIT_FIELD_DESCRIPTION                                                                                                                                                                                    \
        "BFSR[0]          IACCVIOL          Instruction bus error - The fault is triggered only when the faulting instruction is issued."                                                                              , \
        "BFSR[1]          DACCVIOL          Precise data bus error - The stacked PC value points to the instruction that caused the fault."                                                                            , \
        "BFSR[2]          RESERVED"                                                                                                                                                                                    , \
        "BFSR[3]          UNSTKERR          Unstacking operation (for an exception return) has caused one or more access violations - This fault is chained to the handler and the original return is still present."  , \
        "BFSR[4]          STKERR            Stacking operation (for an exception entry) has caused one or more BusFults - Stacked frame might be incorrect."                                                           , \
        "BFSR[5]          LSPERR            A bus fault occurred during FP lazy state preservation."                                                                                                                   , \
        "BFSR[6]          RESERVED"                                                                                                                                                                                    , \
        "BFSR[7]          BFARVALID         BFAR holds a valid fault address - A precise exception took place."


#define MMFSR_BIT_FIELD_DESCRITION                                                                                                                                                                                    \
        "MMFSR[0]         IACCVIOL          Instruction access violation - The processor attempted an instruction fetch from a region that does not permit execution."                                                 , \
        "MMFSR[1]         DACCVIOL          Data access violation - The stacked PC value points to the faulting instruction."                                                                                          , \
        "MMFSR[2]         RESERVED"                                                                                                                                                                                    , \
        "MMFSR[3]         MUNSTKERR         MemManage fault on unstacking operation has caused one or more access violations - This fault is chained to the handler and the original return is still present."         , \
        "MMFSR[4]         MSTKERR           Stacking operation (for an exception entry) has caused one or more BusFults - Stacked frame might be incorrect."                                                           , \
        "MMFSR[5]         MLSPERR           MemManage fault occurred during FP lazy state preservation."                                                                                                               , \
        "MMFSR[6]         RESERVED"                                                                                                                                                                                    , \
        "MMFSR[7]         MMARVALID         MMFAR holds a valid fault address - A precise exception took place."


#define ARMv8_IRQn_DEFINITION              \
        "No interrupt" ,                   \
        "Reset"       ,                    \
        "NMI"         ,                    \
        "HardFault"   ,                    \
        "MemManage"   ,                    \
        "BusFault"    ,                    \
        "UsageFault"  ,                    \
        "SecureFault" ,                    \
        "Reserved"    ,                    \
        "Reserved"    ,                    \
        "Reserved"    ,                    \
        "SVCall"      ,                    \
        "DebugMonitor",                    \
        "Reserved"    ,                    \
        "PendSV"      ,                    \
        "SysTick"

        /* Device-specific interrupts */

#endif /* FAULT_HANDLING_H_ */
