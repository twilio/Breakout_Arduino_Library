/*
 * bin_t.h
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
 * \file bin_t.h - binary data with index and length, used to encode/decode messages
 *
 * When decoding a message, the idx starts from 0 and max is used as a limit.
 * When encoding a message, the max is the allocated size of the buffer, while 0 -> (idx - 1) are the useful written
 * bytes.
 *
 * Usually, in Wharf, the encode functions expand the buffer. Here, the simple macros do not, considering that this
 * is for a constrained memory environment and the buffer shall be pre-allocated.
 *
 */

#ifndef __OWL_UTILS_BIN_T_H__
#define __OWL_UTILS_BIN_T_H__

#include <stdint.h>

#include "str.h"

typedef struct {
  uint8_t *s;
  int idx;
  int max;
} bin_t;



#define bin_t_check_len(_b_, _len_)                                                                                    \
  do {                                                                                                                 \
    if ((_b_)->max - (_b_)->idx < (_len_)) {                                                                           \
      LOG(L_ERR, "Expected or needing %d bytes but only %d left\r\n", _len_, (_b_)->max - (_b_)->idx);                 \
      goto bad_length;                                                                                                 \
    }                                                                                                                  \
  } while (0)



#define bin_t_encode_uint8(_b_, _u8_)                                                                                  \
  do {                                                                                                                 \
    bin_t_check_len(_b_, 1);                                                                                           \
    (_b_)->s[(_b_)->idx++] = _u8_;                                                                                     \
  } while (0)

#define bin_t_decode_uint8(_b_)                                                                                        \
  ({                                                                                                                   \
    bin_t_check_len(_b_, 1);                                                                                           \
    uint8_t _u8_ = (_b_)->s[(_b_)->idx++];                                                                             \
    _u8_;                                                                                                              \
  })


#define bin_t_encode_uint16(_b_, _u16_)                                                                                \
  do {                                                                                                                 \
    bin_t_check_len(_b_, 2);                                                                                           \
    (_b_)->s[(_b_)->idx]     = _u16_ >> 8;                                                                             \
    (_b_)->s[(_b_)->idx + 1] = _u16_ & 0xff;                                                                           \
    (_b_)->idx += 2;                                                                                                   \
  } while (0)

#define bin_t_decode_uint16(_b_)                                                                                       \
  ({                                                                                                                   \
    bin_t_check_len(_b_, 2);                                                                                           \
    uint16_t _u16_ = (_b_)->s[(_b_)->idx] * 256 + (_b_)->s[(_b_)->idx + 1];                                            \
    (_b_)->idx += 2;                                                                                                   \
    _u16_;                                                                                                             \
  })


#define bin_t_encode_varuint(_b_, _uint_, _width_)                                                                     \
  do {                                                                                                                 \
    bin_t_check_len(_b_, _width_);                                                                                     \
    if (_width_ > 8 || _width_ < 0) {                                                                                  \
      LOG(L_CRIT, "Not implemented for %d > 8 bytes\r\n", _width_);                                                    \
      goto error;                                                                                                      \
    }                                                                                                                  \
    for (int i               = (_width_)-1; i >= 0; i--)                                                               \
      (_b_)->s[(_b_)->idx++] = (_uint_ >> (i * 8)) & 0xFF;                                                             \
  } while (0)

#define bin_t_decode_varuint(_b_, _width_)                                                                             \
  ({                                                                                                                   \
    if (_width_ > 8 || _width_ < 0) {                                                                                  \
      LOG(L_CRIT, "Not implemented for %d > 8 bytes\r\n", _width_);                                                    \
      goto error;                                                                                                      \
    }                                                                                                                  \
    bin_t_check_len(_b_, _width_);                                                                                     \
    uint64_t _uint_ = 0;                                                                                               \
    for (int i = _width_ - 1; i >= 0; i--)                                                                             \
      _uint_ += ((uint64_t)((_b_)->s[(_b_)->idx++])) << (i * 8);                                                       \
    _uint_;                                                                                                            \
  })


#define bin_t_encode_mem(_b_, _ptr_, _len_)                                                                            \
  do {                                                                                                                 \
    bin_t_check_len(_b_, _len_);                                                                                       \
    memcpy((_b_)->s + (_b_)->idx, _ptr_, _len_);                                                                       \
    (_b_)->idx += _len_;                                                                                               \
  } while (0)

#define bin_t_decode_mem(_b_, _ptr_, _len_)                                                                            \
  do {                                                                                                                 \
    bin_t_check_len(_b_, _len_);                                                                                       \
    memcpy(_ptr_, (_b_)->s + (_b_)->idx, _len_);                                                                       \
    (_b_)->idx += _len_;                                                                                               \
  } while (0)


#define str_to_bin(_s_)                                                                                                \
  ({                                                                                                                   \
    bin_t _btmp_ = {.s = (uint8_t *)(_s_).s, (_btmp_).idx = 0, (_btmp_).max = (_s_).len};                              \
    _btmp_;                                                                                                            \
  })

#define bin_to_str(_b_)                                                                                                \
  ({                                                                                                                   \
    str _s_tmp_ = {.s = (char *)(_b_).s, .len = (_b_).idx};                                                            \
    _s_tmp_;                                                                                                           \
  })


#endif
