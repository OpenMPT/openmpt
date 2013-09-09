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
		case CharsetLocale:      return "char";        break;
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


} } // namespace mpt::String
