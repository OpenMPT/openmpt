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


bool BuildVariants::IsKnownSystem()
{
	return false
		|| mpt::Windows::IsOriginal()
		|| (mpt::Windows::IsWine() && theApp.GetWineVersion()->Version().IsValid())
		;
}


bool BuildVariants::CurrentBuildIsModern()
{
	return false
		|| (CPU::GetMinimumSSEVersion() > 2)
		|| (CPU::GetMinimumAVXVersion() > 0)
		|| (mpt::Windows::Version::GetMinimumKernelLevel() > mpt::Windows::Version::Win7)
		|| (mpt::Windows::Version::GetMinimumAPILevel() > mpt::Windows::Version::Win7)
		;
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


bool BuildVariants::ProcessorCanRunCurrentBuild()
{
#ifdef ENABLE_ASM
	if((CPU::GetAvailableFeatures() & CPU::GetMinimumFeatures()) != CPU::GetMinimumFeatures()) return false;
	if(CPU::GetMinimumSSEVersion() >= 1)
	{
		if(!(CPU::GetAvailableFeatures() & CPU::feature::sse))
		{
			return false;
		}
	}
	if(CPU::GetMinimumSSEVersion() >= 2)
	{
		if(!(CPU::GetAvailableFeatures() & CPU::feature::sse2))
		{
			return false;
		}
	}
	if(CPU::GetMinimumAVXVersion() >= 1)
	{
		if(!(CPU::GetAvailableFeatures() & CPU::feature::avx))
		{
			return false;
		}
	}
	if(CPU::GetMinimumAVXVersion() >= 2)
	{
		if(!(CPU::GetAvailableFeatures() & CPU::feature::avx2))
		{
			return false;
		}
	}
#endif
	return true;
}


bool BuildVariants::SystemCanRunCurrentBuild() 
{
	if(mpt::Windows::HostCanRun(mpt::Windows::GetHostArchitecture(), mpt::Windows::GetProcessArchitecture()) == mpt::Windows::EmulationLevel::NA)
	{
		return false;
	}
#ifdef ENABLE_ASM
	if((CPU::GetAvailableFeatures() & CPU::GetMinimumFeatures()) != CPU::GetMinimumFeatures())
	{
		return false;
	}
	if(CPU::GetMinimumSSEVersion() >= 1)
	{
		if(!(CPU::GetAvailableFeatures() & CPU::feature::sse))
		{
			return false;
		}
	}
	if(CPU::GetMinimumSSEVersion() >= 2)
	{
		if(!(CPU::GetAvailableFeatures() & CPU::feature::sse2))
		{
			return false;
		}
	}
	if(CPU::GetMinimumAVXVersion() >= 1)
	{
		if(!(CPU::GetAvailableFeatures() & CPU::feature::avx))
		{
			return false;
		}
	}
	if(CPU::GetMinimumAVXVersion() >= 2)
	{
		if(!(CPU::GetAvailableFeatures() & CPU::feature::avx2))
		{
			return false;
		}
	}
#endif
	if(IsKnownSystem())
	{
		if(mpt::Windows::IsOriginal())
		{
			if(mpt::Windows::Version::Current().IsBefore(mpt::Windows::Version::GetMinimumKernelLevel()))
			{
				return false;
			}
			if(mpt::Windows::Version::Current().IsBefore(mpt::Windows::Version::GetMinimumAPILevel()))
			{
				return false;
			}
		} else if(mpt::Windows::IsWine())
		{
			if(theApp.GetWineVersion()->Version().IsBefore(mpt::Wine::GetMinimumWineVersion()))
			{
				return false;
			}
		}
	}
	return true;
}


OPENMPT_NAMESPACE_END
