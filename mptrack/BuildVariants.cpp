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
#include "../common/mptOS.h"
#include "../misc/mptCPU.h"
#include "Mptrack.h"


OPENMPT_NAMESPACE_BEGIN


bool BuildVariants::IsKnownSystem()
{
	return false
		|| mpt::OS::Windows::IsOriginal()
		|| (mpt::OS::Windows::IsWine() && theApp.GetWineVersion()->Version().IsValid())
		;
}


bool BuildVariants::CurrentBuildIsModern()
{
	return false
		|| (CPU::GetMinimumSSEVersion() > 2)
		|| (CPU::GetMinimumAVXVersion() > 0)
		|| (mpt::OS::Windows::Version::GetMinimumKernelLevel() > mpt::OS::Windows::Version::Win7)
		|| (mpt::OS::Windows::Version::GetMinimumAPILevel() > mpt::OS::Windows::Version::Win7)
		;
}


mpt::ustring BuildVariants::GuessCurrentBuildName()
{
	if(mpt::OS::Windows::GetProcessArchitecture() == mpt::OS::Windows::Architecture::amd64)
	{
		if(CurrentBuildIsModern())
		{
			return U_("win64");
		} else
		{
			return U_("win64old");
		}
	} else if(mpt::OS::Windows::GetProcessArchitecture() == mpt::OS::Windows::Architecture::x86)
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
	if(mpt::OS::Windows::HostCanRun(mpt::OS::Windows::GetHostArchitecture(), mpt::OS::Windows::GetProcessArchitecture()) == mpt::OS::Windows::EmulationLevel::NA)
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
		if(mpt::OS::Windows::IsOriginal())
		{
			if(mpt::OS::Windows::Version::Current().IsBefore(mpt::OS::Windows::Version::GetMinimumKernelLevel()))
			{
				return false;
			}
			if(mpt::OS::Windows::Version::Current().IsBefore(mpt::OS::Windows::Version::GetMinimumAPILevel()))
			{
				return false;
			}
		} else if(mpt::OS::Windows::IsWine())
		{
			if(theApp.GetWineVersion()->Version().IsBefore(mpt::OS::Wine::GetMinimumWineVersion()))
			{
				return false;
			}
		}
	}
	return true;
}


OPENMPT_NAMESPACE_END
