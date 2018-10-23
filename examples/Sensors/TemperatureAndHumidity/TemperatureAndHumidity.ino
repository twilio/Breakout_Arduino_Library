#include <DHT.h>

#include <board.h>
#include <BreakoutSDK.h>
#include <stdio.h>
//https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor
#include <DHT.h>

/** Change this to your device purpose */
static const char *device_purpose = "Dev-Kit";
/** Change this to your key for the SIM card inserted in this device 
 *  You can find your PSK under the Breakout SDK tab of your Narrowband SIM detail at
 *  https://www.twilio.com/console/wireless/sims
*/
static const char *psk_key = "00112233445566778899aabbccddeeff";

/** This is the Breakout SDK top API */
Breakout *breakout = &Breakout::getInstance();

#define SENSOR_PIN (D38)
#define INTERVAL   (10000)
#define DHTTYPE DHT11   // DHT 11 

DHT dht(SENSOR_PIN, DHTTYPE);

void setup() {
  dht.begin();
  // Feel free to change the log verbosity. E.g. from most critical to most verbose:
  //   - errors: L_ALERT, L_CRIT, L_ERR, L_ISSUE
  //   - warnings: L_WARN, L_NOTICE
  //   - information & debug: L_INFO, L_DB, L_DBG, L_MEM
  // When logging, the additional L_CLI level ensure that the output will always be visible, no matter the set level.
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
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  LOG(L_INFO, "Current temperature [%f] degrees celcius\r\n", temperature);
  LOG(L_INFO, "Current humidity [%f]\r\n", humidity);
  char commandText[512];
  snprintf(commandText, 512, "Current humidity [%4.2f] and current temp [%4.2f]", humidity, temperature);
  sendCommand(commandText);

  breakout->spin();

  delay(INTERVAL);
}
