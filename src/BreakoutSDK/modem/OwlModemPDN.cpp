/*
 * OwlModemPDN.cpp
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
 * \file OwlModemPDN.cpp - API for managing the Packet Data Network (aka APN configuration)
 */

#include "OwlModemPDN.h"

#include <stdio.h>


OwlModemPDN::OwlModemPDN(OwlModemAT *atModem) : atModem_(atModem) {
}



static str s_cgpaddr = STRDECL("+CGPADDR: ");

int OwlModemPDN::getAPNIPAddress(uint8_t cid, uint8_t ipv4[4], uint8_t ipv6[16]) {
  int cnt   = 0;
  str token = {0};
  if (ipv4) bzero(ipv4, 4);
  if (ipv6) bzero(ipv6, 16);
  char buf[64];
  snprintf(buf, 64, "AT+CGPADDR=%d", cid);
  int result =
      atModem_->doCommandBlocking(buf, 3000, &pdn_response) == AT_Result_Code__OK;
  if (!result) return 0;
  OwlModemAT::filterResponse(s_cgpaddr, pdn_response, &pdn_response);
  while (str_tok(pdn_response, ",\r\n", &token)) {
    str token_ip = {0};
    switch (cnt) {
      case 0:
        // cid, ignore
        break;
      case 1:
      case 2:
        while (str_tok(token, " \"", &token_ip)) {
          if (token_ip.len <= 15) {
            /* IPv4 */
            int digit        = 0;
            str token_number = {0};
            while (str_tok(token_ip, ".", &token_number))
              ipv4[digit++] = str_to_uint32_t(token_number, 10);
            if (digit != 4) {
              LOG(L_ERR, "IPv4 [%.*s] has invalid number of tokens %d\r\n", token_ip.len, token_ip.s, digit);
              return 0;
            }
          } else {
            /* IPv6 */
            int digit        = 0;
            str token_number = {0};
            while (str_tok(token_ip, ".", &token_number))
              ipv6[digit++] = str_to_uint32_t(token_number, 10);
            if (digit != 16) {
              LOG(L_ERR, "IPv6 [%.*s] has invalid number of tokens %d\r\n", token_ip.len, token_ip.s, digit);
              return 0;
            }
          }
        }
        break;
      default:
        LOG(L_ERR, "Not handled %d(-th) token [%.*s]\r\n", cnt, token.len, token.s);
        return 0;
    }
    cnt++;
  }
  return 1;
}
