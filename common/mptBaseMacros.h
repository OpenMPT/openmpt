/*
 * mptBaseMacros.h
 * ---------------
 * Purpose: Basic assorted compiler-related helpers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"



#if MPT_CXX_AT_LEAST(20)
#include <version>
#else // !C++20
#include <array>
#endif // C++20

#include <array>
#include <iterator>
#include <type_traits>

#include <cstddef>
#include <cstdint>

#include <stddef.h>
#include <stdint.h>



OPENMPT_NAMESPACE_BEGIN



#define MPT_PP_DEFER(m, ...) m(__VA_ARGS__)

#define MPT_PP_STRINGIFY(x) #x

#define MPT_PP_JOIN_HELPER(a, b) a ## b
#define MPT_PP_JOIN(a, b) MPT_PP_JOIN_HELPER(a, b)

#define MPT_PP_UNIQUE_IDENTIFIER(prefix) MPT_PP_JOIN(prefix , __LINE__)



#if MPT_COMPILER_MSVC

#define MPT_WARNING(text)           __pragma(message(__FILE__ "(" MPT_PP_DEFER(MPT_PP_STRINGIFY, __LINE__) "): Warning: " text))
#define MPT_WARNING_STATEMENT(text) __pragma(message(__FILE__ "(" MPT_PP_DEFER(MPT_PP_STRINGIFY, __LINE__) "): Warning: " text))

#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG

#define MPT_WARNING(text)           _Pragma(MPT_PP_STRINGIFY(GCC warning text))
#define MPT_WARNING_STATEMENT(text) _Pragma(MPT_PP_STRINGIFY(GCC warning text))

#else

// portable #pragma message or #warning replacement
#define MPT_WARNING(text) \
	static inline int MPT_PP_UNIQUE_IDENTIFIER(MPT_WARNING_NAME) () noexcept { \
		int warning [[deprecated("Warning: " text)]] = 0; \
		return warning; \
	} \
/**/
#define MPT_WARNING_STATEMENT(text) \
	int MPT_PP_UNIQUE_IDENTIFIER(MPT_WARNING_NAME) = [](){ \
		int warning [[deprecated("Warning: " text)]] = 0; \
		return warning; \
	}() \
/**/

#endif



// Advanced inline attributes
#if MPT_COMPILER_MSVC
#define MPT_FORCEINLINE __forceinline
#define MPT_NOINLINE    __declspec(noinline)
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_FORCEINLINE __attribute__((always_inline)) inline
#define MPT_NOINLINE    __attribute__((noinline))
#else
#define MPT_FORCEINLINE inline
#define MPT_NOINLINE
#endif



// constexpr
#define MPT_CONSTEXPRINLINE constexpr MPT_FORCEINLINE
#if MPT_CXX_AT_LEAST(20)
#define MPT_CONSTEXPR20_FUN constexpr MPT_FORCEINLINE
#define MPT_CONSTEXPR20_VAR constexpr
#else // !C++20
#define MPT_CONSTEXPR20_FUN MPT_FORCEINLINE
#define MPT_CONSTEXPR20_VAR const
#endif // C++20



#define MPT_FORCE_CONSTEXPR(expr) [&]() { \
  constexpr auto x = (expr); \
  return x; \
}()



#if MPT_CXX_AT_LEAST(20)
#define MPT_IS_CONSTANT_EVALUATED20() std::is_constant_evaluated()
#define MPT_IS_CONSTANT_EVALUATED() std::is_constant_evaluated()
#else // !C++20
#define MPT_IS_CONSTANT_EVALUATED20() false
#define MPT_IS_CONSTANT_EVALUATED() true // this pessimizes the case for C++17 by always assuming constexpr context, which implies always running constexpr-friendly code
#endif // C++20



// Use MPT_RESTRICT to indicate that a pointer is guaranteed to not be aliased.
#if MPT_COMPILER_MSVC || MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_RESTRICT __restrict
#else
#define MPT_RESTRICT
#endif



#define MPT_DISCARD(expr) static_cast<void>(expr)



#if MPT_COMPILER_MSVC
#define MPT_MAYBE_CONSTANT_IF(x) \
  __pragma(warning(push)) \
  __pragma(warning(disable:4127)) \
  if(x) \
  __pragma(warning(pop)) \
/**/
#endif

#if MPT_COMPILER_GCC
#define MPT_MAYBE_CONSTANT_IF(x) \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wtype-limits\"") \
  if(x) \
  _Pragma("GCC diagnostic pop") \
/**/
#endif

#if MPT_COMPILER_CLANG
#define MPT_MAYBE_CONSTANT_IF(x) \
  _Pragma("clang diagnostic push") \
  _Pragma("clang diagnostic ignored \"-Wunknown-pragmas\"") \
  _Pragma("clang diagnostic ignored \"-Wtype-limits\"") \
  _Pragma("clang diagnostic ignored \"-Wtautological-constant-out-of-range-compare\"") \
  if(x) \
  _Pragma("clang diagnostic pop") \
/**/
#endif

#if !defined(MPT_MAYBE_CONSTANT_IF)
// MPT_MAYBE_CONSTANT_IF disables compiler warnings for conditions that may in some case be either always false or always true (this may turn out to be useful in ASSERTions in some cases).
#define MPT_MAYBE_CONSTANT_IF(x) if(x)
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
