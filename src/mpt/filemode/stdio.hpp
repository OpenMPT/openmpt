/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_STDIO_HPP
#define MPT_FILEMODE_STDIO_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/filemode/filemode.hpp"
#include "mpt/filemode/iostream.hpp"

#include <optional>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace filemode {



template <mpt::filemode::stdio which>
class stdio_guard {
private:
	std::optional<mpt::filemode::iostream_guard<which>> guard;
public:
	stdio_guard(mpt::filemode::mode mode) {
		guard = std::make_optional<mpt::filemode::iostream_guard<which>>(mode);
	}
	stdio_guard(const stdio_guard &) = delete;
	stdio_guard(stdio_guard &&) = default;
	stdio_guard & operator=(const stdio_guard &) = delete;
	stdio_guard & operator=(stdio_guard &&) = default;
	~stdio_guard() {
		guard = std::nullopt;
	}
};



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_STDIO_HPP
