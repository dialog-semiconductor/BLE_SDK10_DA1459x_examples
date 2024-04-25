I2C Adapter Demonstration Example
=================================

## Overview

This example implements a simple read/write communication scenario over 
the I2C interface using the I2C adapter layer. For demonstration, MirkoE Temp&Hum 17 ClickBoard with HS3001 is used.

The I2C interface is configured in the master role while HS3001 is used as an I2C slave.

The I2C master initiates a write to wake up the HS3001 and then after a 40ms delay initiates a read.
Temperature and humidity data is printed on UART.

## HW and SW configuration
* **Hardware configuration**

    - This example runs on the DA14592 Bluetooth Smart SoC devices.
    - The DA14592 Pro Development kit is needed for this example.
    - Connect the Development kit to the host computer.
    - Make sure the right jumpers are placed on J15 and J16.

**UART terminal with the following settings is needed to observe the master task printouts**

        | Setting      | Value    |
        |:------------:|:--------:|
        | Baudrate     | 115200   |
        | Data bits    | 8        |
        | Stop bits    | 1        |
        | Parity       | None     |

**The following pin configuration is according to the ClickBoard connector provided on the DA14592 Development kit. The pins settings are located in `peripheral_setup.h`**

        | DA14592     | Temp&Hum 17  |
        |:-----------:|:------------:|
        | Master      | Slave        |
        | P1_02 (SDA) |  SDA         |
        | P0_14 (SCL) |  SCL         |

                                 ___________
                                |  HS3001   |
                                |           |
                                | SDA  SCL  |
                                |___________|     
                                   |    |                       
     ____________                  |    |           
    | DA14592 SDA|-----------------'    |
    |         SCL|----------------------'
    |____________|                                     

* **Software configuration**
  - e²Studio 2023-10  or greater.
  - SDK 10.1.2.x
  - **SEGGER J-Link** tools should be downloaded and installed.

## How to run the example

### Initial Setup

- Download the source code from the Github.
- Import the project into your workspace.
- Compile and launch eFLASH target.
- Load in the eFLASH and run the example.

The UART will show the following output:

	I2C init 
	Write I2C: 0 
	Successfully read I2C 
	Hum: 61 Temp: 20 
	Write I2C: 0 
	Successfully read I2C 
	Hum: 61 Temp: 20 
	Write I2C: 0 
	Successfully read I2C 
	Hum: 61 Temp: 20 
	Write I2C: 0 
	Successfully read I2C 
	Hum: 61 Temp: 20 