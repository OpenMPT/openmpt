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
	SmpResetCompo = 1
};

// Insert silence to given location.
// Note: Is currently implemented only for inserting silence to the beginning and to the end of the sample.
// Return: Length of the new sample.
SmpLength InsertSilence(MODINSTRUMENT& smp, const SmpLength nSilenceLength, const SmpLength nStartFrom, CSoundFile* pSndFile = nullptr);

// Change sample size. 
// Note: If resized sample is bigger, silence will be added to the sample's tail.
// Return: Length of the new sample.
SmpLength ResizeSample(MODINSTRUMENT& smp, const SmpLength nNewLength, CSoundFile* pSndFile = nullptr);

// Replaces sample in 'smp' with given sample and frees the old sample.
void ReplaceSample(MODINSTRUMENT& smp, const LPSTR pNewSample,  const SmpLength nNewLength);

bool AdjustEndOfSample(MODINSTRUMENT& smp, CSoundFile* pSndFile = 0);

// Returns the number of bytes allocated(at least) for sample data.
// Note: Currently the return value is based on the sample length and the actual 
//       allocation may be more than what this function returns.
inline SmpLength GetSampleCapacity(MODINSTRUMENT& smp) {return smp.GetSampleSizeInBytes();}

// Resets samples.
void ResetSamples(CSoundFile& rSndFile, ResetFlag resetflag);

// Remove DC offset and normalize.
// Return: If DC offset was removed, returns original offset value, zero otherwise.
float RemoveDCOffset(MODINSTRUMENT& smp,
					 SmpLength iStart,		// Start position (for partial DC offset removal).
					 SmpLength iEnd,		// End position (for partial DC offset removal).
					 const MODTYPE modtype,	// Used to determine whether to adjust global or default volume
											// to keep volume level the same given the normalization.
											// Volume adjustment is not done if this param is MOD_TYPE_NONE.
					 CSoundFile* const pSndFile); // Passed to AdjustEndOfSample.

} // Namespace ctrlSmp

#endif
