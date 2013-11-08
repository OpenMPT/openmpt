/*
 * mptString.h
 * ----------
 * Purpose: A wrapper around std::string implemeting the CString interface.
 * Notes  : Should be removed somewhen in the future when all uses of CString have been converted to std::string.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <string>
#include <cstdio>
#include <cstring>
#include <stdio.h>
#if MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#include <strings.h> // for strcasecmp
#endif

#if MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#define MPT_PRINTF_FUNC(formatstringindex,varargsindex) __attribute__((format(printf, formatstringindex, varargsindex)))
#else
#define MPT_PRINTF_FUNC(formatstringindex,varargsindex)
#endif


namespace mpt
{


#ifdef MODPLUG_TRACKER
	// std::string compatible class that is convertible to const char* (and thus CString).
	// This eases interfacing CSoundFile from MFC code in some cases.
	// The goal is to remove this sometime in the future.
	class string : public std::string
	{
	public:
		string() {}
		string(const char *psz) : std::string(psz) {}
		string(const std::string &other) : std::string(other) {}
		string& operator=(const char *psz) { std::string::operator=(psz); return *this; }
		operator const char *() const { return c_str(); } // To allow easy assignment to CString in GUI side.
	};
#else
	typedef std::string string;
#endif


namespace String
{


std::string MPT_PRINTF_FUNC(1,2) Format(const char * format, ...);


// Remove whitespace at start of string
static inline std::string LTrim(std::string str, const std::string &whitespace = " \n\r\t")
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


// Remove whitespace at end of string
static inline std::string RTrim(std::string str, const std::string &whitespace = " \n\r\t")
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


// Remove whitespace at start and end of string
static inline std::string Trim(std::string str, const std::string &whitespace = " \n\r\t")
{
	return RTrim(LTrim(str, whitespace), whitespace);
}


static inline std::string Replace(std::string str, const std::string &oldStr, const std::string &newStr)
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
}


static inline int strnicmp(const char *a, const char *b, size_t count)
{
	#if MPT_COMPILER_MSVC
		return _strnicmp(a, b, count);
	#else
		return strncasecmp(a, b, count);
	#endif
}


enum Charset {
	CharsetLocale,

	CharsetUTF8,

	CharsetUS_ASCII,

	CharsetISO8859_1,
	CharsetISO8859_15,

	CharsetCP437,

	CharsetWindows1252,
};


namespace String {

std::string Encode(const std::wstring &src, Charset charset);
std::wstring Decode(const std::string &src, Charset charset);

std::string Convert(const std::string &src, Charset from, Charset to);

} // namespace String



#if defined(WIN32)
typedef std::wstring RawPathString;
#else // !WIN32
typedef std::string RawPathString;
#endif // WIN32

class PathString
{
private:
	RawPathString path;
private:
	PathString(const RawPathString & path)
		: path(path)
	{
		return;
	}
public:
	PathString()
	{
		return;
	}
	PathString(const PathString & other)
		: path(other.path)
	{
		return;
	}
	PathString & assign(const PathString & other)
	{
		path = other.path;
		return *this;
	}
	PathString & operator = (const PathString & other)
	{
		return assign(other);
	}
	PathString & append(const PathString & other)
	{
		path.append(other.path);
		return *this;
	}
	PathString & operator += (const PathString & other)
	{
		return append(other);
	}
	friend PathString operator + (const PathString & a, const PathString & b)
	{
		return PathString(a).append(b);
	}
	friend bool operator == (const PathString & a, const PathString & b)
	{
		return a.AsNative() == b.AsNative();
	}
	friend bool operator != (const PathString & a, const PathString & b)
	{
		return a.AsNative() != b.AsNative();
	}
	bool empty() const { return path.empty(); }

public:

#if defined(MODPLUG_TRACKER)

	void SplitPath(PathString *drive, PathString *dir, PathString *fname, PathString *ext) const;
	PathString GetDrive() const;
	PathString GetDir() const;
	PathString GetFileName() const;
	PathString GetFileExt() const;

#endif

public:

#if defined(WIN32)

	// conversions
	MPT_DEPRECATED std::string ToLocale() const { return mpt::String::Encode(path, mpt::CharsetLocale); }
	std::string ToUTF8() const { return mpt::String::Encode(path, mpt::CharsetUTF8); }
	std::wstring ToWide() const { return path; }
	MPT_DEPRECATED static PathString FromLocale(const std::string &path) { return PathString(mpt::String::Decode(path, mpt::CharsetLocale)); }
	static PathString FromUTF8(const std::string &path) { return PathString(mpt::String::Decode(path, mpt::CharsetUTF8)); }
	static PathString FromWide(const std::wstring &path) { return PathString(path); }
	RawPathString AsNative() const { return path; }
	static PathString FromNative(const RawPathString &path) { return PathString(path); }
#if defined(_MFC_VER)
	// CString TCHAR, so this is CHAR or WCHAR, depending on UNICODE
	MPT_DEPRECATED CString ToCString() const
	{
		#ifdef UNICODE
			return path;
		#else
			return mpt::String::Encode(path, mpt::CharsetLocale).c_str();
		#endif
	}
	MPT_DEPRECATED static PathString FromCString(const CString &path)
	{
		#ifdef UNICODE
			return PathString(path.GetString());
		#else
			return PathString(mpt::String::Decode(path.GetString(), mpt::CharsetLocale));
		#endif
	}
#endif

#else // !WIN32

	// conversions
	std::string ToLocale() const { return path; }
	std::string ToUTF8() const { return mpt::String::Convert(path, mpt::CharsetLocale, mpt::CharsetUTF8); }
	std::wstring ToWide() const { return mpt::String::Decode(path, mpt::CharsetLocale); }
	static PathString FromLocale(const std::string &path) { return PathString(path); }
	static PathString FromUTF8(const std::string &path) { return PathString(mpt::String::Convert(path, mpt::CharsetUTF8, mpt::CharsetLocale)); }
	static PathString FromWide(const std::wstring &path) { return PathString(mpt::String::Encode(path, mpt::CharsetLocale)); }
	RawPathString AsNative() const { return path; }
	static PathString FromNative(const RawPathString &path) { return PathString(path); }

#endif // WIN32

};

} // namespace mpt

