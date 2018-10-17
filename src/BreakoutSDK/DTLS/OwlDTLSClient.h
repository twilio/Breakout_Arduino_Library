/*
 * OwlDTLSClient.h
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
 * \file OwlDTLS.h - A small interface for tinydtls, mostly for testing stuff out
 */

#ifndef __OWL_DTLS_H__
#define __OWL_DTLS_H__

#include "enums.h"
#include "../modem/OwlModem.h"

extern "C" {
#include "../../tinydtls/dtls.h"
}



class OwlDTLSClient;

typedef void (*OwlDTLS_DataHandler_f)(OwlDTLSClient *owlDTLSClient, session_t *session, str plaintext);
typedef void (*OwlDTLS_EventHandler_f)(OwlDTLSClient *owlDTLSClient, session_t *session, dtls_alert_level_e level,
                                       dtls_alert_description_e code);



class OwlDTLSClient {
 public:
  OwlDTLSClient();
  OwlDTLSClient(str psk_id, str psk_key);
  ~OwlDTLSClient();


  /**
   * Initialize the DTLS context
   * @return 1 on success, 0 on failure
   */
  int init(OwlModem *owlModem);

  /**
   * Start a connection with a remote peer - initiates the connection, call the timer and injects packets to continue
   * @param local_port - local port, 0 if dynamically allocated
   * @param remote_ip - destination IP
   * @param remote_port - destination port
   * @return 1 on success, 0 on failure - this just
   */
  int connect(uint16_t local_port, str remote_ip, uint16_t remote_port);

  /* TODO - test */
  int close();

  /* TODO - test */
  int renegotiate();

  /* TODO - test */
  int rehandshake();

  /**
   * Send raw, plaintext data. Normally, this should only be called by the internal API. Use sendData() for your
   * sensitive application data, which needs to be encrypted!
   * @param data - data to send
   * @return number of bytes successfully sent
   */
  int sendRawData(str data);

  /**
   * Call this every once in a while, to do retransmission and to trigger receive
   * @return 1 on success, 0 on failure
   */
  int triggerPeriodicRetransmit();

  /**
   * Send data over DTLS. Before sending for the first time, wait for an event with code
   * DTLS_Alert_Description__tinydtls_event_connected, or check with getLastStatus() if that is the current
   * state. Otherwise, this would fail as the connection might not be fully up and negotiated.
   * @param plaintext
   * @return
   */
  int sendData(str plaintext);

  /**
   * Sets the handler for deciphered incoming data.
   * @param handler
   */
  void setDataHandler(OwlDTLS_DataHandler_f handler);

  /**
   * Sets the handler for events
   * @param handler
   */
  void setEventHandler(OwlDTLS_EventHandler_f handler);

  /**
   * Get the current status (last alert received) of this DTLS connection.
   * @return the DTLS Alert-Description last received
   */
  dtls_alert_description_e getCurrentStatus();


 private:
  dtls_context_t *dtls_context = 0;
  session_t dtls_dst           = {0}; /**< just a single destination for now, we could do more */

  char c_psk_id[DTLS_PSK_MAX_CLIENT_IDENTITY_LEN];
  str psk_id = {.s = c_psk_id, .len = 0};
  char c_psk_key[DTLS_PSK_MAX_KEY_LEN];
  str psk_key = {.s = c_psk_key, .len = 0};

  OwlModem *owlModem = 0;
  uint8_t socket_id  = 255;
  char c_remote_ip[64];
  str remote_ip = {.s = c_remote_ip, .len = 0};
  uint16_t remote_port = 0;

  clock_time_t next_retransmit_timer = 0;

  dtls_alert_description_e last_status = DTLS_Alert_Description__close_notify;

  OwlDTLS_DataHandler_f handler_data   = 0;
  OwlDTLS_EventHandler_f handler_event = 0;


  static OwlDTLSClient *socketMappings[MODEM_MAX_SOCKETS];
  static void handleRawData(uint8_t socket, str remote_ip, uint16_t remote_port, str data);


 public:
  // TODO - can we make these private maybe?
  int getPSKInfo(const session_t *session, dtls_credentials_type_t type, const unsigned char *desc, size_t desc_len,
                 unsigned char *result, size_t result_length);
  int fireHandlerData(session_t *session, str data);
  int fireHandlerEvent(session_t *session, dtls_alert_level_e level, dtls_alert_description_e description);
};

#endif
