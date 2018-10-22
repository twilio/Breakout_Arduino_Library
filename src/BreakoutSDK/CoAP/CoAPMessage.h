/*
 * CoAPMessage.h
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
 * \file CoAP Message
 */

#ifndef __OWL_COAP_MESSAGE_H__
#define __OWL_COAP_MESSAGE_H__

#include "CoAPOption.h"
#include "enums.h"



/*
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |Ver| T |  TKL  |      Code     |          Message ID           |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |   Token (if any, TKL bytes) ...
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |   Options (if any) ...
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |1 1 1 1 1 1 1 1|    Payload (if any) ...
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */



typedef uint8_t coap_code_t; /**< Composed of class and detail, typically represented as c.dd */

typedef uint16_t coap_message_id_t;

typedef uint8_t coap_token_lenght_t; /**< 4-bit Token length - values 9-15 are considered format error */

typedef uint64_t coap_token_t;


class CoAPMessage {
 public:
  coap_version_e version = CoAP_Version__1;
  coap_type_e type       = CoAP_Type__Confirmable;

  coap_code_class_e code_class   = CoAP_Code_Class__Empty_Message;
  coap_code_detail_e code_detail = CoAP_Code_Detail__Empty_Message;

  coap_message_id_t message_id = 0;

  coap_token_lenght_t token_length = 0;
  coap_token_t token               = 0;

  CoAPOption *options = 0;

  str payload = {0};

  CoAPMessage();
  CoAPMessage(coap_type_e type, coap_code_class_e code_class, coap_code_detail_e code_detail,
              coap_message_id_t message_id);
  CoAPMessage(CoAPMessage *con_to_ack_or_reset, coap_type_e ack_or_reset);
  CoAPMessage(CoAPMessage *request_to_response, coap_type_e type, coap_code_class_e code_class,
              coap_code_detail_e code_detail);
  ~CoAPMessage();

  void destroy();

  void log(log_level_t level);
  int encode(bin_t *dst);
  int decode(bin_t *src);

  static int testCodec(CoAPMessage &msg, uint8_t *buffer, int len);


  CoAPOption *addOptionEmpty(coap_option_number_e number);
  CoAPOption *addOptionOpaque(coap_option_number_e number, str opaque);
  CoAPOption *addOptionUint(coap_option_number_e number, uint64_t uint);
  CoAPOption *addOptionString(coap_option_number_e number, char *string);
  CoAPOption *addOptionString(coap_option_number_e number, str string);

  CoAPOption *getNextOption(coap_option_number_e number, coap_option_format_e format, CoAPOption **iterator);
  CoAPOption *getNextOptionEmpty(coap_option_number_e number, CoAPOption **iterator);
  CoAPOption *getNextOptionOpaque(coap_option_number_e number, CoAPOption **iterator);
  CoAPOption *getNextOptionUint(coap_option_number_e number, CoAPOption **iterator);
  CoAPOption *getNextOptionString(coap_option_number_e number, CoAPOption **iterator);


  /*
   *  https://tools.ietf.org/html/rfc7252#section-5.10
   *    +-----+---+---+---+---+----------------+--------+--------+----------+
   *    | No. | C | U | N | R | Name           | Format | Length | Default  |
   *    +-----+---+---+---+---+----------------+--------+--------+----------+
   *    |   1 | x |   |   | x | If-Match       | opaque | 0-8    | (none)   |
   *    |   3 | x | x | - |   | Uri-Host       | string | 1-255  | (see     |
   *    |     |   |   |   |   |                |        |        | below)   |
   *    |   4 |   |   |   | x | ETag           | opaque | 1-8    | (none)   |
   *    |   5 | x |   |   |   | If-None-Match  | empty  | 0      | (none)   |
   *    |   7 | x | x | - |   | Uri-Port       | uint   | 0-2    | (see     |
   *    |     |   |   |   |   |                |        |        | below)   |
   *    |   8 |   |   |   | x | Location-Path  | string | 0-255  | (none)   |
   *    |  11 | x | x | - | x | Uri-Path       | string | 0-255  | (none)   |
   *    |  12 |   |   |   |   | Content-Format | uint   | 0-2    | (none)   |
   *    |  14 |   | x | - |   | Max-Age        | uint   | 0-4    | 60       |
   *    |  15 | x | x | - | x | Uri-Query      | string | 0-255  | (none)   |
   *    |  17 | x |   |   |   | Accept         | uint   | 0-2    | (none)   |
   *    |  20 |   |   |   | x | Location-Query | string | 0-255  | (none)   |
   *    |  35 | x | x | - |   | Proxy-Uri      | string | 1-1034 | (none)   |
   *    |  39 | x | x | - |   | Proxy-Scheme   | string | 1-255  | (none)   |
   *    |  60 |   |   | x |   | Size1          | uint   | 0-4    | (none)   |
   *    +-----+---+---+---+---+----------------+--------+--------+----------+
   *
   *https://tools.ietf.org/html/rfc7641#section-2
   *    +-----+---+---+---+---+---------+--------+--------+---------+
   *    | No. | C | U | N | R | Name    | Format | Length | Default |
   *    +-----+---+---+---+---+---------+--------+--------+---------+
   *    |   6 |   | x | - |   | Observe | uint   | 0-3 B  | (none)  |
   *    +-----+---+---+---+---+---------+--------+--------+---------+
   *
   *https://tools.ietf.org/html/rfc7959#section-2.1
   *    +-----+---+---+---+---+--------+--------+--------+---------+
   *    | No. | C | U | N | R | Name   | Format | Length | Default |
   *    +-----+---+---+---+---+--------+--------+--------+---------+
   *    |  23 | C | U | - | - | Block2 | uint   |    0-3 | (none)  |
   *    |     |   |   |   |   |        |        |        |         |
   *    |  27 | C | U | - | - | Block1 | uint   |    0-3 | (none)  |
   *    +-----+---+---+---+---+--------+--------+--------+---------+
   *https://tools.ietf.org/html/rfc7959#section-4
   *    +-----+---+---+---+---+-------+--------+--------+---------+
   *    | No. | C | U | N | R | Name  | Format | Length | Default |
   *    +-----+---+---+---+---+-------+--------+--------+---------+
   *    |  60 |   |   | x |   | Size1 | uint   |    0-4 | (none)  |
   *    |     |   |   |   |   |       |        |        |         |
   *    |  28 |   |   | x |   | Size2 | uint   |    0-4 | (none)  |
   *    +-----+---+---+---+---+-------+--------+--------+---------+
   *
   *https://tools.ietf.org/html/rfc7967#section-2
   *    +--------+---+---+---+---+-------------+--------+--------+---------+
   *    | Number | C | U | N | R |   Name      | Format | Length | Default |
   *    +--------+---+---+---+---+-------------+--------+--------+---------+
   *    |   258  |   | X | - |   | No-Response |  uint  |  0-1   |    0    |
   *    +--------+---+---+---+---+-------------+--------+--------+---------+
   *
   *              C=Critical, U=Unsafe, N=NoCacheKey, R=Repeatable
   */
  int addOptionIfMatch(str opaque);
  int getNextOptionIfMatch(str *out_opaque, CoAPOption **iterator);

  int addOptionUriHost(char *string);
  int addOptionUriHost(str string);
  int getNextOptionUriHost(str *out_string, CoAPOption **iterator);

  int addOptionETag(str opaque);
  int getNextOptionETag(str *out_opaque, CoAPOption **iterator);

  int addOptionIfNoneMatch();
  int getOptionIfNoneMatch();

  int addOptionUriPort(uint64_t uint);
  int getNextOptionUriPort(uint64_t *out_uint, CoAPOption **iterator);

  int addOptionLocationPath(char *string);
  int addOptionLocationPath(str string);
  int getNextOptionLocationPath(str *out_string, CoAPOption **iterator);

  int addOptionUriPath(char *string);
  int addOptionUriPath(str string);
  int getNextOptionUriPath(str *out_string, CoAPOption **iterator);

  int addOptionContentFormat(uint64_t uint);
  int getNextOptionContentFormat(uint64_t *out_uint, CoAPOption **iterator);

  int addOptionMaxAge(uint64_t uint);
  int getNextOptionMaxAge(uint64_t *out_uint, CoAPOption **iterator);

  int addOptionUriQuery(char *string);
  int addOptionUriQuery(str string);
  int getNextOptionUriQuery(str *out_string, CoAPOption **iterator);

  int addOptionAccept(uint64_t uint);
  int getNextOptionAccept(uint64_t *out_uint, CoAPOption **iterator);

  int addOptionLocationQuery(char *string);
  int addOptionLocationQuery(str string);
  int getNextOptionLocationQuery(str *out_string, CoAPOption **iterator);

  int addOptionProxyUri(char *string);
  int addOptionProxyUri(str string);
  int getNextOptionProxyUri(str *out_string, CoAPOption **iterator);

  int addOptionProxyScheme(char *string);
  int addOptionProxyScheme(str string);
  int getNextOptionProxyScheme(str *out_string, CoAPOption **iterator);

  int addOptionSize1(uint64_t uint);
  int getNextOptionSize1(uint64_t *out_uint, CoAPOption **iterator);

  int addOptionObserve(uint64_t uint);
  int getNextOptionObserve(uint64_t *out_uint, CoAPOption **iterator);

  int addOptionBlock2(uint64_t uint);
  int getNextOptionBlock2(uint64_t *out_uint, CoAPOption **iterator);

  int addOptionBlock1(uint64_t uint);
  int getNextOptionBlock1(uint64_t *out_uint, CoAPOption **iterator);

  int addOptionSize2(uint64_t uint);
  int getNextOptionSize2(uint64_t *out_uint, CoAPOption **iterator);

  int addOptionNoResponse(uint64_t uint);
  int getNextOptionNoResponse(uint64_t *out_uint, CoAPOption **iterator);

  int addOptionTwilioHostDeviceInformation(str opaque);
  int getNextOptionTwilioHostDeviceInformation(str *out_opaque, CoAPOption **iterator);

  int addOptionTwilioQueuedCommandCount(uint64_t uint);
  int getNextOptionTwilioQueuedCommandCount(uint64_t *out_uint, CoAPOption **iterator);
};

#endif
