# Hibernation Modes Demonstration Example

This sample code demonstrates using the hibernation modes supported by the DA1459x family of devices. In specific, the purpose of this example is to illustrate the usage of the deep sleep and hibernation modes. The latter is also known as the shipping mode. Both of the mentioned sleep modes are not intended for typical sleep operations meaning that upon entering in either of the two states the device is not able to execute any source code.  Following are the supported shipping modes that should enter the device into the deepest sleep state. In specific:

**Hibernation**: No RAM is retained, no clocks are running (so no RTC), all domains are off, except for PD_AON, and the system can only be woken up by POR or a hibernation wake-up trigger that can only happen on dedicated GPIOs.

**Deep Sleep:** This should be considered as the lite hibernation mode in which the device can also wake up by synchronous events. In specific, no RAM is retained, RCX is running (so RTC is on), all domains, except for PD_AON and PD_SLP, are off, and the system can only be woken up by POR, RTC alarm, watchdog reset, or a GPIO trigger (reset on wake-up). 

**It is imperative to highlight that in the deep sleep state the device might wake-up by the watchdog counter even if the selected wake up event has yet to take place. This is because the internal low power oscillators, that should be RCX and RC32/512 kHz, cannot be turned off during the deep sleep state. However, these clocks are used to clock the RTC counters which should trigger the synchronous wake-up events. RCX should be the default low power clock and the maximum watchdog counter value, that is 2^13, should be used which should result in a watchdog expiration after 3 minutes.** 

Since the purpose of this sample code is to demonstrate the sources that can wake up the device from the deeper sleep states, the concepts of the alarm events (RTC controller) and the Power-on-Reset (POR) through GPIOs are also exhibited. The demonstration example makes use of the command line interface (CLI) that comes with the SDK. That means that users can configure the various sleep modes as well as wake-up sources at runtime. Following is the file tree of the CLI implementation on application level:

<img src="assets\file_tree.png" style="zoom: 33%;" />

1. These files include the necessary code required to employ the command line interface module as well as preprocessor macros that can be used to disable SW components that are not required. In doing so, users can select the features of interest and thus saving code space. All features are enabled by default. To change any value, the corresponding macro should be re-defined in customer's configuration file, i.e. `custom_config_eflash.h`. 

   ```
   #ifndef APP_CLI_DEEP_SLEEP_EN
   #define APP_CLI_DEEP_SLEEP_EN   ( 1 )
   #endif
   
   #ifndef APP_CLI_HIBERNATION_EN
   #define APP_CLI_HIBERNATION_EN  ( 1 )
   #endif
   
   #ifndef APP_CLI_POR_GPIO_EN
   #define APP_CLI_POR_GPIO_EN     ( 1 )
   #endif
   
   #ifndef APP_CLI_RTC_EN
   #define APP_CLI_RTC_EN          ( 1 )
   #endif
   ```

The CLI module employs the UART2 serial peripheral instance witch supports HW flow control through the  CTS and RTS control lines. In specific, the CTS control line is used to determine when the serial console is in use. As long as the CTS line is active low (asserted) the UART block is enabled and the device is only 		     allowed to enter the idle state (ARM WFI state). Once the CTS line is active high (de-asserted) the UART instance is disabled and the device is allowed to enter the normal sleep state, that should be the extended sleep state. It's programmers responsibility to setup the wakeup controller and setup the CTS line as input active low wakeup source. Upon CTS detection, application should notify the CLI task by calling a dedicated API so that CLI gets notified and UART2 is re-enabled. All these operations are handled in `cli_app.c`. The current implementation supports registering a callback function to be called upon event detection on P0 where the CTS line is connected to. In doing so, the application should get notified so that a pending hibernation or deep sleep mode request is applied.  Following is the registered user callback that inverts the polarity of the CTS port line. 

```
static void app_gpio0_cb(uint32_t status)
{
#if dg_configUSE_CONSOLE
        if (status & (1 << SER1_CTS_PIN)) {
                HW_WKUP_TRIGGER trigger;

                /* Invert trigger logic */
                trigger = ((hw_wkup_get_trigger(SER1_CTS_PORT, SER1_CTS_PIN) == HW_WKUP_TRIG_EDGE_LO) ?
                        HW_WKUP_TRIG_EDGE_HI : HW_WKUP_TRIG_EDGE_LO);
                hw_wkup_set_trigger(SER1_CTS_PORT, SER1_CTS_PIN, trigger);
    
                OS_TASK_NOTIFY_FROM_ISR(app_task_h, trigger == HW_WKUP_TRIG_EDGE_LO ?
                                APP_WKUP_CTS_N_INACTIVE_NOTIF :
                                APP_WKUP_CTS_N_ACTIVE_NOTIF, OS_NOTIFY_SET_BITS);
        }

#endif
}
```

Keep in mind that the wake-up input source can be either active low or active high at any given time. If both polarities should be supported, this should be handled manually by programmers (as illustrated above).

2. These files include the CLI handlers as well as the necessary code required to enter/exit the device into/from the deep sleep state. Defining a wakeup event typically involves registering one or multiple events to the Power Domain Controller (PDC) and setting up the corresponding input sources to the GPIO controller. PDC is a block that is used to register events upon which a master (typically the application processor) should exit the current sleep state. It is important to highlight that during a sleep state the application processor is completely turned off (clock-less sleep mode) and so a block which consume lesser power is used instead (the PDC in our case). Keep in mind that the events that are used to wakeup the device from the hibernation modes can also be used to exit the device from the extended sleep state which is the default sleep state upon normal code execution. Following is a short description of the supported commands:

   - To get the ports that are configured as wake-up input sources enter `deep_sleep_get_info` with no arguments. Please not that multiple ports can be setup as wake-up input sources.

   - To select a GPIO pin that should act as input wake-up source enter `deep_sleep_sel_port <port> <pin> <pol>` where:

     - `<port>` should be either `p0` or `p1`. Invalid values should be ignored.
     - `<pin>` should be a valid pin number of the selected port and should not exceed `[0..15]`. Invalid value should be ignored. 
     - `<pol>` should be either `active_low` or `<active_high>`. Please not that in case of active low, the internal pull-up resistor is employed and so there is no need for users to add one externally. 

     Apart from setting up the GPIO controller, a PDC entry is also registered for each pin selection. In case PDC is running out of entries, the procedure should abort and a log message should be displayed on the serial console. Keep in mind that he PDC block supports up to 12 entries from which few of them are reserved by SDK. It important to mention that the default state of an I/O pin is the pull down. 

   - To reset a previously defined wake-up input source enter `deep_sleep_reset_port <port> <pin>`. If the selected pin is not already defined as wake-up input source a log message should be displayed on the serial console and the procedure should abort. 

   - To select the status of the deep sleep state enter `deep_sleep_set_status <status>` where:

     - `<status>` is either `enable` or `disable`. Invalid values should be ignored. 

     It is worth mentioning that enabling the deep sleep state does not mean that the power manager will enter the device into the requested state immediately. The reason is that the power manager defines a priority scheme in which a task/process can select a sleep mode of higher priority and thus preventing lower sleep modes from being applied, if requested afterwards. Only when that higher-priority sleep mode is released explicitly by the task/process, a sleep mode of a lower priority can be applied. In this context, CLI module employs the UART peripheral adapter which in turn requests that the device is only allowed to enter the idle state (ARM WFI state) as long as the console is in use. The latter is determined by monitoring the CTS control line of the UART controller. When the CTS control line is de-asserted the UART adapter is closed and the previously requested idle state is released. 

3. These files include the CLI handlers as well as the necessary code required to configure the sub-block of the wakeup controller that is responsible to enter/exit the device into/from the hibernation state. The hibernation circuitry supports a list of dedicated pins than can be used as active-low or active-high wake-up sources. Following is a short description of the supported commands:

   - To get the current hibernation wake-up event enter `hibernation_get_info` with no arguments.

   - To select the GPIO pin that should be able to exit the device from hibernation enter `hibernation_sel_port <port> <pol>` where:

     - `<port>` a hibernation wake-up source from a pre-defined list. In specific:
     - `0` should disable any hibernation wake-up input source
       - `1` should select `P0.14`
       - `2` should select `P1.04`
       - `3` should select both `P0.14` and `P1.04`
     
   - `<pol>`the trigger polarity of the selected hibernation wake-up source. In specific:
       - `0` to select active high for both `P0.14` and `P1.04`
     - `1` to select active low for `P0.14`
       - `2`to select active low for `P1.04`
       - `3` to select active low for both `P0.14` and `P1.04`
       
     
   
   To select the status of the hibernation state enter `hibernation_set_status <status>` where:
   
   - `<status>` is either `enabled` or `disable`. Invalid values should be ignored. 
     
   
   As explained in the deep sleep mode section, enabling the hibernation state does not mean that the power manager will enter the device into the requested state immediately. 
   
4. These files include the CLI handlers as well as the necessary code required to configure the GPIO Power-On-Reset (POR) sub-block, part of the reset controller. Only one I/O pin can be configured to trigger POR events at any given time with configurable polarity and timeout expiration. The latter imposes that the event be present for a certain amount of time before the power-on-reset event takes place. In case the event lasts less than the the selected timeout the POR event should not happen. The counter, integrated into the POR circuitry, is clocked by the selected RCLP clock which can be either 32KHz or 512KHz. Following is a short description of the supported commands:

   - To get the current POR event state enter `por_gpio_get_info` with no arguments. 

   - To set the timeout event enter `por_gpio_set_timeout <raw_timeout>`where:

     - `<raw_timeout>` should not exceed [0..0x7F]. In fact, the raw value represents multiple of 4096 counts of RCLP and so the actual timeout event is calculated based on the following equation: `<raw_timeout> * 4096 * RCLP period`. A zero value should disable the GPIO POR events.

   - To select the GPIO pin that should be able to generate POR events along with the trigger polarity enter `por_gpio_sel_port <port> <pin> <pol>` where:

     - `<port>` should be either `p0` or `p1`. Please note that invalid values should de-configure the currently selected port.
     - `<pin>` should be a valid pin number of the selected port and should not exceed `[0..15]`. Invalid value should de-configure the currently selected port.	 
     - `<pol>` should be either `active_low` or `<active_high>`. 

     Keep in mind that the Reset pad is always capable of generating a power-on-reset event regardless of whether the GPIO POR circuitry is enabled or not.

5. These files include the CLI handlers as well as the necessary code required to configure and interact with the real time clock controller. Following is a short description of the supported commands:

   - To get the current time and calendar enter `rtc_time_get` with no arguments. 
   - To get the next alarm event enter `rtc_alarm_get` with no arguments.
   - To set the current time and calendar enter `rtc_time_set` `<yyyy-mm-dd>T<hh:mm:ss>`
   - To set the current time only enter `rtc_time_set <hh:mm:ss>`
   - To set the current calendar only enter `rtc_time_set <yyyy-mm-dd>`
   - To set the next alarm event including time and calendar fields enter `rtc_alarm_set` `<yyyy-mm-dd>T<hh:mm:ss>`
   - To set the next alarm event including the time fields only enter `rtc_alarm_set <hh:mm:ss>`
   - To set the next alarm event including the calendar fields only enter `rtc_alarm_set <yyyy-mm-dd>.`

   In this demonstration example, the RTC controller should be initiated upon device boot and with the help of pre-processors macros, defined in `cli_rtc.h`, that should reflect the initial timer and calendar fields values. Changing one or multiple fields values is done by re-declaring  the associated pre-processor macros in customer's configuration file e.g. `custom_config_eflash.h`. Keep in mind that RTC should be able to retain its time and calendar counter values upon a SW or HW reset event. As mentioned at the very beginning, the deep sleep mode supports exiting from synchronous wake-up sources. In this context, programmers can setup an alarm event so the device can exit the deep sleep state in a specific time in the future. When the selected alarm counters (time and/or calendar fields) match the corresponding RTC counters an alarm event should take place. The current implementation supports registering a callback function to be called upon alarm events. The register callback should merely send a notification to the user's application task which in turn should print a log message on the serial console. 

   ```
   #if APP_CLI_RTC_EN
   static void app_rtc_alarm_cb(void)
   {
           if (in_interrupt()) {
                   OS_TASK_NOTIFY_FROM_ISR(app_task_h, APP_RTC_ALARM_EVT_NOTIF, OS_NOTIFY_SET_BITS);
           } else {
                   OS_TASK_NOTIFY(app_task_h, APP_RTC_ALARM_EVT_NOTIF, OS_NOTIFY_SET_BITS);
           }
   }
   #endif 
   ```


## HW and SW Configuration

  - **Hardware Configuration**
    - This example runs on DA1459x family of devices.
    - A Pro development kit (DevKit) is needed for this example.
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
- Use the serial console to interact with the device. The corresponding messages should be displayed so users are aware of the progress of the requested command.
- Entering the deep sleep state can be as simple as hitting `deep_sleep_sel_port`

##### Using the K1 push button as wake-up source

This section shortly demonstrates how the user push button, mounted on the Pro DevKit, can be used to exit the device from the deep sleep mode. Similarly, the latter can be used as a general wake-up or POR input source. Please note that K1 is routed to P0.10 port and no external pull-up resistor is present. 

1. By default, P0.10 should be connected to pull down and so its internal pull up resistor should be enabled by hitting  `deep_sleep_sel_port p0 10 active_low`. 

2. Indicate that the deep sleep state is allowed by hitting `deep_sleep_set_status enable`. Please note switching to the mentioned mode should not take place immediately as the UART adapter should already be opened and thus forcing the device to the idle power state. Hit `deep_sleep_get_info` to verify that the deep sleep mode flag is enabled. 

3. Close the serial console window so that the CTS line is de-asserted. This should close the UART peripheral and release the requested idle state. Once the CTS line toggles, the application task should get notified and in turn should request from the Power Manager subsystem to switch to the deep sleep mode. 

4. Press the K1 push button to exit the device from the deep sleep state. The D1 blue led on the power measurement module should toggle once. 

   **Please note that, apart from the K1 push button, the CTS line can also exit the device from the deep sleep state since there should be in place a PDC entry for that specific line. Therefore, if the console is re-opened, and before pressing the push button, the D1 blue led should toggle once.** 

##### Using the RTC alarm as wake-up source

This section shortly demonstrates how an alarm event can exit the device from the deep sleep state.

1. Indicate that the deep sleep state is allowed by hitting `deep_sleep_set_status enable`. Please note switching to the mentioned mode should not take place immediately as the UART adapter should already be opened and thus forcing the device to the idle power state. Hit `deep_sleep_get_info` to verify that the deep sleep mode flag is enabled. 
2. Get the current time by hitting `rtc_time_get`. The current date and time should be displayed on the serial console. 
3. The alarm event can reflect both data and time counters values. In this step we will only declare an alarm event based on the time fields e.g. two minutes based on the current clock by hitting `rtc_alarm_set <hh:mm:ss>`.
4. Close the serial console window so that the CTS line is de-asserted. This should close the UART peripheral and release the requested idle state. Once the CTS line toggles, the application task should get notified and in turn should request from the Power Manager subsystem to switch to the deep sleep mode. 
5. Wait for the alarm to take place. The D1 blue led on the power measurement module should toggle once.

## Known Limitations

In the deep sleep state the clock source of the watchdog (WDOG) counter cannot be gated and so the latter should keep counting even when the device has entered into deep sleep. The downside is that a cold boot (NMI event) should take place every time the WDOG counter reaches the zero value. This should not be an issue if the programmed wake-up event(s) take(s) place before WDOG expiration. Please note that the latter is clocked automatically based on the selected low power (LP) clock. That is, if the RCX oscillator is the selected LP clock, then the same internal clock should be used to clock the WDOG and thus, the maximum expiration time should be ~3 minutes. On the other hand, if the selected LP clock is the external XTAL32K crystal, the internal RC512/32KHz oscillator should be used to clock the WDOG and thus the WDOG should expire every 82 seconds. For more info on the clock and WDOG blocks please refer to the datasheet. 
