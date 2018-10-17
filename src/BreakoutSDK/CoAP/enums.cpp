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
 * \file OwlCoAPEnums.h - various enumerations for CoAP - https://tools.ietf.org/html/rfc7252
 * https://www.iana.org/assignments/core-parameters/core-parameters.xhtml#codes
 */

#include "enums.h"


struct {
  char *value;
  coap_type_e code;
} coap_types[] = {
    {.value = "Confirmable", .code = CoAP_Type__Confirmable},
    {.value = "Non-Confirmable", .code = CoAP_Type__Non_Confirmable},
    {.value = "Acknowledgement", .code = CoAP_Type__Acknowledgement},
    {.value = "Reset", .code = CoAP_Type__Reset},
    {.value = 0, .code = (coap_type_e)0},
};

char *coap_type_text(coap_type_e type) {
  int i;
  for (i = 0; coap_types[i].value != 0; i++)
    if (coap_types[i].code == type) return coap_types[i].value;
  return "<unknown>";
}



struct {
  char *value;
  coap_code_class_e code;
} coap_code_classes[] = {
    {.value = "Request", .code = CoAP_Code_Class__Request},
    {.value = "Response", .code = CoAP_Code_Class__Response},
    {.value = "Error", .code = CoAP_Code_Class__Error},
    {.value = "Server_Error", .code = CoAP_Code_Class__Server_Error},
    {.value = 0, .code = (coap_code_class_e)0},
};

char *coap_code_class_text(coap_code_class_e code_class) {
  int i;
  for (i = 0; coap_code_classes[i].value != 0; i++)
    if (coap_code_classes[i].code == code_class) return coap_code_classes[i].value;
  return "<unknown>";
}



char *coap_code_text(coap_code_class_e code_class, coap_code_detail_e code_detail) {
  int i;
  switch (code_class) {
    case CoAP_Code_Class__Request:  // Same as CoAP_Code_Class__Empty_Message
      switch (code_detail) {
        case CoAP_Code_Detail__Empty_Message:  // For CoAP_Code_Class__Empty_Message
          return "Empty-Message";
        case CoAP_Code_Detail__Request__GET:
          return "Request.GET";
        case CoAP_Code_Detail__Request__POST:
          return "Request.POST";
        case CoAP_Code_Detail__Request__PUT:
          return "Request.PUT";
        case CoAP_Code_Detail__Request__DELETE:
          return "Request.DELETE";
        case CoAP_Code_Detail__Request__FETCH:
          return "Request.Fetch";
        case CoAP_Code_Detail__Request__PATCH:
          return "Request.PATCH";
        case CoAP_Code_Detail__Request__iPATCH:
          return "Request.iPATCH";
        default:
          return "Request.<unknown-code-detail>";
      }
      break;
    case CoAP_Code_Class__Response:
      switch (code_detail) {
        case CoAP_Code_Detail__Response__Created:
          return "Response.Created";
        case CoAP_Code_Detail__Response__Deleted:
          return "Response.Deleted";
        case CoAP_Code_Detail__Response__Valid:
          return "Response.Valid";
        case CoAP_Code_Detail__Response__Changed:
          return "Response.Changed";
        case CoAP_Code_Detail__Response__Content:
          return "Response.Content";
        case CoAP_Code_Detail__Response__Continue:
          return "Response.Continue";
        default:
          return "Response.<unknown-code-detail>";
      }
      break;
    case CoAP_Code_Class__Error:
      switch (code_detail) {
        case CoAP_Code_Detail__Error__Bad_Request:
          return "Error.Bad-Request";
        case CoAP_Code_Detail__Error__Unauthorized:
          return "Error.Unauthorized";
        case CoAP_Code_Detail__Error__Bad_Option:
          return "Error.Bad-Option";
        case CoAP_Code_Detail__Error__Forbidden:
          return "Error.Forbidden";
        case CoAP_Code_Detail__Error__Not_Found:
          return "Error.Not-Found";
        case CoAP_Code_Detail__Error__Method_Not_Allowed:
          return "Method.Not-Allowed";
        case CoAP_Code_Detail__Error__Request_Entity_Incomplete:
          return "Error.Request-Entity-Incomplete";
        case CoAP_Code_Detail__Error__Conflict:
          return "Error.Conflict";
        case CoAP_Code_Detail__Error__Precondition_Fail:
          return "Error.Precondition-Fail";
        case CoAP_Code_Detail__Error__Request_Entity_Too_Large:
          return "Error.Request-Entity-Too-Large";
        case CoAP_Code_Detail__Error__Unsupported_Content_Format:
          return "Error.Unsupported-Content-Format";
        case CoAP_Code_Detail__Error__Unprocessable_Entity:
          return "Error.Unprocessable-Entity";
        default:
          return "Error.<unknown-code-detail>";
      }
      break;
    case CoAP_Code_Class__Server_Error:
      switch (code_detail) {
        case CoAP_Code_Detail__Server_Error__Internal_Server_Error:
          return "Server-Error.Internal-Server-Error";
        case CoAP_Code_Detail__Server_Error__Not_Implemented:
          return "Server-Error.Not-Implemented";
        case CoAP_Code_Detail__Server_Error__Bad_Gateway:
          return "Server-Error.Bad-Gateway";
        case CoAP_Code_Detail__Server_Error__Service_Unavailable:
          return "Server-Error.Service-Unavailable";
        case CoAP_Code_Detail__Server_Error__Gateway_Timeout:
          return "Server-Error.Gateway-Timeout";
        case CoAP_Code_Detail__Server_Error__Proxying_Not_Supported:
          return "Server-Error.Proxying-Not-Supported";

        default:
          return "Server-Error.<unknown-code-detail>";
      }
      break;
    default:
      return "<unknown-code-class>";
  }
}



struct {
  char *value;
  coap_option_number_e code;
} coap_option_numbers[] = {
    {.value = "If-Match", .code = CoAP_Option__If_Match},
    {.value = "Uri-Host", .code = CoAP_Option__Uri_Host},
    {.value = "ETag", .code = CoAP_Option__ETag},
    {.value = "If-None-Match", .code = CoAP_Option__If_None_Match},
    {.value = "Observe", .code = CoAP_Option__Observe},
    {.value = "Uri-Port", .code = CoAP_Option__Uri_Port},
    {.value = "Location-Path", .code = CoAP_Option__Location_Path},
    {.value = "Uri-Path", .code = CoAP_Option__Uri_Path},
    {.value = "Content-Format", .code = CoAP_Option__Content_Format},
    {.value = "Max-Age", .code = CoAP_Option__Max_Age},
    {.value = "Uri-Query", .code = CoAP_Option__Uri_Query},
    {.value = "Accept", .code = CoAP_Option__Accept},
    {.value = "Location-Query", .code = CoAP_Option__Location_Query},
    {.value = "Block2", .code = CoAP_Option__Block2},
    {.value = "Block1", .code = CoAP_Option__Block1},
    {.value = "Size2", .code = CoAP_Option__Size2},
    {.value = "Proxy-Uri", .code = CoAP_Option__Proxy_Uri},
    {.value = "Proxy-Scheme", .code = CoAP_Option__Proxy_Scheme},
    {.value = "Size1", .code = CoAP_Option__Size1},
    {.value = "No-Response", .code = CoAP_Option__No_Response},
    {.value = "OCF-Accept-Content-Format-Version", .code = CoAP_Option__OCF_Accept_Content_Format_Version},
    {.value = "OCF-Content-Format-Version", .code = CoAP_Option__OCF_Content_Format_Version},
    {.value = "Twilio-Queued-Command-Count", .code = CoAP_Option__Twilio_Queued_Command_Count},
    {.value = "Twilio-HostDevice-Information", .code = CoAP_Option__Twilio_HostDevice_Information},
    {.value = 0, .code = (coap_option_number_e)0},
};

char *coap_option_number_text(coap_option_number_e option_number) {
  int i;
  for (i = 0; coap_option_numbers[i].value != 0; i++)
    if (coap_option_numbers[i].code == option_number) return coap_option_numbers[i].value;
  return "<unknown>";
}



struct {
  char *value;
  coap_content_format_e code;
} coap_content_formats[] = {
    {.value = "text/plain; charset=utf-8", CoAP_Content_Format__text_plain_charset_utf8},
    {.value = "application/cose; cose-type=\"cose-encrypt0\"",
     CoAP_Content_Format__application_cose_cose_type_cose_encrypt0},
    {.value = "application/cose; cose-type=\"cose-mac0\"", CoAP_Content_Format__application_cose_cose_type_cose_mac0},
    {.value = "application/cose; cose-type=\"cose-sign1\"", CoAP_Content_Format__application_cose_cose_type_cose_sign1},
    {.value = "application/link-format", CoAP_Content_Format__application_link_format},
    {.value = "application/xml", CoAP_Content_Format__application_xml},
    {.value = "application/octet-stream", CoAP_Content_Format__application_octet_stream},
    {.value = "application/exi", CoAP_Content_Format__application_exi},
    {.value = "application/json", CoAP_Content_Format__application_json},
    {.value = "application/json-patch+json", CoAP_Content_Format__application_json_patch_json},
    {.value = "application/merge-patch+json", CoAP_Content_Format__application_merge_patch_json},
    {.value = "application/cbor", CoAP_Content_Format__application_cbor},
    {.value = "application/cwt", CoAP_Content_Format__application_cwt},
    {.value = "application/multipart-core", CoAP_Content_Format__application_multipart_core},
    {.value = "application/cose; cose-type=\"cose-encrypt\"",
     CoAP_Content_Format__application_cose_cose_type_cose_encrypt},
    {.value = "application/cose; cose-type=\"cose-mac\"", CoAP_Content_Format__application_cose_cose_type_cose_mac},
    {.value = "application/cose; cose-type=\"cose-sign\"", CoAP_Content_Format__application_cose_cose_type_cose_sign},
    {.value = "application/cose-key", CoAP_Content_Format__application_cose_key},
    {.value = "application/cose-key-set", CoAP_Content_Format__application_cose_key_set},
    {.value = "application/senml+json", CoAP_Content_Format__application_senml_json},
    {.value = "application/sensml+json", CoAP_Content_Format__application_sensml_json},
    {.value = "application/senml+cbor", CoAP_Content_Format__application_senml_cbor},
    {.value = "application/sensml+cbor", CoAP_Content_Format__application_sensml_cbor},
    {.value = "application/senml-exi", CoAP_Content_Format__application_senml_exi},
    {.value = "application/sensml-exi", CoAP_Content_Format__application_sensml_exi},
    {.value = "application/coap-group+json", CoAP_Content_Format__application_coap_group_json},
    {.value = "application/pkcs7-mime; smime-type=server-generated-key",
     CoAP_Content_Format__application_pkcs7_mime_smime_type_server_generated_key},
    {.value = "application/pkcs7-mime; smime-type=certs-only",
     CoAP_Content_Format__application_pkcs7_mime_smime_type_certs_only},
    {.value = "application/pkcs7-mime; smime-type=CMC-Request",
     CoAP_Content_Format__application_pkcs7_mime_smime_type_CMC_Request},
    {.value = "application/pkcs7-mime; smime-type=CMC-Response",
     CoAP_Content_Format__application_pkcs7_mime_smime_type_CMC_Response},
    {.value = "application/pkcs8", CoAP_Content_Format__application_pkcs8},
    {.value = "application/csrattrs", CoAP_Content_Format__application_csrattrs},
    {.value = "application/pkcs10", CoAP_Content_Format__application_pkcs10},
    {.value = "application/senml+xml", CoAP_Content_Format__application_senml_xml},
    {.value = "application/sensml+xml", CoAP_Content_Format__application_sensml_xml},
    {.value = "application/vnd.ocf+cbor", CoAP_Content_Format__application_vnd_ocf_cbor},
    {.value = "application/vnd.oma.lwm2m+tlv", CoAP_Content_Format__application_vnd_oma_lwm2m_tlv},
    {.value = "application/vnd.oma.lwm2m+json", CoAP_Content_Format__application_vnd_oma_lwm2m_json},
    {.value = 0, .code = (coap_content_format_e)0},
};

char *coap_content_format_text(coap_content_format_e content_format) {
  int i;
  for (i = 0; coap_content_formats[i].value != 0; i++)
    if (coap_content_formats[i].code == content_format) return coap_content_formats[i].value;
  return "<unknown>";
}
