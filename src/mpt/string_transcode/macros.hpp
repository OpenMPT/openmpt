/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_STRING_TRANSCODE_MACROS_HPP
#define MPT_STRING_TRANSCODE_MACROS_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"

#include <string_view>
#include <type_traits>

#include <cstddef>



namespace mpt {



template <typename Tchar, std::size_t N>
inline mpt::ustring transcode_utf8_helper(const Tchar (&literal)[N]) {
#if MPT_CXX_AT_LEAST(20)
	if constexpr (std::is_same<Tchar, char8_t>::value) {
		return mpt::transcode<mpt::ustring>(std::u8string_view{literal});
	} else {
		return mpt::transcode<mpt::ustring>(mpt::common_encoding::utf8, std::string_view{literal});
	}
#else // !C++20
	return mpt::transcode<mpt::ustring>(mpt::common_encoding::utf8, std::string_view{literal});
#endif
}

// The MPT_UTF8_STRING allows specifying UTF8 char arrays.
// The resulting type is mpt::ustring and the construction might require runtime translation,
// i.e. it is NOT generally available at compile time.
// Use explicit UTF8 encoding,
// i.e. U+00FC (LATIN SMALL LETTER U WITH DIAERESIS) would be written as "\xC3\xBC".
#define MPT_USTRING_UTF8(x) mpt::transcode_utf8_helper(x)



} // namespace mpt



#endif // MPT_STRING_TRANSCODE_MACROS_HPP
