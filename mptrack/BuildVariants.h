/*
 * BuildVariants.h
 * ---------------
 * Purpose: Handling of various OpenMPT build variants.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#include "../common/mptCPU.h"
#include "../common/mptOS.h"


OPENMPT_NAMESPACE_BEGIN


struct BuildVariant
{
	uint8 Score;
	mpt::ustring Name;
	uint8 Bitness;
	uint32 MinimumProcSupportFlags;
	int MinimumSSE;
	int MinimumAVX;
	mpt::Windows::Version::Number MinimumWindowsKernel;
	mpt::Windows::Version::Number MinimumWindowsAPI;
	mpt::Wine::Version MinimumWine;
};

class BuildVariants
{

public:

	static bool CanRunBuild(BuildVariant build);

	static BuildVariant GetCurrentBuildVariant();
	static std::vector<BuildVariant> GetBuildVariants();
	static BuildVariant GetModernWin32BuildVariant();
	static BuildVariant GetModernWin64BuildVariant();

	static std::vector<BuildVariant> GetRecommendedWin32Build();
	static std::vector<BuildVariant> GetRecommendedWin64Build();
	static std::vector<BuildVariant> GetRecommendedBuilds();
	static std::vector<mpt::ustring> GetBuildNames(std::vector<BuildVariant> builds);

	static bool IsKnownSystem();
	static bool HostCan64bits();

	static bool CurrentBuildIsModern();
	static mpt::ustring GuessCurrentBuildName();

	static bool ProcessorCanRunCurrentBuild();
	static bool SystemCanRunModernBuilds();

	static mpt::PathString GetComponentArch();

};

OPENMPT_NAMESPACE_END
