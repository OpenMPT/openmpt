/*
 * SampleBuffer.h
 * --------------
 * Purpose: 
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "mptBuildSettings.h"

#include "../common/mptBaseMacros.h"
#include "../common/mptBaseTypes.h"


OPENMPT_NAMESPACE_BEGIN


template <typename SampleType>
inline std::size_t valid_channels(SampleType* const* buffers, std::size_t maxChannels)
{
	std::size_t channel;
	for (channel = 0; channel < maxChannels; ++channel)
	{
		if (!buffers[channel])
		{
			break;
		}
	}
	return channel;
}


OPENMPT_NAMESPACE_END
