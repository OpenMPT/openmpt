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

#include "mpt/binary/hex.hpp"
#include "mpt/binary/base64.hpp"
#include "mpt/binary/base64url.hpp"

OPENMPT_NAMESPACE_BEGIN

namespace Util
{

mpt::ustring BinToHex(mpt::const_byte_span src)
{
	return mpt::encode_hex(src);
}

std::vector<std::byte> HexToBin(const mpt::ustring &src)
{
	return mpt::decode_hex(src);
}

mpt::ustring BinToBase64url(mpt::const_byte_span src)
{
	return mpt::encode_base64url(src);
}

std::vector<std::byte> Base64urlToBin(const mpt::ustring &src)
{
	return mpt::decode_base64url(src);
}

mpt::ustring BinToBase64(mpt::const_byte_span src)
{
	return mpt::encode_base64(src);
}

std::vector<std::byte> Base64ToBin(const mpt::ustring &src)
{
	return mpt::decode_base64(src);
}

} // namespace Util

OPENMPT_NAMESPACE_END
