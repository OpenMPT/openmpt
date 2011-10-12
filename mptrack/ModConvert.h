/*
 * ModConvert.h
 * ------------
 * Purpose: Headers for module conversion code.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 *
 */

#ifndef MODCONVERT_H
#define MODCONVERT_H
#pragma once

// Warning types
enum enmWarnings
{
	wInstrumentsToSamples = 0,
	wResizedPatterns,
	wSampleBidiLoops,
	wSampleSustainLoops,
	wSampleAutoVibrato,
	wMODSampleFrequency,
	wBrokenNoteMap,
	wInstrumentSustainLoops,
	wInstrumentTuning,
	wMODGlobalVars,
	wMOD31Samples,
	wRestartPos,
	wChannelVolSurround,
	wChannelPanning,
	wPatternSignatures,
	wLinearSlides,
	wTrimmedEnvelopes,
	wReleaseNode,
	wEditHistory,
	wMixmode,
	wCompatibilityMode,
	wPitchToTempoLock,
	wNumWarnings
};

#endif // MODCONVERT_H
