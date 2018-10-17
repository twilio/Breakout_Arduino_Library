/*
 * BreakoutSDK.ino
 * Twilio Breakout SDK
 *
 * Copyright (c) 2018 Twilio, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * \file BreakoutSDK.ino - Default BreakoutSDK example
 */

#include <Seeed_ws2812.h>

#include <BreakoutSDK.h>



/** Change this to your device purpose */
static const char *device_purpose = "Dev-Kit";
/** Change this to your key for the SIM card inserted in this device 
 *  You can find your PSK under the Breakout SDK tab of your Narrowband SIM detail at
 *  https://www.twilio.com/console/wireless/sims
*/
static const char *psk_key = "00112233445566778899aabbccddeeff";



/** This is the Breakout SDK top API */
Breakout *breakout = &Breakout::getInstance();

/** Just an added bonus of a yellow LED, turning green once the registration and connection is done */
WS2812 strip = WS2812(1, RGB_LED_PIN);

void enableLed() {
  pinMode(RGB_LED_PWR_PIN, OUTPUT);
  digitalWrite(RGB_LED_PWR_PIN, HIGH);
  strip.begin();
  strip.brightness = 5;
}



/**
 * Setting up the Arduino platform. This is executed once, at reset.
 */
void setup() {
  // Feel free to change the log verbosity. E.g. from most critical to most verbose:
  //   - errors:   L_ERR
  //   - warnings: L_WARN
  //   - information: L_INFO
  //   - debug: L_DBG
  //
  // When logging, the additional L_CLI level ensure that the output will always be visible, no matter the set level.
  owl_log_set_level(L_DBG);
  LOG(L_WARN, "Arduino setup() starting up\r\n");

  enableLed();
  // Set RGB-LED to yellow
  strip.WS2812SetRGB(0, 0x20, 0x20, 0x00);
  strip.WS2812Send();

  // Set the Breakout SDK parameters
  breakout->setPurpose(device_purpose);
  breakout->setPSKKey(psk_key);
  breakout->setPollingInterval(10 * 60);  // Optional, by default set to 10 minutes

  // Powering the modem and starting up the SDK
  LOG(L_WARN, "Powering on module and registering...");
  breakout->powerModuleOn();

  char test[] = "BreakoutSDK test app";
  sendCommand(test, sizeof(test));
    
  // Set RGB-LED to green
  strip.WS2812SetRGB(0, 0x00, 0x40, 0x00);
  strip.WS2812Send();

  LOG(L_WARN, "... done powering on and registering.\r\n");
  LOG(L_WARN, "Arduino loop() starting up\r\n");
}

void sendCommand(const char * command, size_t command_len) {
  if (breakout->sendTextCommand(command) == COMMAND_STATUS_OK) {
    LOG(L_INFO, "Tx-Command [%.*s]\r\n", command_len, command);
  } else {
    LOG(L_INFO, "Tx-Command ERROR\r\n");
  }
}

/**
 * This is just a simple example of a poll for commands application. See the documentation for
 * many more options and examples.
 */
void your_application_example() {
  // This is a simple example, which drains the waiting commands and echoes them back to the server
  if (breakout->hasWaitingCommand()) {
    // Check if there is a command waiting and log it on the USB serial
    char command[140];
    size_t commandLen = 0;
    bool isBinary     = false;
    // Read a command
    command_status_code_e code = breakout->receiveCommand(140, command, &commandLen, &isBinary);
    switch (code) {
      case COMMAND_STATUS_OK:
        LOG(L_INFO, "Rx-Command [%.*s]\r\n", commandLen, command);
        break;
      case COMMAND_STATUS_ERROR:
        LOG(L_INFO, "Rx-Command ERROR\r\n");
        break;
      case COMMAND_STATUS_BUFFER_TOO_SMALL:
        LOG(L_INFO, "Rx-Command BUFFER_TOO_SMALL\r\n");
        break;
      case COMMAND_STATUS_NO_COMMAND_WAITING:
        LOG(L_INFO, "Rx-Command NO_COMMAND_WAITING\r\n");
        break;
      default:
        LOG(L_INFO, "Rx-Command ERROR %d\r\n", code);
    }
  }
}

/**
 * This is the main loop, which will run forever. Keep the breakout->spin() call, such that the SDK
 * will be able to handle modem events, incoming data, trigger retransmissions and so on.
 *
 * Add in this loop calls to your own application functions. But don't block or sleep inside them.
 *
 * The delay (sleep) here helps conserve power, hence it is advisable to keep it.
 */
void loop() {
  // Add here the code for your application, but don't block
  your_application_example();

  // The Breakout SDK checking things and doing the work
  breakout->spin();

  delay(50);
}
