/*
 * OwlModemPDN.h
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
 * \file OwlModemPDN.h - API for managing the Packet Data Network (aka APN configuration)
 */

#ifndef __OWL_MODEM_PDN_H__
#define __OWL_MODEM_PDN_H__

#include "enums.h"
#include "OwlModemAT.h"



#define MODEM_PDN_RESPONSE_BUFFER_SIZE 512


/**
 * Twilio wrapper for the AT serial interface to a modem - Methods for Packet Data Network (aka APN configuration)
 */
class OwlModemPDN {
 public:
  OwlModemPDN(OwlModemAT *atModem);


  // TODO
  //  /**
  //   * Retrieve the current APN configuration.
  //   *
  //   * Format:
  //   *
  //   * <cid>,"PDP-Type","APN","IP-Addr"... TODO
  //   *
  //   * @param out_response - output buffer to fill with the command response
  //   * @param max_response_len - length of output buffer
  //   * @return 1 on success, 0 on failure
  //   */
  //  int getAPNConfiguration(str *out_response, int max_response_len);
  //  int setAPNConfiguration(uint8_t cid, str pdn_type, str apn, ...);

  /**
   * Retrieve the local IP address assigned on one of the APN contexts
   * @param cid - the context id of the APN configuration
   * @param ipv4 - output the IPv4 assigned - ipv4->s must have at least 15 bytes available
   * @param ipv6 - output the IPv6 assigned - ipv4->s must have at least 32 + 7 bytes available (standard notation)
   * @return 1 on success, 0 on failure
   */
  int getAPNIPAddress(uint8_t cid, uint8_t ipv4[4], uint8_t ipv6[16]);



 private:
  OwlModemAT *atModem_ = 0;

  char pdn_response_buffer[MODEM_PDN_RESPONSE_BUFFER_SIZE];
  str pdn_response = {.s = pdn_response_buffer, .len = 0};
};

#endif
