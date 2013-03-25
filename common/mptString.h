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

	#ifdef MODPLUG_TRACKER
		String(const CString& str) {(*this) = str.GetString();}
		String& operator=(const CString& str) {return operator=(str.GetString());}
	#endif // MODPLUG_TRACKER

		// To allow easy assignment to CString in GUI side.
		operator const CharT*() const {return c_str();}

		// Clears string.
		void Empty() {clear();}

		// Tests whether string is empty.
		bool IsEmpty() const {return empty();}

		// See std::string::compare.
		int Compare(const CharT* psz) const {return compare(psz);}

		// Equivalent to Empty(); Append(psz, nCount);
		void SetString(const CharT* psz, const size_t nCount)
		{
			Empty();
			Append(psz, nCount);
		}

		// Append string to this.
		void Append(const CharT* psz) {append(psz);}

		// Appends given string to this. Stops after 'nCount' chars if null terminator 
		// hasn't been encountered before that.
		void Append(const CharT* psz, const size_t nCount) {append(psz, nCount);}

		// Appends single character to string.
		void AppendChar(const CharT c) {append(1, c);}

		// Difference between Append and AppendChars is that latter can be used to add 
		// potentially non-null terminated char array.
		template <size_t N>
		void AppendChars(const CharT (&arr)[N])
		{
			append(arr, arr + N);
		}

		// Reserves store for the string without resizing.
		void Reserve(size_t nSize) {reserve(nSize);}

		// Formats this string, like CString::Format.
		void Format(const CharT* pszFormat, ...)
		{
			va_list argList;
			va_start( argList, pszFormat );

			// Count the needed array size.
			const size_t nCount = _vscprintf(pszFormat, argList); // null character not included.
			resize(nCount + 1); // + 1 is for null terminator.

			// Hack: directly modify the std::string's string.
			// In C++11 std::string is guaranteed to be contiguous.
			const int nCount2 = vsprintf_s(&*begin(), size(), pszFormat, argList);
			resize(nCount2); // Removes the null character that vsprintf_s adds.

			va_end( argList );
		}
	};
}
