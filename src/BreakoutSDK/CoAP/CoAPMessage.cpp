/*
 * CoAPMessage.cpp
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

#include "CoAPMessage.h"

#include <Arduino.h>



CoAPMessage::CoAPMessage()
    : version(CoAP_Version__1),
      type(CoAP_Type__Confirmable),
      code_class(CoAP_Code_Class__Request),
      code_detail(CoAP_Code_Detail__Empty_Message),
      message_id(0),
      token_length(0),
      token(0) {
  this->options = 0;
  bzero(&this->payload, sizeof(str));
}

CoAPMessage::CoAPMessage(coap_type_e type, coap_code_class_e code_class, coap_code_detail_e code_detail,
                         coap_message_id_t message_id)
    : version(CoAP_Version__1),
      type(type),
      code_class(code_class),
      code_detail(code_detail),
      message_id(message_id),
      token_length(0),
      token(0) {
  this->options = 0;
  bzero(&this->payload, sizeof(str));
}

CoAPMessage::CoAPMessage(CoAPMessage *con_to_ack_or_reset, coap_type_e ack_or_reset)
    : version(con_to_ack_or_reset->version),
      type(ack_or_reset),
      code_class(CoAP_Code_Class__Request),
      code_detail(CoAP_Code_Detail__Empty_Message),
      message_id(con_to_ack_or_reset->message_id),
      token_length(0),
      token(0) {
  this->options = 0;
  bzero(&this->payload, sizeof(str));
}

CoAPMessage::CoAPMessage(CoAPMessage *request_to_response, coap_type_e type, coap_code_class_e code_class,
                         coap_code_detail_e code_detail)
    : version(request_to_response->version),
      type(type),
      code_class(code_class),
      code_detail(code_detail),
      message_id(request_to_response->message_id),
      token_length(request_to_response->token_length),
      token(request_to_response->token) {
  this->options = 0;
  bzero(&this->payload, sizeof(str));
}

CoAPMessage::~CoAPMessage() {
  this->destroy();
}

void CoAPMessage::destroy() {
  this->version      = CoAP_Version__1;
  this->type         = CoAP_Type__Confirmable;
  this->code_class   = CoAP_Code_Class__Request;
  this->code_detail  = CoAP_Code_Detail__Empty_Message;
  this->message_id   = 0;
  this->token_length = 0;
  this->token        = 0;
  while (this->options) {
    CoAPOption *opt = this->options;
    this->options   = opt->next;
    owl_delete(opt);
  }
  this->payload.s   = 0;
  this->payload.len = 0;
}

void CoAPMessage::log(log_level_t level) {
  LOGF(level, "CoAP Message  Version %d  Type %d - %s  Code %d.%02d - %s  Message-Id %05d  Token 0x%.*x\r\n",
       this->version, this->type, coap_type_text(this->type), this->code_class, this->code_detail,
       coap_code_text(this->code_class, this->code_detail), this->message_id, this->token_length * 2, this->token);
  for (CoAPOption *opt = this->options; opt; opt = opt->next)
    opt->log(level);
  if (this->payload.len) {
    LOGF(level, " - Payload:\r\n");
    LOGSTR(level, this->payload);
  }
}

int CoAPMessage::encode(bin_t *dst) {
  coap_option_number_e this_step_number = CoAP_Option__unknown;
  coap_option_number_e next_step_number = CoAP_Option__unknown;
  coap_option_number_e previous_number  = CoAP_Option__unknown;
  CoAPOption *opt                       = 0;

  this->log(L_DBG);

  if (this->version > 3 || this->version < 0) {
    LOG(L_ERR, "Invalid version %d - must be 2-bit long\r\n", this->version);
    goto error;
  }
  if (this->type > 3 || this->type < 0) {
    LOG(L_ERR, "Invalid type %d - must be 2-bit long\r\n", this->type);
    goto error;
  }
  if (this->token_length > 8 || this->token_length < 0) {
    LOG(L_ERR, "Invalid token_length %d - must be between 0 and 8\r\n", this->token_length);
    goto error;
  }
  if (this->code_class > 7) {
    LOG(L_ERR, "Invalid code class %d - must be 3-bit long\r\n", this->code_class);
    goto error;
  }
  if (this->code_detail > 31) {
    LOG(L_ERR, "Invalid code detail %d - must be 5-bit long\r\n", this->code_detail);
    goto error;
  }

  /* Header */
  bin_t_encode_uint8(dst, ((this->version & 0x03) << 6) | ((this->type & 0x03) << 4) | (this->token_length & 0x0f));
  bin_t_encode_uint8(dst, ((this->code_class & 0x07) << 5) | (this->code_detail & 0x1f));
  bin_t_encode_uint16(dst, this->message_id);

  if (this->code_class == CoAP_Code_Class__Empty_Message && this->code_detail == CoAP_Code_Detail__Empty_Message) {
    if (this->token_length) {
      LOG(L_ERR, "Empty message can not contain token_len %d != 0 - this is a message format error\r\n",
          this->token_length);
      goto error;
    }
    if (this->options || this->payload.len) {
      LOG(L_ERR, "Empty message can not contain Options or Payload - this is a message format error\r\n");
      goto error;
    }
  }

  /* Token */
  if (this->token_length) bin_t_encode_varuint(dst, this->token, this->token_length);

  /* Options */
  /* This is slow, but we don't have that many options, so keeping it simple */
  do {
    this_step_number = next_step_number;
    next_step_number = CoAP_Option__unknown;
    for (opt = this->options; opt; opt = opt->next)
      if (opt->number == this_step_number) {
        if (!opt->encode(previous_number, dst)) {
          LOG(L_ERR, "Error encoding option with number %d\r\n", opt->number);
          goto error;
        }
        previous_number = opt->number;
      } else if (opt->number > this_step_number) {
        if (next_step_number == CoAP_Option__unknown || opt->number < next_step_number) next_step_number = opt->number;
      }
  } while (next_step_number != CoAP_Option__unknown);

  /* Payload */
  if (this->payload.len) {
    bin_t_encode_uint8(dst, 0xff);  // Payload marker
    bin_t_encode_mem(dst, this->payload.s, this->payload.len);
  }

  return 1;
bad_length:
error:
  return 0;
}


int CoAPMessage::decode(bin_t *src) {
  uint8_t u8;
  CoAPOption *opt = 0, *last_opt = 0;
  coap_option_number_e previous_number = CoAP_Option__unknown;

  /* Clean-up first */
  this->destroy();

  /* Header */
  u8 = bin_t_decode_uint8(src);

  this->version = (coap_version_e)((u8 >> 6) & 0x03);
  switch (this->version) {
    case CoAP_Version__1:
      break;
    default:
      LOG(L_ERR, "Not supported version %d\r\n", this->version);
      goto error;
  }

  this->type = (coap_type_e)((u8 >> 4) & 0x03);

  this->token_length = u8 & 0x0f;
  if (this->token_length > 8) {
    LOG(L_ERR, "Token length %d > 8 is considered a formatting error\r\n", this->token_length);
    goto error;
  }

  u8 = bin_t_decode_uint8(src);

  this->code_class  = (coap_code_class_e)((u8 >> 5) & 0x07);
  this->code_detail = (coap_code_detail_e)(u8 & 0x1f);

  this->message_id = bin_t_decode_uint16(src);

  if (this->code_class == CoAP_Code_Class__Empty_Message && this->code_detail == CoAP_Code_Detail__Empty_Message) {
    if (this->token_length) {
      LOG(L_ERR, "Empty message received with token_len %d != 0 - this is a message format error\r\n",
          this->token_length);
      goto error;
    }
    if (src->idx < src->max) {
      LOG(L_ERR, "Empty message received with extra bytes after header - this is a message format error\r\n");
      goto error;
    }
  }

  /* Token */
  if (this->token_length) this->token = bin_t_decode_varuint(src, this->token_length);

  /* Options */
  while (src->idx < src->max && src->s[src->idx] != 0xff) {
    opt = owl_new CoAPOption();
    if (!opt) {
      LOG(L_ERR, "Error creating a new empty CoAPOption\r\n");
      goto error;
    } else if (!opt->decode(previous_number, src)) {
      LOG(L_ERR, "Error decoding next option. Consumed and left buffers are:\r\n");
      str data = {.s = (char *)src->s, .len = src->idx};
      LOGSTR(L_ERR, data);
      data.s   = (char *)src->s + src->idx;
      data.len = src->max - src->idx;
      LOGSTR(L_ERR, data);
      owl_delete(opt);
      goto error;
    }
    for (last_opt = this->options; last_opt != 0 && last_opt->next != 0; last_opt = last_opt->next)
      continue;
    if (last_opt)
      last_opt->next = opt;
    else
      this->options = opt;
    previous_number = opt->number;
  }

  /* Payload */
  if (src->idx < src->max && src->s[src->idx] == 0xff) {
    src->idx++;
    if (src->idx >= src->max) {
      LOG(L_ERR,
          "Message format error - Options end marker found, but no data after it. Consumed and left buffers are:\r\n");
      goto error;
      str data = {.s = (char *)src->s, .len = src->idx};
      LOGSTR(L_ERR, data);
      data.s   = (char *)src->s + src->idx;
      data.len = src->max - src->idx;
      LOGSTR(L_ERR, data);
      goto error;
    }
    this->payload.s   = (char *)src->s + src->idx;
    this->payload.len = src->max - src->idx;
    src->idx          = src->max;
  }

  return 1;
bad_length:
error:
  return 0;
}

int CoAPMessage::testCodec(CoAPMessage &msg, uint8_t *buffer, int len) {
  bin_t src = {.s = buffer, .idx = 0, .max = len};
  uint8_t buf[256];
  bin_t dst = {.s = buf, .idx = 0, .max = 256};

  if (!msg.decode(&src)) {
    LOG(L_ERR, "Error decoding CoAP test message\r\n");
    LOGBIN(L_ERR, src);
    return 0;
  } else {
    msg.log(L_NOTICE);
    if (!msg.encode(&dst)) {
      LOG(L_ERR, "Error encoding CoAP test message 2\r\n");
      LOGBIN(L_ERR, dst);
      return 0;
    } else {
      if (dst.idx != src.max || memcmp(dst.s, src.s, src.max) != 0) {
        LOG(L_ERR, "Re-encoded message as:\r\n");
        LOGBIN(L_ERR, dst);
        LOG(L_ERR, "  which is different than the source:\r\n");
        LOGBIN(L_ERR, src);
        return 0;
      }
      LOG(L_NOTICE, "Message bootstrap test successful\r\n");
    }
  }
  return 1;
}



CoAPOption *CoAPMessage::addOptionEmpty(coap_option_number_e number) {
  CoAPOption *opt = owl_new CoAPOption(number), *last_opt = 0;
  if (!opt) {
    LOG(L_ERR, "Error creating a new empty CoAPOption\r\n");
    return 0;
  }
  for (last_opt = this->options; last_opt != 0 && last_opt->next != 0; last_opt = last_opt->next)
    continue;
  if (last_opt)
    last_opt->next = opt;
  else
    this->options = opt;
  return opt;
}

CoAPOption *CoAPMessage::addOptionOpaque(coap_option_number_e number, str opaque) {
  CoAPOption *opt = owl_new CoAPOption(number, opaque), *last_opt = 0;
  if (!opt) {
    LOG(L_ERR, "Error creating a new opaque CoAPOption\r\n");
    return 0;
  }
  for (last_opt = this->options; last_opt != 0 && last_opt->next != 0; last_opt = last_opt->next)
    continue;
  if (last_opt)
    last_opt->next = opt;
  else
    this->options = opt;
  return opt;
}

CoAPOption *CoAPMessage::addOptionUint(coap_option_number_e number, uint64_t uint) {
  CoAPOption *opt = owl_new CoAPOption(number, uint), *last_opt = 0;
  if (!opt) {
    LOG(L_ERR, "Error creating a new uint CoAPOption\r\n");
    return 0;
  }
  for (last_opt = this->options; last_opt != 0 && last_opt->next != 0; last_opt = last_opt->next)
    continue;
  if (last_opt)
    last_opt->next = opt;
  else
    this->options = opt;
  return opt;
}

CoAPOption *CoAPMessage::addOptionString(coap_option_number_e number, char *string) {
  CoAPOption *opt = owl_new CoAPOption(number, string), *last_opt = 0;
  if (!opt) {
    LOG(L_ERR, "Error creating a new string CoAPOption\r\n");
    return 0;
  }
  for (last_opt = this->options; last_opt != 0 && last_opt->next != 0; last_opt = last_opt->next)
    continue;
  if (last_opt)
    last_opt->next = opt;
  else
    this->options = opt;
  return opt;
}

CoAPOption *CoAPMessage::addOptionString(coap_option_number_e number, str string) {
  CoAPOption *opt = owl_new CoAPOption(number, string), *last_opt = 0;
  if (!opt) {
    LOG(L_ERR, "Error creating a new empty CoAPOption\r\n");
    return 0;
  }
  opt->format = CoAP_Option_Format__string;
  for (last_opt = this->options; last_opt != 0 && last_opt->next != 0; last_opt = last_opt->next)
    continue;
  if (last_opt)
    last_opt->next = opt;
  else
    this->options = opt;
  return opt;
}

CoAPOption *CoAPMessage::getNextOption(coap_option_number_e number, coap_option_format_e format,
                                       CoAPOption **iterator) {
  CoAPOption *x = iterator ? (*iterator)->next : this->options;
  while (x && x->number != number && x->format != format)
    x                     = x->next;
  if (iterator) *iterator = x;
  return x;
}

CoAPOption *CoAPMessage::getNextOptionEmpty(coap_option_number_e number, CoAPOption **iterator) {
  return this->getNextOption(number, CoAP_Option_Format__empty, iterator);
}

CoAPOption *CoAPMessage::getNextOptionOpaque(coap_option_number_e number, CoAPOption **iterator) {
  return this->getNextOption(number, CoAP_Option_Format__opaque, iterator);
}

CoAPOption *CoAPMessage::getNextOptionUint(coap_option_number_e number, CoAPOption **iterator) {
  return this->getNextOption(number, CoAP_Option_Format__uint, iterator);
}

CoAPOption *CoAPMessage::getNextOptionString(coap_option_number_e number, CoAPOption **iterator) {
  return this->getNextOption(number, CoAP_Option_Format__string, iterator);
}



int CoAPMessage::addOptionIfMatch(str opaque) {
  return this->addOptionOpaque(CoAP_Option__If_Match, opaque) != 0;
}

int CoAPMessage::getNextOptionIfMatch(str *out_opaque, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionOpaque(CoAP_Option__If_Match, iterator);
  if (!opt) return 0;
  if (out_opaque) *out_opaque = opt->value.opaque;
  return 1;
}


int CoAPMessage::addOptionUriHost(char *string) {
  return this->addOptionString(CoAP_Option__Uri_Host, string) != 0;
}

int CoAPMessage::addOptionUriHost(str string) {
  return this->addOptionString(CoAP_Option__Uri_Host, string) != 0;
}

int CoAPMessage::getNextOptionUriHost(str *out_string, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionString(CoAP_Option__Uri_Host, iterator);
  if (!opt) return 0;
  if (out_string) *out_string = opt->value.string;
  return 1;
}


int CoAPMessage::addOptionETag(str opaque) {
  return this->addOptionOpaque(CoAP_Option__ETag, opaque) != 0;
}

int CoAPMessage::getNextOptionETag(str *out_opaque, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionOpaque(CoAP_Option__ETag, iterator);
  if (!opt) return 0;
  if (out_opaque) *out_opaque = opt->value.opaque;
  return 1;
}


int CoAPMessage::addOptionIfNoneMatch() {
  return this->addOptionEmpty(CoAP_Option__If_None_Match) != 0;
}

int CoAPMessage::getOptionIfNoneMatch() {
  return this->getNextOptionOpaque(CoAP_Option__If_None_Match, 0) != 0;
}


int CoAPMessage::addOptionUriPort(uint64_t uint) {
  return this->addOptionUint(CoAP_Option__Uri_Port, uint) != 0;
}

int CoAPMessage::getNextOptionUriPort(uint64_t *out_uint, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionUint(CoAP_Option__Uri_Port, iterator);
  if (!opt) return 0;
  if (out_uint) *out_uint = opt->value.uint;
  return 1;
}


int CoAPMessage::addOptionLocationPath(char *string) {
  return this->addOptionString(CoAP_Option__Location_Path, string) != 0;
}

int CoAPMessage::addOptionLocationPath(str string) {
  return this->addOptionString(CoAP_Option__Location_Path, string) != 0;
}

int CoAPMessage::getNextOptionLocationPath(str *out_string, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionString(CoAP_Option__Location_Path, iterator);
  if (!opt) return 0;
  if (out_string) *out_string = opt->value.string;
  return 1;
}


int CoAPMessage::addOptionUriPath(char *string) {
  return this->addOptionString(CoAP_Option__Uri_Path, string) != 0;
}

int CoAPMessage::addOptionUriPath(str string) {
  return this->addOptionString(CoAP_Option__Uri_Path, string) != 0;
}

int CoAPMessage::getNextOptionUriPath(str *out_string, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionString(CoAP_Option__Uri_Path, iterator);
  if (!opt) return 0;
  if (out_string) *out_string = opt->value.string;
  return 1;
}


int CoAPMessage::addOptionContentFormat(uint64_t uint) {
  return this->addOptionUint(CoAP_Option__Content_Format, uint) != 0;
}

int CoAPMessage::getNextOptionContentFormat(uint64_t *out_uint, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionUint(CoAP_Option__Content_Format, iterator);
  if (!opt) return 0;
  if (out_uint) *out_uint = opt->value.uint;
  return 1;
}


int CoAPMessage::addOptionMaxAge(uint64_t uint) {
  return this->addOptionUint(CoAP_Option__Max_Age, uint) != 0;
}

int CoAPMessage::getNextOptionMaxAge(uint64_t *out_uint, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionUint(CoAP_Option__Max_Age, iterator);
  if (!opt) return 0;
  if (out_uint) *out_uint = opt->value.uint;
  return 1;
}


int CoAPMessage::addOptionUriQuery(char *string) {
  return this->addOptionString(CoAP_Option__Uri_Query, string) != 0;
}

int CoAPMessage::addOptionUriQuery(str string) {
  return this->addOptionString(CoAP_Option__Uri_Query, string) != 0;
}

int CoAPMessage::getNextOptionUriQuery(str *out_string, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionString(CoAP_Option__Uri_Query, iterator);
  if (!opt) return 0;
  if (out_string) *out_string = opt->value.string;
  return 1;
}


int CoAPMessage::addOptionAccept(uint64_t uint) {
  return this->addOptionUint(CoAP_Option__Accept, uint) != 0;
}

int CoAPMessage::getNextOptionAccept(uint64_t *out_uint, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionUint(CoAP_Option__Accept, iterator);
  if (!opt) return 0;
  if (out_uint) *out_uint = opt->value.uint;
  return 1;
}


int CoAPMessage::addOptionLocationQuery(char *string) {
  return this->addOptionString(CoAP_Option__Location_Query, string) != 0;
}

int CoAPMessage::addOptionLocationQuery(str string) {
  return this->addOptionString(CoAP_Option__Location_Query, string) != 0;
}

int CoAPMessage::getNextOptionLocationQuery(str *out_string, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionString(CoAP_Option__Location_Query, iterator);
  if (!opt) return 0;
  if (out_string) *out_string = opt->value.string;
  return 1;
}


int CoAPMessage::addOptionProxyUri(char *string) {
  return this->addOptionString(CoAP_Option__Proxy_Uri, string) != 0;
}

int CoAPMessage::addOptionProxyUri(str string) {
  return this->addOptionString(CoAP_Option__Proxy_Uri, string) != 0;
}

int CoAPMessage::getNextOptionProxyUri(str *out_string, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionString(CoAP_Option__Proxy_Uri, iterator);
  if (!opt) return 0;
  if (out_string) *out_string = opt->value.string;
  return 1;
}



int CoAPMessage::addOptionProxyScheme(char *string) {
  return this->addOptionString(CoAP_Option__Proxy_Scheme, string) != 0;
}

int CoAPMessage::addOptionProxyScheme(str string) {
  return this->addOptionString(CoAP_Option__Proxy_Scheme, string) != 0;
}

int CoAPMessage::getNextOptionProxyScheme(str *out_string, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionString(CoAP_Option__Proxy_Scheme, iterator);
  if (!opt) return 0;
  if (out_string) *out_string = opt->value.string;
  return 1;
}


int CoAPMessage::addOptionSize1(uint64_t uint) {
  return this->addOptionUint(CoAP_Option__Size1, uint) != 0;
}

int CoAPMessage::getNextOptionSize1(uint64_t *out_uint, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionUint(CoAP_Option__Size1, iterator);
  if (!opt) return 0;
  if (out_uint) *out_uint = opt->value.uint;
  return 1;
}


int CoAPMessage::addOptionObserve(uint64_t uint) {
  return this->addOptionUint(CoAP_Option__Observe, uint) != 0;
}

int CoAPMessage::getNextOptionObserve(uint64_t *out_uint, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionUint(CoAP_Option__Observe, iterator);
  if (!opt) return 0;
  if (out_uint) *out_uint = opt->value.uint;
  return 1;
}


int CoAPMessage::addOptionBlock2(uint64_t uint) {
  return this->addOptionUint(CoAP_Option__Block2, uint) != 0;
}

int CoAPMessage::getNextOptionBlock2(uint64_t *out_uint, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionUint(CoAP_Option__Block2, iterator);
  if (!opt) return 0;
  if (out_uint) *out_uint = opt->value.uint;
  return 1;
}


int CoAPMessage::addOptionBlock1(uint64_t uint) {
  return this->addOptionUint(CoAP_Option__Block1, uint) != 0;
}

int CoAPMessage::getNextOptionBlock1(uint64_t *out_uint, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionUint(CoAP_Option__Block1, iterator);
  if (!opt) return 0;
  if (out_uint) *out_uint = opt->value.uint;
  return 1;
}


int CoAPMessage::addOptionSize2(uint64_t uint) {
  return this->addOptionUint(CoAP_Option__Size2, uint) != 0;
}

int CoAPMessage::getNextOptionSize2(uint64_t *out_uint, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionUint(CoAP_Option__Size2, iterator);
  if (!opt) return 0;
  if (out_uint) *out_uint = opt->value.uint;
  return 1;
}


int CoAPMessage::addOptionNoResponse(uint64_t uint) {
  return this->addOptionUint(CoAP_Option__No_Response, uint) != 0;
}

int CoAPMessage::getNextOptionNoResponse(uint64_t *out_uint, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionUint(CoAP_Option__Size1, iterator);
  if (!opt) return 0;
  if (out_uint) *out_uint = opt->value.uint;
  return 1;
}


int CoAPMessage::addOptionTwilioHostDeviceInformation(str opaque) {
  return this->addOptionOpaque(CoAP_Option__Twilio_HostDevice_Information, opaque) != 0;
}

int CoAPMessage::getNextOptionTwilioHostDeviceInformation(str *out_opaque, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionOpaque(CoAP_Option__Twilio_HostDevice_Information, iterator);
  if (!opt) return 0;
  if (out_opaque) *out_opaque = opt->value.opaque;
  return 1;
}


int CoAPMessage::addOptionTwilioQueuedCommandCount(uint64_t uint) {
  return this->addOptionUint(CoAP_Option__Twilio_Queued_Command_Count, uint) != 0;
}

int CoAPMessage::getNextOptionTwilioQueuedCommandCount(uint64_t *out_uint, CoAPOption **iterator) {
  CoAPOption *opt = this->getNextOptionUint(CoAP_Option__Twilio_Queued_Command_Count, iterator);
  if (!opt) return 0;
  if (out_uint) *out_uint = opt->value.uint;
  return 1;
}
