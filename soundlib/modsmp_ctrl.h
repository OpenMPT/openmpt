/*
 * ModSmp_Ctrl.h
 * -------------
 * Purpose: Basic sample editing code (resizing, adding silence, normalizing, ...).
 * Notes  : Could be merged with ModSample.h / ModSample.cpp at some point.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "Sndfile.h"

namespace ctrlSmp
{

enum ResetFlag
{
	SmpResetCompo = 1,
	SmpResetInit,
	SmpResetVibrato,
};

// Insert silence to given location.
// Note: Is currently implemented only for inserting silence to the beginning and to the end of the sample.
// Return: Length of the new sample.
SmpLength InsertSilence(ModSample &smp, const SmpLength nSilenceLength, const SmpLength nStartFrom, CSoundFile* pSndFile);

// Change sample size. 
// Note: If resized sample is bigger, silence will be added to the sample's tail.
// Return: Length of the new sample.
SmpLength ResizeSample(ModSample &smp, const SmpLength nNewLength, CSoundFile* pSndFile);

// Replaces sample in 'smp' with given sample and frees the old sample.
// If valid CSoundFile pointer is given, the sample will be replaced also from the sounds channels.
void ReplaceSample(ModSample &smp, const LPSTR pNewSample,  const SmpLength nNewLength, CSoundFile* pSndFile);

bool AdjustEndOfSample(ModSample &smp, CSoundFile *pSndFile = nullptr);

// Returns the number of bytes allocated(at least) for sample data.
// Note: Currently the return value is based on the sample length and the actual 
//       allocation may be more than what this function returns.
inline SmpLength GetSampleCapacity(ModSample &smp) {return smp.GetSampleSizeInBytes();}

// Resets samples.
void ResetSamples(CSoundFile &rSndFile, ResetFlag resetflag, SAMPLEINDEX minSample = SAMPLEINDEX_INVALID, SAMPLEINDEX maxSample = SAMPLEINDEX_INVALID);

// Remove DC offset and normalize.
// Return: If DC offset was removed, returns original offset value, zero otherwise.
float RemoveDCOffset(ModSample &smp,
					 SmpLength iStart,		// Start position (for partial DC offset removal).
					 SmpLength iEnd,		// End position (for partial DC offset removal).
					 const MODTYPE modtype,	// Used to determine whether to adjust global or default volume
											// to keep volume level the same given the normalization.
											// Volume adjustment is not done if this param is MOD_TYPE_NONE.
					 CSoundFile* const pSndFile); // Passed to AdjustEndOfSample.

// Amplify / fade  sample data
bool AmplifySample(ModSample &smp, SmpLength iStart, SmpLength iEnd, CSoundFile *pSndFile, double amplifyStart, double amplifyEnd);

// Reverse sample data
bool ReverseSample(ModSample &smp, SmpLength iStart, SmpLength iEnd, CSoundFile *pSndFile);

// Virtually unsign sample data
bool UnsignSample(ModSample &smp, SmpLength iStart, SmpLength iEnd, CSoundFile *pSndFile);

// Invert sample data (flip by 180 degrees)
bool InvertSample(ModSample &smp, SmpLength iStart, SmpLength iEnd, CSoundFile *pSndFile);

// Detect whether to enable smart sample ramping.
bool EnableSmartSampleRamping(const ModSample &smp, SmpLength sampleOffset, const CSoundFile *pSndFile);

// Crossfade sample data to create smooth loops
bool XFadeSample(ModSample &smp, SmpLength iFadeLength, CSoundFile *pSndFile);

// Convert a sample with any number of channels to mono
bool ConvertToMono(ModSample &smp, CSoundFile *pSndFile);

} // Namespace ctrlSmp

namespace ctrlChn
{

// Replaces sample from sound channels by given sample.
void ReplaceSample( ModChannel (&Chn)[MAX_CHANNELS],
					LPCSTR pOldSample,
					LPSTR pNewSample,
					const SmpLength nNewLength,
					FlagSet<ChannelFlags> setFlags,
					FlagSet<ChannelFlags> resetFlags);

} // namespace ctrlChn
