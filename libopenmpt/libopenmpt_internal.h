/*
 * libopenmpt_internal.h
 * ---------------------
 * Purpose: libopenmpt internal interface configuration, overruling the public interface configuration (only used and needed when building libopenmpt)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_INTERNAL_H
#define LIBOPENMPT_INTERNAL_H

#include "libopenmpt_config.h"

#if defined(NO_LIBOPENMPT_C)
#undef LIBOPENMPT_API
#define LIBOPENMPT_API     LIBOPENMPT_API_HELPER_LOCAL
#endif

#if defined(NO_LIBOPENMPT_CXX)
#undef LIBOPENMPT_CXX_API
#define LIBOPENMPT_CXX_API LIBOPENMPT_API_HELPER_LOCAL
#endif

#ifdef __cplusplus
#if defined(LIBOPENMPT_BUILD_DLL) || defined(LIBOPENMPT_USE_DLL)
#if defined(_MSC_VER) && !defined(_DLL)
/* #pragma message( "libopenmpt C++ interface is disabled if libopenmpt is built as a DLL and the runtime is statically linked. This is not supported by microsoft and cannot possibly work. Ever." ) */
#undef LIBOPENMPT_CXX_API
#define LIBOPENMPT_CXX_API LIBOPENMPT_API_HELPER_LOCAL
#endif
#endif
#endif


#if defined(_MSC_VER)
#if (_MSC_VER >= 1500) && (_MSC_VER < 1600)
#ifndef LIBOPENMPT_ANCIENT_COMPILER
#define LIBOPENMPT_ANCIENT_COMPILER
#endif
#ifndef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
#define LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
#define LIBOPENMPT_SHARED_PTR std::tr1::shared_ptr
#endif
#endif
#endif

#if defined(__GNUC__) && !defined(__clang__)
#if (__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__*1 < 40300)
#ifndef LIBOPENMPT_ANCIENT_COMPILER
#define LIBOPENMPT_ANCIENT_COMPILER
#endif
#ifndef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
#define LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
#define LIBOPENMPT_SHARED_PTR std::tr1::shared_ptr
#endif
#elif (__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__*1 < 40400)
#ifndef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
#define LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
#define LIBOPENMPT_SHARED_PTR std::shared_ptr 
#endif
#endif
#endif


#ifdef __cplusplus
#ifdef LIBOPENMPT_QUIRK_NO_CSTDINT
#include <stdint.h>
namespace std {
typedef ::int8_t   int8_t;
typedef ::int16_t  int16_t;
typedef ::int32_t  int32_t;
typedef ::int64_t  int64_t;
typedef ::uint8_t  uint8_t; 
typedef ::uint16_t uint16_t; 
typedef ::uint32_t uint32_t;
typedef ::uint64_t uint64_t;
}
#endif
#endif


#endif // LIBOPENMPT_INTERNAL_H
