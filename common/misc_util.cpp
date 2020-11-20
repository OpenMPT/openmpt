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

#include "mptOSError.h"

#if MPT_OS_WINDOWS
#include <windows.h>
#endif


OPENMPT_NAMESPACE_BEGIN



namespace Util
{


static constexpr mpt::uchar EncodeNibble[16] = {
	UC_('0'), UC_('1'), UC_('2'), UC_('3'),
	UC_('4'), UC_('5'), UC_('6'), UC_('7'),
	UC_('8'), UC_('9'), UC_('A'), UC_('B'),
	UC_('C'), UC_('D'), UC_('E'), UC_('F') };

static inline bool DecodeByte(uint8 &byte, mpt::uchar c1, mpt::uchar c2)
{
	byte = 0;
	if(UC_('0') <= c1 && c1 <= UC_('9'))
	{
		byte += static_cast<uint8>((c1 - UC_('0')) << 4);
	} else if(UC_('A') <= c1 && c1 <= UC_('F'))
	{
		byte += static_cast<uint8>((c1 - UC_('A') + 10) << 4);
	} else if(UC_('a') <= c1 && c1 <= UC_('f'))
	{
		byte += static_cast<uint8>((c1 - UC_('a') + 10) << 4);
	} else
	{
		return false;
	}
	if(UC_('0') <= c2 && c2 <= UC_('9'))
	{
		byte += static_cast<uint8>(c2 - UC_('0'));
	} else if(UC_('A') <= c2 && c2 <= UC_('F'))
	{
		byte += static_cast<uint8>(c2 - UC_('A') + 10);
	} else if(UC_('a') <= c2 && c2 <= UC_('f'))
	{
		byte += static_cast<uint8>(c2 - UC_('a') + 10);
	} else
	{
		return false;
	}
	return true;
}

mpt::ustring BinToHex(mpt::const_byte_span src)
{
	mpt::ustring result;
	result.reserve(src.size() * 2);
	for(std::byte byte : src)
	{
		result.push_back(EncodeNibble[(mpt::byte_cast<uint8>(byte) & 0xf0) >> 4]);
		result.push_back(EncodeNibble[mpt::byte_cast<uint8>(byte) & 0x0f]);
	}
	return result;
}

std::vector<std::byte> HexToBin(const mpt::ustring &src)
{
	std::vector<std::byte> result;
	result.reserve(src.size() / 2);
	for(std::size_t i = 0; (i + 1) < src.size(); i += 2)
	{
		uint8 byte = 0;
		if(!DecodeByte(byte, src[i], src[i + 1]))
		{
			return result;
		}
		result.push_back(mpt::byte_cast<std::byte>(byte));
	}
	return result;
}



static constexpr mpt::uchar base64url[64] = {
	UC_('A'),UC_('B'),UC_('C'),UC_('D'),UC_('E'),UC_('F'),UC_('G'),UC_('H'),UC_('I'),UC_('J'),UC_('K'),UC_('L'),UC_('M'),UC_('N'),UC_('O'),UC_('P'),
	UC_('Q'),UC_('R'),UC_('S'),UC_('T'),UC_('U'),UC_('V'),UC_('W'),UC_('X'),UC_('Y'),UC_('Z'),UC_('a'),UC_('b'),UC_('c'),UC_('d'),UC_('e'),UC_('f'),
	UC_('g'),UC_('h'),UC_('i'),UC_('j'),UC_('k'),UC_('l'),UC_('m'),UC_('n'),UC_('o'),UC_('p'),UC_('q'),UC_('r'),UC_('s'),UC_('t'),UC_('u'),UC_('v'),
	UC_('w'),UC_('x'),UC_('y'),UC_('z'),UC_('0'),UC_('1'),UC_('2'),UC_('3'),UC_('4'),UC_('5'),UC_('6'),UC_('7'),UC_('8'),UC_('9'),UC_('-'),UC_('_')
};

mpt::ustring BinToBase64url(mpt::const_byte_span src)
{
	mpt::ustring result;
	result.reserve(4 * ((src.size() + 2) / 3));
	uint32 bits = 0;
	std::size_t bytes = 0;
	for(std::byte byte : src)
	{
		bits <<= 8;
		bits |= mpt::byte_cast<uint8>(byte);
		bytes++;
		if(bytes == 3)
		{
			result.push_back(base64url[(bits >> 18) & 0x3f]);
			result.push_back(base64url[(bits >> 12) & 0x3f]);
			result.push_back(base64url[(bits >> 6) & 0x3f]);
			result.push_back(base64url[(bits >> 0) & 0x3f]);
			bits = 0;
			bytes = 0;
		}
	}
	std::size_t padding = 0;
	while(bytes != 0)
	{
		bits <<= 8;
		padding++;
		bytes++;
		if(bytes == 3)
		{
			result.push_back(base64url[(bits >> 18) & 0x3f]);
			result.push_back(base64url[(bits >> 12) & 0x3f]);
			if(padding <= 1)
			{
				result.push_back(base64url[(bits >> 6) & 0x3f]);
			}
			if(padding <= 0)
			{
				result.push_back(base64url[(bits >> 0) & 0x3f]);
			}
			bits = 0;
			bytes = 0;
		}
	}
	return result;
}

static uint8 GetBase64urlBits(mpt::uchar c)
{
	for(uint8 i = 0; i < 64; ++i)
	{
		if(base64url[i] == c)
		{
			return i;
		}
	}
	throw base64_parse_error();
}

std::vector<std::byte> Base64urlToBin(const mpt::ustring &src)
{
	std::vector<std::byte> result;
	result.reserve(3 * ((src.length() + 2) / 4));
	uint32 bits = 0;
	std::size_t chars = 0;
	for(mpt::uchar c : src)
	{
		bits <<= 6;
		bits |= GetBase64urlBits(c);
		chars++;
		if(chars == 4)
		{
			result.push_back(mpt::byte_cast<std::byte>(static_cast<uint8>((bits >> 16) & 0xff)));
			result.push_back(mpt::byte_cast<std::byte>(static_cast<uint8>((bits >> 8) & 0xff)));
			result.push_back(mpt::byte_cast<std::byte>(static_cast<uint8>((bits >> 0) & 0xff)));
			bits = 0;
			chars = 0;
		}
	}
	uint32 padding = 0;
	if(chars != 0 && chars < 2)
	{
		throw base64_parse_error();
	}
	while(chars != 0)
	{
		bits <<= 6;
		padding++;
		chars++;
		if(chars == 4)
		{
			result.push_back(mpt::byte_cast<std::byte>(static_cast<uint8>((bits >> 16) & 0xff)));
			if(padding < 2)
			{
				result.push_back(mpt::byte_cast<std::byte>(static_cast<uint8>((bits >> 8) & 0xff)));
			}
			if(padding < 1)
			{
				result.push_back(mpt::byte_cast<std::byte>(static_cast<uint8>((bits >> 0) & 0xff)));
			}
			bits = 0;
			chars = 0;
			padding = 0;
		}
	}
	return result;
}



static constexpr mpt::uchar base64[64] = {
	UC_('A'),UC_('B'),UC_('C'),UC_('D'),UC_('E'),UC_('F'),UC_('G'),UC_('H'),UC_('I'),UC_('J'),UC_('K'),UC_('L'),UC_('M'),UC_('N'),UC_('O'),UC_('P'),
	UC_('Q'),UC_('R'),UC_('S'),UC_('T'),UC_('U'),UC_('V'),UC_('W'),UC_('X'),UC_('Y'),UC_('Z'),UC_('a'),UC_('b'),UC_('c'),UC_('d'),UC_('e'),UC_('f'),
	UC_('g'),UC_('h'),UC_('i'),UC_('j'),UC_('k'),UC_('l'),UC_('m'),UC_('n'),UC_('o'),UC_('p'),UC_('q'),UC_('r'),UC_('s'),UC_('t'),UC_('u'),UC_('v'),
	UC_('w'),UC_('x'),UC_('y'),UC_('z'),UC_('0'),UC_('1'),UC_('2'),UC_('3'),UC_('4'),UC_('5'),UC_('6'),UC_('7'),UC_('8'),UC_('9'),UC_('+'),UC_('/')
};

mpt::ustring BinToBase64(mpt::const_byte_span src)
{
	mpt::ustring result;
	result.reserve(4 * ((src.size() + 2) / 3));
	uint32 bits = 0;
	std::size_t bytes = 0;
	for(std::byte byte : src)
	{
		bits <<= 8;
		bits |= mpt::byte_cast<uint8>(byte);
		bytes++;
		if(bytes == 3)
		{
			result.push_back(base64[(bits >> 18) & 0x3f]);
			result.push_back(base64[(bits >> 12) & 0x3f]);
			result.push_back(base64[(bits >> 6) & 0x3f]);
			result.push_back(base64[(bits >> 0) & 0x3f]);
			bits = 0;
			bytes = 0;
		}
	}
	std::size_t padding = 0;
	while(bytes != 0)
	{
		bits <<= 8;
		padding++;
		bytes++;
		if(bytes == 3)
		{
			result.push_back(base64[(bits >> 18) & 0x3f]);
			result.push_back(base64[(bits >> 12) & 0x3f]);
			if(padding > 1)
			{
				result.push_back(UC_('='));
			} else
			{
				result.push_back(base64[(bits >> 6) & 0x3f]);
			}
			if(padding > 0)
			{
				result.push_back(UC_('='));
			} else
			{
				result.push_back(base64[(bits >> 0) & 0x3f]);
			}
			bits = 0;
			bytes = 0;
		}
	}
	return result;
}

static uint8 GetBase64Bits(mpt::uchar c)
{
	for(uint8 i = 0; i < 64; ++i)
	{
		if(base64[i] == c)
		{
			return i;
		}
	}
	throw base64_parse_error();
}

std::vector<std::byte> Base64ToBin(const mpt::ustring &src)
{
	std::vector<std::byte> result;
	result.reserve(3 * (src.length() / 4));
	uint32 bits = 0;
	std::size_t chars = 0;
	std::size_t padding = 0;
	for(mpt::uchar c : src)
	{
		bits <<= 6;
		if(c == UC_('='))
		{
			padding++;
		} else
		{
			bits |= GetBase64Bits(c);
		}
		chars++;
		if(chars == 4)
		{
			result.push_back(mpt::byte_cast<std::byte>(static_cast<uint8>((bits >> 16) & 0xff)));
			if(padding < 2)
			{
				result.push_back(mpt::byte_cast<std::byte>(static_cast<uint8>((bits >> 8) & 0xff)));
			}
			if(padding < 1)
			{
				result.push_back(mpt::byte_cast<std::byte>(static_cast<uint8>((bits >> 0) & 0xff)));
			}
			bits = 0;
			chars = 0;
			padding = 0;
		}
	}
	if(chars != 0)
	{
		throw base64_parse_error();
	}
	return result;
}


} // namespace Util


#if defined(MODPLUG_TRACKER) || (defined(LIBOPENMPT_BUILD) && defined(LIBOPENMPT_BUILD_TEST))

namespace mpt
{

std::optional<mpt::ustring> getenv(const mpt::ustring &env_var)
{
	#if MPT_OS_WINDOWS && MPT_OS_WINDOWS_WINRT
		MPT_UNREFERENCED_PARAMETER(env_var);
		return std::nullopt;
	#elif MPT_OS_WINDOWS && defined(UNICODE)
		std::vector<WCHAR> buf(32767);
		DWORD size = GetEnvironmentVariable(mpt::ToWide(env_var).c_str(), buf.data(), 32767);
		if(size == 0)
		{
			mpt::Windows::ExpectError(ERROR_ENVVAR_NOT_FOUND);
			return std::nullopt;
		}
		return mpt::ToUnicode(buf.data());
	#else
		const char *val = std::getenv(mpt::ToCharset(mpt::CharsetEnvironment, env_var).c_str());
		if(!val)
		{
			return std::nullopt;
		}
		return mpt::ToUnicode(mpt::CharsetEnvironment, val);
	#endif
}

} // namespace mpt

#endif // MODPLUG_TRACKER || (LIBOPENMPT_BUILD && LIBOPENMPT_BUILD_TEST)


OPENMPT_NAMESPACE_END
