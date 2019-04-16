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
 * \file OwlModemGNSS.h - API for retrieving various data from the GNSS card
 */

#ifndef __OWL_MODEM_GNSS_H__
#define __OWL_MODEM_GNSS_H__

#include "enums.h"



#define MODEM_GNSS_RESPONSE_BUFFER_SIZE 1024



class OwlModem;


typedef struct {
  bool valid; /**< If this data is a valid valid (true) or there is a navigation receiver warning (false) */

  struct {
    int latitude_degrees;    /**< Latitude degrees */
    float latitude_minutes;  /**< Latitude minutes */
    bool is_north;           /**< True for North, false for South */
    int longitude_degrees;   /**< Longitude degrees */
    float longitude_minutes; /**< Latitude minutes */
    bool is_west;            /**< True for West, false for East */

    float speed_knots; /**< Speed over ground in knots (000.0 ~ 999.9) */
    float course;      /**< Course over ground in degrees (000.0 ~ 359.9) */
  } position;

  struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
  } date;

  struct {
    uint8_t hours; /**< Hours 0-23 */
    uint8_t minutes;
    uint8_t seconds;
    uint16_t millis;
  } time;

  char mode_indicator; /**< ‘N’ = Data not valid
                        *   ‘A’ = Autonomous mode
                        *   ‘D’ = Differential mode
                        *   ‘E’ = Estimated (dead reckoning) mode */
} gnss_data_t;

/**
 * Twilio wrapper for the serial interface to a GNSS module
 */
class OwlModemGNSS {
 public:
  OwlModemGNSS(OwlModem *owlModem);


  /**
   * Get fresh positioning data from the GNSS module.
   * @param out_data - output data structure
   * @return 1 on success, 0 on failure
   */
  int getGNSSData(gnss_data_t *out_data);

  /**
   * Log a position data structure.
   * @param level - log level to show on
   */
  void logGNSSData(log_level_t level, gnss_data_t data);


 private:
  OwlModem *owlModem = 0;

  char GNSS_response_buffer[MODEM_GNSS_RESPONSE_BUFFER_SIZE];
  str GNSS_response = {.s = GNSS_response_buffer, .len = 0};
};

#endif
