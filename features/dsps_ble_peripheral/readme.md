Renesas Serial Port Service 
==================

## Overview

The Renesas Serial Port Service emulates a serial cable communication. Currently, the asynchronous UART interface with HW and SW flow control is demonstrated. The DA14592 Bluetooth Smart SoC can function either in the GAP central role (scanner) or GAP advertiser. The scanner role is also available on Android and iOS devices, namely `SmartConsole`.  

To achieve the maximum BLE throughput the following settings are defined, by default:

- UART baud rate @3000000 with HW flow control. 
- System clock @64 MHz.
- Bluetooth connection interval set to 15ms.
- Data length extension (DLE) set to maximum value, that is 251 bytes. The same goes for Maximum Length Unit (MTU).

- 2M Bluetooth physical.

### HW & SW Configurations

- **Hardware Configurations**
  - This example runs on DA14592 family of devices.
  - A Pro development kit is needed for this example.
- **Software Configurations**
  - Download the latest SDK version for the target family of devices.
  - SEGGER J-Link tools are normally downloaded and installed as part of the e2 Studio installation.
  - Download the Renesas `SmartConsole` application on your smartphone. 


## How to run the example

### Initial Setup

- Download the source code from the Support Website or GitHub.

- Import the `dsps_ble_peripheral` folder into your workspace (there should be no path dependencies). 

- Connect the target device to your host PC via USB1. The mentioned port is used to power the device via VBAT and also to support debugging functionality (UART and JTAG).

- Compile the code and load it into the chip. SUOTA build options are also available  to support over-the-air firmware updates. 

- Open a serial terminal of your choice and make sure the following settings are aligned with the application-defined settings:

  - Baud rate should reflect `CFG_UART_SPS_BAUDRATE` (3000000 default value) 
  - Flow control type should reflect the definition of `CFG_UART_HW_FLOW_CTRL` or `CFG_UART_SW_FLOW_CTRL` (HW flow control is the default mode). 
  
- Press the reset button on the daughterboard to start executing the application. 

- Open the SmartConsole APP. The DSPS peripheral device should be appeared. Connect and two text boxes should be appeared. `Receive Console Mobile Data` should display data received from the connect peer device whilst `Send Console Mobile Data` should be used to send data to the connect peer device. 

- Optionally, open the SEGGER JLink RTT Viewer to get various debug messages along with throughput measurements. To do so, the `_SEGGER_RTT` symbol address should be retrieved from the generated `.map` file and be filled in the address field of the viewer as depicted below:  

  

  ![initiating_rtt_viewer](assets\initiating_rtt_viewer.png)



**Note:** The usage of `SmartConsole` scanner can be replaced by a second DA14592 device running the  `dsps_ble_central` firmware. 

## Known Limitations

- For baud rates higher than 115200  (`CFG_UART_SPS_BAUDRATE`) some data loss might be observed when the UART serial interface is selected and the SW flow control is utilized. The larger the baud rate the more the data loss. 
- Right after the flow control activation certain number of on-the-fly packets should be transmitted. This number can vary from 5 to 30 depending on the serial interface speed. Such a condition should cause RX queue full assertions. It is suggested that either the RX queue size (`RX_SPS_QUEUE_SIZE`) is increased or the RX high water-mark level (`RX_QUEUE_HWM`) is reduced so data transmission is forbidden earlier. 
- A deadlock can occur if two devices are employed (central and peripheral role respectively) and under the following conditions:
  - System clock speed @32MHz
  - UART HW flow control is activated
  - Data are being transmitted at both sides simultaneously. 

- Heap overflow might be observed if system's clock speed is set @32MHz and data packets are transmitted at high rates. If this is the case, either increase the OS heap space (`configTOTAL_HEAP_SIZE`) or increase the system clock speed by leveraging DBLR64MHz (`sysclk_DBLR64`).


## License

**************************************************************************************

 Copyright (c) 2023 Dialog Semiconductor. All rights reserved.

 This software ("Software") is owned by Dialog Semiconductor. By using this Software
 you agree that Dialog Semiconductor retains all intellectual property and proprietary
 rights in and to this Software and any use, reproduction, disclosure or distribution
 of the Software without express written permission or a license agreement from Dialog
 Semiconductor is strictly prohibited. This Software is solely for use on or in
 conjunction with Dialog Semiconductor products.

 EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES OR AS
 REQUIRED BY LAW, THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE PROVIDED
 IN A LICENSE AGREEMENT BETWEEN THE PARTIES OR BY LAW, IN NO EVENT SHALL DIALOG
 SEMICONDUCTOR BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL, OR
 CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE SOFTWARE.

**************************************************************************************
