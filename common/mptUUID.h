/*
 * mptUUID.h
 * ---------
 * Purpose: UUID utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "mpt/uuid/uuid.hpp"

#include "Endianness.h"

#include <stdexcept>

#if MPT_OS_WINDOWS
#if defined(MODPLUG_TRACKER) || defined(MPT_WITH_DMO)
#include <guiddef.h>
#include <rpc.h>
#endif // MODPLUG_TRACKER || MPT_WITH_DMO
#endif // MPT_OS_WINDOWS


OPENMPT_NAMESPACE_BEGIN

#if MPT_OS_WINDOWS

namespace Util
{

#if defined(MODPLUG_TRACKER) || defined(MPT_WITH_DMO)

// COM CLSID<->string conversion
// A CLSID string is not necessarily a standard UUID string,
// it might also be a symbolic name for the interface.
// (see CLSIDFromString ( http://msdn.microsoft.com/en-us/library/windows/desktop/ms680589%28v=vs.85%29.aspx ))
mpt::winstring CLSIDToString(CLSID clsid);
CLSID StringToCLSID(const mpt::winstring &str);
bool VerifyStringToCLSID(const mpt::winstring &str, CLSID &clsid);
bool IsCLSID(const mpt::winstring &str);

// COM IID<->string conversion
IID StringToIID(const mpt::winstring &str);
mpt::winstring IIDToString(IID iid);

// General GUID<->string conversion.
// The string must/will be in standard GUID format: {4F9A455D-E7EF-4367-B2F0-0C83A38A5C72}
GUID StringToGUID(const mpt::winstring &str);
mpt::winstring GUIDToString(GUID guid);

// Create a COM GUID
GUID CreateGUID();

// Checks the UUID against the NULL UUID. Returns false if it is NULL, true otherwise.
bool IsValid(UUID uuid);

#endif // MODPLUG_TRACKER || MPT_WITH_DMO

} // namespace Util

#endif // MPT_OS_WINDOWS


using namespace mpt::uuid_literals;


OPENMPT_NAMESPACE_END
