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
#include "../common/misc_util.h"

MixerSettings::MixerSettings()
{

	// SNDMIX: These are global flags for playback control
	m_nStereoSeparation = 128;
	m_nMaxMixChannels = MAX_CHANNELS;

	DSPMask = 0;
	MixerFlags = 0;

	// Mixing Configuration
	gnChannels = 2;
	gdwMixingFreq = 44100;

	m_nPreAmp = 128;

	glVolumeRampUpSamples = 16;
	glVolumeRampDownSamples = 42;

}


int32 MixerSettings::GetVolumeRampUpMicroseconds() const
{
	return Util::muldivr(glVolumeRampUpSamples, 1000000, gdwMixingFreq);
}


int32 MixerSettings::GetVolumeRampDownMicroseconds() const
{
	return Util::muldivr(glVolumeRampDownSamples, 1000000, gdwMixingFreq);
}


void MixerSettings::SetVolumeRampUpMicroseconds(int32 rampUpMicroseconds)
{
	glVolumeRampUpSamples = Util::muldivr(rampUpMicroseconds, gdwMixingFreq, 1000000);
}


void MixerSettings::SetVolumeRampDownMicroseconds(int32 rampDownMicroseconds)
{
	glVolumeRampDownSamples = Util::muldivr(rampDownMicroseconds, gdwMixingFreq, 1000000);
}
