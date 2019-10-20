/*
* Paula.h
* -------
* Purpose: Emulating the Amiga's sound chip, Paula, by implementing resampling using band-limited steps (BLEPs)
* Notes  : (currently none)
* Authors: OpenMPT Devs
*          Antti S. Lankila
* The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
*/

#pragma once

#include "BuildSettings.h"

#include "Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

namespace Paula
{

constexpr int PAULA_HZ = 3546895;
constexpr int MINIMUM_INTERVAL = 16;
constexpr int BLEP_SCALE = 17;
constexpr int BLEP_SIZE = 2048;
constexpr uint16 MAX_BLEPS = BLEP_SIZE / MINIMUM_INTERVAL;

class State
{
	struct Blep
	{
		int16 level;
		uint16 age;
	};

public:
	SamplePosition remainder, stepRemainder;
	int numSteps;  // Number of full-length steps
private:
	uint16 activeBleps = 0, firstBlep = 0;  // Count of simultaneous bleps to keep track of
	int16 globalOutputLevel = 0;            // The instantenous value of Paula output
	Blep blepState[MAX_BLEPS];

public:
	State(uint32 sampleRate = 48000);

	void Reset();
	void InputSample(int16 sample);
	int OutputSample(bool filter);
	void Clock(int cycles);
};

}

OPENMPT_NAMESPACE_END
