
# iBeacon

## Example description

This application demonstrates a DA1459x device operating as an iBeacon, advertising information according to the iBeacon standard.


## HW and SW configuration

* **Hardware configuration**

    - This example runs on DA14592 family of devices.
    - A Pro development kit is needed for this example.

* **Software configuration**

    - Download the latest SDK version for the target family of devices.
    - **SEGGER’s J-Link** tools are normally downloaded and installed as part of the e2 Studio installation.

## How to run the example

### Initial setup

- Download the source code from the Support Website.
- Import the project into your workspace (there should be no path dependencies).
- Connect the target device to your host PC via USB1. The mentioned port is used to power the device via VBAT and also to support debugging functionality (UART and JTAG).
- Compile the code (either in Release or Debug mode) and load it into the chip.
- Press the reset button on the daughterboard to start executing the application.
- Use an iOS device or download a BLE scanner application (eg. Renesas SmartBond) to see the (i)Beacon
- To see iBeacon and not Beacon in some BLE scanner applications other than Renesas SmartBond, use the Apple, Inc. company id, replace the following in main.c:"#define IBEACON_COMPANY_ID 0xd2, 0x00" with "#define IBEACON_COMPANY_ID 0x4c, 0x00".

## Known Limitations

There are no known limitations for this application.
