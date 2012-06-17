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
	SmpLength nLength;						// In samples, not bytes
	SmpLength nLoopStart, nLoopEnd;			// Dito
	SmpLength nSustainStart, nSustainEnd;	// Dito
	LPSTR pSample;							// Pointer to sample data
	uint32 nC5Speed;						// Frequency of middle c, in Hz (for IT/S3M/MPTM)
	uint16 nPan;							// Default sample panning (if pan flag is set)
	uint16 nVolume;							// Default volume
	uint16 nGlobalVol;						// Global volume (sample volume is multiplied by this)
	uint16 uFlags;							// Sample flags
	int8   RelativeTone;					// Relative note to middle c (for MOD/XM)
	int8   nFineTune;						// Finetune period (for MOD/XM)
	uint8  nVibType;						// Auto vibrato type
	uint8  nVibSweep;						// Auto vibrato sweep (i.e. how long it takes until the vibrato effect reaches its full strength)
	uint8  nVibDepth;						// Auto vibrato depth
	uint8  nVibRate;						// Auto vibrato rate (speed)
	//char name[MAX_SAMPLENAME];			// Maybe it would be nicer to have sample names here, but that would require some refactoring. Also, would this slow down the mixer (cache misses)?
	char filename [MAX_SAMPLEFILENAME];

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

	// Initialize sample slot with default values.
	void Initialize(MODTYPE type = MOD_TYPE_NONE);

	// Allocate sample based on a ModSample's properties.
	// Returns number of bytes allocated, 0 on failure.
	size_t AllocateSample();

	void FreeSample();

	// Transpose <-> Frequency conversions
	static uint32 TransposeToFrequency(int transpose, int finetune = 0);
	void TransposeToFrequency();
	static int FrequencyToTranspose(uint32 freq);
	void FrequencyToTranspose();
};
