/*
 * ModSample.h
 * -----------
 * Purpose: Module Sample header class and helpers
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

// Sample Struct
struct ModSample
{
	UINT nLength, nLoopStart, nLoopEnd;	// In samples, not bytes
	UINT nSustainStart, nSustainEnd;	// Dito
	LPSTR pSample;						// Pointer to sample data
	UINT nC5Speed;						// Frequency of middle c, in Hz (for IT/S3M/MPTM)
	WORD nPan;							// Default sample panning (if pan flag is set)
	WORD nVolume;						// Default volume
	WORD nGlobalVol;					// Global volume (sample volume is multiplied by this)
	WORD uFlags;						// Sample flags
	signed char RelativeTone;			// Relative note to middle c (for MOD/XM)
	signed char nFineTune;				// Finetune period (for MOD/XM)
	BYTE nVibType;						// Auto vibrato type
	BYTE nVibSweep;						// Auto vibrato sweep (i.e. how long it takes until the vibrato effect reaches its full strength)
	BYTE nVibDepth;						// Auto vibrato depth
	BYTE nVibRate;						// Auto vibrato rate (speed)
	//CHAR name[MAX_SAMPLENAME];		// Maybe it would be nicer to have sample names here, but that would require some refactoring. Also, would this slow down the mixer (cache misses)?
	CHAR filename[MAX_SAMPLEFILENAME];

	// Return the size of one (elementary) sample in bytes.
	uint8 GetElementarySampleSize() const { return (uFlags & CHN_16BIT) ? 2 : 1; }

	// Return the number of channels in the sample.
	uint8 GetNumChannels() const { return (uFlags & CHN_STEREO) ? 2 : 1; }

	// Return the number of bytes per sampling point. (Channels * Elementary Sample Size)
	uint8 GetBytesPerSample() const { return GetElementarySampleSize() * GetNumChannels(); }

	// Return the size which pSample is at least.
	SmpLength GetSampleSizeInBytes() const { return nLength * GetBytesPerSample(); }

	// Returns sample rate of the sample. The argument is needed because 
	// the sample rate is obtained differently for different module types.
	uint32 GetSampleRate(const MODTYPE type) const;

	// Translate sample properties between two given formats.
	void Convert(MODTYPE fromType, MODTYPE toType);
};
