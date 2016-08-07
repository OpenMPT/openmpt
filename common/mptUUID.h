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

// General UUID<->string conversion.
// The string must/will be in standard UUID format: 4f9a455d-e7ef-4367-b2f0-0c83a38a5c72
UUID StringToUUID(const mpt::ustring &str);
mpt::ustring UUIDToString(UUID uuid);

// Checks the UUID against the NULL UUID. Returns false if it is NULL, true otherwise.
bool IsValid(UUID uuid);

#endif // MODPLUG_TRACKER || !NO_DMO

} // namespace Util

#endif // MPT_OS_WINDOWS

namespace mpt {

struct UUID
{
private:
	uint32 Data1;
	uint16 Data2;
	uint16 Data3;
	uint64 Data4;
public:
	uint32 GetData1() const;
	uint16 GetData2() const;
	uint16 GetData3() const;
	uint64 GetData4() const;
public:
	// xxxxxxxx-xxxx-Mmxx-Nnxx-xxxxxxxxxxxx
	// <--32-->-<16>-<16>-<-------64------>
	bool IsNil() const;
	bool IsValid() const;
	uint8 Variant() const;
	uint8 Version() const;
	bool IsRFC4122() const;
private:
	uint8 Mm() const;
	uint8 Nn() const;
	void MakeRFC4122(uint8 version);
public:
#if MPT_OS_WINDOWS && defined(MODPLUG_TRACKER)
	explicit UUID(::UUID uuid);
	operator ::UUID () const;
#endif // MPT_OS_WINDOWS && MODPLUG_TRACKER
public:
	UUID();
	explicit UUID(uint32 Data1, uint16 Data2, uint16 Data3, uint64 Data4);
	friend bool operator==(const mpt::UUID & a, const mpt::UUID & b);
	friend bool operator!=(const mpt::UUID & a, const mpt::UUID & b);
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
	static UUID FromString(const mpt::ustring &str);
	mpt::ustring ToUString() const;
};

STATIC_ASSERT(sizeof(mpt::UUID) == 16);

bool operator==(const mpt::UUID & a, const mpt::UUID & b);
bool operator!=(const mpt::UUID & a, const mpt::UUID & b);

} // namespace mpt


OPENMPT_NAMESPACE_END
