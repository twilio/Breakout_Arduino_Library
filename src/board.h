/*
 * board.h
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
 * \file board.h - Header to defined some basic configs for the board required for the Arduino SDK
 *
 * This file is specifically written for the Wio LTE Cat. M1/NB-IoT Tracker board
 * http://wiki.seeedstudio.com/Wio_LTE_Cat_M1_NB-IoT_Tracker/
 */


#ifndef _OWL_BOARD_H_
#define _OWL_BOARD_H_



#define SerialGrove Serial         // UART1
#define SerialModule Serial1       // UART2
#define SerialGNSS Serial2         // UART3
#define SerialDebugPort SerialUSB  // USB port

#define SerialModule_Baudrate 115200
#define SerialGNSS_BAUDRATE 9600
#define BG96_Baudrate 9600

/**
 * MCU Pin Definitions
 */
typedef enum {

  CTS_PIN          = 0,   // PA0
  RTS_PIN          = 1,   // PA1
  RGB_LED_PWR_PIN  = 8,   // PA8
  SD_PWR_PIN       = 15,  // PA15
  BAT_C_PIN        = 16,  // PB0
  RGB_LED_PIN      = 17,  // PB1
  MODULE_PWR_PIN   = 21,  // PB5
  ENABLE_VCCB_PIN  = 26,  // PB10
  GNSS_PWR_PIN     = 28,  // PB12
  RX_LED_PIN       = 29,  // PB13
  TX_LED_PIN       = 30,  // PB14
  GNSS_1PPS_PIN    = 31,  // PB15
  GROVE_PWR_PIN    = 32,  // PC0
  GNSS_RST_PIN     = 33,  // PC1
  GNSS_INT_PIN     = 34,  // PC2
  RESET_MODULE_PIN = 35,  // PC3
  PWR_KEY_PIN      = 36,  // PC4

  BG96_RESET_PIN   = 19,  // PB3, Grove DC pin

  ANALOG_RND_PIN = 7,  // PA7 - this port was picked because it's not connected, so random source - but it
                       // is unfortunately on Grove A6 port, so if used, change this maybe
} board_pin_e;



#define TESTING_VARIANT_INIT 0
#define TESTING_VARIANT_REG 0


#define BREAKOUT_IP "54.145.1.94"
#define TESTING_APN "nb.iot"

#endif
