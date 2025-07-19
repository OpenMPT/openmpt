/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_STDIO_HPP
#define MPT_FILEMODE_STDIO_HPP



#include "mpt/base/detect.hpp"
#if MPT_GCC_AT_LEAST(14, 0, 0) && MPT_GCC_BEFORE(15, 1, 0)
#include "mpt/base/macros.hpp"
#endif
#include "mpt/base/namespace.hpp"
#include "mpt/filemode/fd.hpp"
#include "mpt/filemode/filemode.hpp"
#include "mpt/filemode/iostream.hpp"
#include "mpt/filemode/stdfile.hpp"

#include <variant>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace filemode {



enum class api {
	none,
	fd,
	file,
	iostream,
};

template <mpt::filemode::stdio which>
class stdio_guard {
private:
	std::variant<std::monostate, mpt::filemode::iostream_guard<which>, mpt::filemode::FILE_guard<which>, mpt::filemode::fd_guard<which>> guard;
public:
#if MPT_GCC_AT_LEAST(14, 0, 0) && MPT_GCC_BEFORE(15, 1, 0)
	// work-around bogus -Wmaybe-uninitialized
	// See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121173>.
	// clang-format off
	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE
#endif
	explicit stdio_guard(mpt::filemode::api api, mpt::filemode::mode mode) {
		// clang-format on
		switch (api) {
			case mpt::filemode::api::iostream:
				guard.template emplace<mpt::filemode::iostream_guard<which>>(mode);
				break;
			case mpt::filemode::api::file:
				guard.template emplace<mpt::filemode::FILE_guard<which>>(mode);
				break;
			case mpt::filemode::api::fd:
				guard.template emplace<mpt::filemode::fd_guard<which>>(mode);
				break;
			case mpt::filemode::api::none:
				// nothing;
				break;
		}
	}
	stdio_guard(const stdio_guard &) = delete;
	stdio_guard(stdio_guard &&) = delete;
	stdio_guard & operator=(const stdio_guard &) = delete;
	stdio_guard & operator=(stdio_guard &&) = delete;
#if MPT_GCC_AT_LEAST(14, 0, 0) && MPT_GCC_BEFORE(15, 1, 0)
	// work-around bogus -Wmaybe-uninitialized
	// See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121173>.
	// clang-format off
	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE
#endif
	~stdio_guard() {
		// clang-format on
		guard.template emplace<std::monostate>();
	}
};



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_STDIO_HPP
