/*
 * mptUUID.cpp
 * -----------
 * Purpose: UUID utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptUUID.h"

#include "mpt/uuid/guid.hpp"
#include "mpt/uuid/uuid.hpp"


OPENMPT_NAMESPACE_BEGIN


#if MPT_OS_WINDOWS


namespace Util
{


#if defined(MODPLUG_TRACKER) || defined(MPT_WITH_DMO)

mpt::winstring CLSIDToString(CLSID clsid)
{
	return mpt::CLSIDToString(clsid);
}

CLSID StringToCLSID(const mpt::winstring& str_)
{
	return mpt::StringToCLSID(str_);
}

bool VerifyStringToCLSID(const mpt::winstring &str_, CLSID &clsid)
{
	return mpt::VerifyStringToCLSID(str_, clsid);
}

bool IsCLSID(const mpt::winstring &str_)
{
	return mpt::IsCLSID(str_);
}


mpt::winstring IIDToString(IID iid)
{
	return mpt::IIDToString(iid);
}

IID StringToIID(const mpt::winstring &str_)
{
	return mpt::StringToIID(str_);
}


mpt::winstring GUIDToString(GUID guid)
{
	return mpt::GUIDToString(guid);
}

GUID StringToGUID(const mpt::winstring &str)
{
	return mpt::StringToGUID(str);
}


GUID CreateGUID()
{
	return mpt::CreateGUID();
}


bool IsValid(UUID uuid)
{
	return mpt::IsValid(uuid);
}


#endif // MODPLUG_TRACKER || MPT_WITH_DMO


} // namespace Util


#endif // MPT_OS_WINDOWS


OPENMPT_NAMESPACE_END
