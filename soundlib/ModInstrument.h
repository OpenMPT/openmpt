/*
 * ModInstrument.h
 * ---------------
 * Purpose: Module Instrument header class and helpers
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/misc_util.h"

// Instrument Envelopes
struct InstrumentEnvelope
{
	FlagSet<EnvelopeFlags> dwFlags;	// envelope flags
	uint32 nNodes;					// amount of nodes used
	uint8  nLoopStart;				// loop start node
	uint8  nLoopEnd;				// loop end node
	uint8  nSustainStart;			// sustain start node
	uint8  nSustainEnd;				// sustain end node
	uint8  nReleaseNode;			// release node
	uint16 Ticks[MAX_ENVPOINTS];	// envelope point position (x axis)
	uint8  Values[MAX_ENVPOINTS];	// envelope point value (y axis)

	InstrumentEnvelope()
	{
		nNodes = 0;
		nLoopStart = nLoopEnd = 0;
		nSustainStart = nSustainEnd = 0;
		nReleaseNode = ENV_RELEASE_NODE_UNSET;
		MemsetZero(Ticks);
		MemsetZero(Values);
	}

	// Convert envelope data between various formats.
	void Convert(MODTYPE fromType, MODTYPE toType);

	// Get envelope value at a given tick. Returns value in range [0.0, 1.0].
	float GetValueFromPosition(int position) const;

};

// Instrument Struct
struct ModInstrument
{
	FlagSet<InstrumentFlags> dwFlags;	// Instrument flags
	uint32 nFadeOut;					// Instrument fadeout speed
	uint32 nGlobalVol;					// Global volume (0...64, all sample volumes are multiplied with this - TODO: This is 0...128 in Impulse Tracker)
	uint32 nPan;						// Default pan (0...256), if the appropriate flag is set. Sample panning overrides instrument panning.

	uint16 nVolRampUp;					// Default sample ramping up

	uint16 wMidiBank;					// MIDI Bank (1...16384). 0 = Don't send.
	uint8 nMidiProgram;					// MIDI Program (1...128). 0 = Don't send.
	uint8 nMidiChannel;					// MIDI Channel (1...16). 0 = Don't send. 17 = Mapped (Send to tracker channel modulo 16).
	uint8 nMidiDrumKey;					// Drum set note mapping (currently only used by the .MID loader)
	int8 midiPWD;						// MIDI Pitch Wheel Depth in semitones

	uint8 nNNA;							// New note action
	uint8 nDCT;							// Duplicate check type	(i.e. which condition will trigger the duplicate note action)
	uint8 nDNA;							// Duplicate note action
	uint8 nPanSwing;					// Random panning factor (0...64)
	uint8 nVolSwing;					// Random volume factor (0...100)
	uint8 nIFC;							// Default filter cutoff (0...127). Used if the high bit is set
	uint8 nIFR;							// Default filter resonance (0...127). Used if the high bit is set

	int8 nPPS;							// Pitch/Pan separation (i.e. how wide the panning spreads, -32...32)
	uint8 nPPC;							// Pitch/Pan centre

	PLUGINDEX nMixPlug;					// Plugin assigned to this instrument
	uint8 nCutSwing;					// Random cutoff factor (0...64)
	uint8 nResSwing;					// Random resonance factor (0...64)
	uint8 nFilterMode;					// Default filter mode
	uint8 nPluginVelocityHandling;		// How to deal with plugin velocity
	uint8 nPluginVolumeHandling;		// How to deal with plugin volume
	uint16 wPitchToTempoLock;			// BPM at which the samples assigned to this instrument loop correctly
	uint32 nResampling;					// Resampling mode
	CTuning *pTuning;					// sample tuning assigned to this instrument
	static CTuning *s_DefaultTuning;

	InstrumentEnvelope VolEnv;			// Volume envelope data
	InstrumentEnvelope PanEnv;			// Panning envelope data
	InstrumentEnvelope PitchEnv;		// Pitch / filter envelope data

	uint8 NoteMap[128];					// Note mapping, e.g. C-5 => D-5.
	SAMPLEINDEX Keyboard[128];			// Sample mapping, e.g. C-5 => Sample 1

	char name[MAX_INSTRUMENTNAME];		// Note: not guaranteed to be null-terminated.
	char filename[MAX_INSTRUMENTFILENAME];

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// WHEN adding new members here, ALSO update Sndfile.cpp (instructions near the top of this file)!
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	void SetTuning(CTuning* pT)
	{
		pTuning = pT;
	}

	ModInstrument(SAMPLEINDEX sample = 0);

	// Assign all notes to a given sample.
	void AssignSample(SAMPLEINDEX sample)
	{
		for(size_t n = 0; n < CountOf(Keyboard); n++)
		{
			Keyboard[n] = sample;
		}
	}

	// Reset note mapping (i.e. every note is mapped to itself)
	void ResetNoteMap()
	{
		for(size_t n = 0; n < CountOf(NoteMap); n++)
		{
			NoteMap[n] = static_cast<uint8>(n + 1);
		}
	}

	bool IsCutoffEnabled() const { return (nIFC & 0x80) != 0; }
	bool IsResonanceEnabled() const { return (nIFR & 0x80) != 0; }
	uint8 GetCutoff() const { return (nIFC & 0x7F); }
	uint8 GetResonance() const { return (nIFR & 0x7F); }
	void SetCutoff(uint8 cutoff, bool enable) { nIFC = min(cutoff, 0x7F) | (enable ? 0x80 : 0x00); }
	void SetResonance(uint8 resonance, bool enable) { nIFR = min(resonance, 0x7F) | (enable ? 0x80 : 0x00); }

	bool HasValidMIDIChannel() const { return (nMidiChannel >= 1 && nMidiChannel <= 17); }

	// Get a reference to a specific envelope of this instrument
	const InstrumentEnvelope &GetEnvelope(enmEnvelopeTypes envType) const
	{
		switch(envType)
		{
		case ENV_VOLUME:
		default:
			return VolEnv;
		case ENV_PANNING:
			return PanEnv;
		case ENV_PITCH:
			return PitchEnv;
		}
	}

	InstrumentEnvelope &GetEnvelope(enmEnvelopeTypes envType)
	{
		return const_cast<InstrumentEnvelope &>(static_cast<const ModInstrument &>(*this).GetEnvelope(envType));
	}

	// Get a set of all samples referenced by this instrument
	std::set<SAMPLEINDEX> GetSamples() const
	{
		std::set<SAMPLEINDEX> referencedSamples;

		for(size_t i = 0; i < CountOf(Keyboard); i++)
		{
			// 0 isn't a sample.
			if(Keyboard[i] != 0)
			{
				referencedSamples.insert(Keyboard[i]);
			}
		}

		return referencedSamples;
	}

	// Write sample references into a bool vector. If a sample is referenced by this instrument, true is written.
	// The caller has to initialize the vector.
	void GetSamples(std::vector<bool> &referencedSamples) const
	{
		for(size_t i = 0; i < CountOf(Keyboard); i++)
		{
			// 0 isn't a sample.
			if(Keyboard[i] != 0 && Keyboard[i] < referencedSamples.size())
			{
				referencedSamples[Keyboard[i]] = true;
			}
		}
	}

	// Translate instrument properties between two given formats.
	void Convert(MODTYPE fromType, MODTYPE toType);

};