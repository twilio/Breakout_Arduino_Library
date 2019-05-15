/*
 * OwlModemCLI.cpp
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
 * \file OwlModemCLI.cpp - A CLI interface for human interaction with the OwlModem API
 */

#include "OwlModemCLI.h"

#include <stdarg.h>
#include <stdio.h>

#include "../CoAP/CoAPPeer.h"
#include "../DTLS/OwlDTLSClient.h"
#include "../Breakout.h"



static str response = {.s = nullptr, .len = 0};



OwlModemCLIExecutor::OwlModemCLIExecutor(char *c_name, char *c_help_params, char *c_help, int min, int max)
    : minParams(min), maxParams(max) {
  name.s             = c_name;
  name.len           = strlen(c_name);
  helpParameters.s   = c_help_params;
  helpParameters.len = strlen(c_help_params);
  help.s             = c_help;
  help.len           = strlen(c_help);
}

OwlModemCLIExecutor::OwlModemCLIExecutor(char *const c_name, char *const c_help)
    : helpParameters({0}), minParams(0), maxParams(0) {
  name.s   = c_name;
  name.len = strlen(c_name);
  help.s   = c_help;
  help.len = strlen(c_help);
}

OwlModemCLIExecutor::~OwlModemCLIExecutor() {
}

int OwlModemCLIExecutor::checkParams(const OwlModemCLICommand &command) {
  if (command.argc < minParams || command.argc > maxParams) return 0;
  return 1;
}

void OwlModemCLIExecutor::executor(OwlModemCLI &cli, OwlModemCLICommand &command) {
  LOGF(L_CLI, "Not implemented for %.*s\r\n", name.len, name.s);
}

OwlModemCLI *OwlModemCLIExecutor::savedCLI = 0;



class SetDebugLevel : public OwlModemCLIExecutor {
 public:
  SetDebugLevel()
      : OwlModemCLIExecutor("setDebugLevel", "<debug_level>",
                            "Set the OwlModem debug level: 6 (most verbose) -> -4 (least verbose)", 1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    int level = str_to_long_int(cmd.argv[0], 10);
    owl_log_set_level(level);
    LOGF(L_CLI, "OK level=%d\r\n", level);
  }
};



class SoftReset : public OwlModemCLIExecutor {
 public:
  SoftReset() : OwlModemCLIExecutor("softReset", "Try do to a software reset of the MCU") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    //    TODO - find a way to do this...
  }
};



class RawBypass : public OwlModemCLIExecutor {
 public:
  RawBypass() : OwlModemCLIExecutor("bypass", "Go into raw bypass mode with the modem - type exitbypass to quit") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    cli.owlModem->bypassCLI();
  }
};



class RawGNSSBypass : public OwlModemCLIExecutor {
 public:
  RawGNSSBypass()
      : OwlModemCLIExecutor("bypassGNSS", "Go into raw bypass mode with teh GNSS - type exitbypass to quit") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    cli.owlModem->bypassGNSSCLI();
  }
};



class PowerOn : public OwlModemCLIExecutor {
 public:
  PowerOn()
      : OwlModemCLIExecutor("powerOn", "<module_bitmask>", "Power on modules - 1 modem, 2 Grove, 4 RGBLED, 8 GNSS\r\n",
                            1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    owl_power_m bit_mask = (owl_power_m)0;
    bit_mask             = (owl_power_m)str_to_uint32_t(cmd.argv[0], 10);
    cli.owlModem->powerOn(bit_mask);
  }
};



class PowerOff : public OwlModemCLIExecutor {
 public:
  PowerOff()
      : OwlModemCLIExecutor("powerOff", "<module_bitmask>", "Power on modules - 1 modem, 2 Grove, 4 RGBLED, 8 GNSS\r\n",
                            1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    owl_power_m bit_mask = (owl_power_m)0;
    bit_mask             = (owl_power_m)str_to_uint32_t(cmd.argv[0], 10);
    cli.owlModem->powerOff(bit_mask);
  }
};


class GetProductIdentification : public OwlModemCLIExecutor {
 public:
  GetProductIdentification()
      : OwlModemCLIExecutor("information.getProductIdentification", "Retrieve modem product identification (aka ATI)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->information.getProductIdentification(&response)) {
      LOGF(L_CLI, "OK product_identification=[%.*s]\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR status=[%.*s]\r\n", response.len, response.s);
    }
  }
};

class GetManufacturer : public OwlModemCLIExecutor {
 public:
  GetManufacturer() : OwlModemCLIExecutor("information.getManufacturer", "Retrieve modem manufacturer (aka AT+CGMI)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->information.getManufacturer(&response)) {
      LOGF(L_CLI, "OK manufacturer=[%.*s]\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR status=[%.*s]\r\n", response.len, response.s);
    }
  }
};

class GetModel : public OwlModemCLIExecutor {
 public:
  GetModel() : OwlModemCLIExecutor("information.getModel", "Retrieve modem model (aka AT+CGMM)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->information.getModel(&response)) {
      LOGF(L_CLI, "OK model=[%.*s]\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR status=[%.*s]\r\n", response.len, response.s);
    }
  }
};

class GetVersion : public OwlModemCLIExecutor {
 public:
  GetVersion() : OwlModemCLIExecutor("information.getVersion", "Retrieve modem version (aka AT+CGMR)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->information.getVersion(&response)) {
      LOGF(L_CLI, "OK version=[%.*s]\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR status=[%.*s]\r\n", response.len, response.s);
    }
  }
};

class GetIMEI : public OwlModemCLIExecutor {
 public:
  GetIMEI() : OwlModemCLIExecutor("information.getIMEI", "Retrieve IMSI (modem's \"MAC\" address - aka AT+CGSN)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->information.getIMEI(&response)) {
      LOGF(L_CLI, "OK IMEI=[%.*s]\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR status=[%.*s]\r\n", response.len, response.s);
    }
  }
};

class GetBatteryChargeLevels : public OwlModemCLIExecutor {
 public:
  GetBatteryChargeLevels()
      : OwlModemCLIExecutor("information.getBatteryChargeLevels", "Retrieve Battery Charge Levels (aka AT+CBC)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->information.getBatteryChargeLevels(&response)) {
      LOGF(L_CLI, "OK BatteryChargeLevels=[%.*s]\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR status=[%.*s]\r\n", response.len, response.s);
    }
  }
};

class GetIndicators : public OwlModemCLIExecutor {
 public:
  GetIndicators() : OwlModemCLIExecutor("information.getIndicators", "Retrieve indicators (aka AT+CIND)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->information.getIndicators(&response)) {
      LOGF(L_CLI, "OK BatteryChargeLevels=[%.*s]\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR status=[%.*s]\r\n", response.len, response.s);
    }
  }
};

class GetIndicatorsHelp : public OwlModemCLIExecutor {
 public:
  GetIndicatorsHelp()
      : OwlModemCLIExecutor("information.getIndicatorsHelp", "Retrieve indicators help (aka AT+CIND=?)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->information.getIndicatorsHelp(&response)) {
      LOGF(L_CLI, "OK BatteryChargeLevels=[%.*s]\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR status=[%.*s]\r\n", response.len, response.s);
    }
  }
};



class GetICCID : public OwlModemCLIExecutor {
 public:
  GetICCID() : OwlModemCLIExecutor("SIM.getICCID", "Retrieve ICCID (aka AT+CCID?)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->SIM.getICCID(&response)) {
      LOGF(L_CLI, "OK ICCID=%.*s\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR status=[%.*s]\r\n", response.len, response.s);
    }
  }
};

class GetIMSI : public OwlModemCLIExecutor {
 public:
  GetIMSI() : OwlModemCLIExecutor("SIM.getIMSI", "Retrieve IMSI (aka AT+CIMI)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->SIM.getIMSI(&response)) {
      LOGF(L_CLI, "OK IMSI=%.*s\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR status=[%.*s]\r\n", response.len, response.s);
    }
  }
};

class GetMSISDN : public OwlModemCLIExecutor {
 public:
  GetMSISDN() : OwlModemCLIExecutor("SIM.getMSISDN", "Retrieve MSISDN (aka AT+CNUM)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->SIM.getMSISDN(&response)) {
      LOGF(L_CLI, "OK MSISDN=[%.*s]\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR status=[%.*s]\r\n", response.len, response.s);
    }
  }
};

class GetPINStatus : public OwlModemCLIExecutor {
 public:
  GetPINStatus() : OwlModemCLIExecutor("SIM.getPINStatus", "Check PIN status (aka AT+CPIN?)") {
  }

  static void handlerPIN(str cpin) {
    if (!savedCLI) return;
    LOGF(L_CLI, "Received CPIN [%.*s]\r\n", cpin.len, cpin.s);
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    cli.owlModem->SIM.setHandlerPIN(GetPINStatus::handlerPIN);
    GetPINStatus::savedCLI = &cli;
    if (cli.owlModem->SIM.getPINStatus()) {
      LOGF(L_CLI, "OK\r\n");
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class VerifyPIN : public OwlModemCLIExecutor {
 public:
  VerifyPIN() : OwlModemCLIExecutor("SIM.verifyPIN", "<pin>", "Verify PIN (aka AT+CPIN=<pin>)", 1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->SIM.verifyPIN(cmd.argv[0])) {
      LOGF(L_CLI, "OK\r\n");
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class VerifyPUK : public OwlModemCLIExecutor {
 public:
  VerifyPUK()
      : OwlModemCLIExecutor("SIM.verifyPUK", "<puk> <pin>", "Verify PUK and change PIN (aka AT+CPIN=<puk>,<pin>)", 2,
                            2) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->SIM.verifyPUK(cmd.argv[0], cmd.argv[1])) {
      LOGF(L_CLI, "OK\r\n");
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};



class GetModemFunctionality : public OwlModemCLIExecutor {
 public:
  GetModemFunctionality()
      : OwlModemCLIExecutor("network.getModemFunctionality", "Check current modem functionality mode (aka AT+CFUN?)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_cfun_power_mode_e power_mode;
    at_cfun_stk_mode_e stk_mode;
    if (cli.owlModem->network.getModemFunctionality(&power_mode, &stk_mode)) {
      LOGF(L_CLI, "OK power_mode=%d(%s) stk_mode=%d(%s)\r\n", power_mode, at_cfun_power_mode_text(power_mode), stk_mode,
           at_cfun_stk_mode_text(stk_mode));
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class SetModemFunctionality : public OwlModemCLIExecutor {
 public:
  SetModemFunctionality()
      : OwlModemCLIExecutor("network.setModemFunctionality", "<fun>",
                            " 0 for minimum, 1 for full functionality, 4 for airplane mode and more (aka AT+CFUN=x)", 1,
                            1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_cfun_fun_e fun = (at_cfun_fun_e)str_to_long_int(cmd.argv[0], 10);
    at_cfun_rst_e rst = AT_CFUN__RST__No_Modem_Reset;
    if (cmd.argc >= 2) rst = (at_cfun_rst_e)str_to_long_int(cmd.argv[1], 10);
    if (cli.owlModem->network.setModemFunctionality(fun, cmd.argc >= 2 ? &rst : 0)) {
      LOGF(L_CLI, "OK fun=%d(%s) rst=%d(%s)\r\n", fun, at_cfun_fun_text(fun), rst, at_cfun_rst_text(rst));
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};


class GetModemMNOProfile : public OwlModemCLIExecutor {
 public:
  GetModemMNOProfile()
      : OwlModemCLIExecutor("network.getModemMNOProfile",
                            "Check current modem MNO Profile selection (aka AT+UMNOPROF?)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_umnoprof_mno_profile_e profile;
    if (cli.owlModem->network.getModemMNOProfile(&profile)) {
      LOGF(L_CLI, "OK profile=%d(%s)\r\n", profile, at_umnoprof_mno_profile_text(profile));
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class SetModemMNOProfile : public OwlModemCLIExecutor {
 public:
  SetModemMNOProfile()
      : OwlModemCLIExecutor("network.setModemMNOProfile", "<profile>",
                            " 0 - SW Default; 1 - SIM ICCID select; 2 - AT&T; 3 - Verizon; 4 - Telstra; 5 - T-Mobile; "
                            "6 - CT (aka AT+UMNOPROF=x)",
                            1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_umnoprof_mno_profile_e profile = (at_umnoprof_mno_profile_e)str_to_long_int(cmd.argv[0], 10);
    if (cli.owlModem->network.setModemMNOProfile(profile)) {
      LOGF(L_CLI, "OK profile=%d(%s)\r\n", profile, at_umnoprof_mno_profile_text(profile));
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};


class GetOperatorSelection : public OwlModemCLIExecutor {
 public:
  GetOperatorSelection()
      : OwlModemCLIExecutor("network.getOperatorSelection", "Get current operator selection mode (aka AT+COPS?)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_cops_mode_e mode;
    at_cops_format_e format;
    char buf[64];
    str oper = {.s = buf, .len = 0};
    at_cops_act_e act;
    if (cli.owlModem->network.getOperatorSelection(&mode, &format, &oper, 64, &act)) {
      LOGF(L_CLI, "OK mode=%d(%s) format=%d(%s) oper=[%.*s] act=%d(%s)\r\n", mode, at_cops_mode_text(mode), format,
           at_cops_format_text(format), oper.len, oper.s, act, at_cops_act_text(act));
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class SetOperatorSelection : public OwlModemCLIExecutor {
 public:
  SetOperatorSelection()
      : OwlModemCLIExecutor("network.setOperatorSelection", "<mode>[ <format> <oper>[ <act>]]",
                            "Set operator selection (mode: 0 auto, 1 manual, 2 deregister, 3 set format, 4 manual / "
                            "automatic) (format: 0 long, 1 short, 2 numeric) (oper: CCCNN or CCCNNN) (act: 7 LTE, 8 "
                            "Cat-M1, 9 NB1) (aka AT+COPS=...)",
                            1, 4) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_cops_mode_e mode     = (at_cops_mode_e)str_to_long_int(cmd.argv[0], 10);
    at_cops_format_e format = AT_COPS__Format__Long_Alphanumeric;
    str oper                = {0};
    at_cops_act_e act       = AT_COPS__Access_Technology__LTE_NB_S1;
    if (cmd.argc >= 2) format = (at_cops_format_e)str_to_long_int(cmd.argv[1], 10);
    if (cmd.argc >= 3) oper = cmd.argv[2];
    if (cmd.argc >= 4) act = (at_cops_act_e)str_to_long_int(cmd.argv[3], 10);
    if (cli.owlModem->network.setOperatorSelection(mode, cmd.argc >= 2 ? &format : 0, cmd.argc >= 3 ? &oper : 0,
                                                   cmd.argc >= 4 ? &act : 0)) {
      LOGF(L_CLI, "OK mode=%d(%s) format=%d(%s) oper=[%.*s] act=%d(%.*s)\r\n", mode, at_cops_mode_text(mode), format,
           at_cops_format_text(format), oper.len, oper.s, act, at_cops_act_text(act));
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class GetOperatorList : public OwlModemCLIExecutor {
 public:
  GetOperatorList()
      : OwlModemCLIExecutor("network.getOperatorList", "Scan for operators - takes a long time! (aka AT+COPS=?)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (cli.owlModem->network.getOperatorList(&response)) {
      LOGF(L_CLI, "OK\r\n%.*s\r\n", response.len, response.s);
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};



class GetNetworkRegistrationStatus : public OwlModemCLIExecutor {
 public:
  GetNetworkRegistrationStatus() : OwlModemCLIExecutor("network.getNetworkRegistrationStatus", "(aka AT+CREG?)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_creg_n_e n;
    at_creg_stat_e stat;
    uint16_t lac;
    uint32_t ci;
    at_creg_act_e act;
    if (cli.owlModem->network.getNetworkRegistrationStatus(&n, &stat, &lac, &ci, &act)) {
      LOGF(L_CLI, "OK N=%d(%s) Status=%d(%s) LAC=0x%04x CI=0x%08x Act=%d(%s)\r\n", n, at_creg_n_text(n), stat,
           at_creg_stat_text(stat), lac, ci, act, at_creg_act_text(act));
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class SetNetworkRegistrationURC : public OwlModemCLIExecutor {
 public:
  SetNetworkRegistrationURC()
      : OwlModemCLIExecutor("network.setNetworkRegistrationURC", "<n>",
                            "0 for Disabled, 1 for Registration, 2 +Location URCs (aka AT+CREG=<n>)", 1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_creg_n_e n = (at_creg_n_e)str_to_long_int(cmd.argv[0], 10);
    if (cli.owlModem->network.setNetworkRegistrationURC(n))
      LOGF(L_CLI, "OK\r\n");
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class GetGPRSRegistrationStatus : public OwlModemCLIExecutor {
 public:
  GetGPRSRegistrationStatus() : OwlModemCLIExecutor("network.getGPRSRegistrationStatus", "(aka AT+CGREG?)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_cgreg_n_e n;
    at_cgreg_stat_e stat;
    uint16_t lac;
    uint32_t ci;
    at_cgreg_act_e act;
    uint8_t rac;
    if (cli.owlModem->network.getGPRSRegistrationStatus(&n, &stat, &lac, &ci, &act, &rac)) {
      LOGF(L_CLI, "OK N=%d(%s) Status=%d(%s) LAC=0x%04x CI=0x%08x Act=%d(%s) RAC=%02x\r\n", n, at_cgreg_n_text(n), stat,
           at_cgreg_stat_text(stat), lac, ci, act, at_cgreg_act_text(act), rac);
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class SetGPRSRegistrationURC : public OwlModemCLIExecutor {
 public:
  SetGPRSRegistrationURC()
      : OwlModemCLIExecutor("network.setGPRSRegistrationURC", "<n>",
                            "0 for Disabled, 1 for Registration, 2 +Location URCs (aka AT+CGREG=<n>)", 1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_cgreg_n_e n = (at_cgreg_n_e)str_to_long_int(cmd.argv[0], 10);
    if (cli.owlModem->network.setGPRSRegistrationURC(n))
      LOGF(L_CLI, "OK\r\n");
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class GetEPSRegistrationStatus : public OwlModemCLIExecutor {
 public:
  GetEPSRegistrationStatus() : OwlModemCLIExecutor("network.getEPSRegistrationStatus", "(aka AT+CEREG?)") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_cereg_n_e n;
    at_cereg_stat_e stat;
    uint16_t lac;
    uint32_t ci;
    at_cereg_act_e act;
    uint8_t rac;
    at_cereg_cause_type_e cause_type;
    uint32_t reject_cause;
    if (cli.owlModem->network.getEPSRegistrationStatus(&n, &stat, &lac, &ci, &act, &cause_type, &reject_cause)) {
      LOGF(L_CLI, "OK N=%d(%s) Status=%d(%s) LAC=0x%04x CI=0x%08x Act=%d(%s) Cause-Type=%d(%s) Reject-Cause=%u\r\n", n,
           at_cereg_n_text(n), stat, at_cereg_stat_text(stat), lac, ci, act, at_cereg_act_text(act), cause_type,
           at_cereg_cause_type_text(cause_type), reject_cause);
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class SetEPSRegistrationURC : public OwlModemCLIExecutor {
 public:
  SetEPSRegistrationURC()
      : OwlModemCLIExecutor(
            "network.setEPSRegistrationURC", "<n>",
            "0 for Disabled, 1 for Registration, 2 +Location URCs, 3 +EMM, 4 +PSM -EMM, 5 +PSM +EMM (aka AT+CEREG=<n>)",
            1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_cereg_n_e n = (at_cereg_n_e)str_to_long_int(cmd.argv[0], 10);
    if (cli.owlModem->network.setEPSRegistrationURC(n))
      LOGF(L_CLI, "OK\r\n");
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};



class GetSignalQuality : public OwlModemCLIExecutor {
 public:
  GetSignalQuality()
      : OwlModemCLIExecutor("network.getSignalQuality", "[n]",
                            "Retrieve RSSI/Quality. Optionally provide [n] as number of repetitions. Send any byte to "
                            "stop (aka AT+CSQ)",
                            0, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    int count = 1;
    at_csq_rssi_e rssi;
    at_csq_qual_e qual;
    if (cmd.argc >= 1) count = (int)str_to_long_int(cmd.argv[0], 10);
    do {
      if (!cli.owlModem->network.getSignalQuality(&rssi, &qual)) {
        LOGF(L_CLI, "ERROR\r\n");
        break;
      }
      LOGF(L_CLI, "RSSI=%3d Quality=%d\r\n", rssi, qual);
      count--;
      for (int i = 0; i < 10 && count > 0; i++) {
        if (cli.debugPort->available()) {
          count = 0;
          break;
        }
        delay(100);
      }
    } while (count > 0);
  }
};



class GetAPNIPAddress : public OwlModemCLIExecutor {
 public:
  GetAPNIPAddress()
      : OwlModemCLIExecutor(
            "pdn.getAPNIPAddress", "[<cid>]",
            "Retrieve the current local IP addresses, for a certain APN context id; <cid> defaults to 1", 0, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t cid = 1;
    if (cmd.argc >= 1) cid = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    uint8_t ipv4[4];
    uint8_t ipv6[16];
    if (cli.owlModem->pdn.getAPNIPAddress(cid, ipv4, ipv6)) {
      LOGF(L_CLI,
           "OK ipv4=%d.%d.%d.%d ipv6=%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n",
           ipv4[0], ipv4[1], ipv4[2], ipv4[3], ipv6[0], ipv6[1], ipv6[2], ipv6[3], ipv6[4], ipv6[5], ipv6[6], ipv6[7],
           ipv6[8], ipv6[9], ipv6[10], ipv6[11], ipv6[12], ipv6[13], ipv6[14], ipv6[15]);
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};



class OpenSocket : public OwlModemCLIExecutor {
 public:
  OpenSocket()
      : OwlModemCLIExecutor(
            "socket.open", "<protocol> [local_port]",
            "Open a local socket on UDP (protocol=17) or TCP (protocol=6) and optionally on a certain port", 1, 2) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_uso_protocol_e protocol = (at_uso_protocol_e)str_to_uint32_t(cmd.argv[0], 10);
    uint16_t local_port        = 0;
    uint8_t out_socket         = 0;
    if (cmd.argc >= 2) local_port = (int)str_to_long_int(cmd.argv[1], 10);
    if (cli.owlModem->socket.open(protocol, local_port, &out_socket))
      LOGF(L_CLI, "OK socket=%u\r\n", out_socket);
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class CloseSocket : public OwlModemCLIExecutor {
 public:
  CloseSocket() : OwlModemCLIExecutor("socket.close", "<socket>", "Close a local socket", 1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    if (cli.owlModem->socket.close(socket))
      LOGF(L_CLI, "OK\r\n");
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class GetSocketError : public OwlModemCLIExecutor {
 public:
  GetSocketError() : OwlModemCLIExecutor("socket.getError", "Retrieve the last error from a socket") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    at_uso_error_e out_error = AT_USO_Error__Success;
    if (cli.owlModem->socket.getError(&out_error))
      LOGF(L_CLI, "OK error=%d - %s\r\n", out_error, at_uso_error_text(out_error));
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class ConnectSocket : public OwlModemCLIExecutor {
 public:
  ConnectSocket()
      : OwlModemCLIExecutor("socket.connect", "<socket> <remote_ip> <remote_port>",
                            "Connect a UDP or TCP socket to a remote IP:port.", 3, 3) {
  }

  static void handlerSocketClosed(uint8_t socket) {
    if (!savedCLI) return;
    LOGF(L_CLI, "Closed socket event for socket=%d\r\n", socket);
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket          = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    str remote_ip           = cmd.argv[1];
    uint16_t remote_port    = (uint16_t)str_to_uint32_t(cmd.argv[2], 10);
    ConnectSocket::savedCLI = &cli;
    if (cli.owlModem->socket.connect(socket, remote_ip, remote_port, this->handlerSocketClosed))
      LOGF(L_CLI, "OK\r\n");
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class SendUDP : public OwlModemCLIExecutor {
 public:
  SendUDP()
      : OwlModemCLIExecutor("socket.sendUDP", "<socket> <hex_data>",
                            "Send data on a connected UDP socket. Data is in hex in here, but binary in the C++ API.",
                            2, 2) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    if (cmd.argv[1].len > 512) {
      LOGF(L_CLI, "ERROR data input too long %d > max 512 bytes\r\n", cmd.argv[1].len);
      return;
    }
    char buf[512];
    str data           = {.s = buf, .len = 0};
    data.len           = hex_to_str(data.s, 512, cmd.argv[1]);
    int out_bytes_sent = 0;
    if (cli.owlModem->socket.sendUDP(socket, data, &out_bytes_sent))
      LOGF(L_CLI, "OK sent_bytes=%d\r\n", out_bytes_sent);
    else
      LOGF(L_CLI, "ERROR sent_bytes=%d\r\n", out_bytes_sent);
  }
};

class SendTCP : public OwlModemCLIExecutor {
 public:
  SendTCP()
      : OwlModemCLIExecutor("socket.sendTCP", "<socket> <hex_data>",
                            "Send data on a connected TCP socket. Data is in hex in here, but binary in the C++ API.",
                            2, 2) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    if (cmd.argv[1].len > 512) {
      LOGF(L_CLI, "ERROR data input too long %d > max 512 bytes\r\n", cmd.argv[1].len);
      return;
    }
    char buf[512];
    str data           = {.s = buf, .len = 0};
    data.len           = hex_to_str(data.s, 512, cmd.argv[1]);
    int out_bytes_sent = 0;
    if (cli.owlModem->socket.sendTCP(socket, data, &out_bytes_sent))
      LOGF(L_CLI, "OK sent_bytes=%d\r\n", out_bytes_sent);
    else
      LOGF(L_CLI, "ERROR sent_bytes=%d\r\n", out_bytes_sent);
  }
};

class SendToUDP : public OwlModemCLIExecutor {
 public:
  SendToUDP()
      : OwlModemCLIExecutor("socket.sendToUDP", "<socket> <remote_ip> <remote_port> <hex_data>",
                            "Send UDP data to a remote IP:port. Data is in hex in here, but binary in the C++ API.", 4,
                            4) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket       = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    str remote_ip        = cmd.argv[1];
    uint16_t remote_port = (uint16_t)str_to_uint32_t(cmd.argv[2], 10);
    if (cmd.argv[3].len > 512) {
      LOGF(L_CLI, "ERROR data input too long %d > max 512 bytes\r\n", cmd.argv[3].len);
      return;
    }
    char buf[512];
    str data = {.s = buf, .len = 0};
    data.len = hex_to_str(data.s, 512, cmd.argv[3]);
    if (cli.owlModem->socket.sendToUDP(socket, remote_ip, remote_port, data))
      LOGF(L_CLI, "OK\r\n");
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class GetQueuedForReceive : public OwlModemCLIExecutor {
 public:
  GetQueuedForReceive()
      : OwlModemCLIExecutor("socket.getQueuedForReceive", "<socket>",
                            "Retrieve the current data lengths queued for receive on the local side", 1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    int out_receive_tcp;
    int out_receive_udp;
    int out_receivefrom_udp;
    if (cli.owlModem->socket.getQueuedForReceive(socket, &out_receive_tcp, &out_receive_udp, &out_receivefrom_udp)) {
      LOGF(L_CLI, "OK receive_tcp=%d receive_udp=%d receivefrom_udp=%d\r\n", out_receive_tcp, out_receive_udp,
           out_receivefrom_udp);
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class ReceiveUDP : public OwlModemCLIExecutor {
 public:
  ReceiveUDP()
      : OwlModemCLIExecutor(
            "socket.receiveUDP", "<socket> [<length>]",
            "Try to get UDP data buffered from a connected socket. If length not specified, auto-detect it", 1, 2) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    int len        = 0;
    if (cmd.argc >= 2) len = str_to_long_int(cmd.argv[1], 10);
    char buf_ip[64];
    if (cmd.argv[3].len > 512) {
      LOGF(L_CLI, "ERROR data input too long %d > max 512 bytes\r\n", cmd.argv[3].len);
      return;
    }
    char buf_bin[512];
    str data_bin = {.s = buf_bin, .len = 0};
    if (cli.owlModem->socket.receiveUDP(socket, len, &data_bin, 512)) {
      LOGF(L_CLI, "OK len=%d\r\n", data_bin.len);
      LOGSTR(L_CLI, data_bin);
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class ReceiveTCP : public OwlModemCLIExecutor {
 public:
  ReceiveTCP()
      : OwlModemCLIExecutor(
            "socket.receiveTCP", "<socket> [<length>]",
            "Try to get TCP data buffered from a connected socket. If length not specified, auto-detect it", 1, 2) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    int len        = 0;
    if (cmd.argc >= 2) len = str_to_long_int(cmd.argv[1], 10);
    char buf_ip[64];
    if (cmd.argv[3].len > 512) {
      LOGF(L_CLI, "ERROR data input too long %d > max 512 bytes\r\n", cmd.argv[3].len);
      return;
    }
    char buf_bin[512];
    str data_bin = {.s = buf_bin, .len = 0};
    if (cli.owlModem->socket.receiveTCP(socket, len, &data_bin, 512)) {
      LOGF(L_CLI, "OK len=%d\r\n", data_bin.len);
      LOGSTR(L_CLI, data_bin);
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class ReceiveFromUDP : public OwlModemCLIExecutor {
 public:
  ReceiveFromUDP()
      : OwlModemCLIExecutor("socket.receiveFromUDP", "<socket> [<length>]",
                            "Try to get UDP data buffered. If length not specified, auto-detect it.", 1, 2) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    int len        = 0;
    if (cmd.argc >= 2) len = str_to_long_int(cmd.argv[1], 10);
    char buf_ip[64];
    str remote_ip        = {.s = buf_ip, .len = 0};
    uint16_t remote_port = 0;
    if (cmd.argv[3].len > 512) {
      LOGF(L_CLI, "ERROR data input too long %d > max 512 bytes\r\n", cmd.argv[3].len);
      return;
    }
    char buf_bin[512];
    str data_bin = {.s = buf_bin, .len = 0};
    if (cli.owlModem->socket.receiveFromUDP(socket, len, &remote_ip, &remote_port, &data_bin, 512)) {
      LOGF(L_CLI, "OK remote_ip=%.*s remote_port=%u len=%d\r\n", remote_ip.len, remote_ip.s, remote_port, data_bin.len);
      LOGSTR(L_CLI, data_bin);
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class ListenUDP : public OwlModemCLIExecutor {
 public:
  ListenUDP()
      : OwlModemCLIExecutor("socket.listenUDP", "<socket> <local_port>",
                            "Sets the handler to the CLI test one and listens for incoming UDP packets.", 2, 2) {
  }

  static void handlerUDPData(uint8_t socket, str remote_ip, uint16_t remote_port, str data) {
    if (!savedCLI) return;
    LOGF(L_CLI, "Received UDP data from socket=%d remote_ip=%.*s remote_port=%u of %d bytes\r\n", socket, remote_ip.len,
         remote_ip.s, remote_port, data.len);
    LOGSTR(L_CLI, data);
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket      = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    uint16_t local_port = (uint16_t)str_to_uint32_t(cmd.argv[1], 10);
    ListenUDP::savedCLI = &cli;
    if (cli.owlModem->socket.listenUDP(socket, local_port, this->handlerUDPData))
      LOGF(L_CLI, "OK\r\n");
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class ListenTCP : public OwlModemCLIExecutor {
 public:
  ListenTCP()
      : OwlModemCLIExecutor("socket.listenTCP", "<socket> <local_port>",
                            "Sets the handler to the CLI test one and listens for incoming UDP packets.", 2, 2) {
  }

  static void handlerTCPData(uint8_t socket, str data) {
    if (!savedCLI) return;
    LOGF(L_CLI, "URC handlerTCPData socket=%d len=%d\r\n", socket, data.len);
    LOGSTR(L_CLI, data);
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket      = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    uint16_t local_port = (uint16_t)str_to_uint32_t(cmd.argv[1], 10);
    ListenTCP::savedCLI = &cli;
    if (cli.owlModem->socket.listenTCP(socket, local_port, this->handlerTCPData))
      LOGF(L_CLI, "OK\r\n");
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class AcceptTCP : public OwlModemCLIExecutor {
 public:
  AcceptTCP()
      : OwlModemCLIExecutor("socket.acceptTCP", "<socket> <local_port>",
                            "Sets the accept and data handlers to the CLI tests one and waits for incoming TCP "
                            "connections.",
                            2, 2) {
  }

  static void handlerTCPAccept(uint8_t new_socket, str remote_ip, uint16_t remote_port, uint8_t listening_socket,
                               str local_ip, uint16_t local_port) {
    if (!savedCLI) return;
    LOGF(L_CLI,
         "URC handlerTCPAccept listening_socket=%u local_ip=%.*s local_port=%u new_socket=%u remote_ip=%.*s "
         "remote_port=%u\r\n",
         listening_socket, local_ip.len, local_ip.s, local_port, new_socket, remote_ip.len, remote_ip.s, remote_port);
  }

  static void handlerSocketClosed(uint8_t socket) {
    if (!savedCLI) return;
    LOGF(L_CLI, "URC handlerSocketClosed socket=%d\r\n", socket);
  }

  static void handlerTCPData(uint8_t socket, str data) {
    if (!savedCLI) return;
    LOGF(L_CLI, "URC handlerTCPData socket=%d len=%d\r\n", socket, data.len);
    LOGSTR(L_CLI, data);
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint8_t socket      = (uint8_t)str_to_uint32_t(cmd.argv[0], 10);
    uint16_t local_port = (uint16_t)str_to_uint32_t(cmd.argv[1], 10);
    AcceptTCP::savedCLI = &cli;
    if (cli.owlModem->socket.acceptTCP(socket, local_port, this->handlerTCPAccept, this->handlerSocketClosed,
                                       this->handlerTCPData))
      LOGF(L_CLI, "OK\r\n");
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class OpenSocketListenUDP : public OwlModemCLIExecutor {
 public:
  OpenSocketListenUDP()
      : OwlModemCLIExecutor(
            "socket.openListenUDP", "<local_port>",
            "Opens UDP socket and sets the data handler to the CLI test one and listens for incoming UDP "
            "packets.",
            1, 1) {
  }

  static void handlerUDPData(uint8_t socket, str remote_ip, uint16_t remote_port, str data) {
    if (!savedCLI) return;
    LOGF(L_CLI, "URC handlerUDPData socket=%u remote_ip=%.*s remote_port=%u len=%d\r\n", socket, remote_ip.len,
         remote_ip.s, remote_port, data.len);
    LOGSTR(L_CLI, data);
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint16_t local_port           = (uint16_t)str_to_uint32_t(cmd.argv[0], 10);
    uint8_t socket                = 0;
    OpenSocketListenUDP::savedCLI = &cli;
    if (cli.owlModem->socket.openListenUDP(local_port, this->handlerUDPData, &socket))
      LOGF(L_CLI, "OK socket=%u\r\n", socket);
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class OpenSocketListenConnectUDP : public OwlModemCLIExecutor {
 public:
  OpenSocketListenConnectUDP()
      : OwlModemCLIExecutor("socket.openListenConnectUDP", "<local_port> <remote_ip> <remote_port>",
                            "Opens UDP socket, connects it to a remote ip and port and sets the handler to the CLI "
                            "test one and listens for incoming UDP packets.",
                            3, 3) {
  }

  static void handlerUDPData(uint8_t socket, str remote_ip, uint16_t remote_port, str data) {
    if (!savedCLI) return;
    LOGF(L_CLI, "URC handlerUDPData socket=%u len=%d\r\n", socket, data.len);
    LOGSTR(L_CLI, data);
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint16_t local_port                  = (uint16_t)str_to_uint32_t(cmd.argv[0], 10);
    str remote_ip                        = cmd.argv[1];
    uint16_t remote_port                 = (uint16_t)str_to_uint32_t(cmd.argv[2], 10);
    uint8_t socket                       = 0;
    OpenSocketListenConnectUDP::savedCLI = &cli;
    if (cli.owlModem->socket.openListenConnectUDP(local_port, remote_ip, remote_port, this->handlerUDPData, &socket))
      LOGF(L_CLI, "OK socket=%u\r\n", socket);
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class OpenSocketListenConnectTCP : public OwlModemCLIExecutor {
 public:
  OpenSocketListenConnectTCP()
      : OwlModemCLIExecutor("socket.openListenConnectTCP", "<local_port> <remote_ip> <remote_port>",
                            "Opens TCP socket in client mode, connects it to remote IP:port and listens for data and "
                            "the socket closed event.",
                            3, 3) {
  }

  static void handlerSocketClosed(uint8_t socket) {
    if (!savedCLI) return;
    LOGF(L_CLI, "URC handlerSocketClosed socket=%d\r\n", socket);
  }

  static void handlerTCPData(uint8_t socket, str data) {
    if (!savedCLI) return;
    LOGF(L_CLI, "URC handlerTCPData socket=%u len=%d\r\n", socket, data.len);
    LOGSTR(L_CLI, data);
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint16_t local_port                  = (uint16_t)str_to_uint32_t(cmd.argv[0], 10);
    str remote_ip                        = cmd.argv[1];
    uint16_t remote_port                 = (uint16_t)str_to_uint32_t(cmd.argv[2], 10);
    uint8_t socket                       = 0;
    OpenSocketListenConnectTCP::savedCLI = &cli;
    if (cli.owlModem->socket.openListenConnectTCP(local_port, remote_ip, remote_port, this->handlerSocketClosed,
                                                  this->handlerTCPData, &socket))
      LOGF(L_CLI, "OK socket=%u\r\n", socket);
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};

class OpenSocketAcceptTCP : public OwlModemCLIExecutor {
 public:
  OpenSocketAcceptTCP()
      : OwlModemCLIExecutor("socket.openAcceptTCP", "<local_port>",
                            "Opens TCP socket in server mode and sets the accept handler. Accepted sockets will "
                            "inherit from here the data and socket closed handlers.",
                            1, 1) {
  }

  static void handlerTCPAccept(uint8_t new_socket, str remote_ip, uint16_t remote_port, uint8_t listening_socket,
                               str local_ip, uint16_t local_port) {
    if (!savedCLI) return;
    LOG(L_NOTICE,
        "URC handlerTCPAccept listening_socket=%u local_ip=%.*s local_port=%u new_socket=%u "
        "remote_ip=%.*s remote_port=%u\r\n",
        listening_socket, local_ip.len, local_ip.s, local_port, new_socket, remote_ip.len, remote_ip.s, remote_port);
  }

  static void handlerSocketClosed(uint8_t socket) {
    if (!savedCLI) return;
    LOGF(L_CLI, "URC handlerSocketClosed socket=%d\r\n", socket);
  }

  static void handlerTCPData(uint8_t socket, str data) {
    if (!savedCLI) return;
    LOGF(L_CLI, "URC handlerTCPData socket=%u len=%d\r\n", socket, data.len);
    LOGSTR(L_CLI, data);
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    uint16_t local_port           = (uint16_t)str_to_uint32_t(cmd.argv[0], 10);
    str remote_ip                 = cmd.argv[1];
    uint16_t remote_port          = (uint16_t)str_to_uint32_t(cmd.argv[2], 10);
    uint8_t socket                = 0;
    OpenSocketAcceptTCP::savedCLI = &cli;
    if (cli.owlModem->socket.openAcceptTCP(local_port, this->handlerTCPAccept, this->handlerSocketClosed,
                                           this->handlerTCPData, &socket))
      LOGF(L_CLI, "OK socket=%u\r\n", socket);
    else
      LOGF(L_CLI, "ERROR\r\n");
  }
};



class GetGNSSData : public OwlModemCLIExecutor {
 public:
  GetGNSSData() : OwlModemCLIExecutor("gnss.getGNSSData", "Retrieve GNSS data and log it.") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    gnss_data_t data;
    if (cli.owlModem->gnss.getGNSSData(&data)) {
      LOGF(L_CLI, "OK\r\n");
      cli.owlModem->gnss.logGNSSData(L_CLI, data);
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};



static OwlDTLSClient *owlDTLSClient = 0;

class DTLSClientCreate : public OwlModemCLIExecutor {
 public:
  DTLSClientCreate()
      : OwlModemCLIExecutor(
            "dtls.client.create", "<remote_ip> <remote_port> [<local_port>[ <psk_id>[ <psk_key>]",
            "Open a DTLS client connection. If no PSK-Id/Key are provided, default \"username\"/\"password\" is used.",
            2, 5) {
  }

  static void handlerData(OwlDTLSClient *owlDTLSClient, session_t *session, str plaintext) {
    LOG(L_CLI, "DTLS Rx Data - decrypted %d bytes\r\n", plaintext.len);
    LOGSTR(L_CLI, plaintext);
  }

  static void handlerEvent(OwlDTLSClient *owlDTLSClient, session_t *session, dtls_alert_level_e level,
                           dtls_alert_description_e code) {
    LOG(L_CLI, "DTLS Event - level %d (%s) code %d (%s)\r\n", level, dtls_alert_level_text(level), code,
        dtls_alert_description_text(code));
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    str remote_ip        = cmd.argv[0];
    uint16_t remote_port = str_to_uint32_t(cmd.argv[1], 10);
    uint16_t local_port  = 0;
    if (cmd.argc >= 3) local_port = str_to_uint32_t(cmd.argv[2], 10);
    str psk_id = {0};
    if (cmd.argc >= 4) {
      psk_id = cmd.argv[3];
    } else {
      psk_id.s   = "username";
      psk_id.len = 8;
    }
    str psk_key = {0};
    if (cmd.argc >= 5) {
      psk_key = cmd.argv[4];
    } else {
      psk_key.s   = "password";
      psk_key.len = 8;
    }
    if (owlDTLSClient) {
      LOGF(L_CLI, "ERROR - already created - call dtls.client.destroy before calling this again\r\n");
      return;
    }
    owlDTLSClient = owl_new OwlDTLSClient(psk_id, psk_key);
    if (!owlDTLSClient) {
      LOGF(L_CLI, "ERROR - internal\r\n");
      return;
    }
    if (!owlDTLSClient->init(cli.owlModem)) {
      LOG(L_CLI, "ERROR - internal\r\n");
      return;
    }
    owlDTLSClient->setDataHandler(DTLSClientCreate::handlerData);
    owlDTLSClient->setEventHandler(DTLSClientCreate::handlerEvent);
    if (!owlDTLSClient || !owlDTLSClient->connect(local_port, remote_ip, remote_port)) {
      LOGF(L_CLI, "ERROR\r\n");
    } else {
      LOGF(L_CLI, "OK - events will follow\r\n");
    }
  }
};

class DTLSClientGetStatus : public OwlModemCLIExecutor {
 public:
  DTLSClientGetStatus()
      : OwlModemCLIExecutor("dtls.client.getStatus", "Get the current status of the DTLS connection") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (!owlDTLSClient) {
      LOG(L_CLI, "ERROR - no DTLS client context created - do first dtls.client.create\r\n");
      return;
    }
    dtls_alert_description_e status = owlDTLSClient->getCurrentStatus();
    LOG(L_CLI, "OK status=%d status_text=%s\r\n", status, dtls_alert_description_text(status));
  }
};

class DTLSClientSend : public OwlModemCLIExecutor {
 public:
  DTLSClientSend()
      : OwlModemCLIExecutor("dtls.client.send", "<data-in-hex>",
                            "Send data over the last DTLS connection. Data is in hex here, but binary in the C++ API",
                            1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (!owlDTLSClient) {
      LOG(L_CLI, "ERROR - no DTLS client context created - do first dtls.client.create\r\n");
      return;
    }
    if (cmd.argv[0].len > 512) {
      LOGF(L_CLI, "ERROR data input too long %d > max 512 bytes\r\n", cmd.argv[0].len);
      return;
    }
    char buf[512];
    str data = {.s = buf, .len = 0};
    data.len = hex_to_str(data.s, 512, cmd.argv[0]);
    if (owlDTLSClient->sendData(data)) {
      LOGF(L_CLI, "OK\r\n");
    } else {
      LOGF(L_CLI, "ERROR\r\n");
    }
  }
};

class DTLSClientRenegotiate : public OwlModemCLIExecutor {
 public:
  DTLSClientRenegotiate() : OwlModemCLIExecutor("dtls.client.renegotiate", "Trigger a DTLS connection renegotiation") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (!owlDTLSClient) {
      LOG(L_CLI, "ERROR - no DTLS client context created - do first dtls.client.create\r\n");
      return;
    }
    if (!owlDTLSClient->renegotiate()) {
      LOG(L_CLI, "ERROR\r\n");
    } else {
      LOG(L_CLI, "OK\r\n");
    }
  }
};

class DTLSClientRehandshake : public OwlModemCLIExecutor {
 public:
  DTLSClientRehandshake() : OwlModemCLIExecutor("dtls.client.rehandshake", "Trigger a DTLS connection rehandshake") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (!owlDTLSClient) {
      LOG(L_CLI, "ERROR - no DTLS client context created - do first dtls.client.create\r\n");
      return;
    }
    if (!owlDTLSClient->rehandshake()) {
      LOG(L_CLI, "ERROR\r\n");
    } else {
      LOG(L_CLI, "OK\r\n");
    }
  }
};

class DTLSClientDestroy : public OwlModemCLIExecutor {
 public:
  DTLSClientDestroy() : OwlModemCLIExecutor("dtls.client.destroy", "Destroy the current DTLS context") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (!owlDTLSClient) {
      LOG(L_CLI, "ERROR - no DTLS client context created - do first dtls.client.create\r\n");
      return;
    }
    if (!owlDTLSClient->close()) {
      LOG(L_CLI, "ERROR\r\n");
    } else {
      LOG(L_CLI, "OK\r\n");
    }
    delete owlDTLSClient;
    owlDTLSClient = 0;
  }
};



static CoAPPeer *coapPeer = 0;

class CoAPPeerCreate : public OwlModemCLIExecutor {
 public:
  CoAPPeerCreate()
      : OwlModemCLIExecutor(
            "coap.create", "<remote_ip> [remote_port[ <local_port>[ <psk_id>[ <psk_key>]",
            "Open a CoAP connection. If PSK-Id/Key are provided then DTLS will be used, otherwise plaintext.", 1, 5) {
  }

  static void handlerStatelessMessage(CoAPPeer *peer, CoAPMessage *message) {
    if (!message) {
      LOG(L_CLI, "CoAP Rx Data - null ???\r\n");
    } else {
      LOG(L_CLI, "CoAP Rx Message:\r\n");
      message->log(L_CLI);
    }
  }

  static void handlerDTLSEvent(CoAPPeer *peer, dtls_alert_level_e level, dtls_alert_description_e code) {
    LOG(L_CLI, "CoAP Event - level %d (%s) code %d (%s)\r\n", level, dtls_alert_level_text(level), code,
        dtls_alert_description_text(code));
  }

  static coap_handler_follow_up_e handlerCoAPRequest(CoAPPeer *peer, CoAPMessage *message) {
    if (!message) {
      LOG(L_CLI, "CoAP Rx Request - null ???\r\n");
    } else {
      LOG(L_CLI, "CoAP Rx Request:\r\n");
      message->log(L_CLI);
    }
    return CoAP__Handler_Followup__Send_Reset;
  }

  static coap_handler_follow_up_e handlerCoAPResponse(CoAPPeer *peer, CoAPMessage *message) {
    if (!message) {
      LOG(L_CLI, "CoAP Rx Response - null ???\r\n");
    } else {
      LOG(L_CLI, "CoAP Rx Response:\r\n");
      message->log(L_CLI);
    }
    return CoAP__Handler_Followup__Send_Reset;
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    str remote_ip        = cmd.argv[0];
    uint16_t remote_port = 5684;
    if (cmd.argc >= 2) {
      remote_port = str_to_uint32_t(cmd.argv[1], 10);
    }
    uint16_t local_port = 0;
    if (cmd.argc >= 3) {
      local_port = str_to_uint32_t(cmd.argv[2], 10);
    }
    str psk_id = {0, 0};
    if (cmd.argc >= 4) {
      psk_id = cmd.argv[3];
    }
    str psk_key = {0, 0};
    if (cmd.argc >= 5) {
      psk_key = cmd.argv[4];
    }

    if (coapPeer) {
      LOGF(L_CLI, "ERROR - already created - call coap.destroy before calling this again\r\n");
      return;
    }

    if (psk_key.len > 16) {
      LOGF(L_CLI, "Invalid PSK key [%*s] - should be max 16 bytes", psk_key.len, psk_key.s);
      return;
    }
    if (!psk_key.len || !psk_id.len) {
      coapPeer = owl_new CoAPPeer(cli.owlModem, local_port, remote_ip, remote_port);
    } else {
      coapPeer = owl_new CoAPPeer(cli.owlModem, psk_id, psk_key, local_port, remote_ip, remote_port);
    }

    if (!coapPeer) {
      LOGF(L_CLI, "ERROR - internal\r\n");
      return;
    }
    coapPeer->setHandlers(handlerStatelessMessage, handlerDTLSEvent, handlerCoAPRequest, handlerCoAPResponse);
    if (!coapPeer->reinitialize()) {
      LOG(L_CLI, "ERROR - internal\r\n");
      return;
    }
  }
};

class CoAPPeerGetTransportIsReady : public OwlModemCLIExecutor {
 public:
  CoAPPeerGetTransportIsReady()
      : OwlModemCLIExecutor("coap.getTransportIsReady", "Check if the CoAPPeer transport is ready") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (!coapPeer) {
      LOG(L_CLI, "ERROR - no CoAP context created - do first coap.create\r\n");
      return;
    }
    LOG(L_CLI, "OK transportIsReady=%d\r\n", coapPeer->transportIsReady());
  }
};

class CoAPPeerSend : public OwlModemCLIExecutor {
 public:
  CoAPPeerSend()
      : OwlModemCLIExecutor(
            "coap.send", "<type> <class> <detail> [<uri-path> [<uri-query> [<content_format> [<payload>]]]]",
            "Send CoAP request - type is 0 for CON, 1 of NON, 2 for ACK, 3 for RST; class is 0 for "
            "Request/Empty, 2 for Response, 4 for Error, etc; detail is 1 for GET, 2 for POST, etc. or error codes",
            3, 7) {
  }

  static void transaction_callback(CoAPPeer *peer, coap_message_id_t message_id, void *cb_param,
                                   coap_client_transaction_event_e event, CoAPMessage *message) {
    switch (event) {
      case CoAP_Client_Transaction_Event__ACK:
        LOG(L_CLI, "Client-Transaction-Event: Received ACK\r\n");
        break;
      case CoAP_Client_Transaction_Event__RST:
        LOG(L_CLI, "Client-Transaction-Event: Received RST\r\n");
        break;
      case CoAP_Client_Transaction_Event__Timeout:
        LOG(L_CLI, "Client-Transaction-Event: Timed out\r\n");
        break;
      case CoAP_Client_Transaction_Event__Canceled:
        LOG(L_CLI, "Client-Transaction-Event: Canceled\r\n");
        break;
      default:
        LOG(L_CLI, "Client-Transaction-Event: %d\r\n", event);
    }
    if (message) message->log(L_CLI);
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (!coapPeer) {
      LOG(L_CLI, "ERROR - no CoAP context created - do first coap.create\r\n");
      return;
    }
    coap_type_e type               = (coap_type_e)str_to_uint32_t(cmd.argv[0], 10);
    coap_code_class_e code_class   = (coap_code_class_e)str_to_uint32_t(cmd.argv[1], 10);
    coap_code_detail_e code_detail = (coap_code_detail_e)str_to_uint32_t(cmd.argv[2], 10);
    str uri_path                   = {0};
    if (cmd.argc >= 4) uri_path = cmd.argv[3];
    str uri_query = {0};
    if (cmd.argc >= 5) uri_query = cmd.argv[4];
    uint64_t content_format = 7777;
    if (cmd.argc >= 6) content_format = str_to_uint32_t(cmd.argv[5], 10);
    str payload = {0};
    if (cmd.argc >= 7) payload = cmd.argv[6];
    CoAPMessage *msg = owl_new CoAPMessage(type, code_class, code_detail, coapPeer->getNextMessageId());
    if (!msg) GOTOERR(error);
    if (uri_path.len) {
      str uri_path_token = {0};
      while (str_tok(uri_path, "/", &uri_path_token)) {
        if (!msg->addOptionUriPath(uri_path_token)) GOTOERR(error);
      }
    }
    if (uri_query.len && !msg->addOptionUriQuery(uri_query)) GOTOERR(error);
    if (content_format != 7777 && !msg->addOptionContentFormat(content_format)) GOTOERR(error);
    if (payload.len) msg->payload = payload;

    switch (type) {
      case CoAP_Type__Confirmable:
        if (!coapPeer->sendReliably(msg, transaction_callback, 0)) GOTOERR(error);
        break;
      case CoAP_Type__Non_Confirmable:
        if (!coapPeer->sendUnreliably(msg, 1, 60)) GOTOERR(error);
        break;
      default:
        if (!coapPeer->sendUnreliably(msg)) GOTOERR(error);
        break;
    }

    msg->log(L_CLI);
    LOGF(L_CLI, "OK\r\n");
    delete msg;
    return;
  error:
    delete msg;
    LOGF(L_CLI, "ERROR\r\n");
  }
};

class CoAPPeerLogClientTransactions : public OwlModemCLIExecutor {
 public:
  CoAPPeerLogClientTransactions()
      : OwlModemCLIExecutor("coap.logClientTransactions", "Show the internal client transaction state") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (!coapPeer) {
      LOG(L_CLI, "ERROR - no CoAP context created - do first coap.create\r\n");
      return;
    }
    coapPeer->logClientTransactions(L_CLI);
  }
};

class CoAPPeerLogServerTransactions : public OwlModemCLIExecutor {
 public:
  CoAPPeerLogServerTransactions()
      : OwlModemCLIExecutor("coap.logServerTransactions", "Show the internal server transaction state") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (!coapPeer) {
      LOG(L_CLI, "ERROR - no CoAP context created - do first coap.create\r\n");
      return;
    }
    coapPeer->logServerTransactions(L_CLI);
  }
};

class CoAPPeerStopRetransmissions : public OwlModemCLIExecutor {
 public:
  CoAPPeerStopRetransmissions()
      : OwlModemCLIExecutor("coap.stopRetransmissions", "<message_id>", "Stop retransmissions for a given message", 1,
                            1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (!coapPeer) {
      LOG(L_CLI, "ERROR - no CoAP client context created - do first coap.create\r\n");
      return;
    }
    coap_message_id_t message_id = str_to_uint32_t(cmd.argv[0], 10);
    if (!coapPeer->stopRetransmissions(message_id)) {
      LOG(L_CLI, "ERROR\r\n");
    } else {
      LOG(L_CLI, "OK\r\n");
    }
  }
};

class CoAPPeerDestroy : public OwlModemCLIExecutor {
 public:
  CoAPPeerDestroy() : OwlModemCLIExecutor("coap.destroy", "Destroy the current CoAP context") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    if (!coapPeer) {
      LOG(L_CLI, "ERROR - no CoAP context created - do first coap.create\r\n");
      return;
    }
    if (!coapPeer->close()) {
      LOG(L_CLI, "ERROR\r\n");
    } else {
      LOG(L_CLI, "OK\r\n");
    }
    delete coapPeer;
    coapPeer = 0;
  }
};



class BreakoutSendCommand : public OwlModemCLIExecutor {
 public:
  BreakoutSendCommand()
      : OwlModemCLIExecutor("breakout.sendCommand", "<command>",
                            "Send a command over the Breakout SDK without receipt request. Prefix with 0x if you wish "
                            "to send binary hex data",
                            1, 32) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    Breakout *breakout = &Breakout::getInstance();
    if (!breakout) {
      LOG(L_CLI, "ERROR - Breakout not initialized\r\n");
      return;
    }
    char buffer[140];
    str command    = {.s = buffer, .len = 0};
    bool is_binary = false;
    if (str_equalcase_prefix_char(cmd.args, "0x")) {
      str input   = {.s = cmd.args.s + 2, .len = cmd.args.len - 2};
      command.len = hex_to_str(command.s, 140, input);
      if (command.len * 2 != input.len) {
        LOG(L_CLI, "ERROR bad input hex string or more than 140 bytes\r\n");
        return;
      }
      is_binary = true;
    } else {
      if (command.len > 140) {
        LOG(L_CLI, "ERROR more than 140 bytes\r\n");
        return;
      }
      command.len = cmd.args.len;
      memcpy(command.s, cmd.args.s, command.len);
      is_binary = false;
    }
    command_status_code_e code = breakout->sendCommand(command, is_binary);
    switch (code) {
      case COMMAND_STATUS_OK:
        LOG(L_CLI, "OK\r\n");
        break;
      case COMMAND_STATUS_ERROR:
        LOG(L_CLI, "ERROR\r\n");
        break;
      case COMMAND_STATUS_BUFFER_TOO_SMALL:
        LOG(L_CLI, "BUFFER_TOO_SMALL\r\n");
        break;
      case COMMAND_STATUS_NO_COMMAND_WAITING:
        LOG(L_CLI, "NO_COMMAND_WAITING\r\n");
        break;
      default:
        LOG(L_CLI, "ERROR %d\r\n", code);
    }
  }
};

class BreakoutSendCommandWithReceiptRequest : public OwlModemCLIExecutor {
 public:
  BreakoutSendCommandWithReceiptRequest()
      : OwlModemCLIExecutor("breakout.sendCommandWithReceiptRequest", "<command>",
                            "Send a command over the Breakout SDK, with receipt request. Prefix with 0x if you wish "
                            "to send binary hex data",
                            1, 32) {
  }

  static void callbackOnDeliveryReceipt(command_receipt_code_e receipt_code, void *cb_parameter) {
    str *data = (str *)cb_parameter;
    switch (receipt_code) {
      case COMMAND_RECEIPT_CONFIRMED_DELIVERY:
        LOG(L_CLI, "Command [%.*s] delivery is confirmed\r\n", data ? data->len : 0, data ? data->s : 0);
        break;
      case COMMAND_RECEIPT_SERVER_ERROR:
        LOG(L_CLI, "Command [%.*s] server error or command rejected\r\n", data ? data->len : 0, data ? data->s : 0);
        break;
      case COMMAND_RECEIPT_CANCELED:
        LOG(L_CLI, "Command [%.*s] delivery canceled\r\n", data ? data->len : 0, data ? data->s : 0);
        break;
      case COMMAND_RECEIPT_TIMEOUT:
        LOG(L_CLI, "Command [%.*s] delivery timeout\r\n", data ? data->len : 0, data ? data->s : 0);
        break;
      default:
        LOG(L_CLI, "Command [%.*s] unknown receipt code %d\r\n", data ? data->len : 0, data ? data->s : 0,
            receipt_code);
        break;
    }
    if (data) {
      str_free(*data);
      owl_free(data);
    }
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    str *data          = 0;
    Breakout *breakout = &Breakout::getInstance();
    command_status_code_e code;
    if (!breakout) {
      LOG(L_CLI, "ERROR - Breakout not initialized\r\n");
      return;
    }
    char buffer[140];
    str command    = {.s = buffer, .len = 0};
    bool is_binary = false;
    if (str_equalcase_prefix_char(cmd.args, "0x")) {
      str input   = {.s = cmd.args.s + 2, .len = cmd.args.len - 2};
      command.len = hex_to_str(command.s, 140, input);
      if (command.len * 2 != input.len) {
        LOG(L_CLI, "ERROR bad input hex string or more than 140 bytes\r\n");
        return;
      }
      is_binary = true;
    } else {
      if (command.len > 140) {
        LOG(L_CLI, "ERROR more than 140 bytes\r\n");
        return;
      }
      command.len = cmd.args.len;
      memcpy(command.s, cmd.args.s, command.len);
      is_binary = false;
    }
    data = (str *)owl_malloc(sizeof(str));
    if (!data) goto out_of_memory;
    str_dup(*data, cmd.args);
    code = breakout->sendCommandWithReceiptRequest(command, callbackOnDeliveryReceipt, data, is_binary);
    switch (code) {
      case COMMAND_STATUS_OK:
        LOG(L_CLI, "OK\r\n");
        break;
      case COMMAND_STATUS_ERROR:
        LOG(L_CLI, "ERROR\r\n");
        goto error;
        break;
      case COMMAND_STATUS_BUFFER_TOO_SMALL:
        LOG(L_CLI, "BUFFER_TOO_SMALL\r\n");
        goto error;
        break;
      case COMMAND_STATUS_NO_COMMAND_WAITING:
        LOG(L_CLI, "NO_COMMAND_WAITING\r\n");
        goto error;
        break;
      default:
        LOG(L_CLI, "ERROR %d\r\n", code);
        goto error;
    }
    return;
  out_of_memory:
  error:
    LOG(L_CLI, "ERROR - allocating memory for callback parameter\r\n");
    if (data) {
      str_free(*data);
      owl_free(data);
    }
  }
};

class BreakoutSetHandlerCommand : public OwlModemCLIExecutor {
 public:
  BreakoutSetHandlerCommand()
      : OwlModemCLIExecutor("breakout.setHandlerCommand", "(on|off)",
                            "Set or unset the CLI handler - triggering log when commands are received. If you want to "
                            "poll with breakout.hasWaitingCommand and breakout.receiveCommand, set it to off.",
                            1, 1) {
  }

  static void commandHandler(const char *buf, size_t bufSize, bool isBinary) {
    LOG(L_CLI, "Received To-SIM Command as %s:\r\n", isBinary ? "binary" : "text");
    str data = {.s = (char *)buf, .len = bufSize};
    LOGSTR(L_CLI, data);
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    Breakout *breakout = &Breakout::getInstance();
    if (!breakout) {
      LOG(L_CLI, "ERROR - Breakout not initialized\r\n");
      return;
    }
    bool set = str_equalcase_char(cmd.argv[0], "on");
    breakout->setCommandHandler(set ? BreakoutSetHandlerCommand::commandHandler : 0);
    LOG(L_CLI, "OK\r\n");
  }
};

class BreakoutHasWaitingCommand : public OwlModemCLIExecutor {
 public:
  BreakoutHasWaitingCommand(char *commandName = "breakout.hasWaitingCommand")
      : OwlModemCLIExecutor(commandName, "Check if we have a waiting command received") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    Breakout *breakout = &Breakout::getInstance();
    if (!breakout) {
      LOG(L_CLI, "ERROR - Breakout not initialized\r\n");
      return;
    }
    bool result = breakout->hasWaitingCommand();
    if (result)
      LOG(L_CLI, "OK commands are waiting\r\n");
    else
      LOG(L_CLI, "OK no commands waiting\r\n");
  }
};

class BreakoutReceiveCommand : public OwlModemCLIExecutor {
 public:
  BreakoutReceiveCommand()
      : OwlModemCLIExecutor("breakout.receiveCommand", "If there are commands waiting, pop one out") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    Breakout *breakout = &Breakout::getInstance();
    if (!breakout) {
      LOG(L_CLI, "ERROR - Breakout not initialized\r\n");
      return;
    }
    size_t size                = 0;
    bool isBinary              = false;

    static char command_buffer[1024];
    str command_str = {.s = command_buffer, .len = 0};
    command_status_code_e code = breakout->receiveCommand(1024, command_buffer, &size, &isBinary);
    command_str.len               = size;
    switch (code) {
      case COMMAND_STATUS_OK:
        if (isBinary) {
          LOG(L_CLI, "OK binary\r\n");
          LOGSTR(L_CLI, command_str);
        } else {
          LOG(L_CLI, "OK text [%.*s]\r\n", size, command_buffer);
        }
        break;
      case COMMAND_STATUS_ERROR:
        LOG(L_CLI, "ERROR\r\n");
        break;
      case COMMAND_STATUS_BUFFER_TOO_SMALL:
        LOG(L_CLI, "BUFFER_TOO_SMALL\r\n");
        break;
      case COMMAND_STATUS_NO_COMMAND_WAITING:
        LOG(L_CLI, "NO_COMMAND_WAITING\r\n");
        break;
      default:
        LOG(L_CLI, "ERROR %d\r\n", code);
    }
  }
};

class BreakoutCheckForCommands : public OwlModemCLIExecutor {
 public:
  BreakoutCheckForCommands()
      : OwlModemCLIExecutor("breakout.checkForCommands", "Trigger the manual check for commands") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    Breakout *breakout = &Breakout::getInstance();
    if (!breakout) {
      LOG(L_CLI, "ERROR - Breakout not initialized\r\n");
      return;
    }
    breakout->checkForCommands();
    LOG(L_CLI, "OK\r\n");
  }
};


class BreakoutSetPollingInterval : public OwlModemCLIExecutor {
 public:
  BreakoutSetPollingInterval()
      : OwlModemCLIExecutor("breakout.setPollingInterval", "<interval>",
                            "Set the automatic polling interval to a certain number of seconds", 1, 1) {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    Breakout *breakout = &Breakout::getInstance();
    if (!breakout) {
      LOG(L_CLI, "ERROR - Breakout not initialized\r\n");
      return;
    }
    uint32_t interval_seconds = str_to_uint32_t(cmd.argv[0], 10);
    breakout->setPollingInterval(interval_seconds);
  }
};


class BreakoutReinitTransport : public OwlModemCLIExecutor {
 public:
  BreakoutReinitTransport()
      : OwlModemCLIExecutor(
            "breakout.reinitializeTransport",
            "Reinitialize the breakout transport - mostly for testing, otherwise it should happen automatically") {
  }

  void executor(OwlModemCLI &cli, OwlModemCLICommand &cmd) {
    Breakout *breakout = &Breakout::getInstance();
    if (!breakout) {
      LOG(L_CLI, "ERROR - Breakout not initialized\r\n");
      return;
    }
    if (breakout->reinitializeTransport()) {
      LOG(L_CLI, "OK\r\n");
    } else {
      LOG(L_CLI, "ERROR\r\n");
    }
  }
};


#define MAX_COMMANDS 90

OwlModemCLI::OwlModemCLI(OwlModemRN4 *modem, USBSerial *debug_port) {
  this->owlModem  = modem;
  this->debugPort = debug_port;
  if (debug_port) debug_port->enableBlockingTx();  // reliably write to it

  executors = (OwlModemCLIExecutor **)owl_malloc(MAX_COMMANDS * sizeof(OwlModemCLIExecutor *));

  int cnt          = 0;
  executors[cnt++] = owl_new SetDebugLevel();
  executors[cnt++] = owl_new SoftReset();

  executors[cnt++] = owl_new RawBypass();
  executors[cnt++] = owl_new RawGNSSBypass();

  executors[cnt++] = owl_new PowerOn();
  executors[cnt++] = owl_new PowerOff();

  executors[cnt++] = owl_new GetProductIdentification();
  executors[cnt++] = owl_new GetManufacturer();
  executors[cnt++] = owl_new GetModel();
  executors[cnt++] = owl_new GetVersion();
  executors[cnt++] = owl_new GetIMEI();
  executors[cnt++] = owl_new GetBatteryChargeLevels();
  executors[cnt++] = owl_new GetIndicators();
  executors[cnt++] = owl_new GetIndicatorsHelp();


  executors[cnt++] = owl_new GetICCID();
  executors[cnt++] = owl_new GetIMSI();
  executors[cnt++] = owl_new GetMSISDN();
  executors[cnt++] = owl_new GetPINStatus();
  executors[cnt++] = owl_new VerifyPIN();
  executors[cnt++] = owl_new VerifyPUK();


  executors[cnt++] = owl_new GetModemFunctionality();
  executors[cnt++] = owl_new SetModemFunctionality();

  executors[cnt++] = owl_new GetModemMNOProfile();
  executors[cnt++] = owl_new SetModemMNOProfile();

  executors[cnt++] = owl_new GetOperatorSelection();
  executors[cnt++] = owl_new SetOperatorSelection();
  executors[cnt++] = owl_new GetOperatorList();

  executors[cnt++] = owl_new GetNetworkRegistrationStatus();
  executors[cnt++] = owl_new SetNetworkRegistrationURC();
  executors[cnt++] = owl_new GetGPRSRegistrationStatus();
  executors[cnt++] = owl_new SetGPRSRegistrationURC();
  executors[cnt++] = owl_new GetEPSRegistrationStatus();
  executors[cnt++] = owl_new SetEPSRegistrationURC();

  executors[cnt++] = owl_new GetSignalQuality();


  executors[cnt++] = owl_new GetAPNIPAddress();


  executors[cnt++] = owl_new OpenSocket();
  executors[cnt++] = owl_new CloseSocket();
  executors[cnt++] = owl_new GetSocketError();
  executors[cnt++] = owl_new ConnectSocket();
  executors[cnt++] = owl_new SendUDP();
  executors[cnt++] = owl_new SendTCP();
  executors[cnt++] = owl_new SendToUDP();
  executors[cnt++] = owl_new GetQueuedForReceive();
  executors[cnt++] = owl_new ReceiveUDP();
  executors[cnt++] = owl_new ReceiveTCP();
  executors[cnt++] = owl_new ReceiveFromUDP();
  executors[cnt++] = owl_new ListenUDP();
  executors[cnt++] = owl_new ListenTCP();
  executors[cnt++] = owl_new AcceptTCP();

  executors[cnt++] = owl_new OpenSocketListenUDP();
  executors[cnt++] = owl_new OpenSocketListenConnectUDP();
  executors[cnt++] = owl_new OpenSocketListenConnectTCP();
  executors[cnt++] = owl_new OpenSocketAcceptTCP();


  executors[cnt++] = owl_new GetGNSSData();


  executors[cnt++] = owl_new DTLSClientCreate();
  executors[cnt++] = owl_new DTLSClientGetStatus();
  executors[cnt++] = owl_new DTLSClientSend();
  executors[cnt++] = owl_new DTLSClientRenegotiate();
  executors[cnt++] = owl_new DTLSClientRehandshake();
  executors[cnt++] = owl_new DTLSClientDestroy();

  executors[cnt++] = owl_new CoAPPeerCreate();
  executors[cnt++] = owl_new CoAPPeerGetTransportIsReady();
  executors[cnt++] = owl_new CoAPPeerSend();
  executors[cnt++] = owl_new CoAPPeerLogClientTransactions();
  executors[cnt++] = owl_new CoAPPeerLogServerTransactions();
  executors[cnt++] = owl_new CoAPPeerStopRetransmissions();
  executors[cnt++] = owl_new CoAPPeerDestroy();

  executors[cnt++] = owl_new BreakoutSendCommand();
  executors[cnt++] = owl_new BreakoutSendCommandWithReceiptRequest();
  executors[cnt++] = owl_new BreakoutSetHandlerCommand();
  executors[cnt++] = owl_new BreakoutHasWaitingCommand();
  executors[cnt++] = owl_new BreakoutReceiveCommand();
  executors[cnt++] = owl_new BreakoutCheckForCommands();
  executors[cnt++] = owl_new BreakoutSetPollingInterval();
  executors[cnt++] = owl_new BreakoutReinitTransport();

  executors[cnt++] = 0;
  if (cnt > MAX_COMMANDS) {
    LOG(L_CRIT, "Registered more commands than we have space for! %d > %d - Increase MAX_COMMANDS\r\n", cnt,
        MAX_COMMANDS);
  }

  cmdHistoryHead     = -1;
  cmdHistoryTail     = -1;
  cmdHistoryIterator = -1;
  for (int i = 0; i < MODEM_CLI_CMD_LEN; i++) {
    empty_spaces[i] = ' ';
  }
  empty_spaces[MODEM_CLI_CMD_LEN] = 0;
}

OwlModemCLI::~OwlModemCLI() {
  for (int i = 0; executors[i]; i++)
    delete executors[i];
  owl_free(executors);
}



static str s_help = {.s = "help", .len = 4};
static str s_quit = {.s = "quit", .len = 4};

static bool in_command_execution = false; /**< Protection for commands which call handleUserInput() inside them */
static str s_history             = {.s = "history", .len = 7};

int OwlModemCLI::handleUserInput(int resume) {
  if (in_command_execution) return resume;

  if (!owlModem || !debugPort) return 0;
  char c;
  if (!resume) {
    // flush debug port on startup
    while (debugPort->available()) {
      c = debugPort->read();
    }
    LOGF(L_CLI, "\r\n");
    LOGF(L_CLI, "Welcome to the OwlModem simple CLI interface!\r\n");
    LOGF(L_CLI, "  Type your commands to test the OwlModem API. Hit <TAB> for completion and fast help.\r\n");
    LOGF(L_CLI, "  - use 'help' for a list of supported commands.\r\n");
    LOGF(L_CLI, "  - use 'quit' to shut-down this CLI and return control to your own program.\r\n");
    LOGF(L_CLI, "  - use 'history' print the command history (limited to %d entries).\r\n", MODEM_CLI_CMD_HISTORY);
    LOGF(L_CLI, "  - use the Up and Down arrow to scroll through the command history\r\n");
    LOGF(L_CLI, "  - pressing the Down Arrow key while not scrolling through history will clear\r\n");
    LOGF(L_CLI, "    the current command.\r\n");
    LOGF(L_CLI, "\r\n");
    LOGF(L_CLI, CLI_PROMPT);
    command.len = 0;
  }
  while (debugPort->available()) {
    c = debugPort->read();
    switch (c) {
      case '\n':
      case '\r':
        if (command.len > 0) {
          LOGE(L_CLI, "\r\n");

          if (cmdHistoryHead == -1) {
            cmdHistoryHead     = 0;
            cmdHistoryTail     = 0;
            cmdHistoryIterator = -1;
            memcpy(cmdHistory[cmdHistoryHead], command.s, command.len);
            cmdHistoryLen[cmdHistoryHead] = command.len;
          } else {
            if (memcmp(cmdHistory[cmdHistoryHead], command.s, command.len)) {
              cmdHistoryHead = (cmdHistoryHead + 1) % MODEM_CLI_CMD_HISTORY;
              if (cmdHistoryHead == cmdHistoryTail) {
                cmdHistoryTail = (cmdHistoryTail + 1) % MODEM_CLI_CMD_HISTORY;
              }
              memcpy(cmdHistory[cmdHistoryHead], command.s, command.len);
              cmdHistoryLen[cmdHistoryHead] = command.len;
            }
            cmdHistoryIterator = -1;
          }


          cmd.parse(command);
          in_command_execution = true;

          OwlModemCLIExecutor *executor = findExecutor(cmd.name);
          if (executor) {
            /* registered command */
            if (!executor->checkParams(cmd)) {
              LOGF(L_CLI, "Invalid numbers of parameters %d. Must be between %d and %d.\r\n", cmd.argc,
                   executor->minParams, executor->maxParams);
            } else {
              executor->executor(*this, cmd);
            }
          } else if (str_equalcase(cmd.name, s_help)) {
            /* help */
            doHelp(cmd.argc >= 1 ? &cmd.argv[0] : 0);
          } else if (str_equalcase(cmd.name, s_history)) {
            /* history */
            doHistory();
          } else if (str_equalcase(cmd.name, s_quit)) {
            /* exit */
            LOGF(L_CLI, "\r\nBye-Bye!\r\n");
            in_command_execution = false;
            return 0;
          } else if (str_equalcase_prefix_char(command, "AT")) {
            /* AT<something> */
            command.s[command.len >= MODEM_CLI_CMD_LEN ? MODEM_CLI_CMD_LEN - 1 : command.len] = 0;
            at_result_code_e result_code =
                owlModem->AT.doCommandBlocking(command.s, 30 * 1000, &response);
            if (!response.len)
              LOGF(L_CLI, " Command [%.*s] returned with result code %s (%d)\r\n", command.len, command.s,
                   at_result_code_text(result_code), result_code);
            else
              LOGF(L_CLI, " Command [%.*s] returned with result code %s (%d) and output [%.*s]\r\n", command.len,
                   command.s, at_result_code_text(result_code), result_code, response.len, response.s);
          } else {
            /* Invalid Command */
            LOG(L_CLI, "Invalid command: [%.*s]\r\n", command.len, command.s);
          }

          in_command_execution = false;
          command.len          = 0;
          LOGE(L_CLI, "\r\n");
          LOGF(L_CLI, CLI_PROMPT);
        } else {
          cmdHistoryIterator = -1;
          LOGE(L_CLI, "\r\n");
          LOGF(L_CLI, CLI_PROMPT);
        }
        break;
      case 127:
        /* backspace / delete */
        if (command.len > 0) {
          command.s[command.len - 1] = ' ';
          LOGE(L_CLI, "\r");
          LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, command.s);
          command.len--;
        }
        LOGE(L_CLI, "\r");
        LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, command.s);
        break;

      case 21:
        /* Ctrl-u   - Clear line */
        LOGE(L_CLI, "\r");
        LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, empty_spaces);
        command.len  = 0;
        command.s[0] = 0;
        LOGE(L_CLI, "\r");
        LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, command.s);
        break;

      case 23:
        /* Ctrl-w    - Delete last word */
        LOGE(L_CLI, "\r");
        LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, empty_spaces);
        for (int i = command.len; i > 0; i--) {
          if (command.s[i - 1] == ' ') {
            command.len -= 1;
            command.s[command.len] = 0;
            break;
          }
          command.len -= 1;
          command.s[command.len] = 0;
        }
        LOGE(L_CLI, "\r");
        LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, command.s);
        break;

      case 9:
        /* Tab */
        commandCompletion(&command);
        break;

      case 'A':
        /* UP Arrow - If this is the sequence \[A  ESC [ A */
        if (command.len >= 2 && command.s[command.len - 2] == 27 && command.s[command.len - 1] == '[' &&
            cmdHistoryHead != -1) {
          if (cmdHistoryIterator == -1) {
            cmdHistoryIterator = cmdHistoryHead;
          } else if (cmdHistoryIterator > cmdHistoryTail) {
            cmdHistoryIterator -= 1;
          } else if (cmdHistoryIterator <= cmdHistoryHead && cmdHistoryHead < cmdHistoryTail) {
            cmdHistoryIterator -= 1;
            if (cmdHistoryIterator == -1) {
              cmdHistoryIterator = MODEM_CLI_CMD_HISTORY - 1;
            }
          }

          memcpy(command.s, cmdHistory[cmdHistoryIterator], cmdHistoryLen[cmdHistoryIterator]);
          LOGE(L_CLI, "\r");
          LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, empty_spaces);
          command.len = cmdHistoryLen[cmdHistoryIterator];
          LOGE(L_CLI, "\r");
          LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, command.s);

        } else {
          if (command.len < MODEM_CLI_CMD_LEN) command.s[command.len++] = c;
          debugPort->write(c);
        }
        break;

      case 'B':
        /* DOWN Arrow - If this is the sequence \[A  ESC [ A */
        if (command.len >= 2 && command.s[command.len - 2] == 27 && command.s[command.len - 1] == '[') {
          if (cmdHistoryIterator != -1 && cmdHistoryIterator != cmdHistoryHead) {
            if (cmdHistoryIterator < cmdHistoryHead) {
              cmdHistoryIterator += 1;
            } else if (cmdHistoryIterator != cmdHistoryHead) {
              cmdHistoryIterator = (cmdHistoryIterator + 1) % MODEM_CLI_CMD_HISTORY;
            }
            memcpy(command.s, cmdHistory[cmdHistoryIterator], cmdHistoryLen[cmdHistoryIterator]);
            LOGE(L_CLI, "\r");
            LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, empty_spaces);
            command.len = cmdHistoryLen[cmdHistoryIterator];
            LOGE(L_CLI, "\r");
            LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, command.s);
          } else {
            /* Ignore Down Arrow if we are not browsing the history */
            LOGE(L_CLI, "\r");
            LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, empty_spaces);
            command.len  = 0;
            command.s[0] = 0;
            LOGE(L_CLI, "\r");
            LOGF(L_CLI, CLI_PROMPT "%.*s", command.len, command.s);
          }
        } else {
          if (command.len < MODEM_CLI_CMD_LEN) command.s[command.len++] = c;
          debugPort->write(c);
        }
        break;

      case 'C':
        /* Arrow right - Ignore */
        if (command.len >= 2 && command.s[command.len - 2] == 27 && command.s[command.len - 2] == '[') {
          command.len -= 2;
        } else {
          if (command.len < MODEM_CLI_CMD_LEN) command.s[command.len++] = c;
          debugPort->write(c);
        }
        break;

      case 'D':
        /* Arrow left - Ignore */
        if (command.len >= 2 && command.s[command.len - 2] == 27 && command.s[command.len - 2] == '[') {
          command.len -= 2;
        } else {
          if (command.len < MODEM_CLI_CMD_LEN) command.s[command.len++] = c;
          debugPort->write(c);
        }
        break;
      default:
        //          SerialUSB.print("char(");
        //          SerialUSB.print((int)c);
        //          SerialUSB.print(")");
        if (command.len < MODEM_CLI_CMD_LEN) command.s[command.len++] = c;
        debugPort->write(c);
    }
  }

  return 1;
}

OwlModemCLIExecutor *OwlModemCLI::findExecutor(str name) {
  for (int i = 0; executors[i]; i++)
    if (str_equalcase(name, executors[i]->name)) return executors[i];
  return 0;
}

void OwlModemCLI::doHelp(str *prefix) {
  if (!prefix) {
    LOGE(L_CLI, "\r\n");
    LOGE(L_CLI, "List of supported commands\r\n");
    LOGE(L_CLI, "  - %.*s - print this output\r\n", s_help.len, s_help.s);
    LOGE(L_CLI, "  - %.*s - print CLI command history\r\n", s_history.len, s_history.s);
    LOGE(L_CLI, "  - %.*s - shut-down this CLI\r\n", s_quit.len, s_quit.s);
    LOGE(L_CLI, "\r\n");
    LOGE(L_CLI,
         "  - AT<something> - bypass an AT command; replace <something> with your bytes (max 2 minutes timeout)\r\n");
    LOGE(L_CLI, "\r\n");
    for (int i = 0; executors[i] != 0; i++) {
      if (executors[i]->helpParameters.len == 0)
        LOGE(L_CLI, "  - %.*s - %.*s\r\n", executors[i]->name.len, executors[i]->name.s, executors[i]->help.len,
             executors[i]->help.s);
      else
        LOGE(L_CLI, "  - %.*s %.*s - %.*s\r\n", executors[i]->name.len, executors[i]->name.s,
             executors[i]->helpParameters.len, executors[i]->helpParameters.s, executors[i]->help.len,
             executors[i]->help.s);
    }
    LOGE(L_CLI, "\r\n");
  } else {
    LOGE(L_CLI, "\r\n");
    if (str_equalcase_prefix(s_help, *prefix)) LOGE(L_CLI, "  - %.*s - print this output\r\n", s_help.len, s_help.s);
    if (str_equalcase_prefix(s_quit, *prefix)) LOGE(L_CLI, "  - %.*s - shut-down this CLI\r\n", s_quit.len, s_quit.s);
    for (int i = 0; executors[i]; i++) {
      if (!str_equalcase_prefix(executors[i]->name, *prefix)) continue;
      if (executors[i]->helpParameters.len == 0)
        LOGE(L_CLI, "  - %.*s - %.*s\r\n", executors[i]->name.len, executors[i]->name.s, executors[i]->help.len,
             executors[i]->help.s);
      else
        LOGE(L_CLI, "  - %.*s %.*s - %.*s\r\n", executors[i]->name.len, executors[i]->name.s,
             executors[i]->helpParameters.len, executors[i]->helpParameters.s, executors[i]->help.len,
             executors[i]->help.s);
    }
  }
}

void OwlModemCLI::doHistory() {
  if (cmdHistoryHead == -1) {
    LOGF(L_CLI, "Empty\r\n");
  } else if (cmdHistoryHead < cmdHistoryTail) {
    int i = 0;
    for (i = cmdHistoryHead + MODEM_CLI_CMD_HISTORY; i >= cmdHistoryTail; i--) {
#ifdef MODEM_CLI_CMD_HISTORY_DEBUG
      LOGF(L_CLI, "  %2d: %.*s\r\n", (i % MODEM_CLI_CMD_HISTORY), cmdHistoryLen[i % MODEM_CLI_CMD_HISTORY],
           cmdHistory[i % MODEM_CLI_CMD_HISTORY]);
#else
      LOGF(L_CLI, "  %.*s\r\n", cmdHistoryLen[i % MODEM_CLI_CMD_HISTORY], cmdHistory[i % MODEM_CLI_CMD_HISTORY]);
#endif
    }
  } else {
    int i = 0;
    for (i = cmdHistoryHead; i >= cmdHistoryTail; i--) {
#ifdef MODEM_CLI_CMD_HISTORY_DEBUG
      LOGF(L_CLI, "  %2d: %.*s\r\n", (i % MODEM_CLI_CMD_HISTORY), cmdHistoryLen[i % MODEM_CLI_CMD_HISTORY],
           cmdHistory[i % MODEM_CLI_CMD_HISTORY]);
#else
      LOGF(L_CLI, "  %.*s\r\n", cmdHistoryLen[i % MODEM_CLI_CMD_HISTORY], cmdHistory[i % MODEM_CLI_CMD_HISTORY]);
#endif
    }
  }
#ifdef MODEM_CLI_CMD_HISTORY_DEBUG
  LOGF(L_CLI, "Iterator: %d; Head: %d; Tail: %d\r\n", cmdHistoryIterator, cmdHistoryHead, cmdHistoryTail);
#endif
}

void OwlModemCLI::commandCompletion(str *command) {
  if (!command) return;
  command->s[command->len] = 0;
  /* if there is a space, it's too late for completion - show help just for this*/
  for (int i = 0; i < command->len; i++)
    if (command->s[i] == ' ') {
      int k        = command->len;
      command->len = i;
      doHelp(command);
      command->len = k;
      LOGE(L_CLI, "\r\n");
      LOGF(L_CLI, CLI_PROMPT "%.*s", command->len, command->s);
      return;
    }
  /* if it's empty, do all help */
  if (command->len == 0) {
    doHelp(0);
    LOGE(L_CLI, "\r\n");
    LOGF(L_CLI, CLI_PROMPT "%.*s", command->len, command->s);
    return;
  }
  /* count partial matches */
  int matches = 0, last_match = 999999;
  if (str_equalcase_prefix(s_help, *command)) {
    matches++;
    last_match = -2;
  }
  if (str_equalcase_prefix(s_quit, *command)) {
    matches++;
    last_match = -1;
  }
  for (int i = 0; executors[i]; i++)
    if (str_equalcase_prefix(executors[i]->name, *command)) {
      matches++;
      last_match = i;
    }
  if (!matches) return;

  /* fill it up to the max length */
  int match_len = command->len;
  char prefix[MODEM_CLI_CMD_LEN + 1];
  memcpy(prefix, command->s, command->len + 1);
  if (last_match == -2) {
    memcpy(command->s, s_help.s, s_help.len);
    command->len = s_help.len;
  } else if (last_match == -1) {
    memcpy(command->s, s_quit.s, s_quit.len);
    command->len = s_quit.len;
  } else {
    memcpy(command->s, executors[last_match]->name.s, executors[last_match]->name.len);
    command->len = executors[last_match]->name.len;
  }

  if (matches == 1) {
    /* if single match, fill it up */
    command->s[command->len++] = ' ';
  } else {
    /* complete the common part */
    for (int i = 0; executors[i]; i++) {
      if (!str_equalcase_prefix_char(executors[i]->name, prefix)) continue;
      for (int j = command->len - 1; j >= 0; j--)
        if (executors[i]->name.len <= j || tolower(executors[i]->name.s[j]) != tolower(command->s[j])) {
          command->len             = j;
          command->s[command->len] = 0;
        }
    }
    /* if multiple matches, list them */
    doHelp(command);
  }
  LOGE(L_CLI, "\r\n");
  LOGF(L_CLI, CLI_PROMPT "%.*s", command->len, command->s);
}
