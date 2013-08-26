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
	DWORD gdwMixingFreq;
	DWORD gnChannels;
	DWORD m_nPreAmp;

	long glVolumeRampUpSamples;
	long glVolumeRampDownSamples;
	
	int32 GetVolumeRampUpMicroseconds() const;
	int32 GetVolumeRampDownMicroseconds() const;
	void SetVolumeRampUpMicroseconds(int32 rampUpMicroseconds);
	void SetVolumeRampDownMicroseconds(int32 rampDownMicroseconds);

	bool IsValid() const
	{
		return (gdwMixingFreq > 0) && (gnChannels == 1 || gnChannels == 2 || gnChannels == 4);
	}
	
	MixerSettings();

};
