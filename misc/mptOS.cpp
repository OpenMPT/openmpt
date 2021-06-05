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

#include "mpt/binary/hex.hpp"

#if MPT_OS_WINDOWS
#include <windows.h>
#endif


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{
namespace OS
{
namespace Windows
{


#if MPT_OS_WINDOWS

namespace {
struct WindowsVersionCache
{
	mpt::OS::Windows::Version version;
	WindowsVersionCache() noexcept
		: version(mpt::osinfo::windows::Version::Current())
	{
	}
};
}

static mpt::OS::Windows::Version GatherWindowsVersionFromCache() noexcept
{
	static WindowsVersionCache gs_WindowsVersionCache;
	return gs_WindowsVersionCache.version;
}

#endif // MPT_OS_WINDOWS


mpt::OS::Windows::Version Version::Current() noexcept
{
	#if MPT_OS_WINDOWS
		#ifdef MODPLUG_TRACKER
			return GatherWindowsVersionFromCache();
		#else // !MODPLUG_TRACKER
			return mpt::osinfo::windows::Version::Current();
		#endif // MODPLUG_TRACKER
	#else // !MPT_OS_WINDOWS
		return mpt::OS::Windows::Version::NoWindows();
	#endif // MPT_OS_WINDOWS
}


Version::Version() noexcept
	: mpt::osinfo::windows::Version(mpt::osinfo::windows::Version::NoWindows())
{
}


Version Version::NoWindows() noexcept
{
	return Version();
}


Version::Version(mpt::osinfo::windows::Version v) noexcept
	: mpt::osinfo::windows::Version(v)
{
}


Version::Version(mpt::OS::Windows::Version::System system, mpt::OS::Windows::Version::ServicePack servicePack, mpt::OS::Windows::Version::Build build, mpt::OS::Windows::Version::TypeId type) noexcept
	: mpt::osinfo::windows::Version(system, servicePack, build, type)
{
}


static constexpr struct { Version::System version; const mpt::uchar * name; bool showDetails; } versionMap[] =
{
	{ mpt::OS::Windows::Version::WinNewer, UL_("Windows 10 (or newer)"), false },
	{ mpt::OS::Windows::Version::Win10, UL_("Windows 10"), true },
	{ mpt::OS::Windows::Version::Win81, UL_("Windows 8.1"), true },
	{ mpt::OS::Windows::Version::Win8, UL_("Windows 8"), true },
	{ mpt::OS::Windows::Version::Win7, UL_("Windows 7"), true },
	{ mpt::OS::Windows::Version::WinVista, UL_("Windows Vista"), true },
	{ mpt::OS::Windows::Version::WinXP64, UL_("Windows XP x64 / Windows Server 2003"), true },
	{ mpt::OS::Windows::Version::WinXP, UL_("Windows XP"), true },
	{ mpt::OS::Windows::Version::Win2000, UL_("Windows 2000"), true },
	{ mpt::OS::Windows::Version::WinNT4, UL_("Windows NT4"), true }
};


mpt::ustring Version::VersionToString(mpt::OS::Windows::Version::System version)
{
	mpt::ustring result;
	for(const auto &v : versionMap)
	{
		if(version > v.version)
		{
			result = U_("> ") + v.name;
			break;
		} else if(version == v.version)
		{
			result = v.name;
			break;
		}
	}
	if(result.empty())
	{
		result = MPT_UFORMAT("0x{}")(mpt::ufmt::hex0<16>(static_cast<uint64>(version)));
	}
	return result;
}


mpt::ustring Version::GetName() const
{
	mpt::ustring name = U_("Generic Windows NT");
	bool showDetails = false;
	for(const auto &v : versionMap)
	{
		if(IsAtLeast(v.version))
		{
			name = v.name;
			showDetails = v.showDetails;
			break;
		}
	}
	name += U_(" (");
	name += MPT_UFORMAT("Version {}.{}")(m_System.Major, m_System.Minor);
	if(showDetails)
	{
		if(m_ServicePack.HasServicePack())
		{
			if(m_ServicePack.Minor)
			{
				name += MPT_UFORMAT(" Service Pack {}.{}")(m_ServicePack.Major, m_ServicePack.Minor);
			} else
			{
				name += MPT_UFORMAT(" Service Pack {}")(m_ServicePack.Major);
			}
		}
		if(m_Build != 0)
		{
			name += MPT_UFORMAT(" (Build {})")(m_Build);
		}
	}
	name += U_(")");
	mpt::ustring result = name;
	#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS
		if(mpt::OS::Windows::IsWine())
		{
			mpt::OS::Wine::VersionContext v;
			if(v.Version().IsValid())
			{
				result = MPT_UFORMAT("Wine {} ({})")(
					  v.Version().AsString()
					, name
					);
			} else
			{
				result = MPT_UFORMAT("Wine (unknown version: '{}') ({})")(
					  mpt::ToUnicode(mpt::Charset::UTF8, v.RawVersion())
					, name
					);
			}
		}
	#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS
	return result;
}


mpt::ustring Version::GetNameShort() const
{
	mpt::ustring name;
	if(mpt::OS::Windows::IsWine())
	{
		mpt::OS::Wine::VersionContext v;
		if(v.Version().IsValid())
		{
			name = MPT_UFORMAT("wine-{}")(v.Version().AsString());
		} else if(v.RawVersion().length() > 0)
		{
			name = U_("wine-") + mpt::encode_hex(mpt::as_span(v.RawVersion()));
		} else
		{
			name = U_("wine-");
		}
		name += U_("-") + mpt::encode_hex(mpt::as_span(v.RawHostSysName()));
	} else
	{
		name = MPT_UFORMAT("{}.{}")(mpt::ufmt::dec(m_System.Major), mpt::ufmt::dec0<2>(m_System.Minor));
	}
	return name;
}


mpt::OS::Windows::Version::System Version::GetMinimumKernelLevel() noexcept
{
	uint64 minimumKernelVersion = 0;
	#if MPT_OS_WINDOWS && MPT_COMPILER_MSVC
		minimumKernelVersion = std::max(minimumKernelVersion, static_cast<uint64>(mpt::OS::Windows::Version::WinVista));
	#endif
	return mpt::OS::Windows::Version::System(minimumKernelVersion);
}


mpt::OS::Windows::Version::System Version::GetMinimumAPILevel() noexcept
{
	#if MPT_OS_WINDOWS
		return SystemVersionFrom_WIN32_WINNT();
	#else // !MPT_OS_WINDOWS
		return mpt::OS::Windows::Version::System();
	#endif // MPT_OS_WINDOWS
}


#if MPT_OS_WINDOWS


#ifndef PROCESSOR_ARCHITECTURE_NEUTRAL
#define PROCESSOR_ARCHITECTURE_NEUTRAL          11
#endif
#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64            12
#endif
#ifndef PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64   13
#endif
#ifndef PROCESSOR_ARCHITECTURE_IA32_ON_ARM64
#define PROCESSOR_ARCHITECTURE_IA32_ON_ARM64    14
#endif


struct OSArchitecture
{
	uint16 ProcessorArchitectur;
	Architecture Host;
	Architecture Process;
};
static constexpr OSArchitecture architectures [] = {
	{ PROCESSOR_ARCHITECTURE_INTEL         , Architecture::x86    , Architecture::x86     },
	{ PROCESSOR_ARCHITECTURE_AMD64         , Architecture::amd64  , Architecture::amd64   },
	{ PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 , Architecture::amd64  , Architecture::x86     },
	{ PROCESSOR_ARCHITECTURE_ARM           , Architecture::arm    , Architecture::arm     },
	{ PROCESSOR_ARCHITECTURE_ARM64         , Architecture::arm64  , Architecture::arm64   },
	{ PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64, Architecture::arm64  , Architecture::arm     },
	{ PROCESSOR_ARCHITECTURE_IA32_ON_ARM64 , Architecture::arm64  , Architecture::x86     },
	{ PROCESSOR_ARCHITECTURE_MIPS          , Architecture::mips   , Architecture::mips    },
	{ PROCESSOR_ARCHITECTURE_PPC           , Architecture::ppc    , Architecture::ppc     },
	{ PROCESSOR_ARCHITECTURE_SHX           , Architecture::shx    , Architecture::shx     },
	{ PROCESSOR_ARCHITECTURE_ALPHA         , Architecture::alpha  , Architecture::alpha   },
	{ PROCESSOR_ARCHITECTURE_ALPHA64       , Architecture::alpha64, Architecture::alpha64 },
	{ PROCESSOR_ARCHITECTURE_IA64          , Architecture::ia64   , Architecture::ia64    },
	{ PROCESSOR_ARCHITECTURE_MSIL          , Architecture::unknown, Architecture::unknown },
	{ PROCESSOR_ARCHITECTURE_NEUTRAL       , Architecture::unknown, Architecture::unknown },
	{ PROCESSOR_ARCHITECTURE_UNKNOWN       , Architecture::unknown, Architecture::unknown }
};


struct HostArchitecture
{
	Architecture Host;
	Architecture Process;
	EmulationLevel Emulation;
};
static constexpr HostArchitecture hostArchitectureCanRun [] = {
	{ Architecture::x86    , Architecture::x86    , EmulationLevel::Native   },
	{ Architecture::amd64  , Architecture::amd64  , EmulationLevel::Native   },
	{ Architecture::amd64  , Architecture::x86    , EmulationLevel::Virtual  },
	{ Architecture::arm    , Architecture::arm    , EmulationLevel::Native   },
	{ Architecture::arm64  , Architecture::arm64  , EmulationLevel::Native   },
	{ Architecture::arm64  , Architecture::arm    , EmulationLevel::Virtual  },
	{ Architecture::arm64  , Architecture::x86    , EmulationLevel::Software },
	{ Architecture::arm64  , Architecture::amd64  , EmulationLevel::Software },
	{ Architecture::mips   , Architecture::mips   , EmulationLevel::Native   },
	{ Architecture::ppc    , Architecture::ppc    , EmulationLevel::Native   },
	{ Architecture::shx    , Architecture::shx    , EmulationLevel::Native   },
	{ Architecture::alpha  , Architecture::alpha  , EmulationLevel::Native   },
	{ Architecture::alpha64, Architecture::alpha64, EmulationLevel::Native   },
	{ Architecture::alpha64, Architecture::alpha  , EmulationLevel::Virtual  },
	{ Architecture::ia64   , Architecture::ia64   , EmulationLevel::Native   },
	{ Architecture::ia64   , Architecture::x86    , EmulationLevel::Hardware }
};


struct ArchitectureInfo
{
	Architecture Arch;
	int Bitness;
	const mpt::uchar * Name;
};
static constexpr ArchitectureInfo architectureInfo [] = {
	{ Architecture::x86    , 32, UL_("x86")     },
	{ Architecture::amd64  , 64, UL_("amd64")   },
	{ Architecture::arm    , 32, UL_("arm")     },
	{ Architecture::arm64  , 64, UL_("arm64")   },
	{ Architecture::mips   , 32, UL_("mips")    },
	{ Architecture::ppc    , 32, UL_("ppc")     },
	{ Architecture::shx    , 32, UL_("shx")     },
	{ Architecture::alpha  , 32, UL_("alpha")   },
	{ Architecture::alpha64, 64, UL_("alpha64") },
	{ Architecture::ia64   , 64, UL_("ia64")    }
};


int Bitness(Architecture arch) noexcept
{
	for(const auto &info : architectureInfo)
	{
		if(arch == info.Arch)
		{
			return info.Bitness;
		}
	}
	return 0;
}


mpt::ustring Name(Architecture arch)
{
	for(const auto &info : architectureInfo)
	{
		if(arch == info.Arch)
		{
			return info.Name;
		}
	}
	return mpt::ustring();
}


Architecture GetHostArchitecture() noexcept
{
	SYSTEM_INFO systemInfo = {};
	GetNativeSystemInfo(&systemInfo);
	for(const auto &arch : architectures)
	{
		if(systemInfo.wProcessorArchitecture == arch.ProcessorArchitectur)
		{
			return arch.Host;
		}
	}
	return Architecture::unknown;
}


Architecture GetProcessArchitecture() noexcept
{
	SYSTEM_INFO systemInfo = {};
	GetSystemInfo(&systemInfo);
	for(const auto &arch : architectures)
	{
		if(systemInfo.wProcessorArchitecture == arch.ProcessorArchitectur)
		{
			return arch.Process;
		}
	}
	return Architecture::unknown;
}


EmulationLevel HostCanRun(Architecture host, Architecture process) noexcept
{
	for(const auto & can : hostArchitectureCanRun)
	{
		if(can.Host == host && can.Process == process)
		{
			return can.Emulation;
		}
	}
	return EmulationLevel::NA;
}


std::vector<Architecture> GetSupportedProcessArchitectures(Architecture host)
{
	std::vector<Architecture> result;
	for(const auto & entry : hostArchitectureCanRun)
	{
		if(entry.Host == host)
		{
			result.push_back(entry.Process);
		}
	}
	return result;
}


uint64 GetSystemMemorySize()
{
	MEMORYSTATUSEX memoryStatus = {};
	memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
	if(GlobalMemoryStatusEx(&memoryStatus) == 0)
	{
		return 0;
	}
	return memoryStatus.ullTotalPhys;
}


#endif // MPT_OS_WINDOWS


#if defined(MODPLUG_TRACKER)


#if MPT_OS_WINDOWS

static bool GatherSystemIsWine()
{
	bool SystemIsWine = false;
	std::optional<mpt::library> NTDLL = mpt::library::load({ mpt::library::path_search::system, mpt::library::path_prefix::none, MPT_PATH("ntdll.dll"), mpt::library::path_suffix::none });
	if(NTDLL)
	{
		SystemIsWine = (NTDLL->get_address("wine_get_version") != nullptr);
	}
	return SystemIsWine;
}

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

#endif // MPT_OS_WINDOWS

static bool SystemIsWine(bool allowDetection = true)
{
	#if MPT_OS_WINDOWS
		static SystemIsWineCache gs_SystemIsWineCache = allowDetection ? SystemIsWineCache() : SystemIsWineCache(false);
		if(!allowDetection)
		{ // catch too late calls of PreventWineDetection
			MPT_ASSERT(!gs_SystemIsWineCache.SystemIsWine);
		}
		return gs_SystemIsWineCache.SystemIsWine;
	#else
		MPT_UNREFERENCED_PARAMETER(allowDetection);
		return false;
	#endif
}

void PreventWineDetection()
{
	SystemIsWine(false);
}

bool IsOriginal()
{
	return mpt::OS::Windows::Version::Current().IsWindows() && !SystemIsWine();
}

bool IsWine()
{
	return mpt::OS::Windows::Version::Current().IsWindows() && SystemIsWine();
}


#endif // MODPLUG_TRACKER


} // namespace Windows
} // namespace OS
} // namespace mpt



namespace mpt
{
namespace OS
{
namespace Wine
{


Version::Version()
{
	return;
}


Version::Version(const mpt::ustring &rawVersion)
	: mpt::osinfo::windows::wine::version()
{
	if(rawVersion.empty())
	{
		return;
	}
	std::vector<uint8> version = mpt::String::Split<uint8>(rawVersion, U_("."));
	if(version.size() < 2)
	{
		return;
	}
	mpt::ustring parsedVersion = mpt::String::Combine(version, U_("."));
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
	vmajor = version[0];
	vminor = version[1];
	vupdate = (version.size() >= 3) ? version[2] : 0;
}


Version::Version(uint8 vmajor, uint8 vminor, uint8 vupdate)
	: mpt::osinfo::windows::wine::version(vmajor, vminor, vupdate)
{
	return;
}


mpt::ustring Version::AsString() const
{
	return mpt::ufmt::dec(GetMajor()) + U_(".") + mpt::ufmt::dec(GetMinor()) + U_(".") + mpt::ufmt::dec(GetUpdate());
}



mpt::OS::Wine::Version GetMinimumWineVersion()
{
	mpt::OS::Wine::Version minimumWineVersion = mpt::OS::Wine::Version(0,0,0);
	#if MPT_OS_WINDOWS && MPT_COMPILER_MSVC
		minimumWineVersion = mpt::OS::Wine::Version(1,8,0);
	#endif
	return minimumWineVersion;
}


VersionContext::VersionContext()
	: m_IsWine(false)
	, m_HostClass(mpt::osinfo::osclass::Unknown)
{
	#if MPT_OS_WINDOWS
		m_IsWine = mpt::OS::Windows::IsWine();
		if(!m_IsWine)
		{
			return;
		}
		std::optional<mpt::library> NTDLL = mpt::library::load({mpt::library::path_search::system, mpt::library::path_prefix::none, MPT_PATH("ntdll.dll"), mpt::library::path_suffix::none});
		if(NTDLL)
		{
			const char * (__cdecl * wine_get_version)(void) = nullptr;
			const char * (__cdecl * wine_get_build_id)(void) = nullptr;
			void (__cdecl * wine_get_host_version)(const char * *, const char * *) = nullptr;
			NTDLL->bind(wine_get_version, "wine_get_version");
			NTDLL->bind(wine_get_build_id, "wine_get_build_id");
			NTDLL->bind(wine_get_host_version, "wine_get_host_version");
			const char * wine_version = nullptr;
			const char * wine_build_id = nullptr;
			const char * wine_host_sysname = nullptr;
			const char * wine_host_release = nullptr;
			wine_version = wine_get_version ? wine_get_version() : "";
			wine_build_id = wine_get_build_id ? wine_get_build_id() : "";
			if(wine_get_host_version)
			{
				wine_get_host_version(&wine_host_sysname, &wine_host_release);
			}
			m_RawVersion = wine_version ? wine_version : "";
			m_RawBuildID = wine_build_id ? wine_build_id : "";
			m_RawHostSysName = wine_host_sysname ? wine_host_sysname : "";
			m_RawHostRelease = wine_host_release ? wine_host_release : "";
		}
		m_Version = mpt::OS::Wine::Version(mpt::ToUnicode(mpt::Charset::UTF8, m_RawVersion));
		m_HostClass = mpt::osinfo::get_class_from_sysname(m_RawHostSysName);
	#endif // MPT_OS_WINDOWS
}


} // namespace Wine
} // namespace OS
} // namespace mpt


OPENMPT_NAMESPACE_END
