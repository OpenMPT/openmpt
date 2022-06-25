/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_PATH_OS_PATH_LONG_HPP
#define MPT_PATH_OS_PATH_LONG_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/path/path.hpp"
#include "mpt/path/os_path.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"

#if MPT_OS_WINDOWS
#include <vector>
#endif // MPT_OS_WINDOWS

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS



namespace mpt {
inline namespace MPT_INLINE_NS {



inline mpt::os_path support_long_path(const mpt::os_path & path) {
#if MPT_OS_WINDOWS
	if (path.length() < MAX_PATH) {
		// path is short enough
		return path;
	}
	if (path.substr(0, 4) == MPT_OS_PATH_LITERAL("\\\\?\\")) {
		// path is already in prefixed form
		return path;
	}
	mpt::os_path absolute_path = path;
	DWORD size = GetFullPathName(path.c_str(), 0, nullptr, nullptr);
	if (size != 0) {
		std::vector<TCHAR> fullPathName(size, TEXT('\0'));
		if (GetFullPathName(path.c_str(), size, fullPathName.data(), nullptr) != 0) {
			absolute_path = fullPathName.data();
		}
	}
	if (absolute_path.substr(0, 2) == MPT_OS_PATH_LITERAL("\\\\")) {
		// Path is a network share: \\server\foo.bar -> \\?\UNC\server\foo.bar
		return MPT_OS_PATH_LITERAL("\\\\?\\UNC") + absolute_path.substr(1);
	} else {
		// Regular file: C:\foo.bar -> \\?\C:\foo.bar
		return MPT_OS_PATH_LITERAL("\\\\?\\") + absolute_path;
	}
#else
	return path;
#endif
}



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_PATH_OS_PATH_LONG_HPP
