#ifndef __OWL_MODEM_AT_H__
#define __OWL_MODEM_AT_H__

#include "IOwlSerial.h"
#include "enums.h"

#define MODEM_Rx_BUFFER_SIZE 1200
#define MODEM_RESPONSE_BUFFER_SIZE 1200

class OwlModemAT {
 public:
  using UrcHandler = int (*)(str, str, void *);  // code, data, private (pointer to the instance normally)
  static constexpr int MaxUrcHandlers = 8;

  OwlModemAT(IOwlSerial *serial) : serial_(serial) {
  }
  bool initTerminal();
  void pause() {
    paused_ = true;
  }
  void resume() {
    paused_ = false;
  }

  /**
   * Send arbitrary data to the modem.
   * @param data - data to send
   * @return 1 on success, 0 on failure
   */
  int sendData(str data);
  int sendData(char *data);

  /**
   * Call this function periodically, to handle incoming message from the modem. The UART serial interface has a buffer
   * of just 256 bytes and this function drains it by moving the data to the rx_buffer. What can be parsed (complete
   * data) is parsed and passed along to handlers.
   * @return 1 on success, 0 on failure
   */
  void spin();

  /**
   * Send one AT command
   *
   * @param command - command to send
   * @return AT_Result_Code__OK if the command was sent, negative value otherwise.
   *   If the command was sent successfully, be sure to either wait for a response
   *   from getLastCommandResponse or interrupt the waiting with InterruptLastCommand.
   */
  at_result_code_e sendCommand(str command);
  at_result_code_e getLastCommandResponse(str *out_response, int max_response_len);
  void interrupLastCommand() {
    in_command_ = false;
  }
  /**
   * Execute one AT command
   * @param command - command to send
   * @param timeout_millis - timeout for the command in milliseconds - consult the modem manual for maximum timeouts
   * for each command
   * @param out_response - optional output buffer to fill with the command response (not including the result code)
   * @param max_response_len - length of output buffer
   * @return the AT result code, or AT_Result_Code__failure on failure to send the data, or AT_Result_Code__timeout in
   * case of timeout while waiting for one of the standard AT result codes.
   */
  at_result_code_e doCommandBlocking(str command, uint32_t timeout_millis, str *out_response, int max_response_len);
  at_result_code_e doCommandBlocking(char *command, uint32_t timeout_millis, str *out_response, int max_response_len);

  bool registerUrcHandler(UrcHandler handler, void *priv);
  /**
   * Utility function to filter out of the response for a command, lines which do not start with a certain prefix.
   * The prefix is also eliminated, so that you have just your actual data left.
   * @param prefix - prefix to take out
   * @param out_response - response to modify
   */
  static void filterResponse(str prefix, str *response);

  void interruptLastCommand() {
    in_command_ = false;
  }



 private:
  IOwlSerial *serial_;

  int processURC(str line, int report_unknown);
  int getNextCompleteLine(int start_idx, str *line);
  void removeRxBufferLine(str line);
  void consumeUnsolicited();
  void consumeUnsolicitedInCommandResponse();
  at_result_code_e extractResult(str *out_response, int max_response_len);

  int drainModemRxToBuffer();

  UrcHandler urc_handlers_[MaxUrcHandlers];
  void *urc_handler_params_[MaxUrcHandlers];
  int num_urc_handlers_{0};
  /** The modem has been issued a command and is waiting for its response - URC are not expected */
  bool in_command_{false};

  /** The receiving buffer - the modem interface is drained and bytes moved here */
  char c_rx_buffer[MODEM_Rx_BUFFER_SIZE];
  str rx_buffer = {.s = c_rx_buffer, .len = 0};

  /** Response buffer, to be used by the internal functions */
  char response_buffer[MODEM_RESPONSE_BUFFER_SIZE];
  str response = {.s = response_buffer, .len = 0};

  bool paused_{false};
};
#endif  // __OWL_MODEM_AT_H__
