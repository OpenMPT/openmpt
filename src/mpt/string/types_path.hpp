/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_STRING_TYPES_PATH_HPP
#define MPT_STRING_TYPES_PATH_HPP



#include "mpt/base/namespace.hpp"
#include "mpt/string/types.hpp"

#include <filesystem>
#include <type_traits>



namespace mpt {
inline namespace MPT_INLINE_NS {



template <>
struct make_string_type<std::filesystem::path> {
	using type = std::filesystem::path;
};


template <>
struct is_string_type<std::filesystem::path> : public std::true_type { };



#if MPT_OS_WINDOWS && !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
#define MPT_PATH_CHAR(x)    L##x
#define MPT_PATH_LITERAL(x) L##x
#define MPT_PATH(x) \
	std::filesystem::path { \
		L##x \
	}
#elif MPT_OS_WINDOWS
#define MPT_PATH_CHAR(x)    x
#define MPT_PATH_LITERAL(x) x
#define MPT_PATH(x) \
	std::filesystem::path { \
		x \
	}
#elif MPT_CXX_AT_LEAST(20)
#define MPT_PATH_CHAR(x)    u8##x
#define MPT_PATH_LITERAL(x) u8##x
#define MPT_PATH(x) \
	std::filesystem::path { \
		u8##x \
	}
#else
#define MPT_PATH_CHAR(x)    U##x
#define MPT_PATH_LITERAL(x) U##x
#define MPT_PATH(x) \
	std::filesystem::path { \
		U##x \
	}
#endif



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_STRING_TYPES_PATH_HPP
