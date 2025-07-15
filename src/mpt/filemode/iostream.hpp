/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_IOSTREAM_HPP
#define MPT_FILEMODE_IOSTREAM_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/filemode/filemode.hpp"
#include "mpt/filemode/stdfile.hpp"

#include <iostream>
#include <optional>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace filemode {



template <mpt::filemode::stdio which>
class iostream_guard {
private:
	std::optional<mpt::filemode::FILE_guard<which>> guard;
private:
	MPT_CONSTEVAL static auto & get_stream() {
		if constexpr (which == mpt::filemode::stdio::input) {
#if MPT_OS_WINDOWS && defined(UNICODE)
			return std::wcin;
#else
			return std::cin;
#endif
		}
		if constexpr (which == mpt::filemode::stdio::output) {
#if MPT_OS_WINDOWS && defined(UNICODE)
			return std::wcout;
#else
			return std::cout;
#endif
		}
		if constexpr (which == mpt::filemode::stdio::error) {
#if MPT_OS_WINDOWS && defined(UNICODE)
			return std::wcerr;
#else
			return std::cerr;
#endif
		}
		if constexpr (which == mpt::filemode::stdio::log) {
#if MPT_OS_WINDOWS && defined(UNICODE)
			return std::wclog;
#else
			return std::clog;
#endif
		}
	}
public:
	iostream_guard(mpt::filemode::mode mode) {
		guard = std::make_optional<mpt::filemode::FILE_guard<which>>(mode);
	}
	iostream_guard(const iostream_guard &) = delete;
	iostream_guard(iostream_guard &&) = default;
	iostream_guard & operator=(const iostream_guard &) = delete;
	iostream_guard & operator=(iostream_guard &&) = default;
	~iostream_guard() {
		if constexpr (which != mpt::filemode::stdio::input) {
			get_stream().flush();
		}
		guard = std::nullopt;
	}
};



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_IOSTREAM_HPP
