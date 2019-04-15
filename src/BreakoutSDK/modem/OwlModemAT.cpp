#include "OwlModemAT.h"
#include <stdio.h>

bool OwlModemAT::initTerminal() {
  if (doCommandBlocking("ATV1", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Potential error setting commands to always return response codes\r\n");
  }

  if (doCommandBlocking("ATQ0", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_ERR, "Error setting commands to return text response codes\r\n");
    return false;
  }

  if (doCommandBlocking("ATE0", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_ERR, "Error setting echo off\r\n");
    return false;
  }

  if (doCommandBlocking("AT+CMEE=2", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_ERR, "Error setting Modem Errors output to verbose (not numeric) values\r\n");
    return false;
  }

  if (doCommandBlocking("ATS3=13", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Error setting command terminating character\r\n");
  }

  if (doCommandBlocking("ATS4=10", 1000, 0, 0) != AT_Result_Code__OK) {
    LOG(L_WARN, "Error setting response separator character\r\n");
  }

  return true;
}

bool OwlModemAT::registerUrcHandler(UrcHandler handler, void *priv) {
  if (num_urc_handlers_ >= MaxUrcHandlers) {
    return false;
  }

  urc_handlers_[num_urc_handlers_]       = handler;
  urc_handler_params_[num_urc_handlers_] = priv;

  ++num_urc_handlers_;
  return true;
}

int OwlModemAT::sendData(str data) {
  ssize_t written = 0;
  ssize_t cnt;
  do {
    cnt = serial_->write((const uint8_t *)data.s + written, data.len - written);
    if (cnt <= 0) {
      LOG(L_ERR, "Had %d bytes to send on modem_port, but wrote only %d.\r\n", data.len, written);
      return 0;
    }
    written += cnt;
  } while (written < data.len);
  return 1;
}

int OwlModemAT::sendData(char *data) {
  str s = {.s = data, .len = strlen(data)};
  return sendData(s);
}

/*TODO: rewrite as a ring buffer*/
at_result_code_e OwlModemAT::extractResult(str *out_response, int max_response_len) {
  at_result_code_e result_code;
  for (int i = 0; i <= rx_buffer.len - 6; i++) {
    if (rx_buffer.s[i] != '\r' && rx_buffer.s[i] != '\n') continue;
    result_code = at_result_code_extract(rx_buffer.s + i, rx_buffer.len - i);
    if (result_code == AT_Result_Code__cme_error) {
      //      LOG(L_INFO, "Rx-Before [%.*s]\r\n", rx_buffer.len, rx_buffer.s);
      /* CME Error received - extract the text into response and return ERROR */
      result_code = AT_Result_Code__ERROR;
      char *start = rx_buffer.s + i + 2 /* CRLF */ + 12 /* length of "+CME ERROR: " */;
      int len     = 0;
      while (start + len + 1 < rx_buffer.s + rx_buffer.len) {
        if (start[len] == '\r' && start[len + 1] == '\n') break;
        len++;
      }
      if (out_response) {
        memcpy(out_response->s, start, len > max_response_len ? max_response_len : len);
        out_response->len = len;
      }
      /* truncate the rx_buffer */
      int next_index = i + 2 /* CRLF */ + 12 /* length of "+CME ERROR: " */ + len + 2 /* CRLF */;
      rx_buffer.len -= next_index;
      if (rx_buffer.len > 0) memmove(rx_buffer.s, rx_buffer.s + next_index, rx_buffer.len);
      //      LOG(L_INFO, "Result %d - %s\r\n", result_code, at_result_code_text(result_code));
      //      LOG(L_INFO, "Rx-After  [%.*s]\r\n", rx_buffer.len, rx_buffer.s);
      return result_code;
    } else if (result_code >= AT_Result_Code__OK) {
      /* extract the response before the result code */
      if (out_response) {
        /* remove the CR LF prefix/suffix */
        char *start = rx_buffer.s;
        int len     = i;
        while (len > 0 && (start[0] == '\r' || start[0] == '\n')) {
          start++;
          len--;
        }
        while (len > 0 && (start[len - 1] == '\r' || start[len - 1] == '\n')) {
          len--;
        }
        out_response->len = len > max_response_len ? max_response_len : len;
        memcpy(out_response->s, start, out_response->len);
        if (out_response->len < max_response_len) out_response->s[out_response->len] = '\0';
      }
      /* truncate the rx_buffer */
      int next_index = i + 2 /* CRLF */ + strlen(at_result_code_text(result_code)) + 2 /* CRLF */;
      rx_buffer.len -= next_index;
      if (rx_buffer.len > 0) memmove(rx_buffer.s, rx_buffer.s + next_index, rx_buffer.len);
      //      LOG(L_INFO, "Result %d - %s\r\n", result_code,  at_result_code_text(result_code);
      return result_code;
    }
  }
  return AT_Result_Code__unknown;
}

at_result_code_e OwlModemAT::sendCommand(str command) {
  if (serial_ == nullptr) {
    LOG(L_ERR, "sendCommand [%.*s] failed: serial device unavailable\r\n", command.len, command.s);
    return AT_Result_Code__failure;
  }

  if (paused_) {
    LOG(L_ERR, "sendCommand [%.*s] failed: modem is paused\r\n", command.len, command.s);
    return AT_Result_Code__failure;
  }

  if (in_command_) {
    LOG(L_ERR, "sendCommand [%.*s] failed: attempt to send while waiting for a response from the previous one\r\n",
        command.len, command.s);
    return AT_Result_Code__failure;
  }

  /* Tx */
  if (!sendData(command)) {
    LOG(L_ERR, "sendCommand [%.*s] failed: writing to serial device failed\r\n", command.len, command.s);
    return AT_Result_Code__failure;
  }

  /* Tx CRLF*/
  if (!sendData(CMDLT)) {
    LOG(L_ERR, "sendCommand [%.*s] failed: writing to serial device failed\r\n", command.len, command.s);
    return AT_Result_Code__failure;
  }

  in_command_ = true;
  return AT_Result_Code__OK;
}

/*at_result_code_e OwlModemAT::doCommand(str command, uint32_t timeout_millis, str *out_response, int max_response_len)
{ if (out_response) out_response->len = 0; at_result_code_e result_code; owl_time_t timeout; int received;

  in_command_ = 1;
  int i;
  int in_timer = 0;
  for (i = 0; i < 100 && in_timer != 0; i++)
    owl_delay(50);
  if (i >= 100) {
    LOG(L_ERR, "[%.*s] either called from a timer, or timeout waiting for timer to exit\r\n", command.len, command.s);
    return AT_Result_Code__failure;
  }

  if (!sendData(command)) goto failure;
  //  LOG(L_DBG, "[%.*s] sent\r\n", command.len, command.s);

  if (rx_buffer.len) consumeUnsolicited();

  if (!sendData(CMDLT)) goto failure;
  LOG(L_DBG, "[%.*s] sent\r\n", command.len, command.s);
  timeout = owl_time() + timeout_millis;
  do {
    received = drainModemRxToBuffer();
    if (!received) {
      owl_delay(50);
      if (owl_time() < timeout)
        continue;
      else
        break;
    }
    consumeUnsolicitedInCommandResponse();
    result_code = extractResult(out_response, max_response_len);
    if (result_code >= AT_Result_Code__OK) {
      in_command_ = 0;
      if (out_response)
        LOG(L_DBG, " - Execution complete - Result %d - %s Data [%.*s]\r\n", result_code,
            at_result_code_text(result_code), out_response->len, out_response->s);
      else
        LOG(L_DBG, " - Execution complete - Result %d - %s\r\n", result_code, at_result_code_text(result_code));
      return result_code;
    }
  } while (owl_time() < timeout);

  if (!str_equalcase_char(command, "AT")) LOG(L_WARN, " - Timed-out on [%.*s]\r\n", command.len, command.s);

  //  // reset the buffer on timeout
  //  rx_buffer.len = 0;
  // or don't reset on timeout - the handleRxOnTimer() shall drop orphan lines

  in_command_ = 0;
  return AT_Result_Code__timeout;
failure:
  LOG(L_WARN, " - Failure on [%.*s]\r\n", command.len, command.s);
  in_command_ = 0;
  return AT_Result_Code__failure;
}

at_result_code_e OwlModemAT::doCommand(char *command, uint32_t timeout_millis, str *out_response, int max_response_len)
{ str s = {.s = command, .len = strlen(command)}; return doCommand(s, timeout_millis, out_response, max_response_len);
}*/

at_result_code_e OwlModemAT::getLastCommandResponse(str *out_response, int max_response_len) {
  if (out_response) {
    out_response->len = 0;
  }

  consumeUnsolicitedInCommandResponse();
  at_result_code_e result_code = extractResult(out_response, max_response_len);
  if (result_code >= AT_Result_Code__OK) {
    in_command_ = false;
    if (out_response)
      LOG(L_DBG, " - Execution complete - Result %d - %s Data [%.*s]\r\n", result_code,
          at_result_code_text(result_code), out_response->len, out_response->s);
    else
      LOG(L_DBG, " - Execution complete - Result %d - %s\r\n", result_code, at_result_code_text(result_code));
    return result_code;
  } else {
    return AT_Result_Code__in_progress;
  }
}

at_result_code_e OwlModemAT::doCommandBlocking(str command, uint32_t timeout_millis, str *out_response,
                                               int max_response_len) {
  owl_time_t timeout;
  int received;
  at_result_code_e result_code;

  /* Before sending the actual command, get rid of the URC in the pipe. This way, the result of this command will be
   * nice and empty. */
  if (rx_buffer.len) {
    consumeUnsolicited();
  }

  result_code = sendCommand(command);
  if (result_code != AT_Result_Code__OK) {
    LOG(L_WARN, " - Failed to send [%.*s]\r\n", command.len, command.s);
    return result_code;
  }
  LOG(L_DBG, "[%.*s] sent\r\n", command.len, command.s);

  /* Rx */
  timeout = owl_time() + timeout_millis;
  do {
    received = drainModemRxToBuffer();
    if (!received) {
      owl_delay(50);
      if (owl_time() < timeout)
        continue;
      else
        break;
    }
    result_code = getLastCommandResponse(out_response, max_response_len);
    if (result_code >= AT_Result_Code__OK) {
      return result_code;
    }
  } while (owl_time() < timeout);

  if (!str_equalcase_char(command, "AT")) {
    LOG(L_WARN, " - Timed-out on [%.*s]\r\n", command.len, command.s);
  }

  interruptLastCommand();
  return AT_Result_Code__timeout;
}

at_result_code_e OwlModemAT::doCommandBlocking(char *command, uint32_t timeout_millis, str *out_response,
                                               int max_response_len) {
  str s = {.s = command, .len = strlen(command)};
  return doCommandBlocking(s, timeout_millis, out_response, max_response_len);
}

void OwlModemAT::filterResponse(str prefix, str *response) {
  if (!response) return;
  str line = {0};
  while (str_tok(*response, "\r\n", &line)) {
    if (!str_equal_prefix(line, prefix)) {
      /* Remove the line + next CRLF */
      if (line.s + line.len + 2 <= response->s + response->len && line.s[line.len] == '\r' &&
          line.s[line.len + 1] == '\n')
        line.len += 2;
      str_shrink_inside(*response, line.s, line.len);
      line.len = 0;
      continue;
    }
    /* Remove the prefix */
    str_shrink_inside(*response, line.s, prefix.len);
    line.len -= prefix.len;
  }
}

/**
 * These prefixes indicate lines which are not errors if not processed as URC, because they belong
 * to regular commands.
 */
/*static str prefix_non_urc[] = {

    // OwlModemInformation
    STRDECL("+CBC"),
    STRDECL("+CIND"),

    // OwlModemSIM
    STRDECL("+CCID"),
    STRDECL("+CNUM"),

    // OwlModemNetwork
    STRDECL("+CFUN"),
    STRDECL("+UMNOPROF"),
    STRDECL("+COPS"),
    STRDECL("+CSQ"),

    // OwlModemPDN
    STRDECL("+CGPADDR: "),

    // OwlModemSocket
    STRDECL("+USOCR"),
    STRDECL("+USOER"),
    STRDECL("+USOWR"),
    STRDECL("+USOST"),
    STRDECL("+USORD"),
    STRDECL("+USORF"),
    STRDECL("+USOCO"),

    // End of list marker
    {0}};*/

int OwlModemAT::processURC(str line, int report_unknown) {
  if (line.len < 1 || line.s[0] != '+') return 0;
  int k = str_find_char(line, ": ");
  if (k < 0) return 0;
  str urc  = {.s = line.s, .len = k};
  str data = {.s = line.s + k + 2, .len = line.len - k - 2};

  LOG(L_DBG, "URC [%.*s] Data [%.*s]\r\n", urc.len, urc.s, data.len, data.s);

  /* ordered based on expected incoming count of events */
  for (int i = 0; i < num_urc_handlers_; i++) {
    if (urc_handlers_[i](urc, data, urc_handler_params_[i])) {
      return 1;
    }
  }
  /*if (this->network.processURC(urc, data)) return 1;
  if (this->socket.processURC(urc, data)) return 1;
  if (this->SIM.processURC(urc, data)) return 1;*/

  if (report_unknown) {
    //  for (int i = 0; prefix_non_urc[i].s; i++)
    //    if (str_equal(urc, prefix_non_urc[i])) return 0;
    // If it wasn't on the ignored list, report it
    LOG(L_WARN, "Not handled URC [%.*s] with data [%.*s]\r\n", urc.len, urc.s, data.len, data.s);
  }
  return 0;
}

int OwlModemAT::getNextCompleteLine(int start_idx, str *line) {
  if (!line) return 0;
  line->s   = 0;
  line->len = 0;
  int i;
  int start = start_idx;

  for (i = start_idx; i < rx_buffer.len;) {
    if (rx_buffer.s[i] != '\r' && rx_buffer.s[i] != '\n') {
      /* part of current line */
      i++;
      continue;
    }
    if (start == i) {
      /* skip over empty lines */
      i++;
      start = i;
      continue;
    }
    line->s   = rx_buffer.s + start;
    line->len = i - start;
    return 1;
  }

  return 0;
}

void OwlModemAT::removeRxBufferLine(str line) {
  /* Remove the line + next CRLF */
  if (line.s + line.len + 2 <= rx_buffer.s + rx_buffer.len && line.s[line.len] == '\r' && line.s[line.len + 1] == '\n')
    line.len += 2;
  int before_len = line.s - rx_buffer.s;
  int after_len  = rx_buffer.len - before_len - line.len;
  if (after_len > 0) {
    memmove(line.s, line.s + line.len, after_len);
    rx_buffer.len -= line.len;
  } else if (after_len == 0) {
    rx_buffer.len -= line.len;
  } else {
    LOG(L_ERR, "Bad len calculation %d\r\n", after_len);
  }
}

void OwlModemAT::consumeUnsolicited() {
  str line                = {0};
  int start               = 0;
  int saved_rx_buffer_len = rx_buffer.len;

  LOG(L_DBG, "Old-Buffer\r\n");
  LOGSTR(L_DBG, this->rx_buffer);

  while (getNextCompleteLine(start, &line)) {
    processURC(line, 1);
    start = line.s - rx_buffer.s;
    removeRxBufferLine(line);
  }

  /* remove leading \r\n - aka empty lines */
  int cnt = 0;
  for (cnt = 0; cnt < rx_buffer.len; cnt++)
    if (rx_buffer.s[cnt] != '\r' && rx_buffer.s[cnt] != '\n') break;
  if (cnt > 0) {
    memmove(rx_buffer.s, rx_buffer.s + cnt, rx_buffer.len - cnt);
    rx_buffer.len -= cnt;
  }

  if (saved_rx_buffer_len != rx_buffer.len) {
    LOG(L_DBG, "New-Buffer\r\n");
    LOGSTR(L_DBG, this->rx_buffer);
  }
}

void OwlModemAT::consumeUnsolicitedInCommandResponse() {
  str line                = {0};
  int start               = 0;
  int saved_rx_buffer_len = rx_buffer.len;
  int consumed            = 0;

  LOG(L_DBG, "Old-Buffer\r\n");
  LOGSTR(L_DBG, this->rx_buffer);

  while (getNextCompleteLine(start, &line)) {
    LOG(L_DBG, "Line [%.*s]\r\n", line.len, line.s);

    consumed = processURC(line, 0);
    if (consumed) {
      start = line.s - rx_buffer.s;
      removeRxBufferLine(line);
    } else {
      start = line.s - rx_buffer.s + line.len;
    }
  }

  if (saved_rx_buffer_len != rx_buffer.len) {
    LOG(L_DBG, "New-Buffer\r\n");
    LOGSTR(L_DBG, this->rx_buffer);
  }
}


int OwlModemAT::drainModemRxToBuffer() {
  LOG(L_MEM, "Trying to drain modem\r\n");
  int available, received, total = 0;
  while ((available = serial_->available()) > 0) {
    if (available > MODEM_Rx_BUFFER_SIZE) available = MODEM_Rx_BUFFER_SIZE;
    if (available > MODEM_Rx_BUFFER_SIZE - rx_buffer.len) {
      int shift = available - (MODEM_Rx_BUFFER_SIZE - rx_buffer.len);
      LOG(L_WARN, "Rx buffer full with %d bytes. Dropping oldest %d bytes.\r\n", rx_buffer.len, shift);
      rx_buffer.len -= shift;
      memmove(rx_buffer.s, rx_buffer.s + shift, rx_buffer.len);
    }
    received = serial_->read((uint8_t *)rx_buffer.s + rx_buffer.len, available);
    if (received != available) {
      LOG(L_ERR, "modem_port said %d bytes available, but received %d.\r\n", available, received);
      if (received < 0) goto error;
    }

    rx_buffer.len += received;
    total += received;

    if (rx_buffer.len > MODEM_Rx_BUFFER_SIZE) {
      LOG(L_ERR, "Bug in the rx_buffer_len calculation %d > %d\r\n", rx_buffer.len, MODEM_Rx_BUFFER_SIZE);
      goto error;
    }

    LOG(L_DBG, "Modem Rx - size changed from %d to %d bytes\r\n", rx_buffer.len - received, rx_buffer.len);
    LOGSTR(L_DBG, this->rx_buffer);
  }
error:
  LOG(L_MEM, "Done draining modem %d\r\n", total);
  return total;
}

// OYTIS: NEVER call anything non-trivial from an interrupt
void OwlModemAT::spin() {
  LOG(L_MEM, "Spin on modem\r\n");

  /* Don't enter here in case a command is in progress. URC shouldn't come during a command and will call this later */
  if (serial_ == nullptr || paused_) {
    return;
  }

  drainModemRxToBuffer();
  if (rx_buffer.len > 0) {
    consumeUnsolicited();
  }

  LOG(L_MEM, "Done spinning\r\n");
}
