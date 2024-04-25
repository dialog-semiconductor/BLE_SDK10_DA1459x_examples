# Mapping System Clocks To GPIO

This sample code demonstrates mapping system clocks to GPIO, typically for debugging purposes. The clocks that can be exported are XTAL32K, RC32K, RCLP, XTAL32M, RC32M as well as DIVN. A system clock can be outputted in the following two ways. The first method involves mapping a system clock to any GPIO of interest whereas the second method allows mapping specific clocks to dedicated GPIO as follows:

| Clock Source | GPIO     |
| ------------ | -------- |
| RC32M        | PORT1.2  |
| DIVN         | PORT0.8  |
| XTAL32M      | PORT0.11 |

The demonstration example defines a system clock and its associated mapped port as depicted below. Users should change the default values to meet their application's needs.

```
#define CLOCK_MODE_DEFAULT     CLOCK_MODE_MANUAL

#define CLOCK_SOURCE_DEFAULT   CLOCK_SOURCE_RCX

#define CLOCK_PORT_DEFAULT     HW_GPIO_PORT_0

#define CLOCK_PIN_DEFAULT      HW_GPIO_PIN_10

#define CLOCK_STATUS_DEFAULT   true
```

In addition to defining clock setting at compile time, the demonstration example exploits the command line interface (CLI) module provided by SDK. In doing so, any of the clock settings can be changed at run time. That CLI interface can be disabled by zeroing the following macro in `custom_config_xxx.h`.

```
#define CLOCK_TASK_CLI_ENABLE                   ( 0 )
```

Following is a short description of the supported commands. If needed, users can expand the command list by adding further handlers in `cmd_handlers[]`

To change the clock source enter 'clock_source <arg1>', where:

| arg1 | Clock Source                                                 |
| :--- | ------------------------------------------------------------ |
| 0    | To export the external XTAL32K low power clock. It's a pre-requisite that XTAL32K is the selected as LP clock. |
| 1    | To export the internal RCLP oscillator.                      |
| 2    | To export the internal RCX low power clock. It's a pre-requisite that RCX is the selected as LP clock. |
| 3    | To export the external XTAL32M main clock.                   |
| 4    | To export the internal RC32M oscillator.                     |
| 5    | To export the DIVN clock path. That path should reflect the XTAL32M clock. |

To change the clock mode enter 'clock_mode <arg1>', where: 

| arg1 | Clock Mode                                                   |
| :--- | ------------------------------------------------------------ |
| 0    | To select the manual mode (any system clock can be mapped to any port). |
| 1    | To select the auto mode where specific system clocks and be mapped to dedicated ports. The supported clocks are XTAL32M, RC32M and DIVN. It makes sense that the current port might be overwritten to meet system requirements. Before switching to auto mode the current port is stored so, it can be re-stored upon switching to manual mode. |

To change the port where a clock is mapped to enter 'clock_port <arg1> <arg2>', where:

| arg1     | Clock Port            |
| :------- | --------------------- |
| 0        | To select PORT0       |
| 1        | To select PORT1       |
| **arg2** | **Clock Port**        |
| [0..15]  | To select a valid pin |

Please note that, If the auto clock mode is currently selected, an attempt to define a new port will result in switching to manual mode so the new port can be demonstrated.

To enable or disable exporting a clock source enter 'clock_enable <arg1>', where:

| arg1 | Clock Source                        |
| :--- | ----------------------------------- |
| 0    | To disable exporting a system clock |
| 1    | To enable exporting a system clock  |

To get the current clock status enter 'clock_status'. 

## HW and SW Configuration

  - **Hardware Configuration**
    - This example runs on DA14692 family of devices.
    - A Pro development kit is needed for this example.
  - **Software Configuration**
    - Download the latest SDK version for the target family of devices.

    - SEGGER J-Link tools are normally downloaded and installed as part of the e2 Studio installation.

## How to run the example

### Initial Setup

- Download the source code from the Support Website.
- Import the project into your workspace (there should be no path dependencies).
- Connect the target device to your host PC via USB1. The mentioned port is used to power the device via VBAT and also to support debugging functionality (UART and JTAG).
- Compile the code (either in Release or Debug mode) and load it into the chip.
- Open a serial terminal (115200/8 - N - 1)
- Press the reset button on the daughterboard to start executing the application. 
- Use the serial console to to update the clock settings by entering a valid command/parameters. The corresponding messages should be displayed so users are aware of the progress of the requested command.
- The selected system clock can be captured on the selected port with the help of a logic analyzer.

## Known Limitations

There are no known limitations for this sample code.
