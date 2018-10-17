## Breakout SDK code structure

This page documents the internal structure of the Breakout SDK for Arduino.

  ### Folder structure
| Folder | Description |
|--|--|
| src/ | The root directory contains a single header file with some basic board parameters. This header must stay here as it is required by the Arduino IDE. |
| BreakoutSDK/ |  Location of the Twilio Breakout SDK. |
| CLI/ | Command Line Interface helper. This implements a simple human interface over the USB serial port which allows for a direct testing of the internal functionality. The exported commands mirror the internal API structures. The CLI can be used to manually test if a sequence of operations work. It is expected to be disabled for production so that the USB serial is available for use by your application. |
| CoAP/ | A small and simple implementation of a CoAP messages, codec and transport reliability. |
| DTLS/ | Wrapper functionality around the `tinydtls` library. |
| modem/ | AT commands wrapper and interface with the U-Blox Sara-N410 cellular module. |
| utils/ | Various data structures and basic operations to help accelerate development. |
| tinydtls/ | The [`tinydtls`](https://projects.eclipse.org/projects/iot.tinydtls) library with minimal modifications for Arduino and a few bug fixes. | 

### Headers
| File | Description |
|--|--|
| `board.h` | Short header to define the parameters of the board as required by the Arduino SDK. |
| BreakoutSDK.h | The main header to include in your Twilio Breakout client device application. If this is not included, the Arduino builders will consider that the library is not used and other includes will fail |

> Note: [`OwlModem`](BreakoutSDK/modem/OwlModem.h) has the capability to power on/off all modules.