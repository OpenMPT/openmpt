/*
 * MixerSettings.h
 * ---------------
 * Purpose: A struct containing settings for the mixer of soundlib.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once


enum SampleFormatEnum
{
	SampleFormatUnsigned8 =  8,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt16     = 16,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt24     = 24,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt32     = 32,       // do not change value (for compatibility with old configuration settings)
	SampleFormatFloat32   = 32 + 128, // Only supported as mixer output and NOT supported by Mod2Wave or ISoundDevice or settings dialog yet. Keep in mind to update all 3 cases at once.
	SampleFormatInt28q4   = 255,      // mixbuffer format
	SampleFormatInvalid   =  0
};

struct SampleFormat
{
	SampleFormatEnum value;
	SampleFormat(SampleFormatEnum v = SampleFormatInvalid) : value(v) { }
	bool operator == (SampleFormat other) const { return value == other.value; }
	bool operator != (SampleFormat other) const { return value != other.value; }
	operator SampleFormatEnum () const
	{
		return value;
	}
	bool IsValid() const
	{
		return value != SampleFormatInvalid;
	}
	bool IsUnsigned() const
	{
		if(!IsValid()) return false;
		return value == SampleFormatUnsigned8;
	}
	bool IsFloat() const
	{
		if(!IsValid()) return false;
		return value == SampleFormatFloat32;
	}
	bool IsInt() const
	{
		if(!IsValid()) return false;
		return value != SampleFormatFloat32;
	}
	bool IsMixBuffer() const
	{
		if(!IsValid()) return false;
		return value == SampleFormatInt28q4;
	}
	uint8 GetBitsPerSample() const
	{
		if(!IsValid()) return 0;
		switch(value)
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
		case SampleFormatInt28q4:
			return 32;
			break;
		default:
			return 0;
			break;
		}
	}

	// backward compatibility, conversion to/from integers
	operator int () const { return value; }
	SampleFormat(int v) : value(SampleFormatEnum(v)) { }
	operator long () const { return value; }
	SampleFormat(long v) : value(SampleFormatEnum(v)) { }
	operator unsigned int () const { return value; }
	SampleFormat(unsigned int v) : value(SampleFormatEnum(v)) { }
	operator unsigned long () const { return value; }
	SampleFormat(unsigned long v) : value(SampleFormatEnum(v)) { }
};

struct MixerSettings
{

	UINT m_nStereoSeparation;
	UINT m_nMaxMixChannels;
	DWORD DSPMask;
	DWORD MixerFlags;
	DWORD gdwMixingFreq;
	DWORD gnChannels;
	DWORD m_nPreAmp;

	//rewbs.resamplerConf
	long glVolumeRampUpSamples, glVolumeRampDownSamples;
	//end rewbs.resamplerConf
	
	int32 GetVolumeRampUpMicroseconds() const;
	int32 GetVolumeRampDownMicroseconds() const;
	void SetVolumeRampUpMicroseconds(int32 rampUpMicroseconds);
	void SetVolumeRampDownMicroseconds(int32 rampDownMicroseconds);

#ifndef MODPLUG_TRACKER
	DWORD m_FinalOutputGain; // factor multiplied to the final mixer output just before clipping and dithering, fixed point 16.16
#endif

	bool IsValid() const
	{
		return true
			&& (gnChannels == 1 || gnChannels == 2 || gnChannels == 4)
			&& (gdwMixingFreq > 0)
			;
	}
	
	MixerSettings();

};
