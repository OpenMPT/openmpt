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
{
	PathString drive;
	SplitPath(&drive, nullptr, nullptr, nullptr);
	return drive;
}
PathString PathString::GetDir() const
{
	PathString dir;
	SplitPath(nullptr, &dir, nullptr, nullptr);
	return dir;
}
PathString PathString::GetPath() const
{
	PathString drive, dir;
	SplitPath(&drive, &dir, nullptr, nullptr);
	return drive + dir;
}
PathString PathString::GetFileName() const
{
	PathString fname;
	SplitPath(nullptr, nullptr, &fname, nullptr);
	return fname;
}
PathString PathString::GetFileExt() const
{
	PathString ext;
	SplitPath(nullptr, nullptr, nullptr, &ext);
	return ext;
}
PathString PathString::GetFullFileName() const
{
	PathString name, ext;
	SplitPath(nullptr, nullptr, &name, &ext);
	return name + ext;
}

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
