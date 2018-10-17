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
 * \file DTLSEnums.cpp - various enumerations for AT commands and parameters
 */

#include "enums.h"



struct {
  char *value;
  dtls_alert_level_e code;
} dtls_alert_levels[] = {
    {.value = "warning", .code = DTLS_Alert_Description__warning},
    {.value = "fatal", .code = DTLS_Alert_Description__fatal},
    {.value = 0, .code = (dtls_alert_level_e)0},
};

char *dtls_alert_level_text(dtls_alert_level_e code) {
  int i;
  for (i = 0; dtls_alert_levels[i].value != 0; i++)
    if (dtls_alert_levels[i].code == code) return dtls_alert_levels[i].value;
  return "<unknown>";
}



struct {
  char *value;
  dtls_alert_description_e code;
} dtls_alert_descriptions[] = {
    {.value = "close_notify", .code = DTLS_Alert_Description__close_notify},
    {.value = "unexpected_message", .code = DTLS_Alert_Description__unexpected_message},
    {.value = "bad_record_mac", .code = DTLS_Alert_Description__bad_record_mac},
    {.value = "decryption_failed_RESERVED", .code = DTLS_Alert_Description__decryption_failed_RESERVED},
    {.value = "record_overflow", .code = DTLS_Alert_Description__record_overflow},
    {.value = "decompression_failure", .code = DTLS_Alert_Description__decompression_failure},
    {.value = "handshake_failure", .code = DTLS_Alert_Description__handshake_failure},
    {.value = "no_certificate_RESERVED", .code = DTLS_Alert_Description__no_certificate_RESERVED},
    {.value = "bad_certificate", .code = DTLS_Alert_Description__bad_certificate},
    {.value = "unsupported_certificate", .code = DTLS_Alert_Description__unsupported_certificate},
    {.value = "certificate_revoked", .code = DTLS_Alert_Description__certificate_revoked},
    {.value = "certificate_expired", .code = DTLS_Alert_Description__certificate_expired},
    {.value = "certificate_unknown", .code = DTLS_Alert_Description__certificate_unknown},
    {.value = "illegal_parameter", .code = DTLS_Alert_Description__illegal_parameter},
    {.value = "unknown_ca", .code = DTLS_Alert_Description__unknown_ca},
    {.value = "access_denied", .code = DTLS_Alert_Description__access_denied},
    {.value = "decode_error", .code = DTLS_Alert_Description__decode_error},
    {.value = "decrypt_error", .code = DTLS_Alert_Description__decrypt_error},
    {.value = "export_restriction_RESERVED", .code = DTLS_Alert_Description__export_restriction_RESERVED},
    {.value = "protocol_version", .code = DTLS_Alert_Description__protocol_version},
    {.value = "insufficient_security", .code = DTLS_Alert_Description__insufficient_security},
    {.value = "internal_error", .code = DTLS_Alert_Description__internal_error},
    {.value = "user_canceled", .code = DTLS_Alert_Description__user_canceled},
    {.value = "no_renegotiation", .code = DTLS_Alert_Description__no_renegotiation},
    {.value = "unsupported_extension", .code = DTLS_Alert_Description__unsupported_extension},

    /* tinydtls proprietary and internal */
    {.value = "tinydtls_event_connect", .code = DTLS_Alert_Description__tinydtls_event_connect},
    {.value = "tinydtls_event_connected", .code = DTLS_Alert_Description__tinydtls_event_connected},
    {.value = "tinydtls_event_renegotiate", .code = DTLS_Alert_Description__tinydtls_event_renegotiate},

    {.value = 0, .code = (dtls_alert_description_e)0},
};

char *dtls_alert_description_text(dtls_alert_description_e code) {
  int i;
  for (i = 0; dtls_alert_descriptions[i].value != 0; i++)
    if (dtls_alert_descriptions[i].code == code) return dtls_alert_descriptions[i].value;
  return "<unknown>";
}
