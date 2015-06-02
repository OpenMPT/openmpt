/*
 * mptOS.cpp
 * ---------
 * Purpose: Operating system version information.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptOS.h"

#if MPT_OS_WINDOWS
#include <windows.h>
#endif


OPENMPT_NAMESPACE_BEGIN


#if MPT_OS_WINDOWS

namespace mpt
{
namespace Windows
{
namespace Version
{


#if defined(MODPLUG_TRACKER)


static bool SystemIsNT = true;

// Initialize to used SDK version
static uint32 SystemVersion =
#if NTDDI_VERSION >= 0x06030000 // NTDDI_WINBLUE
	mpt::Windows::Version::Win81
#elif NTDDI_VERSION >= 0x06020000 // NTDDI_WIN8
	mpt::Windows::Version::Win8
#elif NTDDI_VERSION >= 0x06010000 // NTDDI_WIN7
	mpt::Windows::Version::Win7
#elif NTDDI_VERSION >= 0x06000000 // NTDDI_VISTA
	mpt::Windows::Version::WinVista
#elif NTDDI_VERSION >= NTDDI_WINXP
	mpt::Windows::Version::WinXP
#elif NTDDI_VERSION >= NTDDI_WIN2K
	mpt::Windows::Version::Win2000
#else
	mpt::Windows::Version::WinNT4
#endif
	;

static bool SystemIsWine = false;


void Init()
//---------
{
	OSVERSIONINFOEXW versioninfoex;
	MemsetZero(versioninfoex);
	versioninfoex.dwOSVersionInfoSize = sizeof(versioninfoex);
	GetVersionExW((LPOSVERSIONINFOW)&versioninfoex);
	SystemIsNT = (versioninfoex.dwPlatformId == VER_PLATFORM_WIN32_NT);
	SystemVersion = ((uint32)mpt::saturate_cast<uint8>(versioninfoex.dwMajorVersion) << 8) | ((uint32)mpt::saturate_cast<uint8>(versioninfoex.dwMinorVersion) << 0);
	SystemIsWine = false;
	HMODULE hNTDLL = LoadLibraryW(L"ntdll.dll");
	if(hNTDLL)
	{
		SystemIsWine = (GetProcAddress(hNTDLL, "wine_get_version") != NULL);
		FreeLibrary(hNTDLL);
		hNTDLL = NULL;
	}
}


bool IsBefore(mpt::Windows::Version::Number version)
//--------------------------------------------------
{
	return (SystemVersion < (uint32)version);
}


bool IsAtLeast(mpt::Windows::Version::Number version)
//---------------------------------------------------
{
	return (SystemVersion >= (uint32)version);
}


bool Is9x()
//---------
{
	return !SystemIsNT;
}


bool IsNT()
//---------
{
	return SystemIsNT;
}


bool IsOriginal()
//---------------
{
	return !SystemIsWine;
}


bool IsWine()
//-----------
{
	return SystemIsWine;
}


#else // !MODPLUG_TRACKER


bool IsBefore(mpt::Windows::Version::Number version)
//--------------------------------------------------
{
	OSVERSIONINFOEXW versioninfoex;
	MemsetZero(versioninfoex);
	versioninfoex.dwOSVersionInfoSize = sizeof(versioninfoex);
	GetVersionExW((LPOSVERSIONINFOW)&versioninfoex);
	uint32 SystemVersion = ((uint32)mpt::saturate_cast<uint8>(versioninfoex.dwMajorVersion) << 8) | ((uint32)mpt::saturate_cast<uint8>(versioninfoex.dwMinorVersion) << 0);
	return (SystemVersion < (uint32)version);
}


bool IsAtLeast(mpt::Windows::Version::Number version)
//---------------------------------------------------
{
	OSVERSIONINFOEXW versioninfoex;
	MemsetZero(versioninfoex);
	versioninfoex.dwOSVersionInfoSize = sizeof(versioninfoex);
	GetVersionExW((LPOSVERSIONINFOW)&versioninfoex);
	uint32 SystemVersion = ((uint32)mpt::saturate_cast<uint8>(versioninfoex.dwMajorVersion) << 8) | ((uint32)mpt::saturate_cast<uint8>(versioninfoex.dwMinorVersion) << 0);
	return (SystemVersion >= (uint32)version);
}


#endif // MODPLUG_TRACKER


} // namespace Version
} // namespace Windows
} // namespace mpt

#endif // MPT_OS_WINDOWS


OPENMPT_NAMESPACE_END
