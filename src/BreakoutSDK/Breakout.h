/*
 * Breakout.h
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

#ifndef _BREAKOUTSDK_BREAKOUT_H_
#define _BREAKOUTSDK_BREAKOUT_H_

#include "../board.h"

#include "modem/OwlModem.h"
#include "CoAP/CoAPPeer.h"
#include "CLI/OwlModemCLI.h"



#define TESTING_WITH_DTLS 1

#define TESTING_WITH_CLI 1



#define MAX_PENDING_COMMANDS 100
#define BREAKOUT_POLLING_INTERVAL_MINIMUM 5
#define BREAKOUT_INIT_CONNECTION_TIMEOUT 60
#define BREAKOUT_INIT_CONNECTION_RETRIES 2
#define BREAKOUT_REINIT_CONNECTION_INTERVAL 600


/**
 * Enumeration for Command status result.
 */
typedef enum {
  COMMAND_STATUS_OK,                 /**< Returned if operation was successful. */
  COMMAND_STATUS_ERROR,              /**< Returned if operation failed. */
  COMMAND_STATUS_BUFFER_TOO_SMALL,   /**< Returned if provided buffer is too small for requested data. */
  COMMAND_STATUS_NO_COMMAND_WAITING, /**< Returned if no Command is available for reading. */
  COMMAND_STATUS_COMMAND_TOO_LONG,   /**< Returned if provided Command is too long. */
} command_status_code_e;

typedef enum {
  COMMAND_RECEIPT_CONFIRMED_DELIVERY, /**< The Command was confirmed as received on the server side. */
  COMMAND_RECEIPT_SERVER_ERROR,       /**< The Command was rejected and/or the server returned an error. */
  COMMAND_RECEIPT_CANCELED,           /**< The delivery receipt was canceled, this is a dummy event for cleanups.
                                       * It might have been delivered. */
  COMMAND_RECEIPT_TIMEOUT,            /**< The Command was not confirmed in time. Note that it might have been
                                       * delivered, but all delivery receipts might've been lost on the way back. */
} command_receipt_code_e;

/**
 * Enumeration for connection status updates.
 */
typedef enum {
  /** The device is not registered on the mobile network - offline */
  CONNECTION_STATUS_OFFLINE = 0x00,
  /** The mobile network denied network registration - check SIM status - offline*/
  CONNECTION_STATUS_NETWORK_REGISTRATION_DENIED = 0x01,
  /** The device is registered to the mobile network but (not yet?) connected to the Twilio Commands SDK - offline */
  CONNECTION_STATUS_REGISTERED_NOT_CONNECTED = 0x02,
  /** The device is registered to the mobile network and connected to the Twilio Commands SDK - online */
  CONNECTION_STATUS_REGISTERED_AND_CONNECTED = 0x03,
} connection_status_e;

/**
 * Handler function signature for connection status updates
 * @param connection_status - the new connection status
 */
typedef void (*BreakoutConnectionStatusHandler_f)(const connection_status_e connection_status);

/**
 * Handler function signature for To-SIM Commands (receiving Commands)
 * @param buf - a buffer containing the Command - duplicate data if you need to use it later
 * @param bufSize - the length of Command in the buffer
 * @param isBinary - indicates if the incoming Command is binary or ascii.
 */
typedef void (*BreakoutCommandHandler_f)(const char *buf, size_t bufSize, bool isBinary);

/**
 * Handler function signature for From-SIM Commands with Receipt Request
 * @param receipt_code - the Receipt Request
 *    COMMAND_RECEIPT_CONFIRMED_DELIVERY - The Command was confirmed as received on the server side.
 *    COMMAND_RECEIPT_SERVER_ERROR       - The Command was rejected and/or the server returned an error.
 *    COMMAND_RECEIPT_CANCELED           - The delivery receipt was canceled, this is a dummy event for cleanups.
 *                                         It might have been delivered.
 *    COMMAND_RECEIPT_TIMEOUT            - The Command was not confirmed in time. Note that it might have been
                                           delivered, but all delivery receipts might've been lost on the way back.
 * @param cb_parameter - a generic pointer to application data, as passed to the sendCommandWithReceiptRequest()
 */
typedef void (*BreakoutCommandReceiptCallback_f)(command_receipt_code_e receipt_code, void *cb_parameter);



typedef struct _breakout_command_list_t_slot {
  str command;
  bool isBinary;

  struct _breakout_command_list_t_slot *prev, *next;
} breakout_command_t;

typedef struct {
  int space_left;
  breakout_command_t *head, *tail;
} breakout_command_list_t;

#define breakout_command_list_t_free(x)                                                                                \
  do {                                                                                                                 \
    if (x) {                                                                                                           \
      str_free((x)->command);                                                                                          \
      owl_free(x);                                                                                                     \
      (x) = 0;                                                                                                         \
    }                                                                                                                  \
  } while (0)



class Breakout {
 public:
  /**
   * Obtain an instance to the Breakout client singleton.
   * @return - breakout client
   */
  static Breakout &getInstance() {
    static Breakout instance;
    return instance;
  }



  /*                      Setting Parameters & Event Handlers                         */

  /**
   * Sets the purpose string.  This defaults to "Dev-Kit" and is informational to the network for
   * the purpose of your device.  Maximum length for purpose string is 32 characters.
   * Must be set before powering on the module or this call will return an error.
   * @param purpose - your developer kit's purpose.
   * @return true is setting purpose was successful, false otherwise
   */
  bool setPurpose(char const *purpose);

  /**
   * Sets the interval (seconds) at which to poll for new Commands from the server.  Minimum
   * supported interval is 60 (every 1 minute).  The default is 0, which indicates no automatic
   * querying of the server.  Value is specified in seconds, values < 60 will result in no polling.
   * If last polling was far away in the past, a poll will follow. Otherwise (e.g. change of interval), the timer
   * for the next poll is simply set to last_polling_time + interval_seconds.
   * @param interval_seconds - Interval to query server in seconds.  Minimum value is 60 or 0, which
   * indicates no polling.
   */
  void setPollingInterval(uint32_t interval_seconds);

  /**
   * Set the PSK-Key
   * Must be set before powering on the module or this call will return an error.
   * @param hex_key - 16 bytes in hex, hence 32 characters
   * @return true is setting purpose was successful, false otherwise
   */
  bool setPSKKey(char const *hex_key);

  /**
   * Sets the connection status handler.
   * @param handler - the handler, of type `void handler(const connection_status_e connection_status)`
   * connection_status will indicate network connection status.
   */
  void setConnectionStatusHandler(BreakoutConnectionStatusHandler_f handler);

  /**
   * Sets the handler for To-SIM Breakout Commands. The handler function will be called automatically when a Command is
   * received.
   * @param handler - the handler of type `void handler(const char * buf, size_t bufSize)`
   */
  void setCommandHandler(BreakoutCommandHandler_f handler);



  /*                      Powering up the module & Status                                */

  /**
   * Checks powered on state of communication module.
   * @return true if powered on, false otherwise.
   */
  bool isPowered();

  /**
   * Powers the communication module on.
   * @return true if powered on, false otherwise.
   */
  bool powerModuleOn();

  /**
   * Powers the communication module off.
   * @return true if powered off, false otherwise.
   */
  bool powerModuleOff();

  /**
   * Checks if the communications network is successfully connected and registered.
   * @return the connection status
   */
  connection_status_e getConnectionStatus();

  /**
   * Manually reinitialize the connection with Twilio.
   */
  bool reinitializeTransport();


  /*                      Main Functionality Loop                                */

  /**
   * Handle incoming SDK events. This function must be called periodically to ensure correct functionality.
   */
  void spin();



  /**
   * Send a text Command to Twilio - without Receipt Request
   * @param buf - the text Command to send to Twilio - max 140 characters.
   * @return
   *    COMMAND_STATUS_SUCCESS on success
   *    COMMAND_STATUS_ERROR on error
   *    COMMAND_STATUS_COMMAND_TOO_LONG if strlen(buf) > 140
   */
  command_status_code_e sendTextCommand(const char *buf);

  /**
   * Send a binary Command to Twilio - without Receipt Request
   * @param buf - buffer containing the binary Command to send to Twilio - max 140 characters.
   * @param bufSize - number of bytes of the binary Command
   * @return
   *    COMMAND_STATUS_SUCCESS on success
   *    COMMAND_STATUS_ERROR on error
   *    COMMAND_STATUS_COMMAND_TOO_LONG if bufSize > 140
   */
  command_status_code_e sendBinaryCommand(const char *buf, size_t bufSize);

  /**
   * Send a text Command to Twilio - with Receipt Request
   * @param buf - the text Command to send to Twilio - max 140 characters.
   * @param callback - Command receipt callback.
   * @param callback_parameter - a generic pointer to application data.
   * @return
   *    COMMAND_STATUS_SUCCESS on success
   *    COMMAND_STATUS_ERROR on error
   *    COMMAND_STATUS_COMMAND_TOO_LONG if strlen(buf) > 140
   */
  command_status_code_e sendTextCommandWithReceiptRequest(const char *buf, BreakoutCommandReceiptCallback_f callback,
                                                          void *callback_parameter);

  /**
   * Send a binary Command to Twilio - with Receipt Request
   * @param buf - buffer containing the binary Command to send to Twilio - max 140 characters.
   * @param bufSize - number of bytes of the binary Command
   * @param callback - Command receipt callback.
   * @param callback_parameter - a generic pointer to application data.
   * @returns
   *    COMMAND_STATUS_SUCCESS on success
   *    COMMAND_STATUS_ERROR on error
   *    COMMAND_STATUS_COMMAND_TOO_LONG if bufSize > 140
   */
  command_status_code_e sendBinaryCommandWithReceiptRequest(const char *buf, size_t bufSize,
                                                            BreakoutCommandReceiptCallback_f callback,
                                                            void *callback_parameter);


  /**
   * Manually initiate a check for waiting Commands.
   * If setPollingInterval() is set to a valid interval, this is automatically called at an interval.
   * If both polling interval is enabled, the polling timer is reset on manual calls to this method.
   * @return - true if the operation was successful, query `hasWaitingCommands()` for result.
   */
  bool checkForCommands(bool isRetry = false);

  /**
   * Indicates the presence of at least one waiting Command. This is an alternative to setting a handler for Breakout
   * Commands, in case polling is the preferred option. Use with receiveCommand() to retrieve the Breakout Commands one
   * by one, in case this returns true.
   * @return - true if there is a Command waiting, false otherwise
   */
  bool hasWaitingCommand();

  /**
   * Pop a received Breakout Command, in case it was received locally (hasWaitingCommand() returns true). Alternatively,
   * in case a triggered behavior is preferred, set a handler with setCommandHandler(handler) and handler will be
   * called with Commands when they arrive, without having to poll.
   * @param maxBufSize - Size of buffer being passed in
   * @param buf - Buffer to receive Command into
   * @param bufSize - Output size of returned Command in buf, will not exceed 141 bytes.
   * @param isBinary - Output indicator if the Command was received with Content-Format indicating text or binary
   * @return
   *    COMMAND_STATUS_OK on success
   *    COMMAND_STATUS_NO_COMMAND_WAITING if no Commands are waiting
   *    COMMAND_STATUS_BUFFER_TOO_SMALL if the receiving buffer would not be large enough
   *    COMMAND_STATUS_ERROR in case the parameters were bad
   */
  command_status_code_e receiveCommand(const size_t maxBufSize, char *buf, size_t *bufSize, bool *isBinary);

  /**
   * Query the GNSS module for position information.
   * @param out_gnss_data - gnss_data_t structure to receive current GNSS data.
   * @return - true if the operation was successful, false otherwise
   */
  bool getGNSSData(gnss_data_t *out_gnss_data);


 private:
  Breakout();
  Breakout(Breakout const &);
  void operator=(Breakout const &);
  ~Breakout();



  /*                     Configuration                 */

  char purpose[33];
  char c_iccid[64];
  str iccid = {.s = c_iccid, .len = 0};
  char c_psk_id[32];
  str psk_id = {.s = c_psk_id, .len = 0};
  char c_psk_key[16];
  str psk_key = {.s = c_psk_key, .len = 0};



  /*                     Sub-objects                   */

  OwlModem *owlModem = 0;
  CoAPPeer *coapPeer = 0;

#ifdef TESTING_WITH_CLI == 1
  OwlModemCLI *owlModemCLI = 0;
  int cli_resume           = 0;
#endif



  /*                     Internal Operations                   */

  bool initModem();
  bool initCoAPPeer();

  at_cereg_stat_e eps_registration_status = AT_CEREG__Stat__Not_Registered;
  bool coap_status                        = false;
  owl_time_t last_coap_status_connected   = 0;
  void notifyConnectionStatusChanged();
  bool receivedCommandInternal(str data, bool isBinary);
  command_status_code_e sendCommand(str cmd, bool isBinary = false);
  command_status_code_e sendCommandWithReceiptRequest(str cmd, BreakoutCommandReceiptCallback_f callback,
                                                      void *callback_parameter, bool isBinary = false);



  /*                     Heartbeats aka Polling                 */

  uint32_t polling_interval = 10 * 60; /**< Polling interval in seconds */
  owl_time_t last_polling   = 0;       /**< Time of last polling - will warn if manually triggering it more often */
  owl_time_t next_polling   = 1;       /**< Time of next automatic polling, or 0 if disabled. Initialize to 0 if to
                                         * disable it, or 1 if to enable it by default, without setting the poll. */
  coap_token_t last_polling_token = 0; /**< Token sent in the last Heartbeats request, to match the Response */
  uint64_t queued_command_count   = 0; /**< Last received Queued-Command-Count, as a Response to Heartbeats */

  BreakoutConnectionStatusHandler_f connection_handler = 0;
  BreakoutCommandHandler_f command_handler             = 0;

  breakout_command_list_t commands = {
      .space_left = MAX_PENDING_COMMANDS, .head = 0, .tail = 0};  //**< ordered by receipt */



  /*                     Handlers - OwlModem                 */

  static void handler_PIN(str message);
  static void handler_NetworkRegistrationStatusChange(at_creg_stat_e stat, uint16_t lac, uint32_t ci,
                                                      at_creg_act_e act);
  static void handler_GPRSRegistrationStatusChange(at_cgreg_stat_e stat, uint16_t lac, uint32_t ci, at_cgreg_act_e act,
                                                   uint8_t rac);
  static void handler_EPSRegistrationStatusChange(at_cereg_stat_e stat, uint16_t lac, uint32_t ci, at_cereg_act_e act,
                                                  at_cereg_cause_type_e cause_type, uint32_t reject_cause);
  static void handler_UDPData(uint8_t socket, str remote_ip, uint16_t remote_port, str data);
  static void handler_SocketClosed(uint8_t socket);



  /*                     Handlers - CoAP                 */

  static void handler_CoAPStatelessMessage(CoAPPeer *peer, CoAPMessage *message);
  static void handler_CoAPDTLSEvent(CoAPPeer *peer, dtls_alert_level_e level, dtls_alert_description_e code);
  static coap_handler_follow_up_e handler_CoAPRequest(CoAPPeer *peer, CoAPMessage *request);
  static coap_handler_follow_up_e handler_CoAPResponse(CoAPPeer *peer, CoAPMessage *response);



  /*                     Callback - CoAP                 */

  static void callback_checkForCommands(CoAPPeer *peer, coap_message_id_t message_id, void *cb_param,
                                        coap_client_transaction_event_e event, CoAPMessage *message);
  static void callback_commandReceipt(CoAPPeer *peer, coap_message_id_t message_id, void *cb_param,
                                      coap_client_transaction_event_e event, CoAPMessage *message);

  friend class BreakoutSendCommand;
  friend class BreakoutSendCommandWithReceiptRequest;
};

#endif
