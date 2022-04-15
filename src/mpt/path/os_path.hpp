/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_PATH_OS_PATH_HPP
#define MPT_PATH_OS_PATH_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/path/path.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"

#if !defined(MPT_COMPILER_QUIRK_NO_FILESYSTEM)
#include <filesystem>
#endif // !MPT_COMPILER_QUIRK_NO_FILESYSTEM
#if MPT_OS_WINDOWS && defined(MPT_COMPILER_QUIRK_NO_FILESYSTEM)
#include <vector>
#endif // MPT_OS_WINDOWS && MPT_COMPILER_QUIRK_NO_FILESYSTEM

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS



namespace mpt {
inline namespace MPT_INLINE_NS {



// mpt::os_path is an alias to a string type that represents native operating system path encoding.
// Note that this differs from std::filesystem::path::string_type on both Windows and Posix.
// On Windows, we actually honor UNICODE and thus allow os_path.c_str() to be usable with WinAPI functions.
// On Posix, we use a type-safe string type in locale encoding, in contrast to the encoding-confused supposedly UTF8 std::string in std::filesystem::path.

#if MPT_OS_WINDOWS
using os_path = mpt::winstring;
#else  // !MPT_OS_WINDOWS
using os_path = mpt::lstring;
#endif // MPT_OS_WINDOWS



// mpt::os_path literals that do not involve runtime conversion.

#if MPT_OS_WINDOWS
#define MPT_OSPATH_CHAR(x)    TEXT(x)
#define MPT_OSPATH_LITERAL(x) TEXT(x)
#define MPT_OSPATH(x) \
	mpt::winstring { \
		TEXT(x) \
	}
#else // !MPT_OS_WINDOWS
#define MPT_OSPATH_CHAR(x)    x
#define MPT_OSPATH_LITERAL(x) x
#define MPT_OSPATH(x) \
	mpt::lstring { \
		x \
	}
#endif // MPT_OS_WINDOWS



inline mpt::os_path support_long_path(const mpt::os_path & path) {
#if MPT_OS_WINDOWS
	if (path.length() < MAX_PATH) {
		// path is short enough
		return path;
	}
	if (path.substr(0, 4) == MPT_OSPATH_LITERAL("\\\\?\\")) {
		// path is already in prefixed form
		return path;
	}
#if !defined(MPT_COMPILER_QUIRK_NO_FILESYSTEM)
	const mpt::os_path absolute_path = mpt::transcode<mpt::os_path>(std::filesystem::absolute(mpt::transcode<std::filesystem::path>(path)));
#else  // !MPT_COMPILER_QUIRK_NO_FILESYSTEM
	mpt::os_path absolute_path = path;
	DWORD size = GetFullPathName(path.c_str(), 0, nullptr, nullptr);
	if (size != 0) {
		std::vector<TCHAR> fullPathName(size, TEXT('\0'));
		if (GetFullPathName(path.c_str(), size, fullPathName.data(), nullptr) != 0) {
			absolute_path = fullPathName.data();
		}
	}
#endif // !MPT_COMPILER_QUIRK_NO_FILESYSTEM
	if (absolute_path.substr(0, 2) == MPT_OSPATH_LITERAL("\\\\")) {
		// Path is a network share: \\server\foo.bar -> \\?\UNC\server\foo.bar
		return MPT_OSPATH_LITERAL("\\\\?\\UNC") + absolute_path.substr(1);
	} else {
		// Regular file: C:\foo.bar -> \\?\C:\foo.bar
		return MPT_OSPATH_LITERAL("\\\\?\\") + absolute_path;
	}
#else
	return path;
#endif
}



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_PATH_OS_PATH_HPP
