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
#include "Snd_defs.h"

MixerSettings::MixerSettings()
{

	// SNDMIX: These are global flags for playback control
	m_nStereoSeparation = 128;
	m_nMaxMixChannels = MAX_CHANNELS;
	// Mixing Configuration (SetWaveConfig)
	gnChannels = 2;
	gdwSoundSetup = SOUNDSETUP_SECONDARY;
	gdwMixingFreq = 44100;
	gnBitsPerSample = 16;

	glVolumeRampUpSamples = 16;
	glVolumeRampDownSamples = 42;
}
