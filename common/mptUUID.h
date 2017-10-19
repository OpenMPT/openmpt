/*
 * mptUUID.h
 * ---------
 * Purpose: UUID utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#include "Endianness.h"

#if MPT_OS_WINDOWS
#if defined(MODPLUG_TRACKER) || !defined(NO_DMO)
#include <guiddef.h>
#include <rpc.h>
#endif // MODPLUG_TRACKER || !NO_DMO
#endif // MPT_OS_WINDOWS


OPENMPT_NAMESPACE_BEGIN

#if MPT_OS_WINDOWS

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

#if !MPT_OS_WINDOWS_WINRT
// General UUID<->string conversion.
// The string must/will be in standard UUID format: 4f9a455d-e7ef-4367-b2f0-0c83a38a5c72
UUID StringToUUID(const mpt::ustring &str);
mpt::ustring UUIDToString(UUID uuid);
#endif // !MPT_OS_WINDOWS_WINRT

// Checks the UUID against the NULL UUID. Returns false if it is NULL, true otherwise.
bool IsValid(UUID uuid);

#endif // MODPLUG_TRACKER || !NO_DMO

} // namespace Util

#endif // MPT_OS_WINDOWS

// Microsoft on-disk layout
struct GUIDms
{
	uint32le Data1;
	uint16le Data2;
	uint16le Data3;
	uint64be Data4; // yes, big endian here
};
MPT_BINARY_STRUCT(GUIDms, 16)

// RFC binary format
struct UUIDbin
{
	uint32be Data1;
	uint16be Data2;
	uint16be Data3;
	uint64be Data4;
};
MPT_BINARY_STRUCT(UUIDbin, 16)

namespace mpt {

struct UUID
{
private:
	uint32 Data1;
	uint16 Data2;
	uint16 Data3;
	uint64 Data4;
public:
	MPT_CONSTEXPR11_FUN uint32 GetData1() const noexcept { return Data1; }
	MPT_CONSTEXPR11_FUN uint16 GetData2() const noexcept { return Data2; }
	MPT_CONSTEXPR11_FUN uint16 GetData3() const noexcept { return Data3; }
	MPT_CONSTEXPR11_FUN uint64 GetData4() const noexcept { return Data4; }
public:
	// xxxxxxxx-xxxx-Mmxx-Nnxx-xxxxxxxxxxxx
	// <--32-->-<16>-<16>-<-------64------>
	MPT_CONSTEXPR11_FUN bool IsNil() const noexcept { return (Data1 == 0) && (Data2 == 0) && (Data3 == 0) && (Data4 == 0); }
	MPT_CONSTEXPR11_FUN bool IsValid() const noexcept { return (Data1 != 0) || (Data2 != 0) || (Data3 != 0) || (Data4 != 0); }
	MPT_CONSTEXPR11_FUN uint8 Variant() const noexcept { return Nn() >> 4u; }
	MPT_CONSTEXPR11_FUN uint8 Version() const noexcept { return Mm() >> 4u; }
	MPT_CONSTEXPR11_FUN bool IsRFC4122() const noexcept { return (Variant() & 0xcu) == 0x8u; }
private:
	MPT_CONSTEXPR11_FUN uint8 Mm() const noexcept { return static_cast<uint8>((Data3 >> 8) & 0xffu); }
	MPT_CONSTEXPR11_FUN uint8 Nn() const noexcept { return static_cast<uint8>((Data4 >> 56) & 0xffu); }
	void MakeRFC4122(uint8 version) noexcept;
public:
#if MPT_OS_WINDOWS && (defined(MODPLUG_TRACKER) || !defined(NO_DMO))
	explicit UUID(::UUID uuid);
	operator ::UUID () const;
#endif // MPT_OS_WINDOWS && (MODPLUG_TRACKER || !NO_DMO)
	static MPT_CONSTEXPR11_FUN UUID FromGroups(uint32 group1, uint16 group2, uint16 group3, uint16 group4, uint64 group5) noexcept
	{
		//MPT_ASSERT((group5 & 0xffff000000000000ull) == 0ull);
		return mpt::UUID
			( group1
			, group2
			, group3
			, (static_cast<uint64>(group4) << 48) ^ group5
			);
	}
	#define MPT_UUID_HELPER( prefix , value , suffix ) ( prefix ## value ## suffix )
	#define MPT_UUID(group1, group2, group3, group4, group5) mpt::UUID::FromGroups(MPT_UUID_HELPER(0x,group1,u), MPT_UUID_HELPER(0x,group2,u), MPT_UUID_HELPER(0x,group3,u), MPT_UUID_HELPER(0x,group4,u), MPT_UUID_HELPER(0x,group5,ull))
public:
	MPT_CONSTEXPR11_FUN UUID() noexcept : Data1(0), Data2(0), Data3(0), Data4(0) { }
	MPT_CONSTEXPR11_FUN explicit UUID(uint32 Data1, uint16 Data2, uint16 Data3, uint64 Data4) noexcept : Data1(Data1), Data2(Data2), Data3(Data3), Data4(Data4) { }
	explicit UUID(UUIDbin uuid);
	explicit UUID(GUIDms guid);
	operator UUIDbin () const;
	operator GUIDms () const;
public:
	// Create a UUID
	static UUID Generate();
	// Create a UUID that contains local, traceable information.
	// Safe for local use. May be faster.
	static UUID GenerateLocalUseOnly();
	// Create a RFC4122 Random UUID.
	static UUID RFC4122Random();
public:
	// General UUID<->string conversion.
	// The string must/will be in standard UUID format: 4f9a455d-e7ef-4367-b2f0-0c83a38a5c72
	static UUID FromString(const std::string &str);
	static UUID FromString(const mpt::ustring &str);
	std::string ToString() const;
	mpt::ustring ToUString() const;
};

MPT_CONSTEXPR11_FUN bool operator==(const mpt::UUID & a, const mpt::UUID & b) noexcept
{
	return (a.GetData1() == b.GetData1()) && (a.GetData2() == b.GetData2()) && (a.GetData3() == b.GetData3()) && (a.GetData4() == b.GetData4());
}
MPT_CONSTEXPR11_FUN bool operator!=(const mpt::UUID & a, const mpt::UUID & b) noexcept
{
	return (a.GetData1() != b.GetData1()) || (a.GetData2() != b.GetData2()) || (a.GetData3() != b.GetData3()) || (a.GetData4() != b.GetData4());	
}

} // namespace mpt


OPENMPT_NAMESPACE_END
