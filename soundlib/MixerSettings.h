/*
 * MixerSettings.h
 * ---------------
 * Purpose: A struct containing settings for the mixer of soundlib.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once


enum SampleFormat
{
	SampleFormatUnsigned8 =  8,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt16     = 16,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt24     = 24,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt32     = 32,       // do not change value (for compatibility with old configuration settings)
	SampleFormatFloat32   = 32 + 128, // Only supported as mixer output and NOT supported by Mod2Wave or ISoundDevice or settings dialog yet. Keep in mind to update all 3 cases at once.
	SampleFormatInvalid   =  0
};

struct MixerSettings
{

	UINT m_nStereoSeparation;
	UINT m_nMaxMixChannels;
	DWORD DSPMask;
	DWORD MixerFlags;
	DWORD gdwMixingFreq;
	SampleFormat m_SampleFormat;
	DWORD gnChannels;
	DWORD m_nPreAmp;

	//rewbs.resamplerConf
	long glVolumeRampUpSamples, glVolumeRampDownSamples;
	//end rewbs.resamplerConf

	bool IsUnsignedSampleFormat() const
	{
		return m_SampleFormat == SampleFormatUnsigned8;
	}
	bool IsFloatSampleFormat() const
	{
		return m_SampleFormat == SampleFormatFloat32;
	}
	uint8 GetBitsPerSample() const
	{
		switch(m_SampleFormat)
		{
		case SampleFormatUnsigned8:
			return 8;
			break;
		case SampleFormatInt16:
			return 16;
			break;
		case SampleFormatInt24:
			return 24;
			break;
		case SampleFormatInt32:
			return 32;
			break;
		case SampleFormatFloat32:
			return 32;
			break;
		default: return 8; break;
		}
	}

	MixerSettings();

};
