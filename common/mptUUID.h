/*
 * mptUUID.h
 * ---------
 * Purpose: UUID utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#if MPT_OS_WINDOWS


#if defined(MODPLUG_TRACKER) || !defined(NO_DMO)
#include <guiddef.h>
#endif // MODPLUG_TRACKER || !NO_DMO

#if defined(MODPLUG_TRACKER) || !defined(NO_DMO) || defined(MPT_ENABLE_TEMPFILE)
#include <rpc.h>
#endif // MODPLUG_TRACKER || !NO_DMO || MPT_ENABLE_TEMPFILE


OPENMPT_NAMESPACE_BEGIN

namespace Util
{

#if defined(MODPLUG_TRACKER) || !defined(NO_DMO)

// COM CLSID<->string conversion
// A CLSID string is not necessarily a standard UUID string,
// it might also be a symbolic name for the interface.
// (see CLSIDFromString ( http://msdn.microsoft.com/en-us/library/windows/desktop/ms680589%28v=vs.85%29.aspx ))
std::wstring CLSIDToString(CLSID clsid);
CLSID StringToCLSID(const std::wstring &str);
bool VerifyStringToCLSID(const std::wstring &str, CLSID &clsid);
bool IsCLSID(const std::wstring &str);

// COM IID<->string conversion
IID StringToIID(const std::wstring &str);
std::wstring IIDToString(IID iid);

// General GUID<->string conversion.
// The string must/will be in standard GUID format: {4F9A455D-E7EF-4367-B2F0-0C83A38A5C72}
GUID StringToGUID(const std::wstring &str);
std::wstring GUIDToString(GUID guid);

// Create a COM GUID
GUID CreateGUID();

#endif // MODPLUG_TRACKER || !NO_DMO

#if defined(MODPLUG_TRACKER) || !defined(NO_DMO) || defined(MPT_ENABLE_TEMPFILE)

// General UUID<->string conversion.
// The string must/will be in standard UUID format: 4f9a455d-e7ef-4367-b2f0-0c83a38a5c72
UUID StringToUUID(const mpt::ustring &str);
mpt::ustring UUIDToString(UUID uuid);

// Checks the UUID against the NULL UUID. Returns false if it is NULL, true otherwise.
bool IsValid(UUID uuid);

// Create a UUID
UUID CreateUUID();

// Create a UUID that contains local, traceable information. Safe for local use.
UUID CreateLocalUUID();

#endif // MODPLUG_TRACKER || !NO_DMO || MPT_ENABLE_TEMPFILE

} // namespace Util

namespace mpt {

struct UUID
{
public:
	uint32 Data1;
	uint16 Data2;
	uint16 Data3;
	uint64 Data4;
	// xxxxxxxx-xxxx-Mmxx-Nnxx-xxxxxxxxxxxx
	// <--32-->-<16>-<16>-<-------64------>
	bool IsNull() const
	{
		return (Data1 == 0) && (Data2 == 0) && (Data3 == 0) && (Data4 == 0);
	}
	bool IsValid() const
	{
		return (Data1 != 0) || (Data2 != 0) || (Data3 != 0) || (Data4 != 0);
	}
	uint8 Mm() const
	{
		return static_cast<uint8>((Data3 >> 8) & 0xffu);
	}
	uint8 Nn() const
	{
		return static_cast<uint8>((Data4 >> 56) & 0xffu);
	}
	uint8 Variant() const
	{
		return Nn() >> 4u;
	}
	uint8 Version() const
	{
		return Mm() >> 4u;
	}
	bool IsRFC4122() const
	{
		return (Variant() & 0xcu) == 0x8u;
	}
private:
	void MakeRFC4122(uint8 version);
public:
	void ConvertEndianness();
public:
#if MPT_OS_WINDOWS
	explicit UUID(GUID guid)
		: Data1(guid.Data1)
		, Data2(guid.Data2)
		, Data3(guid.Data3)
		, Data4(static_cast<uint64>(0)
			| (static_cast<uint64>(guid.Data4[0]) << 56)
			| (static_cast<uint64>(guid.Data4[1]) << 48)
			| (static_cast<uint64>(guid.Data4[2]) << 40)
			| (static_cast<uint64>(guid.Data4[3]) << 32)
			| (static_cast<uint64>(guid.Data4[4]) << 24)
			| (static_cast<uint64>(guid.Data4[5]) << 16)
			| (static_cast<uint64>(guid.Data4[6]) <<  8)
			| (static_cast<uint64>(guid.Data4[7]) <<  0)
			)
	{
		return;
	}
	operator GUID () const
	{
		GUID retval = {0};
		retval.Data1 = Data1;
		retval.Data2 = Data2;
		retval.Data3 = Data3;
		retval.Data4[0] = static_cast<uint8>(Data4 >> 56);
		retval.Data4[1] = static_cast<uint8>(Data4 >> 48);
		retval.Data4[2] = static_cast<uint8>(Data4 >> 40);
		retval.Data4[3] = static_cast<uint8>(Data4 >> 32);
		retval.Data4[4] = static_cast<uint8>(Data4 >> 24);
		retval.Data4[5] = static_cast<uint8>(Data4 >> 16);
		retval.Data4[6] = static_cast<uint8>(Data4 >>  8);
		retval.Data4[7] = static_cast<uint8>(Data4 >>  0);
		return retval;
	}
#endif // MPT_OS_WINDOWS
public:
	UUID() : Data1(0), Data2(0), Data3(0), Data4(0) { }
	friend bool operator==(const mpt::UUID & a, const mpt::UUID & b)
	{
		return (a.Data1 == b.Data1) && (a.Data2 == b.Data2) && (a.Data3 == b.Data3) && (a.Data4 == b.Data4);
	}
	friend bool operator!=(const mpt::UUID & a, const mpt::UUID & b)
	{
		return (a.Data1 != b.Data1) || (a.Data2 != b.Data2) || (a.Data3 != b.Data3) || (a.Data4 != b.Data4);
	}
public:
	static UUID Generate();
	static UUID GenerateLocalUseOnly();
	static UUID RFC4122Random();
public:
	std::string ToString() const;
	mpt::ustring ToUString() const;
};

STATIC_ASSERT(sizeof(mpt::UUID) == 16);

} // namespace mpt


OPENMPT_NAMESPACE_END


#endif // MPT_OS_WINDOWS
