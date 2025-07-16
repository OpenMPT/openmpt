/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_STDIO_HPP
#define MPT_FILEMODE_STDIO_HPP



#include "mpt/base/detect.hpp"
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
	explicit stdio_guard(mpt::filemode::api api, mpt::filemode::mode mode) {
		switch (api) {
			case mpt::filemode::api::iostream:
				guard = mpt::filemode::iostream_guard<which>(mode);
				break;
			case mpt::filemode::api::file:
				guard = mpt::filemode::FILE_guard<which>(mode);
				break;
			case mpt::filemode::api::fd:
				guard = mpt::filemode::fd_guard<which>(mode);
				break;
			case mpt::filemode::api::none:
				// nothing;
				break;
		}
	}
	stdio_guard(const stdio_guard &) = delete;
	stdio_guard(stdio_guard &&) = default;
	stdio_guard & operator=(const stdio_guard &) = delete;
	stdio_guard & operator=(stdio_guard &&) = default;
	~stdio_guard() {
		guard = std::monostate{};
	}
};



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_STDIO_HPP
