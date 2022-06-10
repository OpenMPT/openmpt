/*
 * mptPathString.cpp
 * -----------------
 * Purpose: Wrapper class around the platform-native representation of path names. Should be the only type that is used to store path names.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "mptPathString.h"

#include "mpt/string_transcode/transcode.hpp"

#if MPT_OS_WINDOWS
#include <windows.h>
#include <tchar.h>
#endif

OPENMPT_NAMESPACE_BEGIN



namespace mpt
{

mpt::RawPathString SupportLongPath(const mpt::RawPathString &path)
{
#if MPT_OS_WINDOWS
#if MPT_OS_WINDOWS_WINRT && (_WIN32_WINNT < 0x0a00)
	// For WinRT on Windows 8, there is no official wy to determine an absolute path.
	return path;
#else // !MPT_OS_WINDOWS_WINRT
	if(path.length() < MAX_PATH || path.substr(0, 4) == PL_("\\\\?\\"))
	{
		// Path is short enough or already in prefixed form
		return path;
	}
	const RawPathString absPath = mpt::GetAbsolutePath(mpt::PathString::FromNative(path)).AsNative();
	if(absPath.substr(0, 2) == PL_("\\\\"))
	{
		// Path is a network share: \\server\foo.bar -> \\?\UNC\server\foo.bar
		return PL_("\\\\?\\UNC") + absPath.substr(1);
	} else
	{
		// Regular file: C:\foo.bar -> \\?\C:\foo.bar
		return PL_("\\\\?\\") + absPath;
	}
#endif // MPT_OS_WINDOWS_WINRT
#else // !MPT_OS_WINDOWS
	return path;
#endif // MPT_OS_WINDOWS
}

} // namespace mpt



#if MPT_OS_WINDOWS

namespace mpt
{



#if !MPT_OS_WINDOWS_WINRT

int PathString::CompareNoCase(const PathString & a, const PathString & b)
{
	return lstrcmpi(a.path.c_str(), b.path.c_str());
}

#endif // !MPT_OS_WINDOWS_WINRT



} // namespace mpt

#endif // MPT_OS_WINDOWS



namespace mpt
{

// Convert a path to its simplified form, i.e. remove ".\" and "..\" entries
// Note: We use our own implementation as PathCanonicalize is limited to MAX_PATH
// and unlimited versions are only available on Windows 8 and later.
// Furthermore, we also convert forward-slashes to backslashes and always remove trailing slashes.
PathString PathString::Simplify() const
{
#if MPT_OS_WINDOWS
	if(path.empty())
	{
		return PathString();
	}

	std::vector<RawPathString> components;
	RawPathString root;
	RawPathString::size_type startPos = 0;
	if(path.size() >= 2 && path[1] == PC_(':'))
	{
		// Drive letter
		root = path.substr(0, 2) + PC_('\\');
		startPos = 2;
	} else if(path.substr(0, 2) == PL_("\\\\"))
	{
		// Network share
		root = PL_("\\\\");
		startPos = 2;
	} else if(path.substr(0, 2) == PL_(".\\") || path.substr(0, 2) == PL_("./"))
	{
		// Special case for relative paths
		root = PL_(".\\");
		startPos = 2;
	} else if(path.size() >= 1 && (path[0] == PC_('\\') || path[0] == PC_('/')))
	{
		// Special case for relative paths
		root = PL_("\\");
		startPos = 1;
	}

	while(startPos < path.size())
	{
		auto pos = path.find_first_of(PL_("\\/"), startPos);
		if(pos == RawPathString::npos)
		{
			pos = path.size();
		}
		mpt::RawPathString dir = path.substr(startPos, pos - startPos);
		if(dir == PL_(".."))
		{
			// Go back one directory
			if(!components.empty())
			{
				components.pop_back();
			}
		} else if(dir == PL_("."))
		{
			// nop
		} else if(!dir.empty())
		{
			components.push_back(std::move(dir));
		}
		startPos = pos + 1;
	}

	RawPathString result = root;
	result.reserve(path.size());
	for(const auto &component : components)
	{
		result += component + PL_("\\");
	}
	if(!components.empty())
	{
		result.pop_back();
	}
	return mpt::PathString(result);
#else // !MPT_OS_WINDOWS
	if(path.empty())
	{
		return PathString();
	}

	std::vector<RawPathString> components;
	RawPathString root;
	RawPathString::size_type startPos = 0;
	if(path.substr(0, 2) == PL_("./"))
	{
		// Special case for relative paths
		root = PL_("./");
		startPos = 2;
	} else if(path.size() >= 1 && (path[0] == PC_('/')))
	{
		// Special case for relative paths
		root = PL_("/");
		startPos = 1;
	}

	while(startPos < path.size())
	{
		auto pos = path.find_first_of(PL_("/"), startPos);
		if(pos == RawPathString::npos)
		{
			pos = path.size();
		}
		mpt::RawPathString dir = path.substr(startPos, pos - startPos);
		if(dir == PL_(".."))
		{
			// Go back one directory
			if(!components.empty())
			{
				components.pop_back();
			}
		} else if(dir == PL_("."))
		{
			// nop
		} else if(!dir.empty())
		{
			components.push_back(std::move(dir));
		}
		startPos = pos + 1;
	}

	RawPathString result = root;
	result.reserve(path.size());
	for(const auto &component : components)
	{
		result += component + PL_("/");
	}
	if(!components.empty())
	{
		result.pop_back();
	}
	return mpt::PathString(result);
#endif // MPT_OS_WINDOWS
}

} // namespace mpt



namespace mpt
{



mpt::ustring PathString::ToUnicode() const
{
	return mpt::transcode<mpt::ustring>(path);
}

PathString PathString::FromUnicode(const mpt::ustring &path)
{
	return PathString(mpt::transcode<mpt::RawPathString>(path));
}


std::string PathString::ToUTF8() const
{
	return mpt::transcode<std::string>(mpt::common_encoding::utf8, path);
}

PathString PathString::FromUTF8(const std::string &path)
{
	return PathString(mpt::transcode<mpt::RawPathString>(mpt::common_encoding::utf8, path));
}


#if MPT_WSTRING_CONVERT

std::wstring PathString::ToWide() const
{
	return mpt::transcode<std::wstring>(path);
}

PathString PathString::FromWide(const std::wstring &path)
{
	return PathString(mpt::transcode<mpt::RawPathString>(path));
}

#endif // MPT_WSTRING_CONVERT


#if defined(MPT_ENABLE_CHARSET_LOCALE)

std::string PathString::ToLocale() const
{
	return mpt::transcode<std::string>(mpt::logical_encoding::locale, path);
}

PathString PathString::FromLocale(const std::string &path)
{
	return PathString(mpt::transcode<mpt::RawPathString>(mpt::logical_encoding::locale, path));
}

#endif // MPT_ENABLE_CHARSET_LOCALE


#if defined(MPT_WITH_MFC)

CString PathString::ToCString() const
{
	return mpt::transcode<CString>(path);
}

PathString PathString::FromCString(const CString &path)
{
	return PathString(mpt::transcode<mpt::RawPathString>(path));
}

#endif // MPT_WITH_MFC



#if MPT_OS_WINDOWS

void PathString::SplitPath(PathString *prefix, PathString *drive, PathString *dir, PathString *fbase, PathString *fext) const
{
	// We cannot use CRT splitpath here, because:
	//  * limited to _MAX_PATH or similar
	//  * no support for UNC paths
	//  * no support for \\?\ prefixed paths

	if(prefix) *prefix = mpt::PathString();
	if(drive) *drive = mpt::PathString();
	if(dir) *dir = mpt::PathString();
	if(fbase) *fbase = mpt::PathString();
	if(fext) *fext = mpt::PathString();

	mpt::RawPathString p = path;

	// remove \\?\\ prefix
	if(p.substr(0, 8) == PL_("\\\\?\\UNC\\"))
	{
		if(prefix) *prefix = P_("\\\\?\\UNC");
		p = PL_("\\\\") + p.substr(8);
	} else if(p.substr(0, 4) == PL_("\\\\?\\"))
	{
		if (prefix) *prefix = P_("\\\\?\\");
		p = p.substr(4);
	}

	if (p.length() >= 2 && (
		p.substr(0, 2) == PL_("\\\\")
		|| p.substr(0, 2) == PL_("\\/")
		|| p.substr(0, 2) == PL_("/\\")
		|| p.substr(0, 2) == PL_("//")
		))
	{ // UNC
		mpt::RawPathString::size_type first_slash = p.substr(2).find_first_of(PL_("\\/"));
		if(first_slash != mpt::RawPathString::npos)
		{
			mpt::RawPathString::size_type second_slash = p.substr(2 + first_slash + 1).find_first_of(PL_("\\/"));
			if(second_slash != mpt::RawPathString::npos)
			{
				if(drive) *drive = mpt::PathString::FromNative(p.substr(0, 2 + first_slash + 1 + second_slash));
				p = p.substr(2 + first_slash + 1 + second_slash);
			} else
			{
				if(drive) *drive = mpt::PathString::FromNative(p);
				p = mpt::RawPathString();
			}
		} else
		{
			if(drive) *drive = mpt::PathString::FromNative(p);
			p = mpt::RawPathString();
		}
	} else
	{ // local
		if(p.length() >= 2 && (p[1] == PC_(':')))
		{
			if(drive) *drive = mpt::PathString::FromNative(p.substr(0, 2));
			p = p.substr(2);
		} else
		{
			if(drive) *drive = mpt::PathString();
		}
	}
	mpt::RawPathString::size_type last_slash = p.find_last_of(PL_("\\/"));
	if(last_slash != mpt::RawPathString::npos)
	{
		if(dir) *dir = mpt::PathString::FromNative(p.substr(0, last_slash + 1));
		p = p.substr(last_slash + 1);
	} else
	{
		if(dir) *dir = mpt::PathString();
	}
	mpt::RawPathString::size_type last_dot = p.find_last_of(PL_("."));
	if(last_dot == mpt::RawPathString::npos)
	{
		if(fbase) *fbase = mpt::PathString::FromNative(p);
		if(fext) *fext = mpt::PathString();
	} else if(last_dot == 0)
	{
		if(fbase) *fbase = mpt::PathString::FromNative(p);
		if(fext) *fext = mpt::PathString();
	} else if(p == PL_(".") || p == PL_(".."))
	{
		if(fbase) *fbase = mpt::PathString::FromNative(p);
		if(fext) *fext = mpt::PathString();
	} else
	{
		if(fbase) *fbase = mpt::PathString::FromNative(p.substr(0, last_dot));
		if(fext) *fext = mpt::PathString::FromNative(p.substr(last_dot));
	}

}

PathString PathString::GetPrefix() const
{
	PathString prefix;
	SplitPath(&prefix, nullptr, nullptr, nullptr, nullptr);
	return prefix;
}
PathString PathString::GetDrive() const
{
	PathString drive;
	SplitPath(nullptr, &drive, nullptr, nullptr, nullptr);
	return drive;
}
PathString PathString::GetDirectory() const
{
	PathString dir;
	SplitPath(nullptr, nullptr, &dir, nullptr, nullptr);
	return dir;
}
PathString PathString::GetDirectoryWithDrive() const
{
	PathString drive, dir;
	SplitPath(nullptr, &drive, &dir, nullptr, nullptr);
	return drive + dir;
}
PathString PathString::GetFilenameBase() const
{
	PathString fname;
	SplitPath(nullptr, nullptr, nullptr, &fname, nullptr);
	return fname;
}
PathString PathString::GetFilenameExtension() const
{
	PathString ext;
	SplitPath(nullptr, nullptr, nullptr, nullptr, &ext);
	return ext;
}
PathString PathString::GetFilename() const
{
	PathString name, ext;
	SplitPath(nullptr, nullptr, nullptr, &name, &ext);
	return name + ext;
}

#else // !MPT_OS_WINDOWS

void PathString::SplitPath(PathString *dir, PathString *fbase, PathString *fext) const
{

	if(dir) *dir = mpt::PathString();
	if(fbase) *fbase = mpt::PathString();
	if(fext) *fext = mpt::PathString();

	mpt::RawPathString p = path;

	mpt::RawPathString::size_type last_slash = p.find_last_of(PL_("/"));
	if(last_slash != mpt::RawPathString::npos)
	{
		if(dir) *dir = mpt::PathString::FromNative(p.substr(0, last_slash + 1));
		p = p.substr(last_slash + 1);
	} else
	{
		if(dir) *dir = mpt::PathString();
	}
	mpt::RawPathString::size_type last_dot = p.find_last_of(PL_("."));
	if(last_dot == mpt::RawPathString::npos)
	{
		if(fbase) *fbase = mpt::PathString::FromNative(p);
		if(fext) *fext = mpt::PathString();
	} else if(last_dot == 0)
	{
		if(fbase) *fbase = mpt::PathString::FromNative(p);
		if(fext) *fext = mpt::PathString();
	} else if(p == PL_(".") || p == PL_(".."))
	{
		if(fbase) *fbase = mpt::PathString::FromNative(p);
		if(fext) *fext = mpt::PathString();
	} else
	{
		if(fbase) *fbase = mpt::PathString::FromNative(p.substr(0, last_dot));
		if(fext) *fext = mpt::PathString::FromNative(p.substr(last_dot));
	}

}

PathString PathString::GetPrefix() const
{
	return mpt::PathString();
}
PathString PathString::GetDrive() const
{
	return mpt::PathString();
}
PathString PathString::GetDirectory() const
{
	PathString dir;
	SplitPath(&dir, nullptr, nullptr);
	return dir;
}
PathString PathString::GetDirectoryWithDrive() const
{
	PathString dir;
	SplitPath(&dir, nullptr, nullptr);
	return dir;
}
PathString PathString::GetFilenameBase() const
{
	PathString fname;
	SplitPath(nullptr, &fname, nullptr);
	return fname;
}
PathString PathString::GetFilenameExtension() const
{
	PathString ext;
	SplitPath(nullptr, nullptr, &ext);
	return ext;
}
PathString PathString::GetFilename() const
{
	PathString name, ext;
	SplitPath(nullptr, &name, &ext);
	return name + ext;
}

#endif // MPT_OS_WINDOWS



PathString PathString::ReplaceExtension(const mpt::PathString &newExt) const
{
	return GetDirectoryWithDrive() + GetFilenameBase() + newExt;
}



#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS


// Convert an absolute path to a path that's relative to "&relativeTo".
PathString PathString::AbsolutePathToRelative(const PathString &relativeTo) const
{
	mpt::PathString result = *this;
	if(path.empty())
	{
		return result;
	}
	if(!_tcsncicmp(relativeTo.AsNative().c_str(), AsNative().c_str(), relativeTo.AsNative().length()))
	{
		// Path is OpenMPT's directory or a sub directory ("C:\OpenMPT\Somepath" => ".\Somepath")
		result = P_(".\\"); // ".\"
		result += mpt::PathString::FromNative(AsNative().substr(relativeTo.AsNative().length()));
	} else if(!_tcsncicmp(relativeTo.AsNative().c_str(), AsNative().c_str(), 2))
	{
		// Path is on the same drive as OpenMPT ("C:\Somepath" => "\Somepath")
		result = mpt::PathString::FromNative(AsNative().substr(2));
	}
	return result;
}


// Convert a path that is relative to "&relativeTo" to an absolute path.
PathString PathString::RelativePathToAbsolute(const PathString &relativeTo) const
{
	mpt::PathString result = *this;
	if(path.empty())
	{
		return result;
	}
	if(path.length() >= 2 && path[0] == PC_('\\') && path[1] == PC_('\\'))
	{
		// Network / UNC paths
		return result;
	} if(path.length() >= 1 && path[0] == PC_('\\'))
	{
		// Path is on the same drive as relativeTo ("\Somepath\" => "C:\Somepath\")
		result = mpt::PathString::FromNative(relativeTo.AsNative().substr(0, 2));
		result += mpt::PathString(path);
	} else if(path.length() >= 2 && path.substr(0, 2) == PL_(".\\"))
	{
		// Path is in relativeTo or a sub directory (".\Somepath\" => "C:\OpenMPT\Somepath\")
		result = relativeTo; // "C:\OpenMPT\"
		result += mpt::PathString::FromNative(AsNative().substr(2));
	} else if(path.length() < 3 || path[1] != PC_(':') || path[2] != PC_('\\'))
	{
		// Any other path not starting with drive letter
		result = relativeTo;  // "C:\OpenMPT\"
		result += mpt::PathString(path);
	}
	return result;
}


#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS



bool PathString::IsPathSeparator(RawPathString::value_type c)
{
#if MPT_OS_WINDOWS || MPT_OS_DJGPP
	return (c == PC_('\\')) || (c == PC_('/'));
#else
	return c == PC_('/');
#endif
}

RawPathString::value_type PathString::GetDefaultPathSeparator()
{
#if MPT_OS_WINDOWS || MPT_OS_DJGPP
	return PC_('\\');
#else
	return PC_('/');
#endif
}



bool PathString::IsAbsolute() const
{
#if MPT_OS_WINDOWS
	if(path.substr(0, 8) == PL_("\\\\?\\UNC\\"))
	{
		return true;
	}
	if(path.substr(0, 4) == PL_("\\\\?\\"))
	{
		return true;
	}
	if(path.substr(0, 2) == PL_("\\\\"))
	{
		return true; // UNC
	}
	if(path.substr(0, 2) == PL_("//"))
	{
		return true; // UNC
	}
	return (path.length()) >= 3 && (path[1] == ':') && IsPathSeparator(path[2]);
#else
	return (path.length() >= 1) && IsPathSeparator(path[0]);
#endif
}



#if MPT_OS_WINDOWS

#if !(MPT_OS_WINDOWS_WINRT && (_WIN32_WINNT < 0x0a00))

mpt::PathString GetAbsolutePath(const mpt::PathString &path)
{
	DWORD size = GetFullPathName(path.AsNative().c_str(), 0, nullptr, nullptr);
	if(size == 0)
	{
		return path;
	}
	std::vector<TCHAR> fullPathName(size, TEXT('\0'));
	if(GetFullPathName(path.AsNative().c_str(), size, fullPathName.data(), nullptr) == 0)
	{
		return path;
	}
	return mpt::PathString::FromNative(fullPathName.data());
}

#endif

#endif // MPT_OS_WINDOWS



} // namespace mpt



#if defined(MODPLUG_TRACKER)



static inline char SanitizePathComponentChar(char c)
{
	if(	c == '\\' ||
		c == '\"' ||
		c == '/'  ||
		c == ':'  ||
		c == '?'  ||
		c == '<'  ||
		c == '>'  ||
		c == '|'  ||
		c == '*')
	{
		c = '_';
	}
	return c;
}

static inline wchar_t SanitizePathComponentChar(wchar_t c)
{
	if(	c == L'\\' ||
		c == L'\"' ||
		c == L'/'  ||
		c == L':'  ||
		c == L'?'  ||
		c == L'<'  ||
		c == L'>'  ||
		c == L'|'  ||
		c == L'*')
	{
		c = L'_';
	}
	return c;
}

#if MPT_CXX_AT_LEAST(20)
static inline char8_t SanitizePathComponentChar(char8_t c)
{
	if(	c == u8'\\' ||
		c == u8'\"' ||
		c == u8'/'  ||
		c == u8':'  ||
		c == u8'?'  ||
		c == u8'<'  ||
		c == u8'>'  ||
		c == u8'|'  ||
		c == u8'*')
	{
		c = u8'_';
	}
	return c;
}
#endif

mpt::PathString SanitizePathComponent(const mpt::PathString &filename)
{
	mpt::RawPathString tmp = filename.AsNative();
	for(auto &c : tmp)
	{
		c = SanitizePathComponentChar(c);
	}
	return mpt::PathString::FromNative(tmp);
}

std::string SanitizePathComponent(std::string str)
{
	for(size_t i = 0; i < str.length(); i++)
	{
		str[i] = SanitizePathComponentChar(str[i]);
	}
	return str;
}

std::wstring SanitizePathComponent(std::wstring str)
{
	for(size_t i = 0; i < str.length(); i++)
	{
		str[i] = SanitizePathComponentChar(str[i]);
	}
	return str;
}

#if MPT_USTRING_MODE_UTF8
mpt::u8string SanitizePathComponent(mpt::u8string str)
{
	for(size_t i = 0; i < str.length(); i++)
	{
		str[i] = SanitizePathComponentChar(str[i]);
	}
	return str;
}
#endif // MPT_USTRING_MODE_UTF8

#if defined(MPT_WITH_MFC)
CString SanitizePathComponent(CString str)
{
	for(int i = 0; i < str.GetLength(); i++)
	{
		str.SetAt(i, SanitizePathComponentChar(str.GetAt(i)));
	}
	return str;
}
#endif // MPT_WITH_MFC



#endif // MODPLUG_TRACKER



OPENMPT_NAMESPACE_END
