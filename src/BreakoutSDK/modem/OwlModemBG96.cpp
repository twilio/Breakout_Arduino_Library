/*
 * OwlModemBG96.cpp
 * Twilio Breakout SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
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
 * \file OwlModemBG96.cpp - wrapper for BG96 modems on Seeed WiO tracker board via the Grove port
 */

#include "OwlModemBG96.h"

OwlModemBG96::OwlModemBG96(HardwareSerial *modem_port_in)
    : modem_port(modem_port_in),
      AT(&modem_port),
      information(&AT),
      SIM(&AT),
      network(&AT),
      pdn(&AT),
      ssl(&AT),
      mqtt(&AT) {
  modem_port_in->begin(BG96_Baudrate);
  if (!modem_port_in) {
    LOG(L_ERR, "OwlModemBG96 initialized without modem port. That is not going to work\r\n");
  }

  pinMode(ANALOG_RND_PIN, INPUT);
  randomSeed(analogRead(ANALOG_RND_PIN));

  has_modem_port = (modem_port_in != nullptr);
}

OwlModemBG96::~OwlModemBG96() {
}

int OwlModemBG96::powerOn() {
  pinMode(GROVE_PWR_PIN, OUTPUT);
  digitalWrite(GROVE_PWR_PIN, HIGH);  // VCC_B Enable pin

  pinMode(BG96_RESET_PIN, OUTPUT);
  digitalWrite(BG96_RESET_PIN, LOW);

  delay(300);

  digitalWrite(BG96_RESET_PIN, HIGH);

  owl_time_t timeout = owl_time() + 10 * 1000;
  while (!isPoweredOn()) {
    if (owl_time() > timeout) {
      LOG(L_ERR, "Timed-out waiting for modem to power on\r\n");
      return 0;
    }
    delay(50);
  }
  return 1;
}

int OwlModemBG96::powerOff() {
  AT.doCommandBlocking("AT+QPOWD=0", 500, nullptr);
  delay(300);

  pinMode(BG96_RESET_PIN, OUTPUT);
  digitalWrite(BG96_RESET_PIN, LOW);
}

int OwlModemBG96::isPoweredOn() {
  return AT.doCommandBlocking("AT", 1000, nullptr) == AT_Result_Code__OK;
}

/**
 * Handler for PIN, used during initialization
 * @param message
 */
void OwlModemBG96::initCheckPIN(str message) {
  if (!str_equal_prefix_char(message, "READY")) {
    LOG(L_ERR,
        "PIN status [%.*s] != READY and PIN handler not set. Please disable the SIM card PIN, or set a handler.\r\n",
        message.len, message.s);
  }
}

int OwlModemBG96::initModem() {
  at_result_code_e rc;
  OwlModem_PINHandler_f saved_handler = 0;

  if (!AT.initTerminal()) {
    return 0;
  }

#if defined(COPS_OVERRIDE_LONG) || defined(COPS_OVERRIDE_SHORT) || defined(COPS_OVERRIDE_NUMERIC)
  // deregister from the network before the modem hangs
  if (!network.setOperatorSelection(AT_COPS__Mode__Deregister_from_Network, nullptr, nullptr, nullptr)) {
    LOG(L_ERR, "Potential deregistering from network\r\n");
  }
#endif

  // TODO: skip if modem is already in the desired state. Writing some magic number to a file maybe?
  if (true) {
    /* A modem reset is required */
    if (!network.setModemFunctionality(AT_CFUN__FUN__Minimum_Functionality, 0))
      LOG(L_WARN, "Error turning modem off\r\n");

    if (AT.doCommandBlocking("AT+QCFG=\"nwscanseq\",03,1", 5000, nullptr) != AT_Result_Code__OK)
      LOG(L_WARN, "Error setting RAT priority to NB1\r\n");

    if (AT.doCommandBlocking("AT+QCFG=\"nwscanmode\",3,1", 5000, nullptr) != AT_Result_Code__OK)
      LOG(L_WARN, "Error setting RAT technology to LTE\r\n");

    if (AT.doCommandBlocking("AT+QCFG=\"iotopmode\",1,1", 5000, nullptr) != AT_Result_Code__OK)
      LOG(L_WARN, "Error setting network node to NB1\r\n");

    if (AT.doCommandBlocking("AT+QCFG=\"band\",0,0,A0E189F,1", 5000, nullptr) != AT_Result_Code__OK)
      LOG(L_WARN, "Error setting NB bands to \"all bands\"\r\n");

    // TODO: celevel and servicedomain

    if (AT.doCommandBlocking("AT+QURCCFG=\"urcport\",\"uart1\"", 5000, nullptr) != AT_Result_Code__OK)
      LOG(L_WARN, "Error directing URCs to the main UART\r\n");

    if (AT.doCommandBlocking("AT+QICSGP=1,1,\"" TESTING_APN "\"", 1000, nullptr) != AT_Result_Code__OK)
      LOG(L_WARN, "Error setting custom APN\r\n");


    // at_cfun_rst_e rst = AT_CFUN__RST__Modem_and_SIM_Silent_Reset;
    // NOTE: BG96 seens to go to power down instead of resetting
    if (!network.setModemFunctionality(AT_CFUN__FUN__Full_Functionality, 0)) LOG(L_WARN, "Error resetting modem\r\n");

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


  if (AT.doCommandBlocking("AT+CSCS=\"GSM\"", 1000, nullptr) != AT_Result_Code__OK) {
    LOG(L_WARN, "Potential error setting character set to GSM\r\n");
  }

  if (AT.doCommandBlocking("AT+CREG=2", 1000, nullptr) != AT_Result_Code__OK) {
    LOG(L_WARN,
        "Potential error setting URC to Registration and Location Updates for Network Registration Status events\r\n");
  }
  if (AT.doCommandBlocking("AT+CGREG=2", 1000, nullptr) != AT_Result_Code__OK) {
    LOG(L_WARN,
        "Potential error setting GPRS URC to Registration and Location Updates for Network Registration Status "
        "events\r\n");
  }
  if (AT.doCommandBlocking("AT+CEREG=2", 1000, nullptr) != AT_Result_Code__OK) {
    LOG(L_WARN,
        "Potential error setting EPS URC to Registration and Location Updates for Network Registration Status "
        "events\r\n");
  }

  if (SIM.handler_cpin) saved_handler = SIM.handler_cpin;
  SIM.setHandlerPIN(initCheckPIN);
  if (AT.doCommandBlocking("AT+CPIN?", 5000, nullptr) != AT_Result_Code__OK) {
    LOG(L_WARN, "Error checking PIN status\r\n");
  }

  AT.doCommandBlocking("AT+QIACT=1", 5000, nullptr);  // ignore the result, which will be an error if already activated
  LOG(L_DBG, "Modem correctly initialized\r\n");
  return 1;
}

int OwlModemBG96::waitForNetworkRegistration(owl_time_t timeout) {
  bool network_ready    = false;
  owl_time_t begin_time = owl_time();

  while (true) {
    at_cereg_stat_e stat;
    if (network.getEPSRegistrationStatus(0, &stat, 0, 0, 0, 0, 0)) {
      network_ready = (stat == AT_CEREG__Stat__Registered_Home_Network || stat == AT_CEREG__Stat__Registered_Roaming);
      if (network_ready) break;
    }
    if (timeout != 0 && owl_time() > begin_time + timeout) {
      LOG(L_ERR, "Bailing out from network registration - for testing purposes only\r\n");
      return 0;
    }
    LOG(L_NOTICE, ".. waiting for network registration\r\n");
    delay(200);
  }

  return 1;
}
