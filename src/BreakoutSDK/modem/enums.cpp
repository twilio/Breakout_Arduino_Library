/*
 * enums.cpp
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
 * \file enums.cpp - various enumerations for AT commands and parameters
 */

#include "enums.h"



struct {
  str value;
  at_result_code_e code;
} at_result_codes[] = {
    {.value = {.s = "OK", .len = 2}, .code = AT_Result_Code__OK},
    {.value = {.s = "CONNECT", .len = 7}, .code = AT_Result_Code__CONNECT},
    {.value = {.s = "RING", .len = 4}, .code = AT_Result_Code__RING},
    {.value = {.s = "NO CARRIER", .len = 10}, .code = AT_Result_Code__NO_CARRIER},
    {.value = {.s = "ERROR", .len = 5}, .code = AT_Result_Code__ERROR},
    {.value = {.s = "CONNECT 1200", .len = 12}, .code = AT_Result_Code__CONNECT_1200},
    {.value = {.s = "NO DIALTONE", .len = 11}, .code = AT_Result_Code__NO_DIALTONE},
    {.value = {.s = "BUSY", .len = 4}, .code = AT_Result_Code__BUSY},
    {.value = {.s = "NO ANSWER", .len = 9}, .code = AT_Result_Code__NO_ANSWER},
    {.value = {0}, .code = AT_Result_Code__unknown},
};

static char c_cme_error[] = "+CME ERROR: ";

at_result_code_e at_result_code_extract(const char *value, int max_len) {
  if (max_len < 3) {  // prompt for data '\r\n>'
    return AT_Result_Code__unknown;
  }

  if (value[0] == '\r' && value[1] == '\n' && value[2] == '>') {
    return AT_Result_Code__wait_input;
  }

  /* "Normal" result code, must be at least <CR><LF>OK<CR><LF>, so 6 bytes */
  if (max_len < 6) return AT_Result_Code__unknown;
  if (value[0] != '\r' || value[1] != '\n') return AT_Result_Code__unknown;
  if (strncmp(value + 2, c_cme_error, strlen(c_cme_error)) == 0) {
    return AT_Result_Code__cme_error;
  }
  int i;
  for (i = 0; at_result_codes[i].code != AT_Result_Code__unknown; i++) {
    if (at_result_codes[i].value.len <= max_len - 4 &&
        strncmp(value + 2, at_result_codes[i].value.s, at_result_codes[i].value.len) == 0 &&
        value[2 + at_result_codes[i].value.len] == '\r' && value[3 + at_result_codes[i].value.len] == '\n') {
      return at_result_codes[i].code;
    }
  }

  return AT_Result_Code__unknown;
}

char *at_result_code_text(at_result_code_e code) {
  switch (code) {
    case AT_Result_Code__cme_error:
      return c_cme_error;
    case AT_Result_Code__failure:
      return "failure";
    case AT_Result_Code__timeout:
      return "timeout";
    case AT_Result_Code__unknown:
      return "unknown";
    default:
      if (code <= AT_Result_Code__NO_ANSWER)
        return at_result_codes[code].value.s;
      else
        return "unknown-hayes";
  }
}



struct {
  char *value;
  at_cfun_fun_e code;
} at_cfun_funs[] = {
    {.value = "minimum functionality", .code = AT_CFUN__FUN__Minimum_Functionality},
    {.value = "full functionality", .code = AT_CFUN__FUN__Full_Functionality},
    {.value = "airplane mode", .code = AT_CFUN__FUN__Airplane_Mode},
    {.value = "enable SIM-toolkit interface - SIM-APPL", .code = AT_CFUN__FUN__Enable_SIM_Toolkit_Interface},
    {.value = "disable SIM-toolkit interface - SIM-APPL (7)", .code = AT_CFUN__FUN__Disable_SIM_Toolkit_Interface_7},
    {.value = "disable SIM-toolkit interface - SIM-APPL (8)", .code = AT_CFUN__FUN__Disable_SIM_Toolkit_Interface_8},
    {.value = "enable SIM-toolkit interface - raw", .code = AT_CFUN__FUN__Enable_SIM_Toolkit_Interface_Raw_Mode},
    {.value = "silent modem reset, no SIM reset", .code = AT_CFUN__FUN__Modem_Silent_Reset__No_SIM_Reset},
    {.value = "silent modem reset, with SIM reset", .code = AT_CFUN__FUN__Modem_Silent_Reset__With_SIM_Reset},
    {.value = "minimum functionality, CS/PS/SIM deactivation",
     .code  = AT_CFUN__FUN__Minimum_Functionality_with_CS_PS_and_SIM_Deactivated},
    {.value = "modem deep low power mode", .code = AT_CFUN__FUN__Modem_Deep_Low_Power_Mode},
    {.value = 0, .code = (at_cfun_fun_e)-1},
};

char *at_cfun_fun_text(at_cfun_fun_e code) {
  int i;
  for (i = 0; at_cfun_funs[i].value != 0; i++)
    if (at_cfun_funs[i].code == code) return at_cfun_funs[i].value;
  return "<unknown-cfun-fun>";
}

struct {
  char *value;
  at_cfun_rst_e code;
} at_cfun_rsts[] = {
    {.value = "no modem/SIM reset", .code = AT_CFUN__RST__No_Modem_Reset},
    {.value = "modem/SIM silent reset", .code = AT_CFUN__RST__Modem_and_SIM_Silent_Reset},
    {.value = 0, .code = (at_cfun_rst_e)-1},
};

char *at_cfun_rst_text(at_cfun_rst_e code) {
  int i;
  for (i = 0; at_cfun_rsts[i].value != 0; i++)
    if (at_cfun_rsts[i].code == code) return at_cfun_rsts[i].value;
  return "<unknown-cfun-rst>";
}

struct {
  char *value;
  at_cfun_power_mode_e code;
} at_cfun_power_modes[] = {
    {.value = "minimum functionality", .code = AT_CFUN__POWER_MODE__Minimum_Functionality},
    {.value = "full functionality", .code = AT_CFUN__POWER_MODE__Full_Functionality},
    {.value = "airplane mode", .code = AT_CFUN__POWER_MODE__Airplane_Mode},
    {.value = "minimum functionality, CS/PS/SIM deactivated",
     .code  = AT_CFUN__POWER_MODE__Minimum_Functionality_with_CS_PS_and_SIM_Deactivated},
    {.value = 0, .code = (at_cfun_power_mode_e)-1},
};

char *at_cfun_power_mode_text(at_cfun_power_mode_e code) {
  int i;
  for (i = 0; at_cfun_power_modes[i].value != 0; i++)
    if (at_cfun_power_modes[i].code == code) return at_cfun_power_modes[i].value;
  return "<unknown-cfun-power-mode>";
}

struct {
  char *value;
  at_cfun_stk_mode_e code;
} at_cfun_stk_modes[] = {
    {.value = "Interface_Disabled_Proactive_SIM_APPL_Enabled_0",
     .code  = AT_CFUN__STK_MODE__Interface_Disabled_Proactive_SIM_APPL_Enabled_0},
    {.value = "Dedicated_Mode_Proactive_SIM_APPL_Enabled",
     .code  = AT_CFUN__STK_MODE__Dedicated_Mode_Proactive_SIM_APPL_Enabled},
    {.value = "Interface_Disabled_Proactive_SIM_APPL_Enabled_7",
     .code  = AT_CFUN__STK_MODE__Interface_Disabled_Proactive_SIM_APPL_Enabled_7},
    {.value = "Interface_Disabled_Proactive_SIM_APPL_Enabled_8",
     .code  = AT_CFUN__STK_MODE__Interface_Disabled_Proactive_SIM_APPL_Enabled_8},
    {.value = "Interface_Raw_Mode_Proactive_SIM_APPL_Enabled",
     .code  = AT_CFUN__STK_MODE__Interface_Raw_Mode_Proactive_SIM_APPL_Enabled},
    {.value = 0, .code = (at_cfun_stk_mode_e)-1},
};

char *at_cfun_stk_mode_text(at_cfun_stk_mode_e code) {
  int i;
  for (i = 0; at_cfun_stk_modes[i].value != 0; i++)
    if (at_cfun_stk_modes[i].code == code) return at_cfun_stk_modes[i].value;
  return "<unknown-cfun-stk-mode>";
}

struct {
  char *value;
  at_umnoprof_mno_profile_e code;
} at_umnoprof_mno_profiles[] = {
    {.value = "SW default", .code = AT_UMNOPROF__MNO_PROFILE__SW_Default},
    {.value = "SIM ICCID select", .code = AT_UMNOPROF__MNO_PROFILE__SIM_ICCID_Select},
    {.value = "AT&T", .code = AT_UMNOPROF__MNO_PROFILE__ATT},
    {.value = "Verizon", .code = AT_UMNOPROF__MNO_PROFILE__Verizon},
    {.value = "Telstra", .code = AT_UMNOPROF__MNO_PROFILE__Telstra},
    {.value = "T-Mobile", .code = AT_UMNOPROF__MNO_PROFILE__TMO},
    {.value = "CT", .code = AT_UMNOPROF__MNO_PROFILE__CT},
    {.value = 0, .code = (at_umnoprof_mno_profile_e)-1},
};

char *at_umnoprof_mno_profile_text(at_umnoprof_mno_profile_e code) {
  int i;
  for (i = 0; at_umnoprof_mno_profiles[i].value != 0; i++)
    if (at_umnoprof_mno_profiles[i].code == code) return at_umnoprof_mno_profiles[i].value;
  return "<unknown-umnoprof-mno-profile>";
}



struct {
  char *value;
  at_cops_mode_e code;
} at_cops_modes[] = {
    {.value = "automatic selection", .code = AT_COPS__Mode__Automatic_Selection},
    {.value = "manual selection", .code = AT_COPS__Mode__Manual_Selection},
    {.value = "deregister from network", .code = AT_COPS__Mode__Deregister_from_Network},
    {.value = "set only <format>", .code = AT_COPS__Mode__Set_Only_Format},
    {.value = "manual/automatic selection", .code = AT_COPS__Mode__Manual_Automatic},
    {.value = 0, .code = (at_cops_mode_e)-1},
};

char *at_cops_mode_text(at_cops_mode_e code) {
  int i;
  for (i = 0; at_cops_modes[i].value != 0; i++)
    if (at_cops_modes[i].code == code) return at_cops_modes[i].value;
  return "<unknown-cops-mode>";
}

struct {
  char *value;
  at_cops_format_e code;
} at_cops_formats[] = {
    {.value = "long alphanumeric", .code = AT_COPS__Format__Long_Alphanumeric},
    {.value = "short alphanumeric", .code = AT_COPS__Format__Short_Alphanumeric},
    {.value = "numeric", .code = AT_COPS__Format__Numeric},
    {.value = 0, .code = (at_cops_format_e)-1},
};

char *at_cops_format_text(at_cops_format_e code) {
  int i;
  for (i = 0; at_cops_formats[i].value != 0; i++)
    if (at_cops_formats[i].code == code) return at_cops_formats[i].value;
  return "<unknown-cops-format>";
}

struct {
  char *value;
  at_cops_stat_e code;
} at_cops_stats[] = {
    {.value = "unknown", .code = AT_COPS__Stat__Unknown},
    {.value = "available", .code = AT_COPS__Stat__Available},
    {.value = "current", .code = AT_COPS__Stat__Current},
    {.value = "forbidden", .code = AT_COPS__Stat__Forbidden},
    {.value = 0, .code = (at_cops_stat_e)-1},
};

char *at_cops_stat_text(at_cops_stat_e code) {
  int i;
  for (i = 0; at_cops_stats[i].value != 0; i++)
    if (at_cops_stats[i].code == code) return at_cops_stats[i].value;
  return "<unknown-cops-stat>";
}

struct {
  char *value;
  at_cops_act_e code;
} at_cops_acts[] = {
    {.value = "LTE", .code = AT_COPS__Access_Technology__LTE},
    {.value = "EC-GSM-IoT or LTE-Cat-M1", .code = AT_COPS__Access_Technology__EC_GSM_IoT},
    {.value = "LTE NB-S1", .code = AT_COPS__Access_Technology__LTE_NB_S1},
    {.value = 0, .code = (at_cops_act_e)-1},
};

char *at_cops_act_text(at_cops_act_e code) {
  int i;
  for (i = 0; at_cops_acts[i].value != 0; i++)
    if (at_cops_acts[i].code == code) return at_cops_acts[i].value;
  return "<unknown-cops-act>";
}



struct {
  char *value;
  at_creg_n_e code;
} at_creg_ns[] = {
    {.value = "URC disabled", .code = AT_CREG__N__URC_Disabled},
    {.value = "URC for Network Registration", .code = AT_CREG__N__Network_Registration_URC},
    {.value = "URC for Network Registration and Location Information",
     .code  = AT_CREG__N__Network_Registration_and_Location_Information_URC},

    {.value = 0, .code = (at_creg_n_e)-1},
};

char *at_creg_n_text(at_creg_n_e code) {
  int i;
  for (i = 0; at_creg_ns[i].value != 0; i++)
    if (at_creg_ns[i].code == code) return at_creg_ns[i].value;
  return "<unknown-creg-n>";
}



struct {
  char *value;
  at_creg_stat_e code;
} at_creg_stats[] = {
    {.value = "not-registered, not-searching", .code = AT_CREG__Stat__Not_Registered},
    {.value = "registered, home network", .code = AT_CREG__Stat__Registered_Home_Network},
    {.value = "not-registered, but searching", .code = AT_CREG__Stat__Not_Registered_but_Searching},
    {.value = "registration denied", .code = AT_CREG__Stat__Registration_Denied},
    {.value = "unknown", .code = AT_CREG__Stat__Unknown},
    {.value = "registered, roaming", .code = AT_CREG__Stat__Registered_Roaming},
    {.value = "registered, for SMS only, home network", .code = AT_CREG__Stat__Registered_for_SMS_Only_Home_Network},
    {.value = "registered, for SMS only, roaming", .code = AT_CREG__Stat__Registered_for_SMS_Only_Roaming},
    {.value = "registered, for CSFB-not-preferred, home network,",
     .code  = AT_CREG__Stat__Registered_for_CSFB_Only_Home_Network},
    {.value = "registered, for CSFB-not-preferred, roaming", .code = AT_CREG__Stat__Registered_for_CSFB_Only_Roaming},

    {.value = 0, .code = (at_creg_stat_e)-1},
};

char *at_creg_stat_text(at_creg_stat_e code) {
  int i;
  for (i = 0; at_creg_stats[i].value != 0; i++)
    if (at_creg_stats[i].code == code) return at_creg_stats[i].value;
  return "<unknown-creg-stat>";
}



struct {
  char *value;
  at_creg_act_e code;
} at_creg_acts[] = {
    {.value = "2G/GSM", .code = AT_CREG__AcT__GSM},
    {.value = "GSM-Compact", .code = AT_CREG__AcT__GSM_Compact},
    {.value = "3G/UTRAN", .code = AT_CREG__AcT__UTRAN},
    {.value = "2.5G/GSM+EDGE", .code = AT_CREG__AcT__GSM_with_EDGE_Availability},
    {.value = "3.5G/UTRAN+HSDPA", .code = AT_CREG__AcT__UTRAN_with_HSDPA_Availability},
    {.value = "3.5G/UTRAN+HSUPA", .code = AT_CREG__AcT__UTRAN_with_HSUPA_Availability},
    {.value = "3.5G/UTRAN+HSDPA+HSUPA", .code = AT_CREG__AcT__UTRAN_with_HSDPA_and_HSUPA_Availability},
    {.value = "4G/LTE", .code = AT_CREG__AcT__E_UTRAN},
    {.value = "2G/Extended-Coverage GSM for IoT", .code = AT_CREG__AcT__EC_GSM_IoT},
    {.value = "LTE/NB-S1", .code = AT_CREG__AcT__E_UTRAN_NB},
    {.value = "invalid", .code = AT_CREG__AcT__invalid},

    {.value = 0, .code = (at_creg_act_e)-1},
};

char *at_creg_act_text(at_creg_act_e code) {
  int i;
  for (i = 0; at_creg_acts[i].value != 0; i++)
    if (at_creg_acts[i].code == code) return at_creg_acts[i].value;
  return "<unknown-creg-act>";
}



struct {
  char *value;
  at_cgreg_n_e code;
} at_cgreg_ns[] = {
    {.value = "URC disabled", .code = AT_CGREG__N__URC_Disabled},
    {.value = "URC for Network Registration", .code = AT_CGREG__N__Network_Registration_URC},
    {.value = "URC for Network Registration and Location Information",
     .code  = AT_CGREG__N__Network_Registration_and_Location_Information_URC},

    {.value = 0, .code = (at_cgreg_n_e)-1},
};

char *at_cgreg_n_text(at_cgreg_n_e code) {
  int i;
  for (i = 0; at_cgreg_ns[i].value != 0; i++)
    if (at_cgreg_ns[i].code == code) return at_cgreg_ns[i].value;
  return "<unknown-cgreg-n>";
}



struct {
  char *value;
  at_cgreg_stat_e code;
} at_cgreg_stats[] = {
    {.value = "not-registered, not-searching", .code = AT_CGREG__Stat__Not_Registered},
    {.value = "registered, home network", .code = AT_CGREG__Stat__Registered_Home_Network},
    {.value = "not-registered, but searching", .code = AT_CGREG__Stat__Not_Registered_but_Searching},
    {.value = "registration denied", .code = AT_CGREG__Stat__Registration_Denied},
    {.value = "unknown", .code = AT_CGREG__Stat__Unknown},
    {.value = "registered, roaming", .code = AT_CGREG__Stat__Registered_Roaming},
    {.value = "attached for emergency bearer services only",
     .code  = AT_CGREG__Stat__Attached_for_Emergency_Bearer_Services_Only},

    {.value = 0, .code = (at_cgreg_stat_e)-1},
};

char *at_cgreg_stat_text(at_cgreg_stat_e code) {
  int i;
  for (i = 0; at_cgreg_stats[i].value != 0; i++)
    if (at_cgreg_stats[i].code == code) return at_cgreg_stats[i].value;
  return "<unknown-cgreg-stat>";
}



struct {
  char *value;
  at_cgreg_act_e code;
} at_cgreg_acts[] = {
    {.value = "2G/GSM", .code = AT_CGREG__AcT__GSM},
    {.value = "GSM-Compact", .code = AT_CGREG__AcT__GSM_Compact},
    {.value = "3G/UTRAN", .code = AT_CGREG__AcT__UTRAN},
    {.value = "2.5G/GSM+EDGE", .code = AT_CGREG__AcT__GSM_with_EDGE_Availability},
    {.value = "3.5G/UTRAN+HSDPA", .code = AT_CGREG__AcT__UTRAN_with_HSDPA_Availability},
    {.value = "3.5G/UTRAN+HSUPA", .code = AT_CGREG__AcT__UTRAN_with_HSUPA_Availability},
    {.value = "3.5G/UTRAN+HSDPA+HSUPA", .code = AT_CGREG__AcT__UTRAN_with_HSDPA_and_HSUPA_Availability},
    {.value = "invalid", .code = AT_CGREG__AcT__invalid},

    {.value = 0, .code = (at_cgreg_act_e)-1},
};

char *at_cgreg_act_text(at_cgreg_act_e code) {
  int i;
  for (i = 0; at_cgreg_acts[i].value != 0; i++)
    if (at_cgreg_acts[i].code == code) return at_cgreg_acts[i].value;
  return "<unknown-cgreg-act>";
}



struct {
  char *value;
  at_cereg_n_e code;
} at_cereg_ns[] = {
    {.value = "URC disabled", .code = AT_CEREG__N__URC_Disabled},
    {.value = "URC for Network Registration", .code = AT_CEREG__N__Network_Registration_URC},
    {.value = "URC for Network Registration and Location Information",
     .code  = AT_CEREG__N__Network_Registration_and_Location_Information_URC},
    {.value = "URC for Network Registration, Location Information and EMM",
     .code  = AT_CEREG__N__Network_Registration_Location_Information_and_EMM_URC},
    {.value = "URC for PSM, Network Registration and Location Information",
     .code  = AT_CEREG__N__PSM_Network_Registration_and_Location_Information_URC},
    {.value = "URC for PSM, Network Registration, Location Information and EMM",
     .code  = AT_CEREG__N__PSM_Network_Registration_Location_Information_and_EMM_URC},

    {.value = 0, .code = (at_cereg_n_e)-1},
};

char *at_cereg_n_text(at_cereg_n_e code) {
  int i;
  for (i = 0; at_cereg_ns[i].value != 0; i++)
    if (at_cereg_ns[i].code == code) return at_cereg_ns[i].value;
  return "<unknown-cereg-n>";
}



struct {
  char *value;
  at_cereg_stat_e code;
} at_cereg_stats[] = {
    {.value = "not-registered, not-searching", .code = AT_CEREG__Stat__Not_Registered},
    {.value = "registered, home network", .code = AT_CEREG__Stat__Registered_Home_Network},
    {.value = "not-registered, but searching", .code = AT_CEREG__Stat__Not_Registered_but_Searching},
    {.value = "registration denied", .code = AT_CEREG__Stat__Registration_Denied},
    {.value = "unknown", .code = AT_CEREG__Stat__Unknown},
    {.value = "registered, roaming", .code = AT_CEREG__Stat__Registered_Roaming},
    {.value = "attached for emergency bearer services only",
     .code  = AT_CEREG__Stat__Attached_for_Emergency_Bearer_Services_Only},

    {.value = 0, .code = (at_cereg_stat_e)-1},
};

char *at_cereg_stat_text(at_cereg_stat_e code) {
  int i;
  for (i = 0; at_cereg_stats[i].value != 0; i++)
    if (at_cereg_stats[i].code == code) return at_cereg_stats[i].value;
  return "<unknown-cereg-stat>";
}



struct {
  char *value;
  at_cereg_act_e code;
} at_cereg_acts[] = {
    {.value = "4G/LTE", .code = AT_CEREG__AcT__E_UTRAN},
    {.value = "2G/Extended-Coverage GSM for IoT", .code = AT_CEREG__AcT__EC_GSM_IoT},
    {.value = "LTE/NB-S1", .code = AT_CEREG__AcT__E_UTRAN_NB},
    {.value = "invalid", .code = AT_CEREG__AcT__invalid},

    {.value = 0, .code = (at_cereg_act_e)-1},
};

char *at_cereg_act_text(at_cereg_act_e code) {
  int i;
  for (i = 0; at_cereg_acts[i].value != 0; i++)
    if (at_cereg_acts[i].code == code) return at_cereg_acts[i].value;
  return "<unknown-cereg-act>";
}



struct {
  char *value;
  at_cereg_cause_type_e code;
} at_cereg_cause_types[] = {
    {.value = "EMM-Cause", .code = AT_CEREG__Cause_Type__EMM_Cause},
    {.value = "Manufacturer-Specific Cause", .code = AT_CEREG__Cause_Type__Manufacturer_Specific_Cause},

    {.value = 0, .code = (at_cereg_cause_type_e)-1},
};

char *at_cereg_cause_type_text(at_cereg_cause_type_e code) {
  int i;
  for (i = 0; at_cereg_cause_types[i].value != 0; i++)
    if (at_cereg_cause_types[i].code == code) return at_cereg_cause_types[i].value;
  return "<unknown-cereg-cause_type>";
}



struct {
  char *value;
  at_uso_protocol_e code;
} at_uso_protocols[] = {
    {.value = "<none>", .code = AT_USO_Protocol__none},
    {.value = "TCP", .code = AT_USO_Protocol__TCP},
    {.value = "UDP", .code = AT_USO_Protocol__UDP},

    {.value = 0, .code = (at_uso_protocol_e)-1},
};

char *at_uso_protocol_text(at_uso_protocol_e code) {
  int i;
  for (i = 0; at_uso_protocols[i].value != 0; i++)
    if (at_uso_protocols[i].code == code) return at_uso_protocols[i].value;
  return "<unknown-uso-protocol>";
}



struct {
  char *value;
  at_uso_error_e code;
} at_uso_error_types[] = {
    {.value = "No Error", .code = AT_USO_Error__Success},
    {.value = "Operation not permitted (internal error)", .code = AT_USO_Error__EPERM},
    {.value = "No such resource (internal error)", .code = AT_USO_Error__ENOENT},
    {.value = "Interrupted system call (internal error)", .code = AT_USO_Error__EINTR},
    {.value = "I/O error (internal error)", .code = AT_USO_Error__EIO},
    {.value = "Bad file descriptor (internal error)", .code = AT_USO_Error__EBADF},
    {.value = "No child processes (internal error)", .code = AT_USO_Error__ECHILD},
    {.value = "Current operation would block, try again", .code = AT_USO_Error__EWOULDBLOCK_EAGAIN},
    {.value = "Out of memory (internal error)", .code = AT_USO_Error__ENOMEM},
    {.value = "Bad address (internal error)", .code = AT_USO_Error__EFAULT},
    {.value = "Invalid argument", .code = AT_USO_Error__EINVAL},
    {.value = "Broken pipe (internal error)", .code = AT_USO_Error__EPIPE},
    {.value = "Function not implemented", .code = AT_USO_Error__ENOSYS},
    {.value = "Machine is not on the internet", .code = AT_USO_Error__ENONET},
    {.value = "End of file", .code = AT_USO_Error__EEOF},
    {.value = "Protocol error", .code = AT_USO_Error__EPROTO},
    {.value = "File descriptor in bad state (internal error)", .code = AT_USO_Error__EBADFD},
    {.value = "Remote address changed", .code = AT_USO_Error__EREMCHG},
    {.value = "Destination address required", .code = AT_USO_Error__EDESTADDRREQ},
    {.value = "Wrong protocol type for socket", .code = AT_USO_Error__EPROTOTYPE},
    {.value = "Protocol not available", .code = AT_USO_Error__ENOPROTOOPT},
    {.value = "Protocol not supported", .code = AT_USO_Error__EPROTONOSUPPORT},
    {.value = "Socket type not supported", .code = AT_USO_Error__ESOCKTNNOSUPPORT},
    {.value = "Operation not supported on transport endpoint", .code = AT_USO_Error__EOPNOTSUPP},
    {.value = "Protocol family not supported", .code = AT_USO_Error__EPFNOSUPPORT},
    {.value = "Address family not supported by protocol", .code = AT_USO_Error__EAFNOSUPPORT},
    {.value = "Address already in use", .code = AT_USO_Error__EADDRINUSE},
    {.value = "Cannot assign requested address", .code = AT_USO_Error__EADDRNOTAVAIL},
    {.value = "Network is down", .code = AT_USO_Error__ENETDOWN},
    {.value = "Network is unreachable", .code = AT_USO_Error__ENETUNREACH},
    {.value = "Network dropped connection because of reset", .code = AT_USO_Error__ENETRESET},
    {.value = "Software caused connection abort", .code = AT_USO_Error__ECONNABORTED},
    {.value = "Connection reset by peer", .code = AT_USO_Error__ECONNRESET},
    {.value = "No buffer space available", .code = AT_USO_Error__ENOBUFS},
    {.value = "Transport endpoint is already connected", .code = AT_USO_Error__EISCONN},
    {.value = "Transport endpoint is not connected", .code = AT_USO_Error__ENOTCONN},
    {.value = "Cannot send after transport endpoint  shutdown", .code = AT_USO_Error__ESHUTDOWN},
    {.value = "Connection timed out", .code = AT_USO_Error__ETIMEDOUT},
    {.value = "Connection refused", .code = AT_USO_Error__ECONNREFUSED},
    {.value = "Host is down", .code = AT_USO_Error__EHOSTDOWN},
    {.value = "No route to host", .code = AT_USO_Error__EHOSTUNREACH},
    {.value = "Operation now in progress", .code = AT_USO_Error__EINPROGRESS},
    {.value = "DNS server returned answer with no data", .code = AT_USO_Error__ENSRNODATA},
    {.value = "DNS server claims query was misformatted", .code = AT_USO_Error__ENSRFORMERR},
    {.value = "DNS server returned general failure", .code = AT_USO_Error__ENSRSERVFAIL},
    {.value = "Domain name not found", .code = AT_USO_Error__ENSRNOTFOUND},
    {.value = "DNS server does not implement requested operation", .code = AT_USO_Error__ENSRNOTIMP},
    {.value = "DNS server refused query", .code = AT_USO_Error__ENSRREFUSED},
    {.value = "Misformatted DNS query", .code = AT_USO_Error__ENSRBADQUERY},
    {.value = "Misformatted domain name", .code = AT_USO_Error__ENSRBADNAME},
    {.value = "Unsupported address family", .code = AT_USO_Error__ENSRBADFAMILY},
    {.value = "Misformatted DNS reply", .code = AT_USO_Error__ENSRBADRESP},
    {.value = "Could not contact DNS servers", .code = AT_USO_Error__ENSRCONNREFUSED},
    {.value = "Timeout while contacting DNS servers", .code = AT_USO_Error__ENSRTIMEOUT},
    {.value = "End of file", .code = AT_USO_Error__ENSROF},
    {.value = "Error reading file", .code = AT_USO_Error__ENSRFILE},
    {.value = "Out of memory", .code = AT_USO_Error__ENSRNOMEM},
    {.value = "Application terminated lookup", .code = AT_USO_Error__ENSRDESTRUCTION},
    {.value = "Domain name is too long", .code = AT_USO_Error__ENSRQUERYDOMAINTOOLONG},
    {.value = "Domain name is too long", .code = AT_USO_Error__ENSRCNAMELOOP},

    {.value = 0, .code = (at_uso_error_e)-1},

};

char *at_uso_error_text(at_uso_error_e code) {
  int i;
  for (i = 0; at_uso_error_types[i].value != 0; i++)
    if (at_uso_error_types[i].code == code) return at_uso_error_types[i].value;
  return "<unknown-uso-error>";
}
