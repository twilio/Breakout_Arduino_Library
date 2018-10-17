/*
 * log.h
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
 * \file log.g - logging utilities
 */

#ifndef __OWL_UTILS_LOG_H__
#define __OWL_UTILS_LOG_H__

#include <stdint.h>

#include "../../board.h"
#include "str.h"
#include "bin_t.h"


/*
 * Parameters for logging - change here to disable logging or colors
 */
#define LOG_DISABLED 0
#define LOG_NO_ANSI_COLORS 0
#define LOG_WITH_TIME 1
#define LOG_LINE_MAX_LEN 1024
#define LOG_OUTPUT SerialDebugPort


/*
 * Log levels
 */
#define L_CLI -4
#define L_ALERT -3
#define L_CRIT -2
#define L_ERR -1
#define L_ISSUE 0
#define L_WARN 1
#define L_NOTICE 2
#define L_INFO 3
#define L_DB 4
#define L_DBG 5
#define L_MEM 6



#if LOG_DISABLED == 0

#define IS_PRINTABLE(level) (level <= debug_level || level == L_CLI)

#define LOG(level, format, ...) owl_log(level, "%s():%d " format, __func__, __LINE__, ##__VA_ARGS__)
#define LOGE(level, format, ...) owl_log_empty(level, format, ##__VA_ARGS__)
#define LOGF(level, format, ...) owl_log(level, format, ##__VA_ARGS__)
#define LOGSTR(level, x) owl_log_str(level, x)
#define LOGBIN(level, x) owl_log_bin_t(level, x)

#else

#define IS_PRINTABLE(level) 0

#define LOG(level, format, args...)
#define LOGE(level, format, args...)
#define LOGF(level, format, args...)
#define LOGSTR(level, x)
#define LOGBIN(level, x)

#endif

#define GOTOERR(label)                                                                                                 \
  do {                                                                                                                 \
    LOG(L_ERR, "Going to label " #label "\r\n");                                                                       \
    goto label;                                                                                                        \
  } while (0)



#ifdef __cplusplus
extern "C" {
#endif


typedef int8_t log_level_t;


/**
 * Retrieve the current log level
 * @return the log level
 */
log_level_t owl_log_get_level();

/**
 * Set the current debug level to a certain value - log lower or equal to this will be printed, others ignored
 * @param level - the level to set
 */
void owl_log_set_level(log_level_t level);

/**
 * Check if a level will produce an output at the current debug level
 * @return 1 if it will be printed, 0 if not
 */
int owl_log_is_printable(log_level_t level);


/**
 * Log something out. Use the LOG() macros instead, to also get the function and line information from the code. If you
 * want your log lines to align to the left (e.g. printing a table), use the LOGF() macro, which is a direct call to
 * this function.
 * @param level - level to output on
 * @param format - printf format
 * @param ... - parameters for the printf format
 */
void owl_log(log_level_t, char *format, ...);

/**
 * Log something out, without the time/level/etc prefix. Use the LOGE() macros instead, to also get the function and
 * line information from the code.
 * @param level - level to output on
 * @param format - printf format
 * @param ... - parameters for the printf format
 */
void owl_log_empty(log_level_t level, char *format, ...);

/**
 * Log a binary str in a nice binary dump format. Use the LOGSTR() macro instead, to also get the function and
 * line information from the code.
 * @param level - level to output on
 * @param x - the str to log
 */
void owl_log_str(log_level_t level, str x);

/**
 * Log a binary bin_t in a nice binary dump format. Use the LOGSTR() macro instead, to also get the function and
 * line information from the code.
 * @param level - level to output on
 * @param x - the bin_t to log
 */
void owl_log_bin_t(log_level_t level, bin_t x);


#ifdef __cplusplus
}
#endif



#endif
