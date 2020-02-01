/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

/* Include some files for defining library routines */
#include <string.h>
#include <sys/time.h>

#define LWIP_TIMEVAL_PRIVATE 0

/* Define platform endianness
 * NOTE: We make sure we are building for little endian by using TEST_BIG_ENDIAN in CMakeLists.txt.
 */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

/* Compiler hints for packing structures */
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

/* prototypes for printf() and abort() */
#include <stdio.h>
#include <stdlib.h>

/* Disable assertions only in release builds */
#ifdef NDEBUG
#define LWIP_NOASSERT 1
#endif

/* Plaform specific diagnostic output */
#ifdef PLATFORM_ANDROID
#include <android/log.h>
// We have to use two macros because the argument(s) passed to LWIP_PLATFORM_DIAG is surrounded by brackets
#define ANDROID_PLATFORM_DIAG(...) __android_log_print ( ANDROID_LOG_DEBUG, "lwip", __VA_ARGS__ )
// Also __android_log_print already appends a newline, so don't need to print one after
#define LWIP_PLATFORM_DIAG(x) do { ANDROID_PLATFORM_DIAG x; } while(0)
#elif !defined(NO_LOGGING)
#define NATIVE_PLATFORM_DIAG(...) fprintf ( stderr, __VA_ARGS__ )
#define LWIP_PLATFORM_DIAG(x) do { NATIVE_PLATFORM_DIAG x; fprintf(stderr, "\n"); } while(0)
#else
#define LWIP_PLATFORM_DIAG(x)
#endif

#ifdef LWIP_UNIX_EMPTY_ASSERT
#define LWIP_PLATFORM_ASSERT(x)
#else
#define LWIP_PLATFORM_ASSERT(x) do {printf("Assertion \"%s\" failed at line %d in %s\n", \
                                     x, __LINE__, __FILE__); fflush(NULL); abort();} while(0)
#endif

#define LWIP_RAND() ((u32_t)rand())

struct sio_status_s;
typedef struct sio_status_s sio_status_t;
#define sio_fd_t sio_status_t*
#define __sio_fd_t_defined

#endif /* LWIP_ARCH_CC_H */
