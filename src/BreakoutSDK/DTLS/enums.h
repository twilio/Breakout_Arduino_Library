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
 * \file DTLSEnums.h - various enumerations for DTLS commands and parameters
 */

#ifndef __OWL_DTLS_ENUMS_H__
#define __OWL_DTLS_ENUMS_H__


#include "../utils/utils.h"



typedef enum {
  DTLS_Alert_Description__warning = 1,
  DTLS_Alert_Description__fatal   = 2,
} dtls_alert_level_e;

char *dtls_alert_level_text(dtls_alert_level_e code);



typedef enum {
  DTLS_Alert_Description__close_notify                = 0,
  DTLS_Alert_Description__unexpected_message          = 1,
  DTLS_Alert_Description__bad_record_mac              = 20,
  DTLS_Alert_Description__decryption_failed_RESERVED  = 21,
  DTLS_Alert_Description__record_overflow             = 22,
  DTLS_Alert_Description__decompression_failure       = 30,
  DTLS_Alert_Description__handshake_failure           = 40,
  DTLS_Alert_Description__no_certificate_RESERVED     = 41,
  DTLS_Alert_Description__bad_certificate             = 42,
  DTLS_Alert_Description__unsupported_certificate     = 43,
  DTLS_Alert_Description__certificate_revoked         = 44,
  DTLS_Alert_Description__certificate_expired         = 45,
  DTLS_Alert_Description__certificate_unknown         = 46,
  DTLS_Alert_Description__illegal_parameter           = 47,
  DTLS_Alert_Description__unknown_ca                  = 48,
  DTLS_Alert_Description__access_denied               = 49,
  DTLS_Alert_Description__decode_error                = 50,
  DTLS_Alert_Description__decrypt_error               = 51,
  DTLS_Alert_Description__export_restriction_RESERVED = 60,
  DTLS_Alert_Description__protocol_version            = 70,
  DTLS_Alert_Description__insufficient_security       = 71,
  DTLS_Alert_Description__internal_error              = 80,
  DTLS_Alert_Description__user_canceled               = 90,
  DTLS_Alert_Description__no_renegotiation            = 100,
  DTLS_Alert_Description__unsupported_extension       = 110,

  /* tinydtls proprietary and internal */
  DTLS_Alert_Description__tinydtls_event_connect     = 0x01DC,
  DTLS_Alert_Description__tinydtls_event_connected   = 0x01DE,
  DTLS_Alert_Description__tinydtls_event_renegotiate = 0x01DF,
} dtls_alert_description_e;

char *dtls_alert_description_text(dtls_alert_description_e code);

#endif
