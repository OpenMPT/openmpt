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

	static bool ProcessorCanRunCurrentBuild();
	static bool SystemCanRunCurrentBuild();

};


OPENMPT_NAMESPACE_END
