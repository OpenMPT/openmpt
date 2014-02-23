/*
 * mptString.h
 * ----------
 * Purpose: Small string-related utilities, number and message formatting.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <limits>
#include <string>
#if defined(HAS_TYPE_TRAITS)
#include <type_traits>
#endif

#include <cstring>


namespace mpt
{


namespace String
{


#ifdef MODPLUG_TRACKER
// There are 4 reasons why this is not available for library code:
//  1. printf-like functionality is not type-safe.
//  2. There are portability problems with char/wchar_t and the semantics of %s/%ls/%S .
//  3. There are portability problems with specifying format for 64bit integers.
//  4. Formatting of floating point values depends on the currently set C locale.
//     A library is not allowed to mock with that and thus cannot influence the behavior in this case.
std::string MPT_PRINTF_FUNC(1,2) Format(const char * format, ...);
#endif


// Remove whitespace at start of string
static inline std::string LTrim(std::string str, const std::string &whitespace = " \n\r\t")
//-----------------------------------------------------------------------------------------
{
	std::string::size_type pos = str.find_first_not_of(whitespace);
	if(pos != std::string::npos)
	{
		str.erase(str.begin(), str.begin() + pos);
	} else if(pos == std::string::npos && str.length() > 0 && str.find_last_of(whitespace) == str.length() - 1)
	{
		return std::string();
	}
	return str;
}
static inline std::wstring LTrim(std::wstring str, const std::wstring &whitespace = L" \n\r\t")
{
	std::wstring::size_type pos = str.find_first_not_of(whitespace);
	if(pos != std::wstring::npos)
	{
		str.erase(str.begin(), str.begin() + pos);
	} else if(pos == std::wstring::npos && str.length() > 0 && str.find_last_of(whitespace) == str.length() - 1)
	{
		return std::wstring();
	}
	return str;
}


// Remove whitespace at end of string
static inline std::string RTrim(std::string str, const std::string &whitespace = " \n\r\t")
//-----------------------------------------------------------------------------------------
{
	std::string::size_type pos = str.find_last_not_of(whitespace);
	if(pos != std::string::npos)
	{
		str.erase(str.begin() + pos + 1, str.end());
	} else if(pos == std::string::npos && str.length() > 0 && str.find_first_of(whitespace) == 0)
	{
		return std::string();
	}
	return str;
}
static inline std::wstring RTrim(std::wstring str, const std::wstring &whitespace = L" \n\r\t")
{
	std::wstring::size_type pos = str.find_last_not_of(whitespace);
	if(pos != std::wstring::npos)
	{
		str.erase(str.begin() + pos + 1, str.end());
	} else if(pos == std::wstring::npos && str.length() > 0 && str.find_first_of(whitespace) == 0)
	{
		return std::wstring();
	}
	return str;
}


// Remove whitespace at start and end of string
static inline std::string Trim(std::string str, const std::string &whitespace = " \n\r\t")
//----------------------------------------------------------------------------------------
{
	return RTrim(LTrim(str, whitespace), whitespace);
}
static inline std::wstring Trim(std::wstring str, const std::wstring &whitespace = L" \n\r\t")
{
	return RTrim(LTrim(str, whitespace), whitespace);
}


static inline std::string Replace(std::string str, const std::string &oldStr, const std::string &newStr)
//------------------------------------------------------------------------------------------------------
{
	std::size_t pos = 0;
	while((pos = str.find(oldStr, pos)) != std::string::npos)
	{
		str.replace(pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
	return str;
}


static inline std::wstring Replace(std::wstring str, const std::wstring &oldStr, const std::wstring &newStr)
{
	std::size_t pos = 0;
	while((pos = str.find(oldStr, pos)) != std::string::npos)
	{
		str.replace(pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
	return str;
}


} // namespace String


static inline std::size_t strnlen(const char *str, std::size_t n)
//---------------------------------------------------------------
{
#if MPT_COMPILER_MSVC
	return ::strnlen(str, n);
#else
	if(n >= SIZE_MAX)
	{
		return std::strlen(str);
	}
	for(std::size_t i = 0; i < n; ++i)
	{
		if(str[i] == '\0')
		{
			return i;
		}
	}
	return n;
#endif
}


int strnicmp(const char *a, const char *b, size_t count);


enum Charset {
	CharsetLocale, // CP_ACP on windows, current C locale otherwise

	CharsetUTF8,

	CharsetASCII, // strictly 7-bit ASCII

	CharsetISO8859_1,
	CharsetISO8859_15,

	CharsetCP437,
	CharsetCP437AMS,
	CharsetCP437AMS2,

	CharsetWindows1252,
};


// Convert to a wide character string.
// The wide encoding is UTF-16 or UTF-32, based on sizeof(wchar_t).
// If str does not contain any invalid characters, this conversion is lossless.
// Invalid source bytes will be replaced by some replacement character or string.
static inline std::wstring ToWide(const std::wstring &str) { return str; }
std::wstring ToWide(Charset from, const std::string &str);

// Convert to locale-encoded string.
// On Windows, CP_ACP is used,
// otherwise, the global "C" locale is used.
// If str does not contain any invalid characters,
// this conversion will be lossless iff, and only iff, the system is NOT
// windows AND a UTF8 locale is set.
// Invalid source bytes or characters that are not representable in the
// destination charset will be replaced by some replacement character or string.
std::string ToLocale(const std::wstring &str);
std::string ToLocale(Charset from, const std::string &str);

// Convert to a string encoded in the 'to'-specified character set.
// If str does not contain any invalid characters,
// this conversion will be lossless iff, and only iff,
// 'to' is UTF8.
// Invalid source bytes or characters that are not representable in the
// destination charset will be replaced by some replacement character or string.
std::string To(Charset to, const std::wstring &str);
std::string To(Charset to, Charset from, const std::string &str);

#if defined(_MFC_VER)

// Convert to a MFC CString. The CString encoding depends on UNICODE.
// This should also be used when converting to TCHAR strings.
// If UNICODE is defined, this is a completely lossless operation.
static inline CString ToCString(const CString &str) { return str; }
CString ToCString(const std::wstring &str);
CString ToCString(Charset from, const std::string &str);

// Convert from a MFC CString. The CString encoding depends on UNICODE.
// This should also be used when converting from TCHAR strings.
// If UNICODE is defined, this is a completely lossless operation.
std::wstring ToWide(const CString &str);
std::string ToLocale(const CString &str);
std::string To(Charset to, const CString &str);

// Provide un-ambiguous conversion from wide string literal.
static inline std::wstring ToWide(const wchar_t * str) { return ToWide(str ? std::wstring(str) : std::wstring()); }
static inline std::string ToLocale(const wchar_t * str) { return ToLocale(str ? std::wstring(str) : std::wstring()); }
static inline std::string To(Charset to, const wchar_t * str) { return To(to, str ? std::wstring(str) : std::wstring()); }
static inline CString ToCString(const wchar_t * str) { return ToCString(str ? std::wstring(str) : std::wstring()); }

#endif


} // namespace mpt





// The following section demands a rationale.
//  1. ToString() and ToWString() mimic the semantics of c++11 std::to_string() and std::to_wstring().
//     There is an important difference though. The c++11 versions are specified in terms of sprintf formatting which in turn
//     depend on the current C locale. This renders these functions unusable in a library context because the current
//     C locale is set by the library-using application and could be anything. There is no way a library can get reliable semantics
//     out of these functions. It is thus better to just avoid them.
//     ToString() and ToWString() are based on iostream internally, but the the locale of the stream is forced to std::locale::classic(),
//     which results in "C" ASCII locale behavior.
//  2. The full suite of printf-like or iostream like number formatting is generally not required. Instead, a sane subset functionality
//     is provided here.
//     For convenience, mpt::Format().ParsePrintf(const char *).ToString(float) allows formatting a single floating point value with a
//     standard printf-like format string. This itself relies on iostream with classic() locale internally and is thus current locale
//     agnostic.
//     When formatting integers, it is recommended to use mpt::fmt::dec or mpt::fmt::hex. Appending a template argument '<n>' sets the width,
//     the same way as '%nd' would do. Appending a '0' to the function name causes zero-filling as print-like '%0nd' would do. Spelling 'HEX'
//     in upper-case generates upper-case hex digits. If these are not known at compile-time, a more verbose FormatVal(int, format) can be
//     used.
//  3. mpt::String::Print(format, ...) provides simplified and type-safe message and localization string formatting.
//     The only specifier allowed is '%' followed by a single digit n. It references to n-th parameter after the format string (1-based).
//     This mimics the behaviour of QString::arg() in QT4/5 or MFC AfxFormatString2(). C printf-like functions offer similar functionality
//     with a '%n$TYPE' syntax. In .NET, the syntax is '{n}'. This is useful to support  localization strings that can change the parameter
//     ordering.
//  4. Every function is available for std::string and std::wstring. std::string makes no assumption about the encoding, which basically means,
//     it should work for any 7-bit or 8-bit encoding, including for example ASCII, UTF8 or the current locale encoding.
//     std::string        std::wstring
//     mpt::ToString      mpt::ToWString
//     mpt::FormatVal     mpt::FormatValW
//     mpt::fmt           mpt::wfmt
//     mpt::String::Print mpt::String::PrintW
//  5. All functionality here delegates real work outside of the header file so that <sstream> and <locale> do not need to be included when
//     using this functionality.
//     Advantages:
//      - Avoids binary code bloat when too much of iostream operator << gets inlined at every usage site.
//      - Faster compile times because <sstream> and <locale> (2 very complex headers) are not included everywhere.
//     Disadvantages:
//      - Slightly more c++ code is required for delegating work.
//      - As the header does not use iostreams, custom types need to overload mpt::ToString and mpt::ToWstring instead of iostream
//        operator << to allow for custom type formatting.
//      - std::string and std::wstring are returned from somewhat deep cascades of helper functions. Where possible, code is written in such
//        a  way that return-value-optimization (RVO) or named-return-value-optimization (NRVO) should be able to eliminate almost all these
//        copies. This should not be a problem for any decent modern compiler (and even less so for a c++11 compiler where move-semantics
//        will kick in if RVO/NRVO fails).

namespace mpt
{

// ToString() converts various built-in types to a well-defined, locale-independent string representation.
// This is also used as a type-tunnel pattern for mpt::String::Print.
// Custom types that need to be converted to strings are encouraged to overload ToString() and ToWString().

static inline std::string ToString(const std::string & x) { return x; }
static inline std::string ToString(const char * const & x) { return x; }
MPT_DEPRECATED std::string ToString(const std::wstring & x);
MPT_DEPRECATED std::string ToString(const wchar_t * const & x);
MPT_DEPRECATED std::string ToString(const char & x); // deprecated to catch potential API mis-use, use std::string(1, x) instead
MPT_DEPRECATED std::string ToString(const wchar_t & x); // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
std::string ToString(const bool & x);
std::string ToString(const signed char & x);
std::string ToString(const unsigned char & x);
std::string ToString(const signed short & x);
std::string ToString(const unsigned short & x);
std::string ToString(const signed int & x);
std::string ToString(const unsigned int & x);
std::string ToString(const signed long & x);
std::string ToString(const unsigned long & x);
std::string ToString(const signed long long & x);
std::string ToString(const unsigned long long & x);
std::string ToString(const float & x);
std::string ToString(const double & x);
std::string ToString(const long double & x);

MPT_DEPRECATED std::wstring ToWString(const std::string & x);
MPT_DEPRECATED std::wstring ToWString(const char * const & x);
static inline std::wstring ToWString(const std::wstring & x) { return x; }
static inline std::wstring ToWString(const wchar_t * const & x) { return x; }
MPT_DEPRECATED std::wstring ToWString(const char & x); // deprecated to catch potential API mis-use, use std::string(1, x) instead
MPT_DEPRECATED std::wstring ToWString(const wchar_t & x); // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
std::wstring ToWString(const bool & x);
std::wstring ToWString(const signed char & x);
std::wstring ToWString(const unsigned char & x);
std::wstring ToWString(const signed short & x);
std::wstring ToWString(const unsigned short & x);
std::wstring ToWString(const signed int & x);
std::wstring ToWString(const unsigned int & x);
std::wstring ToWString(const signed long & x);
std::wstring ToWString(const unsigned long & x);
std::wstring ToWString(const signed long long & x);
std::wstring ToWString(const unsigned long long & x);
std::wstring ToWString(const float & x);
std::wstring ToWString(const double & x);
std::wstring ToWString(const long double & x);

#define MPT_FMT

#if defined(MPT_FMT)

namespace fmt
{

enum FormatFlagsEnum
{
	BaseDec = 0x0001, // base 10 (integers only)
	BaseHex = 0x0002, // base 16 (integers only)
	CaseLow = 0x0010, // lower case hex digits
	CaseUpp = 0x0020, // upper case hex digits
	FillOff = 0x0100, // do not fill up width
	FillSpc = 0x0200, // fill up width with spaces
	FillNul = 0x0400, // fill up width with zeros
	NotaNrm = 0x1000, // float: normal/default notation
	NotaFix = 0x2000, // float: fixed point notation
	NotaSci = 0x4000, // float: scientific notation
};

} // namespace fmt

typedef unsigned int FormatFlags;

STATIC_ASSERT(sizeof(FormatFlags) >= sizeof(fmt::FormatFlagsEnum));

class Format;

MPT_DEPRECATED std::string FormatVal(const char & x, const Format & f); // deprecated to catch potential API mis-use, use std::string(1, x) instead
MPT_DEPRECATED std::string FormatVal(const wchar_t & x, const Format & f); // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
std::string FormatVal(const bool & x, const Format & f);
std::string FormatVal(const signed char & x, const Format & f);
std::string FormatVal(const unsigned char & x, const Format & f);
std::string FormatVal(const signed short & x, const Format & f);
std::string FormatVal(const unsigned short & x, const Format & f);
std::string FormatVal(const signed int & x, const Format & f);
std::string FormatVal(const unsigned int & x, const Format & f);
std::string FormatVal(const signed long & x, const Format & f);
std::string FormatVal(const unsigned long & x, const Format & f);
std::string FormatVal(const signed long long & x, const Format & f);
std::string FormatVal(const unsigned long long & x, const Format & f);

std::string FormatVal(const float & x, const Format & f);
std::string FormatVal(const double & x, const Format & f);
std::string FormatVal(const long double & x, const Format & f);

MPT_DEPRECATED std::wstring FormatValW(const char & x, const Format & f); // deprecated to catch potential API mis-use, use std::string(1, x) instead
MPT_DEPRECATED std::wstring FormatValW(const wchar_t & x, const Format & f); // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
std::wstring FormatValW(const bool & x, const Format & f);
std::wstring FormatValW(const signed char & x, const Format & f);
std::wstring FormatValW(const unsigned char & x, const Format & f);
std::wstring FormatValW(const signed short & x, const Format & f);
std::wstring FormatValW(const unsigned short & x, const Format & f);
std::wstring FormatValW(const signed int & x, const Format & f);
std::wstring FormatValW(const unsigned int & x, const Format & f);
std::wstring FormatValW(const signed long & x, const Format & f);
std::wstring FormatValW(const unsigned long & x, const Format & f);
std::wstring FormatValW(const signed long long & x, const Format & f);
std::wstring FormatValW(const unsigned long long & x, const Format & f);

std::wstring FormatValW(const float & x, const Format & f);
std::wstring FormatValW(const double & x, const Format & f);
std::wstring FormatValW(const long double & x, const Format & f);

class Format
{
private:
	FormatFlags flags;
	std::size_t width;
	int precision;
public:
	Format() : flags(0), width(0), precision(-1) {}
	FormatFlags GetFlags() const { return flags; }
	std::size_t GetWidth() const { return width; }
	int GetPrecision() const { return precision; }
	Format & SetFlags(FormatFlags f) { flags = f; return *this; }
	Format & SetWidth(std::size_t w) { width = w; return *this; }
	Format & SetPrecision(int p) { precision = p; return *this; }
public:
	// short-hand construction
	explicit Format(FormatFlags f, std::size_t w = 0, int p = -1) : flags(f), width(w), precision(p) {}
	explicit Format(const char * format) : flags(0), width(0), precision(-1) { ParsePrintf(format); }
	explicit Format(const wchar_t * format) : flags(0), width(0), precision(-1) { ParsePrintf(format); }
	explicit Format(const std::string & format) : flags(0), width(0), precision(-1) { ParsePrintf(format); }
	explicit Format(const std::wstring & format) : flags(0), width(0), precision(-1) { ParsePrintf(format); }
public:
	// only for floating point formats
	Format & ParsePrintf(const char * format);
	Format & ParsePrintf(const wchar_t * format);
	Format & ParsePrintf(const std::string & format);
	Format & ParsePrintf(const std::wstring & format);
public:
	Format & BaseDec() { flags &= ~(fmt::BaseDec|fmt::BaseHex); flags |= fmt::BaseDec; return *this; }
	Format & BaseHex() { flags &= ~(fmt::BaseDec|fmt::BaseHex); flags |= fmt::BaseHex; return *this; }
	Format & CaseLow() { flags &= ~(fmt::CaseLow|fmt::CaseUpp); flags |= fmt::CaseLow; return *this; }
	Format & CaseUpp() { flags &= ~(fmt::CaseLow|fmt::CaseUpp); flags |= fmt::CaseUpp; return *this; }
	Format & FillOff() { flags &= ~(fmt::FillOff|fmt::FillSpc|fmt::FillNul); flags |= fmt::FillOff; return *this; }
	Format & FillSpc() { flags &= ~(fmt::FillOff|fmt::FillSpc|fmt::FillNul); flags |= fmt::FillSpc; return *this; }
	Format & FillNul() { flags &= ~(fmt::FillOff|fmt::FillSpc|fmt::FillNul); flags |= fmt::FillNul; return *this; }
	Format & NotaNrm() { flags &= ~(fmt::NotaNrm|fmt::NotaFix|fmt::NotaSci); flags |= fmt::NotaNrm; return *this; }
	Format & NotaFix() { flags &= ~(fmt::NotaNrm|fmt::NotaFix|fmt::NotaSci); flags |= fmt::NotaFix; return *this; }
	Format & NotaSci() { flags &= ~(fmt::NotaNrm|fmt::NotaFix|fmt::NotaSci); flags |= fmt::NotaSci; return *this; }
	Format & Width(std::size_t w) { width = w; return *this; }
	Format & Prec(int p) { precision = p; return *this; }
public:
	Format & Dec() { return BaseDec(); }
	Format & Hex() { return BaseHex(); }
	Format & Low() { return CaseLow(); }
	Format & Upp() { return CaseUpp(); }
	Format & Off() { return FillOff(); }
	Format & Spc() { return FillSpc(); }
	Format & Nul() { return FillNul(); }
	Format & Nrm() { return NotaNrm(); }
	Format & Fix() { return NotaFix(); }
	Format & Sci() { return NotaSci(); }
public:
	Format & Decimal() { return BaseDec(); }
	Format & Hexadecimal() { return BaseHex(); }
	Format & Lower() { return CaseLow(); }
	Format & Upper() { return CaseUpp(); }
	Format & FillNone() { return FillOff(); }
	Format & FillSpace() { return FillSpc(); }
	Format & FillZero() { return FillNul(); }
	Format & FloatNormal() { return NotaNrm(); }
	Format & FloatFixed() { return NotaFix(); }
	Format & FloatScientific() { return NotaSci(); }
	Format & Precision(int p) { return Prec(p); }
	template<typename T>
	inline std::string ToString(const T & x) const
	{
		return FormatVal(x, *this);
	}
	template<typename T>
	inline std::wstring ToWString(const T & x) const
	{
		return FormatValW(x, *this);
	}
};

namespace fmt
{

template<typename T>
inline std::string dec(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatVal(x, Format().BaseDec().FillOff());
}
template<int width, typename T>
inline std::string dec(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatVal(x, Format().BaseDec().FillSpc().Width(width));
}
template<int width, typename T>
inline std::string dec0(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatVal(x, Format().BaseDec().FillNul().Width(width));
}

template<typename T>
inline std::string hex(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatVal(x, Format().BaseHex().CaseLow().FillOff());
}
template<typename T>
inline std::string HEX(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatVal(x, Format().BaseHex().CaseUpp().FillOff());
}
template<int width, typename T>
inline std::string hex(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatVal(x, Format().BaseHex().CaseLow().FillSpc().Width(width));
}
template<int width, typename T>
inline std::string HEX(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatVal(x, Format().BaseHex().CaseUpp().FillSpc().Width(width));
}
template<int width, typename T>
inline std::string hex0(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatVal(x, Format().BaseHex().CaseLow().FillNul().Width(width));
}
template<int width, typename T>
inline std::string HEX0(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatVal(x, Format().BaseHex().CaseUpp().FillNul().Width(width));
}

template<typename T>
inline std::string flt(const T& x, std::size_t width = 0, int precision = -1)
{
	#if defined(HAS_TYPE_TRAITS)
		STATIC_ASSERT(std::is_floating_point<T>::value);
	#endif
	if(width == 0)
	{
		return FormatVal(x, Format().NotaNrm().FillOff().Precision(precision));
	} else
	{
		return FormatVal(x, Format().NotaNrm().FillSpc().Width(width).Precision(precision));
	}
}
template<typename T>
inline std::string fix(const T& x, std::size_t width = 0, int precision = -1)
{
	#if defined(HAS_TYPE_TRAITS)
		STATIC_ASSERT(std::is_floating_point<T>::value);
	#endif
	if(width == 0)
	{
		return FormatVal(x, Format().NotaFix().FillOff().Precision(precision));
	} else
	{
		return FormatVal(x, Format().NotaFix().FillSpc().Width(width).Precision(precision));
	}
}
template<typename T>
inline std::string sci(const T& x, std::size_t width = 0, int precision = -1)
{
	#if defined(HAS_TYPE_TRAITS)
		STATIC_ASSERT(std::is_floating_point<T>::value);
	#endif
	if(width == 0)
	{
		return FormatVal(x, Format().NotaSci().FillOff().Precision(precision));
	} else
	{
		return FormatVal(x, Format().NotaSci().FillSpc().Width(width).Precision(precision));
	}
}

} // namespace fmt

namespace wfmt
{

template<typename T>
inline std::wstring dec(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValW(x, Format().BaseDec().FillOff());
}
template<int width, typename T>
inline std::wstring dec(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValW(x, Format().BaseDec().FillSpc().Width(width));
}
template<int width, typename T>
inline std::wstring dec0(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValW(x, Format().BaseDec().FillNul().Width(width));
}

template<typename T>
inline std::wstring hex(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValW(x, Format().BaseHex().CaseLow().FillOff());
}
template<typename T>
inline std::wstring HEX(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValW(x, Format().BaseHex().CaseUpp().FillOff());
}
template<int width, typename T>
inline std::wstring hex(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValW(x, Format().BaseHex().CaseLow().FillSpc().Width(width));
}
template<int width, typename T>
inline std::wstring HEX(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValW(x, Format().BaseHex().CaseUpp().FillSpc().Width(width));
}
template<int width, typename T>
inline std::wstring hex0(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValW(x, Format().BaseHex().CaseLow().FillNul().Width(width));
}
template<int width, typename T>
inline std::wstring HEX0(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValW(x, Format().BaseHex().CaseUpp().FillNul().Width(width));
}

template<typename T>
inline std::wstring flt(const T& x, std::size_t width = 0, int precision = -1)
{
	#if defined(HAS_TYPE_TRAITS)
		STATIC_ASSERT(std::is_floating_point<T>::value);
	#endif
	if(width == 0)
	{
		return FormatValW(x, Format().NotaNrm().FillOff().Precision(precision));
	} else
	{
		return FormatValW(x, Format().NotaNrm().FillSpc().Width(width).Precision(precision));
	}
}
template<typename T>
inline std::wstring fix(const T& x, std::size_t width = 0, int precision = -1)
{
	#if defined(HAS_TYPE_TRAITS)
		STATIC_ASSERT(std::is_floating_point<T>::value);
	#endif
	if(width == 0)
	{
		return FormatValW(x, Format().NotaFix().FillOff().Precision(precision));
	} else
	{
		return FormatValW(x, Format().NotaFix().FillSpc().Width(width).Precision(precision));
	}
}
template<typename T>
inline std::wstring sci(const T& x, std::size_t width = 0, int precision = -1)
{
	#if defined(HAS_TYPE_TRAITS)
		STATIC_ASSERT(std::is_floating_point<T>::value);
	#endif
	if(width == 0)
	{
		return FormatValW(x, Format().NotaSci().FillOff().Precision(precision));
	} else
	{
		return FormatValW(x, Format().NotaSci().FillSpc().Width(width).Precision(precision));
	}
}

} // namespace wfmt

#endif // MPT_FMT

} // namespace mpt

#define Stringify(x) mpt::ToString(x)
#define StringifyW(x) mpt::ToWString(x)

namespace mpt { namespace String {

namespace detail
{

std::string PrintImpl(const std::string & format
	, const std::string & x1 = std::string()
	, const std::string & x2 = std::string()
	, const std::string & x3 = std::string()
	, const std::string & x4 = std::string()
	, const std::string & x5 = std::string()
	, const std::string & x6 = std::string()
	, const std::string & x7 = std::string()
	, const std::string & x8 = std::string()
	);

std::wstring PrintImplW(const std::wstring & format
	, const std::wstring & x1 = std::wstring()
	, const std::wstring & x2 = std::wstring()
	, const std::wstring & x3 = std::wstring()
	, const std::wstring & x4 = std::wstring()
	, const std::wstring & x5 = std::wstring()
	, const std::wstring & x6 = std::wstring()
	, const std::wstring & x7 = std::wstring()
	, const std::wstring & x8 = std::wstring()
	);

} // namespace detail

template<
	typename T1
>
std::string Print(const std::string & format
	, const T1& x1
)
{
	return detail::PrintImpl(format
		, ToString(x1)
	);
}

template<
	typename T1,
	typename T2
>
std::string Print(const std::string & format
	, const T1& x1
	, const T2& x2
)
{
	return detail::PrintImpl(format
		, ToString(x1)
		, ToString(x2)
	);
}

template<
	typename T1,
	typename T2,
	typename T3
>
std::string Print(const std::string & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
)
{
	return detail::PrintImpl(format
		, ToString(x1)
		, ToString(x2)
		, ToString(x3)
	);
}

template<
	typename T1,
	typename T2,
	typename T3,
	typename T4
>
std::string Print(const std::string & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
)
{
	return detail::PrintImpl(format
		, ToString(x1)
		, ToString(x2)
		, ToString(x3)
		, ToString(x4)
	);
}

template<
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5
>
std::string Print(const std::string & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
)
{
	return detail::PrintImpl(format
		, ToString(x1)
		, ToString(x2)
		, ToString(x3)
		, ToString(x4)
		, ToString(x5)
	);
}

template<
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6
>
std::string Print(const std::string & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
	, const T6& x6
)
{
	return detail::PrintImpl(format
		, ToString(x1)
		, ToString(x2)
		, ToString(x3)
		, ToString(x4)
		, ToString(x5)
		, ToString(x6)
	);
}

template<
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7
>
std::string Print(const std::string & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
	, const T6& x6
	, const T7& x7
)
{
	return detail::PrintImpl(format
		, ToString(x1)
		, ToString(x2)
		, ToString(x3)
		, ToString(x4)
		, ToString(x5)
		, ToString(x6)
		, ToString(x7)
	);
}

template<
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8
>
std::string Print(const std::string & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
	, const T6& x6
	, const T7& x7
	, const T8& x8
)
{
	return detail::PrintImpl(format
		, ToString(x1)
		, ToString(x2)
		, ToString(x3)
		, ToString(x4)
		, ToString(x5)
		, ToString(x6)
		, ToString(x7)
		, ToString(x8)
	);
}

template<
	typename T1
>
std::wstring PrintW(const std::wstring & format
	, const T1& x1
)
{
	return detail::PrintImplW(format
		, ToWString(x1)
	);
}

template<
	typename T1,
	typename T2
>
std::wstring PrintW(const std::wstring & format
	, const T1& x1
	, const T2& x2
)
{
	return detail::PrintImplW(format
		, ToWString(x1)
		, ToWString(x2)
	);
}

template<
	typename T1,
	typename T2,
	typename T3
>
std::wstring PrintW(const std::wstring & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
)
{
	return detail::PrintImplW(format
		, ToWString(x1)
		, ToWString(x2)
		, ToWString(x3)
	);
}

template<
	typename T1,
	typename T2,
	typename T3,
	typename T4
>
std::wstring PrintW(const std::wstring & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
)
{
	return detail::PrintImplW(format
		, ToWString(x1)
		, ToWString(x2)
		, ToWString(x3)
		, ToWString(x4)
	);
}

template<
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5
>
std::wstring PrintW(const std::wstring & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
)
{
	return detail::PrintImplW(format
		, ToWString(x1)
		, ToWString(x2)
		, ToWString(x3)
		, ToWString(x4)
		, ToWString(x5)
	);
}

template<
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6
>
std::wstring PrintW(const std::wstring & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
	, const T6& x6
)
{
	return detail::PrintImplW(format
		, ToWString(x1)
		, ToWString(x2)
		, ToWString(x3)
		, ToWString(x4)
		, ToWString(x5)
		, ToWString(x6)
	);
}

template<
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7
>
std::wstring PrintW(const std::wstring & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
	, const T6& x6
	, const T7& x7
)
{
	return detail::PrintImplW(format
		, ToWString(x1)
		, ToWString(x2)
		, ToWString(x3)
		, ToWString(x4)
		, ToWString(x5)
		, ToWString(x6)
		, ToWString(x7)
	);
}

template<
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8
>
std::wstring PrintW(const std::wstring & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
	, const T6& x6
	, const T7& x7
	, const T8& x8
)
{
	return detail::PrintImplW(format
		, ToWString(x1)
		, ToWString(x2)
		, ToWString(x3)
		, ToWString(x4)
		, ToWString(x5)
		, ToWString(x6)
		, ToWString(x7)
		, ToWString(x8)
	);
}

} } // namespace mpt::String
