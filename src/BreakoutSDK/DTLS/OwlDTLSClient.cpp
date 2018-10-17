/*
 * OwlDTLSClient.cpp
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
 * \file OwlDTLS.cpp - A small interface for tinydtls, mostly for testing stuff out
 */

#include "OwlDTLSClient.h"



OwlDTLSClient::OwlDTLSClient() {
  dtls_init();
}

OwlDTLSClient::OwlDTLSClient(str psk_id, str psk_key) {
  this->psk_id.len = psk_id.len > DTLS_PSK_MAX_CLIENT_IDENTITY_LEN ? DTLS_PSK_MAX_CLIENT_IDENTITY_LEN : psk_id.len;
  memcpy(this->psk_id.s, psk_id.s, this->psk_id.len);
  this->psk_key.len = psk_key.len > DTLS_PSK_MAX_KEY_LEN ? DTLS_PSK_MAX_KEY_LEN : psk_key.len;
  memcpy(this->psk_key.s, psk_key.s, this->psk_key.len);
  dtls_init();
}

OwlDTLSClient::~OwlDTLSClient() {
  if (owlModem) {
    if (socket_id) owlModem->socket.close(socket_id);
  }
  if (this->dtls_context) dtls_free_context(this->dtls_context);
}


/*
 * Callbacks for linking to tinydtls - static
 */

/**
 * Function called by tinydtls to send data out
 * @param ctx
 * @param session
 * @param buf
 * @param len
 * @return
 */
static int sendToPeer(struct dtls_context_t *ctx, session_t *session, uint8 *buf, size_t len) {
  LOG(L_DBG, "Called to send out %d bytes\r\n", len);
  if (!ctx || !session || !dtls_get_app_data(ctx)) {
    LOG(L_WARN, "Null parameter(s)\r\n");
    return 0;
  }
  OwlDTLSClient *owlDTLS = (OwlDTLSClient *)dtls_get_app_data(ctx);

  str data = {.s = (char *)buf, .len = len};
  int cnt = owlDTLS->sendRawData(data);
  if (cnt != len) {
    LOG(L_ERR, "Error on sending data\r\n");
    return 0;
  }

  return cnt; /* number of sent bytes, or < 0 on error */
}

/**
 * Function called by tinydtls to pass out data received
 * @param ctx
 * @param session
 * @param buf
 * @param len
 * @return
 */
static int readFromPeer(struct dtls_context_t *ctx, session_t *session, uint8 *buf, size_t len) {
  if (!ctx || !session || !dtls_get_app_data(ctx)) {
    LOG(L_WARN, "Null parameter(s)\r\n");
    return 0;
  }
  OwlDTLSClient *owlDTLS = (OwlDTLSClient *)dtls_get_app_data(ctx);

  LOG(L_NOTICE, "Received %d bytes over DTLS\r\n", len);

  str data = {.s = (char *)buf, .len = len};

  if (!owlDTLS->fireHandlerData(session, data)) {
    LOG(L_NOTICE, "Received DTLS data of %d bytes - No handler - set one to receive this data in your application\r\n",
        data.len);
    LOGSTR(L_NOTICE, data);
  }
  return 1; /* ignored */
}

int OwlDTLSClient::fireHandlerData(session_t *session, str plaintext) {
  if (!this->handler_data) return 0;
  (this->handler_data)(this, session, plaintext);
  return 1;
}


/**
 * Function called by tinydtls to pass out events received
 * @param ctx
 * @param session
 * @param level
 * @param code
 * @return
 */
static int handleEvent(struct dtls_context_t *ctx, session_t *session, dtls_alert_level_t level, unsigned short code) {
  if (!ctx || !session || !dtls_get_app_data(ctx)) {
    LOG(L_WARN, "Null parameter(s)\r\n");
    return 0;
  }
  OwlDTLSClient *owlDTLS = (OwlDTLSClient *)dtls_get_app_data(ctx);


  if (!owlDTLS->fireHandlerEvent(session, (dtls_alert_level_e)level, (dtls_alert_description_e)code)) {
    LOG(L_NOTICE,
        "Received DTLS event level %d (%s) code %d (%s) - No handler - set one to receive this event in your "
        "application\r\n",
        level, dtls_alert_level_text((dtls_alert_level_e)level), code,
        dtls_alert_description_text((dtls_alert_description_e)code));
  }
  return 1; /* ignored */
}

int OwlDTLSClient::fireHandlerEvent(session_t *session, dtls_alert_level_e level,
                                    dtls_alert_description_e description) {
  this->last_status = description;
  if (!this->handler_event) return 0;
  (this->handler_event)(this, session, level, description);
  return 1;
}

/**
 * Function used by tinydtls when it needs PSK-related information
 * @param ctx
 * @param session
 * @param type
 * @param desc
 * @param desc_len
 * @param result
 * @param result_length
 * @return
 */
static int getPSKInfo_static(struct dtls_context_t *ctx, const session_t *session, dtls_credentials_type_t type,
                             const unsigned char *desc, size_t desc_len, unsigned char *result, size_t result_length) {
  if (!ctx || !session || !dtls_get_app_data(ctx)) {
    LOG(L_WARN, "Null parameter(s)\r\n");
    return 0;
  }
  OwlDTLSClient *owlDTLS = (OwlDTLSClient *)dtls_get_app_data(ctx);

  return owlDTLS->getPSKInfo(session, type, desc, desc_len, result, result_length);
}

/**
 * The internal function to resolve PSK-related information
 * @param ctx
 * @param session
 * @param type
 * @param desc
 * @param desc_len
 * @param result
 * @param result_length
 * @return
 */
int OwlDTLSClient::getPSKInfo(const session_t *session, dtls_credentials_type_t type, const unsigned char *desc,
                              size_t desc_len, unsigned char *result, size_t result_length) {
  switch (type) {
    case DTLS_PSK_IDENTITY:
      if (desc_len) LOG(L_DBG, "got psk_identity_hint: '%.*s'\r\n", desc_len, desc);
      if (result_length < psk_id.len) {
        LOG(L_ERR, "Cannot set psk_identity -- buffer too short %d < %d\r\n", result_length, psk_id.len);
        return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
      }
      memcpy(result, psk_id.s, psk_id.len);
      return psk_id.len;
      break;
    case DTLS_PSK_KEY:
      if (desc_len != psk_id.len || memcmp(psk_id.s, desc, desc_len) != 0) {
        LOG(L_ERR, "PSK for unknown id [%.*s] != [%.*s] requested, exiting\r\n", desc_len, desc, psk_id.len, psk_id.s);
        return dtls_alert_fatal_create(DTLS_ALERT_ILLEGAL_PARAMETER);
      } else if (result_length < psk_key.len) {
        LOG(L_ERR, "Cannot set psk_key -- buffer too short %d < %d\r\n", result_length, psk_key.len);
        return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
      }
      memcpy(result, psk_key.s, psk_key.len);
      return psk_key.len;
      break;
    default:
      LOG(L_ERR, "Unsupported request type: %d\r\n", type);
  }

  return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR); /* number of bytes written to result, or < 0 on error */
}



// static int getECDSAKey_static(struct dtls_context_t *ctx, const session_t *session, const dtls_ecdsa_key_t **result)
// {
//  if (!ctx || !session || !dtls_get_app_data(ctx)) {
//    LOG(L_WARN, "Null parameter(s)\r\n");
//    return 0;
//  }
//  OwlDTLSClient *owlDTLS = (OwlDTLSClient *)dtls_get_app_data(ctx);
//
//  // TODO - fill ECSDA Key
//
//  return -1; /* 0 if result set, or < 0 on error */
//}
//
// static int verifyECDSAKey_static(struct dtls_context_t *ctx, const session_t *session, const unsigned char
// *other_pub_x,
//                                 const unsigned char *other_pub_y, size_t key_size) {
//  if (!ctx || !session || !dtls_get_app_data(ctx)) {
//    LOG(L_WARN, "Null parameter(s)\r\n");
//    return 0;
//  }
//  OwlDTLSClient *owlDTLS = (OwlDTLSClient *)dtls_get_app_data(ctx);
//
//  // TODO - verify ECSDA Key
//
//  return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR); /* 0 if public key matches, or < 0 on error */
//}



/**
 * tinydtls callback structure
 */
dtls_handler_t OwlDTLS_callbacks = {

    .write = sendToPeer,
    .read  = readFromPeer,
    .event = handleEvent,

#ifdef DTLS_PSK
    .get_psk_info = getPSKInfo_static,
#endif /* DTLS_PSK */

#ifdef DTLS_ECC
    .get_ecdsa_key    = 0 /* Masking this, to disabled declaring it to the other side getECDSAKey_static */,
    .verify_ecdsa_key = 0 /* Masking this, to disabled declaring it to the other side verifyECDSAKey_static */,
#endif /* DTLS_ECC */

};

int OwlDTLSClient::init(OwlModem *owlModem) {
  dtls_context = dtls_new_context((void *)this);
  if (!dtls_context) {
    LOG(L_ERR, "Error creating DTLS context\r\n");
    return 0;
  }

  dtls_set_handler(dtls_context, &OwlDTLS_callbacks);

  if (!owlModem) {
    LOG(L_ERR, "Need the OwlModem link for communication purposes\r\n");
    return 0;
  }
  this->owlModem = owlModem;

  return 1;
}



int OwlDTLSClient::connect(uint16_t local_port, str remote_ip, uint16_t remote_port) {
  if (!this->owlModem) {
    LOG(L_ERR, "Not initialized correctly - owlModem is null\r\n");
    return 0;
  }

  str token = {0};
  if (str_find_char(remote_ip, ":") < 0) {
    /* IPv4 */
    this->dtls_dst.size = IP_Address__IPv4;
    for (int i                    = 0; i < 4 && str_tok(remote_ip, ".", &token); i++)
      this->dtls_dst.addr.ipv4[i] = str_to_uint32_t(token, 10);
  } else {
    /* IPv6 */
    this->dtls_dst.size = IP_Address__IPv6;
    int x               = 0;
    LOG(L_ERR, "Not yet implemented a proper parsing for IPv6 - TODO\r\n");
    return 0;
  }
  this->dtls_dst.port    = remote_port;
  this->dtls_dst.ifindex = 0; /* not used for now */

  this->remote_ip.len = remote_ip.len > 64 ? 64 : remote_ip.len;
  memcpy(this->remote_ip.s, remote_ip.s, remote_ip.len);
  this->remote_port = remote_port;

  if (socket_id != 255) {
    owlModem->socket.close(socket_id);
    socket_id = 255;
  }
  if (!owlModem->socket.openListenConnectUDP(local_port, remote_ip, remote_port, OwlDTLSClient::handleRawData,
                                             &this->socket_id)) {
    if (!owlModem->socket.openConnectUDP(remote_ip, remote_port, OwlDTLSClient::handleRawData, &this->socket_id)) {
      LOG(L_ERR, "Error opening local socket towards %.*s:%u\r\n", remote_ip.len, remote_ip.s, remote_port);
      return 0;
    } else {
      LOG(L_WARN,
          "Potential error opening local socket towards %.*s:%u - listen failed, model wasn't blacklisted\r\n",
          remote_ip.len, remote_ip.s, remote_port);
    }
  }
  if (this->socket_id < 0 || this->socket_id >= MODEM_MAX_SOCKETS) {
    LOG(L_ERR, "Bad socket_id %d returned\r\n", this->socket_id);
    return 0;
  }
  OwlDTLSClient::socketMappings[this->socket_id] = this;

  if (dtls_connect(this->dtls_context, &this->dtls_dst) < 0) {
    LOG(L_ERR, "Error on dtls_connect()\r\n");
    return 0;
  }

  return 1;
}

int OwlDTLSClient::close() {
  if (!this->dtls_context) {
    LOG(L_ERR, "DTLS context not created yet\r\n");
    return 0;
  }
  int err = dtls_close(this->dtls_context, &this->dtls_dst);
  LOG(L_NOTICE, "Close error=%d\r\n", err);
  return err >= 0;
}

int OwlDTLSClient::renegotiate() {
  if (!this->dtls_context) {
    LOG(L_ERR, "DTLS context not created yet\r\n");
    return 0;
  }
  int err = dtls_renegotiate(this->dtls_context, &this->dtls_dst);
  LOG(L_NOTICE, "Renegotiate error=%d\r\n", err);
  return err >= 0;
}

int OwlDTLSClient::rehandshake() {
  if (!this->dtls_context) {
    LOG(L_ERR, "DTLS context not created yet\r\n");
    return 0;
  }
  int err = dtls_connect(this->dtls_context, &this->dtls_dst);
  LOG(L_NOTICE, "Rehandshake error=%d\r\n", err);
  return err >= 0;
}

int OwlDTLSClient::sendRawData(str data) {
  if (!this->owlModem) {
    LOG(L_ERR, "Not initialized correctly - owlModem is null\r\n");
    return 0;
  }
  if (this->socket_id == 255) {
    LOG(L_ERR, "Socket not opened\r\n");
    return 0;
  }
  int out_bytes_sent = 0;
  if (!owlModem->socket.sendUDP(socket_id, data, &out_bytes_sent)) LOG(L_ERR, "Error sending data out\r\n");
  return out_bytes_sent;
}


OwlDTLSClient *OwlDTLSClient::socketMappings[] = {0};

void OwlDTLSClient::handleRawData(uint8_t socket, str remote_ip, uint16_t remote_port, str data) {
  if (socket < 0 || socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERR, "Bad socket_id %d returned\r\n", socket);
    return;
  }
  OwlDTLSClient *owlDTLS = OwlDTLSClient::socketMappings[socket];
  if (!owlDTLS) {
    LOG(L_ERR, "Socket Mappings for socket_id %d not initialized correctly\r\n", socket);
    return;
  }
  if (owlDTLS->socket_id != socket) {
    LOG(L_ERR, "Received data on incorrect socket %d != saved %d 0 - ignoring\r\n", socket, owlDTLS->socket_id);
    return;
  }
  if (remote_ip.len) {
    /* This was received with receiveFromUDP - check from IP:port */
    if (!str_equalcase(remote_ip, owlDTLS->remote_ip) || remote_port != owlDTLS->remote_port) {
      LOG(L_WARN, "Received data on socket %d from incorrect remote %.*s:%u, expected %.*s:%u - ignoring (attack?)\r\n",
          socket, remote_ip.len, remote_ip.s, remote_port, owlDTLS->remote_ip.len, owlDTLS->remote_ip.s,
          owlDTLS->remote_port);
      return;
    }
  }
  /* Pass to tinydtls */
  int err = dtls_handle_message(owlDTLS->dtls_context, &owlDTLS->dtls_dst, (uint8 *)data.s, data.len);
  if (err < 0) {
    LOG(L_NOTICE, "DTLS-Rx Failure - err %d handling Rx message\r\n", err);
  } else {
    LOG(L_DBG, "DTLS-Rx OK - handled Rx message successfully\r\n");
  }
}

int OwlDTLSClient::triggerPeriodicRetransmit() {
  clock_time_t now = 0;
  dtls_ticks(&now);
  if (now >= next_retransmit_timer) dtls_check_retransmit(dtls_context, &next_retransmit_timer);
  if (!next_retransmit_timer) next_retransmit_timer = now + 1000;

  return 1;
error:
  return 0;
}

int OwlDTLSClient::sendData(str plaintext) {
  int res;
  if (this->last_status != DTLS_Alert_Description__tinydtls_event_connected) {
    LOG(L_WARN, "Will try to send, but last status was %d (%.*s) != connected\r\n", this->last_status,
        dtls_alert_description_text(this->last_status));
  }
  res = dtls_write(this->dtls_context, &this->dtls_dst, (uint8 *)plaintext.s, plaintext.len);
  if (res >= 0) {
    LOG(L_DBG, "Successfully sent %d bytes\r\n", plaintext.len);
    return 1;
  } else {
    LOG(L_ERR, "Failed to send %d bytes\r\n", plaintext.len);
    return 0;
  }
}

void OwlDTLSClient::setDataHandler(OwlDTLS_DataHandler_f handler) {
  this->handler_data = handler;
}

void OwlDTLSClient::setEventHandler(OwlDTLS_EventHandler_f handler) {
  this->handler_event = handler;
}

dtls_alert_description_e OwlDTLSClient::getCurrentStatus() {
  return this->last_status;
}
