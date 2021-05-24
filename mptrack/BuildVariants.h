/*
 * BuildVariants.h
 * ---------------
 * Purpose: Handling of various OpenMPT build variants.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"


OPENMPT_NAMESPACE_BEGIN


class BuildVariants
{

public:

	enum Variants {
		Standard,
		Legacy,
		Retro,
		Unknown,
	};

	static bool IsKnownSystem();

	static BuildVariants::Variants GetBuildVariant();
	static mpt::ustring GetBuildVariantName(BuildVariants::Variants variant);
	static mpt::ustring GetBuildVariantDescription(BuildVariants::Variants variant);

	static mpt::ustring GuessCurrentBuildName();

	static bool ProcessorCanRunCurrentBuild();
	static bool SystemCanRunCurrentBuild();

};


OPENMPT_NAMESPACE_END
