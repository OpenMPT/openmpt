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

#include "mpt/base/namespace.hpp"
#include "mpt/path/os_path.hpp"
#include "mpt/string/types.hpp"

#include "openmpt/base/FlagSet.hpp"

#include "mptString.h"

#include <vector>


OPENMPT_NAMESPACE_BEGIN

namespace mpt
{



#if defined(MPT_ENABLE_CHARSET_LOCALE)
using RawPathString = mpt::os_path;
#else // !MPT_ENABLE_CHARSET_LOCALE
using RawPathString = mpt::utf8string;
#endif // MPT_ENABLE_CHARSET_LOCALE



class PathString
{

private:

	RawPathString path;

private:

	explicit PathString(const RawPathString & path_)
		: path(path_)
	{
		return;
	}

public:

	PathString() = default;
	PathString(const PathString &) = default;
	PathString(PathString &&) noexcept = default;

	PathString & assign(const PathString & other)
	{
		path = other.path;
		return *this;
	}
	PathString & assign(PathString && other) noexcept
	{
		path = std::move(other.path);
		return *this;
	}
	PathString & operator = (const PathString & other)
	{
		return assign(other);
	}
	PathString &operator = (PathString && other) noexcept
	{
		return assign(std::move(other));
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

	std::size_t Length() const { return path.size(); }



public:

#if MPT_OS_WINDOWS
#if !MPT_OS_WINDOWS_WINRT
	static int CompareNoCase(const PathString & a, const PathString & b);
#endif // !MPT_OS_WINDOWS_WINRT
#endif

#if MPT_OS_WINDOWS

	void SplitPath(PathString *prefix, PathString *drive, PathString *dir, PathString *fbase, PathString *fext) const;
	// \\?\ prefixes will be removed and \\?\\UNC prefixes converted to canonical \\ form.

#else // !MPT_OS_WINDOWS

	void SplitPath(PathString *dir, PathString *fbase, PathString *fext) const;

#endif // MPT_OS_WINDOWS

	PathString GetPrefix() const;             // \\?\ or \\?\\UNC or empty
	PathString GetDrive() const;              // Drive letter + colon, e.g. "C:" or \\server\\share
	PathString GetDirectory() const;          // Directory, e.g. "\OpenMPT\"
	PathString GetDirectoryWithDrive() const; // Drive + Dir, e.g. "C:\OpenMPT\"
	PathString GetFilenameBase() const;       // File name without extension, e.g. "OpenMPT"
	PathString GetFilenameExtension() const;  // Extension including dot, e.g. ".exe"
	PathString GetFilename() const;           // File name + extension, e.g. "OpenMPT.exe"

	static bool IsPathSeparator(RawPathString::value_type c);
	static RawPathString::value_type GetDefaultPathSeparator();

	// Return the same path string with a different (or appended) extension (including "."), e.g. "foo.bar",".txt" -> "foo.txt" or "C:\OpenMPT\foo",".txt" -> "C:\OpenMPT\foo.txt"
	PathString ReplaceExtension(const mpt::PathString &newExt) const;

	bool HasTrailingSlash() const
	{
		if(path.empty())
		{
			return false;
		}
		RawPathString::value_type c = path[path.length() - 1];
		return IsPathSeparator(c);
	}
	mpt::PathString &EnsureTrailingSlash()
	{
		if(!path.empty() && !HasTrailingSlash())
		{
			path += GetDefaultPathSeparator();
		}
		return *this;
	}

	mpt::PathString WithoutTrailingSlash() const
	{
		mpt::PathString result = *this;
		while(result.HasTrailingSlash())
		{
			if(result.Length() == 1)
			{
				return result;
			}
			result = mpt::PathString(result.AsNative().substr(0, result.AsNative().length() - 1));
		}
		return result;
	}

	mpt::PathString WithTrailingSlash() const
	{
		mpt::PathString result = *this;
		result.EnsureTrailingSlash();
		return result;
	}

#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS

	// Relative / absolute paths conversion

	mpt::PathString AbsolutePathToRelative(const mpt::PathString &relativeTo) const; // similar to std::fs::path::lexically_approximate
	
	mpt::PathString RelativePathToAbsolute(const mpt::PathString &relativeTo) const;

#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS

	bool IsAbsolute() const;

public:



	RawPathString AsNative() const { return path; }
	static PathString FromNative(const RawPathString& path) { return PathString(path); }



	// Convert a path to its simplified form, i.e. remove ".\" and "..\" entries
	mpt::PathString Simplify() const;  // similar to std::fs::path::lexically_normal



	mpt::ustring ToUnicode() const;
	static PathString FromUnicode(const mpt::ustring &path);

	std::string ToUTF8() const;
	static PathString FromUTF8(const std::string &path);

#if MPT_WSTRING_CONVERT
	std::wstring ToWide() const;
	static PathString FromWide(const std::wstring &path);
#endif // MPT_WSTRING_CONVERT

#if defined(MPT_ENABLE_CHARSET_LOCALE)
	std::string ToLocale() const;
	static PathString FromLocale(const std::string &path);
#endif // MPT_ENABLE_CHARSET_LOCALE

#if defined(MPT_WITH_MFC)
	CString ToCString() const;
	static PathString FromCString(const CString &path);
#endif // MPT_WITH_MFC



};



#if defined(MPT_ENABLE_CHARSET_LOCALE)
inline std::string ToAString(const mpt::PathString & x) { return x.ToLocale(); }
#else
inline std::string ToAString(const mpt::PathString& x) { return x.ToUTF8(); }
#endif
inline mpt::ustring ToUString(const mpt::PathString & x) { return x.ToUnicode(); }
#if MPT_WSTRING_FORMAT
inline std::wstring ToWString(const mpt::PathString & x) { return x.ToWide(); }
#endif



#if defined(MPT_ENABLE_CHARSET_LOCALE)
#define MPT_PATHSTRING_LITERAL(x) MPT_OSPATH_LITERAL( x )
#define MPT_PATHSTRING(x) mpt::PathString::FromNative(MPT_OSPATH_LITERAL( x ))
#else // !MPT_ENABLE_CHARSET_LOCALE
#define MPT_PATHSTRING_LITERAL(x) ( x )
#define MPT_PATHSTRING(x) mpt::PathString::FromNative( x )
#endif // MPT_ENABLE_CHARSET_LOCALE

#define PC_(x) MPT_PATHSTRING_LITERAL(x)
#define PL_(x) MPT_PATHSTRING_LITERAL(x)
#define P_(x) MPT_PATHSTRING(x)



// Return native string, with possible \\?\ prefix if it exceeds MAX_PATH characters.
mpt::RawPathString SupportLongPath(const mpt::RawPathString &path);



#if MPT_OS_WINDOWS

#if !(MPT_OS_WINDOWS_WINRT && (_WIN32_WINNT < 0x0a00))

// Returns the absolute path for a potentially relative path and removes ".." or "." components. (same as GetFullPathNameW)
mpt::PathString GetAbsolutePath(const mpt::PathString &path);

#endif

#endif // MPT_OS_WINDOWS



#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS



namespace FS
{

	// Verify if this path represents a valid directory on the file system.
	bool IsDirectory(const mpt::PathString &path);
	// Verify if this path exists and is a file on the file system.
	bool IsFile(const mpt::PathString &path);
	// Verify that a path exists (no matter what type)
	bool PathExists(const mpt::PathString &path);

	// Deletes a complete directory tree. Handle with EXTREME care.
	// Returns false if any file could not be removed and aborts as soon as it
	// encounters any error. path must be absolute.
	bool DeleteDirectoryTree(mpt::PathString path);

} // namespace FS



// Returns the application executable path or an empty string (if unknown), e.g. "C:\mptrack\"
mpt::PathString GetExecutableDirectory();

#if !MPT_OS_WINDOWS_WINRT
// Returns the system directory path, e.g. "C:\Windows\System32\"
mpt::PathString GetSystemDirectory();
#endif // !MPT_OS_WINDOWS_WINRT

// Returns temporary directory (with trailing backslash added) (e.g. "C:\TEMP\")
mpt::PathString GetTempDirectory();



// Returns a new unique absolute path.
class TemporaryPathname
{
private:
	mpt::PathString m_Path;
public:
	TemporaryPathname(const mpt::PathString &fileNamePrefix = mpt::PathString(), const mpt::PathString &fileNameExtension = P_("tmp"));
public:
	mpt::PathString GetPathname() const
	{
		return m_Path;
	}
};



// Scoped temporary file guard. Deletes the file when going out of scope.
// The file itself is not created automatically.
class TempFileGuard
{
private:
	const mpt::PathString filename;
public:
	TempFileGuard(const mpt::TemporaryPathname &pathname = mpt::TemporaryPathname{});
	mpt::PathString GetFilename() const;
	~TempFileGuard();
};


// Scoped temporary directory guard. Deletes the directory when going out of scope.
// The directory itself is created automatically.
class TempDirGuard
{
private:
	mpt::PathString dirname;
public:
	TempDirGuard(const mpt::TemporaryPathname &pathname = mpt::TemporaryPathname{});
	mpt::PathString GetDirname() const;
	~TempDirGuard();
};



#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS

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


#if defined(MODPLUG_TRACKER)

enum FileTypeFormat
{
	FileTypeFormatNone           = 0   , // do not show extensions after description, i.e. "Foo Files"
	FileTypeFormatShowExtensions = 1<<0, // show extensions after descripten, i.e. "Foo Files (*.foo,*.bar)"
};
MPT_DECLARE_ENUM(FileTypeFormat)

class FileType
{
private:
	mpt::ustring m_ShortName; // "flac", "mod" (lowercase)
	mpt::ustring m_Description; // "FastTracker 2 Module"
	std::vector<std::string> m_MimeTypes; // "audio/ogg" (in ASCII)
	std::vector<mpt::PathString> m_Extensions; // "mod", "xm" (lowercase)
	std::vector<mpt::PathString> m_Prefixes; // "mod" for "mod.*"
public:
	FileType() { }
	FileType(const std::vector<FileType> &group)
	{
		for(const auto &type : group)
		{
			mpt::append(m_MimeTypes, type.m_MimeTypes);
			mpt::append(m_Extensions, type.m_Extensions);
			mpt::append(m_Prefixes, type.m_Prefixes);
		}
	}
	static FileType Any()
	{
		return FileType().ShortName(U_("*")).Description(U_("All Files")).AddExtension(P_("*"));
	}
public:
	FileType& ShortName(const mpt::ustring &shortName) { m_ShortName = shortName; return *this; }
	FileType& Description(const mpt::ustring &description) { m_Description = description; return *this; }
	FileType& MimeTypes(const std::vector<std::string> &mimeTypes) { m_MimeTypes = mimeTypes; return *this; }
	FileType& Extensions(const std::vector<mpt::PathString> &extensions) { m_Extensions = extensions; return *this; }
	FileType& Prefixes(const std::vector<mpt::PathString> &prefixes) { m_Prefixes = prefixes; return *this; }
	FileType& AddMimeType(const std::string &mimeType) { m_MimeTypes.push_back(mimeType); return *this; }
	FileType& AddExtension(const mpt::PathString &extension) { m_Extensions.push_back(extension); return *this; }
	FileType& AddPrefix(const mpt::PathString &prefix) { m_Prefixes.push_back(prefix); return *this; }
public:
	mpt::ustring GetShortName() const { return m_ShortName; }
	mpt::ustring GetDescription() const { return m_Description; }
	std::vector<std::string> GetMimeTypes() const { return m_MimeTypes; }
	std::vector<mpt::PathString> GetExtensions() const { return m_Extensions; }
	std::vector<mpt::PathString> GetPrefixes() const { return m_Prefixes; }
public:
	mpt::PathString AsFilterString(FlagSet<FileTypeFormat> format = FileTypeFormatNone) const;
	mpt::PathString AsFilterOnlyString() const;
}; // class FileType


// "Ogg Vorbis|*.ogg;*.oga|" // FileTypeFormatNone
// "Ogg Vorbis (*.ogg,*.oga)|*.ogg;*.oga|" // FileTypeFormatShowExtensions
mpt::PathString ToFilterString(const FileType &fileType, FlagSet<FileTypeFormat> format = FileTypeFormatNone);
mpt::PathString ToFilterString(const std::vector<FileType> &fileTypes, FlagSet<FileTypeFormat> format = FileTypeFormatNone);

// "*.ogg;*.oga" / ";*.ogg;*.oga"
mpt::PathString ToFilterOnlyString(const FileType &fileType, bool prependSemicolonWhenNotEmpty = false);
mpt::PathString ToFilterOnlyString(const std::vector<FileType> &fileTypes, bool prependSemicolonWhenNotEmpty = false);

#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_END
