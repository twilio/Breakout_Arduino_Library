/*
 * ATBypass.ino
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
 * \file ATBypass.ino - Direct AT access to cellular module for diagnostics.
 */

#include <board.h>

#include <BreakoutSDK/modem/OwlModem.h>

OwlModem *owlModem = 0;

void setup() {
  SerialDebugPort.enableSmartBlockingTx(10000);  // reliably write to it

  owl_log_set_level(L_INFO);
  LOG(L_INFO, "Arduino setup() starting up\r\n");

  owlModem    = new OwlModem(&SerialModule, &SerialDebugPort);

  LOG(L_INFO, ".. WioLTE Cat.NB-IoT - powering on modules\r\n");
  if (!owlModem->powerOn()) {
    LOG(L_ERROR, ".. WioLTE Cat.NB-IoT - ... modem failed to power on\r\n");
    return;
  }
  LOG(L_INFO, ".. WioLTE Cat.NB-IoT - now powered on.\r\n");

  LOG(L_INFO, "Entering AT bypass\r\n");
  owlModem->bypassCLI();

  LOG(L_INFO, "Arduino setup() done\r\n");
  LOG(L_INFO, "Arduino loop() starting\r\n");
  return;
}

void loop() {
  owlModem->spin();

  delay(50);
}
