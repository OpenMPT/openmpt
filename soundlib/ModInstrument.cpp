/*
 * ModInstrument.cpp
 * -----------------
 * Purpose: Helper functions for Module Instrument handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include "ModInstrument.h"


OPENMPT_NAMESPACE_BEGIN


// Convert envelope data between various formats.
void InstrumentEnvelope::Convert(MODTYPE fromType, MODTYPE toType)
//----------------------------------------------------------------
{
	if(!(fromType & MOD_TYPE_XM) && (toType & MOD_TYPE_XM))
	{
		// IT / MPTM -> XM: Expand loop by one tick, convert sustain loops to sustain points, remove carry flag.
		nSustainStart = nSustainEnd;
		dwFlags.reset(ENV_CARRY);

		if(nLoopEnd > nLoopStart && dwFlags[ENV_LOOP])
		{
			for(uint32 node = nLoopEnd; node < nNodes; node++)
			{
				Ticks[node]++;
			}
		}
	} else if((fromType & MOD_TYPE_XM) && !(toType & MOD_TYPE_XM))
	{
		if(nSustainStart > nLoopEnd && dwFlags[ENV_LOOP])
		{
			// In the IT format, the sustain loop is always considered before the envelope loop.
			// In the XM format, whichever of the two is encountered first is considered.
			// So we have to disable the sustain loop if it was behind the normal loop.
			dwFlags.reset(ENV_SUSTAIN);
		}

		// XM -> IT / MPTM: Shorten loop by one tick by inserting bogus point
		if(nLoopEnd > nLoopStart && dwFlags[ENV_LOOP])
		{
			if(Ticks[nLoopEnd] - 1 > Ticks[nLoopEnd - 1])
			{
				// Insert an interpolated point just before the loop point.
				uint8 interpolatedValue = static_cast<uint8>(GetValueFromPosition(Ticks[nLoopEnd] - 1, 64));
				

				if(nNodes +  1 < MAX_ENVPOINTS)
				{
					// Should always be possible, since XM only allows for 12 envelope points anyway.
					for(uint32 node = nNodes; node >= nLoopEnd; node--)
					{
						Ticks[node + 1] = Ticks[node];
						Values[node + 1] = Values[node];
					}

					nNodes++;
				}

				Ticks[nLoopEnd]--;
				Values[nLoopEnd] = interpolatedValue;
			} else
			{
				// There is already a point before the loop point: Use it as new loop end.
				nLoopEnd--;
			}
		}
	}
}


// Get envelope value at a given tick. Assumes that the envelope data is in rage [0, rangeIn],
// returns value in range [0, rangeOut].
int32 InstrumentEnvelope::GetValueFromPosition(int position, int32 rangeOut, int32 rangeIn) const
//-----------------------------------------------------------------------------------------------
{
	uint32 pt = nNodes - 1u;
	const int32 ENV_PRECISION = 1 << 16;

	// Checking where current 'tick' is relative to the envelope points.
	for(uint32 i = 0; i < nNodes - 1u; i++)
	{
		if (position <= Ticks[i])
		{
			pt = i;
			break;
		}
	}

	int x2 = Ticks[pt];
	int32 value = 0;

	if(position >= x2)
	{
		// Case: current 'tick' is on a envelope point.
		value = Values[pt] * ENV_PRECISION / rangeIn;
	} else
	{
		// Case: current 'tick' is between two envelope points.
		int x1 = 0;

		if(pt)
		{
			// Get previous node's value and tick.
			value = Values[pt - 1] * ENV_PRECISION / rangeIn;
			x1 = Ticks[pt - 1];
		}

		if(x2 > x1 && position > x1)
		{
			// Linear approximation between the points;
			// f(x + d) ~ f(x) + f'(x) * d, where f'(x) = (y2 - y1) / (x2 - x1)
			value += ((position - x1) * (Values[pt] * ENV_PRECISION / rangeIn - value)) / (x2 - x1);
		}
	}

	Limit(value, 0, ENV_PRECISION);
	return (value * rangeOut + ENV_PRECISION / 2) / ENV_PRECISION;
}


void InstrumentEnvelope::Sanitize(uint8 maxValue)
//-----------------------------------------------
{
	LimitMax(nNodes, uint32(MAX_ENVPOINTS));
	Ticks[0] = 0;
	LimitMax(Values[0], maxValue);
	for(uint32 i = 1; i < nNodes; i++)
	{
		if(Ticks[i] < Ticks[i - 1])
			Ticks[i] = Ticks[i - 1];
		LimitMax(Values[i], maxValue);
	}
	STATIC_ASSERT(MAX_ENVPOINTS <= 255);
	LimitMax(nLoopEnd, static_cast<uint8>(nNodes));
	LimitMax(nLoopStart, nLoopEnd);
	LimitMax(nSustainEnd, static_cast<uint8>(nNodes));
	LimitMax(nSustainStart, nSustainEnd);
	if(nReleaseNode != ENV_RELEASE_NODE_UNSET)
		LimitMax(nReleaseNode, static_cast<uint8>(nNodes));
}


ModInstrument::ModInstrument(SAMPLEINDEX sample)
//----------------------------------------------
{
	nFadeOut = 256;
	dwFlags.reset();
	nGlobalVol = 64;
	nPan = 32 * 4;

	nNNA = NNA_NOTECUT;
	nDCT = DCT_NONE;
	nDNA = DNA_NOTECUT;

	nPanSwing = 0;
	nVolSwing = 0;
	SetCutoff(0, false);
	SetResonance(0, false);

	wMidiBank = 0;
	nMidiProgram = 0;
	nMidiChannel = 0;
	nMidiDrumKey = 0;
	midiPWD = 2;

	nPPC = NOTE_MIDDLEC - 1;
	nPPS = 0;

	nMixPlug = 0;
	nVolRampUp = 0;
	nResampling = SRCMODE_DEFAULT;
	nCutSwing = 0;
	nResSwing = 0;
	nFilterMode = FLTMODE_UNCHANGED;
	pitchToTempoLock.Set(0);
	nPluginVelocityHandling = PLUGIN_VELOCITYHANDLING_CHANNEL;
	nPluginVolumeHandling = PLUGIN_VOLUMEHANDLING_IGNORE;

	pTuning = CSoundFile::GetDefaultTuning();

	AssignSample(sample);
	ResetNoteMap();

	MemsetZero(name);
	MemsetZero(filename);
}


// Translate instrument properties between two given formats.
void ModInstrument::Convert(MODTYPE fromType, MODTYPE toType)
//-----------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(fromType);

	if(toType & MOD_TYPE_XM)
	{
		ResetNoteMap();

		PitchEnv.dwFlags.reset(ENV_ENABLED | ENV_FILTER);

		dwFlags.reset(INS_SETPANNING);
		SetCutoff(GetCutoff(), false);
		SetResonance(GetResonance(), false);
		nFilterMode = FLTMODE_UNCHANGED;

		nCutSwing = nPanSwing = nResSwing = nVolSwing = 0;

		nPPC = NOTE_MIDDLEC - 1;
		nPPS = 0;

		nNNA = NNA_NOTECUT;
		nDCT = DCT_NONE;
		nDNA = DNA_NOTECUT;

		if(nMidiChannel == MidiMappedChannel)
		{
			nMidiChannel = 1;
		}

		// FT2 only has unsigned Pitch Wheel Depth, and it's limited to 0...36 (in the GUI, at least. As you would expect it from FT2, this value is actually not sanitized on load).
		midiPWD = static_cast<int8>(mpt::abs(midiPWD));
		Limit(midiPWD, int8(0), int8(36));

		nGlobalVol = 64;
		nPan = 128;

		LimitMax(nFadeOut, 32767u);
	}

	VolEnv.Convert(fromType, toType);
	PanEnv.Convert(fromType, toType);
	PitchEnv.Convert(fromType, toType);

	// Limit fadeout length for IT / MPT
	if(toType & (MOD_TYPE_IT | MOD_TYPE_MPT))
	{
		LimitMax(nFadeOut, 8192u);
	}

	// MPT-specific features - remove instrument tunings, Pitch/Tempo Lock, cutoff / resonance swing and filter mode for other formats
	if(!(toType & MOD_TYPE_MPT))
	{
		SetTuning(nullptr);
		pitchToTempoLock.Set(0);
		nCutSwing = nResSwing = 0;
		nFilterMode = FLTMODE_UNCHANGED;
		nVolRampUp = 0;
	}
}


// Get a set of all samples referenced by this instrument
std::set<SAMPLEINDEX> ModInstrument::GetSamples() const
//-----------------------------------------------------
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
void ModInstrument::GetSamples(std::vector<bool> &referencedSamples) const
//------------------------------------------------------------------------
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


void ModInstrument::Sanitize(MODTYPE modType)
//-------------------------------------------
{
	LimitMax(nFadeOut, 65536u);
	LimitMax(nGlobalVol, 64u);
	LimitMax(nPan, 256u);

	LimitMax(wMidiBank, uint16(16384));
	LimitMax(nMidiProgram, uint8(128));
	LimitMax(nMidiChannel, uint8(17));

	if(nNNA > NNA_NOTEFADE) nNNA = NNA_NOTECUT;
	if(nDCT > DCT_PLUGIN) nDCT = DCT_NONE;
	if(nDNA > DNA_NOTEFADE) nDNA = DNA_NOTECUT;

	LimitMax(nPanSwing, uint8(64));
	LimitMax(nVolSwing, uint8(100));

	Limit(nPPS, int8(-32), int8(32));

	LimitMax(nCutSwing, uint8(64));
	LimitMax(nResSwing, uint8(64));
	
#ifdef MODPLUG_TRACKER
	MPT_UNREFERENCED_PARAMETER(modType);
	const uint8 range = ENVELOPE_MAX;
#else
	const uint8 range = modType == MOD_TYPE_AMS2 ? uint8_max : ENVELOPE_MAX;
#endif
	VolEnv.Sanitize();
	PanEnv.Sanitize();
	PitchEnv.Sanitize(range);
}


OPENMPT_NAMESPACE_END
