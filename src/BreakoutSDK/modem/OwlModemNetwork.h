/*
 * OwlModemNetwork.h
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
 * \file OwlModemNetwork.h - API for Network Registration Management
 */

#ifndef __OWL_MODEM_NETWORK_H__
#define __OWL_MODEM_NETWORK_H__

#include "enums.h"


#define MODEM_NETWORK_RESPONSE_BUFFER_SIZE 512

/**
 * Handler function signature for Network Registration events (CS)
 * @param stat - status
 * @param lac - local area code
 * @param ci - cell id
 * @param act - access technology
 */
typedef void (*OwlModem_NetworkRegistrationStatusChangeHandler_f)(at_creg_stat_e stat, uint16_t lac, uint32_t ci,
                                                                  at_creg_act_e act);

/**
 * Handler function signature for GPRS Registration events (2G/3G)
 * @param stat - status
 * @param lac - local area code
 * @param ci - cell id
 * @param act - access technology
 * @param rac - routing area code
 */
typedef void (*OwlModem_GPRSRegistrationStatusChangeHandler_f)(at_cgreg_stat_e stat, uint16_t lac, uint32_t ci,
                                                               at_cgreg_act_e act, uint8_t rac);

/**
 * Handler function signature for EPS Registration events (EPC - 2G/3G/LTE/WiFi/etc)
 * @param stat - status
 * @param lac - local area code
 * @param ci - cell id
 * @param act - access technology
 * @param cause_type
 * @param reject_cause
 */
typedef void (*OwlModem_EPSRegistrationStatusChangeHandler_f)(at_cereg_stat_e stat, uint16_t lac, uint32_t ci,
                                                              at_cereg_act_e act, at_cereg_cause_type_e cause_type,
                                                              uint32_t reject_cause);



class OwlModem;


/**
 * Twilio wrapper for the AT serial interface to a modem - Methods to get information from the Network card
 */
class OwlModemNetwork {
 public:
  OwlModemNetwork(OwlModem *owlModem);

  /**
   * Handler for Unsolicited Response Codes from the modem - called from OwlModem on timer, when URC is received
   * @param urc - event id
   * @param data - data of the event
   * @return 1 if the line was handled, 0 if no match here
   */
  int processURC(str urc, str data);



  /**
   * Retrieve the current modem functionality mode.
   * @param out_power_mode - Modem power mode
   * @param out_stk_mode - SIM-Toolkit mode
   * @return 1 on success, 0 on failure
   */
  int getModemFunctionality(at_cfun_power_mode_e *out_power_mode, at_cfun_stk_mode_e *out_stk_mode);

  /**
   * Set the modem functionality mode.
   * @param mode - mode to set
   * @return 1 on success, 0 on failure
   */
  int setModemFunctionality(at_cfun_fun_e fun, at_cfun_rst_e *reset);


  /**
   * Retrieve the current modem MNO profile selection.
   * @param out_profile - Modem MNO profile
   * @return 1 on success, 0 on failure
   */
  int getModemMNOProfile(at_umnoprof_mno_profile_e *out_profile);

  /**
   * Set the current modem MNO profile selection.
   * @param profile - Modem MNO profile to set
   * @return 1 on success, 0 on failure
   */
  int setModemMNOProfile(at_umnoprof_mno_profile_e profile);

  /**
   * Retrieve the current Operator Selection Mode and selected Operator, Radio Access Technology
   * @param out_mode - output current mode
   * @param out_format - output format of the following operator string
   * @param out_oper - output operator string
   * @param max_oper_len - buffer length provided in the operator string
   * @param out_act - output radio access technology type
   * @return 1 on success, 0 on failure
   */
  int getOperatorSelection(at_cops_mode_e *out_mode, at_cops_format_e *out_format, str *out_oper, int max_oper_len,
                           at_cops_act_e *out_act);

  /**
   * Set operator selection mode and/or select manually operator/access technology
   * @param mode - new mode
   * @param opt_format - optional format (to set output and/or indicate what format the next operator string parameter
   * is in
   * @param opt_oper - optional operator string - must also provide format parameter with this
   * @param opt_act - optional radio access technology code - must also provide format and oper parameters with this
   * @return 1 on success, 0 on failure
   */
  int setOperatorSelection(at_cops_mode_e mode, at_cops_format_e *opt_format, str *opt_oper, at_cops_act_e *opt_act);

  /**
   * Get operator list (a.i. do a scan and provide possible values for setting Operator Selection to Manual
   * The format of the response is a list of operators, each formatted as:
   *
   * (<stat>,long <oper>,short <oper>,numeric <oper>[,<act>])[,(<stat>,long <oper>,short <oper>,numeric <oper>[,<act>])
   * [,...]],,(list-of-supported <mode>s),(list of supported <format>s)
   *
   * @param out_response - output buffer to fill with the command response
   * @param max_response_len - length of output buffer
   * @return 1 on success, 0 on failure
   */
  int getOperatorList(str *out_response, int max_response_len);


  /**
   * Retrieve the current Network Registration Status
   * @param out_n - output Unsolicited Result Code status
   * @param out_stat - output network registration status
   * @param out_lac - output Local/Tracking Area Code
   * @param out_ci - output Cell Identifier, or 0xFFFFFFFFu indicates the current value is invalid
   * @param out_act - output Radio Access Technology (AcT)
   * @return 1 on success, 0 on failure
   */
  int getNetworkRegistrationStatus(at_creg_n_e *out_n, at_creg_stat_e *out_stat, uint16_t *out_lac, uint32_t *out_ci,
                                   at_creg_act_e *out_act);

  /**
   * Set the current style of Unregistered Response Code (URC) asynchronous reporting for Network Registration Status.
   * @param n - mode to set
   * @return 1 on success, 0 on failure
   */
  int setNetworkRegistrationURC(at_creg_n_e n);

  /**
   * When setting the Network Registration Unsolicited Response Code to AT_CREG__N__Network_Registration_URC
   * or AT_CREG__N__Network_Registration_and_Location_Information_URC, reports will be generated. This handler
   * will be called when that happens, so that you can handle the event.
   *
   * Note: This will happen asynchronously, so don't forget calling the OwlModem->handleRx() method on an interrupt
   * timer.
   *
   * @param cb
   */
  void setHandlerNetworkRegistrationURC(OwlModem_NetworkRegistrationStatusChangeHandler_f cb);


  /**
   * Retrieve the current GPRS Registration Status
   * @param out_n - output Unsolicited Result Code status
   * @param out_stat - output network registration status
   * @param out_lac - output Local/Tracking Area Code
   * @param out_ci - output Cell Identifier, or 0xFFFFFFFFu indicates the current value is invalid
   * @param out_act - output Radio Access Technology (AcT)
   * @param out_rac - output Routing Area Code
   * @return 1 on success, 0 on failure
   */
  int getGPRSRegistrationStatus(at_cgreg_n_e *out_n, at_cgreg_stat_e *out_stat, uint16_t *out_lac, uint32_t *out_ci,
                                at_cgreg_act_e *out_act, uint8_t *out_rac);

  /**
   * Set the current style of Unregistered Response Code (URC) asynchronous reporting for GPRS Registration Status.
   * @param n - mode to set
   * @return 1 on success, 0 on failure
   */
  int setGPRSRegistrationURC(at_cgreg_n_e n);

  /**
   * When setting the GPRS Registration Unsolicited Response Code to AT_CREG__N__GPRS_Registration_URC
   * or AT_CREG__N__GPRS_Registration_and_Location_Information_URC, reports will be generated. This handler
   * will be called when that happens, so that you can handle the event.
   *
   * Note: This will happen asynchronously, so don't forget calling the OwlModem->handleRx() method on an interrupt
   * timer.
   *
   * @param cb
   */
  void setHandlerGPRSRegistrationURC(OwlModem_GPRSRegistrationStatusChangeHandler_f cb);


  /**
   * Retrieve the current EPS Registration Status
   * @param out_n - output Unsolicited Result Code status
   * @param out_stat - output network registration status
   * @param out_lac - output Local/Tracking Area Code
   * @param out_ci - output Cell Identifier, or 0xFFFFFFFFu indicates the current value is invalid
   * @param out_act - output Radio Access Technology (AcT)
   * @param out_cause_type - output cause type for reject cause
   * @param out_reject_cause - output reject cause
   * @return 1 on success, 0 on failure
   */
  int getEPSRegistrationStatus(at_cereg_n_e *out_n, at_cereg_stat_e *out_stat, uint16_t *out_lac, uint32_t *out_ci,
                               at_cereg_act_e *out_act, at_cereg_cause_type_e *out_cause_type,
                               uint32_t *out_reject_cause);

  /**
   * Set the current style of Unregistered Response Code (URC) asynchronous reporting for EPS Registration Status.
   * @param n - mode to set
   * @return 1 on success, 0 on failure
   */
  int setEPSRegistrationURC(at_cereg_n_e n);

  /**
   * When setting the EPS Registration Unsolicited Response Code to AT_CREG__N__EPS_Registration_URC
   * or AT_CREG__N__EPS_Registration_and_Location_Information_URC, reports will be generated. This handler
   * will be called when that happens, so that you can handle the event.
   *
   * Note: This will happen asynchronously, so don't forget calling the OwlModem->handleRx() method on an interrupt
   * timer.
   *
   * @param cb
   */
  void setHandlerEPSRegistrationURC(OwlModem_EPSRegistrationStatusChangeHandler_f cb);


  /**
   * Retrieve the Signal Quality at the moment
   * @param out_rssi - output RSSI - see at_csq_rssi_e for special values
   * @param out_qual - output Signal Quality (aka channel Bit Error Rate)
   * @return 1 on success, 0 on failure
   */
  int getSignalQuality(at_csq_rssi_e *out_rssi, at_csq_qual_e *out_qual);



 private:
  OwlModem *owlModem = 0;

  char network_response_buffer[MODEM_NETWORK_RESPONSE_BUFFER_SIZE];
  str network_response = {.s = network_response_buffer, .len = 0};

  OwlModem_NetworkRegistrationStatusChangeHandler_f handler_creg = 0;
  OwlModem_GPRSRegistrationStatusChangeHandler_f handler_cgreg   = 0;
  OwlModem_EPSRegistrationStatusChangeHandler_f handler_cereg    = 0;

  int processURCNetworkRegistration(str urc, str data);
  int processURCGPRSRegistration(str urc, str data);
  int processURCEPSRegistration(str urc, str data);


  void parseNetworkRegistrationStatus(str response, at_creg_n_e *out_n, at_creg_stat_e *out_stat, uint16_t *out_lac,
                                      uint32_t *out_ci, at_creg_act_e *out_act);
  void parseGPRSRegistrationStatus(str response, at_cgreg_n_e *out_n, at_cgreg_stat_e *out_stat, uint16_t *out_lac,
                                   uint32_t *out_ci, at_cgreg_act_e *out_act, uint8_t *out_rac);
  void parseEPSRegistrationStatus(str response, at_cereg_n_e *out_n, at_cereg_stat_e *out_stat, uint16_t *out_lac,
                                  uint32_t *out_ci, at_cereg_act_e *out_act, at_cereg_cause_type_e *out_cause_type,
                                  uint32_t *out_reject_cause);

  typedef struct {
    at_creg_n_e n;
    at_creg_stat_e stat;
    uint16_t lac;
    uint32_t ci;
    at_creg_act_e act;
  } owl_network_status_t;

  owl_network_status_t last_network_status = {
      .n    = AT_CREG__N__URC_Disabled,
      .stat = AT_CREG__Stat__Not_Registered,
      .lac  = 0,
      .ci   = 0xFFFFFFFFu,
      .act  = AT_CREG__AcT__invalid,
  };

  typedef struct {
    at_cgreg_n_e n;
    at_cgreg_stat_e stat;
    uint16_t lac;
    uint32_t ci;
    at_cgreg_act_e act;
    uint8_t rac;
  } owl_last_gprs_status_t;

  owl_last_gprs_status_t last_gprs_status = {
      .n    = AT_CGREG__N__URC_Disabled,
      .stat = AT_CGREG__Stat__Not_Registered,
      .lac  = 0,
      .ci   = 0xFFFFFFFFu,
      .act  = AT_CGREG__AcT__invalid,
      .rac  = 0,
  };

  typedef struct {
    at_cereg_n_e n;
    at_cereg_stat_e stat;
    uint16_t lac;
    uint32_t ci;
    at_cereg_act_e act;
    at_cereg_cause_type_e cause_type;
    uint32_t reject_cause;
  } owl_last_eps_status_t;

  owl_last_eps_status_t last_eps_status = {
      .n            = AT_CEREG__N__URC_Disabled,
      .stat         = AT_CEREG__Stat__Not_Registered,
      .lac          = 0,
      .ci           = 0xFFFFFFFFu,
      .act          = AT_CEREG__AcT__invalid,
      .cause_type   = AT_CEREG__Cause_Type__EMM_Cause,
      .reject_cause = 0,
  };
};

#endif
