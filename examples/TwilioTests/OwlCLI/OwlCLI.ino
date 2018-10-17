/*
 * OwlCLI.ino
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
 * \file OwlModemTest.ino - Example for testing out functionality of the modem.
 */

#include <board.h>

#include <BreakoutSDK/modem/OwlModem.h>
#include <BreakoutSDK/CLI/OwlModemCLI.h>
#include <BreakoutSDK/CoAP/CoAPPeer.h>



/* Modem result buffer */
#define MODEM_RESULT_LEN 256
char result_buffer[MODEM_RESULT_LEN];
str modem_result = {.s = result_buffer, .len = 0};

/**
 *  Replace here with your SIM card's PIN, or modify the handler_PIN() function below to get it from somewhere.
 *  Because the PIN can be binary, set correctly also the length in the str structure below.
 */
static str sim_pin = STRDECL("000000");


/* The Twilio-specific modem interface */
OwlModem *owlModem = 0;
/* This is only for API demonstration purposes, can be removed for in-production */
OwlModemCLI *owlModemCLI = 0;



void setup() {
  // This is a good place to make sure the port is initialized properly.
  SerialDebugPort.enableBlockingTx();  // reliably write to it

  owl_log_set_level(L_INFO);
  LOG(L_NOTICE, "Arduino setup() starting up\r\n");

  owlModem    = new OwlModem(&SerialModule, &SerialDebugPort);
  owlModemCLI = new OwlModemCLI(owlModem, &SerialDebugPort);

  LOG(L_NOTICE, ".. WioLTE Cat.NB-IoT - powering on modules\r\n");
  if (!owlModem->powerOn()) {
    LOG(L_ERR, ".. WioLTE Cat.NB-IoT - ... modem failed to power on\r\n");
    goto error_stop;
  }
  LOG(L_NOTICE, ".. WioLTE Cat.NB-IoT - now powered on.\r\n");

  /* Initialize modem configuration to something we can trust. */
  LOG(L_NOTICE, ".. OwlModem - initializing modem\r\n");

  if (!owlModem->initModem(TESTING_VARIANT_INIT)) {
    LOG(L_NOTICE, "..   - failed initializing modem! - resetting in 30 seconds\r\n");
    delay(30000);
    goto error_stop;
  }
  LOG(L_NOTICE, ".. OwlModem - initialization successfully completed\r\n");


  LOG(L_NOTICE, ".. Setting handlers for SIM-PIN and NetworkRegistration Unsolicited Response Codes\r\n");
  owlModem->SIM.setHandlerPIN(handler_PIN);
  owlModem->network.setHandlerNetworkRegistrationURC(handler_NetworkRegistrationStatusChange);
  owlModem->network.setHandlerGPRSRegistrationURC(handler_GPRSRegistrationStatusChange);
  owlModem->network.setHandlerEPSRegistrationURC(handler_EPSRegistrationStatusChange);

  if (!owlModem->waitForNetworkRegistration("devkit", TESTING_VARIANT_REG)) {
    LOG(L_ERR, ".. WioLTE Cat.NB-IoT - ... modem failed to register to the network\r\n");
    goto error_stop;
  }
  LOG(L_NOTICE, ".. OwlModem - registered to network\r\n");



  LOG(L_NOTICE, "Arduino setup() done\r\n");
  LOG(L_NOTICE, "Arduino loop() starting\r\n");
  return;
error_stop:
  // TODO - find something which does work on this board to software reset it
  //    (softwareResetFunc)();
  //  LOG(L_NOTICE, "TODO - try to find a way to software-reset the board. Until then, you are in bypass mode now\r\n");
  //  while (1) {
  //    owlModem->bypass();
  //    delay(100);
  //  }
  return;
}


int cli_resume = 0;

void loop() {
  /* Take user input and run commands interactively - optional, if you want CLI */
  cli_resume = owlModemCLI->handleUserInput(cli_resume);

  /* Take care of async modem events */
  owlModem->handleRxOnTimer();
  /* Important step - handling UDP data */
  owlModem->socket.handleWaitingData();

  /* Take care of DTLS retransmissions */
  //  owlDTLSClient.triggerPeriodicRetransmit();

  /* Take care of CoAP retransmissions */
  CoAPPeer::triggerPeriodicRetransmit();

  delay(50);


  //  test_modem_bypass();

  //  test_modem_get_info();

  //  test_modem_network_management();
}


/*
 * Handlers
 */
void handler_NetworkRegistrationStatusChange(at_creg_stat_e stat, uint16_t lac, uint32_t ci, at_creg_act_e act) {
  LOG(L_INFO,
      "\r\n>>>\r\n>>>URC-Network>>> Change in registration: Status=%d(%s) LAC=0x%04x CI=0x%08x "
      "Act=%d(%s)\r\n>>>\r\n\r\n",
      stat, at_creg_stat_text(stat), lac, ci, act, at_creg_act_text(act));
}

void handler_GPRSRegistrationStatusChange(at_cgreg_stat_e stat, uint16_t lac, uint32_t ci, at_cgreg_act_e act,
                                          uint8_t rac) {
  LOG(L_INFO,
      "\r\n>>>\r\n>>>URC-GPRS>>> Change in registration: Status=%d(%s) LAC=0x%04x CI=0x%08x Act=%d(%s) "
      "RAC=0x%02x\r\n>>>\r\n\r\n",
      stat, at_cgreg_stat_text(stat), lac, ci, act, at_cgreg_act_text(act), rac);
}

void handler_EPSRegistrationStatusChange(at_cereg_stat_e stat, uint16_t lac, uint32_t ci, at_cereg_act_e act,
                                         at_cereg_cause_type_e cause_type, uint32_t reject_cause) {
  LOG(L_INFO,
      "\r\n>>>\r\n>>>URC-EPS>>> Change in registration: Status=%d(%s) LAC=0x%04x CI=0x%08x Act=%d(%s) "
      "Cause-Type=%d(%s) Reject-Cause=%u\r\n>>>\r\n\r\n",
      stat, at_cereg_stat_text(stat), lac, ci, act, at_cereg_act_text(act), cause_type,
      at_cereg_cause_type_text(cause_type), reject_cause);
}

void handler_UDPData(uint8_t socket, str remote_ip, uint16_t remote_port, str data) {
  LOG(L_INFO,
      "\r\n>>>\r\n>>>URC-UDP-Data>>> Received UDP data from socket=%d remote_ip=%.*s remote_port=%u of %d bytes\r\n",
      socket, remote_ip.len, remote_ip.s, remote_port, data.len);
  LOGSTR(L_INFO, data);
  LOGE(L_INFO, "]\r\n>>>\r\n\r\n");
}

static void handler_SocketClosed(uint8_t socket) {
  LOG(L_INFO, "\r\n>>>\r\n>>>URC-Socket-Closed>>> Socket Closed socket=%d", socket);
}


static int pin_count = 0;

void handler_PIN(str message) {
  LOG(L_CLI, "\r\n>>>\r\n>>>PIN>>> %.*s\r\n>>>\r\n\r\n", message.len, message.s);
  if (str_equalcase_char(message, "READY")) {
    /* Seems fine */
  } else if (str_equalcase_char(message, "SIM PIN")) {
    /* The card needs the PIN */
    pin_count++;
    if (pin_count > 1) {
      LOG(L_CLI, "Trying to avoid a PIN lock - too many attempts to enter the PIN\r\n");
    } else {
      LOG(L_CLI, "Verifying PIN...\r\n");
      if (owlModem->SIM.verifyPIN(sim_pin)) {
        LOG(L_CLI, "... PIN verification OK\r\n");
      } else {
        LOG(L_CLI, "... PIN verification Failed\r\n");
      }
    }
  } else if (str_equalcase_char(message, "SIM PUK")) {
    /* and so on ... */
  } else if (str_equalcase_char(message, "SIM PIN2")) {
    /* and so on ... */
  } else if (str_equalcase_char(message, "SIM PUK2")) {
    /* and so on ... */
  } else if (str_equalcase_char(message, "SIM not inserted")) {
    /* Panic mode :) */
    LOG(L_CLI, "No SIM in, not much to do...\r\n");
  }
}



/*
 * Tests and examples for using the modem module
 */

void test_modem_bypass() {
  owlModem->bypass();
  delay(100);
}



void test_modem_get_info() {
  if (!owlModem->information.getProductIdentification(&modem_result, MODEM_RESULT_LEN)) {
    LOG(L_ERR, "Error retrieving Product Information\r\n");
  } else {
    LOG(L_INFO, "Product Information:\r\n%.*s\r\n--------------------\r\n", modem_result.len, modem_result.s);
  }

  if (!owlModem->information.getManufacturer(&modem_result, MODEM_RESULT_LEN)) {
    LOG(L_ERR, "Error retrieving Manufacturer Information\r\n");
  } else {
    LOG(L_INFO, "Manufacturer Information:\r\n%.*s\r\n-------------------------\r\n", modem_result.len, modem_result.s);
  }

  if (!owlModem->information.getModel(&modem_result, MODEM_RESULT_LEN)) {
    LOG(L_ERR, "Error retrieving ModelInformation\r\n");
  } else {
    LOG(L_INFO, "Model Information:\r\n%.*s\r\n-------------------------\r\n", modem_result.len, modem_result.s);
  }

  if (!owlModem->information.getVersion(&modem_result, MODEM_RESULT_LEN)) {
    LOG(L_ERR, "Error retrieving Version Information\r\n");
  } else {
    LOG(L_INFO, "Version Information:\r\n%.*s\r\n-------------------------\r\n", modem_result.len, modem_result.s);
  }



  if (!owlModem->information.getIMEI(&modem_result, MODEM_RESULT_LEN)) {
    LOG(L_ERR, "Error retrieving IMEI\r\n");
  } else {
    LOG(L_INFO, "IMEI: [%.*s]\r\n", modem_result.len, modem_result.s);
  }

  if (!owlModem->SIM.getICCID(&modem_result, MODEM_RESULT_LEN)) {
    LOG(L_ERR, "Error retrieving ICCID\r\n");
  } else {
    LOG(L_INFO, "ICCID: [%.*s]\r\n", modem_result.len, modem_result.s);
  }

  if (!owlModem->SIM.getIMSI(&modem_result, MODEM_RESULT_LEN)) {
    LOG(L_ERR, "Error retrieving IMSI\r\n");
  } else {
    LOG(L_INFO, "IMSI: [%.*s]\r\n", modem_result.len, modem_result.s);
  }

  if (!owlModem->SIM.getMSISDN(&modem_result, MODEM_RESULT_LEN)) {
    LOG(L_ERR, "Error retrieving IMSI\r\n");
  } else {
    LOG(L_INFO, "MSISDN: [%.*s]\r\n", modem_result.len, modem_result.s);
  }

  if (!owlModem->information.getBatteryChargeLevels(&modem_result, MODEM_RESULT_LEN)) {
    LOG(L_ERR, "Error retrieving Battery Charge Levels\r\n");
  } else {
    LOG(L_INFO, "Battery Charge Levels: [%.*s]\r\n", modem_result.len, modem_result.s);
  }

  if (!owlModem->information.getIndicatorsHelp(&modem_result, MODEM_RESULT_LEN)) {
    LOG(L_ERR, "Error retrieving Indicators-Help\r\n");
  } else {
    LOG(L_INFO, "Indicators-Help: [%.*s]\r\n", modem_result.len, modem_result.s);
  }

  if (!owlModem->information.getIndicators(&modem_result, MODEM_RESULT_LEN)) {
    LOG(L_ERR, "Error retrieving Indicators\r\n");
  } else {
    LOG(L_INFO, "Indicators: [%.*s]\r\n", modem_result.len, modem_result.s);
  }

  delay(5000);
}


void test_modem_network_management() {
  at_cfun_power_mode_e mode;

  LOG(L_INFO, "Retrieving modem functionality mode (AT+CFUN?)\r\n");
  if (!owlModem->network.getModemFunctionality(&mode, 0)) {
    LOG(L_ERR, "Error retrieving modem functionality mode\r\n");
    delay(500000000);
  }

  switch (mode) {
    case AT_CFUN__POWER_MODE__Minimum_Functionality:
      LOG(L_INFO, "Functionality is [Minimum] --> switching to [Full]\r\n");
      mode = AT_CFUN__POWER_MODE__Full_Functionality;
      break;
    case AT_CFUN__POWER_MODE__Full_Functionality:
      LOG(L_INFO, "Functionality is [Full] --> switching to [Airplane]\r\n");
      mode = AT_CFUN__POWER_MODE__Airplane_Mode;
      break;
    case AT_CFUN__POWER_MODE__Airplane_Mode:
      LOG(L_INFO, "Functionality is [Airplane] --> switching to [Minimum]\r\n");
      mode = AT_CFUN__POWER_MODE__Minimum_Functionality;
      break;
    default:
      LOG(L_ERR, "Unexpected functionality mode %d\r\n", mode);
      delay(500000000);
  }
  if (!owlModem->network.setModemFunctionality((at_cfun_fun_e)mode, 0)) {
    LOG(L_ERR, "Error setting modem functionality mode\r\n");
    delay(500000000);
  }

  delay(5000);
}
