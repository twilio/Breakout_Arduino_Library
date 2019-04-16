/*
 * OwlModemSIM.h
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
 * \file OwlModemSIM.h - API for retrieving various data from the SIM card
 */

#ifndef __OWL_MODEM_SIM_H__
#define __OWL_MODEM_SIM_H__

#include "enums.h"
#include "OwlModemAT.h"



#define MODEM_SIM_RESPONSE_BUFFER_SIZE 256

/**
 * Handler function signature for SIM card not ready - use this to verify the PIN with the card.
 * @param message - the last message regarding PIN from the card
 */
typedef void (*OwlModem_PINHandler_f)(str message);



/**
 * Twilio wrapper for the AT serial interface to a modem - Methods to get information from the SIM card
 */
class OwlModemSIM {
 public:
  OwlModemSIM(OwlModemAT *atModem);

  /**
   * Handler for Unsolicited Response Codes from the modem - called from OwlModem on timer, when URC is received
   * @param urc - event id
   * @param data - data of the event
   * @return 1 if the line was handled, 0 if no match here
   */
  static int processURC(str urc, str data, void *instance);



  /**
   * Retrieve ICCID (SIM serial number)
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getICCID(str *out_response, int max_response_len);

  /**
   * Retrieve IMSI (Mobile subscriber identifier)
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getIMSI(str *out_response, int max_response_len);

  /**
   * Retrieve MSISDN (SIM card stored own telephone number indication)
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getMSISDN(str *out_response, int max_response_len);

  /**
   * Trigger retrieval of the current PIN status - setting the callback will trigger verification.
   *
   * Note: You must do setHandlerPIN() before calling this. That's where you get the actual status, not here!
   *
   * @return 1 on success, 0 on failure
   */
  int getPINStatus();

  /**
   * Verify the PIN with the card
   *
   * Note: You must do setHandlerPIN() before calling this. That's where you get the new status, not here!
   *
   * @param pin
   * @return 1 on success, 0 on failure
   */
  int verifyPIN(str pin);

  /**
   * Verify the PUK and set a new PIN
   *
   * Note: You must do setHandlerPIN() before calling this. That's where you get the new status, not here!
   *
   * @param pin
   * @return 1 on success, 0 on failure
   */
  int verifyPUK(str puk, str pin);

  /**
   * Set the function to handle PIN requests from the SIM card
   * @param cb - callback function
   */
  void setHandlerPIN(OwlModem_PINHandler_f cb);



  /** Not private because the initialization might call this in a special way */
  OwlModem_PINHandler_f handler_cpin = 0;

 private:
  OwlModemAT *atModem_ = 0;

  // char sim_response_buffer[MODEM_SIM_RESPONSE_BUFFER_SIZE];
  // str sim_response = {.s = sim_response_buffer, .len = 0};

  int handleCPIN(str urc, str data);
};

#endif
