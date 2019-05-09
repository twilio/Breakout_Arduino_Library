/*
 * enums.h
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
 * \file enums.h - various enumerations for AT commands and parameters
 */

#ifndef __OWL_MODEM_ENUMS_H__
#define __OWL_MODEM_ENUMS_H__


#include "../utils/utils.h"


#define CMDLT "\r"



typedef enum {
  AT_Result_Code__wait_input   = -6,
  AT_Result_Code__in_progress  = -5,
  AT_Result_Code__cme_error    = -4,
  AT_Result_Code__failure      = -3,
  AT_Result_Code__timeout      = -2,
  AT_Result_Code__unknown      = -1,
  AT_Result_Code__OK           = 0,
  AT_Result_Code__CONNECT      = 1,
  AT_Result_Code__RING         = 2,
  AT_Result_Code__NO_CARRIER   = 3,
  AT_Result_Code__ERROR        = 4,
  AT_Result_Code__CONNECT_1200 = 5,
  AT_Result_Code__NO_DIALTONE  = 6,
  AT_Result_Code__BUSY         = 7,
  AT_Result_Code__NO_ANSWER    = 8,
} at_result_code_e;

at_result_code_e at_result_code_resolve(str value);
at_result_code_e at_result_code_extract(const char *value, int max_len);
char *at_result_code_text(at_result_code_e code);



typedef enum {
  AT_CFUN__FUN__Minimum_Functionality                 = 0,
  AT_CFUN__FUN__Full_Functionality                    = 1,
  AT_CFUN__FUN__Airplane_Mode                         = 4,
  AT_CFUN__FUN__Enable_SIM_Toolkit_Interface          = 6, /**< only for SARA-R4/N4 series */
  AT_CFUN__FUN__Disable_SIM_Toolkit_Interface_7       = 7, /**< only for SARA-R4/N4 series */
  AT_CFUN__FUN__Disable_SIM_Toolkit_Interface_8       = 8, /**< only for SARA-R4/N4 series */
  AT_CFUN__FUN__Enable_SIM_Toolkit_Interface_Raw_Mode = 9, /**< only for SARA-R4/N4 series */
  AT_CFUN__FUN__Modem_Silent_Reset__No_SIM_Reset =
      15, /**< only for SARA-R4/N4 series - with detach from network and saving of NVM parameters */
  AT_CFUN__FUN__Modem_Silent_Reset__With_SIM_Reset =
      16, /**< only for SARA-R4/N4 series - with detach from network and saving of NVM parameters */
  AT_CFUN__FUN__Minimum_Functionality_with_CS_PS_and_SIM_Deactivated = 19, /**< only for SARA-R4/N4 series */
  AT_CFUN__FUN__Modem_Deep_Low_Power_Mode = 127, /**< only for SARA-R4/N4 series - with detach from network and saving
                                               of NVM parameters; wake up with power cycle or module reset */
} at_cfun_fun_e;

char *at_cfun_fun_text(at_cfun_fun_e code);

typedef enum {
  AT_CFUN__RST__No_Modem_Reset             = 0,
  AT_CFUN__RST__Modem_and_SIM_Silent_Reset = 1, /**< do a modem & SIM silent reset before setting fun mode */
} at_cfun_rst_e;

char *at_cfun_rst_text(at_cfun_rst_e code);

typedef enum {
  AT_CFUN__POWER_MODE__Minimum_Functionality                                = 0,
  AT_CFUN__POWER_MODE__Full_Functionality                                   = 1,
  AT_CFUN__POWER_MODE__Airplane_Mode                                        = 4,
  AT_CFUN__POWER_MODE__Minimum_Functionality_with_CS_PS_and_SIM_Deactivated = 19, /**< only for SARA-R4/N4 series */
} at_cfun_power_mode_e;

char *at_cfun_power_mode_text(at_cfun_power_mode_e code);

typedef enum {
  AT_CFUN__STK_MODE__Interface_Disabled_Proactive_SIM_APPL_Enabled_0 = 0,
  AT_CFUN__STK_MODE__Dedicated_Mode_Proactive_SIM_APPL_Enabled       = 6,
  AT_CFUN__STK_MODE__Interface_Disabled_Proactive_SIM_APPL_Enabled_7 = 7,
  AT_CFUN__STK_MODE__Interface_Disabled_Proactive_SIM_APPL_Enabled_8 = 8,
  AT_CFUN__STK_MODE__Interface_Raw_Mode_Proactive_SIM_APPL_Enabled   = 9,
} at_cfun_stk_mode_e; /**< ? only for SARA-R4/N4 series */

char *at_cfun_stk_mode_text(at_cfun_stk_mode_e code);

typedef enum {
  AT_UMNOPROF__MNO_PROFILE__SW_Default       = 0,
  AT_UMNOPROF__MNO_PROFILE__SIM_ICCID_Select = 1,
  AT_UMNOPROF__MNO_PROFILE__ATT              = 2,
  AT_UMNOPROF__MNO_PROFILE__Verizon          = 3,
  AT_UMNOPROF__MNO_PROFILE__Telstra          = 4,
  AT_UMNOPROF__MNO_PROFILE__TMO              = 5,
  AT_UMNOPROF__MNO_PROFILE__CT               = 6,
} at_umnoprof_mno_profile_e;

char *at_umnoprof_mno_profile_text(at_umnoprof_mno_profile_e code);



typedef enum {
  AT_COPS__Mode__Automatic_Selection     = 0,
  AT_COPS__Mode__Manual_Selection        = 1,
  AT_COPS__Mode__Deregister_from_Network = 2,
  AT_COPS__Mode__Set_Only_Format         = 3,
  AT_COPS__Mode__Manual_Automatic        = 4, /**< Start a manual selection; if it fails, go into automatic mode and
                                               * stay there. */
} at_cops_mode_e;

char *at_cops_mode_text(at_cops_mode_e code);

typedef enum {
  AT_COPS__Format__Long_Alphanumeric  = 0, /**< Up to 24 characters */
  AT_COPS__Format__Short_Alphanumeric = 1, /**< Up to 10 characters */
  AT_COPS__Format__Numeric            = 2, /**< 5 or 6 characters MCC/MNC codes - FFFF means undefined */
} at_cops_format_e;

char *at_cops_format_text(at_cops_format_e code);

typedef enum {
  AT_COPS__Stat__Unknown   = 0,
  AT_COPS__Stat__Available = 1,
  AT_COPS__Stat__Current   = 2,
  AT_COPS__Stat__Forbidden = 3,
} at_cops_stat_e;

char *at_cops_stat_text(at_cops_stat_e code);

typedef enum {
  // TODO - define more values , for other modems too
  AT_COPS__Access_Technology__LTE        = 7,
  AT_COPS__Access_Technology__EC_GSM_IoT = 8, /**< A/Gb mode - on the ublox SARA-R404M-7, SARA-R410M-52B */
  AT_COPS__Access_Technology__LTE_Cat_M1 = 8, /**< On the ublox SARA-R410M-01B, SARA-R410M-02B */
  AT_COPS__Access_Technology__LTE_NB_S1  = 9, /**< NB-S1 mode - on the SARA-R410M-52B, SARA-N4 */
} at_cops_act_e;

char *at_cops_act_text(at_cops_act_e code);



typedef enum {
  AT_CREG__N__URC_Disabled             = 0, /**< No Unsolicited Registration Codes reported */
  AT_CREG__N__Network_Registration_URC = 1, /**< Unsolicited Registration Codes reported for Network Registration */
  AT_CREG__N__Network_Registration_and_Location_Information_URC =
      2, /**< Unsolicited Registration Codes reported for Network Registration and Location Information */
} at_creg_n_e;

char *at_creg_n_text(at_creg_n_e code);

typedef enum {
  AT_CREG__Stat__Not_Registered                       = 0, /**< Not registered and not searching */
  AT_CREG__Stat__Registered_Home_Network              = 1,
  AT_CREG__Stat__Not_Registered_but_Searching         = 2,
  AT_CREG__Stat__Registration_Denied                  = 3,
  AT_CREG__Stat__Unknown                              = 4, /**< E.g. out of coverage */
  AT_CREG__Stat__Registered_Roaming                   = 5,
  AT_CREG__Stat__Registered_for_SMS_Only_Home_Network = 6, /**< Only ublox SARA-R4/N4 modems */
  AT_CREG__Stat__Registered_for_SMS_Only_Roaming      = 7, /**< Only ublox SARA-R4/N4 modems */
  AT_CREG__Stat__Registered_for_CSFB_Only_Home_Network =
      9, /**< Applicable only for AcT E-UTRAN; Only ublox SARA-R4/N4 modems */
  AT_CREG__Stat__Registered_for_CSFB_Only_Roaming =
      10, /**< Applicable only for AcT E-UTRAN; Only ublox SARA-R4/N4 modems */
} at_creg_stat_e;

char *at_creg_stat_text(at_creg_stat_e code);

typedef enum {
  AT_CREG__AcT__GSM                                     = 0, /**< 2G */
  AT_CREG__AcT__GSM_Compact                             = 1, /**< 2G */
  AT_CREG__AcT__UTRAN                                   = 2, /**< 3G */
  AT_CREG__AcT__GSM_with_EDGE_Availability              = 3, /**< 2.5G */
  AT_CREG__AcT__UTRAN_with_HSDPA_Availability           = 4, /**< 3.5G */
  AT_CREG__AcT__UTRAN_with_HSUPA_Availability           = 5, /**< 3.5G */
  AT_CREG__AcT__UTRAN_with_HSDPA_and_HSUPA_Availability = 6, /**< 3.5G */
  AT_CREG__AcT__E_UTRAN                                 = 7, /**< 4G */
  AT_CREG__AcT__EC_GSM_IoT                              = 8, /**<  A/Gb mode - Only ublox SARA-R410M-01B/02B */
  AT_CREG__AcT__E_UTRAN_NB = 9,   /**<  NB-S1 mode - Only ublox SARA-R410M-01B/02B/52B, SARA-N4*/
  AT_CREG__AcT__invalid    = 255, /**< Indicates that the current AcT is invalid */
} at_creg_act_e;

char *at_creg_act_text(at_creg_act_e code);



typedef enum {
  AT_CGREG__N__URC_Disabled             = 0, /**< No Unsolicited Registration Codes reported */
  AT_CGREG__N__Network_Registration_URC = 1, /**< Unsolicited Registration Codes reported for Network Registration */
  AT_CGREG__N__Network_Registration_and_Location_Information_URC =
      2, /**< Unsolicited Registration Codes reported for Network Registration and Location Information */
} at_cgreg_n_e;

char *at_cgreg_n_text(at_cgreg_n_e code);

typedef enum {
  AT_CGREG__Stat__Not_Registered                              = 0, /**< Not registered and not searching */
  AT_CGREG__Stat__Registered_Home_Network                     = 1,
  AT_CGREG__Stat__Not_Registered_but_Searching                = 2,
  AT_CGREG__Stat__Registration_Denied                         = 3,
  AT_CGREG__Stat__Unknown                                     = 4, /**< E.g. out of coverage */
  AT_CGREG__Stat__Registered_Roaming                          = 5,
  AT_CGREG__Stat__Attached_for_Emergency_Bearer_Services_Only = 8,
} at_cgreg_stat_e;

char *at_cgreg_stat_text(at_cgreg_stat_e code);

typedef enum {
  AT_CGREG__AcT__GSM                                     = 0,   /**< 2G */
  AT_CGREG__AcT__GSM_Compact                             = 1,   /**< 2G */
  AT_CGREG__AcT__UTRAN                                   = 2,   /**< 3G */
  AT_CGREG__AcT__GSM_with_EDGE_Availability              = 3,   /**< 2.5G */
  AT_CGREG__AcT__UTRAN_with_HSDPA_Availability           = 4,   /**< 3.5G */
  AT_CGREG__AcT__UTRAN_with_HSUPA_Availability           = 5,   /**< 3.5G */
  AT_CGREG__AcT__UTRAN_with_HSDPA_and_HSUPA_Availability = 6,   /**< 3.5G */
  AT_CGREG__AcT__invalid                                 = 255, /**< Indicates that the current AcT is invalid */
} at_cgreg_act_e;

char *at_cgreg_act_text(at_cgreg_act_e code);



typedef enum {
  AT_CEREG__N__URC_Disabled             = 0, /**< No Unsolicited Registration Codes reported */
  AT_CEREG__N__Network_Registration_URC = 1, /**< Unsolicited Registration Codes reported for Network Registration */
  AT_CEREG__N__Network_Registration_and_Location_Information_URC =
      2, /**< Unsolicited Registration Codes reported for Network Registration and Location Information */
  AT_CEREG__N__Network_Registration_Location_Information_and_EMM_URC     = 3,
  AT_CEREG__N__PSM_Network_Registration_and_Location_Information_URC     = 4,
  AT_CEREG__N__PSM_Network_Registration_Location_Information_and_EMM_URC = 5,
} at_cereg_n_e;

char *at_cereg_n_text(at_cereg_n_e code);

typedef enum {
  AT_CEREG__Stat__Not_Registered                              = 0, /**< Not registered and not searching */
  AT_CEREG__Stat__Registered_Home_Network                     = 1,
  AT_CEREG__Stat__Not_Registered_but_Searching                = 2,
  AT_CEREG__Stat__Registration_Denied                         = 3,
  AT_CEREG__Stat__Unknown                                     = 4, /**< E.g. out of coverage */
  AT_CEREG__Stat__Registered_Roaming                          = 5,
  AT_CEREG__Stat__Attached_for_Emergency_Bearer_Services_Only = 8,
} at_cereg_stat_e;

char *at_cereg_stat_text(at_cereg_stat_e code);

typedef enum {
  AT_CEREG__AcT__E_UTRAN    = 7,   /**< 4G */
  AT_CEREG__AcT__EC_GSM_IoT = 8,   /**<  A/Gb mode - Only ublox SARA-R410M-01B/02B */
  AT_CEREG__AcT__E_UTRAN_NB = 9,   /**<  NB-S1 mode - Only ublox SARA-R410M-01B/02B/52B, SARA-N4*/
  AT_CEREG__AcT__invalid    = 255, /**< Indicates that the current AcT is invalid */
} at_cereg_act_e;

char *at_cereg_act_text(at_cereg_act_e code);

typedef enum {
  AT_CEREG__Cause_Type__EMM_Cause                   = 0,
  AT_CEREG__Cause_Type__Manufacturer_Specific_Cause = 1,
} at_cereg_cause_type_e;

char *at_cereg_cause_type_text(at_cereg_cause_type_e code);



typedef enum {
  AT_CSQ__RSSI__0 = 0,                           /**< -113 dBm or less */
  AT_CSQ__RSSI__1 = 1,                           /**< -111 dBm */
  /* 2..30 */                                    /**< -109..-53 dBm */
  AT_CSQ__RSSI__31                         = 31, /**< -51 dBm or greater */
  AT_CSQ__RSSI__Not_Known_or_Detectable_99 = 99,

  AT_CSQ__RSSI__100 = 100,                        /**< -116 dBm or less */
  AT_CSQ__RSSI__101 = 101,                        /**< -115 dBm */
  /* 102..190 */                                  /**< -114..-26 dBm */
  AT_CSQ__RSSI__191                         = 31, /**< -25 dBm or greater */
  AT_CSQ__RSSI__Not_Known_or_Detectable_199 = 199,
  /* 100..199 - for use in TDSCDMA */
} at_csq_rssi_e;

typedef enum {
  /* Actual values depend on 2G/3G/LTE/etc, so check with 3GPP TS 45.008 */
  AT_CSQ__Qual__0                           = 0,
  AT_CSQ__Qual__1                           = 1,
  AT_CSQ__Qual__2                           = 2,
  AT_CSQ__Qual__3                           = 3,
  AT_CSQ__Qual__4                           = 4,
  AT_CSQ__Qual__5                           = 5,
  AT_CSQ__Qual__6                           = 6,
  AT_CSQ__Qual__7                           = 7,
  AT_CSQ__Qual__Not_Known_or_Not_Detectable = 99,
} at_csq_qual_e;



typedef enum {
  AT_USO_Protocol__none = 0,
  AT_USO_Protocol__TCP  = 6,
  AT_USO_Protocol__UDP  = 17,
} at_uso_protocol_e;

typedef enum {
  AT_USO_Error__Success                = 0,   /**< No Error */
  AT_USO_Error__EPERM                  = 1,   /**< Operation not permitted (internal error) */
  AT_USO_Error__ENOENT                 = 2,   /**< No such resource (internal error) */
  AT_USO_Error__EINTR                  = 4,   /**< Interrupted system call (internal error) */
  AT_USO_Error__EIO                    = 5,   /**< I/O error (internal error) */
  AT_USO_Error__EBADF                  = 9,   /**< Bad file descriptor (internal error) */
  AT_USO_Error__ECHILD                 = 10,  /**< No child processes (internal error) */
  AT_USO_Error__EWOULDBLOCK_EAGAIN     = 11,  /**< Current operation would block, try again */
  AT_USO_Error__ENOMEM                 = 12,  /**< Out of memory (internal error) */
  AT_USO_Error__EFAULT                 = 14,  /**< Bad address (internal error) */
  AT_USO_Error__EINVAL                 = 22,  /**< Invalid argument */
  AT_USO_Error__EPIPE                  = 32,  /**< Broken pipe (internal error) */
  AT_USO_Error__ENOSYS                 = 38,  /**< Function not implemented */
  AT_USO_Error__ENONET                 = 64,  /**< Machine is not on the internet */
  AT_USO_Error__EEOF                   = 65,  /**< End of file */
  AT_USO_Error__EPROTO                 = 71,  /**< Protocol error */
  AT_USO_Error__EBADFD                 = 77,  /**< File descriptor in bad state (internal error) */
  AT_USO_Error__EREMCHG                = 78,  /**< Remote address changed */
  AT_USO_Error__EDESTADDRREQ           = 89,  /**< Destination address required */
  AT_USO_Error__EPROTOTYPE             = 91,  /**< Wrong protocol type for socket */
  AT_USO_Error__ENOPROTOOPT            = 92,  /**< Protocol not available */
  AT_USO_Error__EPROTONOSUPPORT        = 93,  /**< Protocol not supported */
  AT_USO_Error__ESOCKTNNOSUPPORT       = 94,  /**< Socket type not supported */
  AT_USO_Error__EOPNOTSUPP             = 95,  /**< Operation not supported on transport endpoint */
  AT_USO_Error__EPFNOSUPPORT           = 96,  /**< Protocol family not supported */
  AT_USO_Error__EAFNOSUPPORT           = 97,  /**< Address family not supported by protocol */
  AT_USO_Error__EADDRINUSE             = 98,  /**< Address already in use */
  AT_USO_Error__EADDRNOTAVAIL          = 99,  /**< Cannot assign requested address */
  AT_USO_Error__ENETDOWN               = 100, /**< Network is down */
  AT_USO_Error__ENETUNREACH            = 101, /**< Network is unreachable */
  AT_USO_Error__ENETRESET              = 102, /**< Network dropped connection because of reset */
  AT_USO_Error__ECONNABORTED           = 103, /**< Software caused connection abort */
  AT_USO_Error__ECONNRESET             = 104, /**< Connection reset by peer */
  AT_USO_Error__ENOBUFS                = 105, /**< No buffer space available */
  AT_USO_Error__EISCONN                = 106, /**< Transport endpoint is already connected */
  AT_USO_Error__ENOTCONN               = 107, /**< Transport endpoint is not connected */
  AT_USO_Error__ESHUTDOWN              = 108, /**< Cannot send after transport endpoint  shutdown */
  AT_USO_Error__ETIMEDOUT              = 110, /**< Connection timed out */
  AT_USO_Error__ECONNREFUSED           = 111, /**< Connection refused */
  AT_USO_Error__EHOSTDOWN              = 112, /**< Host is down */
  AT_USO_Error__EHOSTUNREACH           = 113, /**< No route to host */
  AT_USO_Error__EINPROGRESS            = 115, /**< Operation now in progress */
  AT_USO_Error__ENSRNODATA             = 160, /**< DNS server returned answer with no data */
  AT_USO_Error__ENSRFORMERR            = 161, /**< DNS server claims query was misformatted */
  AT_USO_Error__ENSRSERVFAIL           = 162, /**< DNS server returned general failure */
  AT_USO_Error__ENSRNOTFOUND           = 163, /**< Domain name not found */
  AT_USO_Error__ENSRNOTIMP             = 164, /**< DNS server does not implement requested operation */
  AT_USO_Error__ENSRREFUSED            = 165, /**< DNS server refused query */
  AT_USO_Error__ENSRBADQUERY           = 166, /**< Misformatted DNS query */
  AT_USO_Error__ENSRBADNAME            = 167, /**< Misformatted domain name */
  AT_USO_Error__ENSRBADFAMILY          = 168, /**< Unsupported address family */
  AT_USO_Error__ENSRBADRESP            = 169, /**< Misformatted DNS reply */
  AT_USO_Error__ENSRCONNREFUSED        = 170, /**< Could not contact DNS servers */
  AT_USO_Error__ENSRTIMEOUT            = 171, /**< Timeout while contacting DNS servers */
  AT_USO_Error__ENSROF                 = 172, /**< End of file */
  AT_USO_Error__ENSRFILE               = 173, /**< Error reading file */
  AT_USO_Error__ENSRNOMEM              = 174, /**< Out of memory */
  AT_USO_Error__ENSRDESTRUCTION        = 175, /**< Application terminated lookup */
  AT_USO_Error__ENSRQUERYDOMAINTOOLONG = 176, /**< Domain name is too long */
  AT_USO_Error__ENSRCNAMELOOP          = 177, /**< Domain name is too long */
} at_uso_error_e;

char *at_uso_error_text(at_uso_error_e code);

#endif
