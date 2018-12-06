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
#include "Mptrack.h"


OPENMPT_NAMESPACE_BEGIN


BuildVariant BuildVariants::GetCurrentBuildVariant()
{
	BuildVariant result = { 0
		, GuessCurrentBuildName()
		, CurrentBuildIsModern()
		, mpt::Windows::GetProcessArchitecture()
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
	if(a.Score > b.Score)
	{
		return true;
	}
	if(a.Score < b.Score)
	{
		return false;
	}
	if(mpt::Windows::Bitness(a.Architecture) > mpt::Windows::Bitness(b.Architecture))
	{
		return true;
	}
	if(mpt::Windows::Bitness(a.Architecture) < mpt::Windows::Bitness(b.Architecture))
	{
		return false;
	}
	return false;
}


std::vector<BuildVariant> BuildVariants::GetBuildVariants()
{
	std::vector<BuildVariant> result
	{
		// VS2015
#ifdef ENABLE_ASM
		{ 1, U_("win32old"), false, mpt::Windows::Architecture::x86  , PROCSUPPORT_i586    , 0, 0, mpt::Windows::Version::WinXP   , mpt::Windows::Version::WinXP  , mpt::Wine::Version(1,8,0) },
		{ 1, U_("win64old"), false, mpt::Windows::Architecture::amd64, PROCSUPPORT_AMD64   , 2, 0, mpt::Windows::Version::WinXP64 , mpt::Windows::Version::WinXP64, mpt::Wine::Version(1,8,0) },
		{ 2, U_("win32"   ), true , mpt::Windows::Architecture::x86  , PROCSUPPORT_x86_SSE2, 2, 0, mpt::Windows::Version::WinVista, mpt::Windows::Version::Win7   , mpt::Wine::Version(1,8,0) },
		{ 2, U_("win64"   ), true , mpt::Windows::Architecture::amd64, PROCSUPPORT_AMD64   , 2, 0, mpt::Windows::Version::WinVista, mpt::Windows::Version::Win7   , mpt::Wine::Version(1,8,0) },
#else
		{ 1, U_("win32old"), false, mpt::Windows::Architecture::x86  , 0                   , 0, 0, mpt::Windows::Version::WinXP   , mpt::Windows::Version::WinXP  , mpt::Wine::Version(1,8,0) },
		{ 1, U_("win64old"), false, mpt::Windows::Architecture::amd64, 0                   , 2, 0, mpt::Windows::Version::WinXP64 , mpt::Windows::Version::WinXP64, mpt::Wine::Version(1,8,0) },
		{ 2, U_("win32"   ), true , mpt::Windows::Architecture::x86  , 0                   , 2, 0, mpt::Windows::Version::WinVista, mpt::Windows::Version::Win7   , mpt::Wine::Version(1,8,0) },
		{ 2, U_("win64"   ), true , mpt::Windows::Architecture::amd64, 0                   , 2, 0, mpt::Windows::Version::WinVista, mpt::Windows::Version::Win7   , mpt::Wine::Version(1,8,0) },
#endif
	};
	std::stable_sort(result.begin(), result.end(), CompareBuildVariantsByScore);
	return result;
}


std::vector<BuildVariant> BuildVariants::GetSupportedBuilds()
{
	std::vector<BuildVariant> result;
	if(IsKnownSystem())
	{
		auto builds = GetBuildVariants();
		for(const auto &b : builds)
		{
			if(CanRunBuild(b))
			{
				result.push_back(b);
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
		auto builds = GetSupportedBuilds();
		uint8 maxScore = 0;
		for(const auto &b : builds)
		{
			maxScore = std::max(maxScore, b.Score);
		}
		for(const auto &b : builds)
		{
			if(b.Score == maxScore)
			{
				result.push_back(b);
			}
		}
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


bool BuildVariants::CurrentBuildIsModern()
{
	if(mpt::Windows::GetProcessArchitecture() == mpt::Windows::Architecture::amd64)
	{
		return false
			|| (GetMinimumSSEVersion() > 2)
			|| (GetMinimumAVXVersion() > 0)
			|| (mpt::Windows::Version::GetMinimumKernelLevel() > mpt::Windows::Version::WinXP64)
			|| (mpt::Windows::Version::GetMinimumAPILevel() > mpt::Windows::Version::WinXP64)
			;
	} else if(mpt::Windows::GetProcessArchitecture() == mpt::Windows::Architecture::x86)
	{
		return false
			|| (GetMinimumSSEVersion() > 0)
			|| (GetMinimumAVXVersion() > 0)
			|| (mpt::Windows::Version::GetMinimumKernelLevel() > mpt::Windows::Version::WinXP)
			|| (mpt::Windows::Version::GetMinimumAPILevel() > mpt::Windows::Version::WinXP)
			;
	} else
	{
		return true;
	}
}


mpt::ustring BuildVariants::GuessCurrentBuildName()
{
	if(mpt::Windows::GetProcessArchitecture() == mpt::Windows::Architecture::amd64)
	{
		if(CurrentBuildIsModern())
		{
			return U_("win64");
		} else
		{
			return U_("win64old");
		}
	} else if(mpt::Windows::GetProcessArchitecture() == mpt::Windows::Architecture::x86)
	{
		if(CurrentBuildIsModern())
		{
			return U_("win32");
		} else
		{
			return U_("win32old");
		}
	} else
	{
		return mpt::ustring();
	}
}


bool BuildVariants::SystemCanRunModernBuilds()
{
	auto builds = GetBuildVariants();
	for(const auto &b : builds)
	{
		if(b.Modern && CanRunBuild(b))
		{
			return true;
		}
	}
	return false;
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
{
	BuildVariant build = GetCurrentBuildVariant();
#ifdef ENABLE_ASM
	if((GetRealProcSupport() & build.MinimumProcSupportFlags) != build.MinimumProcSupportFlags) return false;
	if(build.MinimumSSE >= 1)
	{
		if(!(GetRealProcSupport() & PROCSUPPORT_SSE)) return false;
	}
	if(build.MinimumSSE >= 2)
	{
		if(!(GetRealProcSupport() & PROCSUPPORT_SSE2)) return false;
	}
#endif
	return true;
}


bool BuildVariants::CanRunBuild(BuildVariant build) 
{
	if(mpt::Windows::HostCanRun(mpt::Windows::GetHostArchitecture(), build.Architecture) == mpt::Windows::EmulationLevel::NA)
	{
		return false;
	}
#ifdef ENABLE_ASM
	if((GetRealProcSupport() & build.MinimumProcSupportFlags) != build.MinimumProcSupportFlags)
	{
		return false;
	}
	if(build.MinimumSSE >= 1)
	{
		if(!(GetRealProcSupport() & PROCSUPPORT_SSE))
		{
			return false;
		}
	}
	if(build.MinimumSSE >= 2)
	{
		if(!(GetRealProcSupport() & PROCSUPPORT_SSE2))
		{
			return false;
		}
	}
#endif
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
	return mpt::PathString::FromUnicode(mpt::Windows::Name(mpt::Windows::GetProcessArchitecture()));
}


OPENMPT_NAMESPACE_END
