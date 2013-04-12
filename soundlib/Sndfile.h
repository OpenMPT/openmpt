/*
 * Sndfile.h
 * ---------
 * Purpose: Core class of the playback engine. Every song is represented by a CSoundFile object.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../soundlib/SoundFilePlayConfig.h"
#include "../soundlib/MixerSettings.h"
#include "../common/misc_util.h"
#include "mod_specifications.h"
#include <vector>
#include <bitset>
#include <set>
#include "Snd_defs.h"
#include "tuning.h"
#include "MIDIMacros.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/MIDIMapping.h"
#endif // MODPLUG_TRACKER

#include "ModSample.h"
#include "ModInstrument.h"
#include "ModChannel.h"
#include "modcommand.h"
#include "plugins/PlugInterface.h"
#include "RowVisitor.h"
#include "FileReader.h"

#include "Resampler.h"
#include "../sounddsp/Reverb.h"
#include "../sounddsp/AGC.h"
#include "../sounddsp/DSP.h"
#include "../sounddsp/EQ.h"

// -----------------------------------------------------------------------------------------
// MODULAR ModInstrument FIELD ACCESS : body content at the (near) top of Sndfile.cpp !!!
// -----------------------------------------------------------------------------------------
extern void WriteInstrumentHeaderStruct(ModInstrument * input, FILE * file);
extern char *GetInstrumentHeaderFieldPointer(const ModInstrument * input, uint32 fcode, uint16 fsize);
// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////
// Reverberation

#ifndef NO_REVERB

#define NUM_REVERBTYPES			29

LPCSTR GetReverbPresetName(UINT nPreset);

#endif

typedef VOID (__cdecl * LPSNDMIXHOOKPROC)(int *, unsigned long, unsigned long); // buffer, samples, channels

#include "pattern.h"
#include "patternContainer.h"
#include "ModSequence.h"


// Line ending types (for reading song messages from module files)
enum enmLineEndings
{
	leCR,			// Carriage Return (0x0D, \r)
	leLF,			// Line Feed (0x0A \n)
	leCRLF,			// Carriage Return, Line Feed (0x0D0A, \r\n)
	leMixed,		// It is not defined whether Carriage Return or Line Feed is the actual line ending. Both are accepted.
	leAutodetect,	// Detect suitable line ending
};

#define INTERNAL_LINEENDING	'\r'	// The character that represents line endings internally


#ifdef MODPLUG_TRACKER

// For WAV export (writing pattern positions to file)
struct PatternCuePoint
{
	uint64     offset;			// offset in the file (in samples)
	ORDERINDEX order;			// which order is this?
	bool       processed;		// has this point been processed by the main WAV render function yet?
};

#endif // MODPLUG_TRACKER


// Return values for GetLength()
struct GetLengthType
{
	double duration;		// total time in seconds
	ROWINDEX lastRow;		// last parsed row (dito)
	ROWINDEX endRow;		// last row before module loops (dito)
	ORDERINDEX lastOrder;	// last parsed order (if no target is specified, this is the first order that is parsed twice, i.e. not the *last* played order)
	ORDERINDEX endOrder;	// last order before module loops (UNDEFINED if a target is specified)
	bool targetReached;		// true if the specified order/row combination has been reached while going through the module
};


// Reset mode for GetLength()
enum enmGetLengthResetMode
{
	// Never adjust global variables / mod parameters
	eNoAdjust			= 0x00,
	// Mod parameters (such as global volume, speed, tempo, etc...) will always be memorized if the target was reached (i.e. they won't be reset to the previous values).  If target couldn't be reached, they are reset to their default values.
	eAdjust				= 0x01,
	// Same as above, but global variables will only be memorized if the target could be reached. This does *NOT* influence the visited rows vector - it will *ALWAYS* be adjusted in this mode.
	eAdjustOnSuccess	= 0x02 | eAdjust,
};


// Delete samples assigned to instrument
enum deleteInstrumentSamples
{
	deleteAssociatedSamples,
	doNoDeleteAssociatedSamples,
#ifdef MODPLUG_TRACKER
	askDeleteAssociatedSamples,
#endif // MODPLUG_TRACKER
};


//Note: These are bit indeces. MSF <-> Mod(Specific)Flag.
//If changing these, ChangeModTypeTo() might need modification.
FLAGSET(ModSpecificFlag)
{
	MSF_COMPATIBLE_PLAY		= 1,		//IT/MPT/XM
	MSF_OLDVOLSWING			= 2,		//IT/MPT
	MSF_MIDICC_BUGEMULATION	= 4,		//IT/MPT/XM
	MSF_OLD_MIDI_PITCHBENDS	= 8,		//IT/MPT/XM
};


class CTuningCollection;
#ifdef MODPLUG_TRACKER
class CModDoc;
#endif // MODPLUG_TRACKER

//==============
class CSoundFile
//==============
{
public:
	//Return true if title was changed.
	bool SetTitle(const char*, size_t strSize);

public: //Misc
	void ChangeModTypeTo(const MODTYPE& newType);

	// Returns value in seconds. If given position won't be played at all, returns -1.
	// If updateVars is true, the state of various playback variables will be updated according to the playback position.
	double GetPlaybackTimeAt(ORDERINDEX ord, ROWINDEX row, bool updateVars);

	uint16 GetModFlags() const {return static_cast<uint16>(m_ModFlags);}
	void SetModFlags(const uint16 v) {m_ModFlags = static_cast<ModSpecificFlag>(v);}
	bool GetModFlag(ModSpecificFlag i) const { return m_ModFlags.test(i); }
	void SetModFlag(ModSpecificFlag i, bool val) { m_ModFlags.set(i, val); }

	// Is compatible mode for a specific tracker turned on?
	// Hint 1: No need to poll for MOD_TYPE_MPT, as it will automatically be linked with MOD_TYPE_IT when using TRK_IMPULSETRACKER
	// Hint 2: Always returns true for MOD / S3M format (if that is the format of the current file)
	bool IsCompatibleMode(MODTYPE type) const
	{
		if(GetType() & type & (MOD_TYPE_MOD | MOD_TYPE_S3M))
			return true; // S3M and MOD format don't have compatibility flags, so we will always return true
		return ((GetType() & type) && GetModFlag(MSF_COMPATIBLE_PLAY));
	}

	// Process pingpong loops like Impulse Tracker (see Fastmix.cpp for an explanation)
	bool IsITPingPongMode() const
	{
		return IsCompatibleMode(TRK_IMPULSETRACKER);
	}

	// Check whether a filter algorithm closer to IT's should be used.
	bool UseITFilterMode() const { return IsCompatibleMode(TRK_IMPULSETRACKER) && !m_SongFlags[SONG_EXFILTERRANGE]; }

	//Tuning-->
public:
#ifdef MODPLUG_TRACKER
	static bool LoadStaticTunings();
	static bool SaveStaticTunings();
	static void DeleteStaticdata();
	static CTuningCollection& GetBuiltInTunings() {return *s_pTuningsSharedBuiltIn;}
	static CTuningCollection& GetLocalTunings() {return *s_pTuningsSharedLocal;}
#endif
	static CTuning *GetDefaultTuning() {return nullptr;}
	CTuningCollection& GetTuneSpecificTunings() {return *m_pTuningsTuneSpecific;}

	std::string GetNoteName(const int16&, const INSTRUMENTINDEX inst = INSTRUMENTINDEX_INVALID) const;
private:
	CTuningCollection* m_pTuningsTuneSpecific;
#ifdef MODPLUG_TRACKER
	static CTuningCollection* s_pTuningsSharedBuiltIn;
	static CTuningCollection* s_pTuningsSharedLocal;
#endif
	//<--Tuning

public: // get 'controllers'

#ifdef MODPLUG_TRACKER
	CMIDIMapper& GetMIDIMapper() {return m_MIDIMapper;}
	const CMIDIMapper& GetMIDIMapper() const {return m_MIDIMapper;}
#endif // MODPLUG_TRACKER

private: //Effect functions
	void PortamentoMPT(ModChannel*, int);
	void PortamentoFineMPT(ModChannel*, int);

private: //Misc private methods.
	static void SetModSpecsPointer(const CModSpecifications*& pModSpecs, const MODTYPE type);
	ModSpecificFlag GetModFlagMask(MODTYPE oldtype, MODTYPE newtype) const;

private: //'Controllers'

#ifdef MODPLUG_TRACKER
	CMIDIMapper m_MIDIMapper;
#endif // MODPLUG_TRACKER

private: //Misc data
	FlagSet<ModSpecificFlag, uint16> m_ModFlags;
	const CModSpecifications* m_pModSpecs;

private:
	DWORD gdwSysInfo;

private:
	// Front Mix Buffer (Also room for interleaved rear mix)
	int MixSoundBuffer[MIXBUFFERSIZE * 4];
	int MixRearBuffer[MIXBUFFERSIZE * 2];
#ifndef NO_REVERB
	int MixReverbBuffer[MIXBUFFERSIZE * 2];
#endif
	float MixFloatBuffer[MIXBUFFERSIZE * 2];
	LONG gnDryLOfsVol;
	LONG gnDryROfsVol;

public:
	MixerSettings m_MixerSettings;
	CResampler m_Resampler;
#ifndef NO_REVERB
	CReverb m_Reverb;
#endif
#ifndef NO_DSP
	CDSP m_DSP;
#endif
#ifndef NO_EQ
	CEQ m_EQ;
#endif
#ifndef NO_AGC
	CAGC m_AGC;
#endif
	UINT gnVolumeRampUpSamplesActual;
#ifdef MODPLUG_TRACKER
	static LPSNDMIXHOOKPROC gpSndMixHook;
#endif
#ifndef NO_VST
	static PMIXPLUGINCREATEPROC gpMixPluginCreateProc;
#endif

	typedef uint32 samplecount_t;	// Number of rendered samples

public:	// for Editing
#ifdef MODPLUG_TRACKER
	CModDoc *m_pModDoc;		// Can be a null pointer for example when previewing samples from the treeview.
#endif // MODPLUG_TRACKER
	FlagSet<MODTYPE> m_nType;
	CHANNELINDEX m_nChannels;
	SAMPLEINDEX m_nSamples;
	INSTRUMENTINDEX m_nInstruments;
	UINT m_nDefaultSpeed, m_nDefaultTempo, m_nDefaultGlobalVolume;
	FlagSet<SongFlags> m_SongFlags;
	bool m_bIsRendering;
	UINT m_nMixChannels, m_nMixStat;
	samplecount_t m_nBufferCount;
	double m_dBufferDiff;
	UINT m_nTickCount;
	UINT m_nPatternDelay, m_nFrameDelay;	// m_nPatternDelay = pattern delay (rows), m_nFrameDelay = fine pattern delay (ticks)
	samplecount_t m_lTotalSampleCount;	// rewbs.VSTTimeInfo
	bool m_bPositionChanged;		// Report to plugins that we jumped around in the module
	UINT m_nSamplesPerTick;		// rewbs.betterBPM
	ROWINDEX m_nDefaultRowsPerBeat, m_nDefaultRowsPerMeasure;	// default rows per beat and measure for this module // rewbs.betterBPM
	ROWINDEX m_nCurrentRowsPerBeat, m_nCurrentRowsPerMeasure;	// current rows per beat and measure for this module
	BYTE m_nTempoMode;			// rewbs.betterBPM
	BYTE m_nMixLevels;
	UINT m_nMusicSpeed, m_nMusicTempo;	// Current speed and tempo
	ROWINDEX m_nNextRow, m_nRow;
	ROWINDEX m_nNextPatStartRow; // for FT2's E60 bug
	PATTERNINDEX m_nPattern;
	ORDERINDEX m_nCurrentOrder, m_nNextOrder, m_nRestartPos, m_nSeqOverride;

#ifdef MODPLUG_TRACKER
	// Lock playback between two orders. Lock is active if lock start != ORDERINDEX_INVALID).
	ORDERINDEX m_lockOrderStart, m_lockOrderEnd;
#endif // MODPLUG_TRACKER

	bool m_bPatternTransitionOccurred;
	UINT m_nGlobalVolume, m_nSamplesToGlobalVolRampDest, m_nGlobalVolumeRampAmount,
		 m_nGlobalVolumeDestination, m_nSamplePreAmp, m_nVSTiVolume;
	long m_lHighResRampingGlobalVolume;
	UINT m_nFreqFactor, m_nTempoFactor, m_nOldGlbVolSlide;
	LONG m_nMinPeriod, m_nMaxPeriod;	// min period = highest possible frequency, max period = lowest possible frequency
	LONG m_nRepeatCount;	// -1 means repeat infinitely.
	DWORD m_nGlobalFadeSamples, m_nGlobalFadeMaxSamples;
	UINT m_nMaxOrderPosition;
	LPSTR m_lpszSongComments;
	UINT ChnMix[MAX_CHANNELS];							// Channels to be mixed
	ModChannel Chn[MAX_CHANNELS];						// Mixing channels... First m_nChannel channels are master channels (i.e. they are never NNA channels)!
	ModChannelSettings ChnSettings[MAX_BASECHANNELS];	// Initial channels settings
	CPatternContainer Patterns;							// Patterns
	ModSequenceSet Order;								// Modsequences. Order[x] returns an index of a pattern located at order x of the current sequence.
protected:
	ModSample Samples[MAX_SAMPLES];						// Sample Headers
public:
	ModInstrument *Instruments[MAX_INSTRUMENTS];		// Instrument Headers
	MIDIMacroConfig m_MidiCfg;							// MIDI Macro config table
	SNDMIXPLUGIN m_MixPlugins[MAX_MIXPLUGINS];			// Mix plugins
	char m_szNames[MAX_SAMPLES][MAX_SAMPLENAME];		// Song and sample names
	std::bitset<MAX_BASECHANNELS> m_bChannelMuteTogglePending;

	CSoundFilePlayConfig m_PlayConfig;
	DWORD m_dwCreatedWithVersion;
	DWORD m_dwLastSavedWithVersion;

#ifdef MODPLUG_TRACKER
	std::vector<PatternCuePoint> m_PatternCuePoints;	// For WAV export (writing pattern positions to file)
#endif // MODPLUG_TRACKER

	// For handling backwards jumps and stuff to prevent infinite loops when counting the mod length or rendering to wav.
	RowVisitor visitedSongRows;

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	mpt::String m_szInstrumentPath[MAX_INSTRUMENTS];
// -! NEW_FEATURE#0023

public:
	CSoundFile();
	~CSoundFile();

public:

	void AddToLog(const std::string &text) const;

#ifdef MODPLUG_TRACKER
	BOOL Create(LPCBYTE lpStream, CModDoc *pModDoc, DWORD dwMemLength=0);
	// Get parent CModDoc. Can be nullptr if previewing from tree view, and is always nullptr if we're not actually compiling OpenMPT.
	CModDoc *GetpModDoc() const { return m_pModDoc; }
#else
	BOOL Create(LPCBYTE lpStream, void *pModDoc, DWORD dwMemLength=0);
	void *GetpModDoc() const { return nullptr; }
#endif // MODPLUG_TRACKER

	BOOL Destroy();
	MODTYPE GetType() const { return m_nType; }
	bool TypeIsIT_MPT() const { return (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) != 0; }
	bool TypeIsIT_MPT_XM() const { return (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)) != 0; }
	bool TypeIsS3M_IT_MPT() const { return (m_nType & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)) != 0; }

	void SetPreAmp(UINT vol);
	UINT GetPreAmp() const { return m_MixerSettings.m_nPreAmp; }

	INSTRUMENTINDEX GetNumInstruments() const { return m_nInstruments; }
	SAMPLEINDEX GetNumSamples() const { return m_nSamples; }
	UINT GetCurrentPos() const;
	PATTERNINDEX GetCurrentPattern() const { return m_nPattern; }
	ORDERINDEX GetCurrentOrder() const { return m_nCurrentOrder; }
	CHANNELINDEX GetNumChannels() const { return m_nChannels; }

	IMixPlugin* GetInstrumentPlugin(INSTRUMENTINDEX instr);
	const CModSpecifications& GetModSpecifications() const {return *m_pModSpecs;}
	static const CModSpecifications& GetModSpecifications(const MODTYPE type);

	void PatternTranstionChnSolo(const CHANNELINDEX chnIndex);
	void PatternTransitionChnUnmuteAll();
	double GetCurrentBPM() const;
	void DontLoopPattern(PATTERNINDEX nPat, ROWINDEX nRow = 0);		//rewbs.playSongFromCursor
	void SetCurrentPos(UINT nPos);
	void SetCurrentOrder(ORDERINDEX nOrder);
	LPCSTR GetTitle() const { return m_szNames[0]; }
	LPCTSTR GetSampleName(UINT nSample) const;
	const char *GetInstrumentName(INSTRUMENTINDEX nInstr) const;
	UINT GetMusicSpeed() const { return m_nMusicSpeed; }
	UINT GetMusicTempo() const { return m_nMusicTempo; }

	//Get modlength in various cases: total length, length to
	//specific order&row etc. Return value is in seconds.
	GetLengthType GetLength(enmGetLengthResetMode adjustMode, ORDERINDEX ord = ORDERINDEX_INVALID, ROWINDEX row = ROWINDEX_INVALID);

public:
	//Returns song length in seconds.
	DWORD GetSongTime() { return static_cast<DWORD>(GetLength(eNoAdjust).duration + 0.5); }

	void RecalculateSamplesPerTick();
	double GetRowDuration(UINT tempo, UINT speed) const;
	UINT GetTickDuration(UINT tempo, UINT speed, ROWINDEX rowsPerBeat);

	// A repeat count value of -1 means infinite loop
	void SetRepeatCount(int n) { m_nRepeatCount = n; }
	int GetRepeatCount() const { return m_nRepeatCount; }
	bool IsPaused() const {	return m_SongFlags[SONG_PAUSED | SONG_STEP]; }	// Added SONG_STEP as it seems to be desirable in most cases to check for this as well.
	void LoopPattern(PATTERNINDEX nPat, ROWINDEX nRow = 0);

	bool InitChannel(CHANNELINDEX nChn);

	// Module Loaders
	bool ReadXM(FileReader &file);
	bool ReadS3M(FileReader &file);
	bool ReadMod(FileReader &file);
	bool ReadM15(FileReader &file);
	bool ReadMed(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMTM(FileReader &file);
	bool ReadSTM(FileReader &file);
	bool ReadIT(FileReader &file);
	bool ReadITProject(FileReader &file);
	bool Read669(FileReader &file);
	bool ReadUlt(FileReader &file);
	bool ReadWav(FileReader &file);
	bool ReadDSM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadFAR(FileReader &file);
	bool ReadAMS(FileReader &file);
	bool ReadAMS2(FileReader &file);
	bool ReadMDL(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadOKT(FileReader &file);
	bool ReadDMF(FileReader &file);
	bool ReadPTM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadDBM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadAMF_Asylum(FileReader &file);
	bool ReadAMF_DSMI(FileReader &file);
	bool ReadMT2(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadPSM(FileReader &file);
	bool ReadPSM16(FileReader &file);
	bool ReadUMX(FileReader &file);
	bool ReadMO3(FileReader &file);
	bool ReadGDM(FileReader &file);
	bool ReadIMF(FileReader &file);
	bool ReadAM(FileReader &file);
	bool ReadJ2B(FileReader &file);
	bool ReadMID(const LPCBYTE lpStream, DWORD dwMemLength);

	static std::vector<const char *> GetSupportedExtensions(bool otherFormats);

	void UpgradeModFlags();
	void UpgradeSong();

	// Save Functions
#ifndef MODPLUG_NO_FILESAVE
	bool SaveXM(LPCSTR lpszFileName, bool compatibilityExport = false);
	bool SaveS3M(LPCSTR lpszFileName) const;
	bool SaveMod(LPCSTR lpszFileName) const;
	bool SaveIT(LPCSTR lpszFileName, bool compatibilityExport = false);
	bool SaveITProject(LPCSTR lpszFileName); // -> CODE#0023 -> DESC="IT project files (.itp)" -! NEW_FEATURE#0023
	UINT SaveMixPlugins(FILE *f=NULL, BOOL bUpdate=TRUE);
	void WriteInstrumentPropertyForAllInstruments(__int32 code,  __int16 size, FILE* f, UINT nInstruments) const;
	void SaveExtendedInstrumentProperties(UINT nInstruments, FILE* f) const;
	void SaveExtendedSongProperties(FILE* f) const;
	size_t SaveModularInstrumentData(FILE *f, const ModInstrument *pIns) const;
#endif // MODPLUG_NO_FILESAVE
	void LoadExtendedSongProperties(const MODTYPE modtype, FileReader &file, bool* pInterpretMptMade = nullptr);
	static size_t LoadModularInstrumentData(FileReader &file, ModInstrument &ins);

	// Reads extended instrument properties(XM/IT/MPTM).
	// If no errors occur and song extension tag is found, returns pointer to the beginning
	// of the tag, else returns NULL.
	void LoadExtendedInstrumentProperties(FileReader &file, bool *pInterpretMptMade = nullptr);

	// MOD Convert function
	MODTYPE GetBestSaveFormat() const;
	void ConvertModCommand(ModCommand &m) const;
	void S3MConvert(ModCommand &m, bool fromIT) const;
	void S3MSaveConvert(uint8 &command, uint8 &param, bool toIT, bool compatibilityExport = false) const;
	void ModSaveCommand(uint8 &command, uint8 &param, const bool toXM, const bool compatibilityExport = false) const;

	void SetupMODPanning(bool bForceSetup = false); // Setup LRRL panning, max channel volume

public:
	// Real-time sound functions
	void SuspendPlugins(); //rewbs.VSTCompliance
	void ResumePlugins();  //rewbs.VSTCompliance
	void StopAllVsti();    //rewbs.VSTCompliance
	void RecalculateGainForAllPlugs();
	void ResetChannels();
	UINT Read(LPVOID lpBuffer, UINT cbBuffer);
	UINT CreateStereoMix(int count);
	UINT GetResamplingFlag(const ModChannel *pChannel);
	BOOL FadeSong(UINT msec);
	BOOL GlobalFadeSong(UINT msec);
	void ProcessPlugins(UINT nCount);
	samplecount_t GetTotalSampleCount() const { return m_lTotalSampleCount; }
	bool HasPositionChanged() { bool b = m_bPositionChanged; m_bPositionChanged = false; return b; }
	bool IsRenderingToDisc() const { return m_bIsRendering; }

public:
	// Mixer Config
	void SetMixerSettings(const MixerSettings &mixersettings);
	void SetResamplerSettings(const CResamplerSettings &resamplersettings);
	void InitPlayer(BOOL bReset=FALSE);
	void SetDspEffects(DWORD DSPMask);
	DWORD GetSampleRate() { return m_MixerSettings.gdwMixingFreq; }
	static DWORD GetSysInfo();
#ifndef NO_EQ
	void SetEQGains(const UINT *pGains, UINT nBands, const UINT *pFreqs=NULL, BOOL bReset=FALSE)	{ m_EQ.SetEQGains(pGains, nBands, pFreqs, bReset, m_MixerSettings.gdwMixingFreq); } // 0=-12dB, 32=+12dB
#endif // NO_EQ
	// Float <-> Int conversion routines
	/*static */VOID StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount);
	/*static */VOID FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount);
	/*static */VOID MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount);
	/*static */VOID FloatToMonoMix(const float *pIn, int *pOut, UINT nCount);

public:
	BOOL ReadNote();
	BOOL ProcessRow();
	BOOL ProcessEffects();
	CHANNELINDEX GetNNAChannel(CHANNELINDEX nChn) const;
	void CheckNNA(CHANNELINDEX nChn, UINT instr, int note, bool forceCut);
	void NoteChange(CHANNELINDEX nChn, int note, bool bPorta = false, bool bResetEnv = true, bool bManual = false);
	void InstrumentChange(ModChannel *pChn, UINT instr, bool bPorta = false, bool bUpdVol = true, bool bResetEnv = true);

	// Channel Effects
	void KeyOff(CHANNELINDEX nChn);
	// Global Effects
	void SetTempo(UINT param, bool setAsNonModcommand = false);
	void SetSpeed(UINT param);

protected:
	// Channel effect processing
	int GetVibratoDelta(int type, int position) const;

	void ProcessVolumeSwing(ModChannel *pChn, int &vol);
	void ProcessPanningSwing(ModChannel *pChn);
	void ProcessTremolo(ModChannel *pChn, int &vol);
	void ProcessTremor(ModChannel *pChn, int &vol);

	bool IsEnvelopeProcessed(const ModChannel *pChn, enmEnvelopeTypes env) const;
	void ProcessVolumeEnvelope(ModChannel *pChn, int &vol);
	void ProcessPanningEnvelope(ModChannel *pChn);
	void ProcessPitchFilterEnvelope(ModChannel *pChn, int &period);

	void IncrementEnvelopePosition(ModChannel *pChn, enmEnvelopeTypes envType);
	void IncrementEnvelopePositions(ModChannel *pChn);

	void ProcessInstrumentFade(ModChannel *pChn, int &vol);

	void ProcessPitchPanSeparation(ModChannel *pChn);
	void ProcessPanbrello(ModChannel *pChn);

	void ProcessArpeggio(ModChannel *pChn, int &period, CTuning::NOTEINDEXTYPE &arpeggioSteps);
	void ProcessVibrato(CHANNELINDEX nChn, int &period, CTuning::RATIOTYPE &vibratoFactor);
	void ProcessSampleAutoVibrato(ModChannel *pChn, int &period, CTuning::RATIOTYPE &vibratoFactor, int &nPeriodFrac);

	void ProcessRamping(ModChannel *pChn);

protected:
	// Channel Effects
	void UpdateS3MEffectMemory(ModChannel *pChn, UINT param) const;
	void PortamentoUp(CHANNELINDEX nChn, UINT param, const bool doFinePortamentoAsRegular = false);
	void PortamentoDown(CHANNELINDEX nChn, UINT param, const bool doFinePortamentoAsRegular = false);
	void MidiPortamento(CHANNELINDEX nChn, int param, bool doFineSlides);
	void FinePortamentoUp(ModChannel *pChn, UINT param);
	void FinePortamentoDown(ModChannel *pChn, UINT param);
	void ExtraFinePortamentoUp(ModChannel *pChn, UINT param);
	void ExtraFinePortamentoDown(ModChannel *pChn, UINT param);
	void NoteSlide(ModChannel *pChn, UINT param, bool slideUp);
	void TonePortamento(ModChannel *pChn, UINT param);
	void Vibrato(ModChannel *pChn, UINT param);
	void FineVibrato(ModChannel *pChn, UINT param);
	void VolumeSlide(ModChannel *pChn, UINT param);
	void PanningSlide(ModChannel *pChn, UINT param, bool memory = true);
	void ChannelVolSlide(ModChannel *pChn, UINT param);
	void FineVolumeUp(ModChannel *pChn, UINT param, bool volCol);
	void FineVolumeDown(ModChannel *pChn, UINT param, bool volCol);
	void Tremolo(ModChannel *pChn, UINT param);
	void Panbrello(ModChannel *pChn, UINT param);
	void RetrigNote(CHANNELINDEX nChn, int param, UINT offset=0);  //rewbs.volOffset: added last param
	void SampleOffset(CHANNELINDEX nChn, UINT param);
	void NoteCut(CHANNELINDEX nChn, UINT nTick);
	ROWINDEX PatternLoop(ModChannel *, UINT param);
	void ExtendedMODCommands(CHANNELINDEX nChn, UINT param);
	void ExtendedS3MCommands(CHANNELINDEX nChn, UINT param);
	void ExtendedChannelEffect(ModChannel *, UINT param);
	void InvertLoop(ModChannel* pChn);

	void ProcessMacroOnChannel(CHANNELINDEX nChn);
	void ProcessMIDIMacro(CHANNELINDEX nChn, bool isSmooth, char *macro, uint8 param = 0, PLUGINDEX plugin = 0);
	float CalculateSmoothParamChange(float currentValue, float param) const;
	size_t SendMIDIData(CHANNELINDEX nChn, bool isSmooth, const unsigned char *macro, size_t macroLen, PLUGINDEX plugin);

	void SetupChannelFilter(ModChannel *pChn, bool bReset, int flt_modifier = 256) const;
	// Low-Level effect processing
	void DoFreqSlide(ModChannel *pChn, LONG nFreqSlide);
	void GlobalVolSlide(UINT param, UINT &nOldGlobalVolSlide);
	DWORD IsSongFinished(UINT nOrder, UINT nRow) const;
	void UpdateTimeSignature();

	UINT GetNumTicksOnCurrentRow() const
	{
		return (m_nMusicSpeed  + m_nFrameDelay) * MAX(m_nPatternDelay, 1);
	}

public:
	bool DestroySample(SAMPLEINDEX nSample);
	bool DestroySampleThreadsafe(SAMPLEINDEX nSample);

	// Find an unused sample slot. If it is going to be assigned to an instrument, targetInstrument should be specified.
	// SAMPLEINDEX_INVLAID is returned if no free sample slot could be found.
	SAMPLEINDEX GetNextFreeSample(INSTRUMENTINDEX targetInstrument = INSTRUMENTINDEX_INVALID, SAMPLEINDEX start = 1) const;
	// Find an unused instrument slot.
	// INSTRUMENTINDEX_INVALID is returned if no free instrument slot could be found.
	INSTRUMENTINDEX GetNextFreeInstrument(INSTRUMENTINDEX start = 1) const;
	// Check whether a given sample is used by a given instrument.
	bool IsSampleReferencedByInstrument(SAMPLEINDEX sample, INSTRUMENTINDEX instr) const;

	bool DestroyInstrument(INSTRUMENTINDEX nInstr, deleteInstrumentSamples removeSamples);
	bool IsSampleUsed(SAMPLEINDEX nSample) const;
	bool IsInstrumentUsed(INSTRUMENTINDEX nInstr) const;
	bool RemoveInstrumentSamples(INSTRUMENTINDEX nInstr);
	SAMPLEINDEX DetectUnusedSamples(std::vector<bool> &sampleUsed) const;
	SAMPLEINDEX RemoveSelectedSamples(const std::vector<bool> &keepSamples);
	static void AdjustSampleLoop(ModSample &sample);

	// Samples file I/O
	bool ReadSampleFromFile(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadWAVSample(SAMPLEINDEX nSample, FileReader &file, FileReader *wsmpChunk = nullptr);
	bool ReadPATSample(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadS3ISample(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadAIFFSample(SAMPLEINDEX nSample, FileReader &file);
	bool ReadXISample(SAMPLEINDEX nSample, FileReader &file);
	bool ReadITSSample(SAMPLEINDEX nSample, FileReader &file, bool rewind = true);
	bool ReadITISample(SAMPLEINDEX nSample, FileReader &file);
	bool Read8SVXSample(SAMPLEINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadFLACSample(SAMPLEINDEX sample, FileReader &file);
	bool ReadMP3Sample(SAMPLEINDEX sample, FileReader &file);
	bool SaveWAVSample(SAMPLEINDEX nSample, const LPCSTR lpszFileName) const;
	bool SaveRAWSample(SAMPLEINDEX nSample, const LPCSTR lpszFileName) const;
	bool SaveFLACSample(SAMPLEINDEX nSample, const LPCSTR lpszFileName) const;

	// Instrument file I/O
	bool ReadInstrumentFromFile(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadXIInstrument(INSTRUMENTINDEX nInstr, FileReader &file);
	bool ReadITIInstrument(INSTRUMENTINDEX nInstr, FileReader &file);
	bool ReadPATInstrument(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadSampleAsInstrument(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool SaveXIInstrument(INSTRUMENTINDEX nInstr, const LPCSTR lpszFileName) const;
	bool SaveITIInstrument(INSTRUMENTINDEX nInstr, const LPCSTR lpszFileName, bool compress) const;

	// I/O from another sound file
	bool ReadInstrumentFromSong(INSTRUMENTINDEX targetInstr, const CSoundFile &srcSong, INSTRUMENTINDEX sourceInstr);
	bool ReadSampleFromSong(SAMPLEINDEX targetSample, const CSoundFile &srcSong, SAMPLEINDEX sourceSample);

	// Period/Note functions
	UINT GetNoteFromPeriod(UINT period) const;
	UINT GetPeriodFromNote(UINT note, int nFineTune, UINT nC5Speed) const;
	UINT GetFreqFromPeriod(UINT period, UINT nC5Speed, int nPeriodFrac=0) const;
	// Misc functions
	ModSample &GetSample(SAMPLEINDEX sample) { ASSERT(sample <= m_nSamples && sample < CountOf(Samples)); return Samples[sample]; }
	const ModSample &GetSample(SAMPLEINDEX sample) const { ASSERT(sample <= m_nSamples && sample < CountOf(Samples)); return Samples[sample]; }

	UINT MapMidiInstrument(DWORD dwProgram, UINT nChannel, UINT nNote);
	size_t ITInstrToMPT(FileReader &file, ModInstrument &ins, uint16 trkvers);
	void LoadMixPlugins(FileReader &file);

	DWORD CutOffToFrequency(UINT nCutOff, int flt_modifier=256) const; // [0-127] => [1-10KHz]
#ifdef MODPLUG_TRACKER
	void ProcessMidiOut(CHANNELINDEX nChn);
#endif // MODPLUG_TRACKER
	void ApplyGlobalVolume(int SoundBuffer[], int RearBuffer[], long lTotalSampleCount);

	// System-Dependant functions
public:
	static void *AllocateSample(UINT nbytes);
	static void FreeSample(void *p);

	ModInstrument *AllocateInstrument(INSTRUMENTINDEX instr, SAMPLEINDEX assignedSample = 0);

	// WAV export
	UINT Normalize24BitBuffer(LPBYTE pbuffer, UINT cbsizebytes, DWORD lmax24, DWORD dwByteInc);

	// Song message helper functions
public:
	// Allocate memory for song message.
	// [in]  length: text length in characters, without possible trailing null terminator.
	// [out] returns true on success.
	bool AllocateMessage(size_t length);

	// Free previously allocated song message memory.
	void FreeMessage();

	// Retrieve song message.
	// [in]  lineEnding: line ending formatting of the text in memory.
	// [in]  pTextConverter: Pointer to a callback function which can be used to post-process the written characters, if necessary (nullptr otherwise).
	// [out] returns formatted song message.
	mpt::String GetSongMessage(const enmLineEndings lineEnding, void (*pTextConverter)(char &) = nullptr) const;

protected:
	// Read song message from a mapped file.
	// [in]  data: pointer to the data in memory that is going to be read
	// [in]  length: number of characters that should be read, not including a possible trailing null terminator (it is automatically appended).
	// [in]  lineEnding: line ending formatting of the text in memory.
	// [in]  pTextConverter: Pointer to a callback function which can be used to pre-process the read characters, if necessary (nullptr otherwise).
	// [out] returns true on success.
	bool ReadMessage(const BYTE *data, FileReader::off_t length, enmLineEndings lineEnding, void (*pTextConverter)(char &) = nullptr);
	bool ReadMessage(FileReader &file, FileReader::off_t length, enmLineEndings lineEnding, void (*pTextConverter)(char &) = nullptr)
	{
		FileReader::off_t readLength = Util::Min(length, file.BytesLeft());
		bool success = ReadMessage(reinterpret_cast<const BYTE*>(file.GetRawData()), readLength, lineEnding, pTextConverter);
		file.Skip(readLength);
		return success;
	}

	// Read comments with fixed line length from a mapped file.
	// [in]  data: pointer to the data in memory that is going to be read
	// [in]  length: number of characters that should be read, not including a possible trailing null terminator (it is automatically appended).
	// [in]  lineLength: The fixed length of a line.
	// [in]  lineEndingLength: The padding space between two fixed lines. (there could for example be a null char after every line)
	// [in]  pTextConverter: Pointer to a callback function which can be used to pre-process the read characters, if necessary (nullptr otherwise).
	// [out] returns true on success.
	bool ReadFixedLineLengthMessage(const BYTE *data, const FileReader::off_t length, const size_t lineLength, const size_t lineEndingLength, void (*pTextConverter)(char &) = nullptr);
	bool ReadFixedLineLengthMessage(FileReader &file, const FileReader::off_t length, const size_t lineLength, const size_t lineEndingLength, void (*pTextConverter)(char &) = nullptr)
	{
		FileReader::off_t readLength = Util::Min(length, file.BytesLeft());
		bool success = ReadFixedLineLengthMessage(reinterpret_cast<const BYTE*>(file.GetRawData()), readLength, lineLength, lineEndingLength, pTextConverter);
		file.Skip(readLength);
		return success;
	}

private:
	PLUGINDEX __cdecl GetChannelPlugin(CHANNELINDEX nChn, PluginMutePriority respectMutes) const;
	PLUGINDEX __cdecl GetActiveInstrumentPlugin(CHANNELINDEX, PluginMutePriority respectMutes) const;
	IMixPlugin *__cdecl GetChannelInstrumentPlugin(CHANNELINDEX chn) const;

	void HandlePatternTransitionEvents();

public:
	PLUGINDEX GetBestPlugin(CHANNELINDEX nChn, PluginPriority priority, PluginMutePriority respectMutes) const;
	uint8 GetBestMidiChannel(CHANNELINDEX nChn) const;

};

#pragma warning(default : 4324) //structure was padded due to __declspec(align())


inline IMixPlugin* CSoundFile::GetInstrumentPlugin(INSTRUMENTINDEX instr)
//-----------------------------------------------------------------------
{
	if(instr > 0 && instr < MAX_INSTRUMENTS && Instruments[instr] && Instruments[instr]->nMixPlug && Instruments[instr]->nMixPlug <= MAX_MIXPLUGINS)
		return m_MixPlugins[Instruments[instr]->nMixPlug-1].pMixPlugin;
	else
		return NULL;
}


///////////////////////////////////////////////////////////
// Low-level Mixing functions

#define SCRATCH_BUFFER_SIZE 64 //Used for plug's final processing (cleanup)
#define MIXING_ATTENUATION	4
#define VOLUMERAMPPRECISION	12
#define FADESONGDELAY		100

// Calling conventions
#ifdef WIN32
#define MPPASMCALL	__cdecl
#define MPPFASTCALL	__fastcall
#else
#define MPPASMCALL
#define MPPFASTCALL
#endif

#define MOD2XMFineTune(k)	((int)( (signed char)((k)<<4) ))
#define XM2MODFineTune(k)	((int)( (k>>4)&0x0f ))

// Read instrument property with 'code' and 'size' from 'file' to instrument 'pIns'.
void ReadInstrumentExtensionField(ModInstrument* pIns, const uint32 code, const uint16 size, FileReader &file);

// Read instrument property with 'code' from 'file' to instrument 'pIns'.
void ReadExtendedInstrumentProperty(ModInstrument* pIns, const uint32 code, FileReader &file);

// Read extended instrument properties from 'file' to instrument 'pIns'.
void ReadExtendedInstrumentProperties(ModInstrument* pIns, FileReader &file);

// Convert instrument flags which were read from 'dF..' extension to proper internal representation.
void ConvertReadExtendedFlags(ModInstrument* pIns);
