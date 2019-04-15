/*
 * time.cpp
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


#include "time.h"

#include <Arduino.h>



static uint32_t epoch = 0, last_millis = 0;

extern "C" owl_time_t owl_time() {
  uint32_t now = millis();
  if (now < last_millis) epoch++;
  return (uint64_t)epoch << 32 | now;
}

extern "C" void owl_delay(uint32_t ms) {
  delay(ms);
}
