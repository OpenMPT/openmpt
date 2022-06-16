/*
 * mptPathString.h
 * ---------------
 * Purpose: Wrapper class around the platform-native representation of path names. Should be the only type that is used to store path names.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/path/basic_path.hpp"
#include "mpt/path/os_path.hpp"
#include "mpt/string/types.hpp"

#include "mptString.h"



OPENMPT_NAMESPACE_BEGIN



namespace mpt
{



#if defined(MPT_ENABLE_CHARSET_LOCALE)
using RawPathString = mpt::os_path;
#define MPT_PATHSTRING_LITERAL(x) MPT_OSPATH_LITERAL( x )
#define MPT_PATHSTRING(x) mpt::PathString::FromNative(MPT_OSPATH_LITERAL( x ))
#else // !MPT_ENABLE_CHARSET_LOCALE
using RawPathString = mpt::utf8string;
#define MPT_PATHSTRING_LITERAL(x) ( x )
#define MPT_PATHSTRING(x) mpt::PathString::FromNative( x )
#endif // MPT_ENABLE_CHARSET_LOCALE

#define PC_(x) MPT_PATHSTRING_LITERAL(x)
#define PL_(x) MPT_PATHSTRING_LITERAL(x)
#define P_(x) MPT_PATHSTRING(x)



struct NativePathStyleTag
{
#if MPT_OS_WINDOWS
#if defined(NTDDI_VERSION) || defined(_WIN32_WINNT)
	static inline constexpr mpt::PathStyle path_style = mpt::PathStyle::WindowsNT;
#else
	static inline constexpr mpt::PathStyle path_style = mpt::PathStyle::Windows9x;
#endif
#elif MPT_OS_DJGPP
	static inline constexpr mpt::PathStyle path_style = mpt::PathStyle::DOS_DJGPP;
#else
	static inline constexpr mpt::PathStyle path_style = mpt::PathStyle::Posix;
#endif
};

struct NativePathTraits
	: public PathTraits<RawPathString, NativePathStyleTag>
{
};

#if defined(MPT_ENABLE_CHARSET_LOCALE)
using PathString = mpt::BasicPathString<NativePathTraits>;
#else
using PathString = mpt::BasicPathString<NativePathTraits, false>;
#endif



#if defined(MPT_ENABLE_CHARSET_LOCALE)
inline std::string ToAString(const mpt::PathString &x)
{
	return x.ToLocale();
}
#else
inline std::string ToAString(const mpt::PathString &x)
{
	return x.ToUTF8();
}
#endif
inline mpt::ustring ToUString(const mpt::PathString &x)
{
	return x.ToUnicode();
}
#if MPT_WSTRING_FORMAT
inline std::wstring ToWString(const mpt::PathString &x)
{
	return x.ToWide();
}
#endif



// Return native string, with possible \\?\ prefix if it exceeds MAX_PATH characters.
mpt::RawPathString SupportLongPath(const mpt::RawPathString &path);

#if MPT_OS_WINDOWS
#if !(MPT_OS_WINDOWS_WINRT && (_WIN32_WINNT < 0x0a00))
// Returns the absolute path for a potentially relative path and removes ".." or "." components. (same as GetFullPathNameW)
mpt::PathString GetAbsolutePath(const mpt::PathString &path);
#endif
#endif // MPT_OS_WINDOWS

#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS

// Relative / absolute paths conversion

mpt::PathString AbsolutePathToRelative(const mpt::PathString &p, const mpt::PathString &relativeTo); // similar to std::fs::path::lexically_approximate
	
mpt::PathString RelativePathToAbsolute(const mpt::PathString &p, const mpt::PathString &relativeTo);

#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS



#if MPT_OS_WINDOWS
#if !MPT_OS_WINDOWS_WINRT
int PathCompareNoCase(const PathString &a, const PathString &b);
#endif // !MPT_OS_WINDOWS_WINRT
#endif



} // namespace mpt



#if defined(MODPLUG_TRACKER)

// Sanitize a filename (remove special chars)

mpt::PathString SanitizePathComponent(const mpt::PathString &filename);

std::string SanitizePathComponent(std::string str);
std::wstring SanitizePathComponent(std::wstring str);
#if MPT_USTRING_MODE_UTF8
mpt::u8string SanitizePathComponent(mpt::u8string str);
#endif // MPT_USTRING_MODE_UTF8
#if defined(MPT_WITH_MFC)
CString SanitizePathComponent(CString str);
#endif // MPT_WITH_MFC

#endif // MODPLUG_TRACKER



OPENMPT_NAMESPACE_END
