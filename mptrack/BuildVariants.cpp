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
#include "../common/mptStringFormat.h"
#include "Mptrack.h"


OPENMPT_NAMESPACE_BEGIN


bool BuildVariants::ProcessorCanRunCurrentBuild()
{
#ifdef MPT_ENABLE_ARCH_INTRINSICS
	if((CPU::Info::Get().AvailableFeatures & CPU::GetMinimumFeatures()) != CPU::GetMinimumFeatures())
	{
		return false;
	}
#endif // MPT_ENABLE_ARCH_INTRINSICS
	return true;
}


bool BuildVariants::SystemCanRunCurrentBuild() 
{
	if(mpt::OS::Windows::IsOriginal())
	{
		if(mpt::osinfo::windows::Version::Current().IsBefore(mpt::OS::Windows::Version::GetMinimumKernelLevel()))
		{
			return false;
		}
		if(mpt::osinfo::windows::Version::Current().IsBefore(mpt::OS::Windows::Version::GetMinimumAPILevel()))
		{
			return false;
		}
	} else if(mpt::OS::Windows::IsWine() && theApp.GetWineVersion()->Version().IsValid())
	{
		if(theApp.GetWineVersion()->Version().IsBefore(mpt::OS::Wine::GetMinimumWineVersion()))
		{
			return false;
		}
	}
	return true;
}


OPENMPT_NAMESPACE_END
