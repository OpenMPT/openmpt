/*
 * mptStringFormat.cpp
 * -------------------
 * Purpose: Convert other types to strings.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "mptStringFormat.h"

#if MPT_MSVC_AT_LEAST(2019,4) || MPT_GCC_AT_LEAST(11,1,0)
#define MPT_FORMAT_CXX17_FLOAT 1
#else
#define MPT_FORMAT_CXX17_FLOAT 0
#endif

#include <charconv>
#include <iomanip>
#if MPT_FORMAT_CXX17_FLOAT
#include <iterator>
#endif // MPT_FORMAT_CXX17_FLOAT
#include <locale>
#include <sstream>
#include <string>
#include <system_error>


OPENMPT_NAMESPACE_BEGIN



namespace mpt
{



#if MPT_WSTRING_FORMAT
static std::wstring ToWideSimple(const std::string &nstr)
{
	std::wstring wstr(nstr.size(), L'\0');
	for(std::size_t i = 0; i < nstr.size(); ++i)
	{
		wstr[i] = static_cast<unsigned char>(nstr[i]);
	}
	return wstr;
}
#endif // MPT_WSTRING_FORMAT

template<typename T>
static inline std::string ToCharsInt(const T & x, int base = 10)
{
	std::string str(1, '\0');
	bool done = false;
	while(!done)
	{
		std::to_chars_result result = std::to_chars(str.data(), str.data() + str.size(), x, base);
		if(result.ec != std::errc{})
		{
			str.resize(Util::ExponentialGrow(str.size()), '\0');
		} else
		{
			str.resize(result.ptr - str.data());
			done = true;
		}
	}
	return str;
}

template<typename T>
static inline std::string ToStringHelperInt(const T & x)
{
	return ToCharsInt(x);
}

#if MPT_WSTRING_FORMAT
template<typename T>
static inline std::wstring ToWStringHelperInt(const T & x)
{
	return ToWideSimple(ToCharsInt(x));
}
#endif

#if MPT_FORMAT_CXX17_FLOAT

template<typename T>
static inline std::string ToCharsFloat(const T & x)
{
	std::string str(1, '\0');
	bool done = false;
	while(!done)
	{
		std::to_chars_result result = std::to_chars(str.data(), str.data() + str.size(), x);
		if(result.ec != std::errc{})
		{
			str.resize(Util::ExponentialGrow(str.size()), '\0');
		} else
		{
			str.resize(result.ptr - str.data());
			done = true;
		}
	}
	return str;
}

template<typename T>
static inline std::string ToCharsFloat(const T & x, std::chars_format fmt)
{
	std::string str(1, '\0');
	bool done = false;
	while(!done)
	{
		std::to_chars_result result = std::to_chars(str.data(), str.data() + str.size(), x, fmt);
		if(result.ec != std::errc{})
		{
			str.resize(Util::ExponentialGrow(str.size()), '\0');
		} else
		{
			str.resize(result.ptr - str.data());
			done = true;
		}
	}
	return str;
}

template<typename T>
static inline std::string ToCharsFloat(const T & x, std::chars_format fmt, int precision)
{
	std::string str(1, '\0');
	bool done = false;
	while(!done)
	{
		std::to_chars_result result = std::to_chars(str.data(), str.data() + str.size(), x, fmt, precision);
		if(result.ec != std::errc{})
		{
			str.resize(Util::ExponentialGrow(str.size()), '\0');
		} else
		{
			str.resize(result.ptr - str.data());
			done = true;
		}
	}
	return str;
}

template<typename T>
static inline std::string ToStringHelperFloat(const T & x)
{
	return ToCharsFloat(x);
}

#if MPT_WSTRING_FORMAT
template<typename T>
static inline std::wstring ToWStringHelperFloat(const T & x)
{
	return ToWideSimple(ToCharsFloat(x));
}
#endif

#else // !MPT_FORMAT_CXX17_FLOAT

template<typename T>
static inline std::string ToStringHelperFloat(const T & x)
{
	std::ostringstream o;
	o.imbue(std::locale::classic());
	o << std::setprecision(std::numeric_limits<T>::max_digits10) << x;
	return o.str();
}

#if MPT_WSTRING_FORMAT
template<typename T>
static inline std::wstring ToWStringHelperFloat(const T & x)
{
	std::wostringstream o;
	o.imbue(std::locale::classic());
	o << std::setprecision(std::numeric_limits<T>::max_digits10) << x;
	return o.str();
}
#endif

#endif // MPT_FORMAT_CXX17_FLOAT

std::string ToString(const bool & x) { return ToStringHelperInt(static_cast<int>(x)); }
std::string ToString(const signed char & x) { return ToStringHelperInt(x); }
std::string ToString(const unsigned char & x) { return ToStringHelperInt(x); }
std::string ToString(const signed short & x) { return ToStringHelperInt(x); }
std::string ToString(const unsigned short & x) { return ToStringHelperInt(x); }
std::string ToString(const signed int & x) { return ToStringHelperInt(x); }
std::string ToString(const unsigned int & x) { return ToStringHelperInt(x); }
std::string ToString(const signed long & x) { return ToStringHelperInt(x); }
std::string ToString(const unsigned long & x) { return ToStringHelperInt(x); }
std::string ToString(const signed long long & x) { return ToStringHelperInt(x); }
std::string ToString(const unsigned long long & x) { return ToStringHelperInt(x); }
std::string ToString(const float & x) { return ToStringHelperFloat(x); }
std::string ToString(const double & x) { return ToStringHelperFloat(x); }
std::string ToString(const long double & x) { return ToStringHelperFloat(x); }

#if MPT_WSTRING_FORMAT
#if MPT_USTRING_MODE_UTF8
mpt::ustring ToUString(const std::wstring & x) { return mpt::ToUnicode(x); }
#endif
mpt::ustring ToUString(const wchar_t * const & x) { return mpt::ToUnicode(x); }
#endif
#if defined(MPT_WITH_MFC)
mpt::ustring ToUString(const CString & x)  { return mpt::ToUnicode(x); }
#endif // MPT_WITH_MFC
#if MPT_USTRING_MODE_WIDE
mpt::ustring ToUString(const bool & x) { return ToWStringHelperInt(static_cast<int>(x)); }
mpt::ustring ToUString(const signed char & x) { return ToWStringHelperInt(x); }
mpt::ustring ToUString(const unsigned char & x) { return ToWStringHelperInt(x); }
mpt::ustring ToUString(const signed short & x) { return ToWStringHelperInt(x); }
mpt::ustring ToUString(const unsigned short & x) { return ToWStringHelperInt(x); }
mpt::ustring ToUString(const signed int & x) { return ToWStringHelperInt(x); }
mpt::ustring ToUString(const unsigned int & x) { return ToWStringHelperInt(x); }
mpt::ustring ToUString(const signed long & x) { return ToWStringHelperInt(x); }
mpt::ustring ToUString(const unsigned long & x) { return ToWStringHelperInt(x); }
mpt::ustring ToUString(const signed long long & x) { return ToWStringHelperInt(x); }
mpt::ustring ToUString(const unsigned long long & x) { return ToWStringHelperInt(x); }
mpt::ustring ToUString(const float & x) { return ToWStringHelperFloat(x); }
mpt::ustring ToUString(const double & x) { return ToWStringHelperFloat(x); }
mpt::ustring ToUString(const long double & x) { return ToWStringHelperFloat(x); }
#endif
#if MPT_USTRING_MODE_UTF8
mpt::ustring ToUString(const bool & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperInt(static_cast<int>(x))); }
mpt::ustring ToUString(const signed char & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperInt(x)); }
mpt::ustring ToUString(const unsigned char & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperInt(x)); }
mpt::ustring ToUString(const signed short & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperInt(x)); }
mpt::ustring ToUString(const unsigned short & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperInt(x)); }
mpt::ustring ToUString(const signed int & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperInt(x)); }
mpt::ustring ToUString(const unsigned int & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperInt(x)); }
mpt::ustring ToUString(const signed long & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperInt(x)); }
mpt::ustring ToUString(const unsigned long & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperInt(x)); }
mpt::ustring ToUString(const signed long long & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperInt(x)); }
mpt::ustring ToUString(const unsigned long long & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperInt(x)); }
mpt::ustring ToUString(const float & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperFloat(x)); }
mpt::ustring ToUString(const double & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperFloat(x)); }
mpt::ustring ToUString(const long double & x) { return mpt::ToUnicode(mpt::Charset::UTF8, ToStringHelperFloat(x)); }
#endif

#if MPT_WSTRING_FORMAT
#if MPT_USTRING_MODE_UTF8
std::wstring ToWString(const mpt::ustring & x) { return mpt::ToWide(x); }
#endif
#if defined(MPT_WITH_MFC)
std::wstring ToWString(const CString & x) { return mpt::ToWide(x); }
#endif // MPT_WITH_MFC
std::wstring ToWString(const bool & x) { return ToWStringHelperInt(static_cast<int>(x)); }
std::wstring ToWString(const signed char & x) { return ToWStringHelperInt(x); }
std::wstring ToWString(const unsigned char & x) { return ToWStringHelperInt(x); }
std::wstring ToWString(const signed short & x) { return ToWStringHelperInt(x); }
std::wstring ToWString(const unsigned short & x) { return ToWStringHelperInt(x); }
std::wstring ToWString(const signed int & x) { return ToWStringHelperInt(x); }
std::wstring ToWString(const unsigned int & x) { return ToWStringHelperInt(x); }
std::wstring ToWString(const signed long & x) { return ToWStringHelperInt(x); }
std::wstring ToWString(const unsigned long & x) { return ToWStringHelperInt(x); }
std::wstring ToWString(const signed long long & x) { return ToWStringHelperInt(x); }
std::wstring ToWString(const unsigned long long & x) { return ToWStringHelperInt(x); }
std::wstring ToWString(const float & x) { return ToWStringHelperFloat(x); }
std::wstring ToWString(const double & x) { return ToWStringHelperFloat(x); }
std::wstring ToWString(const long double & x) { return ToWStringHelperFloat(x); }
#endif


template <typename Tchar>
struct NumPunct : std::numpunct<Tchar>
{
private:
	unsigned int group;
	char sep;
public:
	NumPunct(unsigned int g, char s)
		: group(g)
		, sep(s)
	{}
	std::string do_grouping() const override
	{
		return std::string(1, static_cast<char>(group));
	}
	Tchar do_thousands_sep() const override
	{
		return static_cast<Tchar>(sep);
	}
};

template<typename Tostream, typename T>
static inline void ApplyFormat(Tostream & o, const FormatSpec & format, const T &)
{
	if constexpr(!std::numeric_limits<T>::is_integer)
	{
		if(format.GetGroup() > 0)
		{
			o.imbue(std::locale(o.getloc(), new NumPunct<typename Tostream::char_type>(format.GetGroup(), format.GetGroupSep())));
		}
	}
	FormatFlags f = format.GetFlags();
	std::size_t width = format.GetWidth();
	int precision = format.GetPrecision();
	if(precision != -1 && width != 0 && !(f & fmt_base::NotaFix) && !(f & fmt_base::NotaSci))
	{
		// fixup:
		// precision behaves differently from .#
		// avoid default format when precision and width are set
		f &= ~fmt_base::NotaNrm;
		f |= fmt_base::NotaFix;
	}
	if(f & fmt_base::BaseDec) { o << std::dec; }
	else if(f & fmt_base::BaseHex) { o << std::hex; }
	if(f & fmt_base::NotaNrm ) { /*nothing*/ }
	else if(f & fmt_base::NotaFix ) { o << std::setiosflags(std::ios::fixed); }
	else if(f & fmt_base::NotaSci ) { o << std::setiosflags(std::ios::scientific); }
	if(f & fmt_base::CaseLow) { o << std::nouppercase; }
	else if(f & fmt_base::CaseUpp) { o << std::uppercase; }
	if constexpr(!std::numeric_limits<T>::is_integer)
	{
		if(f & fmt_base::FillOff) { /* nothing */ }
		else if(f & fmt_base::FillNul) { o << std::setw(width) << std::setfill(typename Tostream::char_type('0')); }
	}
	if(precision != -1)
	{
		o << std::setprecision(precision);
	} else
	{
		if constexpr(std::is_floating_point<T>::value)
		{
			if(f & fmt_base::NotaNrm)
			{
				o << std::setprecision(std::numeric_limits<T>::max_digits10);
			} else if(f & fmt_base::NotaFix)
			{
				o << std::setprecision(std::numeric_limits<T>::digits10);
			} else if(f & fmt_base::NotaSci)
			{
				o << std::setprecision(std::numeric_limits<T>::max_digits10 - 1);
			} else
			{
				o << std::setprecision(std::numeric_limits<T>::max_digits10);
			}
		}
	}
}

template<typename Tstring>
static inline Tstring PostProcessCase(Tstring str, const FormatSpec & format)
{
	FormatFlags f = format.GetFlags();
	if(f & fmt_base::CaseUpp)
	{
		for(auto & c : str)
		{
			if('a' <= c && c <= 'z')
			{
				c -= 'a' - 'A';
			}
		}
	}
	return str;
}

template<typename Tstring>
static inline Tstring PostProcessDigits(Tstring str, const FormatSpec & format)
{
	FormatFlags f = format.GetFlags();
	std::size_t width = format.GetWidth();
	if(f & fmt_base::FillNul)
	{
		auto pos = str.begin();
		if(str.length() > 0)
		{
			if(str[0] == typename Tstring::value_type('+'))
			{
				pos++;
				width++;
			} else if(str[0] == typename Tstring::value_type('-'))
			{
				pos++;
				width++;
			}
		}
		if(str.length() < width)
		{
			str.insert(pos, width - str.length(), '0');
		}
	}
	return str;
}

#if MPT_FORMAT_CXX17_FLOAT

template<typename Tstring>
static inline Tstring PostProcessFloatWidth(Tstring str, const FormatSpec & format)
{
	FormatFlags f = format.GetFlags();
	std::size_t width = format.GetWidth();
	if(f & fmt_base::FillNul)
	{
		auto pos = str.begin();
		if(str.length() > 0)
		{
			if(str[0] == typename Tstring::value_type('+'))
			{
				pos++;
				width++;
			} else if(str[0] == typename Tstring::value_type('-'))
			{
				pos++;
				width++;
			}
		}
		if(str.length() - std::distance(str.begin(), pos) < width)
		{
			str.insert(pos, width - str.length() - std::distance(str.begin(), pos), '0');
		}
	} else
	{
		if(str.length() < width)
		{
			str.insert(0, width - str.length(), ' ');
		}
	}
	return str;
}

#endif // MPT_FORMAT_CXX17_FLOAT

#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4723) // potential divide by 0
#endif // MPT_COMPILER_MSVC
template<typename Tstring>
static inline Tstring PostProcessGroup(Tstring str, const FormatSpec & format)
{
	if(format.GetGroup() > 0)
	{
		const unsigned int groupSize = format.GetGroup();
		const char groupSep = format.GetGroupSep();
		std::size_t len = str.length();
		for(std::size_t n = 0; n < len; ++n)
		{
			if(n > 0 && (n % groupSize) == 0)
			{
				if(!(n == (len - 1) && (str[0] == typename Tstring::value_type('+') || str[0] == typename Tstring::value_type('-'))))
				{
					str.insert(str.begin() + (len - n), 1, groupSep);
				}
			}
		}
	}
	return str;
}
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC

template<typename T>
static inline std::string FormatValHelperInt(const T & x, const FormatSpec & f)
{
	int base = 10;
	if(f.GetFlags() & fmt_base::BaseDec) { base = 10; }
	if(f.GetFlags() & fmt_base::BaseHex) { base = 16; }
	return PostProcessGroup(PostProcessDigits(PostProcessCase(ToCharsInt(x, base), f), f), f);
}

#if MPT_WSTRING_FORMAT
template<typename T>
static inline std::wstring FormatValWHelperInt(const T & x, const FormatSpec & f)
{
	int base = 10;
	if(f.GetFlags() & fmt_base::BaseDec) { base = 10; }
	if(f.GetFlags() & fmt_base::BaseHex) { base = 16; }
	return ToWideSimple(PostProcessGroup(PostProcessDigits(PostProcessCase(ToCharsInt(x, base), f), f), f));
}
#endif

#if MPT_FORMAT_CXX17_FLOAT

template<typename T>
static inline std::string FormatValHelperFloat(const T & x, const FormatSpec & f)
{
	if(f.GetPrecision() != -1)
	{
		if(f.GetFlags() & fmt_base::NotaSci)
		{
			return PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::scientific, f.GetPrecision()), f);
		} else if(f.GetFlags() & fmt_base::NotaFix)
		{
			return PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::fixed, f.GetPrecision()), f);
		} else
		{
			return PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::general, f.GetPrecision()), f);
		}
	} else
	{
		if(f.GetFlags() & fmt_base::NotaSci)
		{
			return PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::scientific), f);
		} else if(f.GetFlags() & fmt_base::NotaFix)
		{
			return PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::fixed), f);
		} else
		{
			return PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::general), f);
		}
	}
}

#if MPT_WSTRING_FORMAT
template<typename T>
static inline std::wstring FormatValWHelperFloat(const T & x, const FormatSpec & f)
{
	if(f.GetPrecision() != -1)
	{
		if(f.GetFlags() & fmt_base::NotaSci)
		{
			return ToWideSimple(PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::scientific, f.GetPrecision()), f));
		} else if(f.GetFlags() & fmt_base::NotaFix)
		{
			return ToWideSimple(PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::fixed, f.GetPrecision()), f));
		} else
		{
			return ToWideSimple(PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::general, f.GetPrecision()), f));
		}
	} else
	{
		if(f.GetFlags() & fmt_base::NotaSci)
		{
			return ToWideSimple(PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::scientific), f));
		} else if(f.GetFlags() & fmt_base::NotaFix)
		{
			return ToWideSimple(PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::fixed), f));
		} else
		{
			return ToWideSimple(PostProcessFloatWidth(ToCharsFloat(x, std::chars_format::general), f));
		}
	}
}
#endif

#else // !MPT_FORMAT_CXX17_FLOAT

template<typename T>
static inline std::string FormatValHelperFloat(const T & x, const FormatSpec & f)
{
	std::ostringstream o;
	o.imbue(std::locale::classic());
	ApplyFormat(o, f, x);
	o << x;
	return o.str();
}

#if MPT_WSTRING_FORMAT
template<typename T>
static inline std::wstring FormatValWHelperFloat(const T & x, const FormatSpec & f)
{
	std::wostringstream o;
	o.imbue(std::locale::classic());
	ApplyFormat(o, f, x);
	o << x;
	return o.str();
}
#endif

#endif // MPT_FORMAT_CXX17_FLOAT

std::string FormatVal(const bool & x, const FormatSpec & f) { return FormatValHelperInt(static_cast<int>(x), f); }
std::string FormatVal(const signed char & x, const FormatSpec & f) { return FormatValHelperInt(x, f); }
std::string FormatVal(const unsigned char & x, const FormatSpec & f) { return FormatValHelperInt(x, f); }
std::string FormatVal(const signed short & x, const FormatSpec & f) { return FormatValHelperInt(x, f); }
std::string FormatVal(const unsigned short & x, const FormatSpec & f) { return FormatValHelperInt(x, f); }
std::string FormatVal(const signed int & x, const FormatSpec & f) { return FormatValHelperInt(x, f); }
std::string FormatVal(const unsigned int & x, const FormatSpec & f) { return FormatValHelperInt(x, f); }
std::string FormatVal(const signed long & x, const FormatSpec & f) { return FormatValHelperInt(x, f); }
std::string FormatVal(const unsigned long & x, const FormatSpec & f) { return FormatValHelperInt(x, f); }
std::string FormatVal(const signed long long & x, const FormatSpec & f) { return FormatValHelperInt(x, f); }
std::string FormatVal(const unsigned long long & x, const FormatSpec & f) { return FormatValHelperInt(x, f); }
std::string FormatVal(const float & x, const FormatSpec & f) { return FormatValHelperFloat(x, f); }
std::string FormatVal(const double & x, const FormatSpec & f) { return FormatValHelperFloat(x, f); }
std::string FormatVal(const long double & x, const FormatSpec & f) { return FormatValHelperFloat(x, f); }

#if MPT_USTRING_MODE_WIDE
mpt::ustring FormatValU(const bool & x, const FormatSpec & f) { return FormatValWHelperInt(static_cast<int>(x), f); }
mpt::ustring FormatValU(const signed char & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
mpt::ustring FormatValU(const unsigned char & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
mpt::ustring FormatValU(const signed short & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
mpt::ustring FormatValU(const unsigned short & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
mpt::ustring FormatValU(const signed int & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
mpt::ustring FormatValU(const unsigned int & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
mpt::ustring FormatValU(const signed long & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
mpt::ustring FormatValU(const unsigned long & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
mpt::ustring FormatValU(const signed long long & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
mpt::ustring FormatValU(const unsigned long long & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
mpt::ustring FormatValU(const float & x, const FormatSpec & f) { return FormatValWHelperFloat(x, f); }
mpt::ustring FormatValU(const double & x, const FormatSpec & f) { return FormatValWHelperFloat(x, f); }
mpt::ustring FormatValU(const long double & x, const FormatSpec & f) { return FormatValWHelperFloat(x, f); }
#endif
#if MPT_USTRING_MODE_UTF8
mpt::ustring FormatValU(const bool & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperInt(static_cast<int>(x), f)); }
mpt::ustring FormatValU(const signed char & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperInt(x, f)); }
mpt::ustring FormatValU(const unsigned char & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperInt(x, f)); }
mpt::ustring FormatValU(const signed short & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperInt(x, f)); }
mpt::ustring FormatValU(const unsigned short & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperInt(x, f)); }
mpt::ustring FormatValU(const signed int & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperInt(x, f)); }
mpt::ustring FormatValU(const unsigned int & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperInt(x, f)); }
mpt::ustring FormatValU(const signed long & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperInt(x, f)); }
mpt::ustring FormatValU(const unsigned long & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperInt(x, f)); }
mpt::ustring FormatValU(const signed long long & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperInt(x, f)); }
mpt::ustring FormatValU(const unsigned long long & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperInt(x, f)); }
mpt::ustring FormatValU(const float & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperFloat(x, f)); }
mpt::ustring FormatValU(const double & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperFloat(x, f)); }
mpt::ustring FormatValU(const long double & x, const FormatSpec & f) { return mpt::ToUnicode(mpt::Charset::UTF8, FormatValHelperFloat(x, f)); }
#endif

#if MPT_WSTRING_FORMAT
std::wstring FormatValW(const bool & x, const FormatSpec & f) { return FormatValWHelperInt(static_cast<int>(x), f); }
std::wstring FormatValW(const signed char & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
std::wstring FormatValW(const unsigned char & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
std::wstring FormatValW(const signed short & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
std::wstring FormatValW(const unsigned short & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
std::wstring FormatValW(const signed int & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
std::wstring FormatValW(const unsigned int & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
std::wstring FormatValW(const signed long & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
std::wstring FormatValW(const unsigned long & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
std::wstring FormatValW(const signed long long & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
std::wstring FormatValW(const unsigned long long & x, const FormatSpec & f) { return FormatValWHelperInt(x, f); }
std::wstring FormatValW(const float & x, const FormatSpec & f) { return FormatValWHelperFloat(x, f); }
std::wstring FormatValW(const double & x, const FormatSpec & f) { return FormatValWHelperFloat(x, f); }
std::wstring FormatValW(const long double & x, const FormatSpec & f) { return FormatValWHelperFloat(x, f); }
#endif


} // namespace mpt


OPENMPT_NAMESPACE_END
