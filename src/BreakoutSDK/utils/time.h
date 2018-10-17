/*
 * time.h
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
 * \file time.h - time retrieval
 */

#ifndef __OWL_UTILS_TIME_H__
#define __OWL_UTILS_TIME_H__

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef uint64_t owl_time_t;


/**
 * Non-wrapping time in milliseconds. Must be called at least once every 49 days or so to detect the wrap-around.
 * @return time since start in milliseconds - wrap-around is not a concern, but do use owl_time_millis_t and uint64_t
 * arithmetic in your code
 */
owl_time_t owl_time();


#ifdef __cplusplus
}
#endif

#endif
