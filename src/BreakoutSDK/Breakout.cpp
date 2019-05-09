/*
 * Breakout.cpp
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

#include "Breakout.h"

#include <BreakoutSDK/utils/lists.h>
#include <BreakoutSDK/CoAP/CoAP.h>

#include <stdio.h>



Breakout::Breakout() {
  // strip = owl_new WS2812(1, ublox->RGB_LED_PIN);
  // ublox->enableRGBPower();
  // strip->begin();
  // strip->brightness = 20;

  SerialDebugPort.enableBlockingTx();  // reliably write to it
  owl_log_set_level(L_ISSUE);

  setPurpose("Dev-Kit");
}

Breakout::~Breakout() {
  delete owlModem;
  owlModem = 0;
  delete coapPeer;
  coapPeer = 0;

#if TESTING_WITH_CLI == 1
  delete owlModemCLI;
  owlModemCLI = 0;
#endif

  WL_FREE_ALL(&commands, breakout_command_list_t);
}



/*                      Setting Parameters & Event Handlers                         */

bool Breakout::setPurpose(char const *new_purpose) {
  if (owlModem != 0) {
    LOG(L_ERR, "Can only set purpose before initialization\r\n");
    return false;
  }
  if (!new_purpose || strlen(new_purpose) == 0) {
    LOG(L_ERR, "Empty purpose is not supported\r\n");
    return false;
  }

  strncpy(purpose, new_purpose, sizeof(purpose) - 1);
  purpose[sizeof(purpose) - 1] = '\0';

  return true;
}

void Breakout::setPollingInterval(uint32_t interval_seconds) {
  uint32_t old_interval = polling_interval;
  if (interval_seconds >= BREAKOUT_POLLING_INTERVAL_MINIMUM) {
    polling_interval = interval_seconds;
  } else if (interval_seconds == 0) {
    polling_interval = 0;
  } else {
    LOG(L_WARN, "Interval %u seconds less than minimum of %u but not 0. Using minimum polling interval.\r\n",
        interval_seconds, BREAKOUT_POLLING_INTERVAL_MINIMUM);
    polling_interval = BREAKOUT_POLLING_INTERVAL_MINIMUM;
  }

  if (polling_interval == 0) {
    // Cancel polling timer
    next_polling = 0;
  } else if (old_interval != polling_interval) {
    // Setup polling timer - will be triggered in spin()
    if (last_polling == 0) {
      // First time - do it soon
      next_polling = 1;  // 0 means disabled
    } else {
      next_polling = last_polling + polling_interval * 1000;
    }
  }
}

bool Breakout::setPSKKey(char const *hex_key) {
  if (owlModem != 0) {
    LOG(L_ERR, "Can only set PSK-Key before initialization\r\n");
    return false;
  }
  str input = {.s = (char *)hex_key, .len = strlen(hex_key)};
  // c_psk_key is aliased with psk_key.s
  psk_key.len = hex_to_str(c_psk_key, sizeof(c_psk_key), input);
  if (psk_key.len * 2 != input.len) {
    LOG(L_ERR, "Failed setting the PSK-Key - maybe bad hex value?\r\n");
    return false;
  }
  return true;
}

void Breakout::setConnectionStatusHandler(BreakoutConnectionStatusHandler_f handler) {
  connection_handler = handler;
}

void Breakout::setCommandHandler(BreakoutCommandHandler_f handler) {
  command_handler = handler;
}



/*                      Powering up the module & Status                                */

bool Breakout::isPowered(void) {
  if (owlModem == 0) return false;
  return owlModem->isPoweredOn() == 1;
}

bool Breakout::initModem() {
  char c_buffer[64];
  str buffer = {.s = c_buffer, .len = 0};

  if (owlModem != 0) return true;

  LOG(L_NOTICE, "OwlModem starting up\r\n");

  if (!(owlModem = owl_new OwlModemRN4(&SerialModule, &SerialDebugPort, &SerialGNSS))) GOTOERR(error_stop);

  LOG(L_NOTICE, ".. OwlModem - powering on modules\r\n");
  if (!owlModem->powerOn()) {
    LOG(L_ERR, ".. OwlModem - modem failed to power on\r\n");
    goto error_stop;
  }
  LOG(L_NOTICE, ".. OwlModem - now powered on - initializing\r\n");

  /* Initialize modem configuration to something we can trust. */
  if (!owlModem->initModem(TESTING_VARIANT_INIT)) {
    LOG(L_NOTICE, "..   - failed initializing modem! - resetting in 30 seconds\r\n");
    delay(30000);
    goto error_stop;
  }
  LOG(L_NOTICE, ".. OwlModem - initialization successfully completed - next waiting for network registration\r\n");

  /* Read the ICCID */
  if (!owlModem->SIM.getICCID(&buffer, 64)) {
    LOG(L_ERR, "Error reading ICCID\r\n");
    goto error_stop;
  }
  /* The PSK-Id is the ICCID, but saving it somewhere else, just in case we'll change this in the future */
  psk_id.len = buffer.len < sizeof(c_psk_id) ? buffer.len : sizeof(c_psk_id);
  memcpy(c_psk_id, buffer.s, buffer.len);
  /* Prepare the ICCID for UriQuery */
  iccid.len = snprintf(iccid.s, sizeof(c_iccid), "Sim=%.*s", buffer.len, buffer.s);

  owlModem->SIM.setHandlerPIN(Breakout::handler_PIN);
  owlModem->network.setHandlerNetworkRegistrationURC(Breakout::handler_NetworkRegistrationStatusChange);
  owlModem->network.setHandlerGPRSRegistrationURC(Breakout::handler_GPRSRegistrationStatusChange);
  owlModem->network.setHandlerEPSRegistrationURC(Breakout::handler_EPSRegistrationStatusChange);

  if (!owlModem->waitForNetworkRegistration(purpose, TESTING_VARIANT_REG)) {
    LOG(L_ERR, ".. OwlModem - modem failed to register to the network!\r\n");
    if (((TESTING_VARIANT_REG)&Testing__Timeout_Network_Registration_30_Sec) != 0) {
      LOG(L_WARN, ".. Dropping to CLI, for debugging\r\n");
      return true;
    }
    goto error_stop;
  }

  LOG(L_NOTICE, "... OwlModem - registered to network.\r\n");
  return true;
error_stop:
  LOG(L_ERR, "... OwlModem - Network initialization failed, please reset the device.\r\n");
  while (true)
    delay(1000);
  return false;
}

bool Breakout::initCoAPPeer() {
  if (coapPeer) {
    LOG(L_NOTICE, "CoAPPeer - Already initialized\r\n");
    return true;
  }
  owl_time_t timeout = 0;
  str remote_ip      = STRDECL(BREAKOUT_IP);
  int retries        = BREAKOUT_INIT_CONNECTION_RETRIES;

  LOG(L_NOTICE, "CoAPPeer - creating \r\n");

#if TESTING_WITH_DTLS == 0
  coapPeer = owl_new CoAPPeer(owlModem, 0, remote_ip, 5683);
#else
  coapPeer = owl_new CoAPPeer(owlModem, psk_id, psk_key, 0, remote_ip, 5684);
#endif
  if (!coapPeer) GOTOERR(error);

  coapPeer->setHandlers(Breakout::handler_CoAPStatelessMessage, Breakout::handler_CoAPDTLSEvent,
                        Breakout::handler_CoAPRequest, Breakout::handler_CoAPResponse);
  if (!coapPeer->reinitialize()) GOTOERR(error);

  LOG(L_NOTICE, ".. CoAPPeer - waiting for transport to be ready (DTLS handshake)\r\n");
  timeout = owl_time() + BREAKOUT_INIT_CONNECTION_TIMEOUT * 1000;
  while (!coapPeer->transportIsReady()) {
    // Need to spin, because otherwise tinydtls things don't get properly initialized.
    spin();
    delay(50);
    if (timeout < owl_time()) {
      if (retries <= 0) {
        LOG(L_ERR, "Failed to initialize CoAP peer %d times in a row\r\n", BREAKOUT_INIT_CONNECTION_RETRIES);
        goto error;
      }
      LOG(L_NOTICE, ".. CoAPPeer - will try another initialization %d left\r\n", retries - 1);
      timeout = owl_time() + BREAKOUT_INIT_CONNECTION_TIMEOUT * 1000;
      if (!coapPeer->reinitialize()) GOTOERR(error);
      retries--;
    }
  }
  LOG(L_NOTICE, "... CoAP Peer is ready.\r\n");
  LOG(L_NOTICE, "        B R E A K O U T\r\n");
  LOG(L_NOTICE, "▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅\r\n");
  LOG(L_NOTICE, "▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅\r\n");
  LOG(L_NOTICE, "▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅\r\n");
  LOG(L_NOTICE, "▓▓▓▓▓▓▓▓   ▓▓▓▓▓▓▓   ▓▓▓▓▓▓▓▓▓▓\r\n");
  LOG(L_NOTICE, "▒▒▒▒       ▒▒▒▒▒▒          ▒▒▒▒\r\n");
  LOG(L_NOTICE, "▒▒▒▒                       ▒▒▒▒\r\n");
  LOG(L_NOTICE, "░░░░                      ░░░░░\r\n");
  LOG(L_NOTICE, "                               \r\n");
  LOG(L_NOTICE, "                               \r\n");
  LOG(L_NOTICE, "                               \r\n");
  LOG(L_NOTICE, "                               \r\n");
  LOG(L_NOTICE, "                               \r\n");
  LOG(L_NOTICE, "               ⚪               \r\n");
  LOG(L_NOTICE, "     ▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁          \r\n");
  if (!coap_status) {
    coap_status = true;
    notifyConnectionStatusChanged();
  }
  return true;
error:
  LOG(L_ERR, "... CoAP Peer was not initialized correctly.\r\n");
  if (coap_status) {
    coap_status = false;
    notifyConnectionStatusChanged();
  }
  return false;
}


bool Breakout::powerModuleOn() {
  if (owlModem == 0) {
    if (!initModem()) GOTOERR(error);
    if (!initCoAPPeer()) GOTOERR(error);
    return true;
  }

  if (isPowered()) return true;

  eps_registration_status = AT_CEREG__Stat__Not_Registered;
  if (!owlModem->powerOn()) GOTOERR(error);
  // No modem re-init here
  if (!reinitializeTransport()) GOTOERR(error);
  return true;
error:
  LOG(L_ERR, "Error powering module on\r\n");
  return false;
}

bool Breakout::powerModuleOff(void) {
  if (owlModem == 0) {
    LOG(L_ERR, "No modem instance created yet, can't be powered on\r\n");
    return false;
  }

  // we already are powered off
  if (!isPowered()) return true;

  eps_registration_status = AT_CEREG__Stat__Not_Registered;
  // Should we not keep the CoAP peer up?
  //  coapPeer->close();
  //  delete coapPeer;
  //  coapPeer = 0;
  return owlModem->powerOff() != 0;
}

connection_status_e Breakout::getConnectionStatus() {
  connection_status_e status = CONNECTION_STATUS_OFFLINE;
  switch (eps_registration_status) {
    case AT_CEREG__Stat__Registered_Home_Network:
    case AT_CEREG__Stat__Registered_Roaming:
      if (coap_status) {
        status = CONNECTION_STATUS_REGISTERED_AND_CONNECTED;
      } else {
        status = CONNECTION_STATUS_REGISTERED_NOT_CONNECTED;
      }
      break;
    case AT_CEREG__Stat__Registration_Denied:
      status = CONNECTION_STATUS_NETWORK_REGISTRATION_DENIED;
      break;
    default:
      status = CONNECTION_STATUS_OFFLINE;
      break;
  }
  return status;
}

bool Breakout::reinitializeTransport() {
  LOG(L_WARN, "Reinitializing transport connection with the Twilio Commands server\r\n");

  if (!coapPeer) {
    LOG(L_NOTICE, "CoAPPeer - Not yet initialized - starting from scratch\r\n");
    return initCoAPPeer();
  }
  owl_time_t timeout = 0;
  int retries        = BREAKOUT_INIT_CONNECTION_RETRIES;

  if (!coapPeer->reinitialize()) GOTOERR(error);

  LOG(L_NOTICE, ".. CoAPPeer - waiting for transport to be ready (DTLS handshake)\r\n");
  timeout = owl_time() + BREAKOUT_INIT_CONNECTION_TIMEOUT * 1000;
  while (!coapPeer->transportIsReady()) {
    // Need to spin, because otherwise tinydtls things don't get properly initialized.
    spin();
    delay(50);
    if (timeout < owl_time()) {
      if (retries <= 0) {
        LOG(L_ERR, "Failed to re-initialize CoAP peer %d times in a row\r\n", BREAKOUT_INIT_CONNECTION_RETRIES);
        goto error;
      }
      LOG(L_NOTICE, ".. CoAPPeer - will try another reinitialization %d left\r\n", retries - 1);
      timeout = owl_time() + BREAKOUT_INIT_CONNECTION_TIMEOUT * 1000;
      if (!coapPeer->reinitialize()) GOTOERR(error);
      retries--;
    }
  }
  LOG(L_NOTICE, "... CoAP Peer is ready.\r\n");
  LOG(L_NOTICE, "        B R E A K O U T - re\r\n");
  LOG(L_NOTICE, "▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅\r\n");
  LOG(L_NOTICE, "▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅\r\n");
  LOG(L_NOTICE, "▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅\r\n");
  LOG(L_NOTICE, "▓▓▓▓▓▓▓▓   ▓▓▓▓▓▓▓   ▓▓▓▓▓▓▓▓▓▓\r\n");
  LOG(L_NOTICE, "▒▒▒▒       ▒▒▒▒▒▒          ▒▒▒▒\r\n");
  LOG(L_NOTICE, "▒▒▒▒                       ▒▒▒▒\r\n");
  LOG(L_NOTICE, "░░░░                      ░░░░░\r\n");
  LOG(L_NOTICE, "                               \r\n");
  LOG(L_NOTICE, "                               \r\n");
  LOG(L_NOTICE, "                               \r\n");
  LOG(L_NOTICE, "                               \r\n");
  LOG(L_NOTICE, "                               \r\n");
  LOG(L_NOTICE, "               ⚪               \r\n");
  LOG(L_NOTICE, "     ▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁          \r\n");
  if (!coap_status) {
    coap_status = true;
    notifyConnectionStatusChanged();
  }
  return true;
error:
  LOG(L_ERR, "... CoAP Peer was not re-initialized correctly.\r\n");
  if (coap_status) {
    coap_status = false;
    notifyConnectionStatusChanged();
  }
  return false;
}



/*                      Main Functionality Loop                                */

void Breakout::spin() {
  /* Triggering polling on interval expiration */
  if (next_polling != 0 && next_polling <= owl_time()) checkForCommands();

  /* Take care of async modem events */
  owlModem->AT.spin();

  /* Take care of UDP/TCP data from the modem */
  owlModem->socket.handleWaitingData();

  /* Take care of CoAP (and also tinydtls) retransmissions */
  CoAPPeer::triggerPeriodicRetransmit();

#if TESTING_WITH_CLI == 1
  /* Enable also CLI, for intermediary testing */
  if (!owlModemCLI) owlModemCLI = owl_new OwlModemCLI(owlModem, &SerialDebugPort);
  cli_resume = owlModemCLI->handleUserInput(cli_resume);
#endif
}



command_status_code_e Breakout::sendTextCommand(const char *buf) {
  str payload = {.s = (char *)buf, .len = strlen(buf)};
  return sendCommand(payload, false);
}

command_status_code_e Breakout::sendBinaryCommand(const char *buf, size_t bufSize) {
  str payload = {.s = (char *)buf, .len = bufSize};
  return sendCommand(payload, true);
}

command_status_code_e Breakout::sendCommand(str cmd, bool isBinary) {
  if (getConnectionStatus() != CONNECTION_STATUS_REGISTERED_AND_CONNECTED) {
    LOG(L_ERR, "Current Connection-Status is offline - please try again later\r\n");
    return COMMAND_STATUS_ERROR;
  }
  if (cmd.len > 140) {
    LOG(L_ERR, "Command of %d bytes longer than maximum acceptable of 140 bytes\r\n", cmd.len);
    return COMMAND_STATUS_COMMAND_TOO_LONG;
  }

  CoAPMessage request = CoAPMessage(CoAP_Type__Non_Confirmable, CoAP_Code_Class__Request,
                                    CoAP_Code_Detail__Request__POST, coapPeer->getNextMessageId());
  if (!request.addOptionUriPath("v1")) {
    LOG(L_ERR, "Error adding UriPath\r\n");
    goto error;
  }
  if (!request.addOptionUriPath("Commands")) {
    LOG(L_ERR, "Error adding UriPath\r\n");
    goto error;
  }
  if (!request.addOptionUriQuery(iccid)) {
    LOG(L_ERR, "Error adding UriQuery\r\n");
    goto error;
  }
  if (!request.addOptionContentFormat(isBinary ? CoAP_Content_Format__application_octet_stream :
                                                 CoAP_Content_Format__text_plain_charset_utf8)) {
    LOG(L_ERR, "Error adding ContentFormat\r\n");
    goto error;
  }
  if (!request.addOptionTwilioHostDeviceInformation(owlModem->getShortHostDeviceInformation())) {
    LOG(L_ERR, "Error adding Twilio-HostDevice-Information\r\n");
    goto error;
  }
  request.payload = cmd;
  if (!coapPeer->sendUnreliably(&request)) {
    LOG(L_ERR, "Error sending request unreliably\r\n");
    goto error;
  }

  request.destroy();
  return COMMAND_STATUS_OK;
error:
  request.destroy();
  return COMMAND_STATUS_ERROR;
}

command_status_code_e Breakout::sendTextCommandWithReceiptRequest(const char *buf,
                                                                  BreakoutCommandReceiptCallback_f callback,
                                                                  void *callback_parameter) {
  str payload = {.s = (char *)buf, .len = strlen(buf)};
  return sendCommandWithReceiptRequest(payload, callback, callback_parameter, false);
}

command_status_code_e Breakout::sendBinaryCommandWithReceiptRequest(const char *buf, size_t bufSize,
                                                                    BreakoutCommandReceiptCallback_f callback,
                                                                    void *callback_parameter) {
  str payload = {.s = (char *)buf, .len = bufSize};
  return sendCommandWithReceiptRequest(payload, callback, callback_parameter, true);
}


typedef struct {
  BreakoutCommandReceiptCallback_f callback;
  void *callback_parameter;
} receipt_t;

void Breakout::callback_commandReceipt(CoAPPeer *peer, coap_message_id_t message_id, void *cb_param,
                                       coap_client_transaction_event_e event, CoAPMessage *message) {
  if (!cb_param) return;
  receipt_t *receipt = (receipt_t *)cb_param;
  command_receipt_code_e receipt_code;
  switch (event) {
    case CoAP_Client_Transaction_Event__ACK:
      receipt_code = COMMAND_RECEIPT_CONFIRMED_DELIVERY;
      break;
    case CoAP_Client_Transaction_Event__Canceled:
      receipt_code = COMMAND_RECEIPT_CANCELED;
      break;
    case CoAP_Client_Transaction_Event__Timeout:
      receipt_code = COMMAND_RECEIPT_TIMEOUT;
      break;
    case CoAP_Client_Transaction_Event__RST:
    default:
      receipt_code = COMMAND_RECEIPT_SERVER_ERROR;
      break;
  }
  if (receipt->callback) (receipt->callback)(receipt_code, receipt->callback_parameter);
  owl_free(receipt);
}

command_status_code_e Breakout::sendCommandWithReceiptRequest(str cmd, BreakoutCommandReceiptCallback_f callback,
                                                              void *callback_parameter, bool isBinary) {
  receipt_t *receipt = 0;
  if (getConnectionStatus() != CONNECTION_STATUS_REGISTERED_AND_CONNECTED) {
    LOG(L_ERR, "Current Connection-Status is offline - please try again later\r\n");
    return COMMAND_STATUS_ERROR;
  }
  if (cmd.len > 140) {
    LOG(L_ERR, "Command of %d bytes longer than maximum acceptable of 140 bytes\r\n", cmd.len);
    return COMMAND_STATUS_COMMAND_TOO_LONG;
  }

  CoAPMessage request = CoAPMessage(CoAP_Type__Confirmable, CoAP_Code_Class__Request, CoAP_Code_Detail__Request__POST,
                                    coapPeer->getNextMessageId());
  if (!request.addOptionUriPath("v1")) {
    LOG(L_ERR, "Error adding UriPath\r\n");
    goto error;
  }
  if (!request.addOptionUriPath("Commands")) {
    LOG(L_ERR, "Error adding UriPath\r\n");
    goto error;
  }
  if (!request.addOptionUriQuery(iccid)) {
    LOG(L_ERR, "Error adding UriQuery\r\n");
    goto error;
  }
  if (!request.addOptionContentFormat(isBinary ? CoAP_Content_Format__application_octet_stream :
                                                 CoAP_Content_Format__text_plain_charset_utf8)) {
    LOG(L_ERR, "Error adding ContentFormat\r\n");
    goto error;
  }
  if (!request.addOptionTwilioHostDeviceInformation(owlModem->getShortHostDeviceInformation())) {
    LOG(L_ERR, "Error adding Twilio-HostDevice-Information\r\n");
    goto error;
  }
  request.payload = cmd;

  if (callback) {
    receipt = (receipt_t *)owl_malloc(sizeof(receipt_t));
    if (!receipt) goto error;
    receipt->callback           = callback;
    receipt->callback_parameter = callback_parameter;
  }
  if (!coapPeer->sendReliably(&request, callback_commandReceipt, receipt)) {
    LOG(L_ERR, "Error sending request unreliably\r\n");
    goto error;
  }
  receipt = 0;

  request.destroy();
  return COMMAND_STATUS_OK;
error:
  if (receipt) owl_free(receipt);
  request.destroy();
  return COMMAND_STATUS_ERROR;
}

void Breakout::callback_checkForCommands(CoAPPeer *peer, coap_message_id_t message_id, void *cb_param,
                                         coap_client_transaction_event_e event, CoAPMessage *message) {
  int isRetry        = (int)cb_param;
  Breakout *breakout = &Breakout::getInstance();
  switch (event) {
    case CoAP_Client_Transaction_Event__ACK:
      LOG(L_INFO, "Received ACK for polling - transport is working\r\n");
      break;
    case CoAP_Client_Transaction_Event__RST:
      LOG(L_INFO, "Received RST for polling - transport is working, but the server is not accepting the command\r\n");
      break;
    case CoAP_Client_Transaction_Event__Timeout:
      if (isRetry) {
        LOG(L_NOTICE, "Timeout after transport re-initialization - giving up, will retry at next poll interval\r\n");
      } else if (!breakout->reinitializeTransport()) {
        LOG(L_ERR, "Transport re-initialization failed\r\n");
      } else {
        // re-trigger Polling
        breakout->last_polling = owl_time() - BREAKOUT_POLLING_INTERVAL_MINIMUM * 1000;
        if (!breakout->checkForCommands()) {
          LOG(L_ERR, "Transport reinitialized, immediate re-polling failed\r\n");
        } else {
          LOG(L_INFO, "Transport reinitialized and re-polling started\r\n");
        }
      }
      break;
    case CoAP_Client_Transaction_Event__Canceled:
      // ignore
      break;
    default:
      LOG(L_ERR, "Not handled event %d\r\n", event);
  }
}

bool Breakout::checkForCommands() {
  return this->checkForCommands(false);
}

bool Breakout::checkForCommands(bool isRetry) {
  owl_time_t now = owl_time();
  if (getConnectionStatus() != CONNECTION_STATUS_REGISTERED_AND_CONNECTED) {
    // If the connection was dead for too long, try to automatically reinitialize it
    if (getConnectionStatus() == CONNECTION_STATUS_REGISTERED_NOT_CONNECTED &&
        now >= last_coap_status_connected + BREAKOUT_REINIT_CONNECTION_INTERVAL * 1000) {
      // Reset this timer, so that we won't try again for another interval
      last_coap_status_connected = now;
      if (reinitializeTransport()) {
        // Success, new connection is up
        goto recovered_connection;
      }
    }
    // If called on at the polling interval, back off a little, otherwise notify the user
    if (next_polling < now && next_polling != 0) {
      // This was a call on the timer - delay the call a bit
      next_polling = now + 5 * 1000;
    } else {
      LOG(L_ISSUE, "Current Connection-Status is offline - please try again later\r\n");
    }
    return false;
  }
recovered_connection:
  if (last_polling != 0 && now - last_polling < BREAKOUT_POLLING_INTERVAL_MINIMUM * 1000) {
    LOG(L_WARN, "Polling to often! Last was just %d seconds ago, which is less than %d.\r\n",
        (int)(now - last_polling) / 1000, (int)BREAKOUT_POLLING_INTERVAL_MINIMUM);
    return false;
  }

  CoAPMessage request = CoAPMessage(CoAP_Type__Confirmable, CoAP_Code_Class__Request, CoAP_Code_Detail__Request__POST,
                                    coapPeer->getNextMessageId());
  request.token       = coapPeer->getNextToken(&request.token_length);
  if (!request.addOptionUriPath("v1")) {
    LOG(L_ERR, "Error adding UriPath\r\n");
    goto error;
  }
  if (!request.addOptionUriPath("Heartbeats")) {
    LOG(L_ERR, "Error adding UriPath\r\n");
    goto error;
  }
  if (!request.addOptionUriQuery(iccid)) {
    LOG(L_ERR, "Error adding UriQuery\r\n");
    goto error;
  }
  if (!request.addOptionTwilioHostDeviceInformation(owlModem->getShortHostDeviceInformation())) {
    LOG(L_ERR, "Error adding Twilio-HostDevice-Information\r\n");
    goto error;
  }
  if (!coapPeer->sendReliably(&request, callback_checkForCommands, (void *)(isRetry ? 1 : 0))) {
    LOG(L_ERR, "Error sending request unreliably\r\n");
    goto error;
  }

  /* Reset the polling interval - doesn't matter if this was called manually or on interval
   * - after error label, to avoid hammering this on errors */
  last_polling = now;
  if (next_polling != 0) {
    next_polling = now + polling_interval * 1000;
    LOG(L_INFO, "Sent a POST /v1/Heartbeats - next one is in %d seconds\r\n", (next_polling - now) / 1000);
  } else {
    LOG(L_INFO, "Sent a POST /v1/Heartbeats\r\n");
  }


  request.destroy();
  return true;
error:
  /* Reset the polling interval - doesn't matter if this was called manually or on interval
   * - after error label, to avoid hammering this on errors */
  last_polling = now;
  if (next_polling != 0) next_polling = now + polling_interval * 1000;

  request.destroy();
  return false;
}

bool Breakout::hasWaitingCommand() {
  return (commands.head != 0);
}

command_status_code_e Breakout::receiveCommand(const size_t maxBufSize, char *buf, size_t *bufSize, bool *isBinary) {
  if (!hasWaitingCommand()) {
    return COMMAND_STATUS_NO_COMMAND_WAITING;
  }
  if (!bufSize && buf) {
    LOG(L_ERR,
        "Must provide a non-null bufSize parameter, if you provide a buf. To simply drain the command pipe, call with "
        "both buf and bufSize to null\r\n");
    return COMMAND_STATUS_ERROR;
  }

  breakout_command_t *command = commands.head;
  if (command->command.len > maxBufSize) {
    return COMMAND_STATUS_BUFFER_TOO_SMALL;
  }

  if (buf) {
    memcpy(buf, command->command.s, command->command.len);
    *bufSize = command->command.len;
    // Padding with zero, just in case the users are not careful
    if (*bufSize < maxBufSize) buf[*bufSize] = 0;
  }
  if (isBinary) *isBinary = command->isBinary;

  WL_DELETE(&commands, command);
  commands.space_left++;
  WL_FREE(command, breakout_command_list_t);

  return COMMAND_STATUS_OK;
}

bool Breakout::getGNSSData(gnss_data_t *out_gnss_data) {
  bool ret = owlModem->gnss.getGNSSData(out_gnss_data);
  if (ret) {
    LOG(L_DBG, "Current GNSS data:\r\n");
    owlModem->gnss.logGNSSData(L_DBG, *out_gnss_data);
  }
  return ret;
}



/*                 Internal methods   */


void Breakout::notifyConnectionStatusChanged() {
  connection_status_e status = getConnectionStatus();
  if (status == CONNECTION_STATUS_REGISTERED_AND_CONNECTED) last_coap_status_connected = owl_time();
  if (connection_handler) (connection_handler)(status);
}



/*                     Handlers - OwlModem                 */

static int breakout_pin_count = 0;

void Breakout::handler_PIN(str message) {
  str sim_pin = {.s = "0000", .len = 4};
  LOG(L_CLI, "\r\n>>>\r\n>>>PIN>>> %.*s\r\n>>>\r\n\r\n", message.len, message.s);
  if (str_equalcase_char(message, "READY")) {
    /* Seems fine */
  } else if (str_equalcase_char(message, "SIM PIN")) {
    /* The card needs the PIN */
    breakout_pin_count++;
    if (breakout_pin_count > 1) {
      LOG(L_CLI, "Trying to avoid a PIN lock - too many attempts to enter the PIN\r\n");
    } else {
      LOG(L_CLI, "Verifying PIN...\r\n");
      if (getInstance().owlModem->SIM.verifyPIN(sim_pin)) {
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

void Breakout::handler_NetworkRegistrationStatusChange(at_creg_stat_e stat, uint16_t lac, uint32_t ci,
                                                       at_creg_act_e act) {
  LOG(L_INFO,
      "\r\n>>>\r\n>>>URC-Network>>> Change in registration: Status=%d(%s) LAC=0x%04x CI=0x%08x "
      "Act=%d(%s)\r\n>>>\r\n\r\n",
      stat, at_creg_stat_text(stat), lac, ci, act, at_creg_act_text(act));
  // For NB-IoT, I think this is not registering, so ignore it - relying on EPS
}

void Breakout::handler_GPRSRegistrationStatusChange(at_cgreg_stat_e stat, uint16_t lac, uint32_t ci, at_cgreg_act_e act,
                                                    uint8_t rac) {
  LOG(L_INFO,
      "\r\n>>>\r\n>>>URC-GPRS>>> Change in registration: Status=%d(%s) LAC=0x%04x CI=0x%08x Act=%d(%s) "
      "RAC=0x%02x\r\n>>>\r\n\r\n",
      stat, at_cgreg_stat_text(stat), lac, ci, act, at_cgreg_act_text(act), rac);
  // For NB-IoT, I think this is registering, but so is EPS - relying on EPS
}

void Breakout::handler_EPSRegistrationStatusChange(at_cereg_stat_e stat, uint16_t lac, uint32_t ci, at_cereg_act_e act,
                                                   at_cereg_cause_type_e cause_type, uint32_t reject_cause) {
  LOG(L_INFO,
      "\r\n>>>\r\n>>>URC-EPS>>> Change in registration: Status=%d(%s) LAC=0x%04x CI=0x%08x Act=%d(%s) "
      "Cause-Type=%d(%s) Reject-Cause=%u\r\n>>>\r\n\r\n",
      stat, at_cereg_stat_text(stat), lac, ci, act, at_cereg_act_text(act), cause_type,
      at_cereg_cause_type_text(cause_type), reject_cause);

  Breakout *breakout = &Breakout::getInstance();

  if (breakout->eps_registration_status != stat) {
    breakout->eps_registration_status = stat;
    breakout->notifyConnectionStatusChanged();
  }
}

void Breakout::handler_UDPData(uint8_t socket, str remote_ip, uint16_t remote_port, str data) {
  LOG(L_INFO,
      "\r\n>>>\r\n>>>URC-UDP-Data>>> Received UDP data from socket=%d remote_ip=%.*s remote_port=%u of %d bytes\r\n",
      socket, remote_ip.len, remote_ip.s, remote_port, data.len);
  LOGSTR(L_DBG, data);
  LOG(L_DBG, ">>>\r\n");
}

void Breakout::handler_SocketClosed(uint8_t socket) {
  LOG(L_INFO, "\r\n>>>\r\n>>>URC-Socket-Closed>>> Socket Closed socket=%d", socket);
}



/*                     Handlers - CoAP                 */

void Breakout::handler_CoAPStatelessMessage(CoAPPeer *peer, CoAPMessage *message) {
  LOG(L_DBG, "\r\n>>>\r\n>>>Rx-CoAP-Stateless>>> From %.*s:%u\r\n", peer->remote_ip.len, peer->remote_ip.s,
      peer->remote_port);
  if (message) message->log(L_DBG);
  LOG(L_DBG, ">>>\r\n");
}

void Breakout::handler_CoAPDTLSEvent(CoAPPeer *peer, dtls_alert_level_e level, dtls_alert_description_e code) {
  LOG(L_INFO, "\r\n>>>\r\n>>>DTLS-Event>>> From %.*s:%u - level %d (%s) code %d (%s)\r\n>>>\r\n", peer->remote_ip.len,
      peer->remote_ip.s, peer->remote_port, level, dtls_alert_level_text(level), code,
      dtls_alert_description_text(code));
  if (level == DTLS_Alert_Description__fatal) {
    // Reinitialize CoAPPeer
    Breakout *breakout = &Breakout::getInstance();
    if (breakout->coap_status) {
      if (!(breakout->coap_status = breakout->reinitializeTransport())) {
        breakout->coap_status = false;
        breakout->notifyConnectionStatusChanged();
      }
    }
  }
}

/**
 * Internal Rx command handler
 * @param data
 * @param isBinary
 * @return true if to ACK it, or false if not (e.g. error occured, so maybe try again on retransmission)
 */
bool Breakout::receivedCommandInternal(str data, bool isBinary) {
  breakout_command_t *cmd = 0;

  if (command_handler) {
    // If a handler is set, then skip the queue of commands and deliver directly
    (command_handler)(data.s, data.len, isBinary);
    return true;
  }

  // Otherwise queue the command
  if (commands.space_left <= 0) {
    LOG(L_WARN, "Maximum commands queued - will drop the oldest one, to make room for a newly received one\r\n");
    cmd = commands.head;
    WL_DELETE(&commands, cmd);
    commands.space_left++;
    WL_FREE(cmd, breakout_command_list_t);
  }

  WL_NEW(cmd, breakout_command_list_t);
  str_dup(cmd->command, data);
  cmd->isBinary = isBinary;
  WL_APPEND(&commands, cmd);
  commands.space_left--;

  return true;
out_of_memory:
  WL_FREE(cmd, breakout_command_list_t);
  return false;
}

coap_handler_follow_up_e Breakout::handler_CoAPRequest(CoAPPeer *peer, CoAPMessage *request) {
  uint64_t content_format = CoAP_Content_Format__text_plain_charset_utf8;
  str uri_path            = {0};
  Breakout *instance      = &Breakout::getInstance();

  switch (request->code_detail) {
    case CoAP_Code_Detail__Request__GET:
      if (request->getNextOptionUriPath(&uri_path, 0) && str_equalcase_char(uri_path, "HostDeviceInformation")) {
        // Identified as request for the full HostDeviceInformation
        LOG(L_INFO, "Handling GET identified as HostDeviceInformation request\r\n");
        CoAPMessage response = CoAPMessage(
            request, request->type == CoAP_Type__Confirmable ? CoAP_Type__Acknowledgement : CoAP_Type__Non_Confirmable,
            CoAP_Code_Class__Response, CoAP_Code_Detail__Response__Content);
        response.addOptionContentFormat(CoAP_Content_Format__text_plain_charset_utf8);
        response.payload = instance->owlModem->getHostDeviceInformation();
        instance->coapPeer->sendUnreliably(&response);
        return CoAP__Handler_Followup__Do_Nothing;
      }
      // Otherwise - not supported
      LOG(L_WARN, "Not handled CoAP Request\r\n");
      request->log(L_WARN);
      return CoAP__Handler_Followup__Send_Reset;

    case CoAP_Code_Detail__Request__POST:
      LOG(L_INFO, "Handling POST\r\n");
      if (request->getNextOptionContentFormat(&content_format, 0) &&
          (content_format == CoAP_Content_Format__text_plain_charset_utf8 ||
           content_format == CoAP_Content_Format__application_octet_stream) &&
          request->getNextOptionUriPath(&uri_path, 0) && str_equalcase_char(uri_path, "Commands")) {
        // Identified as To-SIM Command
        LOG(L_INFO, "Handling POST identified as To-SIM Command\r\n");
        if (instance->receivedCommandInternal(request->payload,
                                              content_format == CoAP_Content_Format__application_octet_stream))
          return CoAP__Handler_Followup__Send_Acknowledgement;
        else
          return CoAP__Handler_Followup__Do_Nothing;
      }
      // Otherwise - not supported
      LOG(L_WARN, "Not handled CoAP Request\r\n");
      request->log(L_WARN);
      return CoAP__Handler_Followup__Send_Reset;

    default:
      LOG(L_WARN, "Not handled CoAP Request %d.%02d - %s\r\n", request->code_class, request->code_detail,
          coap_code_text(request->code_class, request->code_detail));
      return CoAP__Handler_Followup__Send_Reset;
  }
}

coap_handler_follow_up_e Breakout::handler_CoAPResponse(CoAPPeer *peer, CoAPMessage *response) {
  Breakout *instance = &Breakout::getInstance();
  switch (response->code_detail) {
    case CoAP_Code_Detail__Response__Created:
      if (response->token == instance->last_polling_token) {
        response->getNextOptionTwilioQueuedCommandCount(&instance->queued_command_count, 0);
        LOG(L_INFO, "Received 2.01 Response Created for /v1/Heartbeats - Queued-Command-Count=%llu\r\n",
            instance->queued_command_count);
        return CoAP__Handler_Followup__Send_Acknowledgement;
      }
      LOG(L_WARN, "Received 2.01 Response Created for unknown Request\r\n");
      response->log(L_WARN);
      return CoAP__Handler_Followup__Send_Reset;

    default:
      LOG(L_WARN, "Not handled CoAP Response %d.%02d - %s\r\n", response->code_class, response->code_detail,
          coap_code_text(response->code_class, response->code_detail));
      response->log(L_WARN);
      return CoAP__Handler_Followup__Send_Reset;
      break;
  }
}
