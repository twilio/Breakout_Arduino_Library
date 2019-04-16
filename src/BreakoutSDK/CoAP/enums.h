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
 * \file OwlCoAPEnums.h - various enumerations for CoAP - https://tools.ietf.org/html/rfc7252
 * https://www.iana.org/assignments/core-parameters/core-parameters.xhtml#codes
 */

#ifndef __OWL_COAP_ENUMS_H__
#define __OWL_COAP_ENUMS_H__

#include "../utils/utils.h"



typedef enum {
  CoAP_Version__1 = 1,
} coap_version_e; /**< 2-bit Version */



typedef enum {
  CoAP_Type__Confirmable     = 0,
  CoAP_Type__Non_Confirmable = 1,
  CoAP_Type__Acknowledgement = 2,
  CoAP_Type__Reset           = 3,
} coap_type_e; /**< 2-bit Type */

char *coap_type_text(coap_type_e code);



typedef enum {
  CoAP_Code_Class__Empty_Message = 0,
  CoAP_Code_Class__Request       = 0,
  CoAP_Code_Class__Response      = 2,
  CoAP_Code_Class__Error         = 4,
  CoAP_Code_Class__Server_Error  = 5,
} coap_code_class_e; /**< 3-bit class in code */

char *coap_code_class_text(coap_code_class_e code);



typedef enum {
  CoAP_Code_Detail__Empty_Message = 0,

  /* Request - 0.xx */
  CoAP_Code_Detail__Request__GET    = 1,
  CoAP_Code_Detail__Request__POST   = 2,
  CoAP_Code_Detail__Request__PUT    = 3,
  CoAP_Code_Detail__Request__DELETE = 4,
  CoAP_Code_Detail__Request__FETCH  = 5,
  CoAP_Code_Detail__Request__PATCH  = 6,
  CoAP_Code_Detail__Request__iPATCH = 7,

  /* Response - 2.xx */
  /* 2.00 Unassigned */
  CoAP_Code_Detail__Response__Created = 1,
  CoAP_Code_Detail__Response__Deleted = 2,
  CoAP_Code_Detail__Response__Valid   = 3,
  CoAP_Code_Detail__Response__Changed = 4,
  CoAP_Code_Detail__Response__Content = 5,
  /* 2.06-2.30 Unassigned */
  CoAP_Code_Detail__Response__Continue = 31,

  /* Error - 4.xx */
  CoAP_Code_Detail__Error__Bad_Request        = 0,
  CoAP_Code_Detail__Error__Unauthorized       = 1,
  CoAP_Code_Detail__Error__Bad_Option         = 2,
  CoAP_Code_Detail__Error__Forbidden          = 3,
  CoAP_Code_Detail__Error__Not_Found          = 4,
  CoAP_Code_Detail__Error__Method_Not_Allowed = 5,
  CoAP_Code_Detail__Error__Not_Acceptable     = 6,
  /* 4.7 Unassigned */
  CoAP_Code_Detail__Error__Request_Entity_Incomplete = 8,
  CoAP_Code_Detail__Error__Conflict                  = 9,
  /* 4.10-4.11 Unassigned */
  CoAP_Code_Detail__Error__Precondition_Fail        = 12,
  CoAP_Code_Detail__Error__Request_Entity_Too_Large = 13,
  /* 4.14 Unassigned */
  CoAP_Code_Detail__Error__Unsupported_Content_Format = 15,
  /* 4.16-4.21 Unassigned */
  CoAP_Code_Detail__Error__Unprocessable_Entity = 22,
  /* 4.23-4.31 Unassigned */

  /* Server Error - 5.xx */
  CoAP_Code_Detail__Server_Error__Internal_Server_Error  = 0,
  CoAP_Code_Detail__Server_Error__Not_Implemented        = 1,
  CoAP_Code_Detail__Server_Error__Bad_Gateway            = 2,
  CoAP_Code_Detail__Server_Error__Service_Unavailable    = 3,
  CoAP_Code_Detail__Server_Error__Gateway_Timeout        = 4,
  CoAP_Code_Detail__Server_Error__Proxying_Not_Supported = 5,
  /* 5.06-5.31 Unassigned */

} coap_code_detail_e; /**< 5-bit detail in code */

char *coap_code_text(coap_code_class_e code_class, coap_code_detail_e code_detail);


typedef enum {
  CoAP_Code__Empty_Message = 0,

  CoAP_Code__Method__0_01_GET    = 1,
  CoAP_Code__Method__0_02_POST   = 2,
  CoAP_Code__Method__0_03_PUT    = 3,
  CoAP_Code__Method__0_04_DELETE = 4,
  CoAP_Code__Method__0_05_FETCH  = 5,
  CoAP_Code__Method__0_06_PATCH  = 6,
  CoAP_Code__Method__0_07_iPATCH = 7,
  /* 0.08-0.31 Unassigned */

  CoAP_Code__Response__2_00_Unassigned = 0x40,
  CoAP_Code__Response__2_01_Created    = 0x41,
  CoAP_Code__Response__2_02_Deleted    = 0x42,
  CoAP_Code__Response__2_03_Valid      = 0x43,
  CoAP_Code__Response__2_04_Changed    = 0x44,
  CoAP_Code__Response__2_05_Content    = 0x45,
  /* 2.06-2.30 Unassigned */
  CoAP_Code__Response__2_31_Continue = 0x5F,

  /* 3.00-3.31 Reserved */

  CoAP_Code__Error__4_00_Bad_Request               = 0x80,
  CoAP_Code__Error__4_01_Unauthorized              = 0x81,
  CoAP_Code__Error__4_02_Bad_Option                = 0x82,
  CoAP_Code__Error__4_03_Forbidden                 = 0x83,
  CoAP_Code__Error__4_04_Not_Found                 = 0x84,
  CoAP_Code__Error__4_05_Method_Not_Allowed        = 0x85,
  CoAP_Code__Error__4_06_Not_Acceptable            = 0x86,
  CoAP_Code__Error__4_07_Unassigned                = 0x87,
  CoAP_Code__Error__4_08_Request_Entity_Incomplete = 0x88,
  CoAP_Code__Error__4_09_Conflict                  = 0x89,
  /* 4.10-4.11 Unassigned */
  CoAP_Code__Error__4_12_Precondition_Fail        = 0x8c,
  CoAP_Code__Error__4_13_Request_Entity_Too_Large = 0x8d,
  /* 4.14 Unassigned */
  CoAP_Code__Error__4_15_Unsupported_Content_Format = 0x8f,
  /* 4.16-4.21 Unassigned */
  CoAP_Code__Error__4_22_Unprocessable_Entity = 0x96,
  /* 4.23-4.31 Unassigned */

  CoAP_Code__Server_Error__5_00_Internal_Server_Error  = 0xa0,
  CoAP_Code__Server_Error__5_01_Not_Implemented        = 0xa1,
  CoAP_Code__Server_Error__5_02_Bad_Gateway            = 0xa2,
  CoAP_Code__Server_Error__5_03_Service_Unavailable    = 0xa3,
  CoAP_Code__Server_Error__5_04_Gateway_Timeout        = 0xa4,
  CoAP_Code__Server_Error__5_05_Proxying_Not_Supported = 0xa5,
  /* 5.06-5.31 Unassigned */

} coap_code_e; /**< Composed code class.detail */



typedef enum {
  CoAP_Option__unknown = 0, /**< [RFC7252] */

  CoAP_Option__If_Match                          = 1,    /**< [RFC7252] */
  CoAP_Option__Uri_Host                          = 3,    /**< [RFC7252] */
  CoAP_Option__ETag                              = 4,    /**< [RFC7252] */
  CoAP_Option__If_None_Match                     = 5,    /**< [RFC7252] */
  CoAP_Option__Observe                           = 6,    /**< [RFC7641] */
  CoAP_Option__Uri_Port                          = 7,    /**< [RFC7252] */
  CoAP_Option__Location_Path                     = 8,    /**< [RFC7252] */
  CoAP_Option__Uri_Path                          = 11,   /**< [RFC7252] */
  CoAP_Option__Content_Format                    = 12,   /**< [RFC7252] */
  CoAP_Option__Max_Age                           = 14,   /**< [RFC7252] */
  CoAP_Option__Uri_Query                         = 15,   /**< [RFC7252] */
  CoAP_Option__Accept                            = 17,   /**< [RFC7252] */
  CoAP_Option__Location_Query                    = 20,   /**< [RFC7252] */
  CoAP_Option__Block2                            = 23,   /**< [RFC7959][RFC8323] */
  CoAP_Option__Block1                            = 27,   /**< [RFC7959][RFC8323] */
  CoAP_Option__Size2                             = 28,   /**< [RFC7959] */
  CoAP_Option__Proxy_Uri                         = 35,   /**< [RFC7252] */
  CoAP_Option__Proxy_Scheme                      = 39,   /**< [RFC7252] */
  CoAP_Option__Size1                             = 60,   /**< [RFC7252] */
  CoAP_Option__No_Response                       = 258,  /**< [RFC7967] */
  CoAP_Option__OCF_Accept_Content_Format_Version = 2049, /**< [Michael_Koster] */
  CoAP_Option__OCF_Content_Format_Version        = 2053, /**< [Michael_Koster] */
  /* Twilio - 2048..64999 is reserved for private use */
  CoAP_Option__Twilio_Queued_Command_Count   = 50000, /**< Twilio proprietary */
  CoAP_Option__Twilio_HostDevice_Information = 50001, /**< Twilio proprietary */
  /* 65000-65535 Reserved for Experimental Use [RFC7252] */
} coap_option_number_e;

char *coap_option_number_text(coap_option_number_e option_number);


typedef enum {
  CoAP_Content_Format__text_plain_charset_utf8                  = 0,  /**< [RFC2046][RFC3676][RFC5147] */
  CoAP_Content_Format__application_cose_cose_type_cose_encrypt0 = 16, /**< [RFC8152]*/
  CoAP_Content_Format__application_cose_cose_type_cose_mac0     = 17, /**< [RFC8152]*/
  CoAP_Content_Format__application_cose_cose_type_cose_sign1    = 18, /**< [RFC8152]*/
  CoAP_Content_Format__application_link_format                  = 40, /**< [RFC6690]*/
  CoAP_Content_Format__application_xml                          = 41, /**< [RFC3023]*/
  CoAP_Content_Format__application_octet_stream                 = 42, /**< [RFC2045][RFC2046]*/
  CoAP_Content_Format__application_exi =
      47, /**< [Efficient XML Interchange (EXI) Format 1.0 (Second Edition), February 2014]*/
  CoAP_Content_Format__application_json             = 50, /**< [RFC4627]*/
  CoAP_Content_Format__application_json_patch_json  = 51, /**< [RFC6902]*/
  CoAP_Content_Format__application_merge_patch_json = 52, /**< [RFC7396]*/
  CoAP_Content_Format__application_cbor             = 60, /**< [RFC7049]*/
  CoAP_Content_Format__application_cwt              = 61, /**< [RFC8392]*/
  CoAP_Content_Format__application_multipart_core =
      62, /**<   [draft-ietf-core-multipart-ct] - (TEMPORARY - registered 2018-09-05, expires 2019-09-05)*/
  CoAP_Content_Format__application_cose_cose_type_cose_encrypt = 96,  /**< [RFC8152]*/
  CoAP_Content_Format__application_cose_cose_type_cose_mac     = 97,  /**< [RFC8152]*/
  CoAP_Content_Format__application_cose_cose_type_cose_sign    = 98,  /**< [RFC8152]*/
  CoAP_Content_Format__application_cose_key                    = 101, /**< [RFC8152]*/
  CoAP_Content_Format__application_cose_key_set                = 102, /**< [RFC8152]*/
  CoAP_Content_Format__application_senml_json                  = 110, /**< [RFC8428]*/
  CoAP_Content_Format__application_sensml_json                 = 111, /**< [RFC8428]*/
  CoAP_Content_Format__application_senml_cbor                  = 112, /**< [RFC8428]*/
  CoAP_Content_Format__application_sensml_cbor                 = 113, /**< [RFC8428]*/
  CoAP_Content_Format__application_senml_exi                   = 114, /**< [RFC8428]*/
  CoAP_Content_Format__application_sensml_exi                  = 115, /**< [RFC8428]*/
  CoAP_Content_Format__application_coap_group_json             = 256, /**< [RFC7390]*/
  CoAP_Content_Format__application_pkcs7_mime_smime_type_server_generated_key =
      280, /**< [RFC-ietf-lamps-rfc5751-bis-12][RFC7030][draft-ietf-ace-coap-est] (TEMPORARY - registered 2018-08-09,
              expires 2019-08-09)*/
  CoAP_Content_Format__application_pkcs7_mime_smime_type_certs_only =
      281, /**< [RFC-ietf-lamps-rfc5751-bis-12][draft-ietf-ace-coap-est] (TEMPORARY - registered 2018-08-09, expires
              2019-08-09)*/
  CoAP_Content_Format__application_pkcs7_mime_smime_type_CMC_Request =
      282, /**< [RFC-ietf-lamps-rfc5751-bis-12][RFC5273][draft-ietf-ace-coap-est] (TEMPORARY - registered 2018-08-09,
              expires 2019-08-09)*/
  CoAP_Content_Format__application_pkcs7_mime_smime_type_CMC_Response =
      283, /**< [RFC-ietf-lamps-rfc5751-bis-12][RFC5273][draft-ietf-ace-coap-est] (TEMPORARY - registered 2018-08-09,
              expires 2019-08-09)*/
  CoAP_Content_Format__application_pkcs8 = 284, /**< [RFC-ietf-lamps-rfc5751-bis-12][RFC5958][draft-ietf-ace-coap-est]
                                                   (TEMPORARY - registered 2018-08-09, expires 2019-08-09)*/
  CoAP_Content_Format__application_csrattrs =
      285, /**< [RFC7030][draft-ietf-ace-coap-est] (TEMPORARY - registered 2018-08-09, expires 2019-08-09)*/
  CoAP_Content_Format__application_pkcs10 = 286, /**< [RFC-ietf-lamps-rfc5751-bis-12][RFC5967][draft-ietf-ace-coap-est]
                                                    (TEMPORARY - registered 2018-08-09, expires 2019-08-09)*/
  CoAP_Content_Format__application_senml_xml          = 310,   /**< [RFC8428]*/
  CoAP_Content_Format__application_sensml_xml         = 311,   /**< [RFC8428]*/
  CoAP_Content_Format__application_vnd_ocf_cbor       = 10000, /**< [Michael_Koster]*/
  CoAP_Content_Format__application_vnd_oma_lwm2m_tlv  = 11542, /**< [OMA-TS-LightweightM2M-V1_0]*/
  CoAP_Content_Format__application_vnd_oma_lwm2m_json = 11543, /**< [OMA-TS-LightweightM2M-V1_0]*/
} coap_content_format_e;

char *coap_content_format_text(coap_content_format_e content_format);


#endif
