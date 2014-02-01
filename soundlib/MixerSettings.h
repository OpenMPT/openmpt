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

	int32 VolumeRampUpMicroseconds;
	int32 VolumeRampDownMicroseconds;
	int32 GetVolumeRampUpMicroseconds() const { return VolumeRampUpMicroseconds; }
	int32 GetVolumeRampDownMicroseconds() const { return VolumeRampDownMicroseconds; }
	void SetVolumeRampUpMicroseconds(int32 rampUpMicroseconds) { VolumeRampUpMicroseconds = rampUpMicroseconds; }
	void SetVolumeRampDownMicroseconds(int32 rampDownMicroseconds) { VolumeRampDownMicroseconds = rampDownMicroseconds; }
	
	int32 GetVolumeRampUpSamples() const;
	int32 GetVolumeRampDownSamples() const;

	void SetVolumeRampUpSamples(int32 rampUpSamples);
	void SetVolumeRampDownSamples(int32 rampDownSamples);
	
	bool IsValid() const
	{
		return (gdwMixingFreq > 0) && (gnChannels == 1 || gnChannels == 2 || gnChannels == 4);
	}
	
	MixerSettings();

};
