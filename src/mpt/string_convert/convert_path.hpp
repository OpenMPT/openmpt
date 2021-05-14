/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_STRING_CONVERT_CONVERT_PATH_HPP
#define MPT_STRING_CONVERT_CONVERT_PATH_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string/types_path.hpp"
#include "mpt/string_convert/convert.hpp"

#include <filesystem>
#include <type_traits>



namespace mpt {
inline namespace MPT_INLINE_NS {



template <>
struct string_converter<std::filesystem::path> {
	using string_type = std::filesystem::path;
	static inline mpt::widestring decode(const string_type & src) {
		if constexpr (std::is_same<std::filesystem::path::value_type, char>::value) {
			// In contrast to standard recommendation and cppreference,
			// native encoding on unix-like systems with libstdc++ or libc++ is actually *NOT* UTF8,
			// but instead the conventional std::locale::locale("") encoding (which happens to be UTF8 on all modern systems, but this is not guaranteed).
			// Note: libstdc++ and libc++ however assume that their internal representation is UTF8,
			// which implies that wstring/u32string/u16string/u8string conversions are actually broken and MUST NOT be used, ever.
			return mpt::convert<mpt::widestring>(mpt::logical_encoding::locale, src.string());
#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
		} else if constexpr (std::is_same<std::filesystem::path::value_type, wchar_t>::value) {
			return mpt::convert<mpt::widestring>(src.wstring());
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR
		} else if constexpr (std::is_same<std::filesystem::path::value_type, char32_t>::value) {
			return mpt::convert<mpt::widestring>(src.u32string());
		} else if constexpr (std::is_same<std::filesystem::path::value_type, char16_t>::value) {
			return mpt::convert<mpt::widestring>(src.u16string());
#if MPT_CXX_AT_LEAST(20)
		} else if constexpr (std::is_same<std::filesystem::path::value_type, char8_t>::value) {
			return mpt::convert<mpt::widestring>(src.u8string());
#endif
		} else {
#if MPT_OS_WINDOWS && !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
			return mpt::convert<mpt::widestring>(src.wstring());
#elif MPT_OS_WINDOWS
			return mpt::convert<mpt::widestring>(mpt::logical_encoding::locale, src.string());
#else
			// completely unknown implementation, assume it can sanely convert to/from UTF16/UTF32
			if constexpr (sizeof(mpt::widechar) == sizeof(char32_t)) {
				return mpt::convert<mpt::widestring>(src.u32string());
			} else {
				return mpt::convert<mpt::widestring>(src.u16string());
			}
#endif
		}
	}
	static inline string_type encode(const mpt::widestring & src) {
		if constexpr (std::is_same<std::filesystem::path::value_type, char>::value) {
			// In contrast to standard recommendation and cppreference,
			// native encoding on unix-like systems with libstdc++ or libc++ is actually *NOT* UTF8,
			// but instead the conventional std::locale::locale("") encoding (which happens to be UTF8 on all modern systems, but this is not guaranteed).
			// Note: libstdc++ and libc++ however assume that their internal representation is UTF8,
			// which implies that wstring/u32string/u16string/u8string conversions are actually broken and MUST NOT be used, ever.
			return std::filesystem::path{mpt::convert<std::string>(mpt::logical_encoding::locale, src)};
#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
		} else if constexpr (std::is_same<std::filesystem::path::value_type, wchar_t>::value) {
			return std::filesystem::path{mpt::convert<std::wstring>(src)};
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR
		} else if constexpr (std::is_same<std::filesystem::path::value_type, char32_t>::value) {
			return std::filesystem::path{mpt::convert<std::u32string>(src)};
		} else if constexpr (std::is_same<std::filesystem::path::value_type, char16_t>::value) {
			return std::filesystem::path{mpt::convert<std::u16string>(src)};
#if MPT_CXX_AT_LEAST(20)
		} else if constexpr (std::is_same<std::filesystem::path::value_type, char8_t>::value) {
			return std::filesystem::path{mpt::convert<std::u8string>(src)};
#endif
		} else {
#if MPT_OS_WINDOWS && !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
			return std::filesystem::path{mpt::convert<std::wstring>(src)};
#elif MPT_OS_WINDOWS
			return std::filesystem::path{ pt::convert<std::string>(mpt::logical_encoding::locale, src)};
#else
			// completely unknown implementation, assume it can sanely convert to/from UTF16/UTF32
			if constexpr (sizeof(mpt::widechar) == sizeof(char32_t)) {
				return std::filesystem::path{mpt::convert<std::u32string>(src)};
			} else {
				return std::filesystem::path{mpt::convert<std::u16string>(src)};
			}
#endif
		}
	}
};



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_STRING_CONVERT_CONVERT_PATH_HPP
