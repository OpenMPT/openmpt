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



static uint32 VersionDecimalTo_WIN32_WINNT(uint32 major, uint32 minor)
{
	// GetVersionEx returns decimal.
	// _WIN32_WINNT macro uses BCD for the minor byte (see Windows 98 / ME).
	// We use what _WIN32_WINNT does.
	uint32 result = 0;
	minor = Clamp(minor, 0u, 99u);
	result |= major;
	result <<= 8;
	result |= minor/10*0x10 + minor%10;
	return result;
}



#if defined(MODPLUG_TRACKER)


static bool SystemIsNT = true;

// Initialize to used SDK version
static uint32 SystemVersion =
#if NTDDI_VERSION >= 0x0A000000 // NTDDI_WIN10
	mpt::Windows::Version::Win10
#elif NTDDI_VERSION >= 0x06030000 // NTDDI_WINBLUE
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
	SystemVersion = VersionDecimalTo_WIN32_WINNT(versioninfoex.dwMajorVersion, versioninfoex.dwMinorVersion);
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


mpt::ustring VersionToString(uint16 version)
//------------------------------------------
{
	mpt::ustring result;
	std::vector<std::pair<uint16, mpt::ustring> > versionMap;
	versionMap.push_back(std::make_pair(static_cast<uint16>(mpt::Windows::Version::WinNewer), MPT_USTRING("Windows 10 (or newer)")));
	versionMap.push_back(std::make_pair(static_cast<uint16>(mpt::Windows::Version::Win10), MPT_USTRING("Windows 10")));
	versionMap.push_back(std::make_pair(static_cast<uint16>(mpt::Windows::Version::Win81), MPT_USTRING("Windows 8.1")));
	versionMap.push_back(std::make_pair(static_cast<uint16>(mpt::Windows::Version::Win8), MPT_USTRING("Windows 8")));
	versionMap.push_back(std::make_pair(static_cast<uint16>(mpt::Windows::Version::Win7), MPT_USTRING("Windows 7")));
	versionMap.push_back(std::make_pair(static_cast<uint16>(mpt::Windows::Version::WinVista), MPT_USTRING("Windows Vista")));
	versionMap.push_back(std::make_pair(static_cast<uint16>(mpt::Windows::Version::WinXP), MPT_USTRING("Windows XP")));
	versionMap.push_back(std::make_pair(static_cast<uint16>(mpt::Windows::Version::Win2000), MPT_USTRING("Windows 2000")));
	versionMap.push_back(std::make_pair(static_cast<uint16>(mpt::Windows::Version::WinME), MPT_USTRING("Windows ME")));
	versionMap.push_back(std::make_pair(static_cast<uint16>(mpt::Windows::Version::Win98), MPT_USTRING("Windows 98")));
	versionMap.push_back(std::make_pair(static_cast<uint16>(mpt::Windows::Version::WinNT4), MPT_USTRING("Windows NT4")));
	for(std::size_t i = 0; i < versionMap.size(); ++i)
	{
		if(version > versionMap[i].first)
		{
			result = MPT_USTRING("> ") + versionMap[i].second;
			break;
		} else if(version == versionMap[i].first)
		{
			result = versionMap[i].second;
			break;
		}
	}
	if(result.empty())
	{
		result = MPT_UFORMAT("0x%1", mpt::ufmt::dec0<4>(version));
	}
	return result;
}


mpt::ustring GetName()
//--------------------
{
	mpt::ustring result;
	if(mpt::Windows::Version::IsWine())
	{
		result += MPT_USTRING("Wine");
		if(mpt::Wine::GetVersion())
		{
			result += MPT_UFORMAT(" %1", mpt::ToUnicode(mpt::CharsetUTF8, mpt::Wine::VersionString(mpt::Wine::GetVersion())));
		} else
		{
			result += MPT_UFORMAT(" (unknown version: '%1')", mpt::ToUnicode(mpt::CharsetUTF8, mpt::Wine::RawGetVersion()));
		}
	}
	if(mpt::Windows::Version::IsWine())
	{
		result += MPT_USTRING(" (");
	}
	if(mpt::Windows::Version::IsNT())
	{
		if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinNewer))
		{
			result += MPT_USTRING("Windows 10 (or newer)");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win10))
		{
			result += MPT_USTRING("Windows 10");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win81))
		{
			result += MPT_USTRING("Windows 8.1");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win8))
		{
			result += MPT_USTRING("Windows 8");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win7))
		{
			result += MPT_USTRING("Windows 7");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinVista))
		{
			result += MPT_USTRING("Windows Vista");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinXP))
		{
			result += MPT_USTRING("Windows XP");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win2000))
		{
			result += MPT_USTRING("Windows 2000");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinNT4))
		{
			result += MPT_USTRING("Windows NT4");
		} else
		{
			result += MPT_USTRING("Generic Windows NT");
		}
	} else
	{
		if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinME))
		{
			result += MPT_USTRING("Windows ME (or newer)");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win98))
		{
			result += MPT_USTRING("Windows 98");
		} else
		{
			result += MPT_USTRING("Generic Windows 9x");
		}
	}
	if(mpt::Windows::Version::IsWine())
	{
		result += MPT_USTRING(")");
	}
	return result;
}


uint16 GetMinimumKernelLevel()
//----------------------------
{
	uint16 minimumKernelVersion = 0;
	#if MPT_COMPILER_MSVC
		#if MPT_MSVC_AT_LEAST(2012, 0)
			minimumKernelVersion = std::max<uint16>(minimumKernelVersion, mpt::Windows::Version::WinVista);
		#elif MPT_MSVC_AT_LEAST(2010, 0)
			minimumKernelVersion = std::max<uint16>(minimumKernelVersion, mpt::Windows::Version::Win2000);
		#elif MPT_MSVC_AT_LEAST(2008, 0)
			minimumKernelVersion = std::max<uint16>(minimumKernelVersion, mpt::Windows::Version::Win98);
		#endif
	#endif
	return minimumKernelVersion;
}


uint16 GetMinimumAPILevel()
//-------------------------
{
	uint16 minimumApiVersion = 0;
	#if defined(_WIN32_WINNT)
		minimumApiVersion = std::max<uint16>(minimumApiVersion, _WIN32_WINNT);
	#endif
	return minimumApiVersion;
}


#else // !MODPLUG_TRACKER


bool IsBefore(mpt::Windows::Version::Number version)
//--------------------------------------------------
{
	OSVERSIONINFOEXW versioninfoex;
	MemsetZero(versioninfoex);
	versioninfoex.dwOSVersionInfoSize = sizeof(versioninfoex);
	GetVersionExW((LPOSVERSIONINFOW)&versioninfoex);
	uint32 SystemVersion = VersionDecimalTo_WIN32_WINNT(versioninfoex.dwMajorVersion, versioninfoex.dwMinorVersion);
	return (SystemVersion < (uint32)version);
}


bool IsAtLeast(mpt::Windows::Version::Number version)
//---------------------------------------------------
{
	OSVERSIONINFOEXW versioninfoex;
	MemsetZero(versioninfoex);
	versioninfoex.dwOSVersionInfoSize = sizeof(versioninfoex);
	GetVersionExW((LPOSVERSIONINFOW)&versioninfoex);
	uint32 SystemVersion = VersionDecimalTo_WIN32_WINNT(versioninfoex.dwMajorVersion, versioninfoex.dwMinorVersion);
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
