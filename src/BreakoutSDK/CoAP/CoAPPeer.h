/*
 * CoAPPeer.h
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

#ifndef __OWL_COAP_PEER_H__
#define __OWL_COAP_PEER_H__

#include "CoAPMessage.h"
#include "../DTLS/OwlDTLSClient.h"
#include "../modem/OwlModemRN4.h"



typedef enum {
  CoAP_Transport__plaintext = 0,
  CoAP_Transport__DTLS_PSK  = 1,
} coap_transport_type_e;


/*
 * Retransmit Parameters
 */

/** Client Side - max number of CON and Requests outstanding at one time. Default: 1 */
#define NSTART 5

/** Client Side - Default: 2 s */
#define ACK_TIMEOUT 5

/** Client Side - Default: 1.5*/
#define ACK_RANDOM_FACTOR 1.5

/** Client Side - Default: 4*/
#define MAX_RETRANSMIT 3

/** Client Side - max interval to re-send a message. Default: 45 s */
#define MAX_TRANSMIT_SPAN (ACK_TIMEOUT * (pow(2, MAX_RETRANSMIT) - 1) * ACK_RANDOM_FACTOR)

/** Client Side - max interval to wait for a message. Default: 45 s */
#define MAX_TRANSMIT_WAIT (ACK_TIMEOUT * (pow(2, MAX_RETRANSMIT + 1) - 1) * ACK_RANDOM_FACTOR)

/** Client Side - time after which to safely reuse a message id */
#define NON_LIFETIME MAX_TRANSMIT_SPAN + MAX_LATENCY


/** Transport Side - Default: 100 */
#define MAX_LATENCY 30

/** Transport Side */
#define MAX_RTT ((2 * MAX_LATENCY) + PROCESSING_DELAY)


/** Server Side - max number of incoming server transactions to keep - above this, de-duplication will not work */
#define NSYNC 128

/** Server Side - processing latency. Default: 2 s */
#define PROCESSING_DELAY 2

/** Server Side time to keep a message-id and the answer in the buffer */
#define EXCHANGE_LIFETIME (MAX_TRANSMIT_SPAN + (2 * MAX_LATENCY) + PROCESSING_DELAY)



class CoAPPeer;

typedef void (*CoAPPeer_MessageHandler_f)(CoAPPeer *peer, CoAPMessage *message);
typedef void (*CoAPPeer_DTLSEventHandler_f)(CoAPPeer *peer, dtls_alert_level_e level, dtls_alert_description_e code);

typedef enum {
  CoAP_Client_Transaction_Event__unknown  = 0,
  CoAP_Client_Transaction_Event__ACK      = 1,
  CoAP_Client_Transaction_Event__RST      = 2,
  CoAP_Client_Transaction_Event__Timeout  = 3,
  CoAP_Client_Transaction_Event__Canceled = 4, /**< stopRetransmissions() was called */

  //  CoAP_Client_Transaction_Event__Response = 4, /**< Or maybe this is not for here? - too long lived maybe */
} coap_client_transaction_event_e;

typedef void (*CoAPPeer_ClientTransactionCallback_f)(CoAPPeer *peer, coap_message_id_t message_id, void *cb_param,
                                                     coap_client_transaction_event_e event, CoAPMessage *message);

typedef enum {
  CoAP__Handler_Followup__Do_Nothing           = 0,
  CoAP__Handler_Followup__Send_Acknowledgement = 1,
  CoAP__Handler_Followup__Send_Reset           = 2,
} coap_handler_follow_up_e;

typedef coap_handler_follow_up_e (*CoAPPeer_RequestHandler_f)(CoAPPeer *peer, CoAPMessage *request);
typedef coap_handler_follow_up_e (*CoAPPeer_ResponseHandler_f)(CoAPPeer *peer, CoAPMessage *response);



typedef struct _coap_client_transaction_list_t_slot {
  coap_message_id_t message_id;
  coap_type_e type;
  owl_time_t expires;
  uint32_t retransmission_interval;
  int retransmissions_left;
  str message;
  CoAPPeer_ClientTransactionCallback_f cb;
  void *cb_param;

  struct _coap_client_transaction_list_t_slot *prev, *next;
} coap_client_transaction_t;

typedef struct {
  int space_left;
  coap_client_transaction_t *head, *tail;
} coap_client_transaction_list_t;

#define coap_client_transaction_list_t_free(x)                                                                         \
  do {                                                                                                                 \
    if (x) {                                                                                                           \
      str_free((x)->message);                                                                                          \
      owl_free(x);                                                                                                     \
      (x) = 0;                                                                                                         \
    }                                                                                                                  \
  } while (0)

#define coap_client_transaction_list_t_compare(a, b) (a->expires <= b->expires)



typedef struct _coap_server_transaction_list_t_slot {
  coap_message_id_t message_id;
  coap_type_e type;
  owl_time_t expires;
  str ack_rst;

  struct _coap_server_transaction_list_t_slot *prev, *next;
} coap_server_transaction_t;

typedef struct {
  int space_left;
  coap_server_transaction_t *head, *tail;
} coap_server_transaction_list_t;

#define coap_server_transaction_list_t_free(x)                                                                         \
  do {                                                                                                                 \
    if (x) {                                                                                                           \
      str_free((x)->ack_rst);                                                                                          \
      owl_free(x);                                                                                                     \
      (x) = 0;                                                                                                         \
    }                                                                                                                  \
  } while (0)

#define coap_server_transaction_list_t_compare(a, b) (a->expires <= b->expires)


class CoAPPeer {
 public:
  /**
   * Constructor for plain-text transport over OwlModem/UDP
   */
  CoAPPeer(OwlModemRN4 *owlModem, uint16_t local_port, str remote_ip, uint16_t remote_port = 5683);

  /**
   * Constructor for DTLS with PSK transport over tinydtls over OwlModem/UDP
   */
  CoAPPeer(OwlModemRN *owlModem, str psk_id, str psk_key, uint16_t local_port, str remote_ip,
           uint16_t remote_port = 5684);

  ~CoAPPeer();


  /**
   * Call this function to start initialization, or restart it in case it failed.
   * @return 1 on success, 0 on failure
   */
  int reinitialize();

  /**
   * Returns true if the transport is ready for sending out messages. E.g. DTLS finished the handshake.
   * @return 1 if ready, 0 if not yet
   */
  int transportIsReady();

  /**
   * Close the current transport nicely
   * @return 1 on success, 0 on failure
   */
  int close();

  /**
   * Set handlers.
   * @param handler_message - handler for ALL incoming CoAP messages, including retransmissions, ACKs, etc
   * @param handler_dtls_event - handler for DTLS events
   * @param handler_request - handler for requests, with retransmissions being hidden
   * @param handler_response - handler for responses, with retransmissions being hidden
   */
  void setHandlers(CoAPPeer_MessageHandler_f handler_message, CoAPPeer_DTLSEventHandler_f handler_dtls_event,
                   CoAPPeer_RequestHandler_f handler_request, CoAPPeer_ResponseHandler_f handler_response);


  /**
   * Send a message unreliably - CON, if set, would be set to NON.
   * The probing_rate and max_transmit_span allow for retransmissions, but use with care, as there is no ACK, hence
   * these will all be sent, until you stop them with stopClientRetransmissions. These do not apply to ACK or Reset
   * @param message
   * @param probing_rate - in bytes/second which are acceptable to be sent (typical 2 bytes/second), 0 to disable
   * @param max_transmit_span - interval in seconds after which to stop retransmitting, 0 to use the default value
   * @return 1 on success, 0 on failure
   */
  int sendUnreliably(CoAPMessage *message, int probing_rate = 0, int max_transmit_span = 0);

  /**
   * Send a message reliably - NON, if set, will be set to CON.
   * @param message - message to send
   * @param cb - callback function to call on events
   * @param cb_param - generic callback parameter
   * @param max_retransmit - number of retransmissions - 0 to use the default value
   * @param max_transmit_span - max interval to retransmit - 0 to use the default value
   * @return 1 on success, 0 on failure
   */
  int sendReliably(CoAPMessage *message, CoAPPeer_ClientTransactionCallback_f cb, void *cb_param,
                   int max_retransmit = 0, int max_transmit_span = 0);

  /**
   * Stop retransmissions of the message with the given message_id - works for both sendUnreliably() (if used with
   * retransmission parameters) and for sendReliably()
   * @param message_id
   * @return 1 on success, 0 on failure
   */
  int stopRetransmissions(coap_message_id_t message_id);

  /**
   * Call this regularly, to trigger retransmissions, if requires
   * @return
   */
  static int triggerPeriodicRetransmit();


  /**
   * Print-out the current client transaction table, for debug purposes.
   * @param level - level to print on
   */
  void logClientTransactions(log_level_t level);

  /**
   * Print-out the current server transaction table, for debug purposes.
   * @param level - level to print on
   */
  void logServerTransactions(log_level_t level);

  /**
   * Create a new Message-Id for this peer
   * @return the new message_id
   */
  coap_message_id_t getNextMessageId();

  /**
   * Create a new Token for a Request/Response exchange
   * @return the new token
   */
  coap_token_t getNextToken(coap_token_lenght_t *token_length);

 private:
  OwlModemRN4 *owlModem                   = 0;
  coap_transport_type_e transport_type = CoAP_Transport__plaintext;

  int initDTLSClient();

  /* CoAP_Transport__plaintext */
  uint8_t socket_id = 255;
  static CoAPPeer *socketMappings[MODEM_MAX_SOCKETS];
  static void handlePlaintextData(uint8_t socket, str remote_ip, uint16_t remote_port, str data);

  /* CoAP_Transport__DTLS_PSK */
  str psk_id                   = {0};
  str psk_key                  = {0};
  OwlDTLSClient *owlDTLSClient = 0;
  static void handlerDTLSData(OwlDTLSClient *owlDTLSClient, session_t *session, str plaintext);
  static void handlerDTLSEvent(OwlDTLSClient *owlDTLSClient, session_t *session, dtls_alert_level_e level,
                               dtls_alert_description_e code);

  /* Internal send function */
  int handleTx(str data);
  /* Internal receive function */
  int handleRx(str data);

  CoAPPeer_MessageHandler_f handler_message      = 0;
  CoAPPeer_DTLSEventHandler_f handler_dtls_event = 0;
  CoAPPeer_RequestHandler_f handler_request      = 0;
  CoAPPeer_ResponseHandler_f handler_response    = 0;

  coap_message_id_t last_message_id = 0;
  coap_token_t last_token;

  /*
   * Instances, to allow for a static triggerRetransmit()
   */
  static CoAPPeer **instances;
  static int instances_cnt;
  static int addInstance(CoAPPeer *x);
  static int removeInstance(CoAPPeer *x);

  /*
   * Client transactions
   */

  coap_client_transaction_list_t client_transactions = {.space_left = NSTART, .head = 0, .tail = 0}; /**< unsorted */

  int dropExpiredClientTransactions();
  int triggerClientTransactionRetransmissions();
  int putClientTransactionCON(coap_message_id_t message_id, str message, CoAPPeer_ClientTransactionCallback_f cb,
                              void *cb_param, int max_retransmit = 0, int max_transmit_span = 0);
  int putClientTransactionNON(coap_message_id_t message_id, str message, int probing_rate, int max_transmit_span);
  coap_client_transaction_t *getClientTransaction(coap_message_id_t message_id);
  int dropClientTransaction(coap_client_transaction_t **t);
  int dropClientTransaction(coap_message_id_t message_id);



  /*
   * Server Transactions
   */

  coap_server_transaction_list_t server_transactions = {.space_left = NSYNC, .head = 0, .tail = 0}; /**< sorted */

  int dropExpiredServerTransactions();
  int putServerTransaction(coap_message_id_t message_id, coap_type_e type);
  int setServerTransactionReply(coap_message_id_t message_id, str ack_rst);
  coap_server_transaction_t *getServerTransaction(coap_message_id_t message_id);



  // TODO investigate if it makes sense to make these private
 public:
  uint16_t local_port  = 0;
  str remote_ip        = {0};
  uint16_t remote_port = 0;
};

#endif
