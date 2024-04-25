# GPIO Handling Demonstration Example

This sample code demonstrates using GPIO pins that are handled directly by developers (outside adapter context). For this demonstration LED1, mounted on the daughterboard, is toggled @1Hz. The toggling period can be adjusted via `APP_LED1_TOGGLE_MS`. The sample code introduces two basic approaches namely `static` and `dynamic`. Switching between the mentioned two modes is achieved via `APP_GPIO_DYNAMIC_MODE`. Following is a short description for the mentioned two approaches:

**Static Mode**: In this approach certain actions are performed regardless of whether or not one or multiple pins should be driven to a specific state. in specific:

When the system exits the sleep state `app_gpio_set_active` is called and the following operations are executed:

 * `PD_COM` is enabled (if not handled transparently by SDK).
 * GPIO pins of interest are configured and driven to their ON state.
 * Associated pads are un-locked/latched so their current state is reflected on the output.

The state of the previously configured pins can now be driven by accessing the low level driver API.

When the system is about to enter the sleep state `app_gpio_set_inactive` is called and the following operations are executed:

 * GPIO pins of interest are configured and driven to their OFF state.
 * Associated pads are locked/un-latched so their current state is retained while in the sleep state.
 * `PD_COM` is disabled (if not handled transparently by SDK).

For this to work, user callback functions should be registered to the power manager (PM) by calling `pm_register_adapter`. In doing so, the running application can get notified upon certain events (e.g. exiting sleep, entering sleep, canceling sleep)

**Dynamic Mode**: The following actions take place every a time one or multiple GPIO pins should be driven to a specific state by calling either `app_gpio_set_active`or `app_gpio_set_inactive`. In specific:

 * `PD_COM` is enabled (if not handled transparently by SDK)

 * GPIO pins are configured and driven to a specific state (ON/OFF)

 * Associated pads are enabled and then locked so their state is retained when the device enters

    the sleep state.

 * `PD_COM` is disabled (if not handled transparently by SDK).

For demonstration purposes, transparent handling of `PD_COM` has been disabled by setting `dg_configPM_ENABLES_PD_COM_WHILE_ACTIVE` to zero.

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
- Compile the code and load it into the chip.
- Press the reset button on the daughterboard to start executing the application.

- LED1 should start blinking @1Hz.

## Known Limitations

There are no known limitations for this sample code.
