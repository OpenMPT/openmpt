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
#include "mpt/osinfo/windows_hx_version.hpp"
#include "mpt/osinfo/windows_version.hpp"
#include "mpt/osinfo/windows_wine_version.hpp"
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



static constexpr struct { mpt::osinfo::windows::Epoch epoch; mpt::osinfo::windows::System system; mpt::osinfo::windows::Build build; const mpt::uchar * name; } versionMap[] =
{
	// clang-format off
	// WinNT 10.0
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 28001, UL_("Windows 11 (future)")    },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 28000, UL_("Windows 11 26H1")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 26200, UL_("Windows 11 25H2")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 26100, UL_("Windows 11 24H2")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 22631, UL_("Windows 11 23H2")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 22621, UL_("Windows 11 22H2")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 22000, UL_("Windows 11 21H2")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 19045, UL_("Windows 10 22H2")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 19044, UL_("Windows 10 21H2")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 19043, UL_("Windows 10 21H1")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 19042, UL_("Windows 10 20H2")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 19041, UL_("Windows 10 2004")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 18363, UL_("Windows 10 1909")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 18362, UL_("Windows 10 1903")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 17763, UL_("Windows 10 1809")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 17134, UL_("Windows 10 1803")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 16299, UL_("Windows 10 1709")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 15063, UL_("Windows 10 1703")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 14393, UL_("Windows 10 1607")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 10586, UL_("Windows 10 1511")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10     , 10240, UL_("Windows 10 1507")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win10Pre  ,     0, UL_("Windows 10 (Preview)")   },
	// WinNT 6.x
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win81     ,     0, UL_("Windows 8.1")            },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win8      ,     0, UL_("Windows 8")              },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win7      ,     0, UL_("Windows 7")              },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::WinVista  ,     0, UL_("Windows Vista")          },
	// WinNT 5.x
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::WinXP64   ,     0, UL_("Windows XP x64")         },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::WinXP     ,     0, UL_("Windows XP")             },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::Win2000   ,     0, UL_("Windows 2000")           },
	// WinNT 4.0
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::WinNT4    ,     0, UL_("Windows NT 4")           },
	// WinNT 3.x
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::WinNT351  ,     0, UL_("Windows NT 3.51")        },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::WinNT35   ,     0, UL_("Windows NT 3.5")         },
	{ mpt::osinfo::windows::Epoch::WinNT , mpt::osinfo::windows::Version::WinNT31   ,     0, UL_("Windows NT 3.1")         },
	// Win9x 4.x
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::WinME     ,  3000, UL_("Windows ME")             },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::WinME     ,     0, UL_("Windows ME (Beta)")      },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::Win98     ,  2222, UL_("Windows 98 SE")          },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::Win98     ,  1998, UL_("Windows 98")             },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::Win98     ,     0, UL_("Windows 98 (Beta)")      },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::Win95C    ,  1216, UL_("Windows 95 C OSR 2.5")   },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::Win95OSR21,   971, UL_("Windows 95 B OSR 2.1")   },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::Win95     ,  1212, UL_("Windows 95 B OSR 2.1")   },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::Win95     ,  1111, UL_("Windows 95 B OSR 2")     },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::Win95     ,  1080, UL_("Windows 95 B (Preview)") },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::Win95     ,   951, UL_("Windows 95 a OSR 1")     },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::Win95     ,   950, UL_("Windows 95")             },
	{ mpt::osinfo::windows::Epoch::Win9x , mpt::osinfo::windows::Version::Win95     ,     0, UL_("Windows 95 (Beta)")      },
	// Win32s 3.x
	{ mpt::osinfo::windows::Epoch::Win32s, mpt::osinfo::windows::Version::Win311    ,     0, UL_("Windows 3.11")           },
	{ mpt::osinfo::windows::Epoch::Win32s, mpt::osinfo::windows::Version::Win31     ,     0, UL_("Windows 3.1")            },
	{ mpt::osinfo::windows::Epoch::Win32s, mpt::osinfo::windows::Version::Win30     ,     0, UL_("Windows 3.0")            },
	// clang-format on
};


mpt::ustring Version::GetName(mpt::osinfo::windows::Version version)
{
	mpt::ustring name = U_("Windows");
	for(const auto &v : versionMap)
	{
		if(version.Is(v.epoch, v.system, v.build))
		{
			name = v.name;
			break;
		}
	}
	name += U_(" (");
	name += MPT_UFORMAT("Version {}.{}")(version.GetSystem().Major, version.GetSystem().Minor);
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
	if(version.Is(mpt::osinfo::windows::Epoch::Win9x) && version.GetInternetExplorer().HasInternetExplorer())
	{
		name += MPT_UFORMAT(" IE {}.{}.{}")(version.GetInternetExplorer().Major, version.GetInternetExplorer().Minor, version.GetInternetExplorer().Build);
	}
	if(version.GetBuild() != 0)
	{
		name += MPT_UFORMAT(" (Build {})")(version.GetBuild());
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
			mpt::OS::Wine::VersionContext v = mpt::OS::Wine::VersionContext::Current().value();
			if(v.IsValid())
			{
				result = MPT_UFORMAT("Wine {} ({})")(
					  mpt::ToUnicode(mpt::Charset::UTF8, v.VersionAsString())
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
	#if MPT_OS_WINDOWS && MPT_COMPILER_MSVC
		#if MPT_MSVC_AT_LEAST(2026, 0)
			return mpt::osinfo::windows::Version(mpt::osinfo::windows::Epoch::WinNT, mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::ServicePack{0, 0}, mpt::osinfo::windows::InternetExplorer{0, 0}, 10240);
		#elif MPT_MSVC_AT_LEAST(2022, 0)
			return mpt::osinfo::windows::Version(mpt::osinfo::windows::Epoch::WinNT, mpt::osinfo::windows::Version::Win7, mpt::osinfo::windows::ServicePack{0, 0}, mpt::osinfo::windows::InternetExplorer{0, 0}, 0);
		#elif MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
			return mpt::osinfo::windows::Version(mpt::osinfo::windows::Epoch::WinNT, mpt::osinfo::windows::Version::WinVista, mpt::osinfo::windows::ServicePack{0, 0}, mpt::osinfo::windows::InternetExplorer{0, 0}, 0);
		#else
			return mpt::osinfo::windows::Version(mpt::osinfo::windows::Epoch::WinNT, mpt::osinfo::windows::Version::WinXP, mpt::osinfo::windows::ServicePack{0, 0}, mpt::osinfo::windows::InternetExplorer{0, 0}, 0);
		#endif
	#else
		return mpt::osinfo::windows::Version(mpt::osinfo::windows::Epoch::Win32s, mpt::osinfo::windows::System{0, 0}, mpt::osinfo::windows::ServicePack{0, 0}, mpt::osinfo::windows::InternetExplorer{0, 0}, 0);
	#endif
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
	if(WindowsVersion.IsAtLeast(mpt::osinfo::windows::Version(mpt::osinfo::windows::Epoch::WinNT, mpt::osinfo::windows::Version::Win10, mpt::osinfo::windows::ServicePack(0, 0), mpt::osinfo::windows::InternetExplorer(0, 0), 16299)))
	{
		std::optional<mpt::library> kernel32{mpt::library::load_optional({mpt::library::path_search::system, mpt::library::path_prefix::none, MPT_NATIVE_PATH("kernel32.dll"), mpt::library::path_suffix::none})};
		mpt::library::optional_function<BOOL WINAPI(HANDLE, USHORT *, USHORT *)> IsWow64Process2{kernel32, "IsWow64Process2"};
		USHORT ProcessMachine = 0;
		USHORT NativeMachine = 0;
		if(IsWow64Process2(GetCurrentProcess(), &ProcessMachine, &NativeMachine).value_or(FALSE) != FALSE)
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
		: SystemIsWine(mpt::osinfo::windows::wine::current_is_wine())
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



mpt::osinfo::windows::wine::version GetMinimumWineVersion()
{
	mpt::osinfo::windows::wine::version minimumWineVersion = mpt::osinfo::windows::wine::version{0,0,0};
	#if MPT_OS_WINDOWS && MPT_COMPILER_MSVC
		minimumWineVersion = mpt::osinfo::windows::wine::version{1,8,0};
	#endif
	return minimumWineVersion;
}


} // namespace Wine
} // namespace OS
} // namespace mpt


OPENMPT_NAMESPACE_END
