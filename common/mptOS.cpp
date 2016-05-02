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


namespace mpt
{
namespace Windows
{


#if MPT_OS_WINDOWS


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


static void GatherWindowsVersion(bool & SystemIsNT, uint32 & SystemVersion)
//-------------------------------------------------------------------------
{
	// Initialize to used SDK version
	SystemVersion =
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
	OSVERSIONINFOEXW versioninfoex;
	MemsetZero(versioninfoex);
	versioninfoex.dwOSVersionInfoSize = sizeof(versioninfoex);
	GetVersionExW((LPOSVERSIONINFOW)&versioninfoex);
	SystemIsNT = (versioninfoex.dwPlatformId == VER_PLATFORM_WIN32_NT);
	SystemVersion = VersionDecimalTo_WIN32_WINNT(versioninfoex.dwMajorVersion, versioninfoex.dwMinorVersion);
}


#ifdef MODPLUG_TRACKER

namespace {
struct WindowsVersionCache
{
	bool SystemIsNT;
	uint32 SystemVersion;
	WindowsVersionCache()
		: SystemIsNT(false)
		, SystemVersion(mpt::Windows::Version::WinNT4)
	{
		GatherWindowsVersion(SystemIsNT, SystemVersion);
	}
};
}

static void GatherWindowsVersionFromCache(bool & SystemIsNT, uint32 & SystemVersion)
//----------------------------------------------------------------------------------
{
	static WindowsVersionCache gs_WindowsVersionCache;
	SystemIsNT = gs_WindowsVersionCache.SystemIsNT;
	SystemVersion = gs_WindowsVersionCache.SystemVersion;
}

#endif // MODPLUG_TRACKER


#endif // MPT_OS_WINDOWS


Version::Version()
//----------------
	: SystemIsWindows(false)
	, SystemIsNT(true)
	, SystemVersion(mpt::Windows::Version::WinNT4)
{
	return;
}


mpt::Windows::Version Version::Current()
//--------------------------------------
{
	mpt::Windows::Version result;
	#if MPT_OS_WINDOWS
		result.SystemIsWindows = true;
		#ifdef MODPLUG_TRACKER
			GatherWindowsVersionFromCache(result.SystemIsNT, result.SystemVersion);
		#else // !MODPLUG_TRACKER
			GatherWindowsVersion(result.SystemIsNT, result.SystemVersion);
		#endif // MODPLUG_TRACKER
	#endif // MPT_OS_WINDOWS
	return result;
}


bool Version::IsWindows() const
//-----------------------------
{
	return SystemIsWindows;
}


bool Version::IsBefore(mpt::Windows::Version::Number version) const
//-----------------------------------------------------------------
{
	if(!SystemIsWindows)
	{
		return false;
	}
	return (SystemVersion < static_cast<uint32>(version));
}


bool Version::IsAtLeast(mpt::Windows::Version::Number version) const
//------------------------------------------------------------------
{
	if(!SystemIsWindows)
	{
		return false;
	}
	return (SystemVersion >= static_cast<uint32>(version));
}


bool Version::Is9x() const
//------------------------
{
	if(!SystemIsWindows)
	{
		return false;
	}
	return !SystemIsNT;
}


bool Version::IsNT() const
//------------------------
{
	if(!SystemIsWindows)
	{
		return false;
	}
	return SystemIsNT;
}


mpt::ustring Version::VersionToString(uint16 version)
//---------------------------------------------------
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
		result = mpt::format(MPT_USTRING("0x%1"))(mpt::ufmt::dec0<4>(version));
	}
	return result;
}


mpt::ustring Version::GetName() const
//-----------------------------------
{
	mpt::ustring name;
	if(mpt::Windows::Version::IsNT())
	{
		if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinNewer))
		{
			name = MPT_USTRING("Windows 10 (or newer)");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win10))
		{
			name = MPT_USTRING("Windows 10");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win81))
		{
			name = MPT_USTRING("Windows 8.1");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win8))
		{
			name = MPT_USTRING("Windows 8");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win7))
		{
			name = MPT_USTRING("Windows 7");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinVista))
		{
			name = MPT_USTRING("Windows Vista");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinXP))
		{
			name = MPT_USTRING("Windows XP");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win2000))
		{
			name = MPT_USTRING("Windows 2000");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinNT4))
		{
			name = MPT_USTRING("Windows NT4");
		} else
		{
			name = MPT_USTRING("Generic Windows NT");
		}
	} else
	{
		if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinME))
		{
			name = MPT_USTRING("Windows ME (or newer)");
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::Win98))
		{
			name = MPT_USTRING("Windows 98");
		} else
		{
			name = MPT_USTRING("Generic Windows 9x");
		}
	}
	mpt::ustring result = name;
	#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS
		if(mpt::Windows::IsWine())
		{
			if(mpt::Wine::GetVersion().IsValid())
			{
				result = mpt::format(MPT_USTRING("Wine %1 (%2)"))(
					  mpt::Wine::GetVersion().AsString()
					, name
					);
			} else
			{
				result = mpt::format(MPT_USTRING("Wine (unknown version: '%1') (%2)"))(
					  mpt::ToUnicode(mpt::CharsetUTF8, mpt::Wine::RawGetVersion())
					, name
					);
			}
		}
	#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS
	return result;
}


uint16 Version::GetMinimumKernelLevel()
//-------------------------------------
{
	uint16 minimumKernelVersion = 0;
	#if MPT_OS_WINDOWS && MPT_COMPILER_MSVC
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


uint16 Version::GetMinimumAPILevel()
//----------------------------------
{
	uint16 minimumApiVersion = 0;
	#if MPT_OS_WINDOWS && defined(_WIN32_WINNT)
		minimumApiVersion = std::max<uint16>(minimumApiVersion, _WIN32_WINNT);
	#endif
	return minimumApiVersion;
}


#if defined(MODPLUG_TRACKER)


#if MPT_OS_WINDOWS

static bool GatherSystemIsWine()
//------------------------------
{
	bool SystemIsWine = false;
	HMODULE hNTDLL = LoadLibraryW(L"ntdll.dll");
	if(hNTDLL)
	{
		SystemIsWine = (GetProcAddress(hNTDLL, "wine_get_version") != NULL);
		FreeLibrary(hNTDLL);
		hNTDLL = NULL;
	}
	return SystemIsWine;
}

#endif // MPT_OS_WINDOWS

#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS

namespace {
struct SystemIsWineCache
{
	bool SystemIsWine;
	SystemIsWineCache()
		: SystemIsWine(GatherSystemIsWine())
	{
		return;
	}
	SystemIsWineCache(bool isWine)
		: SystemIsWine(isWine)
	{
		return;
	}
};
}

#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS

static bool SystemIsWine(bool allowDetection = true)
{
	#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS
		static SystemIsWineCache gs_SystemIsWineCache = allowDetection ? SystemIsWineCache() : SystemIsWineCache(false);
		if(!allowDetection)
		{ // catch too late calls of PreventWineDetection
			MPT_ASSERT(!gs_SystemIsWineCache.SystemIsWine);
		}
		return gs_SystemIsWineCache.SystemIsWine;
	#elif MPT_OS_WINDOWS
		MPT_UNREFERENCED_PARAMETER(allowDetection);
		return GatherSystemIsWine();
	#else
		MPT_UNREFERENCED_PARAMETER(allowDetection);
		return false;
	#endif
}

void PreventWineDetection()
//-------------------------
{
	SystemIsWine(false);
}

bool IsOriginal()
//---------------
{
	return mpt::Windows::Version::Current().IsWindows() && !SystemIsWine();
}

bool IsWine()
//-----------
{
	return mpt::Windows::Version::Current().IsWindows() && SystemIsWine();
}


#endif // MODPLUG_TRACKER


} // namespace Windows
} // namespace mpt



#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS

namespace mpt
{
namespace Wine
{


std::string RawGetVersion()
//-------------------------
{
	if(!mpt::Windows::IsWine())
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
	if(!mpt::Windows::IsWine())
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
	if(!mpt::Windows::IsWine())
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
	if(!mpt::Windows::IsWine())
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


Version::Version()
//----------------
	: valid(false)
	, major(0)
	, minor(0)
	, update(0)
{
	return;
}


Version::Version(const mpt::ustring &rawVersion)
//----------------------------------------------
	: valid(false)
	, major(0)
	, minor(0)
	, update(0)
{
	if(rawVersion.empty())
	{
		return;
	}
	std::vector<uint8> version = mpt::String::Split<uint8>(rawVersion, MPT_USTRING("."));
	if(version.size() < 3)
	{
		return;
	}
	mpt::ustring parsedVersion = mpt::String::Combine(version, MPT_USTRING("."));
	std::size_t len = std::min(parsedVersion.length(), rawVersion.length());
	if(len == 0)
	{
		return;
	}
	if(parsedVersion.substr(0, len) != rawVersion.substr(0, len))
	{
		return;
	}
	valid = true;
	major = version[0];
	minor = version[1];
	update = version[2];
}


Version::Version(uint8 major, uint8 minor, uint8 update)
//------------------------------------------------------
	: valid((major > 0) || (minor > 0) || (update > 0)) 
	, major(major)
	, minor(minor)
	, update(update)
{
	return;
}


mpt::Wine::Version Version::FromInteger(uint32 version)
//-----------------------------------------------------
{
	mpt::Wine::Version result;
	result.valid = (version <= 0xffffff);
	result.major = static_cast<uint8>(version >> 16);
	result.minor = static_cast<uint8>(version >> 8);
	result.update = static_cast<uint8>(version >> 0);
	return result;
}


bool Version::IsValid() const
//---------------------------
{
	return valid;
}


mpt::ustring Version::AsString() const
//------------------------------------
{
	return mpt::ufmt::dec(major) + MPT_USTRING(".") + mpt::ufmt::dec(minor) + MPT_USTRING(".") + mpt::ufmt::dec(update);
}


uint32 Version::AsInteger() const
{
	uint32 version = 0;
	version |= static_cast<uint32>(major) << 16;
	version |= static_cast<uint32>(minor) << 8;
	version |= static_cast<uint32>(update) << 0;
	return version;
}


mpt::Wine::Version GetVersion()
//-----------------------------
{
	if(!mpt::Windows::IsWine())
	{
		return mpt::Wine::Version();
	}
	return mpt::Wine::Version(mpt::ToUnicode(mpt::CharsetUTF8, Wine::RawGetVersion()));
}


bool Version::IsBefore(mpt::Wine::Version other) const
//----------------------------------------------------
{
	if(!IsValid())
	{
		return false;
	}
	return (AsInteger() < other.AsInteger());
}


bool Version::IsAtLeast(mpt::Wine::Version other) const
//-----------------------------------------------------
{
	if(!IsValid())
	{
		return false;
	}
	return (AsInteger() >= other.AsInteger());
}


bool HostIsLinux()
//----------------
{
	if(!mpt::Windows::IsWine())
	{
		return false;
	}
	return Wine::RawGetHostSysName() == "Linux";
}


} // namespace Wine
} // namespace mpt

#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS



OPENMPT_NAMESPACE_END
