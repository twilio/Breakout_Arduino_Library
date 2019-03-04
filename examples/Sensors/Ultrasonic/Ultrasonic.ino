#include <board.h>
#include <BreakoutSDK.h>
// Install https://github.com/Seeed-Studio/Grove_Ultrasonic_Ranger
#include <Ultrasonic.h>

/** Change this to your device purpose */
static const char *device_purpose = "Dev-Kit";
/** Change this to your key for the SIM card inserted in this device 
 *  You can find your PSK under the Breakout SDK tab of your Narrowband SIM detail at
 *  https://www.twilio.com/console/wireless/sims
*/
static const char *psk_key = "00112233445566778899aabbccddeeff";

/** This is the Breakout SDK top API */
Breakout *breakout = &Breakout::getInstance();

#define ULTRASONIC_PIN  (38)
#define INTERVAL        (1000)

Ultrasonic UltrasonicRanger(ULTRASONIC_PIN);

/**
 * Setting up the Arduino platform. This is executed once, at reset.
 */
void setup() {

  pinMode(38, INPUT);
  
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
  long distance;
  distance = UltrasonicRanger.MeasureInCentimeters();
  if (distance < 10){
    char message[] = "Object is within 10cm of sensor";
    sendCommand(message);
  } else {
    LOG(L_INFO, "Object is [%ld] CM from the sensor\r\n", distance );
  }
  
  breakout->spin();
  
  delay(INTERVAL);
}
