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
#include "../misc/mptOS.h"
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


BuildVariants::Variants BuildVariants::GetBuildVariant()
{
#if defined(MPT_BUILD_RETRO)
	return Retro;
#else
#if defined(_WIN32_WINNT)
#if (_WIN32_WINNT >= _WIN32_WINNT_WINBLUE)
	return Standard;
#else
	return Legacy;
#endif
#else
	return Unknown;
#endif
#endif
}


mpt::ustring BuildVariants::GetBuildVariantName(BuildVariants::Variants variant)
{
	mpt::ustring result;
	switch(variant)
	{
	case Standard:
		result = U_("Standard");
		break;
	case Legacy:
		result = U_("Legacy");
		break;
	case Retro:
		result = U_("Retro");
		break;
	case Unknown:
		result = U_("Unknown");
		break;
	}
	return result;
}


mpt::ustring BuildVariants::GetBuildVariantDescription(BuildVariants::Variants variant)
{
	mpt::ustring result;
	switch(variant)
	{
	case Standard:
		result = U_("");
		break;
	case Legacy:
		result = U_("");
		break;
	case Retro:
		result = U_(" RETRO");
		break;
	case Unknown:
		result = U_("");
		break;
	}
	return result;
}


mpt::ustring BuildVariants::GuessCurrentBuildName()
{
	if(GetBuildVariant() == Unknown)
	{
		return mpt::ustring();
	}
	if(mpt::arch_bits == 64)
	{
		if(GetBuildVariant() == Standard)
		{
			return U_("win64");
		} else
		{
			return U_("win64old");
		}
	} else if(mpt::arch_bits == 32)
	{
		if(GetBuildVariant() == Standard)
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
	if((CPU::Info::Get().AvailableFeatures & CPU::GetMinimumFeatures()) != CPU::GetMinimumFeatures())
	{
		return false;
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
	if((CPU::Info::Get().AvailableFeatures & CPU::GetMinimumFeatures()) != CPU::GetMinimumFeatures())
	{
		return false;
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
