/*
 * ModConvert.h
 * ------------
 * Purpose: Converting between various module formats.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

// Warning types
enum ConversionWarning
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
	wAdlibInstruments,
	wRestartPos,
	wChannelVolSurround,
	wChannelPanning,
	wPatternSignatures,
	wLinearSlides,
	wTrimmedEnvelopes,
	wReleaseNode,
	wEditHistory,
	wMixmode,
	wVolRamp,
	wPitchToTempoLock,
	wGlobalVolumeNotSupported,
	wFilterVariation,
	wResamplingMode,
	wFractionalTempo,
	wNumWarnings
};

OPENMPT_NAMESPACE_END
