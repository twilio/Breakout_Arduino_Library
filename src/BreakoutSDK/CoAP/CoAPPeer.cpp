/*
 * CoAPPeer.cpp
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
 * \file CoAP Peer
 */

#include "CoAPPeer.h"

#include "../utils/lists.h"


CoAPPeer **CoAPPeer::instances = 0;
int CoAPPeer::instances_cnt    = 0;



CoAPPeer::CoAPPeer(OwlModem *modem, uint16_t local_port, str remote_ip, uint16_t remote_port)
    : transport_type(CoAP_Transport__plaintext), owlModem(modem), local_port(local_port), remote_port(remote_port) {
  randomSeed(random(0xffffff) + millis());  // randomizing again, just in case the ANALOG_RND_PIN was connected
  last_message_id = random(0xFFFFu);
  last_token      = random(0xFFFFFF);
  str_dup(this->remote_ip, remote_ip);
  if (!addInstance()) {
    LOG(L_ERR, "Error adding instance in list\r\n");
    goto out_of_memory;
  }
  return;
out_of_memory:
  LOG(L_ERR, "Reached maximum number of supported concurrent CoAP clients.\r\n");
  return;
}

CoAPPeer::CoAPPeer(OwlModem *modem, str psk_id, str psk_key, uint16_t local_port, str remote_ip, uint16_t remote_port)
    : transport_type(CoAP_Transport__DTLS_PSK),
      owlModem(modem),
      local_port(local_port),
      remote_ip(remote_ip),
      remote_port(remote_port) {
  randomSeed(random(0xffffff) + millis());  // randomizing again, just in case the ANALOG_RND_PIN was connected
  last_message_id = random(0xFFFFu);
  last_token      = random(0xFFFFFF);
  str_dup(this->remote_ip, remote_ip);
  str_dup(this->psk_id, psk_id);
  str_dup(this->psk_key, psk_key);
  if (!addInstance()) {
    LOG(L_ERR, "Error adding instance in list\r\n");
    goto out_of_memory;
  }
  return;
out_of_memory:
  LOG(L_ERR, "Reached maximum number of supported concurrent CoAP clients.\r\n");
  return;
}

CoAPPeer::~CoAPPeer() {
  removeInstance();
  str_free(this->remote_ip);
  str_free(this->psk_id);
  str_free(this->psk_key);
  WL_FREE_ALL(&client_transactions, coap_client_transaction_list_t);
  WL_FREE_ALL(&server_transactions, coap_server_transaction_list_t);
  if (owlDTLSClient) {
    owlDTLSClient->close();
    delete owlDTLSClient;
    owlDTLSClient = 0;
  }
}



int CoAPPeer::initDTLSClient() {
  if (owlDTLSClient) {
    owlDTLSClient->close();
    delete owlDTLSClient;
    owlDTLSClient = 0;
  }
  owlDTLSClient = owl_new OwlDTLSClient(psk_id, psk_key);
  if (!owlDTLSClient) {
    LOG(L_ERR, "Error creating DTLS instance\r\n");
    return 0;
  }
  if (!owlDTLSClient->init(owlModem)) {
    LOG(L_CLI, "ERROR - internal\r\n");
    return 0;
  }
  owlDTLSClient->setDataHandler(CoAPPeer::handlerDTLSData);
  owlDTLSClient->setEventHandler(CoAPPeer::handlerDTLSEvent);
  return 1;
}


CoAPPeer *CoAPPeer::socketMappings[] = {0};

void CoAPPeer::handlePlaintextData(uint8_t socket, str remote_ip, uint16_t remote_port, str data) {
  if (socket < 0 || socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERR, "Bad socket_id %d returned\r\n", socket);
    return;
  }
  CoAPPeer *peer = CoAPPeer::socketMappings[socket];
  if (!peer) {
    LOG(L_ERR, "Socket Mappings for socket_id %d not initialized correctly\r\n", socket);
    return;
  }
  if (peer->socket_id != socket) {
    LOG(L_ERR, "Received data on incorrect socket %d != saved %d 0 - ignoring\r\n", socket, peer->socket_id);
    return;
  }
  if (remote_ip.len) {
    /* This was received with receiveFromUDP - check from IP:port */
    if (!str_equalcase(remote_ip, peer->remote_ip) || remote_port != peer->remote_port) {
      LOG(L_WARN, "Received data on socket %d from incorrect remote %.*s:%u, expected %.*s:%u - ignoring (attack?)\r\n",
          socket, remote_ip.len, remote_ip.s, remote_port, peer->remote_ip.len, peer->remote_ip.s, peer->remote_port);
      return;
    }
  }
  if (remote_ip.len && (!str_equalcase(remote_ip, peer->remote_ip) || remote_port != peer->remote_port)) {
    LOG(L_ERR, "CoAP-Rx Ignoring data coming from %.*s:%u, instead of expected %.*s:%u\r\n", remote_ip.len, remote_ip.s,
        remote_port, peer->remote_ip.len, peer->remote_ip.s, peer->remote_port);
  }
  if (!peer->handleRx(data)) {
    LOG(L_WARN, "CoAP-Rx Failure - err handling Rx message\r\n");
  } else {
    LOG(L_DBG, "CoAP-Rx OK - handled Rx message successfully\r\n");
  }
}

void CoAPPeer::handlerDTLSData(OwlDTLSClient *owlDTLSClient, session_t *session, str plaintext) {
  CoAPPeer *peer = 0;
  for (int i = 0; i < CoAPPeer::instances_cnt; i++)
    if (CoAPPeer::instances[i] != 0 && CoAPPeer::instances[i]->owlDTLSClient == owlDTLSClient) {
      peer = instances[i];
      break;
    }
  if (!peer) {
    LOG(L_ERR,
        "Instances mapping for owlDTLSClient not initialized correctly, or old DTLSClient instance sending this\r\n");
    return;
  }
  if (!peer->handleRx(plaintext)) {
    LOG(L_WARN, "CoAP-Rx Failure - err handling Rx message\r\n");
  } else {
    LOG(L_DBG, "CoAP-Rx OK - handled Rx message successfully\r\n");
  }
}

void CoAPPeer::handlerDTLSEvent(OwlDTLSClient *owlDTLSClient, session_t *session, dtls_alert_level_e level,
                                dtls_alert_description_e code) {
  CoAPPeer *peer = 0;
  for (int i = 0; i < CoAPPeer::instances_cnt; i++)
    if (CoAPPeer::instances[i] != 0 && CoAPPeer::instances[i]->owlDTLSClient == owlDTLSClient) {
      peer = instances[i];
      break;
    }
  if (!peer) {
    LOG(L_ERR, "Instances mapping for owlDTLSClient not initialized correctly\r\n");
    return;
  }
  if (peer->handler_dtls_event)
    (peer->handler_dtls_event)(peer, level, code);
  else
    LOG(L_WARN, "No handler set remote_ip=%.*s:%u - received DTLS event %d - %s / %d - %s\r\n", peer->remote_ip.len,
        peer->remote_ip.s, peer->remote_port, level, dtls_alert_level_text(level), code,
        dtls_alert_description_text(code));
}



int CoAPPeer::reinitializeTransport() {
  switch (this->transport_type) {
    case CoAP_Transport__plaintext:
      if (socket_id != 255) owlModem->socket.close(socket_id);
      if (!owlModem->socket.openListenConnectUDP(local_port, remote_ip, remote_port, CoAPPeer::handlePlaintextData,
                                                 &socket_id)) {
        if (!owlModem->socket.openConnectUDP(remote_ip, remote_port, CoAPPeer::handlePlaintextData, &socket_id)) {
          LOG(L_ERR, "Error opening local socket towards %.*s:%u\r\n", remote_ip.len, remote_ip.s, remote_port);
          return 0;
        } else {
          LOG(L_WARN,
              "Potential error opening local socket towards %.*s:%u - listen failed, model wasn't blacklisted\r\n",
              remote_ip.len, remote_ip.s, remote_port);
        }
      }
      CoAPPeer::socketMappings[socket_id] = this;
      return 1;
      break;
    case CoAP_Transport__DTLS_PSK:
      if (!owlDTLSClient) {
        if (!initDTLSClient()) {
          LOG(L_ERR, "remote_ip=%.*s:%u - owlDTLSClient initialization failed\r\n", remote_ip.len, remote_ip.s,
              remote_port);
          return 0;
        }
      }
      switch (owlDTLSClient->getCurrentStatus()) {
        case DTLS_Alert_Description__tinydtls_event_connected:
          // TODO: support
          if (!owlDTLSClient->renegotiate()) {
            LOG(L_ERR, "Error re-negotiating DTLS connection towards %.*s:%u\r\n", remote_ip.len, remote_ip.s,
                remote_port);
            return 0;
          }
          break;
        case DTLS_Alert_Description__tinydtls_event_renegotiate:
          // TODO: support
          break;
        case DTLS_Alert_Description__tinydtls_event_connect:
        default:
          // TODO: support
          // Cycle everything
          if (!initDTLSClient()) {
            LOG(L_ERR, "remote_ip=%.*s:%u - owlDTLSClient re-initialization failed\r\n", remote_ip.len, remote_ip.s,
                remote_port);
            return 0;
          }
          if (!owlDTLSClient->connect(local_port, remote_ip, remote_port)) {
            LOG(L_ERR, "Error opening DTLS connection towards %.*s:%u\r\n", remote_ip.len, remote_ip.s, remote_port);
            return 0;
          }
          break;
      }
      return 1;
      break;
    default:
      LOG(L_ERR, "Not implemented for transport_type %d\r\n", this->transport_type);
      return 0;
  }
}

int CoAPPeer::close() {
  switch (this->transport_type) {
    case CoAP_Transport__plaintext:
      if (socket_id != 255 && !owlModem->socket.close(socket_id)) {
        LOG(L_ERR, "Error closing local socket towards %.*s:%u\r\n", remote_ip.len, remote_ip.s, remote_port);
        return 0;
      }
      socket_id = 255;
      return 1;
      break;
    case CoAP_Transport__DTLS_PSK:
      if (!owlDTLSClient) {
        LOG(L_ERR, "remote_ip=%.*s:%u - owlDTLSClient is null\r\n", remote_ip.len, remote_ip.s, remote_port);
        return 0;
      } else {
        if (!owlDTLSClient->close()) {
          LOG(L_ERR, "Error closing DTLS connection towards %.*s:%u\r\n", remote_ip.len, remote_ip.s, remote_port);
          return 0;
        }
      }
      return 1;
      break;
    default:
      LOG(L_ERR, "Not implemented for transport_type %d\r\n", this->transport_type);
      return 0;
  }
}

int CoAPPeer::transportIsReady() {
  switch (this->transport_type) {
    case CoAP_Transport__plaintext:
      return (this->socket_id != 255);
      break;
    case CoAP_Transport__DTLS_PSK:
      if (!owlDTLSClient) {
        //        LOG(L_ERR, "remote_ip=%.*s:%u - owlDTLSClient is not initialized\r\n", remote_ip.len, remote_ip.s,
        //        remote_port);
        return 0;
      } else {
        return owlDTLSClient->getCurrentStatus() == DTLS_Alert_Description__tinydtls_event_connected;
      }
      break;
    default:
      LOG(L_ERR, "Not implemented for transport_type %d\r\n", this->transport_type);
      return 0;
  }
}

void CoAPPeer::setHandlers(CoAPPeer_MessageHandler_f handler_message, CoAPPeer_DTLSEventHandler_f handler_dtls_event,
                           CoAPPeer_RequestHandler_f handler_request, CoAPPeer_ResponseHandler_f handler_response) {
  this->handler_message    = handler_message;
  this->handler_dtls_event = handler_dtls_event;
  this->handler_request    = handler_request;
  this->handler_response   = handler_response;
}

int CoAPPeer::sendUnreliably(CoAPMessage *message, int probing_rate, int max_transmit_span) {
  uint8_t buf[MODEM_UDP_BUFFER_SIZE];
  bin_t b = {.s = buf, .idx = 0, .max = MODEM_UDP_BUFFER_SIZE};
  int is_ackrst = 0;
  if (!message) {
    LOG(L_ERR, "Null parameter\r\n");
    return 0;
  }
  if (!transportIsReady()) {
    LOG(L_ERR, "Transport is not ready\r\n");
    return 0;
  }
  switch (message->type) {
    case CoAP_Type__Confirmable:
      LOG(L_WARN, "Changing message type from CON to NON\r\n");
      message->type = CoAP_Type__Non_Confirmable;
      break;
    case CoAP_Type__Non_Confirmable:
      break;
    case CoAP_Type__Acknowledgement:
    case CoAP_Type__Reset:
      if (probing_rate != 0) {
        LOG(L_WARN, "Will not retransmit ACK or RST\r\n");
        probing_rate      = 0;
        max_transmit_span = 0;
      }
      is_ackrst = 1;
      break;
    default:
      break;
  }
  if (!message->encode(&b)) {
    LOG(L_ERR, "Error encoding message\r\n");
    return 0;
  }
  str data = bin_to_str(b);

  // Save reply on Server Transactions
  if (is_ackrst) setServerTransactionReply(message->message_id, data);

  if (probing_rate != 0) {
    if (!putClientTransactionNON(message->message_id, data, probing_rate, max_transmit_span)) {
      LOG(L_ERR, "remote=%.*s:%u message_id=%d - error creating client transaction\r\n", remote_ip.len, remote_ip.s,
          remote_port, message->message_id);
      goto error;
    }
  }
  if (!handleTx(data)) {
    LOG(L_ERR, "Error sending data of %d bytes\r\n", data.len);
    goto error;
  }
  LOG(L_INFO, "remote=%.*s:%u - sent %d bytes\r\n", remote_ip.len, remote_ip.s, remote_port, data.len);
  return 1;
error:
  dropClientTransaction(message->message_id);
  return 0;
}

int CoAPPeer::sendReliably(CoAPMessage *message, CoAPPeer_ClientTransactionCallback_f cb, void *cb_param,
                           int max_retransmit, int max_transmit_span) {
  uint8_t buf[MODEM_UDP_BUFFER_SIZE];
  bin_t b = {.s = buf, .idx = 0, .max = MODEM_UDP_BUFFER_SIZE};
  coap_client_transaction_t *t = 0;
  if (!message) {
    LOG(L_ERR, "Null parameter\r\n");
    return 0;
  }
  if (!transportIsReady()) {
    LOG(L_ERR, "Transport is not ready\r\n");
    return 0;
  }
  switch (message->type) {
    case CoAP_Type__Confirmable:
      break;
    case CoAP_Type__Non_Confirmable:
      LOG(L_WARN, "Changing message type from NON to CON\r\n");
      message->type = CoAP_Type__Confirmable;
      break;
    case CoAP_Type__Acknowledgement:
    case CoAP_Type__Reset:
      LOG(L_WARN, "ACK and RST can not be sent reliably\r\n");
      return 0;
      break;
    default:
      break;
  }
  if (!message->encode(&b)) {
    LOG(L_ERR, "Error encoding message\r\n");
    return 0;
  }
  str data = bin_to_str(b);

  if (!putClientTransactionCON(message->message_id, data, cb, cb_param, max_retransmit, max_transmit_span)) {
    LOG(L_ERR, "remote=%.*s:%u message_id=%d - error creating client transaction\r\n", remote_ip.len, remote_ip.s,
        remote_port, message->message_id);
    goto error;
  }

  if (!this->handleTx(data)) {
    LOG(L_ERR, "Error sending data of %d bytes\r\n", data.len);
    return 0;
  }
  LOG(L_INFO, "remote=%.*s:%u - sent %d bytes\r\n", remote_ip.len, remote_ip.s, remote_port, data.len);
  return 1;
error:
  dropClientTransaction(message->message_id);
  return 0;
}

int CoAPPeer::stopRetransmissions(coap_message_id_t message_id) {
  LOG(L_ERR, "Not yet implemented fully\r\n");

  coap_client_transaction_t *t = getClientTransaction(message_id);
  if (!t) {
    LOG(L_ERR, "message_id=%u - not found client transaction\r\n", message_id);
    return 0;
  }

  /* Event: Canceled */
  if (t->cb) (t->cb)(this, t->message_id, t->cb_param, CoAP_Client_Transaction_Event__Canceled, 0);

  return dropClientTransaction(&t);
}

int CoAPPeer::addInstance() {
  for (int i = 0; i < instances_cnt; i++)
    if (!instances[i]) {
      instances[i] = this;
      return 1;
    }
  CoAPPeer **new_instances = (CoAPPeer **)owl_realloc(instances, sizeof(CoAPPeer *) * (instances_cnt + 1));
  if (!new_instances) return 0;
  instances                  = new_instances;
  instances[instances_cnt++] = this;
  return 1;
}

int CoAPPeer::removeInstance() {
  int cnt = 0, others = 0;
  for (int i = 0; i < instances_cnt; i++)
    if (instances[i] == this) {
      instances[i] = 0;
      cnt++;
    } else {
      others++;
    }
  if (!others) {
    owl_free(instances);
    instances     = 0;
    instances_cnt = 0;
  }
  return cnt;
}

int CoAPPeer::triggerPeriodicRetransmit() {
  int cnt = 0;
  for (int i = 0; i < instances_cnt; i++)
    if (instances[i]) cnt += instances[i]->triggerClientTransactionRetransmissions();
  return 1;
}


int CoAPPeer::handleTx(str data) {
  switch (this->transport_type) {
    case CoAP_Transport__plaintext:
      return owlModem->socket.sendUDP(this->socket_id, data, 0);
      break;
    case CoAP_Transport__DTLS_PSK:
      if (!owlDTLSClient) {
        LOG(L_ERR, "remote_ip=%.*s:%u - owlDTLSClient is null\r\n", remote_ip.len, remote_ip.s, remote_port);
        return 0;
      } else {
        return owlDTLSClient->sendData(data);
      }
      return 1;
      break;
    default:
      LOG(L_ERR, "Not implemented for transport_type %d\r\n", this->transport_type);
      return 0;
  }
}



int CoAPPeer::handleRx(str data) {
  coap_handler_follow_up_e follow_up = CoAP__Handler_Followup__Do_Nothing;
  coap_client_transaction_t *tc      = 0;
  coap_server_transaction_t *ts      = 0;
  CoAPMessage message                = CoAPMessage();
  bin_t b                            = str_to_bin(data);
  if (!message.decode(&b)) {
    LOG(L_ERR, "Error decoding message\r\n");
    LOGBIN(L_ERR, b);
  }

  /* Step 0 - internal short-cuts */
  switch (message.type) {
    case CoAP_Type__Confirmable:
      if (message.code_class == CoAP_Code_Class__Empty_Message &&
          message.code_detail == CoAP_Code_Detail__Empty_Message) {
        // this is a ping - reply with reset
        CoAPMessage reset = CoAPMessage(&message, CoAP_Type__Reset);
        this->sendUnreliably(&reset);
        goto done;
      }
      break;
    case CoAP_Type__Non_Confirmable:
      if (message.code_class == CoAP_Code_Class__Empty_Message &&
          message.code_detail == CoAP_Code_Detail__Empty_Message) {
        // ignore
        LOG(L_INFO, "Ignoring NON Empty message\r\n");
        goto error;
      }
      break;
    case CoAP_Type__Acknowledgement:
      if (message.code_class == CoAP_Code_Class__Request && message.code_detail != CoAP_Code_Detail__Empty_Message) {
        // ignore
        LOG(L_INFO, "Ignoring ACK with Request piggybacked message\r\n");
        goto error;
      }
      break;
    case CoAP_Type__Reset:
      if (message.code_class != CoAP_Code_Class__Empty_Message ||
          message.code_detail != CoAP_Code_Detail__Empty_Message) {
        // ignore
        LOG(L_INFO, "Ignoring RST which is not empty\r\n");
        goto error;
      }
      break;
    default:
      LOG(L_ERR, "Invalid type %d\r\n", message.type);
      goto error;
      break;
  }

  /* Step 1 - Call Stateless handler - everything goes there */
  if (this->handler_message) (this->handler_message)(this, &message);

  /* Step 2 - Call Transactions processing - callbacks, discard&replay duplicates, etc */
  switch (message.type) {
    case CoAP_Type__Confirmable:
    case CoAP_Type__Non_Confirmable:
      ts = getServerTransaction(message.message_id);
      if (ts) {
        if (ts->ack_rst.len) {
          if (handleTx(ts->ack_rst))
            LOG(L_DBG, "remote=%.*s:%u message_id=%u old ACK/RST of bytes=%d resent\r\n", remote_ip.len, remote_ip.s,
                remote_port, ts->message_id, ts->ack_rst.len);
          else
            LOG(L_ERR, "remote=%.*s:%u message_id=%u old ACK/RST of bytes=%d failure to re-send\r\n", remote_ip.len,
                remote_ip.s, remote_port, ts->message_id, ts->ack_rst.len);
        } else {
          LOG(L_INFO, "remote=%.*s:%u message_id=%u - silently ignoring retransmission\r\n", remote_ip.len, remote_ip.s,
              remote_port, ts->message_id);
        }
        // Skip further handlers
        goto done;
      } else {
        if (!putServerTransaction(message.message_id, message.type)) {
          LOG(L_ERR, "remote=%.*s:%u message_id=%u - error saving server transactions\r\n", remote_ip.len, remote_ip.s,
              remote_port, ts->message_id);
        }
      }
      break;
    case CoAP_Type__Acknowledgement:
      tc = getClientTransaction(message.message_id);
      if (tc) {
        LOG(L_DBG, "message_id=%u - received ACK\r\n", message.message_id);
        /* Event: ACK */
        if (tc->cb) (tc->cb)(this, tc->message_id, tc->cb_param, CoAP_Client_Transaction_Event__ACK, &message);
        dropClientTransaction(&tc);
      } else {
        LOG(L_WARN, "message_id=%u - received unexpected ACK\r\n", message.message_id);
      }
      /* Empty ACKs are not passed to other handlers */
      if (message.code_class == CoAP_Code_Class__Empty_Message &&
          message.code_detail == CoAP_Code_Detail__Empty_Message)
        goto done;
      break;
    case CoAP_Type__Reset:
      tc = getClientTransaction(message.message_id);
      if (tc) {
        LOG(L_DBG, "message_id=%u - received RST\r\n", message.message_id);
        /* Event: RST */
        if (tc->cb) (tc->cb)(this, tc->message_id, tc->cb_param, CoAP_Client_Transaction_Event__RST, &message);
        dropClientTransaction(&tc);
      } else {
        LOG(L_WARN, "message_id=%u - received unexpected RST\r\n", message.message_id);
      }
      /* RSTs are not passed to other handlers */
      goto done;
      break;
    default:
      break;
  }


  /* Step 3 - Call Request/Response handlers */
  switch (message.type) {
    case CoAP_Type__Confirmable:
      switch (message.code_class) {
        case CoAP_Code_Class__Request:
          if (this->handler_request) follow_up = (this->handler_request)(this, &message);
          break;
        case CoAP_Code_Class__Response:
        case CoAP_Code_Class__Error:
        case CoAP_Code_Class__Server_Error:
          if (this->handler_response) follow_up = (this->handler_response)(this, &message);
          break;
        default:
          break;
      }
      break;
    case CoAP_Type__Non_Confirmable:
      switch (message.code_class) {
        case CoAP_Code_Class__Request:
          if (this->handler_request) (this->handler_request)(this, &message);
          break;
        case CoAP_Code_Class__Response:
        case CoAP_Code_Class__Error:
        case CoAP_Code_Class__Server_Error:
          if (this->handler_response) (this->handler_response)(this, &message);
          break;
        default:
          break;
      }
      break;
    case CoAP_Type__Acknowledgement:
      switch (message.code_class) {
        case CoAP_Code_Class__Response:
        case CoAP_Code_Class__Error:
        case CoAP_Code_Class__Server_Error:
          if (this->handler_response) (this->handler_response)(this, &message);
          break;
        default:
          break;
      }
      break;
    default:
      LOG(L_ERR, "Invalid type %d\r\n", message.type);
      goto error;
  }

  switch (follow_up) {
    case CoAP__Handler_Followup__Do_Nothing:
      break;
    case CoAP__Handler_Followup__Send_Acknowledgement: {
      CoAPMessage ack = CoAPMessage(&message, CoAP_Type__Acknowledgement);
      this->sendUnreliably(&ack);
      break;
    }
    case CoAP__Handler_Followup__Send_Reset: {
      CoAPMessage rst = CoAPMessage(&message, CoAP_Type__Reset);
      this->sendUnreliably(&rst);
      break;
    }
    default:
      LOG(L_ERR, "Not implemented for follow_up %d\r\n", follow_up);
      goto error;
  }

done:
  message.destroy();
  return 1;
error:
  message.destroy();
  return 0;
}



/*
 * Client transactions
 */

int CoAPPeer::dropExpiredClientTransactions() {
  coap_client_transaction_t *t = 0, *nt = 0;
  owl_time_t now = owl_time();
  int cnt        = 0;

  /* Drop all the expired and retransmitted too many times transactions */
  WL_FOREACH_SAFE (&client_transactions, t, nt) {
    if (t->expires > now || t->retransmissions_left > 0) continue;

    WL_DELETE(&client_transactions, t);

    /* Event: Timeout */
    if (t->cb) (t->cb)(this, t->message_id, t->cb_param, CoAP_Client_Transaction_Event__Timeout, 0);

    WL_FREE(t, coap_client_transaction_list_t);
    client_transactions.space_left++;
  }

  return cnt;
}

int CoAPPeer::triggerClientTransactionRetransmissions() {
  switch (transport_type) {
    case CoAP_Transport__plaintext:
      // nothing here
      break;
    case CoAP_Transport__DTLS_PSK:
      if (!owlDTLSClient)
        LOG(L_ERR, "remote_ip=%.*s:%u - owlDTLSClient is null\r\n", remote_ip.len, remote_ip.s, remote_port);
      else
        owlDTLSClient->triggerPeriodicRetransmit();
      break;
    default:
      LOG(L_ERR, "Not handled transport_type %d\r\n", transport_type);
  }

  dropExpiredClientTransactions();

  coap_client_transaction_t *t = 0, *nt = 0;
  owl_time_t now = owl_time();
  int cnt        = 0;

  WL_FOREACH_SAFE (&client_transactions, t, nt) {
    if (t->expires > now || t->retransmissions_left <= 0) continue;

    if (handleTx(t->message)) {
      LOG(L_INFO, "message_id=%u re-transmitted bytes=%d\r\n", t->message_id, t->message.len);
      cnt++;
    } else {
      LOG(L_ERR, "message_id=%u failed to re-transmit bytes=%d\r\n", t->message_id, t->message.len);
    }

    if (t->type == CoAP_Type__Confirmable) t->retransmission_interval *= 2;
    t->expires = now + t->retransmission_interval;
    t->retransmissions_left--;
  }

  return cnt;
}

int CoAPPeer::putClientTransactionCON(coap_message_id_t message_id, str message,
                                      CoAPPeer_ClientTransactionCallback_f cb, void *cb_param, int max_retransmit,
                                      int max_transmit_span) {
  coap_client_transaction_t *t = 0;

  dropExpiredClientTransactions();

  WL_FOREACH (&client_transactions, t)
    if (t->message_id == message_id) {
      LOG(L_ERR, "message_id=%u - Transaction already saved\r\n", message_id);
      return 0;
    }

  if (client_transactions.space_left == 0) {
    LOG(L_ERR, "message_id=%u - No space left for client transaction (too many in parallel)\r\n", message_id);
    return 0;
  }

  WL_NEW(t, coap_client_transaction_list_t);
  t->message_id              = message_id;
  t->type                    = CoAP_Type__Confirmable;
  t->retransmission_interval = ACK_TIMEOUT * 1000 + random((float)ACK_TIMEOUT * 1000.0 * ACK_RANDOM_FACTOR);
  t->expires                 = owl_time() + t->retransmission_interval;
  t->retransmissions_left    = MAX_RETRANSMIT;
  str_dup(t->message, message);
  t->cb       = cb;
  t->cb_param = cb_param;

  WL_APPEND(&client_transactions, t);
  client_transactions.space_left--;
  //  logClientTransactions(L_NOTICE);

  return 1;
out_of_memory:
  WL_FREE(t, coap_client_transaction_list_t);
  return 0;
}

int CoAPPeer::putClientTransactionNON(coap_message_id_t message_id, str message, int probing_rate,
                                      int max_transmit_span) {
  coap_client_transaction_t *t = 0;

  dropExpiredClientTransactions();

  WL_FOREACH (&client_transactions, t)
    if (t->message_id == message_id) {
      LOG(L_ERR, "message_id=%u - Transaction already saved\r\n", message_id);
      return 0;
    }

  if (client_transactions.space_left == 0) {
    LOG(L_ERR, "message_id=%u - No space left for client transaction (too many in parallel)\r\n", message_id);
    return 0;
  }

  WL_NEW(t, coap_client_transaction_list_t);
  t->message_id              = message_id;
  t->type                    = CoAP_Type__Non_Confirmable;
  t->retransmission_interval = message.len * 1000 / probing_rate;
  t->expires                 = owl_time() + t->retransmission_interval;
  t->retransmissions_left    = max_transmit_span * 1000 / t->retransmission_interval;
  str_dup(t->message, message);

  WL_APPEND(&client_transactions, t);
  client_transactions.space_left--;

  return 1;
out_of_memory:
  WL_FREE(t, coap_client_transaction_list_t);
  return 0;
}

coap_client_transaction_t *CoAPPeer::getClientTransaction(coap_message_id_t message_id) {
  coap_client_transaction_t *t = 0;

  dropExpiredClientTransactions();

  WL_FOREACH (&client_transactions, t)
    if (t->message_id == message_id) return t;

  return 0;
}

int CoAPPeer::dropClientTransaction(coap_client_transaction_t **t) {
  if (!t || !*t) return 0;
  WL_DELETE(&client_transactions, *t);
  LOG(L_DBG, "remote_ip=%.*s:%u message_id=%d - client transaction dropped\r\n", remote_ip.len, remote_ip.s,
      remote_port, (*t)->message_id);
  WL_FREE(*t, coap_client_transaction_list_t);
  client_transactions.space_left++;
  *t = 0;
  return 1;
}

int CoAPPeer::dropClientTransaction(coap_message_id_t message_id) {
  coap_client_transaction_t *t, *nt;
  int cnt = 0;
  WL_FOREACH_SAFE (&client_transactions, t, nt) {
    if (t->message_id != message_id) continue;
    WL_DELETE(&client_transactions, t);
    LOG(L_DBG, "remote_ip=%.*s:%u message_id=%d - client transaction dropped\r\n", remote_ip.len, remote_ip.s,
        remote_port, t->message_id);
    WL_FREE(t, coap_client_transaction_list_t);
    client_transactions.space_left++;
    cnt++;
  }
  return cnt;
}

void CoAPPeer::logClientTransactions(log_level_t level) {
  dropExpiredClientTransactions();

  if (!owl_log_is_printable(level)) return;

  coap_client_transaction_t *t;
  owl_time_t now = owl_time();
  float seconds;
  LOGF(level, "--- CoAP Client Transactions ---\r\n");
  WL_FOREACH (&client_transactions, t) {
    if (t->expires > now)
      seconds = (float)(t->expires - now) / 1000.0;
    else if (t->expires < now)
      seconds = -(float)(now - t->expires) / 1000.0;
    else
      seconds = 0;
    LOGF(level, "message_id=%05d type=%s expires=%5.3f sec retr_interval=%03d sec retr_left=%d message=%02d bytes\r\n",
         t->message_id, coap_type_text(t->type), seconds, t->retransmission_interval / 1000, t->retransmissions_left,
         t->message.len);
  }
  LOGF(level, "--------------------------------\r\n");
}



/*
 * Server Transactions
 */

int CoAPPeer::dropExpiredServerTransactions() {
  owl_time_t now               = owl_time();
  coap_server_transaction_t *t = 0;
  int cnt                      = 0;
  while (server_transactions.head && server_transactions.head->expires <= now) {
    t = server_transactions.head;
    WL_DELETE(&this->server_transactions, t);
    WL_FREE(t, coap_server_transaction_list_t);
    server_transactions.space_left++;
    cnt++;
  }
  return cnt;
}

int CoAPPeer::putServerTransaction(coap_message_id_t message_id, coap_type_e type) {
  coap_server_transaction_t *t = 0;
  owl_time_t expires;
  switch (type) {
    case CoAP_Type__Confirmable:
      expires = owl_time() + EXCHANGE_LIFETIME * 1000;
      break;
    case CoAP_Type__Non_Confirmable:
      expires = owl_time() + NON_LIFETIME * 1000;
      break;
    default:
      LOG(L_ERR, "Not handled for type %d\r\n", type);
      return 0;
  }

  dropExpiredServerTransactions();

  WL_FOREACH (&server_transactions, t)
    if (t->message_id == message_id) {
      LOG(L_ERR, "Transaction for message_id=%u already saved\r\n", message_id);
      return 0;
    }

  if (server_transactions.space_left == 0) {
    // drop the oldest
    if (server_transactions.head) {
      t = server_transactions.head;
      WL_DELETE(&server_transactions, t);
      WL_FREE(t, coap_server_transaction_list_t);
      server_transactions.space_left++;
    } else {
      LOG(L_ERR, "Server transactions list empty - badly configured or bug\r\n");
      return 0;
    }
  }

  WL_NEW(t, coap_server_transaction_list_t);
  WL_INSERT_SORT(&server_transactions, coap_server_transaction_list_t, t);
  t->message_id = message_id;
  t->expires    = expires;
  t->type       = type;
  server_transactions.space_left--;

  return 1;
out_of_memory:
  WL_FREE(t, coap_server_transaction_list_t);
  return 0;
}

int CoAPPeer::setServerTransactionReply(coap_message_id_t message_id, str ack_rst) {
  coap_server_transaction_t *t = 0;

  dropExpiredServerTransactions();

  WL_FOREACH (&server_transactions, t)
    if (t->message_id == message_id) {
      str_dup_safe(t->ack_rst, ack_rst);
      return 1;
    }
out_of_memory:
  return 0;
}

coap_server_transaction_t *CoAPPeer::getServerTransaction(coap_message_id_t message_id) {
  coap_server_transaction_t *t = 0;

  dropExpiredServerTransactions();

  WL_FOREACH (&server_transactions, t)
    if (t->message_id == message_id) return t;

  return 0;
}

void CoAPPeer::logServerTransactions(log_level_t level) {
  dropExpiredServerTransactions();

  if (!owl_log_is_printable(level)) return;

  coap_server_transaction_t *t;
  owl_time_t now = owl_time();
  float seconds;
  LOGF(level, "--- CoAP Server Transactions ---\r\n");
  WL_FOREACH (&server_transactions, t) {
    if (t->expires > now)
      seconds = (float)(t->expires - now) / 1000.0;
    else if (t->expires < now)
      seconds = -(float)(now - t->expires) / 1000.0;
    else
      seconds = 0;
    LOGF(level, "message_id=%05d type=%s expires=%5.3f sec ack_rst=%02d bytes\r\n", t->message_id,
         coap_type_text(t->type), seconds, t->ack_rst.len);
  }
  LOGF(level, "--------------------------------\r\n");
}

coap_message_id_t CoAPPeer::getNextMessageId() {
  return ++last_message_id;
}

coap_token_t CoAPPeer::getNextToken(coap_token_lenght_t *token_length) {
  coap_token_t token = last_token++;
  if (token_length) {
    if (token == 0)
      *token_length = 0;
    else if (token <= 0xffu)
      *token_length = 1;
    else if (token <= 0xffffu)
      *token_length = 2;
    else if (token <= 0xffffffu)
      *token_length = 3;
    else if (token <= 0xffffffffu)
      *token_length = 4;
    else if (token <= 0xffffffffffu)
      *token_length = 5;
    else if (token <= 0xffffffffffffu)
      *token_length = 6;
    else if (token <= 0xffffffffffffffu)
      *token_length = 7;
    else
      *token_length = 8;
  }
  return token;
}
