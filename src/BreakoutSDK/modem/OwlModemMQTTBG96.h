/*
 * OwlModemMQTTBG96.h
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
 * \file OwlModemMQTTBG96.h - API for MQTT support in Quectel BG96 modems
 */

#ifndef __OWL_MODEM_MQTT_BG96_H__
#define __OWL_MODEM_MQTT_BG96_H__

#include "enums.h"

#include "OwlModemAT.h"

class OwlModemMQTTBG96 {
 public:
  enum class qos_t {
    atMostOnce  = 0,
    atLeastOnce = 1,
    exactlyOnce = 2,
  };


  using mqtt_message_callback_t = void (*)(str, str);

  OwlModemMQTTBG96(OwlModemAT* atModem);

  bool openConnection(const char* host_addr, uint16_t port);
  void useTLS(bool use) {
    use_tls_ = use;
  }
  bool closeConnection();
  bool login(const char* client_id, const char* uname, const char* password);
  bool logout();
  /**
   * Publish to a topic
   * @param topic - topic to publish to
   * @param data - published data
   * @param retain - whether the data should be retained on the server
   * @param qos - MQTT Quality of Service
   * @return success status
   */
  bool publish(const char* topic, str data, bool retain = false, qos_t qos = qos_t::atMostOnce, uint16_t msg_id = 1);

  /**
   * Subscribe to a topic filter
   * @param topic_filter - topic filter used for subscription
   * @param max_qos - maximum Quality of Service at which we want to receive messages
   * @return success status
   */
  bool subscribe(const char* topic_filter, uint16_t msg_id, qos_t qos = qos_t::atMostOnce);

  void setMessageCallback(mqtt_message_callback_t callback) {
    message_callback_ = callback;
  }

 private:
  static bool processURC(str urc, str data, void* instance);
  void processURCQmtopen(str data);
  void processURCQmtclose(str data);
  void processURCQmtconn(str data);
  void processURCQmtdisc(str data);
  void processURCQmtpub(str data);
  void processURCQmtsub(str data);
  void processURCQmtuns(str data);
  void processURCQmtrecv(str data);

  OwlModemAT* atModem_;
  mqtt_message_callback_t message_callback_{nullptr};
  bool use_tls_{false};

  enum mqtt_command { qmtopen, qmtclose, qmtconn, qmtdisc, qmtpub, qmtsub, qmtuns, _num_mqtt_commands };

  bool wait_for_command_[_num_mqtt_commands];
  bool command_success_[_num_mqtt_commands];
  bool waitResultBlocking(mqtt_command command, int32_t timeout);
};

#endif  // __OWL_MODEM_MQTT_H__
