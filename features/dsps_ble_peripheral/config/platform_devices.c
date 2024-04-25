/**
 ****************************************************************************************
 *
 * @file platform_devices.c
 *
 * @brief Configuration of devices connected to board
 *
 * Copyright (C) 2016-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include "ad_uart.h"
#include "platform_devices.h"
#include "dsps_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (dg_configUART_ADAPTER == 1)

/* DSPS UART device driver */
const ad_uart_driver_conf_t uart_dsps_drv = {
#define TX_FIFO_LVL  0
#define RX_FIFO_LVL  0

        .hw_conf = {
                .baud_rate = CFG_UART_SPS_BAUDRATE,
                .data = HW_UART_DATABITS_8,
                .parity = HW_UART_PARITY_NONE,
                .stop = HW_UART_STOPBITS_1,
#ifdef CFG_UART_HW_FLOW_CTRL
                .auto_flow_control = 1,
#endif
#ifdef CFG_UART_SW_FLOW_CTRL
                .auto_flow_control = 0,
#endif
                .use_fifo = 1,
                /*
                 * 0 = 1 character in the FIFO,
                 * 1 = FIFO 1/4 full (4 bytes),
                 * 2 = FIFO 1/2 full (8 bytes),
                 * 3 = FIFO 2 less than full
                 */
                .tx_fifo_tr_lvl = TX_FIFO_LVL,
                .rx_fifo_tr_lvl = RX_FIFO_LVL,
#if HW_UART_DMA_SUPPORT
                .use_dma = dg_configUART_RX_CIRCULAR_DMA ? 1 : 0,
                .tx_dma_channel = HW_DMA_CHANNEL_3,
                .rx_dma_channel = HW_DMA_CHANNEL_2,
                /* 0 no burst mode, 1 burst size of 4, 2 burst size of 8 */
                .tx_dma_burst_lvl = ((TX_FIFO_LVL == 0 || TX_FIFO_LVL == 3) ? 0 :
                                                          TX_FIFO_LVL == 1  ? 1 : 2),
                .rx_dma_burst_lvl = ((RX_FIFO_LVL == 0 || RX_FIFO_LVL == 3) ? 0 :
                                                          RX_FIFO_LVL == 1  ? 1 : 2),
                .dma_prio.use_prio = true,
                /* When two channels of the same priority request the bus simultaneously,
                 * the channel with the lowest index will get access i.e. rx_dma_channel.*/
                .dma_prio.rx_prio = HW_DMA_PRIO_7,
                .dma_prio.tx_prio = HW_DMA_PRIO_7
#endif
        }
};

/* DSPS UART bus connections */
const ad_uart_io_conf_t uart_dsps_bus = {
        .rx = {
                .port = UART_RX_PORT, .pin = UART_RX_PIN,
                .on =  { HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_UART2_RX, false },
                .off = { HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO, true },
        },
        .tx = {
                .port = UART_TX_PORT, .pin = UART_TX_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_UART2_TX, false },
                .off = { HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO, true },
        },
#ifdef CFG_UART_HW_FLOW_CTRL
        .rtsn = {
                .port = UART_RTS_PORT, .pin = UART_RTS_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_UART2_RTSN, false },
                /*
                 * Keep the RTS line de-asserted (logic high) as it might happen that the BLE
                 * connection with a peer device is dropped and at the same time the device
                 * is being reading data from the serial interface (e.g. sending a file).
                 * In this case and because the RTS line will no longer be driven high (de-asserted)
                 * the serial console will keep reading data over the UART interface.
                 * A drawback to this approach is the slight increase in power consumption as the
                 * pin is driven high.
                 */
                .off = { HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true },
        },
        .ctsn = {
                .port = UART_CTS_PORT, .pin = UART_CTS_PIN,
                .on =  { HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_UART2_CTSN, false },
                .off = { HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO, true },
        },
#endif
};

/* DSPS UART controller */
const ad_uart_controller_conf_t uart_dsps_ctrl = {
        SER1_UART,
        &uart_dsps_bus,
        &uart_dsps_drv
};

#endif /* dg_configUART_ADAPTER */

#ifdef __cplusplus
}
#endif
