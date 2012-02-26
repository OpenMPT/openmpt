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


// Convert envelope data between various formats.
void InstrumentEnvelope::Convert(MODTYPE fromType, MODTYPE toType)
//----------------------------------------------------------------
{
	if(!(fromType & MOD_TYPE_XM) && (toType & MOD_TYPE_XM))
	{
		// IT / MPTM -> XM: Expand loop by one tick, convert sustain loops to sustain points, remove carry flag.
		nSustainStart = nSustainEnd;
		dwFlags &= ~ENV_CARRY;

		if(nLoopEnd > nLoopStart && (dwFlags & ENV_LOOP))
		{
			for(UINT node = nLoopEnd; node < nNodes; node++)
			{
				Ticks[node]++;
			}
		}
	} else if((fromType & MOD_TYPE_XM) && !(toType & MOD_TYPE_XM))
	{
		// XM -> IT / MPTM: Shorten loop by one tick by inserting bogus point
		if(nLoopEnd > nLoopStart && (dwFlags & ENV_LOOP))
		{
			if(Ticks[nLoopEnd] - 1 > Ticks[nLoopEnd - 1])
			{
				// Insert an interpolated point just before the loop point.
				BYTE interpolatedValue = Util::Round<BYTE>(GetValueFromPosition(Ticks[nLoopEnd] - 1) * 64.0f);
				

				if(nNodes +  1 < MAX_ENVPOINTS)
				{
					// Should always be possible, since XM only allows for 12 envelope points anyway.
					for(UINT node = nNodes; node >= nLoopEnd; node--)
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
};


// Get envelope value at a given tick. Returns value in range [0.0, 1.0].
float InstrumentEnvelope::GetValueFromPosition(int position) const
//----------------------------------------------------------------
{
	UINT pt = nNodes - 1;

	// Checking where current 'tick' is relative to the envelope points.
	for(UINT i = 0; i < nNodes - 1u; i++)
	{
		if (position <= Ticks[i])
		{
			pt = i;
			break;
		}
	}

	int x2 = Ticks[pt];
	float value = 0.0f;

	if(position >= x2)
	{
		// Case: current 'tick' is on a envelope point.
		value = static_cast<float>(Values[pt]) / 64.0f;
	} else
	{
		// Case: current 'tick' is between two envelope points.
		int x1 = 0;

		if (pt)
		{
			// Get previous node's value and tick.
			value = static_cast<float>(Values[pt - 1]) / 64.0f;
			x1 = Ticks[pt - 1];
		}

		if(x2 > x1 && position > x1)
		{
			// Linear approximation between the points;
			// f(x + d) ~ f(x) + f'(x) * d, where f'(x) = (y2 - y1) / (x2 - x1)
			value += ((position - x1) * (static_cast<float>(Values[pt]) / 64.0f - value)) / (x2 - x1);
		}
	}

	return Clamp(value, 0.0f, 1.0f);
};


ModInstrument::ModInstrument(SAMPLEINDEX sample)
//----------------------------------------------
{
	nFadeOut = 256;
	dwFlags = 0;
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

	nPPC = NOTE_MIDDLEC - 1;
	nPPS = 0;

	nMixPlug = 0;
	nVolRampUp = 0;
	nResampling = SRCMODE_DEFAULT;
	nCutSwing = 0;
	nResSwing = 0;
	nFilterMode = FLTMODE_UNCHANGED;
	wPitchToTempoLock = 0;
	nPluginVelocityHandling = PLUGIN_VELOCITYHANDLING_CHANNEL;
	nPluginVolumeHandling = PLUGIN_VOLUMEHANDLING_IGNORE;

	pTuning = s_DefaultTuning;

	AssignSample(sample);
	ResetNoteMap();

	MemsetZero(name);
	MemsetZero(filename);
}


// Translate instrument properties between two given formats.
void ModInstrument::Convert(MODTYPE fromType, MODTYPE toType)
//-----------------------------------------------------------
{
	UNREFERENCED_PARAMETER(fromType);

	if(toType & MOD_TYPE_XM)
	{
		ResetNoteMap();

		PitchEnv.dwFlags &= ~(ENV_ENABLED|ENV_FILTER);

		dwFlags &= ~INS_SETPANNING;
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

	// MPT-specific features - remove instrument tunings, Pitch/Tempo Lock for other formats
	if(!(toType & MOD_TYPE_MPT))
	{
		SetTuning(nullptr);
		wPitchToTempoLock = 0;
	}
}
