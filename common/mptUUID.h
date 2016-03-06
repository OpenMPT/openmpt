/*
 * mptUUID.h
 * ---------
 * Purpose: UUID utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#if (defined(MODPLUG_TRACKER) || !defined(NO_DMO)) && MPT_OS_WINDOWS

#include <guiddef.h>
#include <rpc.h>

OPENMPT_NAMESPACE_BEGIN

namespace Util
{

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

// General UUID<->string conversion.
// The string must/will be in standard UUID format: 4f9a455d-e7ef-4367-b2f0-0c83a38a5c72
UUID StringToUUID(const std::wstring &str);
std::wstring UUIDToString(UUID uuid);

// Checks the UUID against the NULL UUID. Returns false if it is NULL, true otherwise.
bool IsValid(UUID uuid);

// Create a COM GUID
GUID CreateGUID();

// Create a UUID
UUID CreateUUID();

// Create a UUID that contains local, traceable information. Safe for local use.
UUID CreateLocalUUID();

} // namespace Util

OPENMPT_NAMESPACE_END

#endif // (defined(MODPLUG_TRACKER) || !defined(NO_DMO)) && MPT_OS_WINDOWS
