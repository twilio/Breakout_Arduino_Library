#include <board.h>
#include <BreakoutSDK.h>
// Install https://github.com/Seeed-Studio/Grove_Ultrasonic_Ranger
#include <Ultrasonic.h>

#define ULTRASONIC_PIN  (38)
#define INTERVAL        (100)

Ultrasonic UltrasonicRanger(ULTRASONIC_PIN);

void setup()
{
  SerialUSB.begin(9600);
}

void loop()
{
  long distance;
  distance = UltrasonicRanger.MeasureInCentimeters();
  SerialUSB.println(distance);
  
  delay(INTERVAL);
}
