#include <board.h>
#include <BreakoutSDK.h>
#include <stdio.h>

/** Change this to your device purpose */
static const char *device_purpose = "Dev-Kit";
/** Change this to your key for the SIM card inserted in this device 
 *  You can find your PSK under the Breakout SDK tab of your Narrowband SIM detail at
 *  https://www.twilio.com/console/wireless/sims
 */
static const char *psk_key = "00112233445566778899aabbccddeeff";

/** This is the Breakout SDK top API */
Breakout *breakout = &Breakout::getInstance();

#define LOOP_INTERVAL (1 * 1000)
#define SEND_INTERVAL (10 * 60 * 1000)

void setup() {
  // Feel free to change the log verbosity. E.g. from most critical to most verbose:
  //   - errors:   L_ERROR
  //   - warnings: L_WARN
  //   - information: L_INFO
  //   - debug: L_DEBUG
  //
  // When logging, the additional L_BYPASS level ensure that the output will always be visible, no matter the set level.
  owl_log_set_level(L_INFO);
  LOG(L_WARN, "Arduino setup() starting up\r\n");

  // Set the Breakout SDK parameters
  breakout->setPurpose(device_purpose);
  breakout->setPSKKey(psk_key);
  breakout->setPollingInterval(10 * 60);  // Optional, by default set to 10 minutes

  // Powering the modem and starting up the SDK
  LOG(L_WARN, "Powering on module and registering...");
  breakout->powerModuleOn();

  LOG(L_WARN, "... done powering on and registering.\r\n");
  LOG(L_WARN, "Arduino loop() starting up\r\n");
}

/**
 * This is just a simple example to send a command and write out the status to the console.
 */

void sendCommand(const char * command) {
  if (breakout->sendTextCommand(command) == COMMAND_STATUS_OK) {
    LOG(L_INFO, "Tx-Command [%s]\r\n", command);
  } else {
    LOG(L_INFO, "Tx-Command ERROR\r\n");
  }
}

void loop()
{
  static unsigned long last_send = 0;

  gnss_data_t data;
  breakout->getGNSSData(&data);

  if (data.valid && ((last_send == 0) || (millis() - last_send >= SEND_INTERVAL))) {
    last_send = millis();

    if (data.valid) {
      char commandText[512];
      snprintf(commandText, 512, "Current Position:  %d %7.5f %s  %d %7.5f %s\r\n", data.position.latitude_degrees,
          data.position.latitude_minutes, data.position.is_north ? "N" : "S", data.position.longitude_degrees,
          data.position.longitude_minutes, data.position.is_west ? "W" : "E");
      sendCommand(commandText);
    }
  }

  breakout->spin();

  delay(LOOP_INTERVAL);
}
