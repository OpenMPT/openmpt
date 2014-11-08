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


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{


namespace String
{


template <typename Tstring> struct Traits {
	static const char * GetDefaultWhitespace() { return " \n\r\t"; }
};

template <> struct Traits<std::string> {
	static const char * GetDefaultWhitespace() { return " \n\r\t"; }
};

template <> struct Traits<std::wstring> {
	static const wchar_t * GetDefaultWhitespace() { return L" \n\r\t"; }
};


// Remove whitespace at start of string
template <typename Tstring>
inline Tstring LTrim(Tstring str, const Tstring &whitespace = Tstring(mpt::String::Traits<Tstring>::GetDefaultWhitespace()))
//--------------------------------------------------------------------------------------------------------------------------
{
	typename Tstring::size_type pos = str.find_first_not_of(whitespace);
	if(pos != Tstring::npos)
	{
		str.erase(str.begin(), str.begin() + pos);
	} else if(pos == Tstring::npos && str.length() > 0 && str.find_last_of(whitespace) == str.length() - 1)
	{
		return Tstring();
	}
	return str;
}


// Remove whitespace at end of string
template <typename Tstring>
inline Tstring RTrim(Tstring str, const Tstring &whitespace = Tstring(mpt::String::Traits<Tstring>::GetDefaultWhitespace()))
//--------------------------------------------------------------------------------------------------------------------------
{
	typename Tstring::size_type pos = str.find_last_not_of(whitespace);
	if(pos != Tstring::npos)
	{
		str.erase(str.begin() + pos + 1, str.end());
	} else if(pos == Tstring::npos && str.length() > 0 && str.find_first_of(whitespace) == 0)
	{
		return Tstring();
	}
	return str;
}


// Remove whitespace at start and end of string
template <typename Tstring>
inline Tstring Trim(Tstring str, const Tstring &whitespace = Tstring(mpt::String::Traits<Tstring>::GetDefaultWhitespace()))
//-------------------------------------------------------------------------------------------------------------------------
{
	return RTrim(LTrim(str, whitespace), whitespace);
}


template <typename Tstring, typename Tstring2, typename Tstring3>
inline Tstring Replace(Tstring str, const Tstring2 &oldStr_, const Tstring3 &newStr_)
//-----------------------------------------------------------------------------------
{
	std::size_t pos = 0;
	const Tstring oldStr = oldStr_;
	const Tstring newStr = newStr_;
	while((pos = str.find(oldStr, pos)) != Tstring::npos)
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
	if(n >= std::numeric_limits<std::size_t>::max())
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
#if defined(MPT_WITH_CHARSET_LOCALE)
	CharsetLocale, // CP_ACP on windows, current C locale otherwise
#endif

	CharsetUTF8,

	CharsetASCII, // strictly 7-bit ASCII

	CharsetISO8859_1,
	CharsetISO8859_15,

	CharsetCP437,
	CharsetCP437AMS,
	CharsetCP437AMS2,

	CharsetWindows1252,
};


// Checks if the std::string represents an UTF8 string.
// This is currently implemented as converting to std::wstring and back assuming UTF8 both ways,
// and comparing the result to the original string.
// Caveats:
//  - can give false negatives because of possible unicode normalization during conversion
//  - can give false positives if the 8bit encoding contains high-ascii only in valid utf8 groups
//  - slow because of double conversion
bool IsUTF8(const std::string &str);


#define MPT_CHAR_TYPE    char
#define MPT_CHAR(x)      x
#define MPT_LITERAL(x)   x
#define MPT_STRING(x)    std::string( x )

#define MPT_WCHAR_TYPE   wchar_t
#define MPT_WCHAR(x)     L ## x
#define MPT_WLITERAL(x)  L ## x
#define MPT_WSTRING(x)   std::wstring( L ## x )


#if MPT_WITH_U8STRING

template <mpt::Charset charset_tag>
struct charset_char_traits : std::char_traits<char> {
	static mpt::Charset charset() { return charset_tag; }
};
#define MPT_ENCODED_STRING_TYPE(charset) std::basic_string< char, mpt::charset_char_traits< charset > >

typedef MPT_ENCODED_STRING_TYPE(mpt::CharsetUTF8) u8string;

#define MPT_U8CHAR_TYPE  char
#define MPT_U8CHAR(x)    x
#define MPT_U8LITERAL(x) x
#define MPT_U8STRING(x)  mpt::u8string( x )

// mpt::u8string is a moderately type-safe string that is meant to contain
// UTF-8 encoded char bytes.
//
// mpt::u8string is not implicitely convertible to/from std::string, but
// it is convertible to/from C strings the same way as std::string is.
//
// The implementation of mpt::u8string is a compromise of compatibilty
// with implementation-defined STL details, efficiency, source code size,
// executable bloat, type-safety  and simplicity.
//
// mpt::u8string is not meant to be used directly though.
// mpt::u8string is meant as an alternative implementaion to std::wstring
// for implementing the unicode string type mpt::ustring.

#endif // MPT_WITH_U8STRING


#if MPT_WSTRING_CONVERT
// Convert to a wide character string.
// The wide encoding is UTF-16 or UTF-32, based on sizeof(wchar_t).
// If str does not contain any invalid characters, this conversion is lossless.
// Invalid source bytes will be replaced by some replacement character or string.
static inline std::wstring ToWide(const std::wstring &str) { return str; }
static inline std::wstring ToWide(const wchar_t * str) { return (str ? std::wstring(str) : std::wstring()); }
std::wstring ToWide(Charset from, const std::string &str);
#endif

// Convert to a string encoded in the 'to'-specified character set.
// If str does not contain any invalid characters,
// this conversion will be lossless iff, and only iff,
// 'to' is UTF8.
// Invalid source bytes or characters that are not representable in the
// destination charset will be replaced by some replacement character or string.
#if MPT_WSTRING_CONVERT
std::string ToCharset(Charset to, const std::wstring &str);
static inline std::string ToCharset(Charset to, const wchar_t * str) { return ToCharset(to, str ? std::wstring(str) : std::wstring()); }
#endif
std::string ToCharset(Charset to, Charset from, const std::string &str);


#if defined(_MFC_VER)
#if !(MPT_WSTRING_CONVERT)
#error "MFC depends on MPT_WSTRING_CONVERT)"
#endif

// Convert to a MFC CString. The CString encoding depends on UNICODE.
// This should also be used when converting to TCHAR strings.
// If UNICODE is defined, this is a completely lossless operation.
static inline CString ToCString(const CString &str) { return str; }
CString ToCString(const std::wstring &str);
static inline CString ToCString(const wchar_t * str) { return ToCString(str ? std::wstring(str) : std::wstring()); }
CString ToCString(Charset from, const std::string &str);

// Convert from a MFC CString. The CString encoding depends on UNICODE.
// This should also be used when converting from TCHAR strings.
// If UNICODE is defined, this is a completely lossless operation.
std::wstring ToWide(const CString &str);
std::string ToCharset(Charset to, const CString &str);

#ifdef UNICODE
MPT_DEPRECATED static inline CString ToCStringW(const CString &str) { return ToCString(str); }
MPT_DEPRECATED static inline CString ToCStringW(const std::wstring &str) { return ToCString(str); }
MPT_DEPRECATED static inline CString ToCStringW(Charset from, const std::string &str) { return ToCString(from, str); }
#else // !UNICODE
CStringW ToCStringW(const CString &str);
CStringW ToCStringW(const std::wstring &str);
CStringW ToCStringW(Charset from, const std::string &str);
CStringW ToCStringW(const CStringW &str);
std::wstring ToWide(const CStringW &str);
std::string ToCharset(Charset to, const CStringW &str);
CString ToCString(const CStringW &str);
#endif // UNICODE

#endif // MFC


#if defined(MPT_WITH_CHARSET_LOCALE)
// Convert to locale-encoded string.
// On Windows, CP_ACP is used,
// otherwise, the global C locale is used.
// If str does not contain any invalid characters,
// this conversion will be lossless iff, and only iff, the system is NOT
// windows AND a UTF8 locale is set.
// Invalid source bytes or characters that are not representable in the
// destination charset will be replaced by some replacement character or string.
template <typename Tsrc> inline std::string ToLocale(const Tsrc &str) { return ToCharset(CharsetLocale, str); }
static inline std::string ToLocale(Charset from, const std::string &str) { return ToCharset(CharsetLocale, from, str); }
#endif


// mpt::ustring
//
// mpt::ustring is a string type that can hold unicode strings.
// It is implemented as a std::basic_string either based on wchar_t (i.e. the
//  same as std::wstring) or a custom-defined char_traits class that is derived
//  from std::char_traits<char>.
// The selection of the underlying implementation is done at compile-time.
// MPT_UCHAR, MPT_ULITERAL and MPT_USTRING are macros that ease construction
//  of ustring char literals, ustring char array literals and ustring objects
//  from ustring char literals that work consistently in both modes.
//  Note that these are not supported for non-ASCII characters appearing in
//  the macro argument.
// Also note that, as both UTF8 and UTF16 (it is less of an issue for UTF32)
//  are variable-length encodings and mpt::ustring is implemented as a
//  std::basic_string, all member functions that require individual character
//  access will not work consistently or even at all in a meaningful way.
//  This in particular affects operator[], at(), find() and substr().
//  The code makes no effort in preventing these or generating warnings when
//  these are used on mpt::ustring objects. However, compiling in the
//  respectively other mpt::ustring mode will catch most of these anyway.

#if MPT_USTRING_MODE_WIDE
#if MPT_USTRING_MODE_UTF8
#error "MPT_USTRING_MODE_WIDE and MPT_USTRING_MODE_UTF8 are mutually exclusive."
#endif

typedef std::wstring     ustring;
#define MPT_UCHAR_TYPE   wchar_t
#define MPT_UCHAR(x)     L ## x
#define MPT_ULITERAL(x)  L ## x
#define MPT_USTRING(x)   std::wstring( L ## x )

#endif // MPT_USTRING_MODE_WIDE

#if MPT_USTRING_MODE_UTF8
#if MPT_USTRING_MODE_WIDE
#error "MPT_USTRING_MODE_WIDE and MPT_USTRING_MODE_UTF8 are mutually exclusive."
#endif

typedef mpt::u8string    ustring;
#define MPT_UCHAR_TYPE   char
#define MPT_UCHAR(x)     x
#define MPT_ULITERAL(x)  x
#define MPT_USTRING(x)   mpt::ustring( x )

#endif // MPT_USTRING_MODE_UTF8

#if MPT_USTRING_MODE_WIDE
#if !(MPT_WSTRING_CONVERT)
#error "MPT_USTRING_MODE_WIDE depends on MPT_WSTRING_CONVERT)"
#endif
static inline mpt::ustring ToUnicode(const std::wstring &str) { return str; }
static inline mpt::ustring ToUnicode(const wchar_t * str) { return (str ? std::wstring(str) : std::wstring()); }
static inline mpt::ustring ToUnicode(Charset from, const std::string &str) { return ToWide(from, str); }
#if defined(_MFC_VER)
static inline mpt::ustring ToUnicode(const CString &str) { return ToWide(str); }
#ifndef UNICODE
static inline mpt::ustring ToUnicode(const CStringW &str) { return ToWide(str); }
#endif // !UNICODE
#endif // MFC
#else // !MPT_USTRING_MODE_WIDE
static inline mpt::ustring ToUnicode(const mpt::ustring &str) { return str; }
#if MPT_WSTRING_CONVERT
mpt::ustring ToUnicode(const std::wstring &str);
static inline mpt::ustring ToUnicode(const wchar_t * str) { return ToUnicode(str ? std::wstring(str) : std::wstring()); }
#endif
mpt::ustring ToUnicode(Charset from, const std::string &str);
#if defined(_MFC_VER)
mpt::ustring ToUnicode(const CString &str);
#ifndef UNICODE
mpt::ustring ToUnicode(const CStringW &str);
#endif // !UNICODE
#endif // MFC
#endif // MPT_USTRING_MODE_WIDE

#if MPT_USTRING_MODE_WIDE
#if !(MPT_WSTRING_CONVERT)
#error "MPT_USTRING_MODE_WIDE depends on MPT_WSTRING_CONVERT)"
#endif
// nothing, std::wstring overloads will catch all stuff
#else // !MPT_USTRING_MODE_WIDE
#if MPT_WSTRING_CONVERT
std::wstring ToWide(const mpt::ustring &str);
#endif
std::string ToCharset(Charset to, const mpt::ustring &str);
#if defined(_MFC_VER)
CString ToCString(const mpt::ustring &str);
#ifdef UNICODE
MPT_DEPRECATED static inline CString ToCStringW(const mpt::ustring &str) { return ToCString(str); }
#else // !UNICODE
static inline CStringW ToCStringW(const mpt::ustring &str) { return ToCStringW(ToWide(str)); }
#endif // UNICODE
#endif // MFC
#endif // MPT_USTRING_MODE_WIDE

// The MPT_UTF8 allows specifying UTF8 char arrays.
// The resulting type is mpt::ustring and the construction might require runtime translation,
// i.e. it is NOT generally available at compile time.
// Use explicit UTF8 encoding,
// i.e. U+00FC (LATIN SMALL LETTER U WITH DIAERESIS) would be written as "\xC3\xBC".
#define MPT_UTF8(x) mpt::ToUnicode(mpt::CharsetUTF8, x )

} // namespace mpt





// The AnyString types are meant to be used as function argument types only,
// and only during the transition phase to all-unicode strings in the whole codebase.
// Using an AnyString type as function argument avoids the need to overload a function for all the
// different string types that we currently have.
// Warning: These types will silently do charset conversions. Only use them when this can be tolerated.

// BasicAnyString is convertable to mpt::ustring and constructable from any string at all.
template <mpt::Charset charset = mpt::CharsetUTF8, bool tryUTF8 = true>
class BasicAnyString : public mpt::ustring
{

private:
	
	static mpt::ustring From8bit(const std::string &str)
	{
		if(charset == mpt::CharsetUTF8)
		{
			return mpt::ToUnicode(mpt::CharsetUTF8, str);
		}
		// auto utf8 detection
		if(tryUTF8 && mpt::IsUTF8(str))
		{
			return mpt::ToUnicode(mpt::CharsetUTF8, str);
		} else
		{
			return mpt::ToUnicode(charset, str);
		}
	}

public:

	// 8 bit
	BasicAnyString(const char *str) : mpt::ustring(str ? mpt::ToUnicode(charset, str) : mpt::ustring()) { }
	BasicAnyString(const std::string str) : mpt::ustring(mpt::ToUnicode(charset, str)) { }

	// unicode
	BasicAnyString(const mpt::ustring &str) : mpt::ustring(str) { }
#if MPT_COMPILER_HAS_RVALUE_REF
	BasicAnyString(mpt::ustring &&str) : mpt::ustring(std::move(str)) { }
#endif
#if MPT_USTRING_MODE_UTF8 && MPT_WSTRING_CONVERT
	BasicAnyString(const std::wstring &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#endif
#if MPT_WSTRING_CONVERT
	BasicAnyString(const wchar_t *str) : mpt::ustring(str ? mpt::ToUnicode(str) : mpt::ustring()) { }
#endif

	// mfc
#if defined(_MFC_VER)
	BasicAnyString(const CString &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#ifndef UNICODE
	BasicAnyString(const CStringW &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#endif
#endif

	// fallback for custom string types
	template <typename Tstring> BasicAnyString(const Tstring &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#if MPT_COMPILER_HAS_RVALUE_REF
	template <typename Tstring> BasicAnyString(Tstring &&str) : mpt::ustring(mpt::ToUnicode(std::forward<Tstring>(str))) { }
#endif

};

// AnyUnicodeString is convertable to mpt::ustring and constructable from any unicode string,
class AnyUnicodeString : public mpt::ustring
{

public:

	// unicode
	AnyUnicodeString(const mpt::ustring &str) : mpt::ustring(str) { }
#if MPT_COMPILER_HAS_RVALUE_REF
	AnyUnicodeString(mpt::ustring &&str) : mpt::ustring(std::move(str)) { }
#endif
#if MPT_USTRING_MODE_UTF8 && MPT_WSTRING_CONVERT
	AnyUnicodeString(const std::wstring &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#endif
#if MPT_WSTRING_CONVERT
	AnyUnicodeString(const wchar_t *str) : mpt::ustring(str ? mpt::ToUnicode(str) : mpt::ustring()) { }
#endif

	// mfc
#if defined(_MFC_VER)
	AnyUnicodeString(const CString &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#ifndef UNICODE
	AnyUnicodeString(const CStringW &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#endif
#endif

	// fallback for custom string types
	template <typename Tstring> AnyUnicodeString(const Tstring &str) : mpt::ustring(mpt::ToUnicode(str)) { }
#if MPT_COMPILER_HAS_RVALUE_REF
	template <typename Tstring> AnyUnicodeString(Tstring &&str) : mpt::ustring(mpt::ToUnicode(std::forward<Tstring>(str))) { }
#endif

};

// AnyString
// Try to do the smartest auto-magic we can do.
#if defined(MPT_WITH_CHARSET_LOCALE)
typedef BasicAnyString<mpt::CharsetLocale, true> AnyString;
#elif MPT_OS_WINDOWS
typedef BasicAnyString<mpt::CharsetWindows1252, true> AnyString;
#else
typedef BasicAnyString<mpt::CharsetISO8859_1, true> AnyString;
#endif

// AnyStringLocale
// char-based strings are assumed to be in locale encoding.
#if defined(MPT_WITH_CHARSET_LOCALE)
typedef BasicAnyString<mpt::CharsetLocale, false> AnyStringLocale;
#else
typedef BasicAnyString<mpt::CharsetUTF8, false> AnyStringLocale;
#endif

// AnyStringUTF8orLocale
// char-based strings are tried in UTF8 first, if this fails, locale is used.
#if defined(MPT_WITH_CHARSET_LOCALE)
typedef BasicAnyString<mpt::CharsetLocale, true> AnyStringUTF8orLocale;
#else
typedef BasicAnyString<mpt::CharsetUTF8, false> AnyStringUTF8orLocale;
#endif

// AnyStringUTF8
// char-based strings are assumed to be in UTF8.
typedef BasicAnyString<mpt::CharsetUTF8, false> AnyStringUTF8;





// The following section demands a rationale.
//  1. ToString(), ToWString() an ToUString() mimic the semantics of c++11 std::to_string() and std::to_wstring().
//     There is an important difference though. The c++11 versions are specified in terms of sprintf formatting which in turn
//     depend on the current C locale. This renders these functions unusable in a library context because the current
//     C locale is set by the library-using application and could be anything. There is no way a library can get reliable semantics
//     out of these functions. It is thus better to just avoid them.
//     ToString() and ToWString() are based on iostream internally, but the the locale of the stream is forced to std::locale::classic(),
//     which results in "C" ASCII locale behavior.
//  2. The full suite of printf-like or iostream like number formatting is generally not required. Instead, a sane subset functionality
//     is provided here.
//     For convenience, mpt::fmt::f(const char *, float) allows formatting a single floating point value with a
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
//  4. Every function is available for std::string, std::wstring and mpt::ustring. std::string makes no assumption about the encoding, which
//     basically means, it should work for any 7-bit or 8-bit encoding, including for example ASCII, UTF8 or the current locale encoding.
//     std::string        std::wstring       mpt::ustring                           Tstring
//     mpt::ToString      mpt::ToWString     mpt::ToUString                         mpt::ToStringT<Tstring>
//     mpt::FormatVal     mpt::FormatValW    mpt::FormatValTFunctor<mpt::ustring>() mpt::FormatValTFunctor<Tstring>()
//     mpt::fmt           mpt::wfmt          mpt::ufmt                              mpt::fmtT<Tstring>
//     mpt::String::Print mpt::String::Print mpt::String::Print                     mpt::String::Print<Tstring>
//  5. All functionality here delegates real work outside of the header file so that <sstream> and <locale> do not need to be included when
//     using this functionality.
//     Advantages:
//      - Avoids binary code bloat when too much of iostream operator << gets inlined at every usage site.
//      - Faster compile times because <sstream> and <locale> (2 very complex headers) are not included everywhere.
//     Disadvantages:
//      - Slightly more c++ code is required for delegating work.
//      - As the header does not use iostreams, custom types need to overload mpt::ToString, mpt::ToWstring and mpt::ToUString instead of
//        iostream operator << to allow for custom type formatting.
//      - std::string, std::wstring and mpt::ustring are returned from somewhat deep cascades of helper functions. Where possible, code is
//        written in such a way that return-value-optimization (RVO) or named-return-value-optimization (NRVO) should be able to eliminate
//        almost all these copies. This should not be a problem for any decent modern compiler (and even less so for a c++11 compiler where
//        move-semantics will kick in if RVO/NRVO fails).

namespace mpt
{

// ToString() converts various built-in types to a well-defined, locale-independent string representation.
// This is also used as a type-tunnel pattern for mpt::String::Print.
// Custom types that need to be converted to strings are encouraged to overload ToString() and ToWString().

static inline std::string ToString(const std::string & x) { return x; }
static inline std::string ToString(const char * const & x) { return x; }
MPT_DEPRECATED static inline std::string ToString(const char & x) { return std::string(1, x); } // deprecated to catch potential API mis-use, use std::string(1, x) instead
#if MPT_WSTRING_FORMAT
MPT_DEPRECATED std::string ToString(const std::wstring & x);
MPT_DEPRECATED std::string ToString(const wchar_t * const & x);
MPT_DEPRECATED std::string ToString(const wchar_t & x); // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
#endif
#if MPT_USTRING_MODE_UTF8
MPT_DEPRECATED std::string ToString(const mpt::ustring & x);
#endif
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

#if MPT_WSTRING_FORMAT
MPT_DEPRECATED std::wstring ToWString(const std::string & x);
MPT_DEPRECATED std::wstring ToWString(const char * const & x);
MPT_DEPRECATED std::wstring ToWString(const char & x); // deprecated to catch potential API mis-use, use std::string(1, x) instead
static inline std::wstring ToWString(const std::wstring & x) { return x; }
static inline std::wstring ToWString(const wchar_t * const & x) { return x; }
MPT_DEPRECATED static inline std::wstring ToWString(const wchar_t & x) { return std::wstring(1, x); } // deprecated to catch potential API mis-use, use std::wstring(1, x) instead
#if MPT_USTRING_MODE_UTF8
std::wstring ToWString(const mpt::ustring & x);
#endif
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
#endif

#if MPT_USTRING_MODE_UTF8
template<typename T>
mpt::ustring ToUString(const T & x)
{
	return mpt::ToUnicode(mpt::CharsetUTF8, ToString(x));
}
template<>
inline mpt::ustring ToUString<mpt::ustring>(const mpt::ustring & x)
{
	return x;
}
#if MPT_WSTRING_FORMAT
static inline mpt::ustring ToUString(const std::wstring & x) { return mpt::ToUnicode(x); }
#endif
#else
template<typename T>
mpt::ustring ToUString(const T & x)
{
	return ToWString(x);
}
#endif

template <typename Tstring> struct ToStringTFunctor {};
template <> struct ToStringTFunctor<std::string> { template <typename T> inline std::string operator() (const T & x) { return ToString(x); } };
#if MPT_WSTRING_FORMAT
template <> struct ToStringTFunctor<std::wstring> { template <typename T> inline std::wstring operator() (const T & x) { return ToWString(x); } };
#endif
#if MPT_USTRING_MODE_UTF8
template <> struct ToStringTFunctor<mpt::ustring> { template <typename T> inline mpt::ustring operator() (const T & x) { return ToUString(x); } };
#endif

template<typename Tstring, typename T> inline Tstring ToStringT(const T & x) { return ToStringTFunctor<Tstring>()(x); }


struct fmt_base
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

}; // struct fmt_base

typedef unsigned int FormatFlags;

STATIC_ASSERT(sizeof(FormatFlags) >= sizeof(fmt_base::FormatFlagsEnum));

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

#if MPT_WSTRING_FORMAT
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
#endif

template <typename Tstring> struct FormatValTFunctor {};
template <> struct FormatValTFunctor<std::string> { template <typename T> inline std::string operator() (const T & x, const Format & f) { return FormatVal(x, f); } };
template <> struct FormatValTFunctor<std::wstring> { template <typename T> inline std::wstring operator() (const T & x, const Format & f) { return FormatValW(x, f); } };
#if MPT_USTRING_MODE_UTF8
template <> struct FormatValTFunctor<mpt::ustring> { template <typename T> inline mpt::ustring operator() (const T & x, const Format & f) { return mpt::ToUnicode(mpt::CharsetUTF8, FormatVal(x, f)); } };
#endif


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
	Format & BaseDec() { flags &= ~(fmt_base::BaseDec|fmt_base::BaseHex); flags |= fmt_base::BaseDec; return *this; }
	Format & BaseHex() { flags &= ~(fmt_base::BaseDec|fmt_base::BaseHex); flags |= fmt_base::BaseHex; return *this; }
	Format & CaseLow() { flags &= ~(fmt_base::CaseLow|fmt_base::CaseUpp); flags |= fmt_base::CaseLow; return *this; }
	Format & CaseUpp() { flags &= ~(fmt_base::CaseLow|fmt_base::CaseUpp); flags |= fmt_base::CaseUpp; return *this; }
	Format & FillOff() { flags &= ~(fmt_base::FillOff|fmt_base::FillSpc|fmt_base::FillNul); flags |= fmt_base::FillOff; return *this; }
	Format & FillSpc() { flags &= ~(fmt_base::FillOff|fmt_base::FillSpc|fmt_base::FillNul); flags |= fmt_base::FillSpc; return *this; }
	Format & FillNul() { flags &= ~(fmt_base::FillOff|fmt_base::FillSpc|fmt_base::FillNul); flags |= fmt_base::FillNul; return *this; }
	Format & NotaNrm() { flags &= ~(fmt_base::NotaNrm|fmt_base::NotaFix|fmt_base::NotaSci); flags |= fmt_base::NotaNrm; return *this; }
	Format & NotaFix() { flags &= ~(fmt_base::NotaNrm|fmt_base::NotaFix|fmt_base::NotaSci); flags |= fmt_base::NotaFix; return *this; }
	Format & NotaSci() { flags &= ~(fmt_base::NotaNrm|fmt_base::NotaFix|fmt_base::NotaSci); flags |= fmt_base::NotaSci; return *this; }
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
	template<typename Tstring, typename T>
	inline Tstring ToStringT(const T & x) const
	{
		return FormatValTFunctor<Tstring>()(x, *this);
	}
	template<typename T>
	inline std::string ToString(const T & x) const
	{
		return FormatVal(x, *this);
	}
#if MPT_WSTRING_FORMAT
	template<typename T>
	inline std::wstring ToWString(const T & x) const
	{
		return FormatValW(x, *this);
	}
#endif
};


template <typename Tstring>
struct fmtT : fmt_base
{

template<typename T>
static inline Tstring dec(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, Format().BaseDec().FillOff());
}
template<int width, typename T>
static inline Tstring dec(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, Format().BaseDec().FillSpc().Width(width));
}
template<int width, typename T>
static inline Tstring dec0(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, Format().BaseDec().FillNul().Width(width));
}

template<typename T>
static inline Tstring hex(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, Format().BaseHex().CaseLow().FillOff());
}
template<typename T>
static inline Tstring HEX(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, Format().BaseHex().CaseUpp().FillOff());
}
template<int width, typename T>
static inline Tstring hex(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, Format().BaseHex().CaseLow().FillSpc().Width(width));
}
template<int width, typename T>
static inline Tstring HEX(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, Format().BaseHex().CaseUpp().FillSpc().Width(width));
}
template<int width, typename T>
static inline Tstring hex0(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, Format().BaseHex().CaseLow().FillNul().Width(width));
}
template<int width, typename T>
static inline Tstring HEX0(const T& x)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	return FormatValTFunctor<Tstring>()(x, Format().BaseHex().CaseUpp().FillNul().Width(width));
}

template<typename T>
static inline Tstring flt(const T& x, std::size_t width = 0, int precision = -1)
{
	#if defined(HAS_TYPE_TRAITS)
		STATIC_ASSERT(std::is_floating_point<T>::value);
	#endif
	if(width == 0)
	{
		return FormatValTFunctor<Tstring>()(x, Format().NotaNrm().FillOff().Precision(precision));
	} else
	{
		return FormatValTFunctor<Tstring>()(x, Format().NotaNrm().FillSpc().Width(width).Precision(precision));
	}
}
template<typename T>
static inline Tstring fix(const T& x, std::size_t width = 0, int precision = -1)
{
	#if defined(HAS_TYPE_TRAITS)
		STATIC_ASSERT(std::is_floating_point<T>::value);
	#endif
	if(width == 0)
	{
		return FormatValTFunctor<Tstring>()(x, Format().NotaFix().FillOff().Precision(precision));
	} else
	{
		return FormatValTFunctor<Tstring>()(x, Format().NotaFix().FillSpc().Width(width).Precision(precision));
	}
}
template<typename T>
static inline Tstring sci(const T& x, std::size_t width = 0, int precision = -1)
{
	#if defined(HAS_TYPE_TRAITS)
		STATIC_ASSERT(std::is_floating_point<T>::value);
	#endif
	if(width == 0)
	{
		return FormatValTFunctor<Tstring>()(x, Format().NotaSci().FillOff().Precision(precision));
	} else
	{
		return FormatValTFunctor<Tstring>()(x, Format().NotaSci().FillSpc().Width(width).Precision(precision));
	}
}

template <typename T, typename Tformat>
static inline Tstring f(const Tformat & format, const T& x)
{
	#if defined(HAS_TYPE_TRAITS)
		STATIC_ASSERT(std::is_floating_point<T>::value);
	#endif
	return FormatValTFunctor<Tstring>()(x, Format().ParsePrintf(format));
}

}; // struct fmtT

typedef fmtT<std::string> fmt;
#if MPT_WSTRING_FORMAT
typedef fmtT<std::wstring> wfmt;
#endif
#if MPT_USTRING_MODE_WIDE
typedef fmtT<std::wstring> ufmt;
#else
typedef fmtT<mpt::ustring> ufmt;
#endif

} // namespace mpt

#define Stringify(x) mpt::ToString(x)
#if MPT_WSTRING_FORMAT
#define StringifyW(x) mpt::ToWString(x)
#endif

namespace mpt { namespace String {

namespace detail
{

template <typename T> struct to_string_type { };
template <> struct to_string_type<std::string    > { typedef std::string  type; };
template <> struct to_string_type<char           > { typedef std::string  type; };
template <> struct to_string_type<char *         > { typedef std::string  type; };
template <> struct to_string_type<const char *   > { typedef std::string  type; };
template <> struct to_string_type<std::wstring   > { typedef std::wstring type; };
template <> struct to_string_type<wchar_t        > { typedef std::wstring type; };
template <> struct to_string_type<wchar_t *      > { typedef std::wstring type; };
template <> struct to_string_type<const wchar_t *> { typedef std::wstring type; };
#if MPT_USTRING_MODE_UTF8
template <> struct to_string_type<mpt::ustring   > { typedef mpt::ustring type; };
#endif

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

#if MPT_WSTRING_FORMAT
std::wstring PrintImpl(const std::wstring & format
	, const std::wstring & x1 = std::wstring()
	, const std::wstring & x2 = std::wstring()
	, const std::wstring & x3 = std::wstring()
	, const std::wstring & x4 = std::wstring()
	, const std::wstring & x5 = std::wstring()
	, const std::wstring & x6 = std::wstring()
	, const std::wstring & x7 = std::wstring()
	, const std::wstring & x8 = std::wstring()
	);
#endif

#if MPT_USTRING_MODE_UTF8
mpt::ustring PrintImpl(const mpt::ustring & format
	, const mpt::ustring & x1 = mpt::ustring()
	, const mpt::ustring & x2 = mpt::ustring()
	, const mpt::ustring & x3 = mpt::ustring()
	, const mpt::ustring & x4 = mpt::ustring()
	, const mpt::ustring & x5 = mpt::ustring()
	, const mpt::ustring & x6 = mpt::ustring()
	, const mpt::ustring & x7 = mpt::ustring()
	, const mpt::ustring & x8 = mpt::ustring()
	);
#endif

} // namespace detail

// C and C++ string version

template<typename Tformat
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat & format
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
	);
}

template<typename Tformat
	, typename T1
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat & format
	, const T1& x1
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
	);
}

template<typename Tformat
	, typename T1
	, typename T2
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat & format
	, const T1& x1
	, const T2& x2
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
	);
}

template<typename Tformat
	, typename T1
	, typename T2
	, typename T3
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
	);
}

template<typename Tformat
	, typename T1
	, typename T2
	, typename T3
	, typename T4
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
		, ToStringTFunctor<Tstring>()(x4)
	);
}

template<typename Tformat
	, typename T1
	, typename T2
	, typename T3
	, typename T4
	, typename T5
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
		, ToStringTFunctor<Tstring>()(x4)
		, ToStringTFunctor<Tstring>()(x5)
	);
}

template<typename Tformat
	, typename T1
	, typename T2
	, typename T3
	, typename T4
	, typename T5
	, typename T6
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
	, const T6& x6
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
		, ToStringTFunctor<Tstring>()(x4)
		, ToStringTFunctor<Tstring>()(x5)
		, ToStringTFunctor<Tstring>()(x6)
	);
}

template<typename Tformat
	, typename T1
	, typename T2
	, typename T3
	, typename T4
	, typename T5
	, typename T6
	, typename T7
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat & format
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
	, const T6& x6
	, const T7& x7
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
		, ToStringTFunctor<Tstring>()(x4)
		, ToStringTFunctor<Tstring>()(x5)
		, ToStringTFunctor<Tstring>()(x6)
		, ToStringTFunctor<Tstring>()(x7)
	);
}

template<typename Tformat
	, typename T1
	, typename T2
	, typename T3
	, typename T4
	, typename T5
	, typename T6
	, typename T7
	, typename T8
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat & format
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
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
		, ToStringTFunctor<Tstring>()(x4)
		, ToStringTFunctor<Tstring>()(x5)
		, ToStringTFunctor<Tstring>()(x6)
		, ToStringTFunctor<Tstring>()(x7)
		, ToStringTFunctor<Tstring>()(x8)
	);
}

// array version for literals

template<typename Tformat, std::size_t N
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat (&format)[N]
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
	);
}

template<typename Tformat, std::size_t N
	, typename T1
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat (&format)[N]
	, const T1& x1
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
	);
}

template<typename Tformat, std::size_t N
	, typename T1
	, typename T2
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat (&format)[N]
	, const T1& x1
	, const T2& x2
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
	);
}

template<typename Tformat, std::size_t N
	, typename T1
	, typename T2
	, typename T3
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat (&format)[N]
	, const T1& x1
	, const T2& x2
	, const T3& x3
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
	);
}

template<typename Tformat, std::size_t N
	, typename T1
	, typename T2
	, typename T3
	, typename T4
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat (&format)[N]
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
		, ToStringTFunctor<Tstring>()(x4)
	);
}

template<typename Tformat, std::size_t N
	, typename T1
	, typename T2
	, typename T3
	, typename T4
	, typename T5
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat (&format)[N]
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
		, ToStringTFunctor<Tstring>()(x4)
		, ToStringTFunctor<Tstring>()(x5)
	);
}

template<typename Tformat, std::size_t N
	, typename T1
	, typename T2
	, typename T3
	, typename T4
	, typename T5
	, typename T6
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat (&format)[N]
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
	, const T6& x6
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
		, ToStringTFunctor<Tstring>()(x4)
		, ToStringTFunctor<Tstring>()(x5)
		, ToStringTFunctor<Tstring>()(x6)
	);
}

template<typename Tformat, std::size_t N
	, typename T1
	, typename T2
	, typename T3
	, typename T4
	, typename T5
	, typename T6
	, typename T7
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat (&format)[N]
	, const T1& x1
	, const T2& x2
	, const T3& x3
	, const T4& x4
	, const T5& x5
	, const T6& x6
	, const T7& x7
)
{
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
		, ToStringTFunctor<Tstring>()(x4)
		, ToStringTFunctor<Tstring>()(x5)
		, ToStringTFunctor<Tstring>()(x6)
		, ToStringTFunctor<Tstring>()(x7)
	);
}

template<typename Tformat, std::size_t N
	, typename T1
	, typename T2
	, typename T3
	, typename T4
	, typename T5
	, typename T6
	, typename T7
	, typename T8
>
typename mpt::String::detail::to_string_type<Tformat>::type Print(const Tformat (&format)[N]
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
	typedef typename mpt::String::detail::to_string_type<Tformat>::type Tstring;
	return detail::PrintImpl(Tstring(format)
		, ToStringTFunctor<Tstring>()(x1)
		, ToStringTFunctor<Tstring>()(x2)
		, ToStringTFunctor<Tstring>()(x3)
		, ToStringTFunctor<Tstring>()(x4)
		, ToStringTFunctor<Tstring>()(x5)
		, ToStringTFunctor<Tstring>()(x6)
		, ToStringTFunctor<Tstring>()(x7)
		, ToStringTFunctor<Tstring>()(x8)
	);
}

#define PrintW Print

} } // namespace mpt::String

OPENMPT_NAMESPACE_END
