I2C EEPROM example application.
=========================================

## Overview

This example implements a simple read and write communication scenario over the I2C interface using the I2C adapter layer. For demonstration, MirkoE EEPROM click ClickBoard with FT24C08A is used.

Interface I2C1 is configured as I2C master while EEPROM is used as an I2C slave.

The I2C master can write and read the EEPROM. Write and read data is printed on UART.

## HW and SW configuration
* **Hardware configuration**

    - This example runs on the DA14592 Bluetooth Smart SoC devices.
    - The DA1459x Pro Development kit is needed for this example.
    - Connect the Development kit to the host computer.
	- Place the EEPROM ClickBoard on the MikroBUS connector

**UART terminal with the following settings is needed to observe the master task printouts**

        | Setting      | Value    |
        |:------------:|:--------:|
        | Baudrate     | 115200   |
        | Data bits    | 8        |
        | Stop bits    | 1        |
        | Parity       | None     |

**The EEPROM ClickBoard on the MikroBUS connector connects the following wiring/pairs (loopback wiring from MASTER to SLAVE) are needed before starting the application. The pins settings are located in `peripheral_setup.h`**

        | DA1459x     | EEPROM   |
        |:-----------:|:--------:|
        | Master      | Slave    |
        | P0_14 (SDA) |  SDA     |
        | P1_02 (SCL) |  SCL     |

                                 ___________
                                |  EEPROM   |
                                |           |
                                | SDA  SCL  |
                                |___________|     
                                   |    |                       
     ____________                  |    |           
    | DA1459x SDA|---------------------=|
    |         SCL|--------------------=-'
    |____________|                                     

**To enabling I2C for MikroBUS, both J15 and J16 must be set.**

J15:
Power Pull-ups with 3.3V	Mount pos.1-2
Power Pull-ups with 5V		Mount pos.2-3
J16:
Enable SDA 					Mount pos.1-2
Enable SCL					Mount pos.3-4


* **Software configuration**
  - e²Studio V2023-10 or greater.
  - SDK 10.1.2.x
  - **SEGGER J-Link** tools should be downloaded and installed.

## How to run the example

### Initial Setup

- Download the source code from the Support Website.
- Import the project into your workspace.
- Compile and launch EFLASH target
- Run from the EFLASH



