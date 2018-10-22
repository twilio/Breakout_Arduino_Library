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
 * \file mem.h - memory debugging utilities
 */

#ifndef __OWL_UTILS_MEM_H__
#define __OWL_UTILS_MEM_H__


#define WITH_MEM_DEBUG 0

#if WITH_MEM_DEBUG == 1

#include "log.h"

#define L_MEMDBG L_WARN
//#define L_MEMDBG M_MEM


#define owl_malloc(s) owl_internal_malloc((s), __FILE__, __FUNCTION__, __LINE__)
#define owl_realloc(p, s) owl_internal_relloc((p), (s), __FILE__, __FUNCTION__, __LINE__)
#define owl_free(p) owl_internal_free((p), __FILE__, __FUNCTION__, __LINE__)

#ifdef __cplusplus
extern "C" {
#endif

void *owl_internal_malloc(size_t size, const char *file, const char *func, unsigned int line);
void *owl_internal_relloc(void *ptr, size_t size, const char *file, const char *func, unsigned int line);
void owl_internal_free(void *ptr, const char *file, const char *func, unsigned int line);



#ifdef __cplusplus
}

void *operator new(size_t size, const char *file, const char *func, unsigned int line);
void *operator new[](size_t size, const char *file, const char *func, unsigned int line);

void operator delete(void *ptr) throw();
void operator delete[](void *ptr) throw();

void operator delete(void *ptr, const char *file, const char *func, unsigned int line);
void operator delete[](void *ptr, const char *file, const char *func, unsigned int line);

#define owl_new new (__FILE__, __FUNCTION__, __LINE__)
#define owl_delete delete (__FILE__, __FUNCTION__, __LINE__)

#endif

#else
#define owl_malloc(s) malloc((s))
#define owl_realloc(p, s) realloc((p), (s))
#define owl_free(p) free((p))


#define owl_new new
#define owl_delete delete

#endif

#endif
