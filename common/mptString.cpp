/*
 * mptString.cpp
 * -------------
 * Purpose: A wrapper around std::string implemeting the CString.
 * Notes  : Should be removed somewhen in the future when all uses of CString have been converted to std::string.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "mptString.h"

#include <stdexcept>
#include <vector>
#include <cstdarg>

#if !defined(WIN32)
#include <iconv.h>
#endif // !WIN32


namespace mpt { namespace String {


std::string Format(const char *format, ...)
{
	#if MPT_COMPILER_MSVC
		va_list argList;
		va_start(argList, format);

		// Count the needed array size.
		const size_t nCount = _vscprintf(format, argList); // null character not included.
		std::vector<char> buf(nCount + 1); // + 1 is for null terminator.
		vsprintf_s(&(buf[0]), buf.size(), format, argList);

		va_end(argList);
		return &(buf[0]);
	#else
		va_list argList;
		va_start(argList, format);
		int size = vsnprintf(NULL, 0, format, argList); // get required size, requires c99 compliant vsnprintf which msvc does not have
		va_end(argList);
		std::vector<char> temp(size + 1);
		va_start(argList, format);
		vsnprintf(&(temp[0]), size + 1, format, argList);
		va_end(argList);
		return &(temp[0]);
	#endif
}


#if defined(WIN32)
static UINT CharsetToCodepage(Charset charset)
{
	switch(charset)
	{
		case CharsetLocale:      return CP_ACP;  break;
		case CharsetUTF8:        return CP_UTF8; break;
		case CharsetUS_ASCII:    return 20127;   break;
		case CharsetISO8859_1:   return 28591;   break;
		case CharsetISO8859_15:  return 28605;   break;
		case CharsetCP437:       return 437;     break;
		case CharsetWindows1252: return 1252;    break;
	}
	return 0;
}
#else // !WIN32
static const char * CharsetToString(Charset charset)
{
	switch(charset)
	{
		case CharsetLocale:      return "";            break; // "char" breaks with glibc when no locale is set
		case CharsetUTF8:        return "UTF-8";       break;
		case CharsetUS_ASCII:    return "ASCII";       break;
		case CharsetISO8859_1:   return "ISO-8859-1";  break;
		case CharsetISO8859_15:  return "ISO-8859-15"; break;
		case CharsetCP437:       return "CP437";       break;
		case CharsetWindows1252: return "CP1252";      break;
	}
	return 0;
}
#endif // WIN32


std::string Encode(const std::wstring &src, Charset charset)
{
	#if defined(WIN32)
		const UINT codepage = CharsetToCodepage(charset);
		int required_size = WideCharToMultiByte(codepage, 0, src.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if(required_size <= 0)
		{
			return std::string();
		}
		std::vector<CHAR> encoded_string(required_size);
		WideCharToMultiByte(codepage, 0, src.c_str(), -1, &encoded_string[0], required_size, nullptr, nullptr);
		return &encoded_string[0];
	#else // !WIN32
		iconv_t conv = iconv_t();
		conv = iconv_open(CharsetToString(charset), "wchar_t");
		if(!conv)
		{
			throw std::runtime_error("iconv conversion not working");
		}
		std::vector<wchar_t> wide_string(src.c_str(), src.c_str() + src.length() + 1);
		std::vector<char> encoded_string(wide_string.size() * 8); // large enough
		char * inbuf = (char*)&wide_string[0];
		size_t inbytesleft = wide_string.size() * sizeof(wchar_t);
		char * outbuf = &encoded_string[0];
		size_t outbytesleft = encoded_string.size();
		if(iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1)
		{
			iconv_close(conv);
			conv = iconv_t();
			return std::string();
		}
		iconv_close(conv);
		conv = iconv_t();
		return &encoded_string[0];
	#endif // WIN32
}


std::wstring Decode(const std::string &src, Charset charset)
{
	#if defined(WIN32)
		const UINT codepage = CharsetToCodepage(charset);
		int required_size = MultiByteToWideChar(codepage, 0, src.c_str(), -1, nullptr, 0);
		if(required_size <= 0)
		{
			return std::wstring();
		}
		std::vector<WCHAR> decoded_string(required_size);
		MultiByteToWideChar(codepage, 0, src.c_str(), -1, &decoded_string[0], required_size);
		return &decoded_string[0];
	#else // !WIN32
		iconv_t conv = iconv_t();
		conv = iconv_open("wchar_t", CharsetToString(charset));
		if(!conv)
		{
			throw std::runtime_error("iconv conversion not working");
		}
		std::vector<char> encoded_string(src.c_str(), src.c_str() + src.length() + 1);
		std::vector<wchar_t> wide_string(encoded_string.size() * 8); // large enough
		char * inbuf = &encoded_string[0];
		size_t inbytesleft = encoded_string.size();
		char * outbuf = (char*)&wide_string[0];
		size_t outbytesleft = wide_string.size() * sizeof(wchar_t);
		if(iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1)
		{
			iconv_close(conv);
			conv = iconv_t();
			return std::wstring();
		}
		iconv_close(conv);
		conv = iconv_t();
		return &wide_string[0];
	#endif // WIN32
}


std::string Convert(const std::string &src, Charset from, Charset to)
{
	#if defined(WIN32)
		return Encode(Decode(src, from), to);
	#else // !WIN32
		iconv_t conv = iconv_t();
		conv = iconv_open(CharsetToString(to), CharsetToString(from));
		if(!conv)
		{
			throw std::runtime_error("iconv conversion not working");
		}
		std::vector<char> src_string(src.c_str(), src.c_str() + src.length() + 1);
		std::vector<char> dst_string(src_string.size() * 8); // large enough
		char * inbuf = &src_string[0];
		size_t inbytesleft = src_string.size();
		char * outbuf = &dst_string[0];
		size_t outbytesleft = dst_string.size();
		if(iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1)
		{
			iconv_close(conv);
			conv = iconv_t();
			return std::string();
		}
		iconv_close(conv);
		conv = iconv_t();
		return &dst_string[0];
	#endif // WIN32
}


#if defined(_MFC_VER)

CString ToCString(const std::string &src, Charset charset)
{
	#ifdef UNICODE
		return mpt::String::Decode(src, charset).c_str();
	#else
		return mpt::String::Convert(src, charset, mpt::CharsetLocale).c_str();
	#endif
}

CString ToCString(const std::wstring &src)
{
	#ifdef UNICODE
		return src.c_str();
	#else
		return mpt::String::Encode(src, mpt::CharsetLocale).c_str();
	#endif
}

std::string FromCString(const CString &src, Charset charset)
{
	#ifdef UNICODE
		return mpt::String::Encode(src.GetString(), charset);
	#else
		return mpt::String::Convert(src.GetString(), mpt::CharsetLocale, charset);
	#endif
}

std::wstring FromCString(const CString &src)
{
	#ifdef UNICODE
		return src.GetString();
	#else
		return mpt::String::Decode(src.GetString(), mpt::CharsetLocale);
	#endif
}

#endif


} } // namespace mpt::String


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

} // namespace mpt

#endif


FILE * mpt_fopen(const mpt::PathString &filename, const char *mode)
//-----------------------------------------------------------------
{
	#if defined(WIN32)
		return _wfopen(filename.AsNative().c_str(), mode ? mpt::String::Decode(mode, mpt::CharsetLocale).c_str() : nullptr);
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
		return fopen(filename.AsNative().c_str(), mode ? mpt::String::Encode(mode, mpt::CharsetLocale).c_str() : nullptr);
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
//-----------------------------------------------
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
