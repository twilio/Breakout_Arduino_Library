#include <board.h>
#include <BreakoutSDK.h>
#include <WioLTE_Cat_NB1/ublox_sara_r4_gnss.h>


UBLOX_SARA_R4_GNSS gnss = UBLOX_SARA_R4_GNSS();

void setup()  
{
  // Open GNSS module
  gnss.open_GNSS();
  delay(3000);
  SerialDebugPort.println("_Start");
}

void loop() {
  gnss.dataFlowMode();
}
