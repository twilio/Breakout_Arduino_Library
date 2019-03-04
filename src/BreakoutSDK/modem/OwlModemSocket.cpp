/*
 * OwlModemSocket.cpp
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
 * \file OwlModemSocket.cpp - API for TCP/UDP communication over sockets
 */

#include "OwlModemSocket.h"

#include <stdio.h>

#include "OwlModem.h"



void OwlModemSocketStatus::setOpened(at_uso_protocol_e proto) {
  is_opened    = 1;
  is_connected = 0;

  len_outstanding_receive_data     = 0;
  len_outstanding_receivefrom_data = 0;

  protocol = proto;

  handler_UDPData      = 0;
  handler_TCPData      = 0;
  handler_TCPAccept    = 0;
  handler_SocketClosed = 0;
}

void OwlModemSocketStatus::setClosed() {
  is_opened    = 0;
  is_connected = 0;

  // not blanking the values on purpose, because maybe some data was still queued
  //  len_outstanding_receive_data     = 0;
  //  len_outstanding_receivefrom_data = 0;

  protocol = AT_USO_Protocol__none;

  handler_UDPData      = 0;
  handler_TCPData      = 0;
  handler_TCPAccept    = 0;
  handler_SocketClosed = 0;
}



OwlModemSocket::OwlModemSocket(OwlModem *owlModem) : owlModem(owlModem) {
  for (uint8_t socket = 0; socket < MODEM_MAX_SOCKETS; socket++)
    status[socket].setClosed();
}



static str s_uusoco = STRDECL("+UUSOCO");

int OwlModemSocket::processURCConnected(str urc, str data) {
  if (!str_equal(urc, s_uusoco)) return 0;
  str token        = {0};
  uint8_t socket   = 0;
  int socket_error = 0;
  for (int i = 0; str_tok(data, ",", &token); i++) {
    switch (i) {
      case 0:
        socket = str_to_uint32_t(token, 10);
        break;
      case 1:
        socket_error = str_to_long_int(token, 10);
        break;
      default:
        break;
    }
  }
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
  } else {
    if (socket_error == 0) {
      this->status[socket].is_connected = 1;
      LOG(L_INFO, "Connected event on socket %d reported success\r\n", socket);
    } else {
      this->status[socket].is_connected = 0;
      LOG(L_ERROR, "Connected event on socket %d reported socket error %d\r\n", socket, socket_error);
    }
  }
  return 1;
}


static str s_uusocl = STRDECL("+UUSOCL");

int OwlModemSocket::processURCClosed(str urc, str data) {
  if (!str_equal(urc, s_uusocl)) return 0;
  str token      = {0};
  uint8_t socket = 0;
  for (int i = 0; str_tok(data, ",", &token); i++) {
    switch (i) {
      case 0:
        socket = str_to_uint32_t(token, 10);
        break;
      default:
        break;
    }
  }
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "handleClosed()  Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
  } else {
    this->status[socket].is_connected = 0;
    if (!this->status[socket].handler_SocketClosed)
      LOG(L_INFO,
          "Received URC socket-closed for socket %d. Set a handler when you call connect(), acceptTCP(), etc"
          " if you wish to receive this event in your application\r\n",
          socket);
    else
      (this->status[socket].handler_SocketClosed)(socket);
  }
  return 1;
}


static str s_uusoli = STRDECL("+UUSOLI");

int OwlModemSocket::processURCTCPAccept(str urc, str data) {
  if (!str_equal(urc, s_uusoli)) return 0;
  str token                = {0};
  uint8_t new_socket       = 0;
  str remote_ip            = {0};
  uint16_t remote_port     = 0;
  uint8_t listening_socket = 0;
  str local_ip             = {0};
  uint16_t local_port      = 0;
  for (int i = 0; str_tok(data, ",", &token); i++) {
    switch (i) {
      case 0:
        new_socket = str_to_uint32_t(token, 10);
        break;
      case 1:
        remote_ip = token;
        if (remote_ip.len >= 2 && remote_ip.s[0] == '\"' && remote_ip.s[remote_ip.len - 1] == '\"') {
          remote_ip.s += 1;
          remote_ip.len -= 2;
        }
        break;
      case 2:
        remote_port = (uint16_t)str_to_uint32_t(token, 10);
        break;
      case 3:
        listening_socket = str_to_uint32_t(token, 10);
        break;
      case 4:
        local_ip = token;
        if (local_ip.len >= 2 && local_ip.s[0] == '\"' && local_ip.s[local_ip.len - 1] == '\"') {
          local_ip.s += 1;
          local_ip.len -= 2;
        }
        break;
      case 5:
        local_port = (uint16_t)str_to_uint32_t(token, 10);
        break;
      default:
        break;
    }
  }
  if (new_socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad new_socket %d >= %d\r\n", new_socket, MODEM_MAX_SOCKETS);
  } else if (listening_socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad listening_socket %d >= %d\r\n", listening_socket, MODEM_MAX_SOCKETS);
  } else {
    this->status[new_socket].setOpened(AT_USO_Protocol__TCP);
    this->status[new_socket].is_connected         = 1;
    this->status[new_socket].handler_TCPData      = this->status[listening_socket].handler_TCPData;
    this->status[new_socket].handler_SocketClosed = this->status[listening_socket].handler_SocketClosed;

    if (!this->status[listening_socket].handler_TCPAccept)
      LOG(L_INFO,
          "Received URC for TCP-Accept on listening-socket %d local-ip %.*s, from %.*s:%u new socket %d. Set a handler "
          "when calling acceptTCP(), openAcceptTCP(), etc if you wish to receive this event in your application\r\n",
          listening_socket, local_ip.len, local_ip.s, remote_ip.len, remote_ip.len, new_socket);
    else
      (this->status[listening_socket].handler_TCPAccept)(new_socket, remote_ip, remote_port, listening_socket, local_ip,
                                                         local_port);
  }
  return 1;
}


static str s_uusord = STRDECL("+UUSORD");

int OwlModemSocket::processURCReceive(str urc, str data) {
  if (!str_equal(urc, s_uusord)) return 0;
  str token      = {0};
  uint8_t socket = 0;
  uint16_t len   = 0;
  for (int i = 0; str_tok(data, ",", &token); i++) {
    switch (i) {
      case 0:
        socket = str_to_uint32_t(token, 10);
        break;
      case 1:
        len = str_to_uint32_t(token, 10);
        break;
      default:
        break;
    }
  }
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
  } else {
    if (!this->status[socket].is_connected) {
      LOG(L_WARN, "Received +UUSORD event on socket %d which is not connected\r\n", socket);
    }
    this->status[socket].len_outstanding_receive_data = len;
    LOG(L_INFO, "Receive URC for queued received data on socket %d of %d bytes\r\n", socket, len);
  }
  return 1;
}


static str s_uusorf = STRDECL("+UUSORF");

int OwlModemSocket::processURCReceiveFrom(str urc, str data) {
  if (!str_equal(urc, s_uusorf)) return 0;
  str token      = {0};
  uint8_t socket = 0;
  uint16_t len   = 0;
  for (int i = 0; str_tok(data, ",", &token); i++) {
    switch (i) {
      case 0:
        socket = str_to_uint32_t(token, 10);
        break;
      case 1:
        len = str_to_uint32_t(token, 10);
        break;
      default:
        break;
    }
  }
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
  } else {
    this->status[socket].len_outstanding_receivefrom_data = len;
    LOG(L_INFO, "Receive URC for queued received-from data on socket %d of %d bytes\r\n", socket, len);
  }
  return 1;
}


int OwlModemSocket::processURC(str urc, str data) {
  /* ordered based on the expected frequency of arrival */
  if (processURCReceiveFrom(urc, data)) return 1;
  if (processURCReceive(urc, data)) return 1;
  if (processURCTCPAccept(urc, data)) return 1;
  if (processURCConnected(urc, data)) return 1;
  if (processURCClosed(urc, data)) return 1;
  return 0;
}



void OwlModemSocket::handleWaitingData() {
  LOG(L_DEBUG, "Starting handleWaitingData\r\n");

  char buf[64];
  str remote_ip = {.s = buf, .len = 0};
  uint16_t remote_port = 0;
  int data_len         = 0;

  for (uint8_t socket = 0; socket < MODEM_MAX_SOCKETS; socket++) {
    /* Receive-From - UDP */
    if (status[socket].len_outstanding_receivefrom_data) {
      remote_ip.len = 0;
      remote_port   = 0;
      switch (status[socket].protocol) {
        case AT_USO_Protocol__UDP:
          data_len = status[socket].len_outstanding_receivefrom_data;
          /* receive might include an event for the next data, so reset the current value now */
          status[socket].len_outstanding_receivefrom_data = 0;
          if (receiveFromUDP(socket, data_len, &remote_ip, &remote_port, &udp_data, MODEM_UDP_BUFFER_SIZE)) {
            if (status[socket].handler_UDPData)
              (status[socket].handler_UDPData)(socket, remote_ip, remote_port, udp_data);
            else
              LOG(L_INFO, "ReceiveFrom on socket %u UDP Data of %u bytes without handler - ignored\r\n", socket,
                  udp_data.len);
          } else {
            /* Should we reset the indicator here and retry? Maybe that's an infinite loop, so probably not */
          }
          break;
        default:
          LOG(L_ERROR, "Received on socket %u with bad protocol %d - ignored\r\n", socket, status[socket].protocol);
          status[socket].len_outstanding_receivefrom_data = 0;
          break;
      }
    }

    /* Receive - UDP or TCP */
    if (status[socket].len_outstanding_receive_data) {
      remote_ip.len = 0;
      remote_port   = 0;
      switch (status[socket].protocol) {
        case AT_USO_Protocol__UDP:
          data_len = status[socket].len_outstanding_receive_data;
          /* receive might include an event for the next data, so reset the current value now */
          status[socket].len_outstanding_receive_data = 0;
          if (receiveUDP(socket, data_len, &udp_data, MODEM_UDP_BUFFER_SIZE)) {
            if (status[socket].handler_UDPData)
              (status[socket].handler_UDPData)(socket, remote_ip, remote_port, udp_data);
            else
              LOG(L_INFO, "Receive on socket %u UDP Data of %u bytes without handler - ignored\r\n", socket,
                  udp_data.len);
          } else {
            /* Should we reset the indicator here and retry? Maybe that's an infinite loop, so probably not */
          }
          break;
        case AT_USO_Protocol__TCP:
          data_len = status[socket].len_outstanding_receive_data;
          /* receive might include an event for the next data, so reset the current value now */
          status[socket].len_outstanding_receive_data -= data_len;
          if (receiveTCP(socket, status[socket].len_outstanding_receivefrom_data, &udp_data, MODEM_UDP_BUFFER_SIZE)) {
            if (status[socket].handler_TCPData)
              (status[socket].handler_TCPData)(socket, udp_data);
            else
              LOG(L_INFO, "Received on socket %u TCP Data of %u bytes without handler - ignored\r\n", socket,
                  udp_data.len);

            status[socket].len_outstanding_receive_data -= udp_data.len;
            if (status[socket].len_outstanding_receive_data < 0) {
              LOG(L_ERROR, "Bad len_outstanding_receive_data calculation %d < 0\r\n",
                  status[socket].len_outstanding_receive_data);
              status[socket].len_outstanding_receive_data = 0;
            }
          }
          break;
        default:
          LOG(L_ERROR, "Received on socket %u with bad protocol %d - ignored\r\n", socket, status[socket].protocol);
          status[socket].len_outstanding_receive_data = 0;
          break;
      }
    }
  }
  LOG(L_DEBUG, "Done handleWaitingData\r\n");
}


static str s_usocr = STRDECL("+USOCR: ");

int OwlModemSocket::open(at_uso_protocol_e protocol, uint16_t local_port, uint8_t *out_socket) {
  if (out_socket) *out_socket = 255;
  int socket                  = 255;
  char buf[64];
  snprintf(buf, 64, "AT+USOCR=%d,%u", protocol, local_port);
  int result =
      owlModem->doCommand(buf, 3000, &socket_response, MODEM_SOCKET_RESPONSE_BUFFER_SIZE) == AT_Result_Code__OK;
  if (!result) return 0;
  owlModem->filterResponse(s_usocr, &socket_response);
  socket                      = str_to_uint32_t(socket_response, 10);
  if (out_socket) *out_socket = socket;

  this->status[socket].setOpened(protocol);

  return result;
}

int OwlModemSocket::close(uint8_t socket) {
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
    return 0;
  }
  char buf[64];
  switch (status[socket].protocol) {
    case AT_USO_Protocol__TCP:
      snprintf(buf, 64, "AT+USOCL=%u,0", socket);
      break;
    case AT_USO_Protocol__UDP:
    default:
      snprintf(buf, 64, "AT+USOCL=%u", socket);
      break;
  }
  int result = (owlModem->doCommand(buf, 120 * 1000, 0, 0) == AT_Result_Code__OK);
  if (!result) return 0;

  this->status[socket].setClosed();

  return result;
}

static str s_usoer = STRDECL("+USOER: ");

int OwlModemSocket::getError(at_uso_error_e *out_error) {
  if (out_error) *out_error = (at_uso_error_e)-1;
  int result = (owlModem->doCommand("AT+USOER", 1000, &socket_response, MODEM_SOCKET_RESPONSE_BUFFER_SIZE) ==
                AT_Result_Code__OK);
  if (!result) return 0;
  owlModem->filterResponse(s_usoer, &socket_response);

  if (out_error) *out_error = (at_uso_error_e)str_to_long_int(socket_response, 10);

  return result;
}

int OwlModemSocket::connect(uint8_t socket, str remote_ip, uint16_t remote_port, OwlModem_SocketClosedHandler_f cb) {
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
    return 0;
  }
  if (!this->status[socket].is_opened) {
    LOG(L_ERROR, "Socket %d is not opened\r\n", socket);
    return 0;
  }
  this->status[socket].is_connected = 0;
  char buf[64];
  switch (this->status[socket].protocol) {
    case AT_USO_Protocol__TCP:
      snprintf(buf, 64, "AT+USOCO=%u,\"%.*s\",%u,0", socket, remote_ip.len, remote_ip.s, remote_port);
      break;
    case AT_USO_Protocol__UDP:
      snprintf(buf, 64, "AT+USOCO=%u,\"%.*s\",%u", socket, remote_ip.len, remote_ip.s, remote_port);
      break;
    default:
      LOG(L_ERROR, "Socket %u has unsupported protocol %d\r\n", socket, this->status[socket].protocol);
      return 0;
  }
  int result =
      owlModem->doCommand(buf, 120 * 1000, &socket_response, MODEM_SOCKET_RESPONSE_BUFFER_SIZE) == AT_Result_Code__OK;
  if (!result) return 0;
  this->status[socket].is_connected         = 1;
  this->status[socket].handler_SocketClosed = cb;
  return result;
}

static str s_usowr = STRDECL("+USOWR: ");

int OwlModemSocket::send(uint8_t socket, str data) {
  int bytes_sent = 0;
  char buf[1200];
  int len = snprintf(buf, 1200, "AT+USOWR=%u,%d,\"", socket, data.len);
  len += str_to_hex(buf + len, 1200 - len, data);
  buf[len++] = '\"';
  buf[len++] = 0;
  int result =
      owlModem->doCommand(buf, 120 * 1000, &socket_response, MODEM_SOCKET_RESPONSE_BUFFER_SIZE) == AT_Result_Code__OK;
  if (!result) return -1;
  owlModem->filterResponse(s_usowr, &socket_response);
  str token = {0};
  for (int i = 0; str_tok(socket_response, ",\r\n", &token); i++)
    switch (i) {
      case 0:
        // socket
        break;
      case 1:
        bytes_sent = str_to_long_int(token, 10);
        break;
      default:
        break;
    }
  return bytes_sent;
}

int OwlModemSocket::sendUDP(uint8_t socket, str data, int *out_bytes_sent) {
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
    return 0;
  }
  if (data.len > 512) {
    LOG(L_ERROR, "Too much data %d > max 512 bytes\r\n", data.len);
    return 0;
  }
  if (!this->status[socket].is_opened) {
    LOG(L_ERROR, "Socket %d is not opened\r\n", socket);
    return 0;
  }
  if (!this->status[socket].is_connected) {
    LOG(L_ERROR, "Socket %d is not connected\r\n", socket);
    return 0;
  }
  if (this->status[socket].protocol != AT_USO_Protocol__UDP) {
    LOG(L_ERROR, "Socket %d is not an UDP socket\r\n", socket);
    return 0;
  }
  int bytes_sent                      = this->send(socket, data);
  if (out_bytes_sent) *out_bytes_sent = bytes_sent;
  LOG(L_INFO, "Sent data over UDP on socket %u %d bytes\r\n", socket, bytes_sent);
  return bytes_sent == data.len;
}

int OwlModemSocket::sendTCP(uint8_t socket, str data, int *out_bytes_sent) {
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
    return 0;
  }
  if (data.len > 512) {
    LOG(L_ERROR, "Too much data %d > max 512 bytes\r\n", data.len);
    return 0;
  }
  if (!this->status[socket].is_opened) {
    LOG(L_ERROR, "Socket %d is not opened\r\n", socket);
    return 0;
  }
  if (!this->status[socket].is_connected) {
    LOG(L_ERROR, "Socket %d is not connected\r\n", socket);
    return 0;
  }
  if (this->status[socket].protocol != AT_USO_Protocol__TCP) {
    LOG(L_ERROR, "Socket %d is not an TCP socket\r\n", socket);
    return 0;
  }
  int bytes_sent                      = this->send(socket, data);
  if (out_bytes_sent) *out_bytes_sent = bytes_sent;
  LOG(L_INFO, "Sent data over TCP on socket %u %d bytes\r\n", socket, bytes_sent);
  return bytes_sent > 0;
}

static str s_usost = STRDECL("+USOST: ");

int OwlModemSocket::sendToUDP(uint8_t socket, str remote_ip, uint16_t remote_port, str data) {
  int bytes_sent = 0;
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
    return 0;
  }
  if (data.len > 512) {
    LOG(L_ERROR, "Too much data %d > max 512 bytes\r\n", data.len);
    return 0;
  }
  if (!this->status[socket].is_opened) {
    LOG(L_ERROR, "Socket %d is not opened\r\n", socket);
    return 0;
  }
  if (this->status[socket].protocol != AT_USO_Protocol__UDP) {
    LOG(L_ERROR, "Socket %d is not an UDP socket\r\n", socket);
    return 0;
  }
  char buf[1200];
  int len =
      snprintf(buf, 1200, "AT+USOST=%u,\"%.*s\",%u,%d,\"", socket, remote_ip.len, remote_ip.s, remote_port, data.len);
  len += str_to_hex(buf + len, 1200 - len, data);
  buf[len++] = '\"';
  buf[len++] = 0;
  int result =
      owlModem->doCommand(buf, 10 * 1000, &socket_response, MODEM_SOCKET_RESPONSE_BUFFER_SIZE) == AT_Result_Code__OK;
  if (!result) return 0;
  owlModem->filterResponse(s_usost, &socket_response);
  str token = {0};
  for (int i = 0; str_tok(socket_response, ",\r\n", &token); i++)
    switch (i) {
      case 0:
        // socket
        break;
      case 1:
        bytes_sent = str_to_long_int(token, 10);
        break;
      default:
        break;
    }
  LOG(L_INFO, "Sent data over UDP on socket %u %d bytes\r\n", socket, bytes_sent);
  return bytes_sent == data.len;
}

int OwlModemSocket::getQueuedForReceive(uint8_t socket, int *out_receive_tcp, int *out_receive_udp,
                                        int *out_receivefrom_udp) {
  if (out_receive_tcp) *out_receive_tcp         = 0;
  if (out_receive_udp) *out_receive_udp         = 0;
  if (out_receivefrom_udp) *out_receivefrom_udp = 0;
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket);
    return 0;
  }
  switch (status[socket].protocol) {
    case AT_USO_Protocol__TCP:
      if (out_receive_tcp) *out_receive_tcp = status[socket].len_outstanding_receive_data;
      break;
    case AT_USO_Protocol__UDP:
      if (out_receive_udp) *out_receive_udp         = status[socket].len_outstanding_receive_data;
      if (out_receivefrom_udp) *out_receivefrom_udp = status[socket].len_outstanding_receivefrom_data;
      break;
    default:
      break;
  }
  return 1;
}

static str s_usord = STRDECL("+USORD: ");

int OwlModemSocket::receive(uint8_t socket, uint16_t len, str *out_data, int max_data_len) {
  if (out_data) out_data->len = 0;
  char buf[64];
  snprintf(buf, 64, "AT+USORD=%u,%u", socket, len);
  int result =
      owlModem->doCommand(buf, 1000, &socket_response, MODEM_SOCKET_RESPONSE_BUFFER_SIZE) == AT_Result_Code__OK;
  if (!result) return 0;
  owlModem->filterResponse(s_usord, &socket_response);
  str token             = {0};
  str sub               = {0};
  uint16_t received_len = 0;
  for (int i = 0; str_tok(socket_response, "\r\n,", &token); i++)
    switch (i) {
      case 0:
        // socket
        break;
      case 1:
        if (len == 0) {
          // This was a call to figure out how much data is there - re-send command with new length
          int available_data = str_to_long_int(token, 10);
          if (available_data > 0)
            return receive(socket, available_data, out_data, max_data_len);
          else
            return 1;
        }
        received_len = str_to_uint32_t(token, 10);
        break;
      case 2:
        sub = token;
        if (sub.len >= 2 && sub.s[0] == '"' && sub.s[sub.len - 1] == '"') {
          sub.s += 1;
          sub.len -= 2;
        }
        if (sub.len != received_len * 2) {
          LOG(L_ERROR, "Indicator said payload has %d bytes follow, but %d hex characters found\r\n", received_len,
              sub.len);
        }
        if (sub.len) {
          out_data->len = hex_to_str(out_data->s, max_data_len, sub);
          if (!out_data->len) goto error;
        }
        break;
      default:
        break;
    }
  return 1;
error:
  LOG(L_ERROR, "Bad payload\r\n");
  return 0;
}

int OwlModemSocket::receiveUDP(uint8_t socket, uint16_t len, str *out_data, int max_data_len) {
  if (out_data) out_data->len = 0;
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket);
    return 0;
  }
  if (len > 512) {
    LOG(L_ERROR,
        "Unfortunately, only supporting up to 512 binary bytes. 1024 would be possible, but in ASCII mode only\r\n",
        len);
    return 0;
  }
  if (!this->status[socket].is_opened) {
    LOG(L_ERROR, "Socket %d is not opened\r\n", socket);
    return 0;
  }
  if (!this->status[socket].is_connected) {
    LOG(L_ERROR, "Socket %d is not connected - use receiveFromUDP\r\n", socket);
    return 0;
  }
  if (this->status[socket].protocol != AT_USO_Protocol__UDP) {
    LOG(L_ERROR, "Socket %d is not an UDP socket\r\n", socket);
    return 0;
  }
  return this->receive(socket, len, out_data, max_data_len);
}

int OwlModemSocket::receiveTCP(uint8_t socket, uint16_t len, str *out_data, int max_data_len) {
  if (out_data) out_data->len = 0;
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket);
    return 0;
  }
  if (len > 512) {
    LOG(L_ERROR,
        "Unfortunately, only supporting up to 512 binary bytes. 1024 would be possible, but in ASCII mode only\r\n",
        len);
    return 0;
  }
  if (!this->status[socket].is_opened) {
    LOG(L_ERROR, "Socket %d is not opened\r\n", socket);
    return 0;
  }
  if (!this->status[socket].is_connected) {
    LOG(L_ERROR, "Socket %d is not connected\r\n", socket);
    return 0;
  }
  if (this->status[socket].protocol != AT_USO_Protocol__TCP) {
    LOG(L_ERROR, "Socket %d is not a TCP socket\r\n", socket);
    return 0;
  }
  return this->receive(socket, len, out_data, max_data_len);
}

static str s_usorf = STRDECL("+USORF: ");

int OwlModemSocket::receiveFromUDP(uint8_t socket, uint16_t len, str *out_remote_ip, uint16_t *out_remote_port,
                                   str *out_data, int max_data_len) {
  if (out_remote_ip) out_remote_ip->len = 0;
  if (out_remote_port) *out_remote_port = 0;
  if (out_data) out_data->len           = 0;
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket);
    return 0;
  }
  if (len > 512) {
    LOG(L_ERROR,
        "Unfortunately, only supporting up to 512 binary bytes. 1024 would be possible, but in ASCII mode only\r\n",
        len);
    return 0;
  }
  if (!this->status[socket].is_opened) {
    LOG(L_ERROR, "Socket %d is not opened\r\n", socket);
    return 0;
  }
  if (this->status[socket].protocol != AT_USO_Protocol__UDP) {
    LOG(L_ERROR, "Socket %d is not an UDP socket\r\n", socket);
    return 0;
  }
  char buf[64];
  snprintf(buf, 64, "AT+USORF=%u,%u", socket, len);
  int result =
      owlModem->doCommand(buf, 1000, &socket_response, MODEM_SOCKET_RESPONSE_BUFFER_SIZE) == AT_Result_Code__OK;
  if (!result) return 0;
  owlModem->filterResponse(s_usorf, &socket_response);
  str token             = {0};
  str sub               = {0};
  uint16_t received_len = 0;
  for (int i = 0; str_tok(socket_response, ",\r\n", &token); i++)
    switch (i) {
      case 0:
        // socket
        break;
      case 1:
        if (len == 0) {
          // This was a call to figure out how much data is there - re-send command with new length
          int available_data = str_to_long_int(token, 10);
          if (available_data > 0)
            return receiveFromUDP(socket, available_data, out_remote_ip, out_remote_port, out_data, max_data_len);
        }
        if (out_remote_ip) {
          sub = token;
          if (sub.len >= 2 && sub.s[0] == '"' && sub.s[sub.len - 1] == '"') {
            sub.s += 1;
            sub.len -= 2;
          }
          memcpy(out_remote_ip->s, sub.s, sub.len);
          out_remote_ip->len = sub.len;
        }
        break;
      case 2:
        if (out_remote_port) *out_remote_port = (uint16_t)str_to_uint32_t(token, 10);
        break;
      case 3:
        received_len = str_to_uint32_t(token, 10);
        break;
      case 4:
        sub = token;
        if (sub.len >= 2 && sub.s[0] == '"' && sub.s[sub.len - 1] == '"') {
          sub.s += 1;
          sub.len -= 2;
        }
        if (sub.len != received_len * 2) {
          LOG(L_ERROR, "Indicator said payload has %d bytes follow, but %d hex characters found\r\n", received_len,
              sub.len);
        }
        if (sub.len) {
          out_data->len = hex_to_str(out_data->s, max_data_len, sub);
          if (!out_data->len) goto error;
        }
        break;
      default:
        break;
    }

  return 1;
error:
  LOG(L_ERROR, "Bad payload\r\n");
  return 0;
}

int OwlModemSocket::listenUDP(uint8_t socket, uint16_t local_port, OwlModem_UDPDataHandler_f cb) {
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
    return 0;
  }
  if (!this->status[socket].is_opened) {
    LOG(L_ERROR, "Socket %d is not opened\r\n", socket);
    return 0;
  }
  if (this->status[socket].protocol != AT_USO_Protocol__UDP) {
    LOG(L_ERROR, "Socket %d is not an UDP socket\r\n", socket);
    return 0;
  }
  char buf[64];
  snprintf(buf, 64, "AT+USOLI=%u,%u", socket, local_port);
  int result = owlModem->doCommand(buf, 1000, 0, 0) == AT_Result_Code__OK;
  if (!result) return 0;

  this->status[socket].handler_UDPData = cb;

  return result;
}

int OwlModemSocket::listenTCP(uint8_t socket, uint16_t local_port, OwlModem_TCPDataHandler_f handler_tcp_data) {
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
    return 0;
  }
  if (!this->status[socket].is_opened) {
    LOG(L_ERROR, "Socket %d is not opened\r\n", socket);
    return 0;
  }
  if (this->status[socket].protocol != AT_USO_Protocol__TCP) {
    LOG(L_ERROR, "Socket %d is not an TCP socket\r\n", socket);
    return 0;
  }

  this->status[socket].handler_TCPData = handler_tcp_data;

  return 1;
}

int OwlModemSocket::acceptTCP(uint8_t socket, uint16_t local_port, OwlModem_TCPAcceptHandler_f handler_tcp_accept,
                              OwlModem_SocketClosedHandler_f handler_socket_closed,
                              OwlModem_TCPDataHandler_f handler_tcp_data) {
  if (socket >= MODEM_MAX_SOCKETS) {
    LOG(L_ERROR, "Bad socket %d >= %d\r\n", socket, MODEM_MAX_SOCKETS);
    return 0;
  }
  if (!this->status[socket].is_opened) {
    LOG(L_ERROR, "Socket %d is not opened\r\n", socket);
    return 0;
  }
  if (this->status[socket].protocol != AT_USO_Protocol__TCP) {
    LOG(L_ERROR, "Socket %d is not an UDP socket\r\n", socket);
    return 0;
  }
  char buf[64];
  snprintf(buf, 64, "AT+USOLI=%u,%u", socket, local_port);
  int result = owlModem->doCommand(buf, 1000, 0, 0) == AT_Result_Code__OK;
  if (!result) return 0;

  this->status[socket].handler_TCPAccept    = handler_tcp_accept;
  this->status[socket].handler_SocketClosed = handler_socket_closed;
  this->status[socket].handler_TCPData      = handler_tcp_data;

  return result;
}

int OwlModemSocket::openListenUDP(uint16_t local_port, OwlModem_UDPDataHandler_f handler_data, uint8_t *out_socket) {
  if (out_socket) *out_socket = 255;
  uint8_t socket              = 255;

  if (!this->open(AT_USO_Protocol__UDP, 0, &socket)) goto error;
  switch (this->owlModem->model) {
    case Owl_Modem__SARA_N410_02B__Listen_Bug:
      // Skip listen - but do set the handler
      this->status[socket].handler_UDPData = handler_data;
      break;
    default:
      if (!this->listenUDP(socket, local_port, handler_data)) goto error;
      break;
  }

  if (out_socket) *out_socket = socket;
  return 1;
error:
  if (socket != 255) this->close(socket);
  return 0;
}

int OwlModemSocket::openConnectUDP(str remote_ip, uint16_t remote_port, OwlModem_UDPDataHandler_f handler_data,
                                   uint8_t *out_socket) {
  if (out_socket) *out_socket = 255;
  uint8_t socket              = 255;

  if (!this->open(AT_USO_Protocol__UDP, 0, &socket)) goto error;
  if (!this->connect(socket, remote_ip, remote_port, (OwlModem_SocketClosedHandler_f)0)) goto error;

  this->status[socket].handler_UDPData = handler_data;

  if (out_socket) *out_socket = socket;
  return 1;
error:
  if (socket != 255) this->close(socket);
  return 0;
}

int OwlModemSocket::openListenConnectUDP(uint16_t local_port, str remote_ip, uint16_t remote_port,
                                         OwlModem_UDPDataHandler_f handler_data, uint8_t *out_socket) {
  if (out_socket) *out_socket = 255;
  uint8_t socket              = 255;

  if (!this->open(AT_USO_Protocol__UDP, 0, &socket)) goto error;
  switch (this->owlModem->model) {
    case Owl_Modem__SARA_N410_02B__Listen_Bug:
      // Skip listen, but do set the handler
      this->status[socket].handler_UDPData = handler_data;
      break;
    default:
      if (!this->listenUDP(socket, local_port, handler_data)) goto error;
      break;
  }
  if (!this->connect(socket, remote_ip, remote_port, (OwlModem_SocketClosedHandler_f)0)) goto error;

  if (out_socket) *out_socket = socket;
  return 1;
error:
  if (socket != 255) this->close(socket);
  return 0;
}

int OwlModemSocket::openListenConnectTCP(uint16_t local_port, str remote_ip, uint16_t remote_port,
                                         OwlModem_SocketClosedHandler_f handler_close,
                                         OwlModem_TCPDataHandler_f handler_data, uint8_t *out_socket) {
  if (out_socket) *out_socket = 255;
  uint8_t socket              = 255;

  if (!this->open(AT_USO_Protocol__TCP, local_port, &socket)) goto error;
  if (!this->listenTCP(socket, local_port, handler_data)) goto error;
  if (!this->connect(socket, remote_ip, remote_port, handler_close)) goto error;

  if (out_socket) *out_socket = socket;
  return 1;
error:
  if (socket != 255) this->close(socket);
  return 0;
}

int OwlModemSocket::openAcceptTCP(uint16_t local_port, OwlModem_TCPAcceptHandler_f handler_accept,
                                  OwlModem_SocketClosedHandler_f handler_socket_close,
                                  OwlModem_TCPDataHandler_f handler_data, uint8_t *out_socket) {
  if (out_socket) *out_socket = 255;
  uint8_t socket              = 255;

  if (!this->open(AT_USO_Protocol__TCP, 0, &socket)) goto error;
  if (!this->listenTCP(socket, local_port, handler_data)) goto error;
  if (!this->acceptTCP(socket, local_port, handler_accept, handler_socket_close, handler_data)) goto error;

  if (out_socket) *out_socket = socket;
  return 1;
error:
  if (socket != 255) this->close(socket);
  return 0;
}
