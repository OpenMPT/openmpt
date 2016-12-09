/*
 * BuildVariants.cpp
 * -----------------
 * Purpose: Handling of various OpenMPT build variants.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "BuildVariants.h"
#include "../common/version.h"
#include "../common/mptCPU.h"
#include "../common/mptOS.h"
#include "mptrack.h"


OPENMPT_NAMESPACE_BEGIN


BuildVariant BuildVariants::GetCurrentBuildVariant()
{
	BuildVariant result = { 0
		, GuessCurrentBuildName()
		, MPT_ARCH_BITS
		, GetMinimumProcSupportFlags()
		, GetMinimumSSEVersion()
		, GetMinimumAVXVersion()
		, mpt::Windows::Version::GetMinimumKernelLevel()
		, mpt::Windows::Version::GetMinimumAPILevel()
		, mpt::Wine::GetMinimumWineVersion()
	};
	return result;
}


static bool CompareBuildVariantsByScore(const BuildVariant & a, const BuildVariant & b)
{
	if (a.Score > b.Score) return true;
	if (a.Score < b.Score) return false;
	if (a.Bitness > b.Bitness) return true;
	if (a.Bitness < b.Bitness) return false;
	return false;
}


std::vector<BuildVariant> BuildVariants::GetBuildVariants()
{
	std::vector<BuildVariant> result;
	#if 0
		// VS2010
		BuildVariant Win32old = { 1, MPT_USTRING("win32old"), 32, PROCSUPPORT_i586    , 0, 0, mpt::Windows::Version::WinXP   , mpt::Windows::Version::WinXP  , mpt::Wine::Version(1,0,0) };
		BuildVariant Win64old = { 1, MPT_USTRING("win64old"), 64, PROCSUPPORT_AMD64   , 2, 0, mpt::Windows::Version::WinXP64 , mpt::Windows::Version::WinXP64, mpt::Wine::Version(1,4,0) };
		BuildVariant Win32    = { 2, MPT_USTRING("win32"   ), 32, PROCSUPPORT_x86_SSE2, 2, 0, mpt::Windows::Version::WinXP   , mpt::Windows::Version::Win7   , mpt::Wine::Version(1,4,0) };
		BuildVariant Win64    = { 2, MPT_USTRING("win64"   ), 64, PROCSUPPORT_AMD64   , 2, 0, mpt::Windows::Version::WinXP64 , mpt::Windows::Version::Win7   , mpt::Wine::Version(1,4,0) };
		result.push_back(Win32old);
		result.push_back(Win64old);
		result.push_back(Win32);
		result.push_back(Win64);
	#else
		// VS2015
		BuildVariant Win32old = { 1, MPT_USTRING("win32old"), 32, PROCSUPPORT_i586    , 0, 0, mpt::Windows::Version::WinXP   , mpt::Windows::Version::WinXP  , mpt::Wine::Version(1,4,0) };
		BuildVariant Win64old = { 1, MPT_USTRING("win64old"), 64, PROCSUPPORT_AMD64   , 2, 0, mpt::Windows::Version::WinXP64 , mpt::Windows::Version::WinXP64, mpt::Wine::Version(1,4,0) };
		BuildVariant Win32    = { 2, MPT_USTRING("win32"   ), 32, PROCSUPPORT_x86_SSE2, 2, 0, mpt::Windows::Version::WinVista, mpt::Windows::Version::Win7   , mpt::Wine::Version(1,8,0) };
		BuildVariant Win64    = { 2, MPT_USTRING("win64"   ), 64, PROCSUPPORT_AMD64   , 2, 0, mpt::Windows::Version::WinVista, mpt::Windows::Version::Win7   , mpt::Wine::Version(1,8,0) };
		result.push_back(Win32old);
		result.push_back(Win64old);
		result.push_back(Win32);
		result.push_back(Win64);
	#endif
	std::stable_sort(result.begin(), result.end(), CompareBuildVariantsByScore);
	return result;
}


BuildVariant BuildVariants::GetModernWin32BuildVariant()
{
	std::vector<BuildVariant> builds = GetBuildVariants();
	MPT_ASSERT(builds[1].Bitness == 32);
	return builds[1];
}


BuildVariant BuildVariants::GetModernWin64BuildVariant()
{
	std::vector<BuildVariant> builds = GetBuildVariants();
	MPT_ASSERT(builds[0].Bitness == 64);
	return builds[0];
}


std::vector<BuildVariant> BuildVariants::GetRecommendedWin32Build()
{
	std::vector<BuildVariant> result;
	if(IsKnownSystem())
	{
		auto builds = GetBuildVariants();
		for(const auto &b : builds)
		{
			if(!(b.Bitness == 32))
			{
				continue;
			}
			if(CanRunBuild(b))
			{
				result.push_back(b);
				break;
			}
		}
	}
	return result;
}


std::vector<BuildVariant> BuildVariants::GetRecommendedWin64Build()
{
	std::vector<BuildVariant> result;
	if(IsKnownSystem())
	{
		auto builds = GetBuildVariants();
		for(const auto &b : builds)
		{
			if(!(b.Bitness == 64))
			{
				continue;
			}
			if(CanRunBuild(b))
			{
				result.push_back(b);
				break;
			}
		}
	}
	return result;
}


std::vector<BuildVariant> BuildVariants::GetRecommendedBuilds()
{
	std::vector<BuildVariant> result;
	if(IsKnownSystem())
	{
		std::vector<BuildVariant> builds;
		builds = GetRecommendedWin32Build();
		result.insert(result.end(), builds.begin(), builds.end());
		builds = GetRecommendedWin64Build();
		result.insert(result.end(), builds.begin(), builds.end());
	}
	return result;
}


bool BuildVariants::IsKnownSystem()
{
	return false
		|| mpt::Windows::IsOriginal()
		|| (mpt::Windows::IsWine() && theApp.GetWineVersion()->Version().IsValid())
		;
}


bool BuildVariants::HostCan64bits()
{
	#if (MPT_ARCH_BITS == 64)
		return true;
	#else
		#if MPT_OS_WINDOWS
			#if (_WIN32_WINNT >= 0x0501)
				BOOL is64 = FALSE;
				if(IsWow64Process(GetCurrentProcess(), &is64) != 0)
				{
					return is64 ? true : false;
				} else
				{
					return false;
				}
			#else
				return false;
			#endif
		#else
			return false;
		#endif
	#endif
}


bool BuildVariants::CurrentBuildIsModern()
{
	#if (MPT_ARCH_BITS == 64)
		return false
			|| (GetMinimumSSEVersion() > 2)
			|| (GetMinimumAVXVersion() > 0)
			|| (mpt::Windows::Version::GetMinimumKernelLevel() > mpt::Windows::Version::WinXP64)
			|| (mpt::Windows::Version::GetMinimumAPILevel() > mpt::Windows::Version::WinXP64)
			;
	#elif (MPT_ARCH_BITS == 32)
		return false
			|| (GetMinimumSSEVersion() > 0)
			|| (GetMinimumAVXVersion() > 0)
			|| (mpt::Windows::Version::GetMinimumKernelLevel() > mpt::Windows::Version::WinXP)
			|| (mpt::Windows::Version::GetMinimumAPILevel() > mpt::Windows::Version::WinXP)
			;
	#else
		return true;
	#endif
}


mpt::ustring BuildVariants::GuessCurrentBuildName()
{
	#if (MPT_ARCH_BITS == 64)
		if(CurrentBuildIsModern())
		{
			return MPT_USTRING("win64");
		} else
		{
			return MPT_USTRING("win64old");
		}
	#elif (MPT_ARCH_BITS == 32)
		if(CurrentBuildIsModern())
		{
			return MPT_USTRING("win32");
		} else
		{
			return MPT_USTRING("win32old");
		}
	#else
		return mpt::ustring();
	#endif
}


bool BuildVariants::SystemCanRunModernBuilds()
{
	return CanRunBuild(GetModernWin64BuildVariant()) || CanRunBuild(GetModernWin32BuildVariant());
}


std::vector<mpt::ustring> BuildVariants::GetBuildNames(std::vector<BuildVariant> builds)
{
	std::vector<mpt::ustring> names;
	for(std::size_t i = 0; i < builds.size(); ++i)
	{
		names.push_back(builds[i].Name);
	}
	return names;
}


bool BuildVariants::ProcessorCanRunCurrentBuild()
//-------------------------------------------
{
	BuildVariant build = GetCurrentBuildVariant();
	if((GetProcSupport() & build.MinimumProcSupportFlags) != build.MinimumProcSupportFlags) return false;
	if(build.MinimumSSE >= 1)
	{
		if (!(GetProcSupport() & PROCSUPPORT_SSE)) return false;
	}
	if(build.MinimumSSE >= 2)
	{
		if(!(GetProcSupport() & PROCSUPPORT_SSE2)) return false;
	}
	return true;
}


bool BuildVariants::CanRunBuild(BuildVariant build) 
{
	if((build.Bitness == 64) && !HostCan64bits())
	{
		return false;
	}
	if((GetProcSupport() & build.MinimumProcSupportFlags) != build.MinimumProcSupportFlags)
	{
		return false;
	}
	if(build.MinimumSSE >= 1)
	{
		if (!(GetProcSupport() & PROCSUPPORT_SSE))
		{
			return false;
		}
	}
	if(build.MinimumSSE >= 2)
	{
		if(!(GetProcSupport() & PROCSUPPORT_SSE2))
		{
			return false;
		}
	}
	if(IsKnownSystem())
	{
		if(mpt::Windows::IsOriginal())
		{
			if (mpt::Windows::Version::Current().IsBefore(build.MinimumWindowsKernel))
			{
				return false;
			}
			if (mpt::Windows::Version::Current().IsBefore(build.MinimumWindowsAPI))
			{
				return false;
			}
		} else if(mpt::Windows::IsWine())
		{
			if (theApp.GetWineVersion()->Version().IsBefore(build.MinimumWine))
			{
				return false;
			}
		}
	}
	return true;
}


mpt::PathString BuildVariants::GetComponentArch()
{
	#if (MPT_ARCH_BITS == 64)
		return MPT_PATHSTRING("x64");
	#elif (MPT_ARCH_BITS == 32)
		return MPT_PATHSTRING("x86");
	#else
		return MPT_PATHSTRING("");
	#endif
}


OPENMPT_NAMESPACE_END
