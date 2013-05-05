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

namespace mpt
{
	// Quick and incomplete string class to be used as a replacement for
	// CString and std::string in soundlib.
	class String : public std::string
	{
	public:
		typedef std::string BaseClass;
		typedef char CharT;

		String() {}
		String(const CharT* psz) : BaseClass(psz) {}
		String(const std::string& other) : BaseClass(other) {}

		// Move constructors and move assignments.
	#if (_MSC_VER >= MSVC_VER_2010)
		String(BaseClass&& str) : BaseClass(std::move(str)) {}
		String(String&& other) : BaseClass(std::move(static_cast<BaseClass&>(other))) {}
		String& operator=(BaseClass&& str) {BaseClass::operator=(std::move(str)); return *this;}
		String& operator=(String&& other) {BaseClass::operator=(std::move(static_cast<BaseClass&>(other))); return *this;}
	#endif

		String& operator=(const CharT* psz) {BaseClass::operator=(psz); return *this;}

	#ifdef _MFC_VER
		String(const CString& str) {(*this) = str.GetString();}
		String& operator=(const CString& str) {return operator=(str.GetString());}
	#endif // _MFC_VER

		// To allow easy assignment to CString in GUI side.
		operator const CharT*() const {return c_str();}

		// Set the string to psz, copy at most nCount characters
		void SetString(const CharT* psz, const size_t nCount)
		{
			clear();
			Append(psz, nCount);
		}

		// Appends given string to this. Stops after 'nCount' chars if null terminator 
		// hasn't been encountered before that.
		void Append(const CharT* psz, const size_t nCount) {append(psz, nCount);}

		// Difference between Append and AppendChars is that latter can be used to add 
		// potentially non-null terminated char array.
		template <size_t N>
		void AppendChars(const CharT (&arr)[N])
		{
			append(arr, arr + N);
		}

		// Formats this string, like CString::Format.
		void Format(const CharT* pszFormat, ...);

		// Remove whitespaces at start of string
		void LTrim();
		// Remove whitespaces at end of string
		void RTrim();
		// Remove whitespaces at start and end of string
		void Trim();
	};
}


inline std::string string_replace(std::string str, const std::string &oldStr, const std::string &newStr)
{
	std::size_t pos = 0;
	while((pos = str.find(oldStr, pos)) != std::string::npos)
	{
		str.replace(pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
	return str;
}
