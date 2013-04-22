/*
 * ModChannel.h
 * ------------
 * Purpose: Module Channel header class and helpers
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifdef _MSC_VER
#pragma warning(disable : 4324) //structure was padded due to __declspec(align())
#endif

class CSoundFile;

// Mix Channel Struct
struct ALIGN(32) ModChannel
{
	// Envelope playback info
	struct EnvInfo
	{
		FlagSet<EnvelopeFlags> flags;
		uint32 nEnvPosition;
		int32 nEnvValueAtReleaseJump;

		void Reset()
		{
			nEnvPosition = 0;
			nEnvValueAtReleaseJump = NOT_YET_RELEASED;
		}
	};

	// Information used in the mixer (should be kept tight for better caching)
	const void *pCurrentSample;	// Currently playing sample (nullptr if no sample is playing)
	uint32 nPos;			// Current play position
	uint32 nPosLo;			// 16-bit fractional part of play position
	int32 nInc;				// 16.16 fixed point sample speed relative to mixing frequency (0x10000 = one sample per output sample, 0x20000 = two samples per output sample, etc...)
	int32 leftVol;
	int32 rightVol;
	int32 leftRamp;
	int32 rightRamp;

	SmpLength nLength;
	SmpLength nLoopStart;
	SmpLength nLoopEnd;
	FlagSet<ChannelFlags> dwFlags;
	int32 rampLeftVol;
	int32 rampRightVol;
	float nFilter_Y1, nFilter_Y2;	// Mono / left channel filter memory
	float nFilter_Y3, nFilter_Y4;	// Right channel filter memory
	float nFilter_A0, nFilter_B0, nFilter_B1;	// Filter coeffs
	int32 nFilter_HP;
	int32 nROfs, nLOfs;
	int32 nRampLength;

	// Information not used in the mixer
	const void *pSample;				// Currently playing sample, or previously played sample if no sample is playing.
	ModSample *pModSample;				// Currently assigned sample slot
	ModInstrument *pModInstrument;		// Currently assigned instrument slot
	FlagSet<ChannelFlags> dwOldFlags;	// Flags from previous tick
	int32 newLeftVol, newRightVol;
	int32 nRealVolume, nRealPan;
	int32 nVolume, nPan, nFadeOutVol;
	int32 nPeriod, nC5Speed, nPortamentoDest;
	int32 nCalcVolume;								// Calculated channel volume, 14-Bit (without global volume, pre-amp etc applied) - for MIDI macros
	EnvInfo VolEnv, PanEnv, PitchEnv;				// Envelope playback info
	uint32 nVUMeter;
	int32 nGlobalVol;	// Channel volume (CV in ITTECH.TXT)
	int32 nInsVol;		// Sample / Instrument volume (SV * IV in ITTECH.TXT)
	int32 nFineTune, nTranspose;
	int32 nPortamentoSlide, nAutoVibDepth;
	uint32 nAutoVibPos, nVibratoPos, nTremoloPos, nPanbrelloPos;
	int32 nVolSwing, nPanSwing;
	int32 nCutSwing, nResSwing;
	int32 nRestorePanOnNewNote; //If > 0, nPan should be set to nRestorePanOnNewNote - 1 on new note. Used to recover from panswing.
	uint32 nOldGlobalVolSlide;
	uint32 nEFxOffset; // offset memory for Invert Loop (EFx, .MOD only)
	int32 nRetrigCount, nRetrigParam;
	uint32 nNoteSlideCounter, nNoteSlideSpeed, nNoteSlideStep;
	ROWINDEX nPatternLoop;
	CHANNELINDEX nMasterChn;
	// 8-bit members
	uint8 nRestoreResonanceOnNewNote; //Like above
	uint8 nRestoreCutoffOnNewNote; //Like above
	uint8 nNote, nNNA;
	uint8 nLastNote;				// Last note, ignoring note offs and cuts - for MIDI macros
	uint8 nNewNote, nNewIns, nOldIns, nCommand, nArpeggio;
	uint8 nOldVolumeSlide, nOldFineVolUpDown;
	uint8 nOldPortaUpDown, nOldFinePortaUpDown, nOldExtraFinePortaUpDown;
	uint8 nOldPanSlide, nOldChnVolSlide;
	uint8 nVibratoType, nVibratoSpeed, nVibratoDepth;
	uint8 nTremoloType, nTremoloSpeed, nTremoloDepth;
	uint8 nPanbrelloType, nPanbrelloSpeed, nPanbrelloDepth;
	uint8 nOldCmdEx, nOldVolParam, nOldTempo;
	uint8 nOldOffset, nOldHiOffset;
	uint8 nCutOff, nResonance;
	uint8 nTremorCount, nTremorParam;
	uint8 nPatternLoopCount;
	uint8 nLeftVU, nRightVU;
	uint8 nActiveMacro, nFilterMode;
	uint8 nEFxSpeed, nEFxDelay;		// memory for Invert Loop (EFx, .MOD only)

	ModCommand rowCommand;

	//NOTE_PCs memory.
	float m_plugParamValueStep, m_plugParamTargetValue;
	uint16 m_RowPlugParam;
	PLUGINDEX m_RowPlug;

	//-->Variables used to make user-definable tuning modes work with pattern effects.
	int32 m_PortamentoFineSteps, m_PortamentoTickSlide;

	uint32 m_Freq;
	float m_VibratoDepth;

	//If true, freq should be recalculated in ReadNote() on first tick.
	//Currently used only for vibrato things - using in other context might be 
	//problematic.
	bool m_ReCalculateFreqOnFirstTick;

	//To tell whether to calculate frequency.
	bool m_CalculateFreq;
	//<----

	void ClearRowCmd() { rowCommand = ModCommand::Empty(); }

	// Get a reference to a specific envelope of this channel
	const EnvInfo &GetEnvelope(enmEnvelopeTypes envType) const
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

	EnvInfo &GetEnvelope(enmEnvelopeTypes envType)
	{
		return const_cast<EnvInfo &>(static_cast<const ModChannel &>(*this).GetEnvelope(envType));
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

	typedef uint32 volume_t;
	volume_t GetVSTVolume() { return (pModInstrument) ? pModInstrument->nGlobalVol * 4 : nVolume; }

	// Check if the channel has a valid MIDI output. This function guarantees that pModInstrument != nullptr.
	bool HasMIDIOutput() const { return pModInstrument != nullptr && pModInstrument->HasValidMIDIChannel(); }

	ModChannel()
	{
		memset(this, 0, sizeof(*this));
	}

};


// Default pattern channel settings
struct ALIGN(32) ModChannelSettings
{
	FlagSet<ChannelFlags> dwFlags;	// Channel flags
	uint16 nPan;					// Initial pan (0...256)
	uint16 nVolume;					// Initial channel volume (0...64)
	PLUGINDEX nMixPlugin;			// Assigned plugin
	CHAR szName[MAX_CHANNELNAME];	// Channel name

	ModChannelSettings()
	{
		Reset();
	}

	void Reset()
	{
		nPan = 128;
		nVolume = 64;
		nMixPlugin = 0;
		szName[0] = '\0';
	}
};
