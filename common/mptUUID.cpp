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

#include "mptRandom.h"
#include "mptStringFormat.h"
#include "Endianness.h"

#include <cstdlib>

#if MPT_OS_WINDOWS
#if defined(MODPLUG_TRACKER) || !defined(NO_DMO)
#include <windows.h>
#include <objbase.h>
#endif // MODPLUG_TRACKER || !NO_DMO
#endif // MPT_OS_WINDOWS


OPENMPT_NAMESPACE_BEGIN


#if MPT_OS_WINDOWS


namespace Util
{


#if defined(MODPLUG_TRACKER) || !defined(NO_DMO)


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


#endif // MODPLUG_TRACKER || !NO_DMO


#if defined(MODPLUG_TRACKER) || !defined(NO_DMO) || defined(MPT_ENABLE_TEMPFILE)


UUID StringToUUID(const mpt::ustring &str)
//----------------------------------------
{
	UUID uuid = UUID();
	std::wstring wstr = mpt::ToWide(str);
	std::vector<wchar_t> tmp(wstr.c_str(), wstr.c_str() + wstr.length() + 1);
	if(::UuidFromStringW((RPC_WSTR)(&(tmp[0])), &uuid) != RPC_S_OK)
	{
		return UUID();
	}
	return uuid;
}


mpt::ustring UUIDToString(UUID uuid)
//----------------------------------
{
	std::wstring wstr;
	RPC_WSTR tmp = nullptr;
	if(::UuidToStringW(&uuid, &tmp) != RPC_S_OK)
	{
		return mpt::ustring();
	}
	wstr = (wchar_t*)tmp;
	::RpcStringFreeW(&tmp);
	return mpt::ToUnicode(wstr);
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


UUID CreateUUID()
//---------------
{
	UUID uuid = UUID();
	RPC_STATUS status = ::UuidCreate(&uuid);
	if(status != RPC_S_OK && status != RPC_S_UUID_LOCAL_ONLY)
	{
		return mpt::UUID::RFC4122Random();
	}
	if(!Util::IsValid(uuid))
	{
		return mpt::UUID::RFC4122Random();
	}
	return uuid;
}


UUID CreateLocalUUID()
//--------------------
{
	#if _WIN32_WINNT >= 0x0501
		// Available since Win2000, but we check for WinXP in order to not use this
		// function in Win32old builds. It is not available on some non-fully
		// patched Win98SE installs in the wild.
		UUID uuid = UUID();
		RPC_STATUS status = ::UuidCreateSequential(&uuid);
		if(status != RPC_S_OK && status != RPC_S_UUID_LOCAL_ONLY)
		{
			return CreateUUID();
		}
		if(!Util::IsValid(uuid))
		{
			return mpt::UUID::RFC4122Random();
		}
		return uuid;
	#else
		// Fallback to ::UuidCreate is safe as ::UuidCreateSequential is only a
		// tiny performance optimization.
		return CreateUUID();
	#endif
}


#endif // MODPLUG_TRACKER || !NO_DMO || MPT_ENABLE_TEMPFILE


} // namespace Util


#endif // MPT_OS_WINDOWS


namespace mpt
{

UUID UUID::Generate()
{
	#if MPT_OS_WINDOWS && (defined(MODPLUG_TRACKER) || !defined(NO_DMO) || defined(MPT_ENABLE_TEMPFILE))
		return mpt::UUID(Util::CreateUUID());
	#else
		return RFC4122Random();
	#endif
}

UUID UUID::GenerateLocalUseOnly()
{
	#if MPT_OS_WINDOWS && (defined(MODPLUG_TRACKER) || !defined(NO_DMO) || defined(MPT_ENABLE_TEMPFILE))
		return mpt::UUID(Util::CreateLocalUUID());
	#else
		return RFC4122Random();
	#endif
}

UUID UUID::RFC4122Random()
{
	UUID result;
	mpt::best_prng prng = mpt::make_prng<mpt::best_prng>(mpt::global_random_device());
	result.Data1 = mpt::random<uint32>(prng);
	result.Data2 = mpt::random<uint16>(prng);
	result.Data3 = mpt::random<uint16>(prng);
	result.Data4 = mpt::random<uint64>(prng);
	result.MakeRFC4122(4);
	return result;
}

void UUID::MakeRFC4122(uint8 version)
{
	// variant
	uint8 Nn = static_cast<uint8>((Data4 >> 56) & 0xffu);
	Data4 &= 0x00ffffffffffffffull;
	Nn &= ~(0xc0u);
	Nn |= 0x80u;
	Data4 |= static_cast<uint64>(Nn) << 56;
	// version
	version &= 0x0fu;
	uint8 Mm = static_cast<uint8>((Data3 >> 8) & 0xffu);
	Data3 &= 0x00ffu;
	Mm &= ~(0xf0u);
	Mm |= (version << 4u);
	Data3 |= static_cast<uint16>(Mm) << 8;
}

void UUID::ConvertEndianness()
{
	SwapBytesBE(Data1);
	SwapBytesBE(Data2);
	SwapBytesBE(Data3);
	SwapBytesBE(Data4);
}

std::string UUID::ToString() const
{
	return std::string()
		+ mpt::fmt::hex0<8>(Data1)
		+ "-"
		+ mpt::fmt::hex0<4>(Data2)
		+ "-"
		+ mpt::fmt::hex0<4>(Data3)
		+ "-"
		+ mpt::fmt::hex0<4>(static_cast<uint16>(Data4 >> 48))
		+ "-"
		+ mpt::fmt::hex0<4>(static_cast<uint16>(Data4 >> 32))
		+ mpt::fmt::hex0<8>(static_cast<uint32>(Data4 >>  0))
		;
}
mpt::ustring UUID::ToUString() const
{
	return mpt::ustring()
		+ mpt::ufmt::hex0<8>(Data1)
		+ MPT_USTRING("-")
		+ mpt::ufmt::hex0<4>(Data2)
		+ MPT_USTRING("-")
		+ mpt::ufmt::hex0<4>(Data3)
		+ MPT_USTRING("-")
		+ mpt::ufmt::hex0<4>(static_cast<uint16>(Data4 >> 48))
		+ MPT_USTRING("-")
		+ mpt::ufmt::hex0<4>(static_cast<uint16>(Data4 >> 32))
		+ mpt::ufmt::hex0<8>(static_cast<uint32>(Data4 >>  0))
		;
}

} // namespace mpt


OPENMPT_NAMESPACE_END
