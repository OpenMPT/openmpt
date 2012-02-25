/*
 * ModChannel.h
 * ------------
 * Purpose: Module Channel header class and helpers
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

// Envelope playback info for each ModChannel
struct ModChannelEnvInfo
{
	DWORD nEnvPosition;
	DWORD flags;
	LONG nEnvValueAtReleaseJump;
};


#pragma warning(disable : 4324) //structure was padded due to __declspec(align())

// Mix Channel Struct
typedef struct __declspec(align(32)) ModChannel_
{
	// First 32-bytes: Most used mixing information: don't change it
	// These fields are accessed directly by the MMX mixing code (look out for CHNOFS_PCURRENTSAMPLE).
	// In the meantime, MMX mixing has been removed because it interfered with the new resonant filter code, and the byte offsets are also no longer hardcoded...
	LPSTR pCurrentSample;	// Currently playing sample (nullptr if no sample is playing)
	DWORD nPos;
	DWORD nPosLo;			// actually 16-bit (fractional part)
	LONG nInc;				// 16.16 fixed point
	LONG nRightVol;
	LONG nLeftVol;
	LONG nRightRamp;
	LONG nLeftRamp;
	// 2nd cache line
	DWORD nLength;
	DWORD dwFlags;
	DWORD nLoopStart;
	DWORD nLoopEnd;
	LONG nRampRightVol;
	LONG nRampLeftVol;
	float nFilter_Y1, nFilter_Y2, nFilter_Y3, nFilter_Y4;
	float nFilter_A0, nFilter_B0, nFilter_B1;
	int nFilter_HP;
	LONG nROfs, nLOfs;
	LONG nRampLength;
	// Information not used in the mixer
	LPSTR pSample;			// Currently playing sample, or previously played sample if no sample is playing.
	LONG nNewRightVol, nNewLeftVol;
	LONG nRealVolume, nRealPan;
	LONG nVolume, nPan, nFadeOutVol;
	LONG nPeriod, nC5Speed, nPortamentoDest;
	int nCalcVolume;								// Calculated channel volume, 14-Bit (without global volume, pre-amp etc applied) - for MIDI macros
	ModInstrument *pModInstrument;					// Currently assigned instrument slot
	ModChannelEnvInfo VolEnv, PanEnv, PitchEnv;	// Envelope playback info
	ModSample *pModSample;							// Currently assigned sample slot
	CHANNELINDEX nMasterChn;
	DWORD nVUMeter;
	LONG nGlobalVol;	// Channel volume (CV in ITTECH.TXT)
	LONG nInsVol;		// Sample / Instrument volume (SV * IV in ITTECH.TXT)
	LONG nFineTune, nTranspose;
	LONG nPortamentoSlide, nAutoVibDepth;
	UINT nAutoVibPos, nVibratoPos, nTremoloPos, nPanbrelloPos;
	LONG nVolSwing, nPanSwing;
	LONG nCutSwing, nResSwing;
	LONG nRestorePanOnNewNote; //If > 0, nPan should be set to nRestorePanOnNewNote - 1 on new note. Used to recover from panswing.
	UINT nOldGlobalVolSlide;
	DWORD nEFxOffset; // offset memory for Invert Loop (EFx, .MOD only)
	int nRetrigCount, nRetrigParam;
	ROWINDEX nPatternLoop;
	UINT nNoteSlideCounter, nNoteSlideSpeed, nNoteSlideStep;
	// 8-bit members
	BYTE nRestoreResonanceOnNewNote; //Like above
	BYTE nRestoreCutoffOnNewNote; //Like above
	BYTE nNote, nNNA;
	BYTE nLastNote;				// Last note, ignoring note offs and cuts - for MIDI macros
	BYTE nNewNote, nNewIns, nCommand, nArpeggio;
	BYTE nOldVolumeSlide, nOldFineVolUpDown;
	BYTE nOldPortaUpDown, nOldFinePortaUpDown;
	BYTE nOldPanSlide, nOldChnVolSlide;
	BYTE nVibratoType, nVibratoSpeed, nVibratoDepth;
	BYTE nTremoloType, nTremoloSpeed, nTremoloDepth;
	BYTE nPanbrelloType, nPanbrelloSpeed, nPanbrelloDepth;
	BYTE nOldCmdEx, nOldVolParam, nOldTempo;
	BYTE nOldOffset, nOldHiOffset;
	BYTE nCutOff, nResonance;
	BYTE nTremorCount, nTremorParam;
	BYTE nPatternLoopCount;
	BYTE nLeftVU, nRightVU;
	BYTE nActiveMacro, nFilterMode;
	BYTE nEFxSpeed, nEFxDelay;		// memory for Invert Loop (EFx, .MOD only)

	ModCommand rowCommand;

	//NOTE_PCs memory.
	uint16 m_RowPlugParam;
	float m_plugParamValueStep, m_plugParamTargetValue;
	PLUGINDEX m_RowPlug;

	void ClearRowCmd() { rowCommand = ModCommand::Empty(); }

	ModChannelEnvInfo &GetEnvelope(enmEnvelopeTypes envType)
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

	typedef UINT VOLUME;
	VOLUME GetVSTVolume() {return (pModInstrument) ? pModInstrument->nGlobalVol * 4 : nVolume;}

	//-->Variables used to make user-definable tuning modes work with pattern effects.
	bool m_ReCalculateFreqOnFirstTick;
	//If true, freq should be recalculated in ReadNote() on first tick.
	//Currently used only for vibrato things - using in other context might be 
	//problematic.

	bool m_CalculateFreq;
	//To tell whether to calculate frequency.

	int32 m_PortamentoFineSteps;
	long m_PortamentoTickSlide;

	UINT m_Freq;
	float m_VibratoDepth;
	//<----
} ModChannel;

#define CHNRESET_CHNSETTINGS	1	//       1 b 
#define CHNRESET_SETPOS_BASIC	2	//      10 b
#define	CHNRESET_SETPOS_FULL	7	//     111 b
#define CHNRESET_TOTAL			255 // 11111111b


// Default pattern channel settings
struct ModChannelSettings
{
	UINT nPan;						// Initial pan
	UINT nVolume;					// Initial channel volume
	DWORD dwFlags;					// Channel flags
	PLUGINDEX nMixPlugin;			// Assigned plugin
	CHAR szName[MAX_CHANNELNAME];	// Channel name
};
