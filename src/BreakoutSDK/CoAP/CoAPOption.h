/*
 * CoAPOption.h
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

#ifndef __OWL_COAP_OPTION_H__
#define __OWL_COAP_OPTION_H__

#include "enums.h"



typedef enum {
  CoAP_Option_Format__empty = 0,
  CoAP_Option_Format__opaque,
  CoAP_Option_Format__uint,
  CoAP_Option_Format__string,
} coap_option_format_e;



class CoAPOption {
 public:
  coap_option_number_e number;
  coap_option_format_e format;
  union {
    /* void empty; */
    str opaque;
    uint64_t uint;
    str string;
  } value; /**< This is shallow for now, to avoid throwing exceptions from constructors, but also as optimization */

  CoAPOption();
  CoAPOption(coap_option_number_e number);
  CoAPOption(coap_option_number_e number, str opaque);
  CoAPOption(coap_option_number_e number, uint64_t uint);
  CoAPOption(coap_option_number_e number, char *string);
  ~CoAPOption();

  void log(log_level_t level);
  int encode(coap_option_number_e previous_number, bin_t *dst);
  int decode(coap_option_number_e previous_number, bin_t *src);

  CoAPOption *next = 0;
};

#endif
