# Quadrature Decoder (QDEC) Sample Code

This sample code demonstrates using the quadrature decoder peripheral supported by the DA1459x family of devices. The decoder can decode three pairs of axes as illustrated below reporting step count and direction. The decoder can forward a triggering signal to the PDC block upon movement detection on any of the three axes. Thus, exiting the device from the sleep state.

<img src="assets\QDEC_block_diagram.PNG" style="zoom: 67%;" />

For this example, the [KNOB G Click](https://www.mikroe.com/knob-g-click) rotary encoder is demonstrated and is connected to the X axis. The rest of the axes are disabled and left unconnected. The axis selection is arbitrary and developers are free to employ any of the supported axes. Each pair of axes can be routed to a list of pre-defined I/O pins and should automatically be handled by the QDEC upon enabling them. For more info on the integrated decoder block please refer to the [datasheet](https://www.renesas.com/us/en/products/wireless-connectivity/bluetooth-low-energy/da14592-016fdevkt-p-smartbond-da14592-bluetooth-low-energy-52-soc-development-kit-pro#documents). The following picture depicts the I/O connection between the rotary encoder module and the DA14592 PRO DevKit. Although the motherboard exhibits a mikroBUS connector which can be used to directly connect to click boards, such a topology is not feasible as the supported QDEC I/O pins are not routed to the mikroBUS connector. As such, a few female-female jumper wires should be used for the connection.

 <img src="assets\KNOB_G_CLICK.PNG" style="zoom:80%;" />

To facilitate users, all I/O pins definitions are done through pre-processor directives which can be found in the `knob_G_click_config` configuration file. Users are free to change any of the default values by re-defining the corresponding macro(s) in the customer's configuration file, that should be `customer_config_xxx.h`. The following picture illustrates the file tree of the demonstrated example (apart from the `main` and `platform_devices` source files which can be found in every sample code that demonstrates using a peripheral block). The demonstration example exhibits all the capabilities of the rotary encoder module which can be enabled/disable per demand via the corresponding pre-processor directives as defined in `knob_G_click_config`. By default, all features should be enabled.

<img src="assets\file_tree.PNG" style="zoom: 25%;" />



1. The `knob_G_click_key` folder contains all the functionality required to support the push button by employing the WKUP controller. The main application task can get notifications on KEY events, reported by the WKUP, by registering a callback function that is called from with the WKUP ISR. To do so, the `knob_G_click_key_register_cb` API is called once and the `app_kbob_G_click_key_callback` application-defined callback function is called upon KEY events. In this example, the registered callback toggles the triggering polarity and OSAL notifications are used to further process the press and release events. The main functionality of the push button is to change the LEDs state.
2. The `knob_G_click_led` folder contains all the functionality required to support the LED driver IC (PCA9956B), which drivers the 24-segment LED ring mounted on the underlying module. Please note that the current implementation supports only the basic functionality that includes resetting, turning on/off an individual or all LEDs as well as controlling the brightness of a LED. Providing full drivers for the [PCA9956B](https://download.mikroe.com/documents/datasheets/PCA9956B.pdf) IC is out of the scope of the demonstration example. Developer are free to provide more functionality by updating the `knob_G_click_led.x` files. Please note that the drivers implementation has been structured in such a way so that the underlying I2C controller is totally agnostic to the PCA9956B driver source files. This is achieved by using function pointers defined in the porting layer implemented in the `knob_G_click_led_port.c` source file. The DA14592 SoC integrated a single I2C block instance which can operate either in master or slave role on the I2C bus. The `knonb_G_click_port.c` source file exhibits the `app_knob_G_click_led_update` routine which is invoked upon QDEC events reported by the QDEC controller (that should occur when the rotary is rotated in either direction). This routine handles the state of the LEDs according to the current state selected by the push button. Currently three LED states are supported as follows:
   - `KEY STATE 0`: The brightness on all LEDs is increased/decreased by moving the rotary counterclockwise and clockwise respectively.
   - `KEY STATE 1`: A single LED is turned ON at a time reflecting the current position of the encoder. 
   - `KEY STATE 2`: LEDs should be turned ON one-by-one as the rotary is rotated counterclockwise and be turned OFF one-by-one as the rotary is rotated clockwise.
3. This files contains all the I/O pins selection along with pre-processor directives that can enable/disable the KEY and LED features per demand and thus, saving code space.
4.  This file should contain the main application task which registers the user-defined WKUP callback function and initialized the LED driver.

## HW and SW Configuration

  - **Hardware Configuration**
    - This example runs on DA1459x family of devices.
    - A Pro development kit is needed for this example.
  - **Software Configuration**
    - Download the latest SDK version for the target family of devices.

    - SEGGER J-Link tools are normally downloaded and installed as part of the e2 Studio installation.

## How to run the example

### Initial Setup

- Download the source code from the Support Website.
- Import the project into your workspace (there should be no path dependencies).
- Connect the target device to your host PC via USB1. The mentioned port is used to power the device as well as to support debugging (JTAG) and serial functionality (UART).
- Compile the code (either in Release or Debug mode) and load it into the chip.
- Open a serial terminal (115200/8 - N - 1)
- Press the reset button on the daughter board to start executing the application. 
- Use the serial console to interact with the device. 
- Rotate the rotary in either direction to increase/decrease the brightness on all LEDs.
- Press the push button once to move to the next KEY state.  Rotate the rotary in either direction to move a single LED reflecting the current rotary position.
- Press the push button to move to the last supported KEY state which should turn ON/OFF the LEDs one-by-one reflecting the current rotary position. 
- Pressing the push button once more will reset the KEY state to the initial state. 

## Known Limitations

The mikroBUS connector of the PRO development kit cannot route any of the supported QDEC signals and thus the demonstrated rotary encoder module should be connected via jumper wires. 
