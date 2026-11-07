/*
 *      Machine dependent defines/includes for LAME.
 *
 *      Copyright (c) 1999 A.L. Faber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef LAME_MACHINE_H
#define LAME_MACHINE_H

#include "version.h"

#include <stdio.h>
#include <assert.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char   *strchr(), *strrchr();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if  defined(__riscos__)  &&  defined(FPA10)
# include "ymath.h"
#else
# include <math.h>
#endif
#include <limits.h>

/* Mark a function parameter that is intentionally unused (e.g. in a stub
   implementation or a compile-time-selected code path). Expands to nothing
   where the compiler has no such attribute, including MSVC. */
#if defined(__GNUC__) || defined(__clang__)
# define LAME_UNUSED __attribute__((unused))
#else
# define LAME_UNUSED
#endif

#include <ctype.h>

#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if defined(macintosh)
# include <types.h>
# include <stat.h>
#else
# include <sys/types.h>
# include <sys/stat.h>
#endif

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#else
# ifdef HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/* in case this is used without configure */
#ifndef inline
# define inline
#endif

#if defined(_MSC_VER)
# undef inline
# define inline _inline
#elif defined(__SASC) || defined(__GNUC__) || defined(__ICC) || defined(__ECC)
/* if __GNUC__ we always want to inline, not only if the user requests it */
# undef inline
# define inline __inline
#endif

#if    defined(_MSC_VER)
# pragma warning( disable : 4244 )
/*# pragma warning( disable : 4305 ) */
/* 4100: unreferenced formal parameter. Intentional in stub/compatibility
   entry points; the GCC/Clang -Wunused-parameter build is where a genuinely
   unused parameter is caught (see LAME_UNUSED). */
# pragma warning( disable : 4100 )
#endif

/*
 * FLOAT    for variables which require at least 32 bits
 * FLOAT8   for variables which require at least 64 bits
 *
 * On some machines, 64 bit will be faster than 32 bit.  Also, some math
 * routines require 64 bit float, so setting FLOAT=float will result in a
 * lot of conversions.
 */

#if ( defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MINGW32__) )
# include <float.h>
typedef float FLOAT;
# define FLOAT_MAX FLT_MAX
#else
# ifndef FLOAT
typedef float FLOAT;
#  ifdef FLT_MAX
#   define FLOAT_MAX FLT_MAX
#  else
#   define FLOAT_MAX 1e37 /* approx */
#  endif
# endif
#endif

/* sample_t must be floating point, at least 32 bits */
typedef FLOAT sample_t;

#define dimension_of(array) (sizeof(array)/sizeof(array[0]))
/* ##-pasting does not expand its operands, so pasting __LINE__ directly would
 * yield the literal token "__LINE__" instead of the current line number,
 * making every use in a translation unit collide on the same enum tag. The
 * extra indirection through compiletime_assert_paste() forces __LINE__ to be
 * expanded to its numeric value before compiletime_assert_paste2() pastes it. */
#define compiletime_assert_paste2(a,b) a##b
#define compiletime_assert_paste(a,b) compiletime_assert_paste2(a,b)
#define compiletime_assert(expression) enum{compiletime_assert_paste(static_assert_line_,__LINE__) = 1/((expression)?1:0)}
#define lame_calloc(TYPE, COUNT) ((TYPE*)calloc(COUNT, sizeof(TYPE)))
#define multiple_of(CHUNK, COUNT) (\
  ( (COUNT) < 1 || (CHUNK) < 1 || (COUNT) % (CHUNK) == 0 ) \
  ? (COUNT) \
  : ((COUNT) + (CHUNK) - (COUNT) % (CHUNK)) \
  )

#if 1
#define EQ(a,b) (\
(fabs(a) > fabs(b)) \
 ? (fabs((a)-(b)) <= (fabs(a) * 1e-6f)) \
 : (fabs((a)-(b)) <= (fabs(b) * 1e-6f)))
#else
#define EQ(a,b) (fabs((a)-(b))<1E-37)
#endif

#define NEQ(a,b) (!EQ(a,b))

#ifdef _MSC_VER
#  if _MSC_VER < 1400
#  define fabsf fabs
#  define powf pow
#  define log10f log10
#  endif
#endif

/**
 *  \internal
 *  Whether a float is a finite number. An IEEE-754 value is NaN or infinite
 *  exactly when every exponent bit is set.
 *
 *  LAME is built with -ffast-math. With this option, the compiler may assume
 *  that NaN and infinity never occur, and may remove a test for them. Reading
 *  the raw bits does not prevent this. The compiler recognizes the integer
 *  comparison below as a floating-point class test and replaces it with a
 *  constant, as it does for isfinite() or x!=x. So the value is first stored
 *  in a volatile object. The compiler may not make assumptions about a
 *  volatile object.
 *
 *  @param x  the value to test.
 *  @return nonzero when @p x is neither NaN nor an infinity.
 */
static inline int
float_is_finite(float const x)
{
    float volatile opaque = x;
    float   value;
    uint32_t bits;

    value = opaque;
    memcpy(&bits, &value, sizeof bits);
    return ((bits >> 23) & 0xFFu) != 0xFFu;
}

/**
 *  \internal
 *  float_is_finite() for a double.
 *
 *  @param x  the value to test.
 *  @return nonzero when @p x is neither NaN nor an infinity.
 */
static inline int
double_is_finite(double const x)
{
    double volatile opaque = x;
    double  value;
    uint64_t bits;

    value = opaque;
    memcpy(&bits, &value, sizeof bits);
    return ((bits >> 52) & 0x7FFu) != 0x7FFu;
}

#endif

/* end of machine.h */
