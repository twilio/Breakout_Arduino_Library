#ifndef __OWL_MODEM_AT_H__
#define __OWL_MODEM_AT_H__

#include "IOwlSerial.h"
#include "enums.h"

/* AT modem state machine
 * States:
 *   I - idle
 *   W - waiting for response
 *   D - waiting for invitation to input data
 *   S - sending the data
 *
 * Inputs:
 *   s - strings of data
 *     su - URC string (+XXXXX)
 *     sr - result string (OK, ERROR, +CME ERROR)
 *     si - invitation for an input (CONNNECT, >)
 *     so - other strings
 *   r - command requests
 *   t - timeouts
 *
 * Outputs:
 *   URCH - URC handler
 *   Rh   - response handler
 * */

/*
 *                                                 +---------------------------+
 *                                                 |                           |
 *                                               +---+      si      +---+      |
 *                                        +----->| D |------------->| S |      |
 *                                        |      +---+              +---+      |
 *                     sr/RH(code, resp)  |        |                  |        |
 *             +----+    ----------------\|/-------+                  |        |
 * su/URCH(su) |    |   /                 T        |   all data sent  |        |
 *             |  +---+          r        |      +---+ <--------------+        |
 *             +->| I |------------------------->| W |--------+                |
 *                +---+                          +---+        |                |
 *                  ^                              |  \       | so/resp += so  |
 *                  |                              |   \      |                |
 *                  |                              |    ------+                |
 *                  |        t/RH(timeout,"")      |                           |
 *                  +------------------------------+---------------------------+
 *
 */

#define AT_INPUT_BUFFER_SIZE 64
#define AT_LINE_BUFFER_SIZE 256
#define AT_RESPONSE_BUFFER_SIZE 1024

/*
 * Core class the OwlModem group. Every OwlModem* class is using it.
 * Commands can be sent in the idle state with `startATCommand` method.
 * Command result can be retrieved
 *   - either by polling the state with `getModemState` and when the state is
 *       `response_ready` getting it with `getLastCommandResponse`
 *   - or by registering a callback with `registerResponseHandler`.
 * Unsolicited Response Codes (URC) can be subscribed to with `registerURCHandler`
 * Modem state should be advanced regularly by calling `spin`. Energy consuption
 *   can be optimized by only calling `spin` where there is data available on
 *   the input port or when the modem has something to do on timer (i.e. in
 *   `wait_response` and `wait_prompt` state waiting for a timeout or in send_data`
 *   state delaying for a grace period before sending a new chunk of data).
 * This class is not thread-safe, and all calls to its methods should be protected
 * by a mutex when used in multithreaded environment.
 */
class OwlModemAT {
 public:
  /*
   *  Response handler
   *  @param at_result_code_e - result code
   *  @param str - result data
   *  @return whether result was processed and modem can return to idle state
   */
  using ResponseHandler = bool (*)(at_result_code_e, str, void*);
  using UrcHandler    = bool (*)(str, str, void *);  // code, data, private (pointer to the instance normally)
  using PrefixHandler = void (*)(str, void *);      // input string, private (pointer to the instance normally)
  static constexpr int MaxUrcHandlers = 8;
  static constexpr int MaxPrefixes    = 8;

  enum class modem_state_t {
    idle,
    send_data,
    wait_result,
    wait_prompt,
    response_ready,
  };

  OwlModemAT(IOwlSerial *serial) : serial_(serial) {}
  bool initTerminal();

  /**
   * Send arbitrary data to the modem.
   * @param data - data to send
   * @return success status
   */
  bool sendData(str data);
  bool sendData(char *data) { return sendData({.s = data, .len = strlen(data)});}

  /**
   * Call this function periodically, to handle incoming message from the modem.
   */
  void spin();
  
  /* Move modem to WaitResult (if data.s is nullptr) or WaitPrompt (otherwise) state and send the command.
   * @param command - AT command to send (without "\r\n" postfix)
   * @param timeout_ms - timeout on the command or 0 to wait indefinitely
   * @param data - optional data to send after prompt. Only pointer and length are copied over, so it shouldn't
   *   be deallocated until command is the data is sent (modem state is changed to "idle", "wait_result" or "response_ready")
   * @param data_term - optional terminating symbol appended to the sent data
   * @return false if command could not be sent. In that case the state of the modem remains unchanged
   */
  bool startATCommand(str command, owl_time_t timeout_ms, str data = {nullptr, 0}, uint16_t data_term = 0xFFFF);

  /*
   * Get the current state of the modem
   */
  modem_state_t getModemState() { return state_; }

  /**
   * Get the status and response data of the last command. Also acknowledge
   *   the read-out (moves modem from `response_ready` to `idle` state)
   * @param out_response - last response data
   * @return last result code or AT_Result_Code__none if no command was processed.
   */
  at_result_code_e getLastCommandResponse(str *out_response);

  bool registerUrcHandler(const char* unique_id, UrcHandler handler, void *priv);
  void registerPrefixHandler(PrefixHandler handler, void *priv, const str *prefixes, int num_prefixes);
  void deregisterPrefixHandler();
  void registerResponseHandler(ResponseHandler handler, void* priv);

  /**
   * Execute one AT command. Blockingly sleeps until some result is there. Only
   *   use in trivial cases when this default behaviour is good enough for your application
   * @param command - command to send
   * @param timeout_millis - timeout for the command in milliseconds - consult the modem manual for maximum timeouts
   * for each command
   * @param out_response - optional str object to return response slice. The data itself is not copied over.
   * @param command_data - additional data to a command requiring it (e.g. UDWNFILE in U-Blox Sara R4/N4).
   * @return the AT result code, or AT_Result_Code__failure on failure to send the data, or AT_Result_Code__timeout in
   * case of timeout while waiting for one of the standard AT result codes.
   */
  at_result_code_e doCommandBlocking(str command, owl_time_t timeout_millis, str *out_response,
                                     str command_data = {nullptr, 0}, uint16_t data_term = 0xFFFF);
  at_result_code_e doCommandBlocking(char *command, owl_time_t timeout_millis, str *out_response,
                                     str command_data = {nullptr, 0}, uint16_t data_term = 0xFFFF) {
    return doCommandBlocking({.s = command, .len = strlen(command)}, timeout_millis, out_response, command_data, data_term);
  }

  /**
   * Utility function to filter out of the response for a command, lines which do not start with a certain prefix.
   * The prefix is also eliminated, so that you have just your actual data left.
   * @param prefix - prefix to take out
   * @param response - response to filter
   * @param filtered - str object to return the filtered slice to
   */
  static void filterResponse(str prefix, str response, str *filtered);


 private:
  enum class line_state_t {
    idle,
    idle_expect_lf,
    in_line,
  };

  IOwlSerial *serial_{nullptr};

  modem_state_t state_{modem_state_t::idle};
  line_state_t line_state_{line_state_t::idle};

  str command_data_ = {nullptr, 0};
  uint16_t command_data_term_{0xFFFF};
  owl_time_t command_started_{0};
  owl_time_t command_timeout_{0};
  owl_time_t send_data_ts_{0};
  bool ignore_first_line_{false};

  at_result_code_e last_response_code_{AT_Result_Code__unknown};

  char input_buffer_c_[AT_INPUT_BUFFER_SIZE];
  str input_buffer_ = {.s = input_buffer_c_, .len = 0};

  char line_buffer_c_[AT_LINE_BUFFER_SIZE];
  str line_buffer_ = {.s = line_buffer_c_, .len = 0};

  char response_buffer_c_[AT_RESPONSE_BUFFER_SIZE];
  str response_buffer_ = {.s = response_buffer_c_, .len = 0};

  UrcHandler urc_handlers_[MaxUrcHandlers];
  const char* urc_handler_ids_[MaxUrcHandlers];
  void *urc_handler_params_[MaxUrcHandlers];
  int num_urc_handlers_{0};

  ResponseHandler response_handler_{nullptr};
  void *response_handler_param_{nullptr};


  str special_prefixes_[MaxPrefixes];
  PrefixHandler prefix_handler_{nullptr};
  void *prefix_handler_param_{nullptr};
  int num_special_prefixes_{0};

  void spinProcessTime();
  void spinProcessInput();
  void spinProcessLine();
  void appendLineToResponse();
  bool processURC();
  void processPrefix();
  void processInputPrompt();

  at_result_code_e tryParseCode();
};

#endif  // __OWL_MODEM_AT_H__
