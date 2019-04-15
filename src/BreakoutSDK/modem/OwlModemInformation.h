/*
 * OwlModemInformation.h
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
 * \file OwlModemInformation.h - API for retrieving various data from the modem
 */

#ifndef __OWL_MODEM_INFORMATION_H__
#define __OWL_MODEM_INFORMATION_H__

#include "enums.h"
#include "OwlModemAT.h"

/**
 * Twilio wrapper for the AT serial interface to a modem - Methods to get information from the modem
 */
class OwlModemInformation {
 public:
  OwlModemInformation(OwlModemAT *atModem);

  /*
   * Methods to get information from the modem or SIM card
   *
   */

  /**
   * Retrieve Product Identification Information
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getProductIdentification(str *out_response, int max_response_len);

  /**
   * Retrieve Manufacturer Information
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getManufacturer(str *out_response, int max_response_len);

  /**
   * Retrieve Model Information
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getModel(str *out_response, int max_response_len);

  /**
   * Retrieve Version Information
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getVersion(str *out_response, int max_response_len);

  /**
   * Retrieve IMEI (modem serial number)
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getIMEI(str *out_response, int max_response_len);

  /**
   * Retrieve Battery Charge Levels
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getBatteryChargeLevels(str *out_response, int max_response_len);

  /**
   * Get indicators from the modem (various statuses, battery information, etc)
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getIndicators(str *out_response, int max_response_len);

  /**
   * Get help on the format of the indicators
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getIndicatorsHelp(str *out_response, int max_response_len);



 private:
  OwlModemAT *atModem_ = 0;
};

#endif
