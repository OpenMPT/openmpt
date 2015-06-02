/*
 * misc_util.cpp
 * -------------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "misc_util.h"

#include <locale>
#include <sstream>


OPENMPT_NAMESPACE_BEGIN


template<typename T>
inline T ConvertStrToHelper(const std::string &str)
{
	std::istringstream i(str);
	i.imbue(std::locale::classic());
	T x;
	if(!(i >> x))
	{
		return T();
	}
	return x;
}
template<> inline bool ConvertStrToHelper(const std::string &str) { return ConvertStrToHelper<int>(str)?true:false; }
template<> inline signed char ConvertStrToHelper(const std::string &str) { return static_cast<signed char>(ConvertStrToHelper<signed int>(str)); }
template<> inline unsigned char ConvertStrToHelper(const std::string &str) { return static_cast<unsigned char>(ConvertStrToHelper<unsigned int>(str)); }

#if MPT_WSTRING_FORMAT
template<typename T>
inline T ConvertStrToHelper(const std::wstring &str)
{
	std::wistringstream i(str);
	i.imbue(std::locale::classic());
	T x;
	if(!(i >> x))
	{
		return T();
	}
	return x;
}
template<> inline bool ConvertStrToHelper(const std::wstring &str) { return ConvertStrToHelper<int>(str)?true:false; }
template<> inline signed char ConvertStrToHelper(const std::wstring &str) { return static_cast<signed char>(ConvertStrToHelper<signed int>(str)); }
template<> inline unsigned char ConvertStrToHelper(const std::wstring &str) { return static_cast<unsigned char>(ConvertStrToHelper<unsigned int>(str)); }
#endif

bool ConvertStrToBool(const std::string &str) { return ConvertStrToHelper<bool>(str); }
signed char ConvertStrToSignedChar(const std::string &str) { return ConvertStrToHelper<signed char>(str); }
unsigned char ConvertStrToUnsignedChar(const std::string &str) { return ConvertStrToHelper<unsigned char>(str); }
signed short ConvertStrToSignedShort(const std::string &str) { return ConvertStrToHelper<signed short>(str); }
unsigned short ConvertStrToUnsignedShort(const std::string &str) { return ConvertStrToHelper<unsigned short>(str); }
signed int ConvertStrToSignedInt(const std::string &str) { return ConvertStrToHelper<signed int>(str); }
unsigned int ConvertStrToUnsignedInt(const std::string &str) { return ConvertStrToHelper<unsigned int>(str); }
signed long ConvertStrToSignedLong(const std::string &str) { return ConvertStrToHelper<signed long>(str); }
unsigned long ConvertStrToUnsignedLong(const std::string &str) { return ConvertStrToHelper<unsigned long>(str); }
signed long long ConvertStrToSignedLongLong(const std::string &str) { return ConvertStrToHelper<signed long long>(str); }
unsigned long long ConvertStrToUnsignedLongLong(const std::string &str) { return ConvertStrToHelper<unsigned long long>(str); }
float ConvertStrToFloat(const std::string &str) { return ConvertStrToHelper<float>(str); }
double ConvertStrToDouble(const std::string &str) { return ConvertStrToHelper<double>(str); }
long double ConvertStrToLongDouble(const std::string &str) { return ConvertStrToHelper<long double>(str); }

#if MPT_WSTRING_FORMAT
bool ConvertStrToBool(const std::wstring &str) { return ConvertStrToHelper<bool>(str); }
signed char ConvertStrToSignedChar(const std::wstring &str) { return ConvertStrToHelper<signed char>(str); }
unsigned char ConvertStrToUnsignedChar(const std::wstring &str) { return ConvertStrToHelper<unsigned char>(str); }
signed short ConvertStrToSignedShort(const std::wstring &str) { return ConvertStrToHelper<signed short>(str); }
unsigned short ConvertStrToUnsignedShort(const std::wstring &str) { return ConvertStrToHelper<unsigned short>(str); }
signed int ConvertStrToSignedInt(const std::wstring &str) { return ConvertStrToHelper<signed int>(str); }
unsigned int ConvertStrToUnsignedInt(const std::wstring &str) { return ConvertStrToHelper<unsigned int>(str); }
signed long ConvertStrToSignedLong(const std::wstring &str) { return ConvertStrToHelper<signed long>(str); }
unsigned long ConvertStrToUnsignedLong(const std::wstring &str) { return ConvertStrToHelper<unsigned long>(str); }
signed long long ConvertStrToSignedLongLong(const std::wstring &str) { return ConvertStrToHelper<signed long long>(str); }
unsigned long long ConvertStrToUnsignedLongLong(const std::wstring &str) { return ConvertStrToHelper<unsigned long long>(str); }
float ConvertStrToFloat(const std::wstring &str) { return ConvertStrToHelper<float>(str); }
double ConvertStrToDouble(const std::wstring &str) { return ConvertStrToHelper<double>(str); }
long double ConvertStrToLongDouble(const std::wstring &str) { return ConvertStrToHelper<long double>(str); }
#endif




#ifdef MODPLUG_TRACKER

namespace Util
{


#if MPT_OS_WINDOWS

mpt::PathString GetTempDirectory()
//--------------------------------
{
	WCHAR tempPath[MAX_PATH+2];
	MemsetZero(tempPath);
	DWORD result = GetTempPathW(MAX_PATH+1, tempPath);
	if(result == 0 || result > MAX_PATH+1)
	{ // error
		// use app directory as fallback
		return mpt::GetAppPath();
	}
	return mpt::PathString::FromNative(tempPath);
}


mpt::PathString CreateTempFileName(const mpt::PathString &fileNamePrefix, const mpt::PathString &fileNameExtension)
//-----------------------------------------------------------------------------------------------------------------
{
	mpt::PathString filename = Util::GetTempDirectory();
	filename += (!fileNamePrefix.empty() ? fileNamePrefix + MPT_PATHSTRING("_") : mpt::PathString());
	filename += mpt::PathString::FromWide(Util::UUIDToString(Util::CreateLocalUUID()));
	filename += (!fileNameExtension.empty() ? MPT_PATHSTRING(".") + fileNameExtension : mpt::PathString());
	return filename;
}


#endif // MPT_OS_WINDOWS


} // namespace Util

#endif // MODPLUG_TRACKER


namespace Util
{


static const MPT_UCHAR_TYPE EncodeNibble[16] = {
	MPT_UCHAR('0'), MPT_UCHAR('1'), MPT_UCHAR('2'), MPT_UCHAR('3'),
	MPT_UCHAR('4'), MPT_UCHAR('5'), MPT_UCHAR('6'), MPT_UCHAR('7'),
	MPT_UCHAR('8'), MPT_UCHAR('9'), MPT_UCHAR('A'), MPT_UCHAR('B'),
	MPT_UCHAR('C'), MPT_UCHAR('D'), MPT_UCHAR('E'), MPT_UCHAR('F') };

static inline bool DecodeByte(uint8 &byte, MPT_UCHAR_TYPE c1, MPT_UCHAR_TYPE c2)
{
	byte = 0;
	if(MPT_UCHAR('0') <= c1 && c1 <= MPT_UCHAR('9'))
	{
		byte += static_cast<uint8>((c1 - MPT_UCHAR('0')) << 4);
	} else if(MPT_UCHAR('A') <= c1 && c1 <= MPT_UCHAR('F'))
	{
		byte += static_cast<uint8>((c1 - MPT_UCHAR('A') + 10) << 4);
	} else if(MPT_UCHAR('a') <= c1 && c1 <= MPT_UCHAR('f'))
	{
		byte += static_cast<uint8>((c1 - MPT_UCHAR('a') + 10) << 4);
	} else
	{
		return false;
	}
	if(MPT_UCHAR('0') <= c2 && c2 <= MPT_UCHAR('9'))
	{
		byte += static_cast<uint8>(c2 - MPT_UCHAR('0'));
	} else if(MPT_UCHAR('A') <= c2 && c2 <= MPT_UCHAR('F'))
	{
		byte += static_cast<uint8>(c2 - MPT_UCHAR('A') + 10);
	} else if(MPT_UCHAR('a') <= c2 && c2 <= MPT_UCHAR('f'))
	{
		byte += static_cast<uint8>(c2 - MPT_UCHAR('a') + 10);
	} else
	{
		return false;
	}
	return true;
}

mpt::ustring BinToHex(const std::vector<char> &src)
{
	mpt::ustring result;
	for(std::size_t i = 0; i < src.size(); ++i)
	{
		uint8 byte = src[i];
		result.push_back(EncodeNibble[(byte&0xf0)>>4]);
		result.push_back(EncodeNibble[byte&0x0f]);
	}
	return result;
}

std::vector<char> HexToBin(const mpt::ustring &src)
{
	std::vector<char> result;
	for(std::size_t i = 0; (i + 1) < src.size(); i += 2)
	{
		uint8 byte = 0;
		if(!DecodeByte(byte, src[i], src[i + 1]))
		{
			return result;
		}
		result.push_back(byte);
	}
	return result;
}


} // namespace Util


OPENMPT_NAMESPACE_END
