# Twilio Breakout SDK for Arduino

This page documents how to get started using the Breakout SDK and what it provides. Today, the Breakout SDK is built for the STM32F405RG MCU and U-Blox Sara-N410 cellular module in mind. This specific Developer Board was provided in Twilio's Alfa Developer Kit and distributed to [SIGNAL 2018](https://www.twilio.com/signal) attendees, and it came with Grove sensors – humidity, light, and ultrasonic.

## Breakout SDK

Breakout SDK enables developers to exchange data between low-powered devices and IoT cloud services. Breakout makes it easy to adopt low power optimizations provided by the Narrowband network and cellular modem hardware.

#### Retrieve PSK from Console assign PSK in client
A pre shared key (PSK) is required to be retrieved from the Programmable Wireless Console Narrowband SIM Resource. Your Narrowband SIM PSK will need to be copied into your application code and can be regenerated anytime through the Console.

PSKs are retrieved and set in hexadecimal and are unique per SIM.
```
static const char * psk_key = "00112233445566778899aabbccddeeff";
```

#### Retrieve the Breakout instance
Obtain an instance to the Breakout client singleton.
Returns breakout client
```
Breakout *breakout = &Breakout::getInstance();
```

#### Set log level
Owl log provides robust output and color coding to Serial output. 
Log verbosity from most critical to most verbose:
* errors
	* `L_ALERT`,` L_CRIT`, `L_ERR`, `L_ISSUE`
* warnings
	* `L_WARN`, `L_NOTICE`
* information & debug
	* `L_INFO`, `L_DB`, `L_DBG`, `L_MEM`
```
 owl_log_set_level(L_DBG);
```
When logging, the additional L_CLI level ensure that the output will always be visible, no matter the set level.

#### Set PSK in Breakout
Set the Breakout SDK parameters.
Psk value is 16 bytes in hex, or 32 characters.
```
breakout->setPSKKey(psk_key);
```

#### Set Polling Interval
Sets the interval (seconds) at which to poll for new Commands from the server.
The default is 0, which indicates no automatic querying of the server. Value is specified in seconds, values < 60 will result in no polling. If last polling was far away in the past, a poll will follow. Otherwise (e.g. change of interval), the timer for the next poll is simply set to last_polling_time + interval_seconds.
```
breakout->setPollingInterval(10 * 60);
```
#### Power on the cellular module
Powering the modem and starting up the SDK.
Returns `true` if powered on, `false` otherwise.
```
 breakout->powerModuleOn();
```
 
#### Handle all modem events
This is the main loop(), which will run forever. Keep  `breakout->spin()` call such that the SDK will be able to handle modem events, incoming data, and trigger retransmissions.
```
void loop() {
	 // Add here the code for your application, but don't block
	 // Add in this loop calls to your own application functions. 
	 // Don't block or sleep inside them.
	 your_application_example();

	 // The Breakout SDK checking things and doing the work
	 breakout->spin();
	 // The delay (sleep) here helps conserve power, hence it is advisable to keep it.
	 delay(50);
 }
```
### Sending and receiving Commands
 
 #### commands_status_code_e
 Enumeration for connection status updates:
  * `COMMAND_STATUS_OK`
	  * Returned if operation was successful
  * `COMMAND_STATUS_ERROR`
	  * Returned if operation failed
  * `COMMAND_STATUS_BUFFER_TOO_SMALL`
	  * Returned if provided buffer is too small for requested data
  * `COMMAND_STATUS_NO_COMMAND_WAITING`
	  * Returned if no command is available for reading
  * `COMMAND_STATUS_COMMAND_TOO_LONG`
	  * Returned if provided command is too long

####  Send a Command without a receipt request
The Command to send to Twilio - max 140 characters.

* cmd - the command to send to Twilio - max 140 characters.
* isBinary - whether the command should be sent with Content-Format indicating text or binary.
```
command_status_code_e sendCommand(char *const buf, isBinary=false);
```
#### Send a Command with a receipt request
The Command to send to Twilio - max 140 characters.
* buf - buffer containing the binary command to send to Twilio - max 140 characters
* bufSize - number of bytes of the binary command
* callback - command receipt callback.
* callback_parameter - a generic pointer to application data
```
command_status_code_e sendCommandWithReceiptRequest(char *const buf, BreakoutCommandReceiptCallback_f callback, void *callback_parameter);
```

#### Receive Commands
Pop a received Command, in case it was received locally (hasWaitingCommand() returns `true`.
   * maxBufSize - size of buffer being passed in
   * buf - buffer to receive command into
   * bufSize - size of returned command in buf, will not exceed 141 bytes.
```
command_status_code_e receiveCommand(const size_t maxBufSiz, char  *const buf, size_t  *bufSize);
```
#### Commands waiting to be retrieved
Indicates the presence of a waiting command.
Returns `true` if Commands waiting on server, `false` otherwise.
```
bool hasWaitingCommand();
```

## Alfa Developer Kit
Alfa Developer Kit contains:

- Programmable Wireless Narrowband SIM
- Alfa Developer Board
- LTE antenna
- GPS antenna
- 3 Grove sensors:
    1. Button
    2. Ultrasonic
    3. Temperature/Humidity
- Lithium battery
- Micro-USB cable

## Requirements
The following items are required to use Breakout SDK:

- Alfa Development Kit
-  [Arduino IDE 1.8.7+](https://www.arduino.cc/en/Main/Software)
- dfu-util

## Installing the Narrowband SIM and the LTE Antenna into the Developer Board

1.  [Register the Narrowband SIM](https://www.twilio.com/console/wireless/sims/register) in the Programmable Wireless Console
2. Insert the **smallest** form factor of the Narrowband SIM in the **bottom** of the **two** slots available on the Developer Board
  ![Narrowband SIM holder](img/alfa-developer-board-sim-holder.png)
> The smallest form factor of a SIM is the Nano (4FF) SIM size. The top slot is a Micro-SD card slot. If you insert the Narrowband SIM in the top slow, gently pry it out with needle-nose pliers.

3. Connect the LTE Antenna to the LTE pin located on the **bottom** of the board.

## Set up your development environment

The following steps will guide you from downloading the Arduino IDE to installing sample applications on your Developer board.

### Arduino IDE Installation

1. Download [Arduino IDE 1.8.7](https://www.arduino.cc/en/Main/Software)

### dfu-util Installation
##### OSX

The following step is required for OSX:

1. Install Homebrew by typing the following in a Terminal:
``` /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" ```
2. Install dfu-util using Homebrew by typing the following in a Terminal:
``` brew install dfu-util libusb```

> **Note:** Use dfu-util 0.9 or greater if available. Check dfu-util version with ```brew info dfu-util```

##### Windows

The follow steps are required for Windows:
1.  [Install USB Drivers](http://wiki.seeedstudio.com/Wio_LTE_Cat_M1_NB-IoT_Tracker/#install-usb-driver)

### Arduino IDE Developer Board Installation

1. Insert the Micro-USB cable into the Developer Board
2. Insert the other end of the USB cable in your computer
3. Open Arduino IDE
4. Click Arduino > Preferences
5. Copy the following URL into the Additional Boards Manager URLs field:
```https://raw.githubusercontent.com/Seeed-Studio/Seeed_Platform/master/package_seeeduino_boards_index.json```
6. Click OK
7. Click Tools > Boards > Boards Manager
8. Type "Seeed" into the search field
9. Select the Seeed STM32F4 Boards version 1.2.3+
> See [#2 in known limitations and workaround](#limitations-and-workarounds) if you are not using version 1.2.3 or greater.
10. Click Install
11. Close the Boards Manager window
12. Click Tools > Boards > Wio Tracker LTE
13. Click Tools > Port > **{Your Modem Port Here}**
    * OSX: /dev/{cu|tty}.usbmodem{XXXX}
    * Linux: /dev/ttyACM{X}
    * Windows: COM{X}

### Breakout SDK Installation

1. Click the green "Clone or download" button at the top right handside of this repository
2. Click the [Download as ZIP file](#) button
3. Make note of the download location
4. Open Arduino IDE
5. Select Sketch > Include Library > Add .ZIP Library and select the .zip file downloaded
6. Restart Arduino IDE

### Updating Breakout SDK on your local machine

The library will now be present for Arduino IDE to use. To update the library:

1. Delete the library from your Arduino directory
    * You can find the library to delete in your ```Arduino/libraries``` directory
    * OSX: in ~/Documents/Arduino/libraries
2. Follow the steps in the [Breakout SDK Installation](#Breakout-SDK-Installation) section above

>  **Tip:** An alternative to downloading the library as a ZIP is to check the library out using ```git``` in the Arduino/libraries directory, or symlink the locally-checked out copy there.

## Flash the Developer Board with sample applications

1. Open Arduino IDE
2. Click File > examples and navigate to the Breakout library examples
3. Select an example from File > Examples > Breakout SDK > Sensors
4. Enable Bootloader mode on the Developer Board: 
	1. Press and hold the **BOOT0** button underneath the Developer Board
	2. Press and hold the **RST** on the top of the Developer Board
	3. Release the **RST** on the top of the Developer Board
	4. Release the **BOOT0** button to enable Bootloader mode
5. Click Sketch > Upload to upload the example to the Developer Board
6. Click the RST button when the sketch has finished uploading
7. Click Tools > Port > **{Your Modem Port Here}**
    * OSX: /dev/{cu|tty}.usbmodem{XXXX}
    * Linux: /dev/ttyACM{X}
    * Windows: COM{X}
8. Click Tools > Serial Monitor
9. Monitor the output of the board in the Serial Monitor window

> When the board is in Bootloader mode the serial port won't be initialized and not available to monitor.

Receiving the output ```WARNING: Category 'Device' in library Wio LTE Arduino Library is not valid. Setting to 'Uncategorized'``` See known #4 under [Limitations and Workarounds](#Limitations-and-Workarounds) below.

## Developer Board LEDs

The LEDs on the Developer Board are set to function as the following:
- Red CHG LED - Lights up based on the battery charging level. 
- Yellow Status LED - lights up when the modem module is power on.
- Blue Network LED - lights up when the modem module is successfully registered to the mobile NB-IoT network.
- Red RST LED - lights up during the reset procedure. To place the module in firmware flashing mode, press the BOOT0 switch on the board when this LED lights up.
- WS2812 RGB LED - available for your application and is used in the briefly in the examples to indicate the status of the Breakout SDK.

> Note that the lithium battery is recommended to be plugged in at all times, especially if your USB power source does not provide sufficient power for the board at peak levels.

## Limitations and Workarounds

1. U-Blox data size limitation
    *  **Problem:** The U-Blox cellular module can receive up to 512 bytes of data in hex mode. Hence this is a limitation for UDP datagrams.
    *  **Solution:** partial solution would be to switch from the hex mode to binary mode. This shall double the amount of data received, yet it makes the code much more complex. So far 512 was a good upper limit, especially given the NB-IoT targets here, hence this wasn't explored.
2. Arduino Seeed STM32F4 Boards Package - USART buffer limitation
    *  **Problem:** The Seeed STM32F4 Boards package, published through the Boards Manager system, has a limitation in the USART interface implementation which we're using to communicate with the modem. The default USART_RX_BUF_SIZE is set to 256 bytes. With overheads and hex mode, this reduces the maximum chunk of data which we can read from the modem quite significantly. The effect is that the modem data in the receive side is shifted over its beginning, such that only the last 255 bytes are actually received by OwlModem.
    *  **Solution:** see [Update USART buffer below](#Update0USART-buffer).
 3. Compilation errors that ```<arduino.h>``` is not found
    *  **Problem:** Case-insensitive filesystems.
    *  **Solution:** Edit the files in ```/.arduino15/packages/Seeeduino/hardware/Seeed_STM32F4/{{VersionHere}}/``` which do not compile. Replace with ```<Arduino.h>``` and fix similar case issues until you get your system to compile the hardware package.
3. Receiving the output ```WARNING: Category 'Device' in library Wio LTE Arduino Library is not valid. Setting to 'Uncategorized'``` in Arduino IDE.
    *  **Problem:** Incorrect version of dfu-util. Using 0.8 or lower.
    *  **Solution:** Update dfu-util to 0.9+
       * OSX: Type the following in a Terminal: ``brew install dfu-util``
 4. Unable to remove lithium battery from Developer Board.
    *  **Problem:** JST pings lock lithium battery into place.
    *  **Solution:**  If the battery is pushed in a touch too far, it locks. Lift the pins from the JST connector and pull on the lithium battery cable. The JST connector has tabs that dig in and are not meant to be disconnected again.

### Update USART buffer

This is **required** for Wio STM32F4 devices not on 1.2.3.
1. Locate ``/libmaple/usart.h``
    * OSX:  ``/Users/{{UserNameHere}}/Library/Arduino15/packages/Seeeduino/hardware/Seeed_STM32F4/{{VersionHere}}/cores/arduino/libmaple/usart.h``
    * Linux:
    ``~/.arduino15/packages/Seeeduino/hardware/Seeed_STM32F4/{{VersionHere}}/cores/arduino/libmaple/usart.h``
    * Windows:  ``C:/Users/{{UserNameHere}}/Documents/Arduino/hardware/Seeed_STM32F4/{{VersionHere}}/cores/arduino/libmaple/usart.h``
2. Update ``USART_RX_BUF_SIZE`` to ``1280``
3. Update ``USART_TX_BUF_SIZE`` to ``1280``