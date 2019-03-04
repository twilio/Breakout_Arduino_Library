/*
 * log.cpp
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
 * \file utils.cpp - logging utilities
 */

#include "log.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <usb_serial.h>

#include "../../board.h"

#include "time.h"



int write_reliably(USBSerial *serial, const char *buf, int len) {
  if (!serial) return 0;
  int sent = 0;
  int cnt;
  do {
    cnt = serial->write(buf + sent, len - sent);
    if (cnt < 0) continue;
    sent += cnt;
  } while (sent < len);
  return 1;
}

static log_level_t debug_level = L_INFO;

extern "C" log_level_t owl_log_get_level() {
  return debug_level;
}

extern "C" void owl_log_set_level(log_level_t level) {
  debug_level = level;
}

extern "C" int owl_log_is_printable(log_level_t level) {
  return IS_PRINTABLE(level);
}

#if LOG_NO_ANSI_COLORS == 1

#define ANSI_NONE ""
#define ANSI_GRAY ""
#define ANSI_GREEN_FAINT ""
#define ANSI_BLINK_RED ""
#define ANSI_RED ""
#define ANSI_GREEN ""
#define ANSI_YELLOW ""
#define ANSI_YELLOW_FAINT ""
#define ANSI_BLUE ""
#define ANSI_BLUE_FAINT ""
#define ANSI_URL ""
#define ANSI_MAGENTA ""
#define ANSI_CYAN ""
#define ANSI_WHITE ""
#define ANSI_RESET ""
#define ANSI_RESET_BLINK ""

#define ANSI_BYPASS ANSI_NONE

#else
/* ANSI Escape codes:
 * 00 for normal display (or just 0)
 * 01 for bold on (or just 1)
 * 02 faint (or just 2)
 * 03 standout (or just 3)
 * 04 underline (or just 4)
 * 05 blink on (or just 5)
 */
#define ANSI_NONE ""
#define ANSI_GRAY "\033[01;30m"
#define ANSI_GREEN_FAINT "\033[02;32m"
#define ANSI_BLINK_RED "\033[01;05;31m"
#define ANSI_RED "\033[01;31m"
#define ANSI_GREEN "\033[01;32m"
#define ANSI_YELLOW "\033[01;33m"
#define ANSI_YELLOW_FAINT "\033[02;33m"
#define ANSI_BLUE "\033[01;34m"
#define ANSI_BLUE_FAINT "\033[02;34m"
#define ANSI_URL "\033[01;04;34m"
#define ANSI_MAGENTA "\033[01;35m"
#define ANSI_CYAN "\033[01;36m"
#define ANSI_WHITE "\033[01;37m"
#define ANSI_RESET "\033[00;49m"
#define ANSI_RESET_BLINK "\033[01;25m"

#define ANSI_BYPASS ANSI_GREEN

#endif

static char *owl_log_level_name(log_level_t level) {
  switch (level) {
    case L_BYPASS:
      return ANSI_BYPASS "BYPS";
    case L_ERROR:
      return ANSI_RED "ERR ";
    case L_WARN:
      return ANSI_YELLOW_FAINT"WARN";
    case L_INFO:
      return ANSI_CYAN "INFO";
    case L_DEBUG:
      return ANSI_BLUE_FAINT "DEBG";
    default:
      return ANSI_GREEN ">>>>";
  }
  return "<<<<";
}

static char *owl_log_level_color(log_level_t level) {
#if LOG_NO_ANSI_COLORS == 1
  return "";
#else
  switch (level) {
    case L_BYPASS:
      return ANSI_BYPASS;
    case L_ERROR:
      return ANSI_RED;
    case L_WARN:
      return ANSI_YELLOW_FAINT;
    case L_INFO:
      return ANSI_CYAN;
    case L_DEBUG:
      return ANSI_BLUE_FAINT;
    default:
      return ANSI_GREEN;
  }
#endif
}

extern "C" void owl_log(log_level_t level, char *format, ...) {
  if (!IS_PRINTABLE(level)) return;
  char buf[LOG_LINE_MAX_LEN];
  int cnt          = 0;
  char *level_name = owl_log_level_name(level);

#if LOG_WITH_TIME == 1
  owl_time_t now = owl_time();
  owl_time_t ms  = now % 1000;
  owl_time_t sl  = now / 1000;
  uint32_t h     = sl / 3600;
  uint32_t m     = sl % 3600 / 60;
  uint32_t s     = sl % 60;
  cnt +=
      snprintf(buf + cnt, LOG_LINE_MAX_LEN - cnt, ANSI_RESET "%02d:%02d:%02d.%03d %s ", h, m, s, (int)ms, level_name);
#else
  cnt += snprintf(buf + cnt, LOG_LINE_MAX_LEN - cnt, ANSI_RESET "%s ", level_name);
#endif

  write_reliably(&LOG_OUTPUT, buf, cnt);

  cnt = 0;
  va_list ap;
  va_start(ap, format);
  cnt += vsnprintf(buf + cnt, LOG_LINE_MAX_LEN - cnt, format, ap);

  if (cnt < 0) {
    // error in composing output - can't do much about it
  } else if (cnt >= LOG_LINE_MAX_LEN) {
    // truncated - let it as is
    write_reliably(&LOG_OUTPUT, buf, cnt);
    write_reliably(&LOG_OUTPUT, "* truncated", 11);
  } else {
    write_reliably(&LOG_OUTPUT, buf, cnt);
  }
  va_end(ap);

#if LOG_NO_ANSI_COLORS == 1
#else
  write_reliably(&LOG_OUTPUT, ANSI_RESET, strlen(ANSI_RESET));
#endif
}

extern "C" void owl_log_empty(log_level_t level, char *format, ...) {
  if (!IS_PRINTABLE(level)) return;
  char buf[LOG_LINE_MAX_LEN];
  int cnt = 0;

  va_list ap;
  va_start(ap, format);

#if LOG_NO_ANSI_COLORS == 1
#else
  char *color = owl_log_level_color(level);
  cnt         = strlen(color);
  memcpy(buf, color, cnt);
#endif

  cnt += vsnprintf(buf + cnt, LOG_LINE_MAX_LEN - cnt, format, ap);
  if (cnt < 0) {
    // error in composing output - can't do much about it
  } else if (cnt >= LOG_LINE_MAX_LEN) {
    // truncated - let it as is
    write_reliably(&LOG_OUTPUT, buf, cnt);
    write_reliably(&LOG_OUTPUT, "* truncated", 11);
  } else {
    write_reliably(&LOG_OUTPUT, buf, cnt);
  }
  va_end(ap);

#if LOG_NO_ANSI_COLORS == 1
#else
  write_reliably(&LOG_OUTPUT, ANSI_RESET, strlen(ANSI_RESET));
#endif
}

#define BIN_LOG_BYTES_PER_LINE 8
//#define BIN_LOG_MAX_BYTES 2048
#define MAX_STR_BIN_BUFFER 64 + BIN_LOG_BYTES_PER_LINE * 10

extern "C" void owl_log_str(log_level_t level, str x) {
  if (!IS_PRINTABLE(level)) return;

  int i, j, k;
  char buffer[MAX_STR_BIN_BUFFER];
  str s = {.s = buffer, .len = 0};
  LOGE(level, "+-%3d bytes-+---------- hex ----------+--------------- dec --------------+- ascii --+\r\n", x.len);
  for (i = 0; i < x.len; i += BIN_LOG_BYTES_PER_LINE) {
    k = 0;
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "|%5d-%5d|", i, i + BIN_LOG_BYTES_PER_LINE);
    for (j = i; j < i + BIN_LOG_BYTES_PER_LINE && j < x.len; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " %02x", (unsigned char)x.s[j]);
    for (; j < i + BIN_LOG_BYTES_PER_LINE; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "   ");
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " | ");
    for (j = i; j < i + BIN_LOG_BYTES_PER_LINE && j < x.len; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " %3u", (unsigned char)x.s[j]);
    for (; j < i + BIN_LOG_BYTES_PER_LINE; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "    ");
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " | ");
    for (j = i; j < i + BIN_LOG_BYTES_PER_LINE && j < x.len; j++)
      if (x.s[j] > 31 && x.s[j] < 127)
        k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "%c", x.s[j]);
      else
        buffer[k++] = '.';
    for (; j < i + BIN_LOG_BYTES_PER_LINE; j++)
      buffer[k++] = ' ';
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " |\r\n");
    s.len = k;
    LOGE(level, "%.*s", s.len, s.s);
  }
  LOGE(level, "+-----------+-------------------------+----------------------------------+----------+\r\n");
}

extern "C" void owl_log_bin_t(log_level_t level, bin_t x) {
  if (!IS_PRINTABLE(level)) return;

  int i, j, k;
  char buffer[MAX_STR_BIN_BUFFER];
  str s = {.s = buffer, .len = 0};
  LOGE(level, "+-----------+---------- hex ----------+--------------- dec --------------+- ascii --+\r\n");
  for (i = 0; i < x.idx; i += BIN_LOG_BYTES_PER_LINE) {
    k = 0;
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "|%5d-%5d|", i, i + BIN_LOG_BYTES_PER_LINE);
    for (j = i; j < i + BIN_LOG_BYTES_PER_LINE && j < x.idx; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " %02x", (unsigned char)x.s[j]);
    for (; j < i + BIN_LOG_BYTES_PER_LINE; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "   ");
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " | ");
    for (j = i; j < i + BIN_LOG_BYTES_PER_LINE && j < x.idx; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " %3u", (unsigned char)x.s[j]);
    for (; j < i + BIN_LOG_BYTES_PER_LINE; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "    ");
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " | ");
    for (j = i; j < i + BIN_LOG_BYTES_PER_LINE && j < x.idx; j++)
      if (x.s[j] > 31 && x.s[j] < 127)
        k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "%c", x.s[j]);
      else
        buffer[k++] = '.';
    for (; j < i + BIN_LOG_BYTES_PER_LINE; j++)
      buffer[k++] = ' ';
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " |\r\n");
    s.len = k;
    LOGE(level, "%.*s", s.len, s.s);
  }
  LOGE(level, "+--idx=%3d--+-------------------------+----------------------------------+----------+\r\n", x.idx);
  for (i = x.idx; i < x.max; i += BIN_LOG_BYTES_PER_LINE) {
    k = 0;
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "|%5d-%5d|", i, i + BIN_LOG_BYTES_PER_LINE);
    for (j = i; j < i + BIN_LOG_BYTES_PER_LINE && j < x.max; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " %02x", (unsigned char)x.s[j]);
    for (; j < i + BIN_LOG_BYTES_PER_LINE; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "   ");
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " | ");
    for (j = i; j < i + BIN_LOG_BYTES_PER_LINE && j < x.max; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " %3u", (unsigned char)x.s[j]);
    for (; j < i + BIN_LOG_BYTES_PER_LINE; j++)
      k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "    ");
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " | ");
    for (j = i; j < i + BIN_LOG_BYTES_PER_LINE && j < x.max; j++)
      if (x.s[j] > 31 && x.s[j] < 127)
        k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, "%c", x.s[j]);
      else
        buffer[k++] = '.';
    for (; j < i + BIN_LOG_BYTES_PER_LINE; j++)
      buffer[k++] = ' ';
    k += snprintf(buffer + k, MAX_STR_BIN_BUFFER - k, " |\r\n");
    s.len = k;
    LOGE(level, "%.*s", s.len, s.s);
  }
  LOGE(level, "+--max=%3d--+-------------------------+----------------------------------+----------+\r\n", x.max);
}
