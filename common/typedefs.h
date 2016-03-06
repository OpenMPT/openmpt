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



// Advanced inline attributes
#if MPT_COMPILER_MSVC
#define forceinline __forceinline
#define MPT_NOINLINE __declspec(noinline)
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define forceinline __attribute__((always_inline)) inline
#define MPT_NOINLINE __attribute__((noinline))
#else
#define forceinline inline
#define MPT_NOINLINE
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



// For mpt::make_shared<T>, we sacrifice perfect forwarding in order to keep things simple here.
// Templated for up to 4 parameters. Add more when required.

OPENMPT_NAMESPACE_END
#include <memory>
OPENMPT_NAMESPACE_BEGIN

#if MPT_COMPILER_GCC && MPT_GCC_BEFORE(4,3,0)
OPENMPT_NAMESPACE_END
#include <tr1/memory>
OPENMPT_NAMESPACE_BEGIN
#endif

#if (MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)) || (MPT_COMPILER_GCC && MPT_GCC_BEFORE(4,3,0))

#define MPT_SHARED_PTR std::tr1::shared_ptr
#define MPT_CONST_POINTER_CAST std::tr1::const_pointer_cast
#define MPT_STATIC_POINTER_CAST std::tr1::static_pointer_cast
#define MPT_DYNAMIC_POINTER_CAST std::tr1::dynamic_pointer_cast
namespace mpt {
template <typename T> inline MPT_SHARED_PTR<T> make_shared() { return MPT_SHARED_PTR<T>(new T()); }
template <typename T, typename T1> inline MPT_SHARED_PTR<T> make_shared(const T1 &x1) { return MPT_SHARED_PTR<T>(new T(x1)); }
template <typename T, typename T1, typename T2> inline MPT_SHARED_PTR<T> make_shared(const T1 &x1, const T2 &x2) { return MPT_SHARED_PTR<T>(new T(x1, x2)); }
template <typename T, typename T1, typename T2, typename T3> inline MPT_SHARED_PTR<T> make_shared(const T1 &x1, const T2 &x2, const T3 &x3) { return MPT_SHARED_PTR<T>(new T(x1, x2, x3)); }
template <typename T, typename T1, typename T2, typename T3, typename T4> inline MPT_SHARED_PTR<T> make_shared(const T1 &x1, const T2 &x2, const T3 &x3, const T4 &x4) { return MPT_SHARED_PTR<T>(new T(x1, x2, x3, x4)); }
} // namespace mpt

#else

#define MPT_SHARED_PTR std::shared_ptr
#define MPT_CONST_POINTER_CAST std::const_pointer_cast
#define MPT_STATIC_POINTER_CAST std::static_pointer_cast
#define MPT_DYNAMIC_POINTER_CAST std::dynamic_pointer_cast
namespace mpt {
template <typename T> inline MPT_SHARED_PTR<T> make_shared() { return std::make_shared<T>(); }
template <typename T, typename T1> inline MPT_SHARED_PTR<T> make_shared(const T1 &x1) { return std::make_shared<T>(x1); }
template <typename T, typename T1, typename T2> inline MPT_SHARED_PTR<T> make_shared(const T1 &x1, const T2 &x2) { return std::make_shared<T>(x1, x2); }
template <typename T, typename T1, typename T2, typename T3> inline MPT_SHARED_PTR<T> make_shared(const T1 &x1, const T2 &x2, const T3 &x3) { return std::make_shared<T>(x1, x2, x3); }
template <typename T, typename T1, typename T2, typename T3, typename T4> inline MPT_SHARED_PTR<T> make_shared(const T1 &x1, const T2 &x2, const T3 &x3, const T4 &x4) { return std::make_shared<T>(x1, x2, x3, x4); }
} // namespace mpt

#endif



#if MPT_COMPILER_MSVC
#define MPT_CONSTANT_IF(x) \
  __pragma(warning(push)) \
  __pragma(warning(disable:4127)) \
  if(x) \
  __pragma(warning(pop)) \
/**/
#define MPT_MAYBE_CONSTANT_IF(x) \
  __pragma(warning(push)) \
  __pragma(warning(disable:4127)) \
  if(x) \
  __pragma(warning(pop)) \
/**/
#endif

#if MPT_COMPILER_GCC
#if MPT_GCC_AT_LEAST(4,6,0)
#define MPT_MAYBE_CONSTANT_IF(x) \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wtype-limits\"") \
  if(x) \
  _Pragma("GCC diagnostic pop") \
/**/
#elif MPT_GCC_AT_LEAST(4,5,0)
#define MPT_MAYBE_CONSTANT_IF(x) \
  _Pragma("GCC diagnostic ignored \"-Wtype-limits\"") \
  if(x) \
/**/
#elif MPT_GCC_AT_LEAST(4,4,0)
// GCC 4.4 does not like _Pragma diagnostic inside functions.
// As GCC 4.4 is one of our major compilers, we do not want a noisy build.
// Thus, just disable this warning globally. (not required for now)
//#pragma GCC diagnostic ignored "-Wtype-limits"
#endif
#endif

#if MPT_COMPILER_CLANG
#define MPT_MAYBE_CONSTANT_IF(x) \
  _Pragma("clang diagnostic push") \
  _Pragma("clang diagnostic ignored \"-Wtype-limits\"") \
  if(x) \
  _Pragma("clang diagnostic pop") \
/**/
#endif

#if !defined(MPT_CONSTANT_IF)
// MPT_CONSTANT_IF disables compiler warnings for conditions that are either always true or always false for some reason (dependent on template arguments for example)
#define MPT_CONSTANT_IF(x) if(x)
#endif

#if !defined(MPT_MAYBE_CONSTANT_IF)
// MPT_MAYBE_CONSTANT_IF disables compiler warnings for conditions that may in some case be either always false or always true (this may turn out to be useful in ASSERTions in some cases).
#define MPT_MAYBE_CONSTANT_IF(x) if(x)
#endif



#if MPT_COMPILER_MSVC
// MSVC warns for the well-known and widespread "do { } while(0)" idiom with warning level 4 ("conditional expression is constant").
// It does not warn with "while(0,0)". However this again causes warnings with other compilers.
// Solve it with a macro.
#define MPT_DO do
#define MPT_WHILE_0 while(0,0)
#endif

#ifndef MPT_DO
#define MPT_DO do
#endif
#ifndef MPT_WHILE_0
#define MPT_WHILE_0 while(0)
#endif



// Static code checkers might need to get the knowledge of our assertions transferred to them.
#define MPT_CHECKER_ASSUME_ASSERTIONS 1
//#define MPT_CHECKER_ASSUME_ASSERTIONS 0

#if MPT_COMPILER_MSVC
#ifdef MPT_BUILD_ANALYZED
#if MPT_CHECKER_ASSUME_ASSERTIONS
#define MPT_CHECKER_ASSUME(x) __analysis_assume(!!(x))
#endif
#endif
#endif

#ifndef MPT_CHECKER_ASSUME
#define MPT_CHECKER_ASSUME(x) MPT_DO { } MPT_WHILE_0
#endif



#if defined(_MFC_VER)

#if !defined(ASSERT)
#error "MFC is expected to #define ASSERT"
#endif // !defined(ASERRT)
#define MPT_FRAMEWORK_ASSERT_IS_DEFINED

#if defined(_DEBUG)
 #define MPT_FRAMEWORK_ASSERT_IS_ACTIVE 1
#else // !_DEBUG
 #define MPT_FRAMEWORK_ASSERT_IS_ACTIVE 0
#endif // _DEBUG

// let MFC handle our asserts
#define MPT_ASSERT_USE_FRAMEWORK 1

#else // !_MFC_VER

#if defined(ASSERT)
#define MPT_FRAMEWORK_ASSERT_IS_DEFINED
#if defined(_DEBUG)
 #define MPT_FRAMEWORK_ASSERT_IS_ACTIVE 1
#else // !_DEBUG
 #define MPT_FRAMEWORK_ASSERT_IS_ACTIVE 0
#endif // _DEBUG
#endif // !defined(ASERRT)

// handle assert in our own way without relying on some platform-/framework-specific assert implementation
#define MPT_ASSERT_USE_FRAMEWORK 0

#endif // _MFC_VER


#if defined(MPT_FRAMEWORK_ASSERT_IS_DEFINED) && (MPT_ASSERT_USE_FRAMEWORK == 1)

#define MPT_ASSERT_NOTREACHED()          ASSERT(0)
#define MPT_ASSERT(expr)                 ASSERT((expr))
#define MPT_ASSERT_MSG(expr, msg)        ASSERT((expr) && (msg))
#if (MPT_FRAMEWORK_ASSERT_IS_ACTIVE == 1)
#define MPT_ASSERT_ALWAYS(expr)          ASSERT((expr))
#define MPT_ASSERT_ALWAYS_MSG(expr, msg) ASSERT((expr) && (msg))
#else
#define MPT_ASSERT_ALWAYS(expr)          MPT_DO { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr); } MPT_CHECKER_ASSUME(expr); } MPT_WHILE_0
#define MPT_ASSERT_ALWAYS_MSG(expr, msg) MPT_DO { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr, msg); } MPT_CHECKER_ASSUME(expr); } MPT_WHILE_0
#ifndef MPT_ASSERT_HANDLER_NEEDED
#define MPT_ASSERT_HANDLER_NEEDED
#endif
#endif

#elif defined(NO_ASSERTS)

#define MPT_ASSERT_NOTREACHED()          MPT_CHECKER_ASSUME(0)
#define MPT_ASSERT(expr)                 MPT_CHECKER_ASSUME(expr)
#define MPT_ASSERT_MSG(expr, msg)        MPT_CHECKER_ASSUME(expr)
#define MPT_ASSERT_ALWAYS(expr)          MPT_DO { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr); } MPT_CHECKER_ASSUME(expr); } MPT_WHILE_0
#define MPT_ASSERT_ALWAYS_MSG(expr, msg) MPT_DO { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr, msg); } MPT_CHECKER_ASSUME(expr); } MPT_WHILE_0
#ifndef MPT_ASSERT_HANDLER_NEEDED
#define MPT_ASSERT_HANDLER_NEEDED
#endif

#else // !NO_ASSERTS

#define MPT_ASSERT_NOTREACHED()          MPT_DO { MPT_CONSTANT_IF(!(0)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, "0"); } MPT_CHECKER_ASSUME(0); } MPT_WHILE_0
#define MPT_ASSERT(expr)                 MPT_DO { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr); } MPT_CHECKER_ASSUME(expr); } MPT_WHILE_0
#define MPT_ASSERT_MSG(expr, msg)        MPT_DO { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr, msg); } MPT_CHECKER_ASSUME(expr); } MPT_WHILE_0
#define MPT_ASSERT_ALWAYS(expr)          MPT_DO { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr); } MPT_CHECKER_ASSUME(expr); } MPT_WHILE_0
#define MPT_ASSERT_ALWAYS_MSG(expr, msg) MPT_DO { if(!(expr)) { AssertHandler(__FILE__, __LINE__, __FUNCTION__, #expr, msg); } MPT_CHECKER_ASSUME(expr); } MPT_WHILE_0
#ifndef MPT_ASSERT_HANDLER_NEEDED
#define MPT_ASSERT_HANDLER_NEEDED
#endif

#endif // NO_ASSERTS


#if defined(MPT_ASSERT_HANDLER_NEEDED)
// custom assert handler needed
MPT_NOINLINE void AssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg=nullptr);
#endif // MPT_ASSERT_HANDLER_NEEDED


// Compile time assert.
#if (MPT_COMPILER_GCC && MPT_GCC_BEFORE(4,3,0))
	#define MPT_SA_CONCAT(x, y) x ## y
	#define MPT_SA_HELPER(x) MPT_SA_CONCAT(OPENMPT_STATIC_ASSERT_, x)
	#define OPENMPT_STATIC_ASSERT MPT_SA_HELPER(__LINE__)
	#define static_assert(expr, msg) typedef char OPENMPT_STATIC_ASSERT[(expr)?1:-1]
#elif (MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0))
	#define static_assert(expr, msg) typedef char OPENMPT_STATIC_ASSERT[(expr)?1:-1]
#endif
#define STATIC_ASSERT(expr) static_assert((expr), "compile time assertion failed: " #expr)


// Macro for marking intentional fall-throughs in switch statements - can be used for static analysis if supported.
#if MPT_COMPILER_MSVC
#define MPT_FALLTHROUGH __fallthrough
#elif MPT_COMPILER_CLANG
#define MPT_FALLTHROUGH [[clang::fallthrough]]
#else
#define MPT_FALLTHROUGH MPT_DO { } MPT_WHILE_0
#endif



#if (MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)) || (MPT_COMPILER_GCC && MPT_GCC_BEFORE(4,3,0))

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

OPENMPT_NAMESPACE_END
#include <stdint.h>
OPENMPT_NAMESPACE_BEGIN

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

typedef double float64;
STATIC_ASSERT(sizeof(float64) == 8);



#if MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_PRINTF_FUNC(formatstringindex,varargsindex) __attribute__((format(printf, formatstringindex, varargsindex)))
#else
#define MPT_PRINTF_FUNC(formatstringindex,varargsindex)
#endif



#if MPT_COMPILER_MSVC && defined(UNREFERENCED_PARAMETER)
#define MPT_UNREFERENCED_PARAMETER(x) UNREFERENCED_PARAMETER(x)
#else
#define MPT_UNREFERENCED_PARAMETER(x) (void)(x)
#endif

#define MPT_UNUSED_VARIABLE(x) MPT_UNREFERENCED_PARAMETER(x)



#if MPT_COMPILER_MSVC
// warning LNK4221: no public symbols found; archive member will be inaccessible
// There is no way to selectively disable linker warnings.
// #pragma warning does not apply and a command line option does not exist.
// Some options:
//  1. Macro which generates a variable with a unique name for each file (which means we have to pass the filename to the macro)
//  2. unnamed namespace containing any symbol (does not work for c++11 compilers because they actually have internal linkage now)
//  3. An unused trivial inline function.
// Option 3 does not actually solve the problem though, which leaves us with option 1.
// In any case, for optimized builds, the linker will just remove the useless symbol.
#define MPT_MSVC_WORKAROUND_LNK4221_CONCAT_DETAIL(x,y) x##y
#define MPT_MSVC_WORKAROUND_LNK4221_CONCAT(x,y) MPT_MSVC_WORKAROUND_LNK4221_CONCAT_DETAIL(x,y)
#define MPT_MSVC_WORKAROUND_LNK4221(x) int MPT_MSVC_WORKAROUND_LNK4221_CONCAT(mpt_msvc_workaround_lnk4221_,x) = 0;
#endif

#ifndef MPT_MSVC_WORKAROUND_LNK4221
#define MPT_MSVC_WORKAROUND_LNK4221(x)
#endif



OPENMPT_NAMESPACE_END
