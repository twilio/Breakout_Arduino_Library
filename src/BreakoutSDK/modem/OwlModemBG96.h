/*
 * OwlModemBG96.h
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
 * \file OwlModemBG96.h - wrapper for BG96 modems on Seeed WiO tracker board via the Grove port
 */

#include <Arduino.h>
#include "ArduinoSeeedOwlSerial.h"

#include "enums.h"
#include "OwlModemAT.h"
#include "OwlModemInformation.h"
#include "OwlModemNetwork.h"
#include "OwlModemPDN.h"
#include "OwlModemSIM.h"
#include "OwlModemMQTTBG96.h"
#include "OwlModemSSLBG96.h"

class OwlModemBG96 {
 public:
  /**
   * Constructor for OwlModemBG96
   * @param modem_port - mandatory modem port
   */
  OwlModemBG96(HardwareSerial *modem_port_in);

  /**
   * Destructror of OwlModemRN4
   */
  ~OwlModemBG96();


  /**
   * Reset, power on the module and wait until the AT interface is up. Might take up to 10 seconds.
   * @return 1 on success, 0 on failure
   */
  int powerOn();

  /**
   * Power off the module and wait until the AT interface is up. Might take up to 10 seconds.
   * @return 1 on success, 0 on failure
   */
  int powerOff();

  /**
   * Check if the modem is powered on
   * @return 1 if the modem is powered on, 0 if not
   */
  int isPoweredOn();


  /**
   * Set the default parameters of the modem, to ensure that we have a consistent experience, no matter how they could
   * be originally configured by the vendors.
   * @return - 1 on success, 0 on failure
   */
  int initModem();

  /**
   * Wait for the modem to fully attach to the network. Usually, without this, there is little use for this class.
   * @param timeout - timeout to give up. 0 to try indefinitely.
   * @return 1 on success, 0 on failure
   */
  int waitForNetworkRegistration(owl_time_t timeout = 0);

 private:
  ArduinoSeeedHwOwlSerial modem_port;

 public:
  /*
   * Main APIs
   */
  OwlModemAT AT;
  /** Methods to get information from the modem */
  OwlModemInformation information;

  /** Method to get information and interact with the SIM card */
  OwlModemSIM SIM;

  /** Network Registration and Management */
  OwlModemNetwork network;

  /** APN Management */
  OwlModemPDN pdn;

  /** TLS set up */
  OwlModemSSLBG96 ssl;

  /** MQTT client */
  OwlModemMQTTBG96 mqtt;

 private:
  bool has_modem_port{false};

  char response_buffer[MODEM_RESPONSE_BUFFER_SIZE];
  str response = {.s = response_buffer, .len = 0};

  static void initCheckPIN(str message);
};
