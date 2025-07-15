/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_STDFILE_HPP
#define MPT_FILEMODE_STDFILE_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/filemode/fd.hpp"
#include "mpt/filemode/filemode.hpp"

#include <optional>

#include <cstddef>
#include <cstdio>

#include <stdio.h>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace filemode {



template <mpt::filemode::stdio which>
class FILE_guard {
private:
	std::optional<mpt::filemode::fd_guard<which>> guard;
public:
	constexpr static std::FILE * get_FILE() {
		std::FILE * file = NULL;
		switch (which) {
			case mpt::filemode::stdio::input:
				file = stdin;
				break;
			case mpt::filemode::stdio::output:
				file = stdout;
				break;
			case mpt::filemode::stdio::error:
				file = stderr;
				break;
			case mpt::filemode::stdio::log:
				file = stderr;
				break;
		}
		return file;
	}
public:
	explicit FILE_guard(mpt::filemode::mode new_mode) {
		std::fflush(get_FILE());
		guard = std::make_optional<mpt::filemode::fd_guard<which>>(new_mode);
	}
	FILE_guard(const FILE_guard &) = delete;
	FILE_guard(FILE_guard &&) = default;
	FILE_guard & operator=(const FILE_guard &) = delete;
	FILE_guard & operator=(FILE_guard &&) = default;
	~FILE_guard() {
		std::fflush(get_FILE());
		guard = std::nullopt;
	}
};



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_STDFILE_HPP
