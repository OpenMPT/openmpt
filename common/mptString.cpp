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


namespace mpt {

// Formats this string, like CString::Format.
void String::Format(const CharT* pszFormat, ...)
{
	#ifdef _MSC_VER
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
	#else
		va_list argList;
		va_start(argList, pszFormat);
		int size = vsnprintf(NULL, 0, pszFormat, argList); // get required size, requires c99 compliant vsnprintf which msvc does not have
		va_end(argList);
		std::vector<char> temp(size + 1);
		va_start(argList, pszFormat);
		vsnprintf(&(temp[0]), size + 1, pszFormat, argList);
		va_end(argList);
		assign(&(temp[0]));
	#endif
}

} // namespace mpt
