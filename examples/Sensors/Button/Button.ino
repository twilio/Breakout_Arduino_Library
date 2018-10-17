#include <board.h>
#include <BreakoutSDK.h>

// Use D38 Grove port
#define BUTTON_PIN  38
#define INTERVAL    (100)

void setup()
{
  pinMode(38, INPUT);
}

void loop()
{
  int buttonState = digitalRead(BUTTON_PIN);
  SerialUSB.print(buttonState ? '*' : '.');
  
  delay(INTERVAL);
}
