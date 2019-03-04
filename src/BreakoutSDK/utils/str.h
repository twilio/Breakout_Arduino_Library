/*
 * str.h
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
 * \file str.h - string and operations - not null terminated
 */

#ifndef __OWL_UTILS_STR_H__
#define __OWL_UTILS_STR_H__

#include <stdint.h>
#include <string.h>


/* str type & friends */

typedef struct {
  char *s;
  int len;
} str;



#define STRDEF(_string_, _value_) ((_string_).s = (_value_), (_string_).len = strlen(_value_))

#define STRDECL(_value_)                                                                                               \
  { .s = (_value_), .len = strlen(_value_) }



#define str_equal(a, b) ((a).len == (b).len && memcmp((a).s, (b).s, (a).len) == 0)
#define str_equal_char(a, c) ((a).len == strlen(c) && memcmp((a).s, (c), (a).len) == 0)
#define str_equal_prefix(a, p) ((a).len >= (p).len && memcmp((a).s, (p).s, (p).len) == 0)
#define str_equal_prefix_char(a, p) ((a).len >= strlen(p) && memcmp((a).s, (p), strlen(p)) == 0)

#define str_equalcase(a, b) ((a).len == (b).len && strncasecmp((a).s, (b).s, (a).len) == 0)
#define str_equalcase_char(a, c) ((a).len == strlen(c) && strncasecmp((a).s, (c), (a).len) == 0)
#define str_equalcase_prefix(a, p) ((a).len >= (p).len && strncasecmp((a).s, (p).s, (p).len) == 0)
#define str_equalcase_prefix_char(a, p) ((a).len >= strlen(p) && strncasecmp((a).s, (p), strlen(p)) == 0)


#define str_free(x)                                                                                                    \
  do {                                                                                                                 \
    if ((x).s) owl_free((x).s);                                                                                        \
    (x).s   = 0;                                                                                                       \
    (x).len = 0;                                                                                                       \
  } while (0)

#define str_dup(dst, src)                                                                                              \
  do {                                                                                                                 \
    if ((src).len) {                                                                                                   \
      (dst).s = (char *)owl_malloc((src).len);                                                                         \
      if (!(dst).s) {                                                                                                  \
        LOG(L_ERROR, "Error allocating %d bytes\r\n", (src).len);                                                        \
        (dst).len = 0;                                                                                                 \
        goto out_of_memory;                                                                                            \
      }                                                                                                                \
      memcpy((dst).s, (src).s, (src).len);                                                                             \
      (dst).len = (src).len;                                                                                           \
    } else {                                                                                                           \
      (dst).s   = 0;                                                                                                   \
      (dst).len = 0;                                                                                                   \
    }                                                                                                                  \
  } while (0)

#define str_dup_safe(dst, src)                                                                                         \
  do {                                                                                                                 \
    str_free(dst);                                                                                                     \
    str_dup(dst, src);                                                                                                 \
  } while (0)



#define str_shrink_inside(dst, startp, size)                                                                           \
  do {                                                                                                                 \
    int before_len = startp - (dst).s;                                                                                 \
    int after_len  = (dst).len - before_len - (size);                                                                  \
    if (after_len > 0) {                                                                                               \
      memmove(startp, (startp) + (size), after_len);                                                                   \
      (dst).len -= size;                                                                                               \
    } else if (after_len == 0) {                                                                                       \
      (dst).len -= size;                                                                                               \
    } else {                                                                                                           \
      LOG(L_ERROR, "Bad len calculation %d\r\n", after_len);                                                             \
    }                                                                                                                  \
  } while (0)



#ifdef __cplusplus
extern "C" {
#endif


void str_remove_prefix(str *x, char *prefix);
void str_skipover_prefix(str *x, str prefix);
int str_tok(str src, char *sep, str *dst);
int str_tok_with_empty_tokens(str src, char *sep, str *dst);
long int str_to_long_int(str x, int base);
uint32_t str_to_uint32_t(str x, int base);
double str_to_double(str x);

int hex_to_int(char c);

int hex_to_str(char *dst, int max_dst_len, str src);
int str_to_hex(char *dst, int max_dst_len, str src);

int str_find(str x, str y);
int str_find_char(str x, char *y);


#ifdef __cplusplus
}
#endif


#endif
