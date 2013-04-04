/*
 * MixerSettings.cpp
 * -----------------
 * Purpose: A struct containing settings for the mixer of soundlib.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "MixerSettings.h"

MixerSettings::MixerSettings()
{
	glVolumeRampUpSamples = 16;
	glVolumeRampDownSamples = 42;
}
