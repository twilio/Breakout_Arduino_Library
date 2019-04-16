/*
 * OwlModemNetwork.cpp
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
 * \file OwlModemNetwork.cpp - API for Network Registration Management
 */

#include "OwlModemNetwork.h"

#include <stdio.h>

#include "OwlModem.h"



OwlModemNetwork::OwlModemNetwork(OwlModemAT *atModem) : atModem_(atModem) {
  if (atModem_ != nullptr) {
    atModem_->registerUrcHandler(OwlModemNetwork::processURC, this);
  }
}

static str s_creg      = STRDECL("+CREG");
static str s_creg_full = STRDECL("+CREG: ");

void OwlModemNetwork::parseNetworkRegistrationStatus(str response, at_creg_n_e *out_n, at_creg_stat_e *out_stat,
                                                     uint16_t *out_lac, uint32_t *out_ci, at_creg_act_e *out_act) {
  int cnt   = 0;
  str token = {0};
  if (out_n) *out_n = AT_CREG__N__URC_Disabled;
  if (out_stat) *out_stat = AT_CREG__Stat__Not_Registered;
  if (out_lac) *out_lac = 0;
  if (out_ci) *out_ci = 0xFFFFFFFFu;
  if (out_act) *out_act = AT_CREG__AcT__invalid;

  str data = response;
  str_skipover_prefix(&data, s_creg_full);
  int has_n = 1;
  while (str_tok(data, ",", &token)) {
    if (cnt == 1) {
      if (token.len && token.s[0] == '\"') has_n = 0;
      break;
    }
    cnt++;
  }
  token = {0};
  if (has_n)
    cnt = 0;
  else
    cnt = 1;
  while (str_tok(data, ",", &token)) {
    switch (cnt) {
      case 0:
        if (out_n) *out_n = (at_creg_n_e)str_to_long_int(token, 10);
        break;
      case 1:
        if (out_stat) *out_stat = (at_creg_stat_e)str_to_long_int(token, 10);
        break;
      case 2:
        if (out_lac) *out_lac = (uint16_t)str_to_long_int(token, 16);
        break;
      case 3:
        if (out_ci) *out_ci = (uint32_t)str_to_uint32_t(token, 16);
        break;
      case 4:
        if (out_act) *out_act = (at_creg_act_e)str_to_long_int(token, 16);
        break;
      default:
        LOG(L_ERR, "Not handled %d(-th) token [%.*s] data was [%.*s]\r\n", cnt, token.len, token.s, data.len, data.s);
    }
    cnt++;
  }
}

int OwlModemNetwork::processURCNetworkRegistration(str urc, str data) {
  if (!str_equal(urc, s_creg)) return 0;
  this->parseNetworkRegistrationStatus(data, &last_network_status.n, &last_network_status.stat,
                                       &last_network_status.lac, &last_network_status.ci, &last_network_status.act);

  if (!this->handler_creg) {
    LOG(L_INFO,
        "Received URC for CREG [%.*s]. Set a handler with setHandlerNetworkRegistrationURC() if you wish to "
        "receive this event in your application\r\n",
        data.len, data.s);
  } else {
    (this->handler_creg)(last_network_status.stat, last_network_status.lac, last_network_status.ci,
                         last_network_status.act);
  }
  return 1;
}



static str s_cgreg      = STRDECL("+CGREG");
static str s_cgreg_full = STRDECL("+CGREG: ");

void OwlModemNetwork::parseGPRSRegistrationStatus(str response, at_cgreg_n_e *out_n, at_cgreg_stat_e *out_stat,
                                                  uint16_t *out_lac, uint32_t *out_ci, at_cgreg_act_e *out_act,
                                                  uint8_t *out_rac) {
  int cnt   = 0;
  str token = {0};
  if (out_n) *out_n = AT_CGREG__N__URC_Disabled;
  if (out_stat) *out_stat = AT_CGREG__Stat__Not_Registered;
  if (out_lac) *out_lac = 0;
  if (out_ci) *out_ci = 0xFFFFFFFFu;
  if (out_act) *out_act = AT_CGREG__AcT__invalid;
  if (out_rac) *out_rac = 0;

  str data = response;
  str_skipover_prefix(&data, s_cgreg_full);
  int has_n = 1;
  while (str_tok(data, ",", &token)) {
    if (cnt == 1) {
      if (token.len && token.s[0] == '\"') has_n = 0;
      break;
    }
    cnt++;
  }
  token = {0};
  if (has_n)
    cnt = 0;
  else
    cnt = 1;
  while (str_tok(data, ",", &token)) {
    switch (cnt) {
      case 0:
        if (out_n) *out_n = (at_cgreg_n_e)str_to_long_int(token, 10);
        break;
      case 1:
        if (out_stat) *out_stat = (at_cgreg_stat_e)str_to_long_int(token, 10);
        break;
      case 2:
        if (out_lac) *out_lac = (uint16_t)str_to_long_int(token, 16);
        break;
      case 3:
        if (out_ci) *out_ci = (uint32_t)str_to_uint32_t(token, 16);
        break;
      case 4:
        if (out_act) *out_act = (at_cgreg_act_e)str_to_long_int(token, 10);
        break;
      case 5:
        if (out_rac) *out_rac = (uint8_t)str_to_long_int(token, 16);
        break;
      default:
        LOG(L_ERR, "Not handled %d(-th) token [%.*s] data was [%.*s]\r\n", cnt, token.len, token.s, data.len, data.s);
    }
    cnt++;
  }
}

int OwlModemNetwork::processURCGPRSRegistration(str urc, str data) {
  if (!str_equal(urc, s_cgreg)) return 0;
  this->parseGPRSRegistrationStatus(data, &last_gprs_status.n, &last_gprs_status.stat, &last_gprs_status.lac,
                                    &last_gprs_status.ci, &last_gprs_status.act, &last_gprs_status.rac);
  if (!this->handler_cgreg) {
    LOG(L_INFO,
        "Received URC for CGREG [%.*s]. Set a handler with setHandlerGPRSRegistrationURC() if you wish to "
        "receive this event in your application\r\n",
        data.len, data.s);
  } else {
    (this->handler_cgreg)(last_gprs_status.stat, last_gprs_status.lac, last_gprs_status.ci, last_gprs_status.act,
                          last_gprs_status.rac);
  }
  return 1;
}



static str s_cereg      = STRDECL("+CEREG");
static str s_cereg_full = STRDECL("+CEREG: ");

void OwlModemNetwork::parseEPSRegistrationStatus(str response, at_cereg_n_e *out_n, at_cereg_stat_e *out_stat,
                                                 uint16_t *out_lac, uint32_t *out_ci, at_cereg_act_e *out_act,
                                                 at_cereg_cause_type_e *out_cause_type, uint32_t *out_reject_cause) {
  int cnt   = 0;
  str token = {0};
  if (out_n) *out_n = AT_CEREG__N__URC_Disabled;
  if (out_stat) *out_stat = AT_CEREG__Stat__Not_Registered;
  if (out_lac) *out_lac = 0;
  if (out_ci) *out_ci = 0xFFFFFFFFu;
  if (out_act) *out_act = AT_CEREG__AcT__invalid;
  if (out_cause_type) *out_cause_type = AT_CEREG__Cause_Type__EMM_Cause;
  if (out_reject_cause) *out_reject_cause = 0;

  str data = response;
  str_skipover_prefix(&data, s_cereg_full);
  int has_n = 1;
  while (str_tok(data, ",", &token)) {
    if (cnt == 1) {
      if (token.len && token.s[0] == '\"') has_n = 0;
      break;
    }
    cnt++;
  }
  token = {0};
  if (has_n)
    cnt = 0;
  else
    cnt = 1;
  while (str_tok(data, ",", &token)) {
    switch (cnt) {
      case 0:
        if (out_n) *out_n = (at_cereg_n_e)str_to_long_int(token, 10);
        break;
      case 1:
        if (out_stat) *out_stat = (at_cereg_stat_e)str_to_long_int(token, 10);
        break;
      case 2:
        if (out_lac) *out_lac = (uint16_t)str_to_long_int(token, 16);
        break;
      case 3:
        if (out_ci) *out_ci = (uint32_t)str_to_uint32_t(token, 16);
        break;
      case 4:
        if (out_act) *out_act = (at_cereg_act_e)str_to_long_int(token, 10);
        break;
      case 5:
        if (out_cause_type) *out_cause_type = (at_cereg_cause_type_e)str_to_long_int(token, 10);
        break;
      case 6:
        if (out_reject_cause) *out_reject_cause = (uint32_t)str_to_uint32_t(token, 10);
        break;
      default:
        LOG(L_ERR, "Not handled %d(-th) token [%.*s] data was [%.*s]\r\n", cnt, token.len, token.s, data.len, data.s);
    }
    cnt++;
  }
}

int OwlModemNetwork::processURCEPSRegistration(str urc, str data) {
  if (!str_equal(urc, s_cereg)) return 0;
  this->parseEPSRegistrationStatus(data, &last_eps_status.n, &last_eps_status.stat, &last_eps_status.lac,
                                   &last_eps_status.ci, &last_eps_status.act, &last_eps_status.cause_type,
                                   &last_eps_status.reject_cause);
  if (!this->handler_cereg) {
    LOG(L_INFO,
        "Received URC for CEREG [%.*s]. Set a handler with setHandlerEPSRegistrationURC() if you wish to "
        "receive this event in your application\r\n",
        data.len, data.s);
  } else {
    (this->handler_cereg)(last_eps_status.stat, last_eps_status.lac, last_eps_status.ci, last_eps_status.act,
                          last_eps_status.cause_type, last_eps_status.reject_cause);
  }
  return 1;
}



int OwlModemNetwork::processURC(str urc, str data, void *instance) {
  OwlModemNetwork *inst = reinterpret_cast<OwlModemNetwork *>(instance);

  if (inst->processURCNetworkRegistration(urc, data)) return 1;
  if (inst->processURCGPRSRegistration(urc, data)) return 1;
  if (inst->processURCEPSRegistration(urc, data)) return 1;
  return 0;
}



static str s_cfun = STRDECL("+CFUN: ");

int OwlModemNetwork::getModemFunctionality(at_cfun_power_mode_e *out_power_mode, at_cfun_stk_mode_e *out_stk_mode) {
  if (out_power_mode) *out_power_mode = AT_CFUN__POWER_MODE__Minimum_Functionality;
  if (out_stk_mode) *out_stk_mode = AT_CFUN__STK_MODE__Interface_Disabled_Proactive_SIM_APPL_Enabled_0;
  int result = atModem_->doCommandBlocking("AT+CFUN?", 15 * 1000, &network_response,
                                           MODEM_NETWORK_RESPONSE_BUFFER_SIZE) == AT_Result_Code__OK;
  if (!result) return 0;
  OwlModemAT::filterResponse(s_cfun, &network_response);
  str token = {0};
  int cnt   = 0;
  while (str_tok(network_response, ",\r\n", &token)) {
    switch (cnt) {
      case 0:
        if (out_power_mode) *out_power_mode = (at_cfun_power_mode_e)str_to_long_int(token, 10);
        break;
      case 1:
        if (out_stk_mode) *out_stk_mode = (at_cfun_stk_mode_e)str_to_long_int(token, 10);
        break;
      default:
        LOG(L_ERR, "Not handled %d(-th) token [%.*s]\r\n", cnt, token.len, token.s);
    }
    cnt++;
  }
  return 1;
}

int OwlModemNetwork::setModemFunctionality(at_cfun_fun_e fun, at_cfun_rst_e *reset) {
  char buffer[64];
  if (!reset)
    snprintf(buffer, 64, "AT+CFUN=%d", fun);
  else
    snprintf(buffer, 64, "AT+CFUN=%d,%d", fun, *reset);
  return atModem_->doCommandBlocking(buffer, 3 * 60 * 1000, &network_response, MODEM_NETWORK_RESPONSE_BUFFER_SIZE) ==
         AT_Result_Code__OK;
}



static str s_umnoprof = STRDECL("+UMNOPROF: ");

int OwlModemNetwork::getModemMNOProfile(at_umnoprof_mno_profile_e *out_profile) {
  if (out_profile) *out_profile = AT_UMNOPROF__MNO_PROFILE__SW_Default;
  int result = atModem_->doCommandBlocking("AT+UMNOPROF?", 15 * 1000, &network_response,
                                           MODEM_NETWORK_RESPONSE_BUFFER_SIZE) == AT_Result_Code__OK;
  if (!result) return 0;
  OwlModemAT::filterResponse(s_umnoprof, &network_response);
  *out_profile = (at_umnoprof_mno_profile_e)str_to_long_int(network_response, 10);
  return 1;
}

int OwlModemNetwork::setModemMNOProfile(at_umnoprof_mno_profile_e profile) {
  char buffer[64];
  snprintf(buffer, 64, "AT+UMNOPROF=%d", profile);
  return atModem_->doCommandBlocking(buffer, 3 * 60 * 1000, &network_response, MODEM_NETWORK_RESPONSE_BUFFER_SIZE) ==
         AT_Result_Code__OK;
}


static str s_cops = STRDECL("+COPS: ");

int OwlModemNetwork::getOperatorSelection(at_cops_mode_e *out_mode, at_cops_format_e *out_format, str *out_oper,
                                          int max_oper_len, at_cops_act_e *out_act) {
  int cnt   = 0;
  str token = {0};
  char save = 0;
  if (out_mode) *out_mode = AT_COPS__Mode__Automatic_Selection;
  if (out_format) *out_format = AT_COPS__Format__Long_Alphanumeric;
  if (out_oper) out_oper->len = 0;
  if (out_act) *out_act = (at_cops_act_e)0;
  int result = atModem_->doCommandBlocking("AT+COPS?", 3 * 60 * 1000, &network_response,
                                           MODEM_NETWORK_RESPONSE_BUFFER_SIZE) == AT_Result_Code__OK;
  if (!result) return 0;
  OwlModemAT::filterResponse(s_cops, &network_response);
  while (str_tok(network_response, ",\r\n", &token)) {
    switch (cnt) {
      case 0:
        if (out_mode) *out_mode = (at_cops_mode_e)str_to_long_int(token, 10);
        break;
      case 1:
        if (out_format) *out_format = (at_cops_format_e)str_to_long_int(token, 10);
        break;
      case 2:
        if (out_oper) {
          memcpy(out_oper->s, token.s, token.len > max_oper_len ? max_oper_len : token.len);
          out_oper->len = token.len;
        }
        break;
      case 3:
        if (out_act) *out_act = (at_cops_act_e)str_to_long_int(token, 10);
        break;
      default:
        LOG(L_ERR, "Not handled %d(-th) token [%.*s]\r\n", cnt, token.len, token.s);
    }
    cnt++;
  }
  return 1;
}

int OwlModemNetwork::setOperatorSelection(at_cops_mode_e mode, at_cops_format_e *opt_format, str *opt_oper,
                                          at_cops_act_e *opt_act) {
  if (opt_oper && !opt_format) {
    LOG(L_ERR, " - when opt_oper is specific, opt_format must be also specified\r\n");
    return 0;
  }
  if (opt_act && !opt_oper) {
    LOG(L_ERR, " - when opt_act is specific, opt_format and opt_oper must be also specified\r\n");
    return 0;
  }
  char buf[64];
  if (opt_oper) {
    if (opt_act) {
      snprintf(buf, 64, "AT+COPS=%d,%d,%.*s,%d", mode, *opt_format, opt_oper->len, opt_oper->s, *opt_act);
    } else {
      snprintf(buf, 64, "AT+COPS=%d,%d,%.*s", mode, *opt_format, opt_oper->len, opt_oper->s);
    }
  } else {
    snprintf(buf, 64, "AT+COPS=%d", mode);
  }
  int result = atModem_->doCommandBlocking(buf, 3 * 60 * 1000, 0, 0) == AT_Result_Code__OK;
  if (!result) return 0;
  return 1;
}

int OwlModemNetwork::getOperatorList(str *out_response, int max_response_len) {
  // TODO - parse maybe the structures. But then we'll have a more complex list structure, which should be freed by
  // the user, so more complex to use.
  if (out_response) out_response->len = 0;
  int result =
      atModem_->doCommandBlocking("AT+COPS=?", 3 * 60 * 1000, out_response, max_response_len) == AT_Result_Code__OK;
  if (!result) return 0;
  OwlModemAT::filterResponse(s_cops, out_response);
  return 1;
}



int OwlModemNetwork::getNetworkRegistrationStatus(at_creg_n_e *out_n, at_creg_stat_e *out_stat, uint16_t *out_lac,
                                                  uint32_t *out_ci, at_creg_act_e *out_act) {
  if (out_n) *out_n = AT_CREG__N__URC_Disabled;
  if (out_stat) *out_stat = AT_CREG__Stat__Not_Registered;
  if (out_lac) *out_lac = 0;
  if (out_ci) *out_ci = 0xFFFFFFFFu;
  if (out_act) *out_act = AT_CREG__AcT__invalid;
  int result = atModem_->doCommandBlocking("AT+CREG?", 1000, &network_response, MODEM_NETWORK_RESPONSE_BUFFER_SIZE) ==
               AT_Result_Code__OK;
  if (!result) return 0;
  // the URC handlers are catching this, so serving from local cache
  if (out_n) *out_n = last_network_status.n;
  if (out_stat) *out_stat = last_network_status.stat;
  if (out_lac) *out_lac = last_network_status.lac;
  if (out_ci) *out_ci = last_network_status.ci;
  if (out_act) *out_act = last_network_status.act;
  return 1;
}

int OwlModemNetwork::setNetworkRegistrationURC(at_creg_n_e n) {
  char buf[64];
  snprintf(buf, 64, "AT+CREG=%d", n);
  return atModem_->doCommandBlocking(buf, 180000, 0, 0) == AT_Result_Code__OK;
}

void OwlModemNetwork::setHandlerNetworkRegistrationURC(OwlModem_NetworkRegistrationStatusChangeHandler_f cb) {
  this->handler_creg = cb;
}


int OwlModemNetwork::getGPRSRegistrationStatus(at_cgreg_n_e *out_n, at_cgreg_stat_e *out_stat, uint16_t *out_lac,
                                               uint32_t *out_ci, at_cgreg_act_e *out_act, uint8_t *out_rac) {
  if (out_n) *out_n = AT_CGREG__N__URC_Disabled;
  if (out_stat) *out_stat = AT_CGREG__Stat__Not_Registered;
  if (out_lac) *out_lac = 0;
  if (out_ci) *out_ci = 0xFFFFFFFFu;
  if (out_act) *out_act = AT_CGREG__AcT__invalid;
  if (out_rac) *out_rac = 0;
  int result = atModem_->doCommandBlocking("AT+CGREG?", 1000, &network_response, MODEM_NETWORK_RESPONSE_BUFFER_SIZE) ==
               AT_Result_Code__OK;
  if (!result) return 0;
  // the URC handlers are catching this, so serving from local cache
  if (out_n) *out_n = last_gprs_status.n;
  if (out_stat) *out_stat = last_gprs_status.stat;
  if (out_lac) *out_lac = last_gprs_status.lac;
  if (out_ci) *out_ci = last_gprs_status.ci;
  if (out_act) *out_act = last_gprs_status.act;
  if (out_act) *out_rac = last_gprs_status.rac;
  return 1;
}

int OwlModemNetwork::setGPRSRegistrationURC(at_cgreg_n_e n) {
  char buf[64];
  snprintf(buf, 64, "AT+CGREG=%d", n);
  return atModem_->doCommandBlocking(buf, 180000, 0, 0) == AT_Result_Code__OK;
}

void OwlModemNetwork::setHandlerGPRSRegistrationURC(OwlModem_GPRSRegistrationStatusChangeHandler_f cb) {
  this->handler_cgreg = cb;
}

int OwlModemNetwork::getEPSRegistrationStatus(at_cereg_n_e *out_n, at_cereg_stat_e *out_stat, uint16_t *out_lac,
                                              uint32_t *out_ci, at_cereg_act_e *out_act,
                                              at_cereg_cause_type_e *out_cause_type, uint32_t *out_reject_cause) {
  if (out_n) *out_n = AT_CEREG__N__URC_Disabled;
  if (out_stat) *out_stat = AT_CEREG__Stat__Not_Registered;
  if (out_lac) *out_lac = 0;
  if (out_ci) *out_ci = 0xFFFFFFFFu;
  if (out_act) *out_act = AT_CEREG__AcT__invalid;
  if (out_cause_type) *out_cause_type = AT_CEREG__Cause_Type__EMM_Cause;
  if (out_reject_cause) *out_reject_cause = 0;
  int result = atModem_->doCommandBlocking("AT+CEREG?", 1000, &network_response, MODEM_NETWORK_RESPONSE_BUFFER_SIZE) ==
               AT_Result_Code__OK;
  if (!result) return 0;
  // the URC handlers are catching this, so serving from local cache
  if (out_n) *out_n = last_eps_status.n;
  if (out_stat) *out_stat = last_eps_status.stat;
  if (out_lac) *out_lac = last_eps_status.lac;
  if (out_ci) *out_ci = last_eps_status.ci;
  if (out_act) *out_act = last_eps_status.act;
  if (out_cause_type) *out_cause_type = last_eps_status.cause_type;
  if (out_reject_cause) *out_reject_cause = last_eps_status.reject_cause;
  return 1;
}

int OwlModemNetwork::setEPSRegistrationURC(at_cereg_n_e n) {
  char buf[64];
  snprintf(buf, 64, "AT+CEREG=%d", n);
  return atModem_->doCommandBlocking(buf, 180000, 0, 0) == AT_Result_Code__OK;
}

void OwlModemNetwork::setHandlerEPSRegistrationURC(OwlModem_EPSRegistrationStatusChangeHandler_f cb) {
  this->handler_cereg = cb;
}



static str s_csq = STRDECL("+CSQ: ");

int OwlModemNetwork::getSignalQuality(at_csq_rssi_e *out_rssi, at_csq_qual_e *out_qual) {
  int cnt   = 0;
  str token = {0};
  char save = 0;
  if (out_rssi) *out_rssi = AT_CSQ__RSSI__Not_Known_or_Detectable_99;
  if (out_qual) *out_qual = AT_CSQ__Qual__Not_Known_or_Not_Detectable;
  int result = atModem_->doCommandBlocking("AT+CSQ", 1000, &network_response, MODEM_NETWORK_RESPONSE_BUFFER_SIZE) ==
               AT_Result_Code__OK;
  if (!result) return 0;
  OwlModemAT::filterResponse(s_csq, &network_response);
  while (str_tok(network_response, ",", &token)) {
    switch (cnt) {
      case 0:
        if (out_rssi) *out_rssi = (at_csq_rssi_e)str_to_long_int(token, 10);
        break;
      case 1:
        if (out_qual) *out_qual = (at_csq_qual_e)str_to_long_int(token, 10);
        break;
      default:
        LOG(L_ERR, "Not handled %d(-th) token [%.*s]\r\n", cnt, token.len, token.s);
    }
    cnt++;
  }
  return 1;
}
