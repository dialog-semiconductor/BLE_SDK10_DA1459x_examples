/**
 ****************************************************************************************
 *
 * @file ascii_opcodes.h
 *
 * @brief ASCII colors opcodes
 *
 * Copyright (C) 2020-2024 Renesas Electronics Corporation and/or its affiliates.
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

#ifndef ASCII_OPCODES_H_
#define ASCII_OPCODES_H_

#define BOLD        "\e[1m"
#define DIM         "\e[2m"
#define UNDERLINED  "\e[4m"
#define BLINK       "\e[5m"
#define REVERSE     "\e[7m"
#define HIDDEN      "\e[8m"

#define RESET_ALL         "\e[0m"
#define RESET_BOLD        "\e[21m"
#define RESET_DIM         "\e[22m"
#define RESET_UNDERLINED  "\e[24m"
#define RESET_BLINK       "\e[25m"
#define RESET_REVERSE     "\e[27m"
#define RESET_HIDDEN      "\e[28m"

#define FG_DEFAULT  "\e[39m"
#define FG_BLACK    "\e[30m"
#define FG_RED      "\e[31m"
#define FG_GREEN    "\e[32m"
#define FG_YELLOW   "\e[33m"
#define FG_BLUE     "\e[34m"
#define FG_MAGENTA  "\e[35m"
#define FG_CYAN     "\e[36m"
#define FG_LGRAY    "\e[37m"
#define FG_DGRAY    "\e[90m"
#define FG_LRED     "\e[91m"
#define FG_LGREEN   "\e[92m"
#define FG_LYELLOW  "\e[93m"
#define FG_LBLUE    "\e[94m"
#define FG_LMAGENTA "\e[95m"
#define FG_LCYAN    "\e[96m"
#define FG_WHITE    "\e[97m"

#define BG_DEFAULT  "\e[49m"
#define BG_BLACK    "\e[40m"
#define BG_RED      "\e[41m"
#define BG_GREEN    "\e[42m"
#define BG_YELLOW   "\e[43m"
#define BG_BLUE     "\e[44m"
#define BG_MAGENTA  "\e[45m"
#define BG_CYAN     "\e[46m"
#define BG_LGRAY    "\e[47m"
#define BG_DGRAY    "\e[100m"
#define BG_LRED     "\e[101m"
#define BG_LGREEN   "\e[102m"
#define BG_LYELLOW  "\e[103m"
#define BG_LBLUE    "\e[104m"
#define BG_LMAGENTA "\e[105m"
#define BG_LCYAN    "\e[106m"
#define BG_WHITE    "\e[107m"

#endif /* ASCII_OPCODES_H_ */
