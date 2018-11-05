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



OwlModem::OwlModem(HardwareSerial *modem_port, USBSerial *debug_port, HardwareSerial *gnss_port)
    : modem_port(modem_port), debug_port(debug_port), gnss_port(gnss_port) {
  if (debug_port) debug_port->enableBlockingTx();  // reliably write to it
  // Seed the random
  pinMode(ANALOG_RND_PIN, INPUT);
  randomSeed(analogRead(ANALOG_RND_PIN));
}

OwlModem::~OwlModem() {
}


int OwlModem::powerOn(owl_power_m bit_mask) {
  if (!modem_port) return 0;

  /*
   * This code is mostly taken from the Seeed ublox_sara_r4.cpp, then simplified. For a different board, this has to be
   * re-implemented.
   */
  int pwr_status = 1;
  int errCnt     = 0;

  if ((bit_mask & Owl_PowerOnOff__Modem) != 0) {
    modem_port->begin(SerialModule_Baudrate);

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
    if (gnss_port) gnss_port->begin(SerialGNSS_BAUDRATE);
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
  return doCommand("AT", 1000, 0, 0) == AT_Result_Code__OK;
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


  if (doCommand("ATV1", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Potential error setting commands to always return response codes [%.*s]\r\n", response.len,
        response.s);
  }

  if (doCommand("ATQ0", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Potential error setting commands to return text response codes\r\n");
    goto error;
  }

  if (doCommand("ATE0", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Error setting echo off\r\n");
    goto error;
  }

  if (doCommand("AT+CMEE=2", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Potential error setting Modem Errors output to verbose (not numeric) values\r\n");
    goto error;
  }

  if (doCommand("ATS3=13", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Error setting command terminating character\r\n");
  }

  if (doCommand("ATS4=10", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Error setting response separator character\r\n");
  }

  /* Resetting the modem network parameters */
  if (!network.getModemMNOProfile(&current_profile)) {
    LOG(L_ERR, "Error retrieving current MNO Profile\r\n");
    goto error;
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
        goto error;
      }
    }

    if ((testing_variant & Testing__Set_APN_Bands_to_Berlin) != 0) {
      if (doCommand("AT+URAT=8", 5000, 0, 0) != AT_Result_Code__OK) LOG(L_WARN, "Error setting RAT to NB1\r\n");
      // This is a bitmask, LSB meaning Band1, MSB meaning Band64
      if (doCommand("AT+UBANDMASK=0,0", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting band mask for Cat-M1 to none\r\n");
      if (doCommand("AT+UBANDMASK=1,168761503", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting band mask for NB1 to 168761503 (manual default\r\n");
      //
      //      if (doCommand("AT+UBANDMASK=1,524416", 5000, 0, 0) != AT_Result_Code__OK)
      //        LOG(L_WARN, "Error setting band mask for NB1 to Band8 (900MHz) and Band20 (800MHz)\r\n");
      if (doCommand("AT+CGDCONT=1,\"IP\",\"" TESTING_APN "\"", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting custom APN\r\n");
    }
    if ((testing_variant & Testing__Set_APN_Bands_to_US) != 0) {
      if (doCommand("AT+URAT=8", 5000, 0, 0) != AT_Result_Code__OK) LOG(L_WARN, "Error setting RAT to NB1\r\n");
      // This is a bitmask, LSB meaning Band1, MSB meaning Band64
      if (doCommand("AT+UBANDMASK=0,0", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting band mask for Cat-M1 to 2/4/5/12\r\n");
      if (doCommand("AT+UBANDMASK=1,2074", 5000, 0, 0) != AT_Result_Code__OK)
        LOG(L_WARN, "Error setting band mask for NB1 to 2/4/5/12 (manual default)\r\n");
      // if (doCommand("AT+CGDCONT=1,\"IP\",\"" TESTING_APN "\"", 5000, 0, 0) != AT_Result_Code__OK)
      // LOG(L_WARN, "Error setting custom APN\r\n");
    }

    if (!network.setModemFunctionality(AT_CFUN__FUN__Modem_Silent_Reset__No_SIM_Reset, 0))
      LOG(L_WARN, "Error resetting modem\r\n");
    // wait for the modem to come back
    while (!isPoweredOn()) {
      LOG(L_INFO, "..  - waiting for modem to power back on after reset\r\n");
      delay(100);
    }

    /* Redo the basic initialization */
    if (doCommand("ATV1", 1000, 0, 0) != AT_Result_Code__OK) {
      LOG(L_WARN, "Potential error setting commands to always return response codes [%.*s]\r\n", response.len,
          response.s);
    }

    if (doCommand("ATQ0", 1000, 0, 0) != AT_Result_Code__OK) {
      LOG(L_WARN, "Potential error setting commands to return text response codes\r\n");
      goto error;
    }

    if (doCommand("ATE0", 1000, 0, 0) != AT_Result_Code__OK) {
      LOG(L_WARN, "Error setting echo off\r\n");
      goto error;
    }

    if (doCommand("AT+CMEE=2", 1000, 0, 0) != AT_Result_Code__OK) {
      LOG(L_WARN, "Potential error setting Modem Errors output to verbose (not numeric) values\r\n");
      goto error;
    }

    if (doCommand("ATS3=13", 1000, 0, 0) != AT_Result_Code__OK) {
      LOG(L_WARN, "Error setting command terminating character\r\n");
    }

    if (doCommand("ATS4=10", 1000, 0, 0) != AT_Result_Code__OK) {
      LOG(L_WARN, "Error setting response separator character\r\n");
    }
  }


  if (doCommand("AT+CSCS=\"GSM\"", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Potential error setting character set to GSM\r\n");
  }

  /* Set the on-board LEDs */
  if (doCommand("AT+UGPIOC=23,10", 5000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "..  - failed to map pin 23 (yellow led)  to \"module operating status indication\"\r\n");
  }
  if (doCommand("AT+UGPIOC=16,2", 5000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "..  - failed to map pin 16 (blue led) to \"network status indication\"\r\n");
  }

  /* TODO - decide if to keep this in */
  if (doCommand("AT+CREG=2", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN,
        "Potential error setting URC to Registration and Location Updates for Network Registration Status events\r\n");
  }
  if (doCommand("AT+CGREG=2", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN,
        "Potential error setting GPRS URC to Registration and Location Updates for Network Registration Status "
        "events\r\n");
  }
  if (doCommand("AT+CEREG=2", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN,
        "Potential error setting EPS URC to Registration and Location Updates for Network Registration Status "
        "events\r\n");
  }

  if (SIM.handler_cpin) saved_handler = SIM.handler_cpin;
  SIM.setHandlerPIN(initCheckPIN);
  if (doCommand("AT+CPIN?", 5000, &response, MODEM_RESPONSE_BUFFER_SIZE) != AT_Result_Code__OK) {
    LOG(L_WARN, "Error checking PIN status\r\n");
  }
  SIM.setHandlerPIN(saved_handler);

  if (doCommand("AT+UDCONF=1,1", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Potential error setting ublox HEX mode for socket ops send/receive\r\n");
  }

  if (!information.getModel(&response, MODEM_RESPONSE_BUFFER_SIZE)) {
    LOG(L_WARN, "Potential error caching the modem model\r\n");
  } else {
    if (str_equal_char(response, "SARA-N410-02B")) {
      LOG(L_NOTICE, "Detected the SARA-N410-02B - marking for the listen bug workaround\r\n");
      model = Owl_Modem__SARA_N410_02B__Listen_Bug;
    }
  }

  LOG(L_DBG, "Modem correctly initialized\r\n");
  return 1;
error:
  LOG(L_ERR, "Failed modem initialization\r\n");
  return 0;
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
  if (!modem_port || !debug_port) return;
  // TODO - set echo on/off - maybe with parameter to this function? but that will mess with other code
  in_bypass = 1;
  char c;
  int index = 0;
  while (1) {
    if (modem_port->available()) debug_port->write(modem_port->read());
    if (debug_port->available()) {
      c = debug_port->read();
      modem_port->write(c);
      if (s_exitbypass.s[index] == c)
        index++;
      else
        index = 0;
      if (index == s_exitbypass.len) {
        modem_port->write("\r\n", 2);
        in_bypass = 0;
        return;
      }
    }
  }
}

void OwlModem::bypassGNSSCLI() {
  if (!gnss_port || !debug_port) return;
  // TODO - set echo on/off - maybe with parameter to this function? but that will mess with other code
  char c;
  int index = 0;
  while (1) {
    if (gnss_port->available()) debug_port->write(gnss_port->read());
    if (debug_port->available()) {
      c = debug_port->read();
      gnss_port->write(c);
      if (s_exitbypass.s[index] == c)
        index++;
      else
        index = 0;
      if (index == s_exitbypass.len) {
        gnss_port->write("\r\n", 2);
        return;
      }
    }
  }
}

void OwlModem::bypass() {
  while (modem_port->available())
    debug_port->write(modem_port->read());
  while (debug_port->available())
    modem_port->write(debug_port->read());
}

void OwlModem::bypassGNSS() {
  while (gnss_port->available())
    debug_port->write(gnss_port->read());
  while (debug_port->available())
    gnss_port->write(debug_port->read());
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
    if (doCommand(command_buffer, 1000, &response, MODEM_RESPONSE_BUFFER_SIZE) == AT_Result_Code__OK) {
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



int OwlModem::sendData(str data) {
  if (!modem_port) return 0;
  int written = 0, cnt;
  do {
    cnt = modem_port->write(data.s, data.len);
    if (cnt <= 0) {
      LOG(L_ERR, "Had %d bytes to send on modem_port, but wrote only %d.\r\n", data.len, written);
      return 0;
    }
    written += cnt;
  } while (written < data.len);
  // LOG(L_INFO, "Tx\r\n");
  // LOGSTR(L_INFO, data);
  return 1;
}

int OwlModem::sendData(char *data) {
  str s = {.s = data, .len = strlen(data)};
  return sendData(s);
}


/**
 * These prefixes indicate lines which are not errors if not processed as URC, because they belong
 * to regular commands.
 */
str prefix_non_urc[] = {

    /* OwlModemInformation */
    STRDECL("+CBC"),
    STRDECL("+CIND"),

    /* OwlModemSIM */
    STRDECL("+CCID"),
    STRDECL("+CNUM"),

    /* OwlModemNetwork */
    STRDECL("+CFUN"),
    STRDECL("+UMNOPROF"),
    STRDECL("+COPS"),
    STRDECL("+CSQ"),

    /* OwlModemPDN */
    STRDECL("+CGPADDR: "),

    /* OwlModemSocket */
    STRDECL("+USOCR"),
    STRDECL("+USOER"),
    STRDECL("+USOWR"),
    STRDECL("+USOST"),
    STRDECL("+USORD"),
    STRDECL("+USORF"),
    STRDECL("+USOCO"),

    /* End of list marker */
    {0}};

int OwlModem::processURC(str line, int report_unknown) {
  if (line.len < 1 || line.s[0] != '+') return 0;
  int k = str_find_char(line, ": ");
  if (k < 0) return 0;
  str urc = {.s = line.s, .len = k};
  str data = {.s = line.s + k + 2, .len = line.len - k - 2};

  LOG(L_DBG, "URC [%.*s] Data [%.*s]\r\n", urc.len, urc.s, data.len, data.s);

  /* ordered based on expected incoming count of events */
  if (this->network.processURC(urc, data)) return 1;
  if (this->socket.processURC(urc, data)) return 1;
  if (this->SIM.processURC(urc, data)) return 1;

  if (report_unknown) {
    for (int i = 0; prefix_non_urc[i].s; i++)
      if (str_equal(urc, prefix_non_urc[i])) return 0;
    // If it wasn't on the ignored list, report it
    LOG(L_WARN, "Not handled URC [%.*s] with data [%.*s]\r\n", urc.len, urc.s, data.len, data.s);
  }
  return 0;
}

int OwlModem::getNextCompleteLine(int start_idx, str *line) {
  if (!line) return 0;
  line->s   = 0;
  line->len = 0;
  int i;
  int start = start_idx;

  for (i = start_idx; i < rx_buffer.len;) {
    if (rx_buffer.s[i] != '\r' && rx_buffer.s[i] != '\n') {
      /* part of current line */
      i++;
      continue;
    }
    if (start == i) {
      /* skip over empty lines */
      i++;
      start = i;
      continue;
    }
    line->s   = rx_buffer.s + start;
    line->len = i - start;
    return 1;
  }

  return 0;
}

void OwlModem::removeRxBufferLine(str line) {
  /* Remove the line + next CRLF */
  if (line.s + line.len + 2 <= rx_buffer.s + rx_buffer.len && line.s[line.len] == '\r' && line.s[line.len + 1] == '\n')
    line.len += 2;
  int before_len = line.s - rx_buffer.s;
  int after_len  = rx_buffer.len - before_len - line.len;
  if (after_len > 0) {
    memmove(line.s, line.s + line.len, after_len);
    rx_buffer.len -= line.len;
  } else if (after_len == 0) {
    rx_buffer.len -= line.len;
  } else {
    LOG(L_ERR, "Bad len calculation %d\r\n", after_len);
  }
}

void OwlModem::consumeUnsolicited() {
  str line                = {0};
  int start               = 0;
  int saved_rx_buffer_len = rx_buffer.len;

  LOG(L_DBG, "Old-Buffer\r\n");
  LOGSTR(L_DBG, this->rx_buffer);

  while (getNextCompleteLine(start, &line)) {
    processURC(line, 1);
    start = line.s - rx_buffer.s;
    removeRxBufferLine(line);
  }

  /* remove leading \r\n - aka empty lines */
  int cnt = 0;
  for (cnt = 0; cnt < rx_buffer.len; cnt++)
    if (rx_buffer.s[cnt] != '\r' && rx_buffer.s[cnt] != '\n') break;
  if (cnt > 0) {
    memmove(rx_buffer.s, rx_buffer.s + cnt, rx_buffer.len - cnt);
    rx_buffer.len -= cnt;
  }

  if (saved_rx_buffer_len != rx_buffer.len) {
    LOG(L_DBG, "New-Buffer\r\n");
    LOGSTR(L_DBG, this->rx_buffer);
  }
}

void OwlModem::consumeUnsolicitedInCommandResponse() {
  str line                = {0};
  int start               = 0;
  int saved_rx_buffer_len = rx_buffer.len;
  int consumed            = 0;

  LOG(L_DBG, "Old-Buffer\r\n");
  LOGSTR(L_DBG, this->rx_buffer);

  while (getNextCompleteLine(start, &line)) {
    LOG(L_DBG, "Line [%.*s]\r\n", line.len, line.s);

    consumed = processURC(line, 0);
    if (consumed) {
      start = line.s - rx_buffer.s;
      removeRxBufferLine(line);
    } else {
      start = line.s - rx_buffer.s + line.len;
    }
  }

  if (saved_rx_buffer_len != rx_buffer.len) {
    LOG(L_DBG, "New-Buffer\r\n");
    LOGSTR(L_DBG, this->rx_buffer);
  }
}


int OwlModem::drainModemRxToBuffer() {
  LOG(L_MEM, "Trying to drain modem\r\n");
  int available, received, total = 0;
  while ((available = modem_port->available()) > 0) {
    if (available > MODEM_Rx_BUFFER_SIZE) available = MODEM_Rx_BUFFER_SIZE;
    if (available > MODEM_Rx_BUFFER_SIZE - rx_buffer.len) {
      int shift = available - (MODEM_Rx_BUFFER_SIZE - rx_buffer.len);
      LOG(L_WARN, "Rx buffer full with %d bytes. Dropping oldest %d bytes.\r\n", rx_buffer.len, shift);
      rx_buffer.len -= shift;
      memmove(rx_buffer.s, rx_buffer.s + shift, rx_buffer.len);
    }
    received = modem_port->readBytes(rx_buffer.s + rx_buffer.len, available);
    if (received != available) {
      LOG(L_ERR, "modem_port said %d bytes available, but received %d.\r\n", available, received);
      if (received < 0) goto error;
    }

    rx_buffer.len += received;
    total += received;

    if (rx_buffer.len > MODEM_Rx_BUFFER_SIZE) {
      LOG(L_ERR, "Bug in the rx_buffer_len calculation %d > %d\r\n", rx_buffer.len, MODEM_Rx_BUFFER_SIZE);
      goto error;
    }

    LOG(L_DBG, "Modem Rx - size changed from %d to %d bytes\r\n", rx_buffer.len - received, rx_buffer.len);
    LOGSTR(L_DBG, this->rx_buffer);
  }
error:
  LOG(L_MEM, "Done draining modem %d\r\n", total);
  return total;
}

int OwlModem::drainGNSSRx(str *gnss_buffer, int gnss_buffer_len) {
  if (!gnss_buffer || !gnss_port) return 0;
  LOG(L_MEM, "Trying to drain GNSS data\r\n");
  int available, received, total = 0, full = 0;
  while ((available = gnss_port->available()) > 0) {
    if (available > gnss_buffer_len) available = gnss_buffer_len;
    //    LOG(L_DBG, "Available %d bytes\r\n", available);
    if (available > gnss_buffer_len - gnss_buffer->len) {
      int shift = available - (gnss_buffer_len - gnss_buffer->len);
      LOG(L_WARN, "GNSS buffer full with %d bytes. Dropping oldest %d bytes.\r\n", gnss_buffer->len, shift);
      gnss_buffer->len -= shift;
      memmove(gnss_buffer->s, gnss_buffer->s + shift, gnss_buffer->len);
      full = 1;
    }
    received = gnss_port->readBytes(gnss_buffer->s + gnss_buffer->len, available);
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

int OwlModem::handleRxOnTimer() {
  LOG(L_MEM, "Entering timer on interrupt\r\n");
  int received = 0;

  if (!modem_port) goto error;

  if (in_timer) goto error;

  in_timer = 1;  // no goto error after this, we need to reset this flag

  /* Don't enter here in case a command is in progress. URC shouldn't come during a command and will call this later */
  if (in_command || in_bypass) goto done;

  received = drainModemRxToBuffer();
  if (rx_buffer.len > 0) consumeUnsolicited();

done:
  in_timer = 0;
  LOG(L_MEM, "Done timer on interrupt\r\n");
  return 1;
error:
  LOG(L_MEM, "Done timer on interrupt\r\n");
  return 0;
}


at_result_code_e OwlModem::extractResult(str *out_response, int max_response_len) {
  at_result_code_e result_code;
  for (int i = 0; i <= rx_buffer.len - 6; i++) {
    if (rx_buffer.s[i] != '\r' && rx_buffer.s[i] != '\n') continue;
    result_code = at_result_code_extract(rx_buffer.s + i, rx_buffer.len - i);
    if (result_code == AT_Result_Code__cme_error) {
      //      LOG(L_INFO, "Rx-Before [%.*s]\r\n", rx_buffer.len, rx_buffer.s);
      /* CME Error received - extract the text into response and return ERROR */
      result_code = AT_Result_Code__ERROR;
      char *start = rx_buffer.s + i + 2 /* CRLF */ + 12 /* length of "+CME ERROR: " */;
      int len     = 0;
      while (start + len + 1 < rx_buffer.s + rx_buffer.len) {
        if (start[len] == '\r' && start[len + 1] == '\n') break;
        len++;
      }
      if (out_response) {
        memcpy(out_response->s, start, len > max_response_len ? max_response_len : len);
        out_response->len = len;
      }
      /* truncate the rx_buffer */
      int next_index = i + 2 /* CRLF */ + 12 /* length of "+CME ERROR: " */ + len + 2 /* CRLF */;
      rx_buffer.len -= next_index;
      if (rx_buffer.len > 0) memmove(rx_buffer.s, rx_buffer.s + next_index, rx_buffer.len);
      //      LOG(L_INFO, "Result %d - %s\r\n", result_code, at_result_code_text(result_code));
      //      LOG(L_INFO, "Rx-After  [%.*s]\r\n", rx_buffer.len, rx_buffer.s);
      return result_code;
    } else if (result_code >= AT_Result_Code__OK) {
      /* extract the response before the result code */
      if (out_response) {
        /* remove the CR LF prefix/suffix */
        char *start = rx_buffer.s;
        int len     = i;
        while (len > 0 && (start[0] == '\r' || start[0] == '\n')) {
          start++;
          len--;
        }
        while (len > 0 && (start[len - 1] == '\r' || start[len - 1] == '\n')) {
          len--;
        }
        out_response->len = len > max_response_len ? max_response_len : len;
        memcpy(out_response->s, start, out_response->len);
        if (out_response->len < max_response_len) out_response->s[out_response->len] = '\0';
      }
      /* truncate the rx_buffer */
      int next_index = i + 2 /* CRLF */ + strlen(at_result_code_text(result_code)) + 2 /* CRLF */;
      rx_buffer.len -= next_index;
      if (rx_buffer.len > 0) memmove(rx_buffer.s, rx_buffer.s + next_index, rx_buffer.len);
      //      LOG(L_INFO, "Result %d - %s\r\n", result_code,  at_result_code_text(result_code);
      return result_code;
    }
  }
  return AT_Result_Code__unknown;
}

at_result_code_e OwlModem::doCommand(str command, uint32_t timeout_millis, str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  at_result_code_e result_code;
  owl_time_t timeout;
  int received;
  if (!modem_port) goto failure;

  /* Make sure that the timer has finished handling Rx and prevent it from re-entering */
  in_command = 1;
  /* wait for the timer to exit */
  int i;
  for (i = 0; i < 100 && in_timer != 0; i++)
    delay(50);
  if (i >= 100) {
    LOG(L_ERR, "[%.*s] either called from a timer, or timeout waiting for timer to exit\r\n", command.len, command.s);
    return AT_Result_Code__failure;
  }

  /* Tx */
  if (!sendData(command)) goto failure;
  //  LOG(L_DBG, "[%.*s] sent\r\n", command.len, command.s);

  /* Before sending the actual command, get rid of the URC in the pipe. This way, the result of this command will be
   * nice and empty. */
  if (rx_buffer.len) consumeUnsolicited();

  /* Tx CRLF*/
  if (!sendData(CMDLT)) goto failure;
  LOG(L_DBG, "[%.*s] sent\r\n", command.len, command.s);
  //  LOG(L_DBG, "[CRLF] sent\r\n");

  /* Rx */
  timeout = owl_time() + timeout_millis;
  do {
    received = drainModemRxToBuffer();
    if (!received) {
      delay(50);
      if (owl_time() < timeout)
        continue;
      else
        break;
    }
    consumeUnsolicitedInCommandResponse();
    result_code = extractResult(out_response, max_response_len);
    if (result_code >= AT_Result_Code__OK) {
      in_command = 0;
      if (out_response)
        LOG(L_DBG, " - Execution complete - Result %d - %s Data [%.*s]\r\n", result_code,
            at_result_code_text(result_code), out_response->len, out_response->s);
      else
        LOG(L_DBG, " - Execution complete - Result %d - %s\r\n", result_code, at_result_code_text(result_code));
      return result_code;
    }
  } while (owl_time() < timeout);

  if (!str_equalcase_char(command, "AT")) LOG(L_WARN, " - Timed-out on [%.*s]\r\n", command.len, command.s);

  //  // reset the buffer on timeout
  //  rx_buffer.len = 0;
  // or don't reset on timeout - the handleRxOnTimer() shall drop orphan lines

  in_command = 0;
  return AT_Result_Code__timeout;
failure:
  LOG(L_WARN, " - Failure on [%.*s]\r\n", command.len, command.s);
  in_command = 0;
  return AT_Result_Code__failure;
}

at_result_code_e OwlModem::doCommand(char *command, uint32_t timeout_millis, str *out_response, int max_response_len) {
  str s = {.s = command, .len = strlen(command)};
  return doCommand(s, timeout_millis, out_response, max_response_len);
}

void OwlModem::setDebugLevel(int level) {
  owl_log_set_level(level);
}

void OwlModem::filterResponse(str prefix, str *response) {
  if (!response) return;
  str line = {0};
  while (str_tok(*response, "\r\n", &line)) {
    if (!str_equal_prefix(line, prefix)) {
      /* Remove the line + next CRLF */
      if (line.s + line.len + 2 <= response->s + response->len && line.s[line.len] == '\r' &&
          line.s[line.len + 1] == '\n')
        line.len += 2;
      str_shrink_inside(*response, line.s, line.len);
      line.len = 0;
      continue;
    }
    /* Remove the prefix */
    str_shrink_inside(*response, line.s, prefix.len);
    line.len -= prefix.len;
  }
}
