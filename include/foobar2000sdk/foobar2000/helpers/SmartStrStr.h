#pragma once


#include <set>
#include <map>
#include <string>

//! Implementation of string matching for search purposes, such as media library search or typefind in list views. \n
//! Inspired by Unicode asymetic search, but not strictly implementing the Unicode asymetric search specifications. \n
//! Bootstraps its character mapping data from various Win32 API methods, requires no externally provided character mapping data. \n
//! Windows-only code. \n
//! \n
//! Keeping a global instance of it is recommended, due to one time init overhead. \n
//! Thread safety: safe to call concurrently once constructed.

class SmartStrStr {
public:
	SmartStrStr();

	//! Returns ptr to the end of the string if positive (for continuing search), nullptr if negative.
	const char * strStrEnd(const char * pString, const char * pSubString, size_t * outFoundAt = nullptr) const;
	const wchar_t * strStrEndW(const wchar_t * pString, const wchar_t * pSubString, size_t * outFoundAt = nullptr) const;
	//! Returns ptr to the end of the string if positive (for continuing search), nullptr if negative.
	const char * matchHere(const char * pString, const char * pUserString) const;
	const wchar_t * matchHereW( const wchar_t * pString, const wchar_t * pUserString) const;

	//! One-char match. Doesn't use twoCharMappings, use only if you have to operate on char by char basis rather than call the other methods.
	bool matchOneChar(uint32_t cInput, uint32_t cData) const;
private:

	static uint32_t Transform(uint32_t c);
	static uint32_t ToLower(uint32_t c);

	void ImportTwoCharMappings(const wchar_t * list, const char * replacement);
	void InitTwoCharMappings();

	std::map<uint32_t, std::set<uint32_t> > substituions;
	std::map<uint32_t, const char* > twoCharMappings;
};
