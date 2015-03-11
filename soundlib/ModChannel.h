/*
 * ModChannel.h
 * ------------
 * Purpose: Module Channel header class and helpers
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;

// Mix Channel Struct
struct ModChannel
{
	// Envelope playback info
	struct EnvInfo
	{
		FlagSet<EnvelopeFlags> flags;
		uint32_t nEnvPosition;
		int32_t nEnvValueAtReleaseJump;

		void Reset()
		{
			nEnvPosition = 0;
			nEnvValueAtReleaseJump = NOT_YET_RELEASED;
		}
	};

	// Information used in the mixer (should be kept tight for better caching)
	// Byte sizes are for 32-bit builds and 32-bit integer / float mixer
	const void *pCurrentSample;	// Currently playing sample (nullptr if no sample is playing)
	uint32_t nPos;			// Current play position
	uint32_t nPosLo;			// 16-bit fractional part of play position
	int32_t nInc;				// 16.16 fixed point sample speed relative to mixing frequency (0x10000 = one sample per output sample, 0x20000 = two samples per output sample, etc...)
	int32_t leftVol;			// 0...4096 (12 bits, since 16 bits + 12 bits = 28 bits = 0dB in integer mixer, see MIXING_ATTENUATION)
	int32_t rightVol;			// Ditto
	int32_t leftRamp;			// Ramping delta, 20.12 fixed point (see VOLUMERAMPPRECISION)
	int32_t rightRamp;		// Ditto
	// Up to here: 32 bytes
	int32_t rampLeftVol;		// Current ramping volume, 20.12 fixed point (see VOLUMERAMPPRECISION)
	int32_t rampRightVol;		// Ditto
	mixsample_t nFilter_Y[2][2];					// Filter memory - two history items per sample channel
	mixsample_t nFilter_A0, nFilter_B0, nFilter_B1;	// Filter coeffs
	mixsample_t nFilter_HP;
	// Up to here: 72 bytes

	SmpLength nLength;
	SmpLength nLoopStart;
	SmpLength nLoopEnd;
	FlagSet<ChannelFlags> dwFlags;
	mixsample_t nROfs, nLOfs;
	uint32_t nRampLength;
	// Up to here: 100 bytes

	const ModSample *pModSample;			// Currently assigned sample slot (can already be stopped)

	// Information not used in the mixer
	const ModInstrument *pModInstrument;	// Currently assigned instrument slot
	SmpLength proTrackerOffset;				// Offset for instrument-less notes in ProTracker mode
	SmpLength oldOffset;
	FlagSet<ChannelFlags> dwOldFlags;		// Flags from previous tick
	int32_t newLeftVol, newRightVol;
	int32_t nRealVolume, nRealPan;
	int32_t nVolume, nPan, nFadeOutVol;
	int32_t nPeriod, nC5Speed, nPortamentoDest;
	int32_t cachedPeriod, glissandoPeriod;
	int32_t nCalcVolume;								// Calculated channel volume, 14-Bit (without global volume, pre-amp etc applied) - for MIDI macros
	EnvInfo VolEnv, PanEnv, PitchEnv;				// Envelope playback info
	int32_t nGlobalVol;	// Channel volume (CV in ITTECH.TXT)
	int32_t nInsVol;		// Sample / Instrument volume (SV * IV in ITTECH.TXT)
	int32_t nFineTune, nTranspose;
	int32_t nPortamentoSlide, nAutoVibDepth;
	uint32_t nAutoVibPos, nVibratoPos, nTremoloPos, nPanbrelloPos;
	int32_t nVolSwing, nPanSwing;
	int32_t nCutSwing, nResSwing;
	int32_t nRestorePanOnNewNote; //If > 0, nPan should be set to nRestorePanOnNewNote - 1 on new note. Used to recover from panswing.
	uint32_t nEFxOffset; // offset memory for Invert Loop (EFx, .MOD only)
	int32_t nRetrigCount, nRetrigParam;
	ROWINDEX nPatternLoop;
	CHANNELINDEX nMasterChn;
	ModCommand rowCommand;
	// 8-bit members
	uint8_t resamplingMode;
	uint8_t nRestoreResonanceOnNewNote;	// See nRestorePanOnNewNote
	uint8_t nRestoreCutoffOnNewNote;		// ditto
	uint8_t nNote, nNNA;
	uint8_t nLastNote;				// Last note, ignoring note offs and cuts - for MIDI macros
	uint8_t nArpeggioLastNote, nArpeggioBaseNote;	// For plugin arpeggio
	uint8_t nNewNote, nNewIns, nOldIns, nCommand, nArpeggio;
	uint8_t nOldVolumeSlide, nOldFineVolUpDown;
	uint8_t nOldPortaUpDown, nOldFinePortaUpDown, nOldExtraFinePortaUpDown;
	uint8_t nOldPanSlide, nOldChnVolSlide;
	uint8_t nOldGlobalVolSlide;
	uint8_t nVibratoType, nVibratoSpeed, nVibratoDepth;
	uint8_t nTremoloType, nTremoloSpeed, nTremoloDepth;
	uint8_t nPanbrelloType, nPanbrelloSpeed, nPanbrelloDepth;
	int8_t  nPanbrelloOffset, nPanbrelloRandomMemory;
	uint8_t nOldCmdEx, nOldVolParam, nOldTempo;
	uint8_t nOldHiOffset;
	uint8_t nCutOff, nResonance;
	uint8_t nTremorCount, nTremorParam;
	uint8_t nPatternLoopCount;
	uint8_t nLeftVU, nRightVU;
	uint8_t nActiveMacro, nFilterMode;
	uint8_t nEFxSpeed, nEFxDelay;		// memory for Invert Loop (EFx, .MOD only)
	uint8_t nNoteSlideCounter, nNoteSlideSpeed, nNoteSlideStep;	// IMF / PTM Note Slide
	uint8_t lastZxxParam;	// Memory for \xx slides
	bool isFirstTick : 1;

	//-->Variables used to make user-definable tuning modes work with pattern effects.
	//If true, freq should be recalculated in ReadNote() on first tick.
	//Currently used only for vibrato things - using in other context might be 
	//problematic.
	bool m_ReCalculateFreqOnFirstTick : 1;

	//To tell whether to calculate frequency.
	bool m_CalculateFreq : 1;

	int32_t m_PortamentoFineSteps, m_PortamentoTickSlide;

	uint32_t m_Freq;
	float m_VibratoDepth;
	//<----

	//NOTE_PCs memory.
	float m_plugParamValueStep, m_plugParamTargetValue;
	uint16_t m_RowPlugParam;
	PLUGINDEX m_RowPlug;

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

	typedef uint32_t volume_t;
	volume_t GetVSTVolume() { return (pModInstrument) ? pModInstrument->nGlobalVol * 4 : nVolume; }

	// Check if the channel has a valid MIDI output. This function guarantees that pModInstrument != nullptr.
	bool HasMIDIOutput() const { return pModInstrument != nullptr && pModInstrument->HasValidMIDIChannel(); }

	// Check if currently processed loop is a sustain loop. pModSample is not checked for validity!
	bool InSustainLoop() const { return (dwFlags & (CHN_LOOP | CHN_KEYOFF)) == CHN_LOOP && pModSample->uFlags[CHN_SUSTAINLOOP]; }

	ModChannel()
	{
		memset(this, 0, sizeof(*this));
	}

};


// Default pattern channel settings
struct ModChannelSettings
{
	FlagSet<ChannelFlags> dwFlags;	// Channel flags
	uint16_t nPan;					// Initial pan (0...256)
	uint16_t nVolume;					// Initial channel volume (0...64)
	PLUGINDEX nMixPlugin;			// Assigned plugin
	char szName[MAX_CHANNELNAME];	// Channel name

	ModChannelSettings()
	{
		Reset();
	}

	void Reset()
	{
		dwFlags.reset();
		nPan = 128;
		nVolume = 64;
		nMixPlugin = 0;
		szName[0] = '\0';
	}
};

OPENMPT_NAMESPACE_END
