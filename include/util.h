/* This file is part of {{ samplecontrol }}.
 * 
 * 2024 Benedict R. Gaster (cuberoo_)
 * 
 * Licensed under either of
 * Apache License, Version 2.0 (LICENSE-APACHE or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license (LICENSE-MIT or http://opensource.org/licenses/MIT)
 * at your option.
 */
#ifndef UTIL_HEADER_H
#define UTIL_HEADER_H

#include <stdio.h>
#include <stdlib.h>

#if !defined(__APPLE__) && !defined(__linux__)
#error "Currently we support only MacOS and Linux"
#endif

typedef int sc_bool;
#define TRUE 1
#define FALSE 0

typedef unsigned int sc_uint;
typedef int sc_int;
typedef unsigned short sc_ushort;
typedef short sc_short;
typedef unsigned char sc_uchar;
typedef char sc_char;
typedef float sc_float;
typedef size_t sc_size_t;
typedef void sc_void;

//------------------------------------------------------------------
// some useful functions
//------------------------------------------------------------------
#define sc_print(fmt, args...) printf(fmt, ##args)
#define sc_error(fmt, args...) fprintf(stderr, fmt, ##args)

sc_int scmp(const sc_char *a, const sc_char *b, sc_int len);
sc_int slen(const sc_char *a);
sc_char *copy(const sc_char *src, sc_char *dst, sc_char c);
sc_char *mcopy(const sc_char *src, sc_char *dst, sc_size_t len);


sc_bool is_digit(sc_int c);
sc_bool is_alpha(sc_int c);
sc_bool is_upper(sc_int c);

sc_void *sc_malloc( sc_size_t size );
sc_void sc_free(sc_void *ptr);

//------------------------------------------------------------------
// Some functions useful for adding asserts
//------------------------------------------------------------------

#define __ASSERT_LEVEL_DEBUG__ 1
#define __ASSERT_LEVEL_INFO__  2
#define __ASSERT_LEVEL_WARNING__ 3
#define __ASSERT_LEVEL_ERROR__ 4

#ifndef __ASSERT_LEVEL__
#define __ASSERT_LEVEL__ __ASSERT_LEVEL_ERROR__
#endif // __ASSERT_LEVEL__

#define ASSERT(condition, message) \
    do { \
        if (ASSERT_LEVEL >= ASSERT_LEVEL_ERROR && !(condition)) { \
            fprintf(stderr, "Assertion failed: %s\n", message); \
            abort(); \
        } else if (ASSERT_LEVEL >= ASSERT_LEVEL_WARNING && !(condition)) { \
            fprintf(stderr, "Warning: %s\n", message); \
        } else if (ASSERT_LEVEL >= ASSERT_LEVEL_INFO && !(condition)) { \
            fprintf(stdout, "Info: %s\n", message); \
        } \
    } while (0)

//------------------------------------------------------------------
// Some functions useful for debugging
//------------------------------------------------------------------

#if defined(__DEBUG__) && __DEBUG__ > 0
 #define DEBUG(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
 #define DEBUG(fmt, args...) /* Don't do anything in release builds */
#endif

#endif // UTIL_HEADER_H

