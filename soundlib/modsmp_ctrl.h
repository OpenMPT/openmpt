/*
 * MODINSTRUMENT related functions.
 */

#ifndef MODSMP_CTRL_H
#define MODSMP_CTRL_H

#include "Sndfile.h"

namespace ctrlSmp
{

typedef uintptr_t SmpLength;

// Insert silence to given location.
// Note: Is currently implemented only for inserting silence to the beginning and to the end of the sample.
// Return: Length of the new sample.
SmpLength InsertSilence(MODINSTRUMENT& smp, const SmpLength nSilenceLength, const SmpLength nStartFrom, CSoundFile* pSndFile = 0);

// Replaces sample in 'smp' with given sample and frees the old sample.
void ReplaceSample(MODINSTRUMENT& smp, const LPSTR pNewSample,  const SmpLength nNewLength);

bool AdjustEndOfSample(MODINSTRUMENT& smp, CSoundFile* pSndFile = 0);

// Returns the number of bytes allocated(at least) for sample data.
// Note: Currently the return value is based on the sample length and the actual 
//       allocation may be more than what this function returns.
inline SmpLength GetSampleCapacity(MODINSTRUMENT& smp) {return smp.GetSampleSizeInBytes();}

}

#endif
