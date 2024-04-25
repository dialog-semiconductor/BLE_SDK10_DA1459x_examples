# HW Timers Demonstration Example

This application demonstrates using HW timers. The DA1459x family of devices incorporates four identical HW timer blocks. Each block consists of a counter and PWM sub-block which can operate independently. Capture events are also available for all four instances. In this demonstration, the first block instance (HW_TIMER) is configured so that PWM pulses of 1kHz are generated on P0.12. The PWM port can be changed via `APP_TIMER_PWM_PORT` and `APP_TIMER_PWM_PIN`. Please note that the selected pin can operate even when the device has entered the extended sleep state. Input capture events are also demonstrated using P0.10. The latter can be changed via `APP_TIMER_CAPTURE_PORT`.  The counter block is configured in down counting mode which in turn should blink LED1 @10Hz. Counter's expiration frequency can be adjusted via `APP_TIMER_COUNTER_RELOAD_HZ`.   

## HW and SW Configuration

  - **Hardware Configuration**
    - This example runs on DA14692 family of devices.
    - A Pro development kit is needed for this example.
  - **Software Configuration**
    - Download the latest SDK version for the target family of devices.

    - SEGGER J-Link tools are normally downloaded and installed as part of the e2 Studio installation.

## How to Run the Example

### Initial Setup

- Download the source code from the Support Website.

- Import the project into your workspace (there should be no path dependencies).

- Connect the target device to your host PC via USB1. The mentioned port is used to power the device via VBAT and also to support debugging functionality (UART and JTAG).

- With a help of a jumper wire shortcut P1.1 (LED1) to P0.10 (input capture).

- Compile the code and load it into the chip.
- Open a serial terminal (115200/8 - N - 1).
- Press the reset button on the daughterboard to start executing the application. 
- LED1, mounted on the daughterboard, should start blinking @10Hz as defined by `APP_TIMER_COUNTER_RELOAD_HZ`.
- PWM output can be captured on P0.12 with the help of a logic analyzer.

## Known Limitations

There are no known limitations for this application.
