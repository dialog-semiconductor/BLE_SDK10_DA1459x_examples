## Bare Metal Retarget Demonstration Example

This is a simple implementation of a basic retarget application that runs is bare metal (no OS) context.

## HW and SW Configuration

  - **Hardware Configuration**
    - This example runs on DA14592 family of devices.
    - A Pro development kit is needed for this example.
  - **Software Configuration**
    - Download the latest SDK version for the target family of devices.

    - SEGGER J-Link tools are normally downloaded and installed as part of the e2 Studio installation.

## How to run the example

### Initial Setup

- Download the source code from the Support Website.

- Import the project into your workspace (there should be no path dependencies).

- Connect the target device to your host PC via USB1. The mentioned port is used to power the device via VBAT and also to support debugging functionality (UART and JTAG).

- Compile the code and load it into the chip.

- Open a serial terminal (115200/8 - N - 1).
- Press the reset button on the daughterboard to start executing the application. 
- `Hello World` should be printed on the console every 1 second.


## Known Limitations

There are no known limitations for this example.