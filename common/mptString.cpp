/*
 * mptString.cpp
 * -------------
 * Purpose: Small string-related utilities, number and message formatting.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "mptString.h"

#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

#include <cstdarg>

#if !defined(WIN32)
#include <iconv.h>
#endif // !WIN32


namespace mpt { namespace String {


#ifdef MODPLUG_TRACKER

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

#endif


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
static const char * CharsetToStringTranslit(Charset charset)
{
	switch(charset)
	{
		case CharsetLocale:      return "//TRANSLIT";            break; // "char" breaks with glibc when no locale is set
		case CharsetUTF8:        return "UTF-8//TRANSLIT";       break;
		case CharsetUS_ASCII:    return "ASCII//TRANSLIT";       break;
		case CharsetISO8859_1:   return "ISO-8859-1//TRANSLIT";  break;
		case CharsetISO8859_15:  return "ISO-8859-15//TRANSLIT"; break;
		case CharsetCP437:       return "CP437//TRANSLIT";       break;
		case CharsetWindows1252: return "CP1252//TRANSLIT";      break;
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
		conv = iconv_open(CharsetToStringTranslit(charset), "wchar_t");
		if(!conv)
		{
			conv = iconv_open(CharsetToString(charset), "wchar_t");
			if(!conv)
			{
				throw std::runtime_error("iconv conversion not working");
			}
		}
		std::vector<wchar_t> wide_string(src.c_str(), src.c_str() + src.length() + 1);
		std::vector<char> encoded_string(wide_string.size() * 8); // large enough
		char * inbuf = (char*)&wide_string[0];
		size_t inbytesleft = wide_string.size() * sizeof(wchar_t);
		char * outbuf = &encoded_string[0];
		size_t outbytesleft = encoded_string.size();
		while(iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1)
		{
			if(errno == EILSEQ || errno == EILSEQ)
			{
				inbuf += sizeof(wchar_t);
				inbytesleft -= sizeof(wchar_t);
				outbuf[0] = '?';
				outbuf++;
				outbytesleft--;
				iconv(conv, NULL, NULL, NULL, NULL); // reset state
			} else
			{
				iconv_close(conv);
				conv = iconv_t();
				return std::string();
			}
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
		while(iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1)
		{
			if(errno == EILSEQ || errno == EILSEQ)
			{
				inbuf++;
				inbytesleft--;
				for(std::size_t i = 0; i < sizeof(wchar_t); ++i)
				{
					outbuf[i] = 0;
				}
				#ifdef MPT_PLATFORM_BIG_ENDIAN
					outbuf[sizeof(wchar_t)-1 - 1] = 0xff; outbuf[sizeof(wchar_t)-1 - 0] = 0xfd;
				#else
					outbuf[1] = 0xff; outbuf[0] = 0xfd;
				#endif
				outbuf += sizeof(wchar_t);
				outbytesleft -= sizeof(wchar_t);
				iconv(conv, NULL, NULL, NULL, NULL); // reset state
			} else
			{
				iconv_close(conv);
				conv = iconv_t();
				return std::wstring();
			}
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
		conv = iconv_open(CharsetToStringTranslit(to), CharsetToString(from));
		if(!conv)
		{
			conv = iconv_open(CharsetToString(to), CharsetToString(from));
			if(!conv)
			{
				throw std::runtime_error("iconv conversion not working");
			}
		}
		std::vector<char> src_string(src.c_str(), src.c_str() + src.length() + 1);
		std::vector<char> dst_string(src_string.size() * 8); // large enough
		char * inbuf = &src_string[0];
		size_t inbytesleft = src_string.size();
		char * outbuf = &dst_string[0];
		size_t outbytesleft = dst_string.size();
		while(iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1)
		{
			if(errno == EILSEQ || errno == EILSEQ)
			{
				inbuf++;
				inbytesleft--;
				outbuf[0] = '?';
				outbuf++;
				outbytesleft--;
				iconv(conv, NULL, NULL, NULL, NULL); // reset state
			} else
			{
				iconv_close(conv);
				conv = iconv_t();
				return std::string();
			}
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



namespace mpt
{


template<typename Tstream, typename T> inline void SaneInsert(Tstream & s, const T & x) { s << x; }
// do the right thing for signed/unsigned char and bool
template<typename Tstream> void SaneInsert(Tstream & s, const bool & x) { s << static_cast<int>(x); }
template<typename Tstream> void SaneInsert(Tstream & s, const signed char & x) { s << static_cast<signed int>(x); }
template<typename Tstream> void SaneInsert(Tstream & s, const unsigned char & x) { s << static_cast<unsigned int>(x); }
 
template<typename T>
inline std::string ToStringHelper(const T & x)
{
	std::ostringstream o;
	o.imbue(std::locale::classic());
	SaneInsert(o, x);
	return o.str();
}

template<typename T>
inline std::wstring ToWStringHelper(const T & x)
{
	std::wostringstream o;
	o.imbue(std::locale::classic());
	SaneInsert(o, x);
	return o.str();
}

std::string ToString(const char & x) { return std::string(1, x); }
std::string ToString(const wchar_t & x) { return mpt::String::Encode(std::wstring(1, x), mpt::CharsetLocale); }
std::string ToString(const bool & x) { return ToStringHelper(x); }
std::string ToString(const signed char & x) { return ToStringHelper(x); }
std::string ToString(const unsigned char & x) { return ToStringHelper(x); }
std::string ToString(const signed short & x) { return ToStringHelper(x); }
std::string ToString(const unsigned short & x) { return ToStringHelper(x); }
std::string ToString(const signed int & x) { return ToStringHelper(x); }
std::string ToString(const unsigned int & x) { return ToStringHelper(x); }
std::string ToString(const signed long & x) { return ToStringHelper(x); }
std::string ToString(const unsigned long & x) { return ToStringHelper(x); }
std::string ToString(const signed long long & x) { return ToStringHelper(x); }
std::string ToString(const unsigned long long & x) { return ToStringHelper(x); }
std::string ToString(const float & x) { return ToStringHelper(x); }
std::string ToString(const double & x) { return ToStringHelper(x); }
std::string ToString(const long double & x) { return ToStringHelper(x); }

std::wstring ToWString(const char & x) { return mpt::String::Decode(std::string(1, x), mpt::CharsetLocale); }
std::wstring ToWString(const wchar_t & x) { return std::wstring(1, x); }
std::wstring ToWString(const bool & x) { return ToWStringHelper(x); }
std::wstring ToWString(const signed char & x) { return ToWStringHelper(x); }
std::wstring ToWString(const unsigned char & x) { return ToWStringHelper(x); }
std::wstring ToWString(const signed short & x) { return ToWStringHelper(x); }
std::wstring ToWString(const unsigned short & x) { return ToWStringHelper(x); }
std::wstring ToWString(const signed int & x) { return ToWStringHelper(x); }
std::wstring ToWString(const unsigned int & x) { return ToWStringHelper(x); }
std::wstring ToWString(const signed long & x) { return ToWStringHelper(x); }
std::wstring ToWString(const unsigned long & x) { return ToWStringHelper(x); }
std::wstring ToWString(const signed long long & x) { return ToWStringHelper(x); }
std::wstring ToWString(const unsigned long long & x) { return ToWStringHelper(x); }
std::wstring ToWString(const float & x) { return ToWStringHelper(x); }
std::wstring ToWString(const double & x) { return ToWStringHelper(x); }
std::wstring ToWString(const long double & x) { return ToWStringHelper(x); }


#if defined(MPT_FMT)


template<typename Tostream>
inline void ApplyFormat(Tostream & o, const Format & format)
{
	FormatFlags f = format.GetFlags();
	std::size_t width = format.GetWidth();
	int precision = format.GetPrecision();
	if(precision != -1 && width != 0 && !(f & fmt::NotaFix) && !(f & fmt::NotaSci))
	{
		// fixup:
		// precision behaves differently from .#
		// avoid default format when precision and width are set
		f &= ~fmt::NotaNrm;
		f |= fmt::NotaFix;
	}
	if(f & fmt::BaseDec) { o << std::dec; }
	else if(f & fmt::BaseHex) { o << std::hex; }
	if(f & fmt::NotaNrm ) { /*nothing*/ }
	else if(f & fmt::NotaFix ) { o << std::setiosflags(std::ios::fixed); }
	else if(f & fmt::NotaSci ) { o << std::setiosflags(std::ios::scientific); }
	if(f & fmt::CaseLow) { o << std::nouppercase; }
	else if(f & fmt::CaseUpp) { o << std::uppercase; }
	if(f & fmt::FillOff) { /* nothing */ }
	else if(f & fmt::FillNul) { o << std::setw(width) << std::setfill(typename Tostream::char_type('0')); }
	else if(f & fmt::FillSpc) { o << std::setw(width) << std::setfill(typename Tostream::char_type(' ')); }
	if(precision != -1) { o << std::setprecision(precision); }
}


template<typename T>
inline std::string FormatValHelper(const T & x, const Format & f)
{
	std::ostringstream o;
	o.imbue(std::locale::classic());
	ApplyFormat(o, f);
	SaneInsert(o, x);
	return o.str();
}

template<typename T>
inline std::wstring FormatValWHelper(const T & x, const Format & f)
{
	std::wostringstream o;
	o.imbue(std::locale::classic());
	ApplyFormat(o, f);
	SaneInsert(o, x);
	return o.str();
}

// Parses a useful subset of standard sprintf syntax for specifying floating point formatting.
template<typename Tchar>
inline Format ParseFormatStringFloat(const Tchar * str)
{
	ASSERT(str);
	FormatFlags f = FormatFlags();
	std::size_t width = 0;
	int precision = -1;
	if(!str)
	{
		return Format();
	}
	const Tchar * p = str;
	while(*p && *p != Tchar('%'))
	{
		++p;
	}
	++p;
	while(*p && (*p == Tchar(' ') || *p == Tchar('0')))
	{
		if(*p == Tchar(' ')) f |= mpt::fmt::FillSpc;
		if(*p == Tchar('0')) f |= mpt::fmt::FillNul;
		++p;
	}
	if(!(f & mpt::fmt::FillSpc) && !(f & mpt::fmt::FillNul))
	{
		f |= mpt::fmt::FillOff;
	}
	while(*p && (Tchar('0') <= *p && *p <= Tchar('9')))
	{
		if(f & mpt::fmt::FillOff)
		{
			f &= ~mpt::fmt::FillOff;
			f |= mpt::fmt::FillSpc;
		}
		width *= 10;
		width += *p - Tchar('0');
		++p;
	}
	if(*p && *p == Tchar('.'))
	{
		++p;
		precision = 0;
		while(*p && (Tchar('0') <= *p && *p <= Tchar('9')))
		{
			precision *= 10;
			precision += *p - Tchar('0');
			++p;
		}
	}
	if(*p && (*p == Tchar('g') || *p == Tchar('G') || *p == Tchar('f') || *p == Tchar('F') || *p == Tchar('e') || *p == Tchar('E')))
	{
		if(*p == Tchar('g')) f |= mpt::fmt::NotaNrm | mpt::fmt::CaseLow;
		if(*p == Tchar('G')) f |= mpt::fmt::NotaNrm | mpt::fmt::CaseUpp;
		if(*p == Tchar('f')) f |= mpt::fmt::NotaFix | mpt::fmt::CaseLow;
		if(*p == Tchar('F')) f |= mpt::fmt::NotaFix | mpt::fmt::CaseUpp;
		if(*p == Tchar('e')) f |= mpt::fmt::NotaSci | mpt::fmt::CaseLow;
		if(*p == Tchar('E')) f |= mpt::fmt::NotaSci | mpt::fmt::CaseUpp;
		++p;
	}
	return Format().SetFlags(f).SetWidth(width).SetPrecision(precision);
}

Format & Format::ParsePrintf(const char * format)
{
	*this = ParseFormatStringFloat(format);
	return *this;
}
Format & Format::ParsePrintf(const wchar_t * format)
{
	*this = ParseFormatStringFloat(format);
	return *this;
}
Format & Format::ParsePrintf(const std::string & format)
{
	*this = ParseFormatStringFloat(format.c_str());
	return *this;
}
Format & Format::ParsePrintf(const std::wstring & format)
{
	*this = ParseFormatStringFloat(format.c_str());
	return *this;
}


std::string FormatVal(const char & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const wchar_t & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const bool & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const signed char & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const unsigned char & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const signed short & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const unsigned short & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const signed int & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const unsigned int & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const signed long & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const unsigned long & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const signed long long & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const unsigned long long & x, const Format & f) { return FormatValHelper(x, f); }

std::string FormatVal(const float & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const double & x, const Format & f) { return FormatValHelper(x, f); }
std::string FormatVal(const long double & x, const Format & f) { return FormatValHelper(x, f); }

std::wstring FormatValW(const char & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const wchar_t & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const bool & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const signed char & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const unsigned char & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const signed short & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const unsigned short & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const signed int & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const unsigned int & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const signed long & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const unsigned long & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const signed long long & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const unsigned long long & x, const Format & f) { return FormatValWHelper(x, f); }

std::wstring FormatValW(const float & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const double & x, const Format & f) { return FormatValWHelper(x, f); }
std::wstring FormatValW(const long double & x, const Format & f) { return FormatValWHelper(x, f); }


#endif // MPT_FMT


namespace String
{


namespace detail
{

template<typename Tstring>
Tstring PrintImplTemplate(const Tstring & format
	, const Tstring & x1
	, const Tstring & x2
	, const Tstring & x3
	, const Tstring & x4
	, const Tstring & x5
	, const Tstring & x6
	, const Tstring & x7
	, const Tstring & x8
	)
{
	Tstring result;
	const std::size_t len = format.length();
	for(std::size_t pos = 0; pos != len; ++pos)
	{
		typename Tstring::value_type c = format[pos];
		if(pos + 1 != len && c == '%')
		{
			pos++;
			c = format[pos];
			if('1' <= c && c <= '9')
			{
				const std::size_t n = c - '0';
				switch(n)
				{
					case 1: result.append(x1); break;
					case 2: result.append(x2); break;
					case 3: result.append(x3); break;
					case 4: result.append(x4); break;
					case 5: result.append(x5); break;
					case 6: result.append(x6); break;
					case 7: result.append(x7); break;
					case 8: result.append(x8); break;
				}
				continue;
			} else if(c != '%')
			{
				result.append(1, '%');
			}
		}
		result.append(1, c);
	}
	return result;
}

std::string PrintImpl(const std::string & format
	, const std::string & x1
	, const std::string & x2
	, const std::string & x3
	, const std::string & x4
	, const std::string & x5
	, const std::string & x6
	, const std::string & x7
	, const std::string & x8
	)
{
	return PrintImplTemplate<std::string>(format, x1,x2,x3,x4,x5,x6,x7,x8);
}

std::wstring PrintImplW(const std::wstring & format
	, const std::wstring & x1
	, const std::wstring & x2
	, const std::wstring & x3
	, const std::wstring & x4
	, const std::wstring & x5
	, const std::wstring & x6
	, const std::wstring & x7
	, const std::wstring & x8
	)
{
	return PrintImplTemplate<std::wstring>(format, x1,x2,x3,x4,x5,x6,x7,x8);
}

} // namespace detail


} // namespace String


} // namespace mpt
