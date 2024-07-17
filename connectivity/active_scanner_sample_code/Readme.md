# Active Scanner Demonstration Example

This example demonstrating setting up the Bluetooth controller in central role. Active scanning can be initiated through the serial console. Once the active scanning session times out, scanning is allowed to be re-initiated. If logging is enabled (enabled by default) each device scanned along with its advertising data are displayed on the serial console.

## HW and SW Configuration

  - **Hardware Configuration**
    - This example runs on the DA1459x family of devices.
    - A [Pro Development Kit](https://www.renesas.com/us/en/products/wireless-connectivity/bluetooth-low-energy/da14592-016fdevkt-p-smartbond-da14592-bluetooth-low-energy-52-soc-development-kit-pro) (DevKit) is needed for this example.
  - **Software Configuration**
    - Download the latest [SDK](https://www.renesas.com/us/en/products/wireless-connectivity/bluetooth-low-energy/da14592-smartbond-multi-core-bluetooth-le-52-soc-embedded-flash?gad_source=1) version for the target family of devices.

    - SEGGER J-Link tools are normally downloaded and installed as part of the [e2 Studio](https://www.renesas.com/us/en/software-tool/smartbond-development-tools) installation.

## How to run the example

### Initial Setup

- Download the source code from [GitHub](https://github.com/dialog-semiconductor/BLE_SDK10_DA1459x_examples). 
- Import the project into your workspace (there should be no path dependencies). If you are not familiar with these processes it's advised that you first familiarize yourself with the [Getting Started](https://lpccs-docs.renesas.com/um-b-166-da1459x_getting_started/index.html) guide.
- Connect the target device to your host PC via USB1. The mentioned port is used to power the device and to support serial and JTAG interfaces. These two interfaces can be used both for flashing and debugging purposes.
- Compile the source code (either in Release or Debug mode) and flash it into the chip. Please note that the debug flavor should be used merely for debugging purposes since it should increase the generated binary file, significantly. In addition, the source code is built to work with the embedded flash Working with external flash memory devices is out of the scope of this demonstration example.  
- Open a serial terminal (115200/8-N-1).
- Once the application image is flashed, press the RESET button on the daughter board to start executing the application. 

- The terminal should display the random address generated for device.

- Scanning can be initiated by typing `active_scanner_start`. If the command is typed correctly, all scanned devices along with their data should be displayed. 

## Known Limitations

There should be no known limitations for this example.
