/*
 * OwlModemSIM.cpp
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
 * \file OwlModemSIM.cpp - API for retrieving various data from the SIM card
 */

#include "OwlModemSIM.h"

#include <stdio.h>

#include "OwlModem.h"



OwlModemSIM::OwlModemSIM(OwlModem *owlModem) : owlModem(owlModem) {
}



static str s_cpin = {.s = "+CPIN", .len = 5};

int OwlModemSIM::handleCPIN(str urc, str data) {
  if (!str_equal(urc, s_cpin)) return 0;
  if (!this->handler_cpin) {
    LOG(L_INFO,
        "Received URC for PIN [%.*s]. Set a handler with setHandlerPIN() if you wish to receive this event "
        "in your application\r\n",
        data.len, data.s);
  } else {
    (this->handler_cpin)(data);
  }
  return 1;
}

int OwlModemSIM::processURC(str urc, str data) {
  if (handleCPIN(urc, data)) return 1;
  return 0;
}


static str s_ccid = STRDECL("+CCID: ");

int OwlModemSIM::getICCID(str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  int result = owlModem->doCommand("AT+CCID", 1000, out_response, max_response_len) == AT_Result_Code__OK;
  if (!result) return 0;
  owlModem->filterResponse(s_ccid, out_response);
  return 1;
}

int OwlModemSIM::getIMSI(str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  return owlModem->doCommand("AT+CIMI", 1000, out_response, max_response_len) == AT_Result_Code__OK;
}

static str s_cnum = STRDECL("+CNUM: ");

int OwlModemSIM::getMSISDN(str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  int result = owlModem->doCommand("AT+CNUM", 1000, out_response, max_response_len) == AT_Result_Code__OK;
  if (!result) return 0;
  owlModem->filterResponse(s_cnum, out_response);
  return 1;
}

int OwlModemSIM::getPINStatus() {
  int result = 0;
  result     = owlModem->doCommand("AT+CPIN?", 10 * 1000, 0, 0) == AT_Result_Code__OK;
  return result;
}

int OwlModemSIM::verifyPIN(str pin) {
  char buffer[64];
  snprintf(buffer, 64, "AT+CPIN=%.*s", pin.len, pin.s);
  int result = owlModem->doCommand(buffer, 10 * 1000, 0, 0) == AT_Result_Code__OK;
  return result;
}

int OwlModemSIM::verifyPUK(str puk, str pin) {
  char buffer[64];
  snprintf(buffer, 64, "AT+CPIN=%.*s,%.*s", puk.len, puk.s, pin.len, pin.s);
  int result = owlModem->doCommand(buffer, 10 * 1000, 0, 0) == AT_Result_Code__OK;
  return result;
}

void OwlModemSIM::setHandlerPIN(OwlModem_PINHandler_f cb) {
  this->handler_cpin = cb;
}
