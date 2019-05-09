/*
 * OwlModemSSLBG96.h
 * Twilio Breakout SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
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
 * \file OwlModemSSLBG96.h - API for SSL support in Quectel BG96 modems
 */

#ifndef __OWL_MODEM_SSL_BG96_H__
#define __OWL_MODEM_SSL_BG96_H__

#include "enums.h"

#include "OwlModemAT.h"

#define SSL_RESPONSE_BUFFER_SIZE 32

class OwlModemSSLBG96 {
 public:
  OwlModemSSLBG96(OwlModemAT* atModem);

  bool setDeviceCert(str cert, bool force = false);
  bool setDevicePkey(str pkey, bool force = false);
  bool setServerCA(str ca, bool force = false);

  bool initContext();

 private:
  OwlModemAT* atModem_;
  char ssl_response_buffer[SSL_RESPONSE_BUFFER_SIZE];
  str ssl_response = {.s = ssl_response_buffer, .len = 0};
};

#endif  // __OWL_MODEM_SSL_BG96_H__
