/*
 * OwlModemGNSS.h
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
 * \file OwlModemGNSS.cpp - API for retrieving various data from the GNSS card
 */

#include "OwlModemGNSS.h"
#include "OwlModem.h"


OwlModemGNSS::OwlModemGNSS(OwlModem *owlModem) : owlModem(owlModem) {
}


static str s_rmc = STRDECL("RMC,");

int OwlModemGNSS::getGNSSData(gnss_data_t *out_data) {
  owl_time_t timeout = owl_time() + 5 * 1000;
  int available, received, total = 0;
  str line  = {0};
  str token = {0};

  if (!out_data) {
    LOG(L_ERR, "Null parameter\r\n");
    return false;
  }
  GNSS_response.len = 0;
  do {
    /* drain data to buffer */
    //    LOG(L_DBG, "Draining GNSS-Rx\r\n");
    owlModem->drainGNSSRx(&GNSS_response, MODEM_GNSS_RESPONSE_BUFFER_SIZE);
    bzero(&line, sizeof(str));
    while (str_tok(GNSS_response, "\r\n", &line)) {
      // Looking for $--RMC,hhmmss.sss,x,llll.lll,a,yyyyy.yyy,a,x.x,u.u,xxxxxx,,,v*hh<CR><LF>
      // If this is incomplete, skip it
      if (line.s + line.len >= GNSS_response.s + GNSS_response.len) break;
      LOG(L_DB, "Line [%.*s]\r\n", line.len, line.s);
      if (line.len < 3 || line.s[0] != '$') continue;
      line.s += 3;
      line.len -= 3;
      if (!str_equalcase_prefix(line, s_rmc)) continue;
      line.s += s_rmc.len - 1;
      line.len -= s_rmc.len - 1;
      // hhmmss.sss,x,llll.lll,a,yyyyy.yyy,a,x.x,u.u,xxxxxx,,,v*hh
      bzero(&token, sizeof(str));
      bzero(out_data, sizeof(gnss_data_t));
      for (int cnt = 0; str_tok_with_empty_tokens(line, ",", &token); cnt++) {
        LOG(L_DB, "  Token[%d] = [%.*s]\r\n", cnt, token.len, token.s);
        switch (cnt) {
          case 0:
            // time hhmmss.sss
            if (token.len < 6) break;
            out_data->time.hours   = (token.s[0] - '0') * 10 + token.s[1] - '0';
            out_data->time.minutes = (token.s[2] - '0') * 10 + token.s[3] - '0';
            out_data->time.seconds = (token.s[4] - '0') * 10 + token.s[5] - '0';
            //.
            for (int k = 7; k < token.len; k++)
              out_data->time.millis = (token.s[k] - '0') * pow(10, 9 - k);
            break;
          case 1:
            // V/A
            if (token.len < 1) {
              LOG(L_ERR, "Bad status format\r\n");
              goto next_line;
            }
            if (token.s[0] == 'A') out_data->valid = true;
            break;
          case 2:
            // llll.lll
            if (token.len < 8) break;
            out_data->position.latitude_degrees = (token.s[0] - '0') * 10 + token.s[1] - '0';
            token.s += 2;
            token.len -= 2;
            out_data->position.latitude_minutes = str_to_double(token);
            break;
          case 3:
            // N/S
            if (token.len < 1) break;
            if (token.s[0] == 'N') out_data->position.is_north = true;
            break;
          case 4:
            // yyyyy.yyy
            if (token.len < 9) break;
            out_data->position.longitude_degrees =
                (token.s[0] - '0') * 100 + (token.s[1] - '0') * 10 + token.s[2] - '0';
            token.s += 3;
            token.len -= 3;
            out_data->position.longitude_minutes = str_to_double(token);
            break;
          case 5:
            // E/W
            if (token.len < 1) break;
            if (token.s[0] == 'W') out_data->position.is_west = true;
            break;
          case 6:
            // x.x
            if (token.len < 10) break;
            out_data->position.speed_knots = str_to_double(token);
            break;
          case 7:
            // u.u
            if (token.len < 10) break;
            out_data->position.course = str_to_double(token);
            break;
          case 8:
            // date ddmmyy
            if (token.len < 6) break;
            out_data->date.day   = (token.s[0] - '0') * 10 + token.s[1] - '0';
            out_data->date.month = (token.s[2] - '0') * 10 + token.s[3] - '0';
            out_data->date.year  = 2000 + (token.s[4] - '0') * 10 + token.s[5] - '0';
            break;
          case 9:
            break;
          case 10:
            break;
          case 11:
            // Mode indicator N/A/D/E
            if (token.len < 1) break;
            out_data->mode_indicator = token.s[0];
            break;
          default:
            break;
        }
      }

      for (int cnt = 0; str_tok_with_empty_tokens(line, ",", &token); cnt++) {
        int k = str_find_char(line, ",");
        if (k < 0) {
          token.s   = line.s;
          token.len = line.len;
        }
      }
      return 1;
    }
  next_line:
    if (owl_time() > timeout) {
      LOG(L_ERR, "Timed-out waiting for GNSS data\r\n");
      return false;
    }
    delay(50);
  } while (true);
  return false;
}



void OwlModemGNSS::logGNSSData(log_level_t level, gnss_data_t data) {
  if (!owl_log_is_printable(level)) return;
  LOG(level, "GNSS Data:  data_valid %s  mode_indicator %c(%s)\r\n", data.valid ? "yes" : "no", data.mode_indicator,
      data.mode_indicator == 'N' ? "Data not valid" : data.mode_indicator == 'A' ?
                                   "Autonomous mode" :
                                   data.mode_indicator == 'D' ? "Differential mode" : data.mode_indicator == 'E' ?
                                                                "Estimated (dead reckoning) mode" :
                                                                "<unknown>");
  if (data.valid) {
    LOG(level, "  - Position:  %d %7.5f %s  %d %7.5f %s\r\n", data.position.latitude_degrees,
        data.position.latitude_minutes, data.position.is_north ? "N" : "S", data.position.longitude_degrees,
        data.position.longitude_minutes, data.position.is_west ? "W" : "E");
    LOG(level, "  - Course:  %4.1f knots  %4.1f degrees\r\n", data.position.speed_knots, data.position.course);
  }
  if (data.date.year) {
    LOG(level, "  - DateTime:  %u-%02u-%02u %02u:%02u:%02u.%03u UTC\r\n", data.date.year, data.date.month,
        data.date.day, data.time.hours, data.time.minutes, data.time.seconds, data.time.millis);
  }
}
