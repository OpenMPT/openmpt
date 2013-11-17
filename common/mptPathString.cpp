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


#if defined(MODPLUG_TRACKER)

namespace mpt
{

void PathString::SplitPath(PathString *drive, PathString *dir, PathString *fname, PathString *ext) const
//------------------------------------------------------------------------------------------------------
{
	wchar_t tempDrive[_MAX_DRIVE];
	wchar_t tempDir[_MAX_DIR];
	wchar_t tempFname[_MAX_FNAME];
	wchar_t tempExt[_MAX_EXT];
	_wsplitpath(path.c_str(), tempDrive, tempDir, tempFname, tempExt);
	if(drive) *drive = mpt::PathString::FromNative(tempDrive);
	if(dir) *dir = mpt::PathString::FromNative(tempDir);
	if(fname) *fname = mpt::PathString::FromNative(tempFname);
	if(ext) *ext = mpt::PathString::FromNative(tempExt);
}

PathString PathString::GetDrive() const
//-------------------------------------
{
	PathString drive;
	SplitPath(&drive, nullptr, nullptr, nullptr);
	return drive;
}
PathString PathString::GetDir() const
//-----------------------------------
{
	PathString dir;
	SplitPath(nullptr, &dir, nullptr, nullptr);
	return dir;
}
PathString PathString::GetPath() const
//------------------------------------
{
	PathString drive, dir;
	SplitPath(&drive, &dir, nullptr, nullptr);
	return drive + dir;
}
PathString PathString::GetFileName() const
//----------------------------------------
{
	PathString fname;
	SplitPath(nullptr, nullptr, &fname, nullptr);
	return fname;
}
PathString PathString::GetFileExt() const
//---------------------------------------
{
	PathString ext;
	SplitPath(nullptr, nullptr, nullptr, &ext);
	return ext;
}
PathString PathString::GetFullFileName() const
//--------------------------------------------
{
	PathString name, ext;
	SplitPath(nullptr, nullptr, &name, &ext);
	return name + ext;
}


PathString PathString::ReplaceExt(const mpt::PathString &newExt) const
//--------------------------------------------------------------------
{
	return GetDrive() + GetDir() + GetFileName() + newExt;
}


PathString PathString::SanitizeComponent() const
//----------------------------------------------
{
	PathString result = *this;
	SanitizeFilename(result);
	return result;
}


// Convert an absolute path to a path that's relative to "&relativeTo".
PathString PathString::AbsolutePathToRelative(const PathString &relativeTo) const
//-------------------------------------------------------------------------------
{
	mpt::PathString result = path;
	if(path.empty())
	{
		return result;
	}
	if(!_wcsnicmp(relativeTo.AsNative().c_str(), AsNative().c_str(), relativeTo.AsNative().length()))
	{
		// Path is OpenMPT's directory or a sub directory ("C:\OpenMPT\Somepath" => ".\Somepath")
		result = MPT_PATHSTRING(".\\"); // ".\"
		result += mpt::PathString::FromNative(AsNative().substr(relativeTo.AsNative().length()));
	} else if(!_wcsnicmp(relativeTo.AsNative().c_str(), AsNative().c_str(), 2))
	{
		// Path is on the same drive as OpenMPT ("C:\Somepath" => "\Somepath")
		result = mpt::PathString::FromNative(AsNative().substr(2));
	}
	return result;
}


// Convert a path that is relative to "&relativeTo" to an absolute path.
PathString PathString::RelativePathToAbsolute(const PathString &relativeTo) const
//-------------------------------------------------------------------------------
{
	mpt::PathString result = path;
	if(path.empty())
	{
		return result;
	}
	if(AsNative().length() >= 2 && AsNative().substr(0, 1) == L"\\" && AsNative().substr(0, 2) != L"\\\\")
	{
		// Path is on the same drive as OpenMPT ("\Somepath\" => "C:\Somepath\"), but ignore network paths starting with "\\"
		result = mpt::PathString::FromNative(relativeTo.AsNative().substr(0, 2));
		result += path;
	} else if(AsNative().length() >= 2 && AsNative().substr(0, 2) == L".\\")
	{
		// Path is OpenMPT's directory or a sub directory (".\Somepath\" => "C:\OpenMPT\Somepath\")
		result = relativeTo; // "C:\OpenMPT\"
		result += mpt::PathString::FromNative(AsNative().substr(2));
	}
	return result;
}


#if defined(WIN32)
#if defined(_MFC_VER)

mpt::PathString PathString::TunnelOutofCString(const CString &path)
//-----------------------------------------------------------------
{
	#ifdef UNICODE
		return mpt::PathString::FromWide(path.GetString());
	#else
		// Since MFC code can call into our code from a lot of places, we cannot assume
		// that filenames we get from MFC are always encoded in our hacked UTF8-in-CString encoding.
		// Instead, we use a rough heuristic: if the string is parseable as UTF8, we assume it is.
		// This fails for CP_ACP strings, that are also valid UTF8. That's the trade-off here.
		if(mpt::To(mpt::CharsetUTF8, mpt::ToWide(mpt::CharsetUTF8, path.GetString())) == path.GetString())
		{
			// utf8
			return mpt::PathString::FromUTF8(path.GetString());
		} else
		{
			// ANSI
			return mpt::PathString::FromWide(mpt::ToWide(path));
		}
	#endif
}


CString PathString::TunnelIntoCString(const mpt::PathString &path)
//----------------------------------------------------------------
{
	#ifdef UNICODE
		return path.ToWide().c_str();
	#else
		return path.ToUTF8().c_str();
	#endif
}

#endif
#endif

} // namespace mpt

#endif


FILE * mpt_fopen(const mpt::PathString &filename, const char *mode)
//-----------------------------------------------------------------
{
	#if defined(WIN32)
		return _wfopen(filename.AsNative().c_str(), mode ? mpt::ToWide(mpt::CharsetLocale, mode).c_str() : nullptr);
	#else // !WIN32
		return fopen(filename.AsNative().c_str(), mode);
	#endif // WIN32
}

FILE * mpt_fopen(const mpt::PathString &filename, const wchar_t *mode)
//--------------------------------------------------------------------
{
	#if defined(WIN32)
		return _wfopen(filename.AsNative().c_str(), mode);
	#else // !WIN32
		return fopen(filename.AsNative().c_str(), mode ? mpt::ToLocale(mode).c_str() : nullptr);
	#endif // WIN32
}


#if defined(MODPLUG_TRACKER)

static inline char SanitizeFilenameChar(char c)
//---------------------------------------------
{
	if(	c == '\\' ||
		c == '\"' ||
		c == '/'  ||
		c == ':'  ||
		c == '?'  ||
		c == '<'  ||
		c == '>'  ||
		c == '*')
	{
		c = '_';
	}
	return c;
}

static inline wchar_t SanitizeFilenameChar(wchar_t c)
//---------------------------------------------------
{
	if(	c == L'\\' ||
		c == L'\"' ||
		c == L'/'  ||
		c == L':'  ||
		c == L'?'  ||
		c == L'<'  ||
		c == L'>'  ||
		c == L'*')
	{
		c = L'_';
	}
	return c;
}

void SanitizeFilename(mpt::PathString &filename)
//----------------------------------------------
{
	mpt::RawPathString tmp = filename.AsNative();
	for(mpt::RawPathString::iterator it = tmp.begin(); it != tmp.end(); ++it)
	{
		*it = SanitizeFilenameChar(*it);
	}
	filename = mpt::PathString::FromNative(tmp);
}

void SanitizeFilename(char *beg, char *end)
//-----------------------------------------
{
	for(char *it = beg; it != end; ++it)
	{
		*it = SanitizeFilenameChar(*it);
	}
}

void SanitizeFilename(wchar_t *beg, wchar_t *end)
//-----------------------------------------------
{
	for(wchar_t *it = beg; it != end; ++it)
	{
		*it = SanitizeFilenameChar(*it);
	}
}

void SanitizeFilename(std::string &str)
//-------------------------------------
{
	for(size_t i = 0; i < str.length(); i++)
	{
		str[i] = SanitizeFilenameChar(str[i]);
	}
}

void SanitizeFilename(std::wstring &str)
//--------------------------------------
{
	for(size_t i = 0; i < str.length(); i++)
	{
		str[i] = SanitizeFilenameChar(str[i]);
	}
}

#if defined(_MFC_VER)
void SanitizeFilename(CString &str)
//---------------------------------
{
	std::basic_string<TCHAR> tmp = str;
	SanitizeFilename(tmp);
	str = tmp.c_str();
}
#endif

#endif // MODPLUG_TRACKER
