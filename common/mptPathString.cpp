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

#include "mpt/path/os_path_long.hpp"

#include <vector>

#if MPT_OS_WINDOWS
#include <tchar.h>
#endif

#if MPT_OS_WINDOWS
#include <windows.h>
#endif

OPENMPT_NAMESPACE_BEGIN



namespace mpt
{



mpt::RawPathString SupportLongPath(const mpt::RawPathString &path)
{
	return mpt::support_long_path(path);
}



#if MPT_OS_WINDOWS

#if !MPT_OS_WINDOWS_WINRT

int PathCompareNoCase(const PathString & a, const PathString & b)
{
	return lstrcmpi(a.AsNative().c_str(), b.AsNative().c_str());
}

#endif // !MPT_OS_WINDOWS_WINRT

#endif // MPT_OS_WINDOWS



#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS


// Convert an absolute path to a path that's relative to "&relativeTo".
mpt::PathString AbsolutePathToRelative(const mpt::PathString &path, const mpt::PathString &relativeTo)
{
	using namespace path_literals;
	using char_type = RawPathString::value_type;
	mpt::PathString result = path;
	if(path.empty())
	{
		return result;
	}
	if(!_tcsncicmp(relativeTo.AsNative().c_str(), path.AsNative().c_str(), relativeTo.AsNative().length()))
	{
		// Path is OpenMPT's directory or a sub directory ("C:\OpenMPT\Somepath" => ".\Somepath")
		result = mpt::PathString::FromNative(L<char_type>(".\\")); // ".\"
		result += mpt::PathString::FromNative(path.AsNative().substr(relativeTo.AsNative().length()));
	} else if(!_tcsncicmp(relativeTo.AsNative().c_str(), path.AsNative().c_str(), 2))
	{
		// Path is on the same drive as OpenMPT ("C:\Somepath" => "\Somepath")
		result = mpt::PathString::FromNative(path.AsNative().substr(2));
	}
	return result;
}


// Convert a path that is relative to "&relativeTo" to an absolute path.
mpt::PathString RelativePathToAbsolute(const mpt::PathString &path, const mpt::PathString &relativeTo)
{
	using namespace path_literals;
	using char_type = RawPathString::value_type;
	mpt::PathString result = path;
	if(path.empty())
	{
		return result;
	}
	if(path.length() >= 2 && path.AsNative()[0] == L<char_type>('\\') && path.AsNative()[1] == L<char_type>('\\'))
	{
		// Network / UNC paths
		return result;
	} if(path.length() >= 1 && path.AsNative()[0] == L<char_type>('\\'))
	{
		// Path is on the same drive as relativeTo ("\Somepath\" => "C:\Somepath\")
		result = mpt::PathString::FromNative(relativeTo.AsNative().substr(0, 2));
		result += mpt::PathString(path);
	} else if(path.length() >= 2 && path.AsNative().substr(0, 2) == L<char_type>(".\\"))
	{
		// Path is in relativeTo or a sub directory (".\Somepath\" => "C:\OpenMPT\Somepath\")
		result = relativeTo; // "C:\OpenMPT\"
		result += mpt::PathString::FromNative(path.AsNative().substr(2));
	} else if(path.length() < 3 || path.AsNative()[1] != L<char_type>(':') || path.AsNative()[2] != L<char_type>('\\'))
	{
		// Any other path not starting with drive letter
		result = relativeTo;  // "C:\OpenMPT\"
		result += mpt::PathString(path);
	}
	return result;
}


#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS



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
