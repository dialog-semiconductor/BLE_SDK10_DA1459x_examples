FreeRTOS RTC demo application
=============================

## Overview

This application is a simple implementation of a FreeRTOS application showcasing the use of the RTC. The main.c creates an (RTC) task which wakes up the system every second and print outs the new time. In this task an alarm timer is set, which will invoke a callback function after 1 minute and print a notification on the console.

## Installation procedure

To install the project follow the [General Installation and Debugging Procedure](@ref install_and_debug_procedure).

## File structure

The following file structure will be created:

* projects/dk_apps/templates/freertos_retarget
        * config
                * custom_config_eflash.h
                * custom_config_ram.h
        * sdk
        * startup
        * main.c
        * rtc_demo_task.c

## Existing build configurations

The template contains build configurations for executing it from RAM or eFLASH. 

- `DA1459X-00-Debug_eFLASH`.    The project is built to  run from eFLASH. The executable is built with debug (-Og) information.
- `DA1469X-00-Debug_RAM`.       The project is built to run from RAM.    The executable is built with debug (-Og) information.
- `DA1459X-00-Release_eFLASH`.  The project is built to run from eFLASH. The executable is built with no debug information and size optimization (-Os).

Note: By default serial output information is printed on the console. To disable printing set the following flag to (0): DBG_SERIAL_CONSOLE
