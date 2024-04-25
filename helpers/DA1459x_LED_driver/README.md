Blinky application
===============================

## Overview

This application is a simple implementation of a FreeRTOS application based on 'freertos_retarget' application which can be found in the SDK.
The main.c creates a task which toggles an LED on the development board.

## Installation procedure

To install the project follow the -General Installation and Debugging Procedure.

## File structure

The following file structure will be created:

* starting_new_project
        * config
        		* custom_config_eflash.h 
                * custom_config_qspi.h
                * custom_config_ram.h
        * sdk
        * startup
        * main.c

## Existing build configurations

The template contains build configurations for executing it from RAM or eFLASH. 

- `DA149X-00-Debug_RAM`. The project is built to be run from RAM. The executable is built with debug (-Og) information.
- `DA1459X-00-Debug_eFLASH`. The project is built to be run from embedded FLASH. The executable is built with debug (-Og) information.
- `DA1459X-00-Release_eFLASH`. The project is built to be run from embedded FLASH. The executable is built with no debug information but with size optimization (-Os).

## Enable printf

The application gives the user the option to print '#' to the output by defining the CONFIG_RETARGET pre-processor macro in the custom_config_xxx.h file
