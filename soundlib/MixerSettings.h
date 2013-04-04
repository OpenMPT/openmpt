/*
 * MixerSettings.h
 * ---------------
 * Purpose: A struct containing settings for the mixer of soundlib.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once


#define SOUNDSETUP_ENABLEMMX	0x08
#define SOUNDSETUP_SOFTPANNING	0x10
#define SOUNDSETUP_SECONDARY	0x40
#define SOUNDSETUP_RESTARTMASK	SOUNDSETUP_SECONDARY


struct MixerSettings
{

	UINT m_nStereoSeparation;
	UINT m_nMaxMixChannels;
	DWORD gdwSoundSetup, gdwMixingFreq, gnBitsPerSample, gnChannels;

	//rewbs.resamplerConf
	long glVolumeRampUpSamples, glVolumeRampDownSamples;
	//end rewbs.resamplerConf

	MixerSettings();

};
