/*
 * typedefs.h
 * ----------
 * Purpose: Basic data type definitions and assorted compiler-related helpers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once



OPENMPT_NAMESPACE_BEGIN



#if MPT_COMPILER_MSVC
#pragma warning(error : 4309) // Treat "truncation of constant value"-warning as error.
#endif



#if MPT_COMPILER_MSVC && MPT_MSVC_AT_LEAST(2010,0)
#define MPT_COMPILER_HAS_RVALUE_REF
#endif



#if MPT_COMPILER_MSVC && MPT_MSVC_AT_LEAST(2010,0)
#define HAS_TYPE_TRAITS
#endif



#if MPT_COMPILER_GCC || MPT_COMPILER_MSVC
// Compiler supports type-punning through unions. This is not stricly standard-conforming.
// For GCC, this is documented, for MSVC this is apparently not documented, but we assume it.
#define MPT_COMPILER_UNION_TYPE_ALIASES 1
#endif

#ifndef MPT_COMPILER_UNION_TYPE_ALIASES
// Compiler does not support type-punning through unions. std::memcpy is used instead.
// This is the safe fallback and strictly standard-conforming.
// Another standard-compliant alternative would be casting pointers to a character type pointer.
// This results in rather unreadable code and,
// in most cases, compilers generate better code by just inlining the memcpy anyway.
// (see <http://blog.regehr.org/archives/959>).
#define MPT_COMPILER_UNION_TYPE_ALIASES 0
#endif



// Platform has native IEEE floating point representation.
// (Currently always assumed)
#define MPT_PLATFORM_IEEE_FLOAT 1



#if MPT_COMPILER_MSVC

#if MPT_MSVC_BEFORE(2010,0)
#define nullptr 0
#endif

#elif MPT_COMPILER_GCC

#if MPT_GCC_BEFORE(4,6,0)
#define nullptr 0
#endif

#endif



//  CountOf macro computes the number of elements in a statically-allocated array.
#if MPT_COMPILER_MSVC
	#define CountOf(x) _countof(x)
#else
	#define CountOf(x) (sizeof((x))/sizeof((x)[0]))
#endif



#if MPT_COMPILER_MSVC
#define PACKED __declspec(align(1))
#define NEEDS_PRAGMA_PACK
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#if MPT_COMPILER_GCC && MPT_OS_WINDOWS
// Some versions of mingw64 need this when windows-hosted. Strange.
#define NEEDS_PRAGMA_PACK
#endif
#define PACKED __attribute__((packed)) __attribute__((aligned(1)))
#else
#define PACKED alignas(1)
#endif



#if MPT_COMPILER_MSVC
#define ALIGN(n) __declspec(align(n))
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define ALIGN(n) __attribute__((aligned(n)))
#else
#define ALIGN(n) alignas(n)
#endif



// Advanced inline attributes
#if MPT_COMPILER_MSVC
#define forceinline __forceinline
#define noinline __declspec(noinline)
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define forceinline __attribute__((always_inline)) inline
#define noinline __attribute__((noinline))
#else
#define forceinline inline
#define noinline
#endif



// Use MPT_RESTRICT to indicate that a pointer is guaranteed to not be aliased.
#if MPT_COMPILER_MSVC || MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_RESTRICT __restrict
#else
#define MPT_RESTRICT
#endif



// Some functions might be deprecated although they are still in use.
// Tag them with "MPT_DEPRECATED".
#if MPT_COMPILER_MSVC
#define MPT_DEPRECATED __declspec(deprecated)
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_DEPRECATED __attribute__((deprecated))
#else
#define MPT_DEPRECATED
#endif
#if defined(MODPLUG_TRACKER)
#define MPT_DEPRECATED_TRACKER    MPT_DEPRECATED
#define MPT_DEPRECATED_LIBOPENMPT 
#elif defined(LIBOPENMPT_BUILD)
#define MPT_DEPRECATED_TRACKER    
#define MPT_DEPRECATED_LIBOPENMPT MPT_DEPRECATED
#else
#define MPT_DEPRECATED_TRACKER    MPT_DEPRECATED
#define MPT_DEPRECATED_LIBOPENMPT MPT_DEPRECATED
#endif



// Exception type that is used to catch "operator new" exceptions.
#if defined(_MFC_VER)
typedef CMemoryException * MPTMemoryException;
#else
OPENMPT_NAMESPACE_END
#include <new>
OPENMPT_NAMESPACE_BEGIN
typedef std::bad_alloc & MPTMemoryException;
#endif



OPENMPT_NAMESPACE_END
#include <memory>
OPENMPT_NAMESPACE_BEGIN
#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)
#define MPT_SHARED_PTR std::tr1::shared_ptr
#else
#define MPT_SHARED_PTR std::shared_ptr
#endif



#if defined(_MFC_VER)

#if !defined(ASSERT)
#error "MFC is expected to #define ASSERT"
#endif // !defined(ASERRT)
#define MPT_ASSERT_IS_DEFINED

#if defined(_DEBUG)
 #define MPT_ASSERT_IS_ACTIVE 1
#else // !_DEBUG
 #define MPT_ASSERT_IS_ACTIVE 0
#endif // _DEBUG

#else // !_MFC_VER

#if defined(ASSERT)
#error "ASSERT(expr) is expected to NOT be defined by any other header"
#endif // !defined(ASERRT)

#endif // _MFC_VER


#if defined(MPT_ASSERT_IS_DEFINED)

//#define ASSERT // already defined
#define ASSERT_WARN_MESSAGE(expr,msg) ASSERT(expr)

#elif defined(NO_ASSERTS)

#define MPT_ASSERT_IS_DEFINED
#define MPT_ASSERT_IS_ACTIVE 0
#define ASSERT(expr) do { } while(0)
#define ASSERT_WARN_MESSAGE(expr,msg) do { } while(0)

#else // !NO_ASSERTS

#define MPT_ASSERT_IS_DEFINED
#define MPT_ASSERT_IS_ACTIVE 1
#define ASSERT(expr) do { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr); } } while(0)
#define ASSERT_WARN_MESSAGE(expr,msg) do { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr, msg); } } while(0)
#ifndef MPT_ASSERT_HANDLER_NEEDED
#define MPT_ASSERT_HANDLER_NEEDED
#endif

#endif // NO_ASSERTS

// error checking
#if !defined(MPT_ASSERT_IS_DEFINED)
#error "ASSERT(expr) has to be defined"
#endif // !MPT_ASSERT_IS_DEFINED
#if MPT_ASSERT_IS_ACTIVE && defined(NO_ASSERTS)
#error "ASSERT is active but NO_ASSERT is defined"
#elif !MPT_ASSERT_IS_ACTIVE && !defined(NO_ASSERTS)
#error "NO_ASSERT is not defined but ASSERTs are not active"
#endif


#if (MPT_ASSERT_IS_ACTIVE == 1)

#define ALWAYS_ASSERT(expr) ASSERT(expr)
#define ALWAYS_ASSERT_WARN_MESSAGE(expr,msg) ASSERT_WARN_MESSAGE(expr,msg)

#else // (MPT_ASSERT_IS_ACTIVE != 1)

#define ALWAYS_ASSERT(expr) do { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr); } } while(0)
#define ALWAYS_ASSERT_WARN_MESSAGE(expr,msg) do { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr, msg); } } while(0)
#ifndef MPT_ASSERT_HANDLER_NEEDED
#define MPT_ASSERT_HANDLER_NEEDED
#endif

#endif // MPT_ASSERT_IS_ACTIVE


#if defined(MPT_ASSERT_HANDLER_NEEDED)
// custom assert handler needed
noinline void AssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg=nullptr);
#endif // MPT_ASSERT_HANDLER_NEEDED


// Compile time assert.
#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)
	#define static_assert(expr, msg) typedef char OPENMPT_STATIC_ASSERT[(expr)?1:-1]
#endif
#define STATIC_ASSERT(expr) static_assert((expr), "compile time assertion failed: " #expr)


// Macro for marking intentional fall-throughs in switch statements - can be used for static analysis if supported.
#if MPT_COMPILER_MSVC
#define MPT_FALLTHROUGH __fallthrough
#elif MPT_COMPILER_CLANG
#define MPT_FALLTHROUGH [[clang::fallthrough]]
#else
#define MPT_FALLTHROUGH do { } while(0)
#endif


OPENMPT_NAMESPACE_END
#include <cstdarg>
OPENMPT_NAMESPACE_BEGIN
#if MPT_COMPILER_MSVC
#ifndef va_copy
#define va_copy(dst, src) do { (dst) = (src); } while (0)
#endif
#endif



#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)

OPENMPT_NAMESPACE_END
#include "stdint.h"
OPENMPT_NAMESPACE_BEGIN

typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#else

OPENMPT_NAMESPACE_END
#include <cstdint>
OPENMPT_NAMESPACE_BEGIN

typedef std::int8_t   int8;
typedef std::int16_t  int16;
typedef std::int32_t  int32;
typedef std::int64_t  int64;
typedef std::uint8_t  uint8;
typedef std::uint16_t uint16;
typedef std::uint32_t uint32;
typedef std::uint64_t uint64;

#endif

#ifdef ANDROID

OPENMPT_NAMESPACE_END
#include <stdint.h>
OPENMPT_NAMESPACE_BEGIN

// Android NDK appears to provide a different (and incomplete) <stdint.h> when compiling C++.
// Provide these macros ourselves if they are not defined by here.

#ifndef INT8_MIN
#define INT8_MIN (-128)
#endif
#ifndef INT8_MAX
#define INT8_MAX (127)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX (255)
#endif

#ifndef INT16_MIN
#define INT16_MIN (-32768)
#endif
#ifndef INT16_MAX
#define INT16_MAX (32767)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX (65535)
#endif

#ifndef INT32_MIN
#define INT32_MIN (-2147483647-1)
#endif
#ifndef INT32_MAX
#define INT32_MAX (2147483647)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif

#ifndef INT64_MIN
#define INT64_MIN ((-9223372036854775807ll)-1)
#endif
#ifndef INT64_MAX
#define INT64_MAX ((9223372036854775807ll))
#endif
#ifndef UINT64_MAX
#define UINT64_MAX ((18446744073709551615ull))
#endif

#endif // ANDROID

const int8 int8_min     = INT8_MIN;
const int16 int16_min   = INT16_MIN;
const int32 int32_min   = INT32_MIN;
const int64 int64_min   = INT64_MIN;

const int8 int8_max     = INT8_MAX;
const int16 int16_max   = INT16_MAX;
const int32 int32_max   = INT32_MAX;
const int64 int64_max   = INT64_MAX;

const uint8 uint8_max   = UINT8_MAX;
const uint16 uint16_max = UINT16_MAX;
const uint32 uint32_max = UINT32_MAX;
const uint64 uint64_max = UINT64_MAX;


// 24-bit integer wrapper (for 24-bit PCM)
struct int24
{
	uint8 bytes[3];
	int24() { bytes[0] = bytes[1] = bytes[2] = 0; }
	explicit int24(int other)
	{
		#ifdef MPT_PLATFORM_BIG_ENDIAN
			bytes[0] = (static_cast<unsigned int>(other)>>16)&0xff;
			bytes[1] = (static_cast<unsigned int>(other)>> 8)&0xff;
			bytes[2] = (static_cast<unsigned int>(other)>> 0)&0xff;
		#else
			bytes[0] = (static_cast<unsigned int>(other)>> 0)&0xff;
			bytes[1] = (static_cast<unsigned int>(other)>> 8)&0xff;
			bytes[2] = (static_cast<unsigned int>(other)>>16)&0xff;
		#endif
	}
	operator int() const
	{
		#ifdef MPT_PLATFORM_BIG_ENDIAN
			return (static_cast<int8>(bytes[0]) * 65536) + (bytes[1] * 256) + bytes[2];
		#else
			return (static_cast<int8>(bytes[2]) * 65536) + (bytes[1] * 256) + bytes[0];
		#endif
	}
};
STATIC_ASSERT(sizeof(int24) == 3);
#define int24_min (0-0x00800000)
#define int24_max (0+0x007fffff)


typedef float float32;
STATIC_ASSERT(sizeof(float32) == 4);



#if !defined(MPT_USE_WINDOWS_H)

// openmpt assumes these type have exact WIN32 semantics

namespace mpt { namespace Legacy {
typedef std::uint8_t  BYTE;
typedef std::uint16_t WORD;
typedef std::uint32_t DWORD;
typedef std::int32_t  LONG;
typedef std::uint32_t UINT;
} } // namespace mpt::Legacy
using namespace mpt::Legacy;

#endif // !MPT_USE_WINDOWS_H




#if MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_PRINTF_FUNC(formatstringindex,varargsindex) __attribute__((format(printf, formatstringindex, varargsindex)))
#else
#define MPT_PRINTF_FUNC(formatstringindex,varargsindex)
#endif

//To mark string that should be translated in case of multilingual version.
#define GetStrI18N(x)	(x)

#define MULTICHAR4_LE_MSVC(a,b,c,d) static_cast<uint32>( (static_cast<uint8>(a) << 24) | (static_cast<uint8>(b) << 16) | (static_cast<uint8>(c) << 8) | (static_cast<uint8>(d) << 0) )



//STRINGIZE makes a string of given argument. If used with #defined value,
//the string is made of the contents of the defined value.
#define HELPER_STRINGIZE(x)			#x
#define STRINGIZE(x)				HELPER_STRINGIZE(x)



#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif



#if MPT_COMPILER_MSVC && defined(UNREFERENCED_PARAMETER)
#define MPT_UNREFERENCED_PARAMETER(x) UNREFERENCED_PARAMETER(x)
#else
#define MPT_UNREFERENCED_PARAMETER(x) (void)(x)
#endif



OPENMPT_NAMESPACE_END
