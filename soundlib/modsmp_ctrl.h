/*
 * MODINSTRUMENT related functions.
 */

#ifndef MODSMP_CTRL_H
#define MODSMP_CTRL_H

#include "Sndfile.h"

namespace ctrlSmp
{

typedef uintptr_t SmpLength;

enum ResetFlag
{
	SmpResetCompo = 1,
	SmpResetInit,
	SmpResetVibrato,
};

// Insert silence to given location.
// Note: Is currently implemented only for inserting silence to the beginning and to the end of the sample.
// Return: Length of the new sample.
SmpLength InsertSilence(MODSAMPLE& smp, const SmpLength nSilenceLength, const SmpLength nStartFrom, CSoundFile* pSndFile);

// Change sample size. 
// Note: If resized sample is bigger, silence will be added to the sample's tail.
// Return: Length of the new sample.
SmpLength ResizeSample(MODSAMPLE& smp, const SmpLength nNewLength, CSoundFile* pSndFile);

// Replaces sample in 'smp' with given sample and frees the old sample.
// If valid CSoundFile pointer is given, the sample will be replaced also from the sounds channels.
void ReplaceSample(MODSAMPLE& smp, const LPSTR pNewSample,  const SmpLength nNewLength, CSoundFile* pSndFile);

bool AdjustEndOfSample(MODSAMPLE& smp, CSoundFile* pSndFile = 0);

// Returns the number of bytes allocated(at least) for sample data.
// Note: Currently the return value is based on the sample length and the actual 
//       allocation may be more than what this function returns.
inline SmpLength GetSampleCapacity(MODSAMPLE& smp) {return smp.GetSampleSizeInBytes();}

// Resets samples.
void ResetSamples(CSoundFile& rSndFile, ResetFlag resetflag);

// Remove DC offset and normalize.
// Return: If DC offset was removed, returns original offset value, zero otherwise.
float RemoveDCOffset(MODSAMPLE& smp,
					 SmpLength iStart,		// Start position (for partial DC offset removal).
					 SmpLength iEnd,		// End position (for partial DC offset removal).
					 const MODTYPE modtype,	// Used to determine whether to adjust global or default volume
											// to keep volume level the same given the normalization.
											// Volume adjustment is not done if this param is MOD_TYPE_NONE.
					 CSoundFile* const pSndFile); // Passed to AdjustEndOfSample.

} // Namespace ctrlSmp

namespace ctrlChn
{

// Replaces sample from sound channels by given sample.
void ReplaceSample( MODCHANNEL (&Chn)[MAX_CHANNELS],
					LPCSTR pOldSample,
					LPSTR pNewSample,
					const ctrlSmp::SmpLength nNewLength,
					DWORD orFlags = 0,
					DWORD andFlags = MAXDWORD);

} // namespace ctrlChn

#endif
