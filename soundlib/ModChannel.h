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

	void Reset()
	{
		nEnvPosition = 0;
		nEnvValueAtReleaseJump = NOT_YET_RELEASED;
	}
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
	int32 nInc;				// 16.16 fixed point
	int nRightVol;
	int nLeftVol;
	int nRightRamp;
	int nLeftRamp;
	// 2nd cache line
	DWORD nLength;
	DWORD dwFlags;
	DWORD nLoopStart;
	DWORD nLoopEnd;
	int nRampRightVol;
	int nRampLeftVol;
	float nFilter_Y1, nFilter_Y2, nFilter_Y3, nFilter_Y4;
	float nFilter_A0, nFilter_B0, nFilter_B1;
	int nFilter_HP;
	int nROfs, nLOfs;
	int nRampLength;
	// Information not used in the mixer
	LPSTR pSample;			// Currently playing sample, or previously played sample if no sample is playing.
	int nNewRightVol, nNewLeftVol;
	int nRealVolume, nRealPan;
	int nVolume, nPan, nFadeOutVol;
	int nPeriod, nC5Speed, nPortamentoDest;
	int nCalcVolume;								// Calculated channel volume, 14-Bit (without global volume, pre-amp etc applied) - for MIDI macros
	ModInstrument *pModInstrument;					// Currently assigned instrument slot
	ModChannelEnvInfo VolEnv, PanEnv, PitchEnv;	// Envelope playback info
	ModSample *pModSample;							// Currently assigned sample slot
	CHANNELINDEX nMasterChn;
	DWORD nVUMeter;
	int nGlobalVol;	// Channel volume (CV in ITTECH.TXT)
	int nInsVol;		// Sample / Instrument volume (SV * IV in ITTECH.TXT)
	int nFineTune, nTranspose;
	int nPortamentoSlide, nAutoVibDepth;
	UINT nAutoVibPos, nVibratoPos, nTremoloPos, nPanbrelloPos;
	int nVolSwing, nPanSwing;
	int nCutSwing, nResSwing;
	int nRestorePanOnNewNote; //If > 0, nPan should be set to nRestorePanOnNewNote - 1 on new note. Used to recover from panswing.
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

	// Get a reference to a specific envelope of this channel
	const ModChannelEnvInfo &GetEnvelope(enmEnvelopeTypes envType) const
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

	ModChannelEnvInfo &GetEnvelope(enmEnvelopeTypes envType)
	{
		return const_cast<ModChannelEnvInfo &>(static_cast<const ModChannel &>(*this).GetEnvelope(envType));
	}

	void ResetEnvelopes()
	{
		VolEnv.Reset();
		PanEnv.Reset();
		PitchEnv.Reset();
	}

	enum ResetFlags
	{
		resetChannelSettings =	1,		// Reload initial channel settings
		resetSetPosBasic =		2,		// Reset basic runtime channel attributes
		resetSetPosAdvanced =	4,		// Reset more runtime channel attributes
		resetSetPosFull = 		resetSetPosBasic | resetSetPosAdvanced | resetChannelSettings,		// Reset all runtime channel attributes
		resetTotal =			resetSetPosFull,

	};

	void Reset(ResetFlags resetMask, const CSoundFile &sndFile, CHANNELINDEX sourceChannel);

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


// Default pattern channel settings
struct ModChannelSettings
{
	UINT nPan;						// Initial pan
	UINT nVolume;					// Initial channel volume
	DWORD dwFlags;					// Channel flags
	PLUGINDEX nMixPlugin;			// Assigned plugin
	CHAR szName[MAX_CHANNELNAME];	// Channel name
};
