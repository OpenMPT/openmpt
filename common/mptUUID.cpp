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

#if (defined(MODPLUG_TRACKER) || !defined(NO_DMO)) && MPT_OS_WINDOWS
#include <windows.h>
#include <objbase.h>
#endif


OPENMPT_NAMESPACE_BEGIN


#if (defined(MODPLUG_TRACKER) || !defined(NO_DMO)) && MPT_OS_WINDOWS


namespace Util
{


std::wstring CLSIDToString(CLSID clsid)
//-------------------------------------
{
	std::wstring str;
	LPOLESTR tmp = nullptr;
	::StringFromCLSID(clsid, &tmp);
	if(tmp)
	{
		str = tmp;
		::CoTaskMemFree(tmp);
		tmp = nullptr;
	}
	return str;
}


CLSID StringToCLSID(const std::wstring &str)
//------------------------------------------
{
	CLSID clsid = CLSID();
	std::vector<OLECHAR> tmp(str.c_str(), str.c_str() + str.length() + 1);
	if(::CLSIDFromString(&tmp[0], &clsid) != S_OK)
	{
		return CLSID();
	}
	return clsid;
}


bool VerifyStringToCLSID(const std::wstring &str, CLSID &clsid)
//-------------------------------------------------------------
{
	std::vector<OLECHAR> tmp(str.c_str(), str.c_str() + str.length() + 1);
	return (::CLSIDFromString(&tmp[0], &clsid) == S_OK);
}


bool IsCLSID(const std::wstring &str)
//-----------------------------------
{
	CLSID clsid = CLSID();
	std::vector<OLECHAR> tmp(str.c_str(), str.c_str() + str.length() + 1);
	return (::CLSIDFromString(&tmp[0], &clsid) == S_OK);
}


std::wstring IIDToString(IID iid)
//-------------------------------
{
	std::wstring str;
	LPOLESTR tmp = nullptr;
	::StringFromIID(iid, &tmp);
	if(tmp)
	{
		str = tmp;
		::CoTaskMemFree(tmp);
		tmp = nullptr;
	}
	return str;
}


IID StringToIID(const std::wstring &str)
//--------------------------------------
{
	IID iid = IID();
	std::vector<OLECHAR> tmp(str.c_str(), str.c_str() + str.length() + 1);
	::IIDFromString(&tmp[0], &iid);
	return iid;
}


std::wstring GUIDToString(GUID guid)
//----------------------------------
{
	std::vector<OLECHAR> tmp(256);
	::StringFromGUID2(guid, &tmp[0], static_cast<int>(tmp.size()));
	return &tmp[0];
}


GUID StringToGUID(const std::wstring &str)
//----------------------------------------
{
	return StringToIID(str);
}


UUID StringToUUID(const std::wstring &str)
//----------------------------------------
{
	UUID uuid = UUID();
	std::vector<wchar_t> tmp(str.c_str(), str.c_str() + str.length() + 1);
	if(::UuidFromStringW((RPC_WSTR)(&(tmp[0])), &uuid) != RPC_S_OK)
	{
		return UUID();
	}
	return uuid;
}


std::wstring UUIDToString(UUID uuid)
//----------------------------------
{
	std::wstring str;
	RPC_WSTR tmp = nullptr;
	if(::UuidToStringW(&uuid, &tmp) != RPC_S_OK)
	{
		return std::wstring();
	}
	str = (wchar_t*)tmp;
	::RpcStringFreeW(&tmp);
	return str;
}


bool IsValid(UUID uuid)
//---------------------
{
	return false
		|| uuid.Data1 != 0
		|| uuid.Data2 != 0
		|| uuid.Data3 != 0
		|| uuid.Data4[0] != 0
		|| uuid.Data4[1] != 0
		|| uuid.Data4[2] != 0
		|| uuid.Data4[3] != 0
		|| uuid.Data4[4] != 0
		|| uuid.Data4[5] != 0
		|| uuid.Data4[6] != 0
		|| uuid.Data4[7] != 0
		;
}


GUID CreateGUID()
//---------------
{
	GUID guid = GUID();
	if(::CoCreateGuid(&guid) != S_OK)
	{
		return GUID();
	}
	return guid;
}


UUID CreateUUID()
//---------------
{
	UUID uuid = UUID();
	RPC_STATUS status = ::UuidCreate(&uuid);
	if(status != RPC_S_OK && status != RPC_S_UUID_LOCAL_ONLY)
	{
		return UUID();
	}
	return uuid;
}


UUID CreateLocalUUID()
//--------------------
{
	UUID uuid = UUID();
	RPC_STATUS status = ::UuidCreateSequential(&uuid);
	if(status != RPC_S_OK && status != RPC_S_UUID_LOCAL_ONLY)
	{
		return UUID();
	}
	return uuid;
}




} // namespace Util


#else // !((defined(MODPLUG_TRACKER) || !defined(NO_DMO)) && MPT_OS_WINDOWS)


MPT_MSVC_WORKAROUND_LNK4221(mptUUID)


#endif // (defined(MODPLUG_TRACKER) || !defined(NO_DMO)) && MPT_OS_WINDOWS


OPENMPT_NAMESPACE_END
