
# Twilio Breakout SDK for Arduino
This page documents how to get started using the Breakout SDK and what it provides. Today, the Breakout SDK is built for the STM32F405RG MCU and U-Blox Sara-N410 cellular module in mind. This specific Developer Board was provided in Twilio's Alfa Developer Kit and distributed to [SIGNAL 2018](https://www.twilio.com/signal) attendees, and it came with Grove sensors – humidity, light, and ultrasonic.

## Setting up your development environment
1. [Visit the Arduino IDE setup guide](IDESETUP.md) to learn how to set up your development environment.
2. [Visit the Alfa Developer Kit Quickstart](https://www.twilio.com/docs/wireless/quickstart/alfa-developer-kit) to learn how to register and activate your Narrowband SIM in the [Programmable Wireless Console](https://twilio.com/console/wireless/sims).

## Breakout SDK

Breakout SDK enables developers to exchange data between low-powered devices and IoT cloud services. Breakout makes it easy to adopt low power optimizations provided by the Narrowband network and cellular modem hardware.

#### Retrieve PSK from Console and assign PSK in client
A pre shared key (PSK) is required to be retrieved from the Programmable Wireless Console Narrowband SIM Resource. Your Narrowband SIM PSK will need to be copied into your application code.

PSKs are retrieved and set in hexadecimal and are unique per SIM.
```
static const char * psk_key = "00112233445566778899aabbccddeeff";
```
> Your PSK can be found by navigating to the [SIMs section of the Console](https://twilio.com/console/wireless/sims), click on your Narrowband SIM, Click the *Breakout SDK* tab at the top. There you will see your `ICCID` and your `PSK`.

#### Retrieve the Breakout instance
Obtain an instance to the Breakout client singleton.
* @returns breakout client instance
```
Breakout *breakout = &Breakout::getInstance();
```
#### Set log level
Owl log provides robust output and color coding to Serial output. 
Log verbosity from most critical to most verbose:
* errors
	* `L_ERR`
* warnings
	* `L_WARN`
* information 
	* `L_INFO`
* debug
        * `L_DBG`
```
 owl_log_set_level(L_DBG);
```
> When logging, the additional L_CLI level ensure that the output will always be visible, no matter the set level.

#### Set PSK in Breakout
PSK value is 16 bytes in hex or 32 characters.
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

### Sending and receiving Commands from the device

### Types
#### `commands_status_code_e`
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

#### `command_receipt_code_e`
Enumeration for command status results:
* `CONNECTION_STATUS_OFFLINE`
	* The device is not registered on the mobile network - offline.
* `CONNECTION_STATUS_NETWORK_REGISTRATION_DENIED`
	* The mobile network denied network registration - check SIM status - offline.
* `CONNECTION_STATUS_REGISTERED_NOT_CONNECTED`
	* The device is registered to the mobile network but not yet connected to the Twilio Commands SDK - offline.
* `CONNECTION_STATUS_REGISTERED_AND_CONNECTED`
	* The device is registered to the mobile network and connected to the Twilio Commands SDK - online.
#### Handler for Commands with receipt requests
Handler function signature for Commands with receipt requests. Used in `sendTextCommandWithReceiptRequest()` and `sendBinaryCommandWithReceiptRequest()`.

* @param `receipt_code` - the receipt request
* @param `cb_parameter` - a generic pointer to application data, as passed to the `sendCommandWithReceiptRequest()`
```
typedef void (*BreakoutCommandReceiptCallback_f)(command_receipt_code_e receipt_code, void *cb_parameter);
```
### Methods
####  Send a text Command without a receipt request
The Command to send to Twilio - max 140 characters.
* @param `cmd` - the command to send to Twilio - max 140 characters.
* @returns `command_status_code_e `
```
sendTextCommand(const char *buf);
```
####  Send a binary Command without a receipt request
The Command to send to Twilio - max 140 characters.
* @param `cmd` - the command to send to Twilio - max 140 characters.
* @returns `command_status_code_e `
```
sendBinaryCommand(const char *buf);
```
#### Send a text Command with a receipt request
The text Command to send to Twilio - max 140 characters.
* @param `buf` - the text command to send to Twilio - max 140 characters.
* @param `callback` - command receipt callback.
* @param `callback_parameter` - a generic pointer to application data.
* @returns: `command_status_code_e`
```
sendTextCommandWithReceiptRequest(const char *buf, BreakoutCommandReceiptCallback_f callback, void *callback_parameter);
```
#### Send a binary Command with a receipt request
The binary Command to send to Twilio - max 140 characters.
* @param `buf` - the text command to send to Twilio - max 140 characters.
* @param `callback` - command receipt callback.
* @param `callback_parameter` - a generic pointer to application data.
* @returns: `command_status_code_e`
```
sendBinaryCommandWithReceiptRequest(const char *buf, BreakoutCommandReceiptCallback_f callback, void *callback_parameter);
```
#### Receive Commands
Pop a received Command, in case it was received locally (hasWaitingCommand() returns `true`.
* @param `maxBufSize` - size of buffer being passed in.
* @param `buf` - buffer to receive command into.
* @param `bufSize` - output size of returned command in buf, will not exceed 141 bytes.
* @param `isBinary` - output indicator if the command was received with Content-Format indicating text or binary.
 * @returns `command_status_code_e`
```
receiveCommand(const  size_t maxBufSize, char  *buf, size_t  *bufSize, bool *isBinary);
```
#### Check if Commands are waiting to be retrieved
Indicates the presence of a waiting command.
@returns `true` if Commands waiting on server, `false` otherwise.
```
hasWaitingCommand();
```
#### Manually check for Commands
Manually initiate a check for waiting Commands.
If `setPollingInterval()` is set to a valid interval, this is automatically called at an interval. If both polling interval is enabled, the polling timer is reset on manual calls to this method.
* @returns `true` if the operation was successful.

Query `hasWaitingCommands()` for results.
```
checkForCommands(bool isRetry = false);
```

### Sending Commands to the SIM
Commands to the SIM are sent via an HTTP request to Twilio. Visit the [Commands API](https://www.twilio.com/docs/wireless/api/commands) documentation to learn more.

#### Create a text Command
```
curl -X POST https://wireless.twilio.com/v1/Commands \
--data-urlencode "Sim=MyNarrowbandSim" \
--data-urlencode "Command=wakeup" \
-u ACXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:your_auth_token
```

#### Create a binary Command
```
curl -X POST https://wireless.twilio.com/v1/Commands \
--data-urlencode "CommandMode=binary" \
--data-urlencode "Sim=MyNarrowbandSIM" \
--data-urlencode "Command=SGVsbG8sIE1hY2hpbmUh==" \
-u ACXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:your_auth_token
```

#### Receive Command from SIM to your servers
To receive a Command from a SIM 'mobile-originated', you should create or update an existing SIM instance with a `CommandsCallbackUrl` property, and optionally a `CommandsCallbackMethod` property (defaults to POST).

## Limitations and Workarounds
1. U-Blox data size limitation
    *  **Problem:** The U-Blox cellular module can receive up to 512 bytes of data in hex mode. Hence this is a limitation for UDP datagrams.
    *  **Solution:** partial solution would be to switch from the hex mode to binary mode. This shall double the amount of data received, yet it makes the code much more complex. So far 512 was a good upper limit, especially given the NB-IoT targets here, hence this wasn't explored.
2. Arduino Seeed STM32F4 Boards Package - USART buffer limitation
    *  **Problem:** The Seeed STM32F4 Boards package, published through the Boards Manager system, has a limitation in the USART interface implementation which we're using to communicate with the modem. The default USART_RX_BUF_SIZE is set to 256 bytes. With overheads and hex mode, this reduces the maximum chunk of data which we can read from the modem quite significantly. The effect is that the modem data in the receive side is shifted over its beginning, such that only the last 255 bytes are actually received by OwlModem.
    *  **Solution:** see [Update USART buffer below](#update-usart-buffer).

### Update USART buffer
This is **required** for Wio STM32F4 devices not on 1.2.3.
1. Locate ``/libmaple/usart.h``
    * OSX:  ``/Users/{{UserNameHere}}/Library/Arduino15/packages/Seeeduino/hardware/Seeed_STM32F4/{{VersionHere}}/cores/arduino/libmaple/usart.h``
    * Linux:
    ``~/.arduino15/packages/Seeeduino/hardware/Seeed_STM32F4/{{VersionHere}}/cores/arduino/libmaple/usart.h``
    * Windows:  ``C:/Users/{{UserNameHere}}/Documents/Arduino/hardware/Seeed_STM32F4/{{VersionHere}}/cores/arduino/libmaple/usart.h``
2. Update ``USART_RX_BUF_SIZE`` to ``1280``
3. Update ``USART_TX_BUF_SIZE`` to ``1280``