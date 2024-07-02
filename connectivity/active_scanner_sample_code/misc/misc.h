/**
 ****************************************************************************************
 *
 * @file misc.h
 *
 * @brief Miscellaneous functionality
 *
 * Copyright (C) 2015-2024 Renesas Electronics Corporation and/or its affiliates.
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

#ifndef MISC_H_
#define MISC_H_

#include <stdio.h>
#include "hw_gpio.h"

/*********************************************************************
 *
 *       Defines
 *
 *********************************************************************
 */
#define KiB    1024

#if DBG_LOG_ENABLE
   #define DBG_LOG(_f, args...)   printf((_f), ## args)
#else
   #define DBG_LOG(_f, args...)
#endif /* DBG_LOG_ENABLE */

#if DBG_IO_ENABLE
   #define DBG_IO_ON(_port, _pin)                                                                    \
        {                                                                                          \
                hw_sys_pd_com_enable();                                                            \
                hw_gpio_set_pin_function((_port), (_pin), HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO); \
                hw_gpio_set_active((_port), (_pin));                                               \
                hw_gpio_pad_latch_enable((_port), (_pin));                                         \
                hw_gpio_pad_latch_disable((_port), (_pin));                                        \
                hw_sys_pd_com_disable();                                                           \
        }

   #define DBG_IO_OFF(_port, _pin)                                                                   \
        {                                                                                          \
                hw_sys_pd_com_enable();                                                            \
                hw_gpio_set_pin_function((_port), (_pin), HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO); \
                hw_gpio_set_inactive((_port), (_pin));                                             \
                hw_gpio_pad_latch_enable((_port), (_pin));                                         \
                hw_gpio_pad_latch_disable((_port), (_pin));                                        \
                hw_sys_pd_com_disable();                                                           \
        }

#else
   #define DBG_IO_ON(_port, _pin)
   #define DBG_IO_OFF(_port, _pin)
#endif /* DBG_IO_ENABLE */

#endif /* MISC_H_ */
