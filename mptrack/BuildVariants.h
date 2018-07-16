/*
 * BuildVariants.h
 * ---------------
 * Purpose: Handling of various OpenMPT build variants.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "../common/mptCPU.h"
#include "../common/mptOS.h"


OPENMPT_NAMESPACE_BEGIN


struct BuildVariant
{
	uint8 Score;
	mpt::ustring Name;
	bool Modern;
	mpt::Windows::Architecture Architecture;
	uint32 MinimumProcSupportFlags;
	int MinimumSSE;
	int MinimumAVX;
	mpt::Windows::Version::System MinimumWindowsKernel;
	mpt::Windows::Version::System MinimumWindowsAPI;
	mpt::Wine::Version MinimumWine;
};

class BuildVariants
{

public:

	static bool CanRunBuild(BuildVariant build);

	static BuildVariant GetCurrentBuildVariant();
	static std::vector<BuildVariant> GetBuildVariants();

	static std::vector<BuildVariant> GetSupportedBuilds();
	static std::vector<BuildVariant> GetRecommendedBuilds();
	static std::vector<mpt::ustring> GetBuildNames(std::vector<BuildVariant> builds);

	static bool IsKnownSystem();

	static bool CurrentBuildIsModern();
	static mpt::ustring GuessCurrentBuildName();

	static bool ProcessorCanRunCurrentBuild();
	static bool SystemCanRunModernBuilds();

	static mpt::PathString GetComponentArch();

};


OPENMPT_NAMESPACE_END
