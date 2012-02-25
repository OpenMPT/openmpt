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
