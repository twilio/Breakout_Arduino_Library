/*
 * OwlModemInformation.cpp
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
 * \file OwlModemInformation.cpp - API for retrieving various data from the modem
 */

#include "OwlModemInformation.h"

#include "OwlModem.h"



OwlModemInformation::OwlModemInformation(OwlModem *owlModem) : owlModem(owlModem) {
}


int OwlModemInformation::getProductIdentification(str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  return owlModem->doCommand("ATI", 1000, out_response, max_response_len) == AT_Result_Code__OK;
}

int OwlModemInformation::getManufacturer(str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  return owlModem->doCommand("AT+CGMI", 1000, out_response, max_response_len) == AT_Result_Code__OK;
}

int OwlModemInformation::getModel(str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  return owlModem->doCommand("AT+CGMM", 1000, out_response, max_response_len) == AT_Result_Code__OK;
}

int OwlModemInformation::getVersion(str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  return owlModem->doCommand("AT+CGMR", 1000, out_response, max_response_len) == AT_Result_Code__OK;
}

int OwlModemInformation::getIMEI(str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  return owlModem->doCommand("AT+CGSN", 1000, out_response, max_response_len) == AT_Result_Code__OK;
}

static str s_cbc = STRDECL("+CBC: ");

int OwlModemInformation::getBatteryChargeLevels(str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  int result = owlModem->doCommand("AT+CBC", 1000, out_response, max_response_len) == AT_Result_Code__OK;
  if (!result) return 0;
  owlModem->filterResponse(s_cbc, out_response);
  return 1;
}

static str s_cind = STRDECL("+CIND: ");

int OwlModemInformation::getIndicators(str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  int result = owlModem->doCommand("AT+CIND?", 1000, out_response, max_response_len) == AT_Result_Code__OK;
  if (!result) return 0;
  owlModem->filterResponse(s_cind, out_response);
  return 1;
}

int OwlModemInformation::getIndicatorsHelp(str *out_response, int max_response_len) {
  if (out_response) out_response->len = 0;
  int result = owlModem->doCommand("AT+CIND=?", 1000, out_response, max_response_len) == AT_Result_Code__OK;
  if (!result) return 0;
  owlModem->filterResponse(s_cind, out_response);
  return 1;
}
