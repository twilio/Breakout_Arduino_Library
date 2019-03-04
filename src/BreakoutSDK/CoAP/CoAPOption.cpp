/*
 * CoAPOption.cpp
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
 * \file CoAP Option
 */

#include "CoAPOption.h"



CoAPOption::CoAPOption() : number(CoAP_Option__unknown), format(CoAP_Option_Format__empty), value({0}), next(0) {
}

CoAPOption::CoAPOption(coap_option_number_e number)
    : number(number), format(CoAP_Option_Format__empty), value({0}), next(0) {
}

CoAPOption::CoAPOption(coap_option_number_e number, str opaque)
    : number(number), format(CoAP_Option_Format__opaque), next(0) {
  this->value.opaque = opaque;
}

CoAPOption::CoAPOption(coap_option_number_e number, uint64_t uint)
    : number(number), format(CoAP_Option_Format__uint), next(0) {
  this->value.uint = uint;
}

CoAPOption::CoAPOption(coap_option_number_e number, char *string)
    : number(number), format(CoAP_Option_Format__string), next(0) {
  this->value.string.s   = string;
  this->value.string.len = strlen(string);
}

CoAPOption::~CoAPOption() {
}

void CoAPOption::log(log_level_t level) {
  switch (this->format) {
    case CoAP_Option_Format__empty:
      LOGF(level, " - CoAPOption %d (%s): (empty)\r\n", this->number, coap_option_number_text(this->number));
      break;
    case CoAP_Option_Format__opaque:
      LOGF(level, " - CoAPOption %d (%s):\r\n", this->number, coap_option_number_text(this->number));
      LOGSTR(level, this->value.opaque);
      break;
    case CoAP_Option_Format__uint:
      switch (this->number) {
        case CoAP_Option__Content_Format:
          LOGF(level, " - CoAPOption %d (%s): %llu - %s\r\n", this->number, coap_option_number_text(this->number),
               this->value.uint, coap_content_format_text((coap_content_format_e) this->value.uint));
          break;
        default:
          LOGF(level, " - CoAPOption %d (%s): %llu\r\n", this->number, coap_option_number_text(this->number),
               this->value.uint);
          break;
      }
      break;
    case CoAP_Option_Format__string:
      LOGF(level, " - CoAPOption %d (%s): [%.*s]\r\n", this->number, coap_option_number_text(this->number),
           this->value.string.len, this->value.string.s);
      break;
    default:
      break;
  }
}


int CoAPOption::encode(coap_option_number_e previous_number, bin_t *dst) {
  if (!dst) {
    LOG(L_ERROR, "Null parameter(s)\r\n");
    return 0;
  }
  int delta = this->number - previous_number;
  int len   = 0;
  uint8_t *first_byte;
  if (delta < 0) {
    LOG(L_ERROR, "Invalid order of calling this function previous %d > this %d - the option numbers must be ordered\r\n",
        previous_number, this->number);
    goto error;
  }
  switch (this->format) {
    case CoAP_Option_Format__empty:
      break;
    case CoAP_Option_Format__opaque:
      len = this->value.opaque.len;
      break;
    case CoAP_Option_Format__uint:
      if (this->value.uint == 0)
        len = 0;
      else if (this->value.uint <= 0xFFu)
        len = 1;
      else if (this->value.uint <= 0xFFFFu)
        len = 2;
      else if (this->value.uint <= 0xFFFFFFu)
        len = 3;
      else if (this->value.uint <= 0xFFFFFFFFu)
        len = 4;
      else if (this->value.uint <= 0xFFFFFFFFFFu)
        len = 5;
      else if (this->value.uint <= 0xFFFFFFFFFFFFu)
        len = 6;
      else if (this->value.uint <= 0xFFFFFFFFFFFFFFu)
        len = 7;
      else
        len = 8;
      break;
    case CoAP_Option_Format__string:
      len = this->value.string.len;
      break;
    default:
      LOG(L_ERROR, "Not implemented for format %d\r\n", this->format);
      goto error;
  }

  first_byte = dst->s + dst->idx;
  bin_t_encode_uint8(dst, 0);

  /* Option Delta */
  if (delta <= 12) {
    *first_byte = delta << 4;
  } else if (delta <= 13 + 255) {
    *first_byte = 13 << 4;
    bin_t_encode_uint8(dst, delta - 13);
  } else if (delta <= 269 + 65535) {
    *first_byte = 14 << 4;
    bin_t_encode_uint16(dst, delta - 269);
  } else {
    LOG(L_ERROR, "Can not encode delta %d > %d\r\n", delta, 269 + 65335);
    goto error;
  }

  /* Option Length */
  if (len < 0) {
    LOG(L_ERROR, "Invalid length %d\r\n", len);
    goto error;
  }
  if (len <= 12) {
    *first_byte |= len;
  } else if (len <= 13 + 255) {
    *first_byte |= 13;
    bin_t_encode_uint8(dst, len - 13);
  } else if (len <= 269 + 65535) {
    *first_byte |= 14;
    bin_t_encode_uint16(dst, len - 269);
  } else {
    LOG(L_ERROR, "Can not encode len %d > %d\r\n", len, 269 + 65335);
    goto error;
  }

  /* Value */
  switch (this->format) {
    case CoAP_Option_Format__empty:
      break;
    case CoAP_Option_Format__opaque:
      bin_t_encode_mem(dst, this->value.opaque.s, this->value.opaque.len);
      break;
    case CoAP_Option_Format__uint:
      if (len) bin_t_encode_varuint(dst, this->value.uint, len);
      break;
    case CoAP_Option_Format__string:
      bin_t_encode_mem(dst, this->value.string.s, this->value.string.len);
      break;
    default:
      LOG(L_ERROR, "Not implemented for format %d\r\n", this->format);
      goto error;
  }

  return 1;
bad_length:
error:
  return 0;
}

int CoAPOption::decode(coap_option_number_e previous_number, bin_t *src) {
  int delta          = 0;
  int len            = 0;
  uint8_t first_byte = bin_t_decode_uint8(src);

  /* Option Delta */
  delta = first_byte >> 4;
  if (delta <= 12) {
  } else if (delta == 13) {
    delta = bin_t_decode_uint8(src) + 13;
  } else if (delta == 14) {
    delta = bin_t_decode_uint16(src) + 269;
  } else if (delta == 15) {
    LOG(L_ERROR, "Found Option Delta set to 15 - this is an error, or indicator for payload\r\n");
    goto error;
  }
  this->number = (coap_option_number_e)(previous_number + delta);

  /* Option Length */
  len = first_byte & 0x0f;
  if (len <= 12) {
  } else if (len == 13) {
    len = bin_t_decode_uint8(src) + 13;
  } else if (len == 14) {
    len = bin_t_decode_uint16(src) + 269;
  } else if (len == 15) {
    LOG(L_ERROR, "Found Option Len set to 15 - this is an error, or for future use\r\n");
    goto error;
  }

  /* Value */
  bin_t_check_len(src, len);
  switch (this->number) {
    case CoAP_Option__If_None_Match:
      this->format = CoAP_Option_Format__empty;
      src->idx += len;
      break;
    case CoAP_Option__If_Match:
    case CoAP_Option__ETag:
    case CoAP_Option__Twilio_HostDevice_Information:
      this->format           = CoAP_Option_Format__opaque;
      this->value.opaque.s   = (char *)src->s + src->idx;
      this->value.opaque.len = len;
      src->idx += len;
      break;
    case CoAP_Option__Uri_Port:
    case CoAP_Option__Content_Format:
    case CoAP_Option__Max_Age:
    case CoAP_Option__Accept:
    case CoAP_Option__Size1:
    case CoAP_Option__Observe:
    case CoAP_Option__Block2:
    case CoAP_Option__Block1:
    case CoAP_Option__Size2:
    case CoAP_Option__No_Response:
    case CoAP_Option__Twilio_Queued_Command_Count:
      this->format     = CoAP_Option_Format__uint;
      this->value.uint = bin_t_decode_varuint(src, len);
      break;
    case CoAP_Option__Uri_Host:
    case CoAP_Option__Location_Path:
    case CoAP_Option__Uri_Path:
    case CoAP_Option__Uri_Query:
    case CoAP_Option__Location_Query:
    case CoAP_Option__Proxy_Uri:
    case CoAP_Option__Proxy_Scheme:
      this->format           = CoAP_Option_Format__string;
      this->value.string.s   = (char *)src->s + src->idx;
      this->value.string.len = len;
      src->idx += len;
      break;
    default:
      LOG(L_WARN, "Not supported Option %d - handling as opaque\r\n", this->number);
      this->format           = CoAP_Option_Format__opaque;
      this->value.opaque.s   = (char *)src->s + src->idx;
      this->value.opaque.len = len;
      src->idx += len;
      break;
  }

  return 1;
bad_length:
error:
  return 0;
}
