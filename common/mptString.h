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


} // namespace mpt


static inline int mpt_strnicmp(const char *a, const char *b, size_t count)
{
	#if MPT_COMPILER_MSVC
		return _strnicmp(a, b, count);
	#else
		return strncasecmp(a, b, count);
	#endif
}


#if MPT_COMPILER_MSVC
#define snprintf _snprintf
#endif

