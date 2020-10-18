/*
 * mptUUIDNamespace.cpp
 * --------------------
 * Purpose: UUID utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptUUIDNamespace.h"

#include "../common/mptUUID.h"
#include "../misc/mptCrypto.h"


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{


UUID UUIDRFC4122NamespaceV3(const UUID &ns, const mpt::ustring &name)
{
	UUIDbin binns = ns;
	std::vector<std::byte> buf;
	buf.resize(sizeof(UUIDbin));
	std::copy(mpt::as_raw_memory(binns).data(), mpt::as_raw_memory(binns).data() + sizeof(UUIDbin), buf.data());
	std::string utf8name = mpt::ToCharset(mpt::Charset::UTF8, name);
	buf.resize(buf.size() + utf8name.length());
	std::transform(utf8name.begin(), utf8name.end(), buf.data() + sizeof(UUIDbin), [](char c){ return mpt::byte_cast<std::byte>(c); });
	std::array<std::byte, 16> hash = mpt::crypto::hash::MD5().process(mpt::as_span(buf)).result();
	UUIDbin uuidbin;
	std::copy(hash.begin(), hash.begin() + 16, mpt::as_raw_memory(uuidbin).data());
	UUID uuid{uuidbin};
	uuid.MakeRFC4122(3);
	return uuid;
}

UUID UUIDRFC4122NamespaceV5(const UUID &ns, const mpt::ustring &name)
{
	UUIDbin binns = ns;
	std::vector<std::byte> buf;
	buf.resize(sizeof(UUIDbin));
	std::copy(mpt::as_raw_memory(binns).data(), mpt::as_raw_memory(binns).data() + sizeof(UUIDbin), buf.data());
	std::string utf8name = mpt::ToCharset(mpt::Charset::UTF8, name);
	buf.resize(buf.size() + utf8name.length());
	std::transform(utf8name.begin(), utf8name.end(), buf.data() + sizeof(UUIDbin), [](char c){ return mpt::byte_cast<std::byte>(c); });
	std::array<std::byte, 20> hash = mpt::crypto::hash::SHA1().process(mpt::as_span(buf)).result();
	UUIDbin uuidbin;
	std::copy(hash.begin(), hash.begin() + 16, mpt::as_raw_memory(uuidbin).data());
	UUID uuid{uuidbin};
	uuid.MakeRFC4122(5);
	return uuid;
}


} // namespace mpt


OPENMPT_NAMESPACE_END
