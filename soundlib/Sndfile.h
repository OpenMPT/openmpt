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

#include "Mixer.h"
#include "Resampler.h"
#ifndef NO_REVERB
#include "../sounddsp/Reverb.h"
#endif
#ifndef NO_AGC
#include "../sounddsp/AGC.h"
#endif
#ifndef NO_DSP
#include "../sounddsp/DSP.h"
#endif
#ifndef NO_EQ
#include "../sounddsp/EQ.h"
#endif

#include "ModSample.h"
#include "ModInstrument.h"
#include "ModChannel.h"
#include "modcommand.h"
#include "plugins/PlugInterface.h"
#include "RowVisitor.h"
#include "Message.h"


class FileReader;
// -----------------------------------------------------------------------------------------
// MODULAR ModInstrument FIELD ACCESS : body content at the (near) top of Sndfile.cpp !!!
// -----------------------------------------------------------------------------------------
extern void WriteInstrumentHeaderStructOrField(ModInstrument * input, FILE * file, uint32 only_this_code = -1 /* -1 for all */, int16 fixedsize = 0);
extern bool ReadInstrumentHeaderField(ModInstrument * input, uint32 fcode, uint16 fsize, FileReader &file);
// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------

// Sample decompression
void AMSUnpack(const int8 * const source, size_t sourceSize, void * const dest, const size_t destSize, char packCharacter);
uint16 MDLReadBits(uint32 &bitbuf, uint32 &bitnum, const uint8 *(&ibuf), int8 n);
uintptr_t DMFUnpack(LPBYTE psample, const uint8 *ibuf, const uint8 *ibufmax, uint32 maxlen);
bool IMAADPCMUnpack16(int16 *target, SmpLength sampleLen, FileReader file, uint16 blockAlign);

// Module decompression
bool UnpackXPK(std::vector<char> &unpackedData, FileReader &file);
bool UnpackPP20(std::vector<char> &unpackedData, FileReader &file);
bool UnpackMMCMP(std::vector<char> &unpackedData, FileReader &file);

////////////////////////////////////////////////////////////////////////
// Reverberation

#ifndef NO_REVERB

#define NUM_REVERBTYPES			29

const char *GetReverbPresetName(UINT nPreset);

#endif

typedef void (* LPSNDMIXHOOKPROC)(int *, unsigned long, unsigned long); // buffer, samples, channels

#include "pattern.h"
#include "patternContainer.h"
#include "ModSequence.h"


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


// Target seek mode for GetLength()
struct GetLengthTarget
{
	union
	{
		double time;
		struct
		{
			ROWINDEX row;
			ORDERINDEX order;
		} pos;
	};

	enum Mode
	{
		NoTarget,		// Don't seek, i.e. return complete module length.
		SeekPosition,	// Seek to given pattern position.
		SeekSeconds,	// Seek to given time.
	} mode;

	// Don't seek, i.e. return complete module length.
	GetLengthTarget()
	{
		mode = NoTarget;
	}

	// Seek to given pattern position if position is valid.
	GetLengthTarget(ORDERINDEX order, ROWINDEX row)
	{
		mode = NoTarget;
		if(order != ORDERINDEX_INVALID && row != ROWINDEX_INVALID)
		{
			mode = SeekPosition;
			pos.row = row;
			pos.order = order;
		}
	}

	// Seek to given time if t is valid (i.e. not negative).
	GetLengthTarget(double t)
	{
		mode = NoTarget;
		if(t >= 0.0)
		{
			mode = SeekSeconds;
			time = t;
		}
	}
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
};


//Note: These are bit indeces. MSF <-> Mod(Specific)Flag.
//If changing these, ChangeModTypeTo() might need modification.
enum ModSpecificFlag
{
	MSF_COMPATIBLE_PLAY		= 1,		//IT/MPT/XM
	MSF_OLDVOLSWING			= 2,		//IT/MPT
	MSF_MIDICC_BUGEMULATION	= 4,		//IT/MPT/XM
	MSF_OLD_MIDI_PITCHBENDS	= 8,		//IT/MPT/XM
	MSF_VOLRAMP				= 16,		//XM(FT2)
};
DECLARE_FLAGSET(ModSpecificFlag)


class CTuningCollection;
#ifdef MODPLUG_TRACKER
class CModDoc;
#endif // MODPLUG_TRACKER


/////////////////////////////////////////////////////////////////////////
// File edit history

#define HISTORY_TIMER_PRECISION	18.2f

//================
struct FileHistory
//================
{
	// Date when the file was loaded in the the tracker or created.
	tm loadDate;
	// Time the file was open in the editor, in 1/18.2th seconds (frequency of a standard DOS timer, to keep compatibility with Impulse Tracker easy).
	uint32 openTime;
	// Return the date as a (possibly truncated if not enough precision is available) ISO 8601 formatted date.
	std::string AsISO8601() const;
};


struct TimingInfo
{
	double InputLatency; // seconds
	double OutputLatency; // seconds
	int64 StreamFrames;
	uint64 SystemTimestamp; // nanoseconds
	double Speed;
	TimingInfo()
		: InputLatency(0.0)
		, OutputLatency(0.0)
		, StreamFrames(0)
		, SystemTimestamp(0)
		, Speed(1.0)
	{
		return;
	}
};


class IAudioReadTarget
{
public:
	virtual void DataCallback(int *MixSoundBuffer, std::size_t channels, std::size_t countChunk) = 0;
};


#if MPT_COMPILER_MSVC
#pragma warning(disable:4324) //structure was padded due to __declspec(align())
#endif


//==============
class CSoundFile
//==============
{

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
	bool SaveStaticTunings();
	static void DeleteStaticdata();
	static CTuningCollection& GetBuiltInTunings() {return *s_pTuningsSharedBuiltIn;}
	static CTuningCollection& GetLocalTunings() {return *s_pTuningsSharedLocal;}
#else
	void LoadBuiltInTunings();
	CTuningCollection& GetBuiltInTunings() {return *m_pTuningsBuiltIn;}
#endif
	static CTuning *GetDefaultTuning() {return nullptr;}
	CTuningCollection& GetTuneSpecificTunings() {return *m_pTuningsTuneSpecific;}

	std::string GetNoteName(const ModCommand::NOTE note, const INSTRUMENTINDEX inst) const;
	static std::string GetNoteName(const ModCommand::NOTE note);
private:
	CTuningCollection* m_pTuningsTuneSpecific;
#ifdef MODPLUG_TRACKER
	static CTuningCollection* s_pTuningsSharedBuiltIn;
	static CTuningCollection* s_pTuningsSharedLocal;
#else
	CTuningCollection* m_pTuningsBuiltIn;
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
	const CModSpecifications *m_pModSpecs;
	FlagSet<ModSpecificFlag, uint16> m_ModFlags;

private:
	// Interleaved Front Mix Buffer (Also room for interleaved rear mix)
	mixsample_t MixSoundBuffer[MIXBUFFERSIZE * 4];
	mixsample_t MixRearBuffer[MIXBUFFERSIZE * 2];
	// Non-interleaved plugin processing buffer
	float MixFloatBuffer[2][MIXBUFFERSIZE];
	mixsample_t gnDryLOfsVol;
	mixsample_t gnDryROfsVol;

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
	CQuadEQ m_EQ;
#endif
#ifndef NO_AGC
	CAGC m_AGC;
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
private:
	MODCONTAINERTYPE m_ContainerType;
public:
	CHANNELINDEX m_nChannels;
	SAMPLEINDEX m_nSamples;
	INSTRUMENTINDEX m_nInstruments;
	UINT m_nDefaultSpeed, m_nDefaultTempo, m_nDefaultGlobalVolume;
	FlagSet<SongFlags> m_SongFlags;
	CHANNELINDEX m_nMixChannels;
private:
	CHANNELINDEX m_nMixStat;
public:
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
	UINT m_nMusicSpeed, m_nMusicTempo;	// Current speed and tempo
	ROWINDEX m_nNextRow, m_nRow;
	ROWINDEX m_nNextPatStartRow; // for FT2's E60 bug
	PATTERNINDEX m_nPattern;
	ORDERINDEX m_nCurrentOrder, m_nNextOrder, m_nRestartPos, m_nSeqOverride;

#ifdef MODPLUG_TRACKER
	// Lock playback between two orders. Lock is active if lock start != ORDERINDEX_INVALID).
	ORDERINDEX m_lockOrderStart, m_lockOrderEnd;
#endif // MODPLUG_TRACKER

	UINT m_nGlobalVolume, m_nSamplesToGlobalVolRampDest, m_nGlobalVolumeRampAmount,
		 m_nGlobalVolumeDestination, m_nSamplePreAmp, m_nVSTiVolume;
	long m_lHighResRampingGlobalVolume;
	bool IsGlobalVolumeUnset() const { return IsFirstTick(); }
#ifndef MODPLUG_TRACKER
	UINT m_nFreqFactor; // Pitch shift factor (128 = no pitch shifting). Interesting ModPlug Player relict, but unused in OpenMPT.
	UINT m_nTempoFactor; // Tempo factor (128 = no tempo adjustment). Interesting ModPlug Player relict, but unused in OpenMPT.
#endif
	UINT m_nOldGlbVolSlide;
	LONG m_nMinPeriod, m_nMaxPeriod;	// min period = highest possible frequency, max period = lowest possible frequency
	LONG m_nRepeatCount;	// -1 means repeat infinitely.
	UINT m_nMaxOrderPosition;
	CHANNELINDEX ChnMix[MAX_CHANNELS];							// Channels to be mixed
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
#ifdef MODPLUG_TRACKER
	std::bitset<MAX_BASECHANNELS> m_bChannelMuteTogglePending;
#endif // MODPLUG_TRACKER

	uint32 m_dwCreatedWithVersion;
	uint32 m_dwLastSavedWithVersion;

#ifdef MODPLUG_TRACKER
	std::vector<PatternCuePoint> m_PatternCuePoints;	// For WAV export (writing pattern positions to file)
#endif // MODPLUG_TRACKER

protected:
	// Mix level stuff
	CSoundFilePlayConfig m_PlayConfig;
	mixLevels m_nMixLevels;

	// For handling backwards jumps and stuff to prevent infinite loops when counting the mod length or rendering to wav.
	RowVisitor visitedSongRows;

public:

	std::string songName;
	std::string songArtist;

	// Song message
	SongMessage songMessage;
	std::string madeWithTracker;

protected:
	std::vector<FileHistory> m_FileHistory;	// File edit history
public:
	std::vector<FileHistory> &GetFileHistory() { return m_FileHistory; }
	const std::vector<FileHistory> &GetFileHistory() const { return m_FileHistory; }

#ifdef MODPLUG_TRACKER
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	mpt::PathString m_szInstrumentPath[MAX_INSTRUMENTS];
// -! NEW_FEATURE#0023
#endif // MODPLUG_TRACKER

	bool m_bIsRendering;
	TimingInfo m_TimingInfo; // only valid if !m_bIsRendering
	bool m_bPatternTransitionOccurred;

private:
	// logging and user interaction
	ILog *m_pCustomLog;

public:
	CSoundFile();
	~CSoundFile();

public:
	// logging and user interaction
	void SetCustomLog(ILog *pLog) { m_pCustomLog = pLog; }
	void AddToLog(LogLevel level, const std::string &text) const;
	void AddToLog(const std::string &text) const { AddToLog(LogInformation, text); }

public:

	enum ModLoadingFlags
	{
		onlyVerifyHeader	= 0x00,
		loadPatternData		= 0x01,	// If unset, advise loaders to not process any pattern data (if possible)
		loadSampleData		= 0x02,	// If unset, advise loaders to not process any sample data (if possible)
		loadPluginData		= 0x04,	// If unset, plugins are not instanciated.
		// Shortcuts
		loadCompleteModule	= loadSampleData | loadPatternData | loadPluginData,
		loadNoPatternOrPluginData	= loadSampleData,
	};

#ifdef MODPLUG_TRACKER
	// Get parent CModDoc. Can be nullptr if previewing from tree view, and is always nullptr if we're not actually compiling OpenMPT.
	CModDoc *GetpModDoc() const { return m_pModDoc; }

	BOOL Create(FileReader file, ModLoadingFlags loadFlags = loadCompleteModule, CModDoc *pModDoc = nullptr);
#else
	BOOL Create(FileReader file, ModLoadingFlags loadFlags);
#endif // MODPLUG_TRACKER

	BOOL Destroy();
	MODTYPE GetType() const { return m_nType; }
	bool TypeIsIT_MPT() const { return (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) != 0; }
	bool TypeIsIT_MPT_XM() const { return (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)) != 0; }
	bool TypeIsS3M_IT_MPT() const { return (m_nType & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)) != 0; }

	MODCONTAINERTYPE GetContainerType() const { return m_ContainerType; }

	// rough heuristic, could be improved
	std::pair<MOD_CHARSET_CERTAINTY, mpt::Charset> GetCharset() const { return GetCharsetFromModType(GetType()); }

	void SetPreAmp(UINT vol);
	UINT GetPreAmp() const { return m_MixerSettings.m_nPreAmp; }

	void SetMixLevels(mixLevels levels);
	mixLevels GetMixLevels() const { return m_nMixLevels; }
	const CSoundFilePlayConfig &GetPlayConfig() const { return m_PlayConfig; }

	INSTRUMENTINDEX GetNumInstruments() const { return m_nInstruments; }
	SAMPLEINDEX GetNumSamples() const { return m_nSamples; }
	UINT GetCurrentPos() const;
	PATTERNINDEX GetCurrentPattern() const { return m_nPattern; }
	ORDERINDEX GetCurrentOrder() const { return m_nCurrentOrder; }
	CHANNELINDEX GetNumChannels() const { return m_nChannels; }

	IMixPlugin* GetInstrumentPlugin(INSTRUMENTINDEX instr);
	const CModSpecifications& GetModSpecifications() const {return *m_pModSpecs;}
	static const CModSpecifications& GetModSpecifications(const MODTYPE type);

#ifdef MODPLUG_TRACKER
	void PatternTranstionChnSolo(const CHANNELINDEX chnIndex);
	void PatternTransitionChnUnmuteAll();
#endif // MODPLUG_TRACKER

	double GetCurrentBPM() const;
	void DontLoopPattern(PATTERNINDEX nPat, ROWINDEX nRow = 0);		//rewbs.playSongFromCursor
	CHANNELINDEX GetMixStat() const { return m_nMixStat; }
	void ResetMixStat() { m_nMixStat = 0; }
	void SetCurrentPos(UINT nPos);
	void SetCurrentOrder(ORDERINDEX nOrder);
	std::string GetTitle() const { return songName; }
	bool SetTitle(const std::string &newTitle); // Return true if title was changed.
	const char *GetSampleName(UINT nSample) const;
	const char *GetInstrumentName(INSTRUMENTINDEX nInstr) const;
	UINT GetMusicSpeed() const { return m_nMusicSpeed; }
	UINT GetMusicTempo() const { return m_nMusicTempo; }
	bool IsFirstTick() const { return (m_lTotalSampleCount == 0); }

	//Get modlength in various cases: total length, length to
	//specific order&row etc. Return value is in seconds.
	GetLengthType GetLength(enmGetLengthResetMode adjustMode, GetLengthTarget target = GetLengthTarget());

	void InitializeVisitedRows() { visitedSongRows.Initialize(true); }

public:
	//Returns song length in seconds.
	double GetSongTime() { return GetLength(eNoAdjust).duration; }

	void RecalculateSamplesPerTick();
	double GetRowDuration(UINT tempo, UINT speed) const;
	UINT GetTickDuration(UINT tempo, UINT speed, ROWINDEX rowsPerBeat);

	// A repeat count value of -1 means infinite loop
	void SetRepeatCount(int n) { m_nRepeatCount = n; }
	int GetRepeatCount() const { return m_nRepeatCount; }
	bool IsPaused() const {	return m_SongFlags[SONG_PAUSED | SONG_STEP]; }	// Added SONG_STEP as it seems to be desirable in most cases to check for this as well.
	void LoopPattern(PATTERNINDEX nPat, ROWINDEX nRow = 0);

	bool InitChannel(CHANNELINDEX nChn);

	// Global variable initializer for loader functions
	void InitializeGlobals();
	void InitializeChannels();

	// Module Loaders
	bool ReadXM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadS3M(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMod(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadM15(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMed(const LPCBYTE lpStream, const DWORD dwMemLength, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMTM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadSTM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadIT(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadITProject(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool Read669(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadUlt(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadWav(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadDSM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadFAR(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadAMS(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadAMS2(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMDL(const LPCBYTE lpStream, const DWORD dwMemLength, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadOKT(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadDMF(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadPTM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadDBM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadAMF_Asylum(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadAMF_DSMI(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMT2(const LPCBYTE lpStream, const DWORD dwMemLength, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadPSM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadPSM16(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadUMX(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMO3(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadGDM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadIMF(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadAM(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadJ2B(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadDIGI(FileReader &file, ModLoadingFlags loadFlags = loadCompleteModule);
	bool ReadMID(const LPCBYTE lpStream, DWORD dwMemLength, ModLoadingFlags loadFlags = loadCompleteModule);

	static std::vector<const char *> GetSupportedExtensions(bool otherFormats);
	static std::pair<MOD_CHARSET_CERTAINTY, mpt::Charset> GetCharsetFromModType(MODTYPE modtype);
	static const char * ModTypeToString(MODTYPE modtype);
	static std::string ModContainerTypeToString(MODCONTAINERTYPE containertype);
	static std::string ModTypeToTracker(MODTYPE modtype);
	static std::string ModContainerTypeToTracker(MODCONTAINERTYPE containertype);

	void UpgradeModFlags();
	void UpgradeSong();

	// Save Functions
#ifndef MODPLUG_NO_FILESAVE
	bool SaveXM(const mpt::PathString &filename, bool compatibilityExport = false);
	bool SaveS3M(const mpt::PathString &filename) const;
	bool SaveMod(const mpt::PathString &filename) const;
	bool SaveIT(const mpt::PathString &filename, bool compatibilityExport = false);
	bool SaveITProject(const mpt::PathString &filename); // -> CODE#0023 -> DESC="IT project files (.itp)" -! NEW_FEATURE#0023
	UINT SaveMixPlugins(FILE *f=NULL, BOOL bUpdate=TRUE);
	void WriteInstrumentPropertyForAllInstruments(uint32 code,  int16 size, FILE* f, UINT nInstruments) const;
	void SaveExtendedInstrumentProperties(UINT nInstruments, FILE* f) const;
	void SaveExtendedSongProperties(FILE* f) const;
	size_t SaveModularInstrumentData(FILE *f, const ModInstrument *pIns) const;
#endif // MODPLUG_NO_FILESAVE
	void LoadExtendedSongProperties(const MODTYPE modtype, FileReader &file, bool* pInterpretMptMade = nullptr);
	static size_t LoadModularInstrumentData(FileReader &file, ModInstrument &ins);

	std::string GetSchismTrackerVersion(uint16 cwtv);

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
	void ReadMODPatternEntry(FileReader &file, ModCommand &m) const;

	void SetupMODPanning(bool bForceSetup = false); // Setup LRRL panning, max channel volume

public:
	// Real-time sound functions
	void SuspendPlugins(); //rewbs.VSTCompliance
	void ResumePlugins();  //rewbs.VSTCompliance
	void StopAllVsti();    //rewbs.VSTCompliance
	void RecalculateGainForAllPlugs();
	void ResetChannels();
	samplecount_t Read(samplecount_t count, IAudioReadTarget &target);
private:
	void CreateStereoMix(int count);
public:
	BOOL FadeSong(UINT msec);
private:
	void ProcessDSP(std::size_t countChunk);
	void ProcessPlugins(UINT nCount);
public:
	samplecount_t GetTotalSampleCount() const { return m_lTotalSampleCount; }
	bool HasPositionChanged() { bool b = m_bPositionChanged; m_bPositionChanged = false; return b; }
	bool IsRenderingToDisc() const { return m_bIsRendering; }

	void PrecomputeSampleLoops(bool updateChannels = false);

public:
	// Mixer Config
	void SetMixerSettings(const MixerSettings &mixersettings);
	void SetResamplerSettings(const CResamplerSettings &resamplersettings);
	void InitPlayer(BOOL bReset=FALSE);
	void SetDspEffects(DWORD DSPMask);
	DWORD GetSampleRate() { return m_MixerSettings.gdwMixingFreq; }
#ifndef NO_EQ
	void SetEQGains(const UINT *pGains, UINT nBands, const UINT *pFreqs=NULL, BOOL bReset=FALSE)	{ m_EQ.SetEQGains(pGains, nBands, pFreqs, bReset, m_MixerSettings.gdwMixingFreq); } // 0=-12dB, 32=+12dB
#endif // NO_EQ
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

	void ProcessArpeggio(CHANNELINDEX nChn, int &period, CTuning::NOTEINDEXTYPE &arpeggioSteps);
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
	void NoteSlide(ModChannel *pChn, UINT param, bool slideUp, bool retrig);
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
	void NoteCut(CHANNELINDEX nChn, UINT nTick, bool cutSample);
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

	ModInstrument *AllocateInstrument(INSTRUMENTINDEX instr, SAMPLEINDEX assignedSample = 0);
	bool DestroyInstrument(INSTRUMENTINDEX nInstr, deleteInstrumentSamples removeSamples);
	bool IsSampleUsed(SAMPLEINDEX nSample) const;
	bool IsInstrumentUsed(INSTRUMENTINDEX nInstr) const;
	bool RemoveInstrumentSamples(INSTRUMENTINDEX nInstr);
	SAMPLEINDEX DetectUnusedSamples(std::vector<bool> &sampleUsed) const;
	SAMPLEINDEX RemoveSelectedSamples(const std::vector<bool> &keepSamples);

	// Samples file I/O
	bool ReadSampleFromFile(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize=false);
	bool ReadWAVSample(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize=false, FileReader *wsmpChunk = nullptr);
	bool ReadPATSample(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadS3ISample(SAMPLEINDEX nSample, FileReader &file);
	bool ReadAIFFSample(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize=false);
	bool ReadXISample(SAMPLEINDEX nSample, FileReader &file);
	bool ReadITSSample(SAMPLEINDEX nSample, FileReader &file, bool rewind = true);
	bool ReadITISample(SAMPLEINDEX nSample, FileReader &file);
	bool ReadIFFSample(SAMPLEINDEX nInstr, FileReader &file);
	bool ReadFLACSample(SAMPLEINDEX sample, FileReader &file);
	bool ReadMP3Sample(SAMPLEINDEX sample, FileReader &file);
#ifndef MODPLUG_NO_FILESAVE
	bool SaveWAVSample(SAMPLEINDEX nSample, const mpt::PathString &filename) const;
	bool SaveRAWSample(SAMPLEINDEX nSample, const mpt::PathString &filename) const;
	bool SaveFLACSample(SAMPLEINDEX nSample, const mpt::PathString &filename) const;
#endif

	// Instrument file I/O
	bool ReadInstrumentFromFile(INSTRUMENTINDEX nInstr, FileReader &file, bool mayNormalize=false);
	bool ReadXIInstrument(INSTRUMENTINDEX nInstr, FileReader &file);
	bool ReadITIInstrument(INSTRUMENTINDEX nInstr, FileReader &file);
	bool ReadPATInstrument(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadSampleAsInstrument(INSTRUMENTINDEX nInstr, FileReader &file, bool mayNormalize=false);
#ifndef MODPLUG_NO_FILESAVE
	bool SaveXIInstrument(INSTRUMENTINDEX nInstr, const mpt::PathString &filename) const;
	bool SaveITIInstrument(INSTRUMENTINDEX nInstr, const mpt::PathString &filename, bool compress) const;
#endif

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
	void ApplyGlobalVolume(int *SoundBuffer, int *RearBuffer, long countChunk);

private:
	PLUGINDEX GetChannelPlugin(CHANNELINDEX nChn, PluginMutePriority respectMutes) const;
	PLUGINDEX GetActiveInstrumentPlugin(CHANNELINDEX, PluginMutePriority respectMutes) const;
	IMixPlugin * GetChannelInstrumentPlugin(CHANNELINDEX chn) const;

	void HandlePatternTransitionEvents();

public:
	PLUGINDEX GetBestPlugin(CHANNELINDEX nChn, PluginPriority priority, PluginMutePriority respectMutes) const;
	uint8 GetBestMidiChannel(CHANNELINDEX nChn) const;

};

#if MPT_COMPILER_MSVC
#pragma warning(default : 4324) //structure was padded due to __declspec(align())
#endif


extern const char szNoteNames[12][4];


inline IMixPlugin* CSoundFile::GetInstrumentPlugin(INSTRUMENTINDEX instr)
//-----------------------------------------------------------------------
{
	if(instr > 0 && instr < MAX_INSTRUMENTS && Instruments[instr] && Instruments[instr]->nMixPlug && Instruments[instr]->nMixPlug <= MAX_MIXPLUGINS)
		return m_MixPlugins[Instruments[instr]->nMixPlug - 1].pMixPlugin;
	else
		return nullptr;
}


///////////////////////////////////////////////////////////
// Low-level Mixing functions

#define SCRATCH_BUFFER_SIZE 64 //Used for plug's final processing (cleanup)
#define FADESONGDELAY		100

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
