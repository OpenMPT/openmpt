/*
 * mptPathString.h
 * ---------------
 * Purpose: Wrapper class around the platform-native representation of path names. Should be the only type that is used to store path names.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <cstdio>
#include <stdio.h>

//#define MPT_DEPRECATED_PATH
#define MPT_DEPRECATED_PATH MPT_DEPRECATED

namespace mpt
{

#if defined(WIN32)
typedef std::wstring RawPathString;
#else // !WIN32
typedef std::string RawPathString;
#endif // WIN32

class PathString
{
private:
	RawPathString path;
private:
	PathString(const RawPathString & path)
		: path(path)
	{
		return;
	}
public:
	PathString()
	{
		return;
	}
	PathString(const PathString & other)
		: path(other.path)
	{
		return;
	}
	PathString & assign(const PathString & other)
	{
		path = other.path;
		return *this;
	}
	PathString & operator = (const PathString & other)
	{
		return assign(other);
	}
	PathString & append(const PathString & other)
	{
		path.append(other.path);
		return *this;
	}
	PathString & operator += (const PathString & other)
	{
		return append(other);
	}
	friend PathString operator + (const PathString & a, const PathString & b)
	{
		return PathString(a).append(b);
	}
	friend bool operator < (const PathString & a, const PathString & b)
	{
		return a.AsNative() < b.AsNative();
	}
	friend bool operator == (const PathString & a, const PathString & b)
	{
		return a.AsNative() == b.AsNative();
	}
	friend bool operator != (const PathString & a, const PathString & b)
	{
		return a.AsNative() != b.AsNative();
	}
	bool empty() const { return path.empty(); }

#if defined(WIN32)
	static int CompareNoCase(const PathString & a, const PathString & b)
	{
		return lstrcmpiW(a.ToWide().c_str(), b.ToWide().c_str());
	}
#endif

public:

#if defined(MODPLUG_TRACKER)

	void SplitPath(PathString *drive, PathString *dir, PathString *fname, PathString *ext) const;
	PathString GetDrive() const;		// Drive letter + colon, e.g. "C:"
	PathString GetDir() const;			// Directory, e.g. "\OpenMPT\"
	PathString GetPath() const;			// Drive + Dir, e.g. "C:\OpenMPT\"
	PathString GetFileName() const;		// File name without extension, e.g. "mptrack"
	PathString GetFileExt() const;		// Extension including dot, e.g. ".exe"
	PathString GetFullFileName() const;	// File name + extension, e.g. "mptrack.exe"

	// Return the same path string with a different (or appended) extension (including "."), e.g. "foo.bar",".txt" -> "foo.txt" or "C:\OpenMPT\foo",".txt" -> "C:\OpenMPT\foo.txt"
	PathString ReplaceExt(const mpt::PathString &newExt) const;

	// Removes special characters from a filename component and replaces them with a safe replacement character ("_" on windows).
	// Returns the result.
	// Note that this also removes path component separators, so this should only be used on single-component PathString objects.
	PathString SanitizeComponent() const;

	bool HasTrailingSlash() const
	{
		if(empty())
			return false;
		const RawPathString::value_type &c = path[path.length() - 1];
#if defined(WIN32)
		if(c == L'\\' || c == L'/')
			return true;
#else
		if(c == '/')
			return true;
#endif
		return false;
	}

	// Relative / absolute paths conversion
	mpt::PathString AbsolutePathToRelative(const mpt::PathString &relativeTo) const;
	mpt::PathString RelativePathToAbsolute(const mpt::PathString &relativeTo) const;

#endif // MODPLUG_TRACKER

public:

#if defined(WIN32)

	// conversions
	MPT_DEPRECATED_PATH std::string ToLocale() const { return mpt::ToLocale(path); }
	std::string ToUTF8() const { return mpt::To(mpt::CharsetUTF8, path); }
	std::wstring ToWide() const { return path; }
	MPT_DEPRECATED_PATH static PathString FromLocale(const std::string &path) { return PathString(mpt::ToWide(mpt::CharsetLocale, path)); }
	static PathString FromUTF8(const std::string &path) { return PathString(mpt::ToWide(mpt::CharsetUTF8, path)); }
	static PathString FromWide(const std::wstring &path) { return PathString(path); }
	RawPathString AsNative() const { return path; }
	static PathString FromNative(const RawPathString &path) { return PathString(path); }
#if defined(_MFC_VER)
	// CString TCHAR, so this is CHAR or WCHAR, depending on UNICODE
	MPT_DEPRECATED_PATH CString ToCString() const { return mpt::ToCString(path); }
	MPT_DEPRECATED_PATH static PathString FromCString(const CString &path) { return PathString(mpt::ToWide(path)); }
	// Non-warning-generating versions of the above. Use with extra care.
	CString ToCStringSilent() const { return mpt::ToCString(path); }
	static PathString FromCStringSilent(const CString &path) { return PathString(mpt::ToWide(path)); }
	// really special purpose, if !UNICODE, encode unicode in CString as UTF8:
	static mpt::PathString TunnelOutofCString(const CString &path);
	static CString TunnelIntoCString(const mpt::PathString &path);
#endif

#else // !WIN32

	// conversions
	std::string ToLocale() const { return path; }
	std::string ToUTF8() const { return mpt::To(mpt::CharsetUTF8, mpt::CharsetLocale, path); }
	std::wstring ToWide() const { return mpt::ToWide(mpt::CharsetLocale, path); }
	static PathString FromLocale(const std::string &path) { return PathString(path); }
	static PathString FromUTF8(const std::string &path) { return PathString(mpt::To(mpt::CharsetLocale, mpt::CharsetUTF8, path)); }
	static PathString FromWide(const std::wstring &path) { return PathString(mpt::To(mpt::CharsetLocale, path)); }
	RawPathString AsNative() const { return path; }
	static PathString FromNative(const RawPathString &path) { return PathString(path); }

#endif // WIN32

};

MPT_DEPRECATED_PATH static inline std::string ToString(const mpt::PathString & x) { return mpt::ToLocale(x.ToWide()); }
static inline std::wstring ToWString(const mpt::PathString & x) { return x.ToWide(); }

} // namespace mpt

#if defined(WIN32)

#define MPT_PATHSTRING(x) mpt::PathString::FromNative( L ## x )

#else // !WIN32

#define MPT_PATHSTRING(x) mpt::PathString::FromNative( x )

#endif // WIN32

FILE * mpt_fopen(const mpt::PathString &filename, const char *mode);
FILE * mpt_fopen(const mpt::PathString &filename, const wchar_t *mode);

#if defined(MODPLUG_TRACKER)

// Sanitize a filename (remove special chars)
void SanitizeFilename(mpt::PathString &filename);

void SanitizeFilename(char *beg, char *end);
void SanitizeFilename(wchar_t *beg, wchar_t *end);

void SanitizeFilename(std::string &str);
void SanitizeFilename(std::wstring &str);

template <std::size_t size>
void SanitizeFilename(char (&buffer)[size])
//-----------------------------------------
{
	STATIC_ASSERT(size > 0);
	SanitizeFilename(buffer, buffer + size);
}

template <std::size_t size>
void SanitizeFilename(wchar_t (&buffer)[size])
//--------------------------------------------
{
	STATIC_ASSERT(size > 0);
	SanitizeFilename(buffer, buffer + size);
}

#if defined(_MFC_VER)
MPT_DEPRECATED_PATH void SanitizeFilename(CString &str);
#endif

#endif // MODPLUG_TRACKER
