/*
 * OwlModem.cpp
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
 * \file OwlModem.cpp - a more elaborate, yet still simple modem interface.
 */

#include "OwlModem.h"

#include <stdarg.h>
#include <stdio.h>

#include "OwlModemSIM.h"



OwlModem::OwlModem(HardwareSerial *modem_port_in, USBSerial *debug_port_in, HardwareSerial *gnss_port_in)
    : modem_port(modem_port_in),
      debug_port(debug_port_in),
      gnss_port(gnss_port_in),
      AT(&modem_port),
      information(&AT),
      SIM(&AT),
      network(&AT),
      pdn(&AT),
      socket(&AT) {
  modem_port_in->begin(SerialModule_Baudrate);
  if (!modem_port_in) {
    LOG(L_ERR, "OwlModem initialized without modem port. That is not going to work\r\n");
  }

  if (gnss_port_in) {
    gnss_port_in->begin(SerialGNSS_BAUDRATE);
  }

  if (debug_port_in) {
    debug_port_in->enableBlockingTx();
  }
  // Seed the random
  pinMode(ANALOG_RND_PIN, INPUT);
  randomSeed(analogRead(ANALOG_RND_PIN));

  has_modem_port = (modem_port_in != nullptr);
  has_debug_port = (debug_port_in != nullptr);
  has_gnss_port  = (gnss_port_in != nullptr);
}

OwlModem::~OwlModem() {
}


int OwlModem::powerOn(owl_power_m bit_mask) {
  if (!has_modem_port) {
    return false;
  }

  /*
   * This code is mostly taken from the Seeed ublox_sara_r4.cpp, then simplified. For a different board, this has to be
   * re-implemented.
   */
  int pwr_status = 1;
  int errCnt     = 0;

  if ((bit_mask & Owl_PowerOnOff__Modem) != 0) {
    if (isPoweredOn()) return 1;

    // Set RTS pin down to enable UART communication
    pinMode(RTS_PIN, OUTPUT);
    digitalWrite(RTS_PIN, LOW);

    pinMode(MODULE_PWR_PIN, OUTPUT);
    digitalWrite(MODULE_PWR_PIN, HIGH);  // Module Power Default HIGH

    /* Is this just for the modem? or also the rest? */
    pinMode(PWR_KEY_PIN, OUTPUT);
    digitalWrite(PWR_KEY_PIN, LOW);
    digitalWrite(PWR_KEY_PIN, HIGH);
    delay(800);
    digitalWrite(PWR_KEY_PIN, LOW);

    owl_time_t timeout = owl_time() + 10 * 1000;
    while (!isPoweredOn()) {
      if (owl_time() > timeout) {
        LOG(L_ERR, "Timed-out waiting for modem to power on\r\n");
        return 0;
      }
      delay(50);
    }
  }

  if ((bit_mask & Owl_PowerOnOff__Grove) != 0) {
    pinMode(GROVE_PWR_PIN, OUTPUT);
    digitalWrite(GROVE_PWR_PIN, HIGH);  // VCC_B Enable pin
  }

  if ((bit_mask & Owl_PowerOnOff__RGBLED) != 0) {
    pinMode(RGB_LED_PWR_PIN, OUTPUT);
    digitalWrite(RGB_LED_PWR_PIN, HIGH);
  }

  if ((bit_mask & Owl_PowerOnOff__GNSS) != 0) {
    pinMode(GNSS_PWR_PIN, OUTPUT);
    digitalWrite(GNSS_PWR_PIN, HIGH);
  }

  return 1;
}

int OwlModem::powerOff(owl_power_m bit_mask) {
  if ((bit_mask & Owl_PowerOnOff__Modem) != 0) {
    pinMode(MODULE_PWR_PIN, OUTPUT);
    digitalWrite(MODULE_PWR_PIN, HIGH);  // Module Power Default HIGH
  }

  if ((bit_mask & Owl_PowerOnOff__Grove) != 0) {
    pinMode(GROVE_PWR_PIN, OUTPUT);
    digitalWrite(GROVE_PWR_PIN, LOW);  // VCC_B Enable pin
  }

  if ((bit_mask & Owl_PowerOnOff__RGBLED) != 0) {
    pinMode(RGB_LED_PWR_PIN, OUTPUT);
    digitalWrite(RGB_LED_PWR_PIN, LOW);
  }

  if ((bit_mask & Owl_PowerOnOff__GNSS) != 0) {
    pinMode(GNSS_PWR_PIN, OUTPUT);
    digitalWrite(GNSS_PWR_PIN, LOW);
  }
  return 1;
}

int OwlModem::isPoweredOn() {
  return AT.doCommandBlocking("AT", 1000, 0, 0) == AT_Result_Code__OK;
}


/**
 * Handler for PIN, used during initialization
 * @param message
 */
void initCheckPIN(str message) {
  if (!str_equal_prefix_char(message, "READY")) {
    LOG(L_ERR,
        "PIN status [%.*s] != READY and PIN handler not set. Please disable the SIM card PIN, or set a handler.\r\n",
        message.len, message.s);
  }
}



int OwlModem::initModem(int testing_variant) {
  at_result_code_e rc;
  OwlModem_PINHandler_f saved_handler = 0;
  at_umnoprof_mno_profile_e current_profile;
  at_umnoprof_mno_profile_e expected_profile = (testing_variant & Testing__Set_MNO_Profile_to_Default) == 0 ?
                                                   AT_UMNOPROF__MNO_PROFILE__TMO :
                                                   AT_UMNOPROF__MNO_PROFILE__SW_Default;

  if (!AT.initTerminal()) {
    return 0;
  }

#if defined(COPS_OVERRIDE_LONG) || defined(COPS_OVERRIDE_SHORT) || defined(COPS_OVERRIDE_NUMERIC)
  // deregister from the network before the modem hangs
  if (!network.setOperatorSelection(AT_COPS__Mode__Deregister_from_Network, nullptr, nullptr, nullptr)) {
    LOG(L_ERR, "Potential deregistering from network\r\n");
  }
#endif

  /* Resetting the modem network parameters */
  if (!network.getModemMNOProfile(&current_profile)) {
    LOG(L_ERR, "Error retrieving current MNO Profile\r\n");
    return 0;
  }

  if (current_profile != expected_profile || (testing_variant & Testing__Set_APN_Bands_to_Berlin) != 0 ||
      (testing_variant & Testing__Set_APN_Bands_to_US) != 0) {
    /* A modem reset is required */
    if (!network.setModemFunctionality(AT_CFUN__FUN__Minimum_Functionality, 0))
      LOG(L_WARN, "Error turning modem off\r\n");

    if (current_profile != expected_profile) {
      LOG(L_WARN, "Updating MNO Profile to %d - %s\r\n", expected_profile,
          at_umnoprof_mno_profile_text(expected_profile));
      if (!network.setModemMNOProfile(expected_profile)) {
        LOG(L_ERR, "Error re-setting MNO Profile from %d to %d\r\n", current_profile, expected_profile);
        return 0;
      }
    }

    if ((testing_variant & Testing__Set_APN_Bands_to_Berlin) != 0) {
      if (AT.doCommandBlocking("AT+URAT=8", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting RAT to NB1\r\n");
      // This is a bitmask, LSB meaning Band1, MSB meaning Band64
      if (AT.doCommandBlocking("AT+UBANDMASK=0,0", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting band mask for Cat-M1 to none\r\n");
      if (AT.doCommandBlocking("AT+UBANDMASK=1,168761503", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting band mask for NB1 to 168761503 (manual default\r\n");
      //
      //      if (AT.doCommandBlocking("AT+UBANDMASK=1,524416", 5000, 0, 0) != AT_Result_Code__OK)
      //        LOG(L_WARN, "Error setting band mask for NB1 to Band8 (900MHz) and Band20 (800MHz)\r\n");
      if (AT.doCommandBlocking("AT+CGDCONT=1,\"IP\",\"" TESTING_APN "\"", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting custom APN\r\n");
    }
    if ((testing_variant & Testing__Set_APN_Bands_to_US) != 0) {
      if (AT.doCommandBlocking("AT+URAT=8", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting RAT to NB1\r\n");
      // This is a bitmask, LSB meaning Band1, MSB meaning Band64
      if (AT.doCommandBlocking("AT+UBANDMASK=0,0", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting band mask for Cat-M1 to 2/4/5/12\r\n");
      if (AT.doCommandBlocking("AT+UBANDMASK=1,2074", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting band mask for NB1 to 2/4/5/12 (manual default)\r\n");
      // if (AT.doCommandBlocking("AT+CGDCONT=1,\"IP\",\"" TESTING_APN "\"", 5000, 0, 0) != AT_Result_Code__OK)
      // LOG(L_WARN, "Error setting custom APN\r\n");
    }

    if (!network.setModemFunctionality(AT_CFUN__FUN__Modem_Silent_Reset__No_SIM_Reset, 0))
      LOG(L_WARN, "Error resetting modem\r\n");
    // wait for the modem to come back
    while (!isPoweredOn()) {
      LOG(L_INFO, "..  - waiting for modem to power back on after reset\r\n");
      delay(100);
    }

    if (!AT.initTerminal()) {
      return 0;
    }

#if defined(COPS_OVERRIDE_LONG) || defined(COPS_OVERRIDE_SHORT) || defined(COPS_OVERRIDE_NUMERIC)
    // deregister from the network before the modem hangs
    if (!network.setOperatorSelection(AT_COPS__Mode__Deregister_from_Network, nullptr, nullptr, nullptr)) {
      LOG(L_ERR, "Potential deregistering from network\r\n");
    }
#endif
  }


#if defined(COPS_OVERRIDE_LONG)
  at_cops_format_e cops_format = AT_COPS__Format__Long_Alphanumeric;
  at_cops_act_e cops_act       = AT_COPS__Access_Technology__LTE_NB_S1;
  str oper                     = STRDECL(COPS_OVERRIDE_LONG);

  LOG(L_INFO, "Selecting network operator \"%s\", it can take a while.\r\n", COPS_OVERRIDE_LONG);
  if (!network.setOperatorSelection(AT_COPS__Mode__Manual_Selection, &cops_format, &oper, &cops_act)) {
    LOG(L_ERR, "Error selecting mobile operator\r\n");
    return 0;
  }
#elif defined(COPS_OVERRIDE_SHORT)
  at_cops_format_e cops_format = AT_COPS__Format__Short_Alphanumeric;
  at_cops_act_e cops_act       = AT_COPS__Access_Technology__LTE_NB_S1;
  str oper                     = STRDECL(COPS_OVERRIDE_SHORT);

  LOG(L_INFO, "Selecting network operator \"%s\", it can take a while.\r\n", COPS_OVERRIDE_SHORT);
  if (!network.setOperatorSelection(AT_COPS__Mode__Manual_Selection, &cops_format, &oper, &cops_act)) {
    LOG(L_ERR, "Error selecting mobile operator\r\n");
    return 0;
  }
#elif defined(COPS_OVERRIDE_NUMERIC)
  at_cops_format_e cops_format = AT_COPS__Format__Numeric;
  at_cops_act_e cops_act       = AT_COPS__Access_Technology__LTE_NB_S1;
  str oper                     = STRDECL(COPS_OVERRIDE_NUMERIC);

  LOG(L_INFO, "Selecting network operator \"%s\", it can take a while.\r\n", COPS_OVERRIDE_NUMERIC);
  if (!network.setOperatorSelection(AT_COPS__Mode__Manual_Selection, &cops_format, &oper, &cops_act)) {
    LOG(L_ERR, "Error selecting mobile operator\r\n");
    return 0;
  }
#endif


  if (AT.doCommandBlocking("AT+CSCS=\"GSM\"", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Potential error setting character set to GSM\r\n");
  }

  /* Set the on-board LEDs */
  if (AT.doCommandBlocking("AT+UGPIOC=23,10", 5000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "..  - failed to map pin 23 (yellow led)  to \"module operating status indication\"\r\n");
  }
  if (AT.doCommandBlocking("AT+UGPIOC=16,2", 5000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "..  - failed to map pin 16 (blue led) to \"network status indication\"\r\n");
  }

  /* TODO - decide if to keep this in */
  if (AT.doCommandBlocking("AT+CREG=2", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN,
        "Potential error setting URC to Registration and Location Updates for Network Registration Status events\r\n");
  }
  if (AT.doCommandBlocking("AT+CGREG=2", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN,
        "Potential error setting GPRS URC to Registration and Location Updates for Network Registration Status "
        "events\r\n");
  }
  if (AT.doCommandBlocking("AT+CEREG=2", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN,
        "Potential error setting EPS URC to Registration and Location Updates for Network Registration Status "
        "events\r\n");
  }

  if (SIM.handler_cpin) saved_handler = SIM.handler_cpin;
  SIM.setHandlerPIN(initCheckPIN);
  if (AT.doCommandBlocking("AT+CPIN?", 5000, &response, MODEM_RESPONSE_BUFFER_SIZE) != AT_Result_Code__OK) {
    LOG(L_WARN, "Error checking PIN status\r\n");
  }
  SIM.setHandlerPIN(saved_handler);

  if (AT.doCommandBlocking("AT+UDCONF=1,1", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Potential error setting ublox HEX mode for socket ops send/receive\r\n");
  }

  /* TODO: probably deprecated*/
  if (!information.getModel(&response, MODEM_RESPONSE_BUFFER_SIZE)) {
    LOG(L_WARN, "Potential error caching the modem model\r\n");
  }

  LOG(L_DBG, "Modem correctly initialized\r\n");
  return 1;
}

int OwlModem::waitForNetworkRegistration(char *purpose, int testing_variant) {
  bool network_ready = false;
  owl_time_t timeout = owl_time() + 30 * 1000;
  while (true) {
    at_cereg_stat_e stat;
    if (network.getEPSRegistrationStatus(0, &stat, 0, 0, 0, 0, 0)) {
      network_ready = (stat == AT_CEREG__Stat__Registered_Home_Network || stat == AT_CEREG__Stat__Registered_Roaming);
      if (network_ready) break;
    }
    if ((testing_variant & Testing__Timeout_Network_Registration_30_Sec) != 0 && owl_time() > timeout) {
      LOG(L_ERR, "Bailing out from network registration - for testing purposes only\r\n");
      return 0;
    }
    LOG(L_NOTICE, ".. waiting for network registration\r\n");
    delay(2000);
  }

  if ((testing_variant & Testing__Skip_Set_Host_Device_Information) != 0) return 1;

  if (!setHostDeviceInformation(purpose)) {
    LOG(L_WARN, "Error setting HostDeviceInformation.  If this persists, please inform Twilio support.\r\n");
    // TODO: set a flag to report to Twilio Object-16 registration timed out or failed
  }

  return 1;
}



static str s_exitbypass = {.s = "exitbypass", .len = 10};

void OwlModem::bypassCLI() {
  if (!has_modem_port || !has_debug_port) {
    return;
  }

  // TODO - set echo on/off - maybe with parameter to this function? but that will mess with other code
  in_bypass = 1;
  uint8_t c;
  int index = 0;
  AT.pause();
  while (1) {
    if (modem_port.available()) {
      modem_port.read(&c, 1);
      debug_port.write(&c, 1);
    }
    if (debug_port.available()) {
      debug_port.read(&c, 1);
      modem_port.write(&c, 1);
      if (s_exitbypass.s[index] == c)
        index++;
      else
        index = 0;
      if (index == s_exitbypass.len) {
        modem_port.write((uint8_t *)"\r\n", 2);
        in_bypass = 0;
        AT.resume();
        return;
      }
    }
  }
}

void OwlModem::bypassGNSSCLI() {
  if (!has_gnss_port || !has_debug_port) {
    return;
  }
  // TODO - set echo on/off - maybe with parameter to this function? but that will mess with other code
  uint8_t c;
  int index = 0;
  while (1) {
    if (gnss_port.available()) {
      gnss_port.read(&c, 1);
      debug_port.write(&c, 1);
    }
    if (debug_port.available()) {
      debug_port.read(&c, 1);
      gnss_port.write(&c, 1);
      if (s_exitbypass.s[index] == c)
        index++;
      else
        index = 0;
      if (index == s_exitbypass.len) {
        gnss_port.write((uint8_t *)"\r\n", 2);
        return;
      }
    }
  }
}

void OwlModem::bypass() {
  if (!has_modem_port || !has_debug_port) {
    return;
  }

  uint8_t c;
  while (modem_port.available()) {
    modem_port.read(&c, 1);
    debug_port.write(&c, 1);
  }
  while (debug_port.available())
    debug_port.read(&c, 1);
  modem_port.write(&c, 1);
}

void OwlModem::bypassGNSS() {
  if (!has_gnss_port || !has_debug_port) {
    return;
  }

  uint8_t c;
  while (gnss_port.available())
    gnss_port.read(&c, 1);
  debug_port.write(&c, 1);
  while (debug_port.available())
    debug_port.read(&c, 1);
  gnss_port.write(&c, 1);
}

static str s_dev_kit = STRDECL("devkit");

int OwlModem::setHostDeviceInformation(char *purpose) {
  str s_purpose;
  if (purpose) {
    s_purpose.s   = purpose;
    s_purpose.len = strlen(purpose);
  } else {
    s_purpose = s_dev_kit;
  }

  return setHostDeviceInformation(s_purpose);
}

void OwlModem::computeHostDeviceInformation(str purpose) {
  char *hostDeviceID      = "Twilio-Alfa";
  char *hostDeviceIDShort = "alfa";
  char *board_name        = "WioLTE-Cat-NB1";
  char *sdk_ver           = "0.1.0";

  // Param 2: Twilio_Seeed_(AT+CGMI -> u-blox) // OwlModemInformation::getManufacturer()
  char module_mfgr_buffer[64];
  str module_mfgr = {.s = module_mfgr_buffer, .len = 0};
  information.getManufacturer(&module_mfgr, 64);

  // Param 3: Wio-LTE-Cat-NB1_(AT+CGMM -> SARA-N410-02B) // OwlModemInformation::getModel()
  char module_model_buffer[64];
  str module_model = {.s = module_model_buffer, .len = 0};
  information.getModel(&module_model, 64);

  // Param 4: twilio-v0.9_u-blox-v7.4 (AT+CGMR -> L0.0.00.00.07.04 [May 25 2018 15:05:31]) //
  // OwlModemInformation::getVersion()
  char module_ver_buffer[64];
  str module_ver = {.s = module_ver_buffer, .len = 0};
  information.getVersion(&module_ver, 64);
  // TODO: trim module_ver down to just important bit?

  hostdevice_information.len =
      snprintf(hostdevice_information.s, MODEM_HOSTDEVICE_INFORMATION_SIZE,
               "\"%s_%.*s\",\"Twilio_%.*s\",\"%s_%.*s\",\"twilio-v%s_%.*s-v%.*s\"", hostDeviceID, purpose.len,
               purpose.s, module_mfgr.len, module_mfgr.s, board_name, module_model.len, module_model.s, sdk_ver,
               module_mfgr.len, module_mfgr.s, module_ver.len, module_ver.s);

  /* v<sdk_version>/<hostDeviceID> */
  short_hostdevice_information.len =
      snprintf(short_hostdevice_information.s, MODEM_HOSTDEVICE_INFORMATION_SIZE, "v%s/%s", sdk_ver, hostDeviceIDShort);
}

int OwlModem::setHostDeviceInformation(str purpose) {
  bool registered = false;

  if (!purpose.len) purpose = s_dev_kit;
  computeHostDeviceInformation(purpose);

  char command_buffer[MODEM_HOSTDEVICE_INFORMATION_SIZE + 24];
  snprintf(command_buffer, MODEM_HOSTDEVICE_INFORMATION_SIZE + 24, "AT+UHOSTDEV=%.*s", hostdevice_information.len,
           hostdevice_information.s);
  LOG(L_INFO, "Setting HostDeviceInformation to: %.*s\r\n", hostdevice_information.len, hostdevice_information.s);

  for (int attempts = 10; attempts > 0; attempts--) {
    if (AT.doCommandBlocking(command_buffer, 1000, &response, MODEM_RESPONSE_BUFFER_SIZE) == AT_Result_Code__OK) {
      LOG(L_INFO, ".. setting HostDeviceInformation successful.\r\n");
      registered = true;
      break;
    }
    if (attempts > 0) {
      LOG(L_INFO, ".. setting HostDeviceInformation failed - will retry after a short delay\r\n");
      delay(7000);
    }
  }
  if (!registered) {
    LOG(L_ERR, "Setting HostDeviceInformation failed.\r\n");
    return 0;
  }

  return 1;
}

str OwlModem::getHostDeviceInformation() {
  if (!hostdevice_information.len) computeHostDeviceInformation(s_dev_kit);
  return hostdevice_information;
}

str OwlModem::getShortHostDeviceInformation() {
  if (!short_hostdevice_information.len) computeHostDeviceInformation(s_dev_kit);
  return short_hostdevice_information;
}


int OwlModem::drainGNSSRx(str *gnss_buffer, int gnss_buffer_len) {
  if (gnss_buffer == nullptr || !has_gnss_port) {
    return 0;
  }

  LOG(L_MEM, "Trying to drain GNSS data\r\n");
  int available, received, total = 0, full = 0;
  while ((available = gnss_port.available()) > 0) {
    if (available > gnss_buffer_len) available = gnss_buffer_len;
    //    LOG(L_DBG, "Available %d bytes\r\n", available);
    if (available > gnss_buffer_len - gnss_buffer->len) {
      int shift = available - (gnss_buffer_len - gnss_buffer->len);
      LOG(L_WARN, "GNSS buffer full with %d bytes. Dropping oldest %d bytes.\r\n", gnss_buffer->len, shift);
      gnss_buffer->len -= shift;
      memmove(gnss_buffer->s, gnss_buffer->s + shift, gnss_buffer->len);
      full = 1;
    }
    received = gnss_port.read((uint8_t *)gnss_buffer->s + gnss_buffer->len, available);
    //    LOG(L_WARN, "Rx %d bytes\r\n", received);
    if (received != available) {
      LOG(L_ERR, "gnss_port said %d bytes available, but received %d.\r\n", available, received);
      if (received < 0) goto error;
    }

    gnss_buffer->len += received;
    total += received;

    if (gnss_buffer->len > gnss_buffer_len) {
      LOG(L_ERR, "Bug in the gnss_buffer_len calculation %d > %d\r\n", gnss_buffer->len, gnss_buffer_len);
      goto error;
    }

    LOG(L_DBG, "GNSS Rx - size changed from %d to %d bytes\r\n", gnss_buffer->len - received, gnss_buffer->len);
    LOGSTR(L_DBG, *gnss_buffer);
    if (full) return total;
  }
error:
  LOG(L_MEM, "Done draining GNSS %d\r\n", total);
  return total;
}

/*void OwlModem::setDebugLevel(int level) {
  owl_log_set_level(level);
}*/
