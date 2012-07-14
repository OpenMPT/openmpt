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

#include "../mptrack/SoundFilePlayConfig.h"
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

// -----------------------------------------------------------------------------------------
// MODULAR ModInstrument FIELD ACCESS : body content at the (near) top of Sndfile.cpp !!!
// -----------------------------------------------------------------------------------------
extern void WriteInstrumentHeaderStruct(ModInstrument * input, FILE * file);
extern BYTE * GetInstrumentHeaderFieldPointer(const ModInstrument * input, __int32 fcode, __int16 fsize);
// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////
// Reverberation

struct SNDMIX_REVERB_PROPERTIES
{
	LONG  lRoom;                   // [-10000, 0]      default: -10000 mB
    LONG  lRoomHF;                 // [-10000, 0]      default: 0 mB
    FLOAT flDecayTime;             // [0.1, 20.0]      default: 1.0 s
    FLOAT flDecayHFRatio;          // [0.1, 2.0]       default: 0.5
    LONG  lReflections;            // [-10000, 1000]   default: -10000 mB
    FLOAT flReflectionsDelay;      // [0.0, 0.3]       default: 0.02 s
    LONG  lReverb;                 // [-10000, 2000]   default: -10000 mB
    FLOAT flReverbDelay;           // [0.0, 0.1]       default: 0.04 s
    FLOAT flDiffusion;             // [0.0, 100.0]     default: 100.0 %
    FLOAT flDensity;               // [0.0, 100.0]     default: 100.0 %
};
typedef SNDMIX_REVERB_PROPERTIES* PSNDMIX_REVERB_PROPERTIES;

#ifndef NO_REVERB

#define NUM_REVERBTYPES			29

LPCSTR GetReverbPresetName(UINT nPreset);

#endif

typedef VOID (__cdecl * LPSNDMIXHOOKPROC)(int *, unsigned long, unsigned long); // buffer, samples, channels

#include "pattern.h"
#include "patternContainer.h"
#include "ModSequence.h"
#include "PlaybackEventer.h"


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


// For WAV export (writing pattern positions to file)
struct PatternCuePoint
{
	bool		processed;		// has this point been processed by the main WAV render function yet?
	ULONGLONG	offset;			// offset in the file (in samples)
	ORDERINDEX	order;			// which order is this?
};


// Return values for GetLength()
struct GetLengthType
{
	double duration;		// total time in seconds
	bool targetReached;		// true if the specified order/row combination has been reached while going through the module
	ORDERINDEX lastOrder;	// last parsed order (if no target is specified, this is the first order that is parsed twice, i.e. not the *last* played order)
	ROWINDEX lastRow;		// last parsed row (dito)
	ORDERINDEX endOrder;	// last order before module loops (UNDEFINED if a target is specified)
	ROWINDEX endRow;		// last row before module loops (dito)
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
const BYTE MSF_COMPATIBLE_PLAY		= 0;		//IT/MPT/XM
const BYTE MSF_OLDVOLSWING			= 1;		//IT/MPT
const BYTE MSF_MIDICC_BUGEMULATION	= 2;		//IT/MPT/XM

class CTuningCollection;
class CModDoc;

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

	uint16 GetModFlags() const {return m_ModFlags;}
	void SetModFlags(const uint16 v) {m_ModFlags = v;}
	bool GetModFlag(BYTE i) const {return ((m_ModFlags & (1<<i)) != 0);}
	void SetModFlag(BYTE i, bool val) {if(i < 8*sizeof(m_ModFlags)) {m_ModFlags = (val) ? m_ModFlags |= (1 << i) : m_ModFlags &= ~(1 << i);}}

	// Is compatible mode for a specific tracker turned on?
	// Hint 1: No need to poll for MOD_TYPE_MPT, as it will automatically be linked with MOD_TYPE_IT when using TRK_IMPULSETRACKER
	// Hint 2: Always returns true for MOD / S3M format (if that is the format of the current file)
	bool IsCompatibleMode(MODTYPE type) const
	{
		if(GetType() & type & (MOD_TYPE_MOD | MOD_TYPE_S3M))
			return true; // S3M and MOD format don't have compatibility flags, so we will always return true
		return ((GetType() & type) && GetModFlag(MSF_COMPATIBLE_PLAY));
	}

	// Check whether a filter algorithm closer to IT's should be used.
	bool UseITFilterMode() const { return IsCompatibleMode(TRK_IMPULSETRACKER) && !m_SongFlags[SONG_EXFILTERRANGE]; }

	//Tuning-->
public:
	static bool LoadStaticTunings();
	static bool SaveStaticTunings();
	static void DeleteStaticdata();
	static CTuningCollection& GetBuiltInTunings() {return *s_pTuningsSharedBuiltIn;}
	static CTuningCollection& GetLocalTunings() {return *s_pTuningsSharedLocal;}
	CTuningCollection& GetTuneSpecificTunings() {return *m_pTuningsTuneSpecific;}

	std::string GetNoteName(const int16&, const INSTRUMENTINDEX inst = INSTRUMENTINDEX_INVALID) const;
private:
	CTuningCollection* m_pTuningsTuneSpecific;
	static CTuningCollection* s_pTuningsSharedBuiltIn;
	static CTuningCollection* s_pTuningsSharedLocal;
	//<--Tuning

public: //Get 'controllers'
	CPlaybackEventer& GetPlaybackEventer() {return m_PlaybackEventer;}
	const CPlaybackEventer& GetPlaybackEventer() const {return m_PlaybackEventer;}

#ifdef MODPLUG_TRACKER
	CMIDIMapper& GetMIDIMapper() {return m_MIDIMapper;}
	const CMIDIMapper& GetMIDIMapper() const {return m_MIDIMapper;}
#endif // MODPLUG_TRACKER

private: //Effect functions
	void PortamentoMPT(ModChannel*, int);
	void PortamentoFineMPT(ModChannel*, int);

private: //Misc private methods.
	static void SetModSpecsPointer(const CModSpecifications*& pModSpecs, const MODTYPE type);
	uint16 GetModFlagMask(const MODTYPE oldtype, const MODTYPE newtype) const;

private: //'Controllers'
	CPlaybackEventer m_PlaybackEventer;

#ifdef MODPLUG_TRACKER
	CMIDIMapper m_MIDIMapper;
#endif // MODPLUG_TRACKER

private: //Misc data
	uint16 m_ModFlags;
	const CModSpecifications* m_pModSpecs;
	bool m_bITBidiMode;	// Process bidi loops like Impulse Tracker (see Fastmix.cpp for an explanation)

public:	// Static Members
	static UINT m_nXBassDepth, m_nXBassRange;
	static UINT m_nReverbDepth, gnReverbType;
	static UINT m_nProLogicDepth, m_nProLogicDelay;
	static UINT m_nStereoSeparation;
	static UINT m_nMaxMixChannels;
	static LONG m_nStreamVolume;
	static DWORD gdwSysInfo, gdwSoundSetup, gdwMixingFreq, gnBitsPerSample, gnChannels;
	static UINT gnAGC;
	static UINT gnVolumeRampUpSamples, gnVolumeRampDownSamples;
	static UINT gnCPUUsage;
	static LPSNDMIXHOOKPROC gpSndMixHook;
	static PMIXPLUGINCREATEPROC gpMixPluginCreateProc;
	static uint8 s_DefaultPlugVolumeHandling;

	typedef uint32 samplecount_t;	// Number of rendered samples

public:	// for Editing
	CModDoc *m_pModDoc;		// Can be a null pointer for example when previewing samples from the treeview.
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
	bool m_bPatternTransitionOccurred;
	UINT m_nMasterVolume, m_nGlobalVolume, m_nSamplesToGlobalVolRampDest, m_nGlobalVolumeRampAmount,
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
	CHAR m_szNames[MAX_SAMPLES][MAX_SAMPLENAME];		// Song and sample names
	std::bitset<MAX_BASECHANNELS> m_bChannelMuteTogglePending;

	CSoundFilePlayConfig* m_pConfig;
	DWORD m_dwCreatedWithVersion;
	DWORD m_dwLastSavedWithVersion;

	vector<PatternCuePoint> m_PatternCuePoints;			// For WAV export (writing pattern positions to file)

	// For handling backwards jumps and stuff to prevent infinite loops when counting the mod length or rendering to wav.
	RowVisitor visitedSongRows;

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	CHAR m_szInstrumentPath[MAX_INSTRUMENTS][_MAX_PATH];
// -! NEW_FEATURE#0023

public:
	CSoundFile();
	~CSoundFile();

public:
	BOOL Create(LPCBYTE lpStream, CModDoc *pModDoc, DWORD dwMemLength=0);
	BOOL Destroy();
	MODTYPE GetType() const { return m_nType; }
	bool TypeIsIT_MPT() const { return (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) != 0; }
	bool TypeIsIT_MPT_XM() const { return (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)) != 0; }
	bool TypeIsS3M_IT_MPT() const { return (m_nType & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)) != 0; }
	bool TypeIsXM_MOD() const { return (m_nType & (MOD_TYPE_XM | MOD_TYPE_MOD)) != 0; }
	bool TypeIsMOD_S3M() const { return (m_nType & (MOD_TYPE_MOD | MOD_TYPE_S3M)) != 0; }

	// Get parent CModDoc. Can be nullptr if previewing from tree view, and is always nullptr if we're not actually compiling OpenMPT.
#ifdef MODPLUG_TRACKER
	CModDoc *GetpModDoc() const { return m_pModDoc; }
#else
	void *GetpModDoc() const { return nullptr; }
#endif // MODPLUG_TRACKER

	void SetMasterVolume(UINT vol, bool adjustAGC = false);
	UINT GetMasterVolume() const { return m_nMasterVolume; }

	INSTRUMENTINDEX GetNumInstruments() const { return m_nInstruments; }
	SAMPLEINDEX GetNumSamples() const { return m_nSamples; }
	UINT GetCurrentPos() const;
	PATTERNINDEX GetCurrentPattern() const { return m_nPattern; }
	ORDERINDEX GetCurrentOrder() const { return m_nCurrentOrder; }
	CHANNELINDEX GetNumChannels() const { return m_nChannels; }

	IMixPlugin* GetInstrumentPlugin(INSTRUMENTINDEX instr);
	const CModSpecifications& GetModSpecifications() const {return *m_pModSpecs;}
	static const CModSpecifications& GetModSpecifications(const MODTYPE type);

	double GetCurrentBPM() const;
	void DontLoopPattern(PATTERNINDEX nPat, ROWINDEX nRow = 0);		//rewbs.playSongFromCursor
	void SetCurrentPos(UINT nPos);
	void SetCurrentOrder(ORDERINDEX nOrder);
	LPCSTR GetTitle() const { return m_szNames[0]; }
	LPCTSTR GetSampleName(UINT nSample) const;
	CString GetInstrumentName(UINT nInstr) const;
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
	void CheckCPUUsage(UINT nCPU);

	void SetupITBidiMode();

	bool InitChannel(CHANNELINDEX nChn);

	// Module Loaders
	bool ReadXM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadS3M(FileReader &file);
	bool ReadMod(FileReader &file);
	bool ReadMed(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMTM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadSTM(FileReader &file);
	bool ReadIT(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadITProject(FileReader &file);
	bool Read669(FileReader &file);
	bool ReadUlt(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadWav(FileReader &file);
	bool ReadDSM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadFAR(FileReader &file);
	bool ReadAMS(FileReader &file);
	bool ReadAMS2(FileReader &file);
	bool ReadMDL(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadOKT(FileReader &file);
	bool ReadDMF(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadPTM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadDBM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadAMF(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMT2(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadPSM(FileReader &file);
	bool ReadPSM16(FileReader &file);
	bool ReadUMX(FileReader &file);
	bool ReadMO3(FileReader &file);
	bool ReadGDM(FileReader &file);
	bool ReadIMF(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadAM(FileReader &file);
	bool ReadJ2B(FileReader &file);
	bool ReadMID(const LPCBYTE lpStream, DWORD dwMemLength);

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
	void LoadExtendedSongProperties(const MODTYPE modtype, LPCBYTE ptr, const LPCBYTE startpos, const size_t seachlimit, bool* pInterpretMptMade = nullptr);
	size_t LoadModularInstrumentData(const LPCBYTE lpStream, const DWORD dwMemLength, ModInstrument *pIns) const;

	// Reads extended instrument properties(XM/IT/MPTM).
	// If no errors occur and song extension tag is found, returns pointer to the beginning
	// of the tag, else returns NULL.
	LPCBYTE LoadExtendedInstrumentProperties(const LPCBYTE pStart, const LPCBYTE pEnd, bool* pInterpretMptMade = nullptr);

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
	UINT ReadMix(LPVOID lpBuffer, UINT cbBuffer, CSoundFile *, DWORD *, LPBYTE ps=NULL);
	UINT CreateStereoMix(int count);
	UINT GetResamplingFlag(const ModChannel *pChannel);
	BOOL FadeSong(UINT msec);
	BOOL GlobalFadeSong(UINT msec);
	void ProcessPlugins(UINT nCount);
	size_t GetTotalSampleCount() const { return m_lTotalSampleCount; }
	bool HasPositionChanged() { bool b = m_bPositionChanged; m_bPositionChanged = false; return b; }

public:
	// Mixer Config
	static BOOL InitPlayer(BOOL bReset=FALSE);
	static BOOL SetWaveConfig(UINT nRate,UINT nBits,UINT nChannels,BOOL bMMX=FALSE);
	static BOOL SetDspEffects(BOOL bSurround,BOOL bReverb,BOOL xbass,BOOL dolbynr=FALSE,BOOL bEQ=FALSE);
	static BOOL SetResamplingMode(UINT nMode); // SRCMODE_XXXX
	static DWORD GetSampleRate() { return gdwMixingFreq; }
	static DWORD GetBitsPerSample() { return gnBitsPerSample; }
	static DWORD InitSysInfo();
	static DWORD GetSysInfo() { return gdwSysInfo; }
	static void EnableMMX(bool b) { if (b) gdwSoundSetup |= SNDMIX_ENABLEMMX; else gdwSoundSetup &= ~SNDMIX_ENABLEMMX; }
	// AGC
	static BOOL GetAGC() { return (gdwSoundSetup & SNDMIX_AGC) ? TRUE : FALSE; }
	static void SetAGC(BOOL b);
	static void ResetAGC();
	static void ProcessAGC(int count);
	// DSP Effects
	static void InitializeDSP(BOOL bReset);
	static void ProcessStereoDSP(int count);
	static void ProcessMonoDSP(int count);
	// [Reverb level 0(quiet)-100(loud)], [REVERBTYPE_XXXX]
	static BOOL SetReverbParameters(UINT nDepth, UINT nType);
	// [XBass level 0(quiet)-100(loud)], [cutoff in Hz 10-100]
	static BOOL SetXBassParameters(UINT nDepth, UINT nRange);
	// [Surround level 0(quiet)-100(heavy)] [delay in ms, usually 5-40ms]
	static BOOL SetSurroundParameters(UINT nDepth, UINT nDelay);
#ifdef ENABLE_EQ
	// EQ
	static void InitializeEQ(BOOL bReset=TRUE);
	static void SetEQGains(const UINT *pGains, UINT nBands, const UINT *pFreqs=NULL, BOOL bReset=FALSE);	// 0=-12dB, 32=+12dB
	/*static*/ void EQStereo(int *pbuffer, UINT nCount);
	/*static*/ void EQMono(int *pbuffer, UINT nCount);
#endif
	// Analyzer Functions
	static UINT WaveConvert(LPBYTE lpSrc, signed char *lpDest, UINT nSamples);
	static UINT WaveStereoConvert(LPBYTE lpSrc, signed char *lpDest, UINT nSamples);
	static LONG SpectrumAnalyzer(signed char *pBuffer, UINT nSamples, UINT nInc, UINT nChannels);
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
	void CheckNNA(CHANNELINDEX nChn, UINT instr, int note, BOOL bForceCut);
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
	void PanningSlide(ModChannel *pChn, UINT param);
	void ChannelVolSlide(ModChannel *pChn, UINT param);
	void FineVolumeUp(ModChannel *pChn, UINT param);
	void FineVolumeDown(ModChannel *pChn, UINT param);
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
		return (m_nMusicSpeed  + m_nFrameDelay) * max(m_nPatternDelay, 1);
	}

public:
	bool DestroySample(SAMPLEINDEX nSample);

// -> CODE#0020
// -> DESC="rearrange sample list"
	bool MoveSample(SAMPLEINDEX from, SAMPLEINDEX to);
// -! NEW_FEATURE#0020

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
	SAMPLEINDEX DetectUnusedSamples(vector<bool> &sampleUsed) const;
	SAMPLEINDEX RemoveSelectedSamples(const vector<bool> &keepSamples);
	static void AdjustSampleLoop(ModSample &sample);

	// Samples file I/O
	bool ReadSampleFromFile(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadWAVSample(SAMPLEINDEX nSample, FileReader &file, FileReader *wsmpChunk = nullptr);
	bool ReadPATSample(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadS3ISample(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadAIFFSample(SAMPLEINDEX nSample, FileReader &file);
	bool ReadXISample(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength);
	UINT ReadITSSample(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength, DWORD dwOffset=0);
	bool Read8SVXSample(SAMPLEINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool SaveWAVSample(SAMPLEINDEX nSample, const LPCSTR lpszFileName) const;
	bool SaveRAWSample(SAMPLEINDEX nSample, const LPCSTR lpszFileName) const;

	// Instrument file I/O
	bool ReadInstrumentFromFile(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadXIInstrument(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadITIInstrument(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadPATInstrument(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadSampleAsInstrument(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool SaveXIInstrument(INSTRUMENTINDEX nInstr, const LPCSTR lpszFileName) const;
	bool SaveITIInstrument(INSTRUMENTINDEX nInstr, const LPCSTR lpszFileName) const;

	// I/O from another sound file
	bool ReadInstrumentFromSong(INSTRUMENTINDEX targetInstr, const CSoundFile *pSrcSong, INSTRUMENTINDEX sourceInstr);
	bool ReadSampleFromSong(SAMPLEINDEX targetSample, const CSoundFile *pSrcSong, SAMPLEINDEX sourceSample);

	// Period/Note functions
	UINT GetNoteFromPeriod(UINT period) const;
	UINT GetPeriodFromNote(UINT note, int nFineTune, UINT nC5Speed) const;
	UINT GetFreqFromPeriod(UINT period, UINT nC5Speed, int nPeriodFrac=0) const;
	// Misc functions
	ModSample &GetSample(SAMPLEINDEX sample) { ASSERT(sample <= m_nSamples && sample < CountOf(Samples)); return Samples[sample]; }
	const ModSample &GetSample(SAMPLEINDEX sample) const { ASSERT(sample <= m_nSamples && sample < CountOf(Samples)); return Samples[sample]; }

	UINT MapMidiInstrument(DWORD dwProgram, UINT nChannel, UINT nNote);
	size_t ITInstrToMPT(const void *p, ModInstrument *pIns, UINT trkvers, size_t memLength);
	UINT LoadMixPlugins(const void *pData, UINT nLen);

	DWORD CutOffToFrequency(UINT nCutOff, int flt_modifier=256) const; // [0-127] => [1-10KHz]
#ifdef MODPLUG_TRACKER
	void ProcessMidiOut(CHANNELINDEX nChn);
#endif // MODPLUG_TRACKER
	void ApplyGlobalVolume(int SoundBuffer[], int RearBuffer[], long lTotalSampleCount);

	// System-Dependant functions
public:
	static LPSTR AllocateSample(UINT nbytes);
	static void FreeSample(void *p);

	ModInstrument *AllocateInstrument(INSTRUMENTINDEX instr, SAMPLEINDEX assignedSample = 0);

	// WAV export
	static UINT Normalize24BitBuffer(LPBYTE pbuffer, UINT cbsizebytes, DWORD lmax24, DWORD dwByteInc);

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
	CString GetSongMessage(const enmLineEndings lineEnding, void (*pTextConverter)(char &) = nullptr) const;

protected:
	// Read song message from a mapped file.
	// [in]  data: pointer to the data in memory that is going to be read
	// [in]  length: number of characters that should be read, not including a possible trailing null terminator (it is automatically appended).
	// [in]  lineEnding: line ending formatting of the text in memory.
	// [in]  pTextConverter: Pointer to a callback function which can be used to pre-process the read characters, if necessary (nullptr otherwise).
	// [out] returns true on success.
	bool ReadMessage(const BYTE *data, size_t length, enmLineEndings lineEnding, void (*pTextConverter)(char &) = nullptr);
	bool ReadMessage(FileReader &file, size_t length, enmLineEndings lineEnding, void (*pTextConverter)(char &) = nullptr)
	{
		size_t readLength = Util::Min(length, file.BytesLeft());
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
	bool ReadFixedLineLengthMessage(const BYTE *data, const size_t length, const size_t lineLength, const size_t lineEndingLength, void (*pTextConverter)(char &) = nullptr);
	bool ReadFixedLineLengthMessage(FileReader &file, const size_t length, const size_t lineLength, const size_t lineEndingLength, void (*pTextConverter)(char &) = nullptr)
	{
		size_t readLength = Util::Min(length, file.BytesLeft());
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

#define MIXBUFFERSIZE		512
#define SCRATCH_BUFFER_SIZE 64 //Used for plug's final processing (cleanup)
#define MIXING_ATTENUATION	4
#define VOLUMERAMPPRECISION	12
#define FADESONGDELAY		100
#define EQ_BUFFERSIZE		(MIXBUFFERSIZE)
#define AGC_PRECISION		10
#define AGC_UNITY			(1 << AGC_PRECISION)

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

int _muldiv(long a, long b, long c);
int _muldivr(long a, long b, long c);

///////////////////////////////////////////////////////////
// File Formats Information (name, extension, etc)

#ifndef FASTSOUNDLIB

typedef struct MODFORMATINFO
{
	MODTYPE mtFormatId;		// MOD_TYPE_XXXX
	LPCSTR  lpszFormatName;	// "ProTracker"
	LPCSTR  lpszExtension;	// ".xxx"
	DWORD   dwPadding;
} MODFORMATINFO;

extern MODFORMATINFO gModFormatInfo[];

#endif


// Used in instrument/song extension reading to make sure the size field is valid.
bool IsValidSizeField(const LPCBYTE pData, const LPCBYTE pEnd, const int16 size);

// Read instrument property with 'code' and 'size' from 'ptr' to instrument 'pIns'.
// Note: (ptr, size) pair must be valid (e.g. can read 'size' bytes from 'ptr')
void ReadInstrumentExtensionField(ModInstrument* pIns, LPCBYTE& ptr, const int32 code, const int16 size);

// Read instrument property with 'code' from 'pData' to instrument 'pIns'.
void ReadExtendedInstrumentProperty(ModInstrument* pIns, const int32 code, LPCBYTE& pData, const LPCBYTE pEnd);

// Read extended instrument properties from 'pDataStart' to instrument 'pIns'.
void ReadExtendedInstrumentProperties(ModInstrument* pIns, const LPCBYTE pDataStart, const size_t nMemLength);

// Convert instrument flags which were read from 'dF..' extension to proper internal representation.
void ConvertReadExtendedFlags(ModInstrument* pIns);