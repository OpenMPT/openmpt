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



#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS

namespace mpt
{
namespace Wine
{

	
std::string RawGetVersion()
//-------------------------
{
	if(!mpt::Windows::Version::IsWine())
	{
		return std::string();
	}
	mpt::Library wine(mpt::LibraryPath::FullPath(MPT_PATHSTRING("ntdll.dll")));
	if(!wine.IsValid())
	{
		return std::string();
	}
	const char * (__cdecl * wine_get_version)(void) = nullptr;
	if(!wine.Bind(wine_get_version, "wine_get_version"))
	{
		return std::string();
	}
	const char * wine_version = nullptr;
	wine_version = wine_get_version();
	if(!wine_version)
	{
		return std::string();
	}
	return wine_version;
}

std::string RawGetBuildID()
//-------------------------
{
	if(!mpt::Windows::Version::IsWine())
	{
		return std::string();
	}
	mpt::Library wine(mpt::LibraryPath::FullPath(MPT_PATHSTRING("ntdll.dll")));
	if(!wine.IsValid())
	{
		return std::string();
	}
	const char * (__cdecl * wine_get_build_id)(void) = nullptr;
	if(!wine.Bind(wine_get_build_id, "wine_get_build_id"))
	{
		return std::string();
	}
	const char * wine_build_id = nullptr;
	wine_build_id = wine_get_build_id();
	if(!wine_build_id)
	{
		return std::string();
	}
	return wine_build_id;
}

std::string RawGetHostSysName()
//-----------------------------
{
	if(!mpt::Windows::Version::IsWine())
	{
		return std::string();
	}
	mpt::Library wine(mpt::LibraryPath::FullPath(MPT_PATHSTRING("ntdll.dll")));
	if(!wine.IsValid())
	{
		return std::string();
	}
	void (__cdecl * wine_get_host_version)(const char * *, const char * *) = nullptr;
	if(!wine.Bind(wine_get_host_version, "wine_get_host_version"))
	{
		return std::string();
	}
	const char * wine_host_sysname = nullptr;
	const char * wine_host_release = nullptr;
	wine_get_host_version(&wine_host_sysname, &wine_host_release);
	if(!wine_host_sysname)
	{
		return std::string();
	}
	return wine_host_sysname;
}

std::string RawGetHostRelease()
//-----------------------------
{
	if(!mpt::Windows::Version::IsWine())
	{
		return std::string();
	}
	mpt::Library wine(mpt::LibraryPath::FullPath(MPT_PATHSTRING("ntdll.dll")));
	if(!wine.IsValid())
	{
		return std::string();
	}
	void (__cdecl * wine_get_host_version)(const char * *, const char * *) = nullptr;
	if(!wine.Bind(wine_get_host_version, "wine_get_host_version"))
	{
		return std::string();
	}
	const char * wine_host_sysname = nullptr;
	const char * wine_host_release = nullptr;
	wine_get_host_version(&wine_host_sysname, &wine_host_release);
	if(!wine_host_release)
	{
		return std::string();
	}
	return wine_host_release;
}


uint32 Version(uint8 a, uint8 b, uint8 c)
//---------------------------------------
{
	return (a << 16) | (b << 8) | c;
}

std::string VersionString(uint8 a, uint8 b, uint8 c)
//--------------------------------------------------
{
	return mpt::fmt::dec(a) + "." + mpt::fmt::dec(b) + "." + mpt::fmt::dec(c);
}

std::string VersionString(uint32 v)
//---------------------------------
{
	if(v > 0xffffff)
	{
		return std::string();
	}
	return VersionString((uint8)(v >> 16), (uint8)(v >> 8), (uint8)(v >> 0));
}


uint32 GetVersion()
//-----------------
{
	if(!mpt::Windows::Version::IsWine())
	{
		return 0;
	}
	mpt::ustring rawVersion = mpt::ToUnicode(mpt::CharsetUTF8, Wine::RawGetVersion());
	if(rawVersion.empty())
	{
		return 0;
	}
	std::vector<uint8> version = mpt::String::Split<uint8>(rawVersion, MPT_USTRING("."));
	if(version.size() < 3)
	{
		return 0;
	}
	mpt::ustring parsedVersion = mpt::String::Combine(version, MPT_USTRING("."));
	std::size_t len = std::min(parsedVersion.length(), rawVersion.length());
	if(len == 0)
	{
		return 0;
	}
	if(parsedVersion.substr(0, len) != rawVersion.substr(0, len))
	{
		return 0;
	}
	return Wine::Version(version[0], version[1], version[2]);
}


bool VersionIsBefore(uint8 major, uint8 minor, uint8 update)
//----------------------------------------------------------
{
	if(!mpt::Windows::Version::IsWine())
	{
		return false;
	}
	uint32 version = mpt::Wine::GetVersion();
	if(version == 0)
	{
		return false;
	}
	return (version < mpt::Wine::Version(major, minor, update));
}

bool VersionIsAtLeast(uint8 major, uint8 minor, uint8 update)
//-----------------------------------------------------------
{
	if(!mpt::Windows::Version::IsWine())
	{
		return false;
	}
	uint32 version = mpt::Wine::GetVersion();
	if(version == 0)
	{
		return false;
	}
	return (version >= mpt::Wine::Version(major, minor, update));
}

bool HostIsLinux()
//----------------
{
	if(!mpt::Windows::Version::IsWine())
	{
		return false;
	}
	return Wine::RawGetHostSysName() == "Linux";
}


} // namespace Wine
} // namespace mpt

#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS



OPENMPT_NAMESPACE_END
