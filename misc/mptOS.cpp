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
#include "mpt/format/join.hpp"
#include "mpt/library/library.hpp"
#include "mpt/osinfo/windows_version.hpp"
#include "mpt/parse/split.hpp"
#include "mpt/path/native_path.hpp"

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


static constexpr struct { mpt::osinfo::windows::Version version; const mpt::uchar * name; bool showDetails; } versionMap[] =
{
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::WinNewer, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 26100, 0 }, UL_("Windows 11 (or newer)"), false },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 26100, 0 }, UL_("Windows 11 24H2"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 22631, 0 }, UL_("Windows 11 23H2"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 22621, 0 }, UL_("Windows 11 22H2"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 22000, 0 }, UL_("Windows 11 21H2"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 19045, 0 }, UL_("Windows 10 22H2"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 19044, 0 }, UL_("Windows 10 21H2"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 19043, 0 }, UL_("Windows 10 21H1"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 19042, 0 }, UL_("Windows 10 20H2"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 19041, 0 }, UL_("Windows 10 2004"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 18363, 0 }, UL_("Windows 10 1909"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 18362, 0 }, UL_("Windows 10 1903"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 17763, 0 }, UL_("Windows 10 1809"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 17134, 0 }, UL_("Windows 10 1803"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 16299, 0 }, UL_("Windows 10 1709"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 15063, 0 }, UL_("Windows 10 1703"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 14393, 0 }, UL_("Windows 10 1607"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 10586, 0 }, UL_("Windows 10 1511"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 10240, 0 }, UL_("Windows 10 1507"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win10Pre, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 0, 0 }, UL_("Windows 10 Preview"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win81, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 0, 0 }, UL_("Windows 8.1"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win8, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 0, 0 }, UL_("Windows 8"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win7, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 0, 0 }, UL_("Windows 7"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::WinVista, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 0, 0 }, UL_("Windows Vista"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::WinXP64, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 0, 0 }, UL_("Windows XP x64 / Windows Server 2003"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::WinXP, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 0, 0 }, UL_("Windows XP"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::Win2000, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 0, 0 }, UL_("Windows 2000"), true },
	{ mpt::osinfo::windows::Version{ mpt::osinfo::windows::Version::WinNT4, mpt::osinfo::windows::Version::ServicePack{ 0, 0 }, 0, 0 }, UL_("Windows NT4"), true }
};


mpt::ustring Version::GetName(mpt::osinfo::windows::Version version)
{
	mpt::ustring name = U_("Generic Windows NT");
	bool showDetails = false;
	for(const auto &v : versionMap)
	{
		if(version.IsAtLeast(v.version))
		{
			name = v.name;
			showDetails = v.showDetails;
			break;
		}
	}
	name += U_(" (");
	name += MPT_UFORMAT("Version {}.{}")(version.GetSystem().Major, version.GetSystem().Minor);
	if(showDetails)
	{
		if(version.GetServicePack().HasServicePack())
		{
			if(version.GetServicePack().Minor)
			{
				name += MPT_UFORMAT(" Service Pack {}.{}")(version.GetServicePack().Major, version.GetServicePack().Minor);
			} else
			{
				name += MPT_UFORMAT(" Service Pack {}")(version.GetServicePack().Major);
			}
		}
		if(version.GetBuild() != 0)
		{
			name += MPT_UFORMAT(" (Build {})")(version.GetBuild());
		}
	}
	name += U_(")");
	return name;
}


mpt::ustring Version::GetCurrentName()
{
	mpt::ustring name = GetName(mpt::osinfo::windows::Version::Current());
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


mpt::osinfo::windows::Version Version::GetMinimumKernelLevel() noexcept
{
	uint64 minimumKernelVersion = 0;
	#if MPT_OS_WINDOWS && MPT_COMPILER_MSVC
		#if defined(MPT_BUILD_RETRO)
			minimumKernelVersion = std::max(minimumKernelVersion, static_cast<uint64>(mpt::osinfo::windows::Version::WinXP));
		#else
			minimumKernelVersion = std::max(minimumKernelVersion, static_cast<uint64>(mpt::osinfo::windows::Version::WinVista));
		#endif
	#endif
	return mpt::osinfo::windows::Version(mpt::osinfo::windows::Version::System(minimumKernelVersion), mpt::osinfo::windows::Version::ServicePack(0, 0), 0, 0);
}


mpt::osinfo::windows::Version Version::GetMinimumAPILevel() noexcept
{
	#if MPT_OS_WINDOWS
		return mpt::osinfo::windows::Version::FromSDK();
	#else // !MPT_OS_WINDOWS
		return mpt::osinfo::windows::Version::NoWindows();
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


struct MachineArchitecture
{
	USHORT ImageFileMachine;
	Architecture Host;
};
static constexpr MachineArchitecture machinearchitectures [] = {
#ifdef IMAGE_FILE_MACHINE_UNKNOWN
	{ IMAGE_FILE_MACHINE_UNKNOWN    , Architecture::unknown },
#endif
#ifdef IMAGE_FILE_MACHINE_TARGET_HOST
	{ IMAGE_FILE_MACHINE_TARGET_HOST, Architecture::unknown },
#endif
#ifdef IMAGE_FILE_MACHINE_I386
	{ IMAGE_FILE_MACHINE_I386       , Architecture::x86     },
#endif
#ifdef IMAGE_FILE_MACHINE_R3000
	{ IMAGE_FILE_MACHINE_R3000      , Architecture::mips    },
#endif
#ifdef IMAGE_FILE_MACHINE_R4000
	{ IMAGE_FILE_MACHINE_R4000      , Architecture::mips    },
#endif
#ifdef IMAGE_FILE_MACHINE_R10000
	{ IMAGE_FILE_MACHINE_R10000     , Architecture::mips    },
#endif
#ifdef IMAGE_FILE_MACHINE_WCEMIPSV2
	{ IMAGE_FILE_MACHINE_WCEMIPSV2  , Architecture::mips    },
#endif
#ifdef IMAGE_FILE_MACHINE_ALPHA
	{ IMAGE_FILE_MACHINE_ALPHA      , Architecture::alpha   },
#endif
#ifdef IMAGE_FILE_MACHINE_SH3
	{ IMAGE_FILE_MACHINE_SH3        , Architecture::shx     },
#endif
#ifdef IMAGE_FILE_MACHINE_SH3DSP
	{ IMAGE_FILE_MACHINE_SH3DSP     , Architecture::shx     },
#endif
#ifdef IMAGE_FILE_MACHINE_SH3E
	{ IMAGE_FILE_MACHINE_SH3E       , Architecture::shx     },
#endif
#ifdef IMAGE_FILE_MACHINE_SH4
	{ IMAGE_FILE_MACHINE_SH4        , Architecture::shx     },
#endif
#ifdef IMAGE_FILE_MACHINE_SH5
	{ IMAGE_FILE_MACHINE_SH5        , Architecture::shx     },
#endif
#ifdef IMAGE_FILE_MACHINE_ARM
	{ IMAGE_FILE_MACHINE_ARM        , Architecture::arm     },
#endif
#ifdef IMAGE_FILE_MACHINE_THUMB
	{ IMAGE_FILE_MACHINE_THUMB      , Architecture::arm     },
#endif
#ifdef IMAGE_FILE_MACHINE_ARMNT
	{ IMAGE_FILE_MACHINE_ARMNT      , Architecture::arm     },
#endif
#ifdef IMAGE_FILE_MACHINE_AM33
	{ IMAGE_FILE_MACHINE_AM33       , Architecture::unknown },
#endif
#ifdef IMAGE_FILE_MACHINE_POWERPC
	{ IMAGE_FILE_MACHINE_POWERPC    , Architecture::ppc     },
#endif
#ifdef IMAGE_FILE_MACHINE_POWERPCFP
	{ IMAGE_FILE_MACHINE_POWERPCFP  , Architecture::ppc     },
#endif
#ifdef IMAGE_FILE_MACHINE_MIPS16
	{ IMAGE_FILE_MACHINE_MIPS16     , Architecture::unknown },
#endif
#ifdef IMAGE_FILE_MACHINE_ALPHA64
	{ IMAGE_FILE_MACHINE_ALPHA64    , Architecture::alpha64 },
#endif
#ifdef IMAGE_FILE_MACHINE_MIPSFPU
	{ IMAGE_FILE_MACHINE_MIPSFPU    , Architecture::mips    },
#endif
#ifdef IMAGE_FILE_MACHINE_MIPSFPU16
	{ IMAGE_FILE_MACHINE_MIPSFPU16  , Architecture::unknown },
#endif
#ifdef IMAGE_FILE_MACHINE_AXP64
	{ IMAGE_FILE_MACHINE_AXP64      , Architecture::alpha64 },
#endif
#ifdef IMAGE_FILE_MACHINE_TRICORE
	{ IMAGE_FILE_MACHINE_TRICORE    , Architecture::unknown },
#endif
#ifdef IMAGE_FILE_MACHINE_CEF
	{ IMAGE_FILE_MACHINE_CEF        , Architecture::unknown },
#endif
#ifdef IMAGE_FILE_MACHINE_EBC
	{ IMAGE_FILE_MACHINE_EBC        , Architecture::unknown },
#endif
#ifdef IMAGE_FILE_MACHINE_AMD64
	{ IMAGE_FILE_MACHINE_AMD64      , Architecture::amd64   },
#endif
#ifdef IMAGE_FILE_MACHINE_M32R
	{ IMAGE_FILE_MACHINE_M32R       , Architecture::unknown },
#endif
#ifdef IMAGE_FILE_MACHINE_ARM64
	{ IMAGE_FILE_MACHINE_ARM64      , Architecture::arm64   },
#endif
#ifdef IMAGE_FILE_MACHINE_CEE
	{ IMAGE_FILE_MACHINE_CEE        , Architecture::unknown }
#endif
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
	mpt::osinfo::windows::Version WindowsVersion = mpt::osinfo::windows::Version::Current();
	if(WindowsVersion.IsAtLeast(mpt::osinfo::windows::Version(mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::Version::ServicePack(0, 0), 16299, 0))) {
		std::optional<mpt::library> kernel32{mpt::library::load({ mpt::library::path_search::system, mpt::library::path_prefix::none, MPT_NATIVE_PATH("kernel32.dll"), mpt::library::path_suffix::none })};
		if(kernel32.has_value())
		{
			BOOL (WINAPI * fIsWow64Process2)(HANDLE hProcess, USHORT *pProcessMachine, USHORT *pNativeMachine) = NULL;
			if(kernel32->bind(fIsWow64Process2, "IsWow64Process2"))
			{
				USHORT ProcessMachine = 0;
				USHORT NativeMachine = 0;
				if(fIsWow64Process2(GetCurrentProcess(), &ProcessMachine, &NativeMachine) != FALSE)
				{
					for(const auto &arch : machinearchitectures)
					{
						if(NativeMachine == arch.ImageFileMachine)
						{
							if(arch.Host != Architecture::unknown)
							{
								return arch.Host;
							}
						}
					}
				}
			}
		}
	}
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

namespace {
struct SystemIsWineCache
{
	bool SystemIsWine;
	SystemIsWineCache()
		: SystemIsWine(mpt::osinfo::windows::current_is_wine())
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
	return mpt::osinfo::windows::Version::Current().IsWindows() && !SystemIsWine();
}

bool IsWine()
{
	return mpt::osinfo::windows::Version::Current().IsWindows() && SystemIsWine();
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
	std::vector<uint8> version = mpt::split_parse<uint8>(rawVersion, U_("."));
	if(version.size() < 2)
	{
		return;
	}
	mpt::ustring parsedVersion = mpt::join_format(version, U_("."));
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
		std::optional<mpt::library> NTDLL = mpt::library::load({mpt::library::path_search::system, mpt::library::path_prefix::none, MPT_NATIVE_PATH("ntdll.dll"), mpt::library::path_suffix::none});
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
