/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_IOSTREAM_HPP
#define MPT_FILEMODE_IOSTREAM_HPP



#include "mpt/base/detect.hpp"
#if MPT_GCC_AT_LEAST(14, 0, 0) && MPT_GCC_BEFORE(15, 1, 0)
#include "mpt/base/macros.hpp"
#endif
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
	static auto & get_stream() {
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
			// cppcheck false-positive
			// cppcheck-suppress missingReturn
		}
	}
public:
#if MPT_GCC_AT_LEAST(14, 0, 0) && MPT_GCC_BEFORE(15, 1, 0)
	// work-around bogus -Wmaybe-uninitialized
	// See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121173>.
	// clang-format off
	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE
#endif
	explicit iostream_guard(mpt::filemode::mode mode) {
		// clang-format on
		guard.emplace(mode);
	}
	iostream_guard(const iostream_guard &) = delete;
	iostream_guard(iostream_guard &&) = delete;
	iostream_guard & operator=(const iostream_guard &) = delete;
	iostream_guard & operator=(iostream_guard &&) = delete;
#if MPT_GCC_AT_LEAST(14, 0, 0) && MPT_GCC_BEFORE(15, 1, 0)
	// work-around bogus -Wmaybe-uninitialized
	// See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121173>.
	// clang-format off
	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE
#endif
	~iostream_guard() {
		// clang-format on
		if constexpr (which != mpt::filemode::stdio::input) {
			get_stream().flush();
		}
		guard.reset();
	}
};



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_IOSTREAM_HPP
