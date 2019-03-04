/*
 * utils.h
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
 * \file mem.c - memory debugging utilities
 */

#include "mem.h"

#if WITH_MEM_DEBUG == 1

#include <stdlib.h>


extern "C" void *owl_internal_malloc(size_t size, const char *file, const char *func, unsigned int line) {
  void *ptr = malloc(size);
  if (ptr) {
    LOGF(L_MEMDBG, "MEMDBG, %p, %5d bytes,  malloc, %s/%s():%u\r\n", ptr, size, file, func, line);
    return ptr;
  } else {
    LOGF(L_ERROR, "MEMDBG, ERR-MALLOC, %5d bytes,  malloc, %s/%s():%u\r\n", size, file, func, line);
    return 0;
  }
}

extern "C" void *owl_internal_relloc(void *ptr, size_t size, const char *file, const char *func, unsigned int line) {
  void *new_ptr = realloc(ptr, size);
  if (new_ptr) {
    LOGF(L_MEMDBG, "MEMDBG, %p, %5d bytes, realloc (%p), %s/%s():%u\r\n", new_ptr, size, ptr, file, func, line);
    return new_ptr;
  } else {
    LOGF(L_MEMDBG, "MEMDBG, ERRREALLOC, %5d bytes, realloc (%p), %s/%s():%u\r\n", size, ptr, file, func, line);
    return 0;
  }
}

extern "C" void owl_internal_free(void *ptr, const char *file, const char *func, unsigned int line) {
  if (ptr) free(ptr);
  LOGF(L_MEMDBG, "MEMDBG, %p,            ,    free, %s/%s():%u\r\n", ptr, file, func, line);
}


void *operator new(size_t size, const char *file, const char *func, unsigned int line) {
  void *ptr = malloc(size);
  if (ptr) {
    LOGF(L_MEMDBG, "MEMDBG, %p, %5d bytes,     new, %s/%s():%u\r\n", ptr, size, file, func, line);
    return ptr;
  } else {
    LOGF(L_ERROR, "MEMDBG, ERR-MALLOC, %5d bytes,     new, %s/%s():%u\r\n", size, file, func, line);
    return 0;
  }
  //  if (ptr == nullptr) throw std::bad_alloc { };
  return ptr;
}

void *operator new[](size_t size, const char *file, const char *func, unsigned int line) {
  void *ptr = malloc(size);
  if (ptr) {
    LOGF(L_MEMDBG, "MEMDBG, %p, %5d bytes,     new, %s/%s():%u\r\n", ptr, size, file, func, line);
    return ptr;
  } else {
    LOGF(L_ERROR, "MEMDBG, ERR-MALLOC, %5d bytes,     new, %s/%s():%u\r\n", size, file, func, line);
    return 0;
  }
  //  if (ptr == nullptr) throw std::bad_alloc { };
  return ptr;
}


void operator delete(void *ptr) throw() {
  if (ptr) free(ptr);
  LOGF(L_MEMDBG, "MEMDBG, %p,            ,  delete, %s/%s():%u\r\n", ptr, __FILE__, __FUNCTION__, __LINE__);
}

void operator delete[](void *ptr) throw() {
  if (ptr) free(ptr);
  LOGF(L_MEMDBG, "MEMDBG, %p,            ,  delete, %s/%s():%u\r\n", ptr, __FILE__, __FUNCTION__, __LINE__);
}


void operator delete(void *ptr, const char *file, const char *func, unsigned int line) throw() {
  if (ptr) free(ptr);
  LOGF(L_MEMDBG, "MEMDBG, %p,            ,  delete, %s/%s():%u\r\n", ptr, file, func, line);
}

void operator delete[](void *ptr, const char *file, const char *func, unsigned int line) throw() {
  if (ptr) free(ptr);
  LOGF(L_MEMDBG, "MEMDBG, %p,            ,  delete, %s/%s():%u\r\n", ptr, file, func, line);
}

#endif
