/*
 * MixerSettings.h
 * ---------------
 * Purpose: A struct containing settings for the mixer of soundlib.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once


struct MixerSettings
{

	UINT m_nStereoSeparation;
	UINT m_nMaxMixChannels;
	DWORD DSPMask;
	DWORD MixerFlags;
	DWORD gdwMixingFreq, gnBitsPerSample, gnChannels;
	DWORD m_nPreAmp;

	//rewbs.resamplerConf
	long glVolumeRampUpSamples, glVolumeRampDownSamples;
	//end rewbs.resamplerConf

	MixerSettings();

};
