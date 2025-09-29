/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_MACROS_HPP
#define MPT_BASE_MACROS_HPP



#include "mpt/base/detect.hpp"

#include <type_traits>

#if MPT_COMPILER_MSVC && MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_COMPILER_MSVC && MPT_OS_WINDOWS



#if MPT_COMPILER_MSVC
#if MPT_MSVC_AT_LEAST(2022, 0)
#define MPT_ATTR_FLATTEN [[msvc::flatten]]
#else
#define MPT_ATTR_FLATTEN
#endif
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_ATTR_FLATTEN [[gnu::flatten]]
#else
#define MPT_ATTR_FLATTEN
#endif



#if MPT_COMPILER_MSVC
#if MPT_MSVC_AT_LEAST(2019, 1) && MPT_CXX_AT_LEAST(20)
#define MPT_ATTR_NOINLINE [[msvc::noinline]]
#else
#define MPT_ATTR_NOINLINE
#endif
#elif MPT_COMPILER_GCC
// https://godbolt.org/z/1P8758q89
#if MPT_GCC_AT_LEAST(8, 1, 0)
#define MPT_ATTR_NOINLINE [[gnu::noinline]] [[gnu::noclone]] [[gnu::no_icf]] [[gnu::noipa]]
#else
#define MPT_ATTR_NOINLINE [[gnu::noinline]] [[gnu::noclone]] [[gnu::no_icf]]
#endif
#elif MPT_COMPILER_CLANG
#if MPT_CLANG_AT_LEAST(15, 0, 0)
#define MPT_ATTR_NOINLINE [[clang::noinline]]
#else
#define MPT_ATTR_NOINLINE [[gnu::noinline]]
#endif
#else
#define MPT_ATTR_NOINLINE
#endif



#if MPT_COMPILER_MSVC
#if MPT_MSVC_AT_LEAST(2019, 1) && MPT_CXX_AT_LEAST(20)
#define MPT_ATTR_ALWAYSINLINE [[msvc::forceinline]]
#else
#define MPT_ATTR_ALWAYSINLINE
#endif
#elif MPT_COMPILER_GCC
#define MPT_ATTR_ALWAYSINLINE [[gnu::always_inline]]
#elif MPT_COMPILER_CLANG
#if MPT_CLANG_AT_LEAST(15, 0, 0)
#define MPT_ATTR_ALWAYSINLINE [[clang::always_inline]]
#else
#define MPT_ATTR_ALWAYSINLINE [[gnu::always_inline]]
#endif
#else
#define MPT_ATTR_ALWAYSINLINE
#endif



// for compilers that do not support C++ attribute syntax for noinline
#if MPT_COMPILER_MSVC && (MPT_MSVC_BEFORE(2019, 1) || MPT_CXX_BEFORE(20))
#define MPT_DECL_NOINLINE __declspec(noinline)
#else
#define MPT_DECL_NOINLINE
#endif



// for compilers that do not support C++ attribute syntax for forced inline
#if MPT_COMPILER_MSVC && (MPT_MSVC_BEFORE(2019, 1) || MPT_CXX_BEFORE(20))
#define MPT_INLINE_FORCE __forceinline
#else
#define MPT_INLINE_FORCE inline
#endif



#if MPT_CXX_AT_LEAST(23)
#define MPT_CONSTEXPR20_FUN MPT_INLINE_FORCE constexpr
#define MPT_CONSTEXPR20_VAR constexpr
#define MPT_CONSTEXPR23_FUN MPT_INLINE_FORCE constexpr
#define MPT_CONSTEXPR23_VAR constexpr
#elif MPT_CXX_AT_LEAST(20)
#define MPT_CONSTEXPR20_FUN MPT_INLINE_FORCE constexpr
#define MPT_CONSTEXPR20_VAR constexpr
#define MPT_CONSTEXPR23_FUN MPT_INLINE_FORCE
#define MPT_CONSTEXPR23_VAR const
#else // C++
#define MPT_CONSTEXPR20_FUN MPT_INLINE_FORCE
#define MPT_CONSTEXPR20_VAR const
#define MPT_CONSTEXPR23_FUN MPT_INLINE_FORCE
#define MPT_CONSTEXPR23_VAR const
#endif // C++

#if !defined(MPT_LIBCXX_QUIRK_NO_CXX20_CONSTEXPR_ALGORITHM)
#define MPT_CONSTEXPR20_ALGORITHM_FUN MPT_CONSTEXPR20_FUN
#define MPT_CONSTEXPR20_ALGORITHM_VAR MPT_CONSTEXPR20_VAR
#else
#define MPT_CONSTEXPR20_ALGORITHM_FUN MPT_CONSTEXPR23_FUN
#define MPT_CONSTEXPR20_ALGORITHM_VAR MPT_CONSTEXPR23_VAR
#endif

#if !defined(MPT_LIBCXX_QUIRK_NO_CXX20_CONSTEXPR_CONTAINER)
#define MPT_CONSTEXPR20_CONTAINER_FUN MPT_CONSTEXPR20_FUN
#define MPT_CONSTEXPR20_CONTAINER_VAR MPT_CONSTEXPR20_VAR
#else
#define MPT_CONSTEXPR20_CONTAINER_FUN MPT_CONSTEXPR23_FUN
#define MPT_CONSTEXPR20_CONTAINER_VAR MPT_CONSTEXPR23_VAR
#endif



#if MPT_CXX_AT_LEAST(20)
#define MPT_CONSTEVAL consteval
#else // !C++20
// fallback to constexpr
#define MPT_CONSTEVAL MPT_INLINE_FORCE constexpr
#endif // C++20



#if MPT_CXX_AT_LEAST(20)
#define MPT_CONSTEVAL_NOEXCEPT noexcept
#else // !C++20
#define MPT_CONSTEVAL_NOEXCEPT
#endif // C++20



#define MPT_FORCE_CONSTEXPR_EXPRESSION(expr) [&]() { \
	constexpr auto x = (expr); \
	return x; \
}()
#define MPT_FORCE_CONSTEXPR_VALUE(val) []() { \
	constexpr auto x = (val); \
	return x; \
}()



#if MPT_CXX_AT_LEAST(20)
// this assumes that for C++20, a consteval function will be used
#define MPT_FORCE_CONSTEVAL_EXPRESSION(expr) (expr)
#define MPT_FORCE_CONSTEVAL_VALUE(val)       (val)
#else // !C++20
#define MPT_FORCE_CONSTEVAL_EXPRESSION(expr) [&]() { \
	constexpr auto x = (expr); \
	return x; \
}()
#define MPT_FORCE_CONSTEVAL_VALUE(val) []() { \
	constexpr auto x = (val); \
	return x; \
}()
#endif // C++20



#if MPT_CXX_AT_LEAST(20)
#define MPT_CONSTINIT constinit
#else // !C++20
// fallback to nothing
#define MPT_CONSTINIT
#endif // C++20



#if MPT_CXX_AT_LEAST(20)
#define MPT_IS_CONSTANT_EVALUATED20() std::is_constant_evaluated()
#define MPT_IS_CONSTANT_EVALUATED()   std::is_constant_evaluated()
#else // !C++20
#define MPT_IS_CONSTANT_EVALUATED20() false
// this pessimizes the case for C++17 by always assuming constexpr context, which implies always running constexpr-friendly code
#define MPT_IS_CONSTANT_EVALUATED()   true
#endif // C++20



#if MPT_COMPILER_MSVC
#define MPT_MAYBE_CONSTANT_IF(x) \
	__pragma(warning(push)) \
	__pragma(warning(disable : 4127)) \
	if (x) \
		__pragma(warning(pop)) \
/**/
#endif

#if MPT_COMPILER_GCC
#define MPT_MAYBE_CONSTANT_IF(x) \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wtype-limits\"") \
	if (x) \
		_Pragma("GCC diagnostic pop") \
/**/
#endif

#if MPT_COMPILER_CLANG
#define MPT_MAYBE_CONSTANT_IF(x) \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wunknown-pragmas\"") \
	_Pragma("clang diagnostic ignored \"-Wtype-limits\"") \
	_Pragma("clang diagnostic ignored \"-Wtautological-constant-out-of-range-compare\"") \
	if (x) \
		_Pragma("clang diagnostic pop") \
/**/
#endif

#if !defined(MPT_MAYBE_CONSTANT_IF)
// MPT_MAYBE_CONSTANT_IF disables compiler warnings for conditions that may in some case be either always false or always true (this may turn out to be useful in ASSERTions in some cases).
#define MPT_MAYBE_CONSTANT_IF(x) if (x)
#endif



#if MPT_COMPILER_MSVC && MPT_OS_WINDOWS
#define MPT_UNUSED(x) UNREFERENCED_PARAMETER(x)
#else
#define MPT_UNUSED(x) static_cast<void>(x)
#endif



#define MPT_DISCARD(expr) static_cast<void>(expr)



// Use MPT_RESTRICT to indicate that a pointer is guaranteed to not be aliased.
#if MPT_COMPILER_MSVC || MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_RESTRICT __restrict
#else
#define MPT_RESTRICT
#endif



#if MPT_CXX_AT_LEAST(23) && !MPT_GCC_BEFORE(13, 0, 0) && !MPT_CLANG_BEFORE(19, 0, 0) && !MPT_COMPILER_MSVC
#define MPT_ASSUME(expr) [[assume(expr)]]
#else // !C++23
#if MPT_COMPILER_CLANG
#define MPT_ASSUME(expr) __builtin_assume(expr)
#endif
#if MPT_COMPILER_MSVC
#define MPT_ASSUME(expr) __assume(expr)
#endif
#if MPT_COMPILER_GCC
#define MPT_ASSUME(expr) \
	do { \
		if (!expr) { \
			__builtin_unreachable(); \
		} \
	} while (0)
#endif
#if !defined(MPT_ASSUME)
#define MPT_ASSUME(expr) MPT_DISCARD(expr)
#endif
#endif // C++23



#endif // MPT_BASE_MACROS_HPP
