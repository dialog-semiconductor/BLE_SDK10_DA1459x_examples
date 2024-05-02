/*
 * MCP4921 DAC Module Configuration Macros
 *********************************************************************************************************
 */

#ifndef MCP4921_H_
#define MCP4921_H_

#include <stdio.h>

/* MCP4921 control register bits-masks */
#define MCP4921_AB_Msk           ( 0x8000 )
#define MCP4921_GA_Msk           ( 0x2000 )
#define MCP4921_SHDN_Msk         ( 0x1000 )
#define MCP4921_DATA_Msk         ( 0x0FFF )
#define MCP4921_CTRL_Msk         ( 0xF000 )

#define MCP4921_GET_MSK(x)       MCP4921_ ## x ## _Msk

/* MCP4921 control register bit-fields */
typedef enum {
        MCP4921_AB_CONTROL_BIT_RESET   = 0, /* Select DACA channel */
        MCP4921_GA_CONTROL_BIT_RESET   = 0, /* Output gain 1x */
        MCP4921_SHDN_CONTROL_BIT_RESET = 0, /* Shutdown the selected DAC channel  */
        MCP4921_AB_CONTROL_BIT_SET     = /*MCP4921_GET_MSK(AB)*/0,  /* Select DACB channel */
        MCP4921_GA_CONTROL_BIT_SET     = MCP4921_GET_MSK(GA),  /* Output gain 2x */
        MCP4921_SHDN_CONTROL_BIT_SET   = MCP4921_GET_MSK(SHDN) /* Activate the selected DAC channel */
} MCP4921_CONTROL_BITS;

/*
 * Set the MCP4921 2-byte register:
 *
 *          +------+-----+------+------+------+------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
 *  Bit:    |  15  |  14 |  13  |  12  |  11  |  10  |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
 *          +------+-----+------+------+------+------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
 *  Func:   |  AB  |  -  |  GA  | SHDN |  D11 |  D10 |  D9 |  D8 |  D7 |  D6 |  D5 |  D4 |  D3 |  D2 |  D1 |  D0 |
 *          +------+-----+------+------+------+------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
 **/
#define MCP4921_SET_REG(_val, _reg)     ( ((_val) & MCP4921_GET_MSK(DATA)) | ((_reg) & MCP4921_GET_MSK(CTRL)) )

#endif /* MCP4921_H_ */
