/*
 * OpenMPT
 *
 * Sndfile.h
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *          OpenMPT devs
*/

#ifndef __SNDFILE_H
#define __SNDFILE_H

#include "../mptrack/SoundFilePlayConfig.h"
#include "../mptrack/misc_util.h"
#include "mod_specifications.h"
#include <vector>
#include <bitset>
#include "midi.h"
#include "Snd_defs.h"

class CTuningBase;
typedef CTuningBase CTuning;


// Sample Struct
struct MODSAMPLE
{
	UINT nLength,nLoopStart,nLoopEnd;
		//nLength <-> Number of 'frames'?
	UINT nSustainStart, nSustainEnd;
	LPSTR pSample;
	UINT nC5Speed;
	WORD nPan;
	WORD nVolume;
	WORD nGlobalVol;
	WORD uFlags;
	signed char RelativeTone;
	signed char nFineTune;
	BYTE nVibType;
	BYTE nVibSweep;
	BYTE nVibDepth;
	BYTE nVibRate;
	CHAR name[32];
	CHAR filename[22];

	// Return the size of one (elementary) sample in bytes.
	uint8 GetElementarySampleSize() const {return (uFlags & CHN_16BIT) ? 2 : 1;}

	// Return the number of channels in the sample.
	uint8 GetNumChannels() const {return (uFlags & CHN_STEREO) ? 2 : 1;}

	// Return the size which pSample is at least.
	DWORD GetSampleSizeInBytes() const {return nLength * GetNumChannels() * GetElementarySampleSize();}

	// Returns sample rate of the sample. The argument is needed because 
	// the sample rate is obtained differently for different module types.
	uint32 GetSampleRate(const MODTYPE type) const;
};


// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

/*---------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------
MODULAR STRUCT DECLARATIONS :
-----------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------*/

// Instrument Envelopes
struct INSTRUMENTENVELOPE
{
	WORD Ticks[MAX_ENVPOINTS];
	BYTE Values[MAX_ENVPOINTS];
	UINT nNodes;
	BYTE nLoopStart;
	BYTE nLoopEnd;
	BYTE nSustainStart;
	BYTE nSustainEnd;
	BYTE nReleaseNode;
};

// Instrument Struct
struct MODINSTRUMENT
{
	UINT nFadeOut;
	DWORD dwFlags;
	UINT nGlobalVol;
	UINT nPan;

	INSTRUMENTENVELOPE VolEnv;
	INSTRUMENTENVELOPE PanEnv;
	INSTRUMENTENVELOPE PitchEnv;

	BYTE NoteMap[128];
	WORD Keyboard[128];

	BYTE nNNA;
	BYTE nDCT;
	BYTE nDNA;
	BYTE nPanSwing;
	BYTE nVolSwing;
	BYTE nIFC;
	BYTE nIFR;

	WORD wMidiBank;
	BYTE nMidiProgram;
	BYTE nMidiChannel;
	BYTE nMidiDrumKey;
	signed char nPPS; //Pitch to Pan Separator?
	unsigned char nPPC; //Pitch Centre?

	CHAR name[32];
	CHAR filename[12];

	BYTE nMixPlug;							//rewbs.instroVSTi
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
	USHORT nVolRamp;
// -! NEW_FEATURE#0027
	UINT nResampling;
	BYTE nCutSwing;
	BYTE nResSwing;
	BYTE nFilterMode;
	WORD wPitchToTempoLock;
	BYTE nPluginVelocityHandling;
	BYTE nPluginVolumeHandling;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// WHEN adding new members here, ALSO update Sndfile.cpp (instructions near the top of this file)!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	CTuning* pTuning;
	static CTuning* s_DefaultTuning;

	void SetTuning(CTuning* pT)
	{
		pTuning = pT;
	}

	

};

//MODINSTRUMENT;

// -----------------------------------------------------------------------------------------
// MODULAR MODINSTRUMENT FIELD ACCESS : body content at the (near) top of Sndfile.cpp !!!
// -----------------------------------------------------------------------------------------
extern void WriteInstrumentHeaderStruct(MODINSTRUMENT * input, FILE * file);
extern BYTE * GetInstrumentHeaderFieldPointer(MODINSTRUMENT * input, __int32 fcode, __int16 fsize);

// -! NEW_FEATURE#0027

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------

#pragma warning(disable : 4324) //structure was padded due to __declspec(align())

// Channel Struct
typedef struct __declspec(align(32)) _MODCHANNEL
{
	// First 32-bytes: Most used mixing information: don't change it
	LPSTR pCurrentSample;		
	DWORD nPos;
	DWORD nPosLo;	// actually 16-bit
	LONG nInc;		// 16.16
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
	LONG nFilter_Y1, nFilter_Y2, nFilter_Y3, nFilter_Y4;
	LONG nFilter_A0, nFilter_B0, nFilter_B1, nFilter_HP;
	LONG nROfs, nLOfs;
	LONG nRampLength;
	// Information not used in the mixer
	LPSTR pSample;
	LONG nNewRightVol, nNewLeftVol;
	LONG nRealVolume, nRealPan;
	LONG nVolume, nPan, nFadeOutVol;
	LONG nPeriod, nC5Speed, nPortamentoDest;
	MODINSTRUMENT *pModInstrument;
	MODSAMPLE *pModSample;
	DWORD nVolEnvPosition, nPanEnvPosition, nPitchEnvPosition;
	LONG nVolEnvValueAtReleaseJump, nPanEnvValueAtReleaseJump, nPitchEnvValueAtReleaseJump;
	DWORD nMasterChn, nVUMeter;
	LONG nGlobalVol, nInsVol;
	LONG nFineTune, nTranspose;
	LONG nPortamentoSlide, nAutoVibDepth;
	UINT nAutoVibPos, nVibratoPos, nTremoloPos, nPanbrelloPos;
	LONG nVolSwing, nPanSwing;
	LONG nCutSwing, nResSwing;
	LONG nRestorePanOnNewNote; //If > 0, nPan should be set to nRestorePanOnNewNote - 1 on new note. Used to recover from panswing.
	UINT nOldGlobalVolSlide;
	// 8-bit members
	BYTE nRestoreResonanceOnNewNote; //Like above
	BYTE nRestoreCutoffOnNewNote; //Like above
	BYTE nNote, nNNA;
	BYTE nNewNote, nNewIns, nCommand, nArpeggio;
	BYTE nOldVolumeSlide, nOldFineVolUpDown;
	BYTE nOldPortaUpDown, nOldFinePortaUpDown;
	BYTE nOldPanSlide, nOldChnVolSlide;
    UINT nNoteSlideCounter, nNoteSlideSpeed, nNoteSlideStep;
	BYTE nVibratoType, nVibratoSpeed, nVibratoDepth;
	BYTE nTremoloType, nTremoloSpeed, nTremoloDepth;
	BYTE nPanbrelloType, nPanbrelloSpeed, nPanbrelloDepth;
	BYTE nOldCmdEx, nOldVolParam, nOldTempo;
	BYTE nOldOffset, nOldHiOffset;
	BYTE nCutOff, nResonance;
	BYTE nRetrigCount, nRetrigParam;
	BYTE nTremorCount, nTremorParam;
	BYTE nPatternLoop, nPatternLoopCount;
	BYTE nRowNote, nRowInstr;
	BYTE nRowVolCmd, nRowVolume;
	BYTE nRowCommand, nRowParam;
	BYTE nLeftVU, nRightVU;
	BYTE nActiveMacro, nFilterMode;

	uint16 m_RowPlugParam;			//NOTE_PCs memory.
	float m_nPlugParamValueStep;  //rewbs.smoothVST 
	float m_nPlugInitialParamValue; //rewbs.smoothVST
	PLUGINDEX m_RowPlug;			//NOTE_PCs memory.
	
	void ClearRowCmd() {nRowNote = NOTE_NONE; nRowInstr = 0; nRowVolCmd = VOLCMD_NONE; nRowVolume = 0; nRowCommand = CMD_NONE; nRowParam = 0;}

	typedef UINT VOLUME;
	VOLUME GetVSTVolume() {return (pModInstrument) ? pModInstrument->nGlobalVol*4 : nVolume;}

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
} MODCHANNEL;

#define CHNRESET_CHNSETTINGS	1 //  1 b 
#define CHNRESET_SETPOS_BASIC	2 // 10 b
#define	CHNRESET_SETPOS_FULL	7 //111 b
#define CHNRESET_TOTAL			255 //11111111b


struct MODCHANNELSETTINGS
{
	UINT nPan;			// 0...256
	UINT nVolume;		// 0...64
	DWORD dwFlags;		// Suround/Mute
	UINT nMixPlugin;
	bool bIsSticky;		// Channel will always be visible in the pattern editor
	CHAR szName[MAX_CHANNELNAME];
};

#include "modcommand.h"

////////////////////////////////////////////////////////////////////
// Mix Plugins
#define MIXPLUG_MIXREADY			0x01	// Set when cleared

typedef long PlugParamIndex;
typedef float PlugParamValue;

class IMixPlugin
{
public:
	virtual int AddRef() = 0;
	virtual int Release() = 0;
	virtual void SaveAllParameters() = 0;
	virtual void RestoreAllParameters(long nProg=-1) = 0; //rewbs.plugDefaultProgram: added param
	virtual void Process(float *pOutL, float *pOutR, unsigned long nSamples) = 0;
	virtual void Init(unsigned long nFreq, int bReset) = 0;
	virtual bool MidiSend(DWORD dwMidiCode) = 0;
	virtual void MidiCC(UINT nMidiCh, UINT nController, UINT nParam, UINT trackChannel) = 0;
	virtual void MidiPitchBend(UINT nMidiCh, int nParam, UINT trackChannel) = 0;
	virtual void MidiCommand(UINT nMidiCh, UINT nMidiProg, WORD wMidiBank, UINT note, UINT vol, UINT trackChan) = 0;
	virtual void HardAllNotesOff() = 0;		//rewbs.VSTCompliance
	virtual void RecalculateGain() = 0;		
	virtual bool isPlaying(UINT note, UINT midiChn, UINT trackerChn) = 0; //rewbs.VSTiNNA
	virtual bool MoveNote(UINT note, UINT midiChn, UINT sourceTrackerChn, UINT destTrackerChn) = 0; //rewbs.VSTiNNA
	virtual void SetParameter(PlugParamIndex paramindex, PlugParamValue paramvalue) = 0;
	virtual void SetZxxParameter(UINT nParam, UINT nValue) = 0;
	virtual PlugParamValue GetParameter(PlugParamIndex nIndex) = 0;
	virtual UINT GetZxxParameter(UINT nParam) = 0; //rewbs.smoothVST 
	virtual void ModifyParameter(PlugParamIndex nIndex, PlugParamValue diff);
	virtual long Dispatch(long opCode, long index, long value, void *ptr, float opt) =0; //rewbs.VSTCompliance
	virtual void NotifySongPlaying(bool)=0;	//rewbs.VSTCompliance
	virtual bool IsSongPlaying()=0;
	virtual bool IsResumed()=0;
	virtual void Resume()=0;
	virtual void Suspend()=0;
	virtual BOOL isInstrument()=0;
	virtual BOOL CanRecieveMidiEvents()=0;
	virtual void SetDryRatio(UINT param)=0;

};

inline void IMixPlugin::ModifyParameter(PlugParamIndex nIndex, PlugParamValue diff)
//---------------------------------------------------------------------------------
{
	float val = GetParameter(nIndex) + diff;
	Limit(val, PlugParamValue(0), PlugParamValue(1));
	SetParameter(nIndex, val);
}


												///////////////////////////////////////////////////
												// !!! bits 8 -> 15 reserved for mixing mode !!! //
												///////////////////////////////////////////////////
#define MIXPLUG_INPUTF_MASTEREFFECT				0x01	// Apply to master mix
#define MIXPLUG_INPUTF_BYPASS					0x02	// Bypass effect
#define MIXPLUG_INPUTF_WETMIX					0x04	// Wet Mix (dry added)
// -> CODE#0028
// -> DESC="effect plugin mixing mode combo"
#define MIXPLUG_INPUTF_MIXEXPAND				0x08	// [0%,100%] -> [-200%,200%]
// -! BEHAVIOUR_CHANGE#0028


struct SNDMIXPLUGINSTATE
{
	DWORD dwFlags;					// MIXPLUG_XXXX
	LONG nVolDecayL, nVolDecayR;	// Buffer click removal
	int *pMixBuffer;				// Stereo effect send buffer
	float *pOutBufferL;				// Temp storage for int -> float conversion
	float *pOutBufferR;
};
typedef SNDMIXPLUGINSTATE* PSNDMIXPLUGINSTATE;

struct SNDMIXPLUGININFO
{
	DWORD dwPluginId1;
	DWORD dwPluginId2;
	DWORD dwInputRouting;	// MIXPLUG_INPUTF_XXXX
	DWORD dwOutputRouting;	// 0=mix 0x80+=fx
	DWORD dwReserved[4];	// Reserved for routing info
	CHAR szName[32];
	CHAR szLibraryName[64];	// original DLL name
}; // Size should be 128 							
typedef SNDMIXPLUGININFO* PSNDMIXPLUGININFO;

struct SNDMIXPLUGIN
{
	const char* GetName() const {return Info.szName;}
	const char* GetLibraryName();
	CString GetParamName(const UINT index) const;

	IMixPlugin *pMixPlugin;
	PSNDMIXPLUGINSTATE pMixState;
	ULONG nPluginDataSize;
	PVOID pPluginData;
	SNDMIXPLUGININFO Info;
	float fDryRatio;		    // rewbs.dryRatio [20040123]
	long defaultProgram;		// rewbs.plugDefaultProgram
}; // rewbs.dryRatio: Hopefully this doesn't need to be a fixed size.
typedef SNDMIXPLUGIN* PSNDMIXPLUGIN;

//class CSoundFile;
class CModDoc;
typedef	BOOL (__cdecl *PMIXPLUGINCREATEPROC)(PSNDMIXPLUGIN, CSoundFile*);

struct SNDMIXSONGEQ
{
	ULONG nEQBands;
	ULONG EQFreq_Gains[MAX_EQ_BANDS];
};
typedef SNDMIXSONGEQ* PSNDMIXSONGEQ;

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

////////////////////////////////////////////////////////////////////

enum {
	MIDIOUT_START=0,
	MIDIOUT_STOP,
	MIDIOUT_TICK,
	MIDIOUT_NOTEON,
	MIDIOUT_NOTEOFF,
	MIDIOUT_VOLUME,
	MIDIOUT_PAN,
	MIDIOUT_BANKSEL,
	MIDIOUT_PROGRAM,
};


struct MODMIDICFG
{
	CHAR szMidiGlb[9*32];
	CHAR szMidiSFXExt[16*32];
	CHAR szMidiZXXExt[128*32];
};
typedef MODMIDICFG* LPMODMIDICFG;

typedef VOID (__cdecl * LPSNDMIXHOOKPROC)(int *, unsigned long, unsigned long); // buffer, samples, channels

#include "pattern.h"
#include "patternContainer.h"
#include "ModSequence.h"
#include "PlaybackEventer.h"



class CSoundFile;

//======================
class CPatternSizesMimic
//======================
{
public:
	const ROWINDEX operator[](const int i) const;
	CPatternSizesMimic(const CSoundFile& csf) : m_rSndFile(csf) {}
private:
	const CSoundFile& m_rSndFile;
};

//Note: These are bit indeces. MSF <-> Mod(Specific)Flag.
//If changing these, ChangeModTypeTo() might need modification.
const BYTE MSF_COMPATIBLE_PLAY		= 0;		//IT/MPT/XM
const BYTE MSF_OLDVOLSWING			= 1;		//IT/MPT
const BYTE MSF_MIDICC_BUGEMULATION	= 2;		//IT/MPT/XM


class CTuningCollection;

//==============
class CSoundFile
//==============
{
public:
	//Return true if title was changed.
	bool SetTitle(const char*, size_t strSize);

public: //Misc
	void ChangeModTypeTo(const MODTYPE& newType);
	
	//Returns value in seconds. If given position won't be played at all, returns -1.
	double GetPlaybackTimeAt(ORDERINDEX, ROWINDEX);

	uint16 GetModFlags() const {return m_ModFlags;}
	void SetModFlags(const uint16 v) {m_ModFlags = v;}
	bool GetModFlag(BYTE i) const {return ((m_ModFlags & (1<<i)) != 0);}
	void SetModFlag(BYTE i, bool val) {if(i < 8*sizeof(m_ModFlags)) {m_ModFlags = (val) ? m_ModFlags |= (1 << i) : m_ModFlags &= ~(1 << i);}}

	// Is compatible mode for a specific tracker turned on?
	// Hint 1: No need to poll for MOD_TYPE_MPT, as it will automatically be linked with MOD_TYPE_IT
	// Hint 2:  Always returns true for MOD / S3M format (if that is the format of the current file)
	bool IsCompatibleMode(MODTYPE type) {
		if(GetType() & type & (MOD_TYPE_MOD | MOD_TYPE_S3M))
			return true; // those formats don't have flags so we will always return true
		return ((GetType() & ((type & MOD_TYPE_IT) ? type | MOD_TYPE_MPT : type)) && GetModFlag(MSF_COMPATIBLE_PLAY)) ? true : false;
	}
	
	//Tuning-->
public:
	static bool LoadStaticTunings();
	static bool SaveStaticTunings();
	static void DeleteStaticdata();
	static CTuningCollection& GetBuiltInTunings() {return *s_pTuningsSharedBuiltIn;}
	static CTuningCollection& GetLocalTunings() {return *s_pTuningsSharedLocal;}
	CTuningCollection& GetTuneSpecificTunings() {return *m_pTuningsTuneSpecific;}

	std::string GetNoteName(const int16&, const int inst = -1) const;
private:
	CTuningCollection* m_pTuningsTuneSpecific;
	static CTuningCollection* s_pTuningsSharedBuiltIn;
	static CTuningCollection* s_pTuningsSharedLocal;
	//<--Tuning

public: //Get 'controllers'
	CPlaybackEventer& GetPlaybackEventer() {return m_PlaybackEventer;}
	const CPlaybackEventer& GetPlaybackEventer() const {return m_PlaybackEventer;}

	CMIDIMapper& GetMIDIMapper() {return m_MIDIMapper;}
	const CMIDIMapper& GetMIDIMapper() const {return m_MIDIMapper;}

private: //Effect functions
	void PortamentoMPT(MODCHANNEL*, int);
	void PortamentoFineMPT(MODCHANNEL*, int);

private: //Misc private methods.
	static void SetModSpecsPointer(const CModSpecifications*& pModSpecs, const MODTYPE type);
	uint16 GetModFlagMask(const MODTYPE oldtype, const MODTYPE newtype) const;

private: //'Controllers'
	CPlaybackEventer m_PlaybackEventer;
	CMIDIMapper m_MIDIMapper;

private: //Misc data
	uint16 m_ModFlags;
	const CModSpecifications* m_pModSpecs;



public:	// Static Members
	static UINT m_nXBassDepth, m_nXBassRange;
	static float m_nMaxSample;
	static UINT m_nReverbDepth, gnReverbType;
	static UINT m_nProLogicDepth, m_nProLogicDelay;
	static UINT m_nStereoSeparation;
	static UINT m_nMaxMixChannels;
	static LONG m_nStreamVolume;
	static DWORD gdwSysInfo, gdwSoundSetup, gdwMixingFreq, gnBitsPerSample, gnChannels;
	static UINT gnAGC, gnVolumeRampSamples, gnCPUUsage;
	static LPSNDMIXHOOKPROC gpSndMixHook;
	static PMIXPLUGINCREATEPROC gpMixPluginCreateProc;
	static uint8 s_DefaultPlugVolumeHandling;



public:	// for Editing
	CModDoc* m_pModDoc;
	UINT m_nType;
	CHANNELINDEX m_nChannels;
	SAMPLEINDEX m_nSamples;
	INSTRUMENTINDEX m_nInstruments;
	UINT m_nDefaultSpeed, m_nDefaultTempo, m_nDefaultGlobalVolume;
	DWORD m_dwSongFlags;							// Song flags SONG_XXXX
	bool m_bIsRendering;
	UINT m_nMixChannels, m_nMixStat, m_nBufferCount;
	double m_dBufferDiff;
	UINT m_nTickCount, m_nTotalCount, m_nPatternDelay, m_nFrameDelay;
	ULONG m_lTotalSampleCount;	// rewbs.VSTTimeInfo
	UINT m_nSamplesPerTick;	// rewbs.betterBPM
	UINT m_nRowsPerBeat;	// rewbs.betterBPM
	UINT m_nRowsPerMeasure;	// rewbs.betterBPM
	BYTE m_nTempoMode;		// rewbs.betterBPM
	BYTE m_nMixLevels;
    UINT m_nMusicSpeed, m_nMusicTempo;
	UINT m_nNextRow, m_nRow;
	UINT m_nPattern,m_nCurrentPattern,m_nNextPattern,m_nRestartPos, m_nSeqOverride;
	//NOTE: m_nCurrentPattern and m_nNextPattern refer to order index - not pattern index.
	bool m_bPatternTransitionOccurred;
	UINT m_nMasterVolume, m_nGlobalVolume, m_nSamplesToGlobalVolRampDest,
		 m_nGlobalVolumeDestination, m_nSamplePreAmp, m_nVSTiVolume;
	long m_lHighResRampingGlobalVolume;
	UINT m_nFreqFactor, m_nTempoFactor, m_nOldGlbVolSlide;
	LONG m_nMinPeriod, m_nMaxPeriod, m_nRepeatCount;
	DWORD m_nGlobalFadeSamples, m_nGlobalFadeMaxSamples;
	UINT m_nMaxOrderPosition, m_nPatternNames;
	LPSTR m_lpszSongComments, m_lpszPatternNames;
	UINT ChnMix[MAX_CHANNELS];							// Channels to be mixed
	MODCHANNEL Chn[MAX_CHANNELS];						// Channels
	MODCHANNELSETTINGS ChnSettings[MAX_BASECHANNELS];	// Channels settings
	CPatternContainer Patterns;							// Patterns
	CPatternSizesMimic PatternSize;						// Mimics old PatternsSize-array(is read-only).
	ModSequenceSet Order;								// Modsequences. Order[x] returns an index of a pattern located at order x.
	MODSAMPLE Samples[MAX_SAMPLES];						// Sample Headers
	MODINSTRUMENT *Instruments[MAX_INSTRUMENTS];		// Instrument Headers
	MODINSTRUMENT m_defaultInstrument;					// Currently only used to get default values for extented properties. 
	CHAR m_szNames[MAX_SAMPLES][32];					// Song and sample names
	MODMIDICFG m_MidiCfg;								// Midi macro config table
	SNDMIXPLUGIN m_MixPlugins[MAX_MIXPLUGINS];			// Mix plugins
	SNDMIXSONGEQ m_SongEQ;								// Default song EQ preset
	CHAR CompressionTable[16];
	bool m_bChannelMuteTogglePending[MAX_BASECHANNELS];

	CSoundFilePlayConfig* m_pConfig;
	DWORD m_dwCreatedWithVersion;
	DWORD m_dwLastSavedWithVersion;

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	CHAR m_szInstrumentPath[MAX_INSTRUMENTS][_MAX_PATH];
	bool instrumentModified[MAX_INSTRUMENTS];
// -! NEW_FEATURE#0023

public:
	CSoundFile();
	~CSoundFile();

public:
	BOOL Create(LPCBYTE lpStream, CModDoc *pModDoc, DWORD dwMemLength=0);
	BOOL Destroy();
	MODTYPE GetType() const { return m_nType; }
	inline bool TypeIsIT_MPT() const {return (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) != 0;}
	inline bool TypeIsIT_MPT_XM() const {return (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)) != 0;}
	inline bool TypeIsS3M_IT_MPT() const {return (m_nType & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)) != 0;}
	inline bool TypeIsXM_MOD() const {return (m_nType & (MOD_TYPE_XM | MOD_TYPE_MOD)) != 0;}
	inline bool TypeIsMOD_S3M() const {return (m_nType & (MOD_TYPE_MOD | MOD_TYPE_S3M)) != 0;}
	CModDoc* GetpModDoc() {return m_pModDoc;}


	//Return the number of channels in the pattern. In 1.17.02.45
	//it returned the number of channels with volume != 0
	CHANNELINDEX GetNumChannels() const {return static_cast<CHANNELINDEX>(m_nChannels);}

	BOOL SetMasterVolume(UINT vol, BOOL bAdjustAGC=FALSE);
	UINT GetMasterVolume() const { return m_nMasterVolume; }
	UINT GetNumPatterns() const;
	UINT GetNumInstruments() const {return m_nInstruments;} 
	UINT GetNumSamples() const { return m_nSamples; }
	UINT GetCurrentPos() const;
	UINT GetCurrentPattern() const { return m_nPattern; }
	ORDERINDEX GetCurrentOrder() const { return static_cast<ORDERINDEX>(m_nCurrentPattern); }
	UINT GetSongComments(LPSTR s, UINT cbsize, UINT linesize=32);
	UINT GetRawSongComments(LPSTR s, UINT cbsize, UINT linesize=32);
	UINT GetMaxPosition() const;

	IMixPlugin* GetInstrumentPlugin(INSTRUMENTINDEX instr);
	const CModSpecifications& GetModSpecifications() const {return *m_pModSpecs;}
	static const CModSpecifications& GetModSpecifications(const MODTYPE type);

	double GetCurrentBPM() const;
	ORDERINDEX FindOrder(PATTERNINDEX pat, UINT startFromOrder=0, bool direction=true);	//rewbs.playSongFromCursor
	void DontLoopPattern(int nPat, int nRow=0);		//rewbs.playSongFromCursor
	void SetCurrentPos(UINT nPos);
	void SetCurrentOrder(UINT nOrder);
	void GetTitle(LPSTR s) const { lstrcpyn(s,m_szNames[0],32); }
	LPCSTR GetTitle() const { return m_szNames[0]; }
	CString GetSampleName(UINT nSample) const;
	CString GetInstrumentName(UINT nInstr) const;
	CString GetPatternViewInstrumentName(UINT nInstr, bool returnEmptyInsteadOfNoName = false) const;
	UINT GetMusicSpeed() const { return m_nMusicSpeed; }
	UINT GetMusicTempo() const { return m_nMusicTempo; }
    
	double GetLength(BOOL bAdjust, BOOL bTotal=FALSE);
private:
	//Get modlength in various cases: total length, length to 
	//specific order&row etc. Return value is in seconds.
	double GetLength(bool& targetReached, BOOL bAdjust, BOOL bTotal=FALSE, ORDERINDEX ord = ORDERINDEX_MAX, ROWINDEX row = ROWINDEX_MAX);

public:
	//Returns song length in seconds.
	DWORD GetSongTime() { return static_cast<DWORD>((m_nTempoMode == tempo_mode_alternative) ? GetLength(FALSE, TRUE)+1.0 : GetLength(FALSE, TRUE)+0.5); }

	void SetRepeatCount(int n) { m_nRepeatCount = n; }
	int GetRepeatCount() const { return m_nRepeatCount; }
	BOOL IsPaused() const {	return (m_dwSongFlags & SONG_PAUSED) ? TRUE : FALSE; }
	void LoopPattern(int nPat, int nRow=0);
	void CheckCPUUsage(UINT nCPU);
	BOOL SetPatternName(UINT nPat, LPCSTR lpszName);
	BOOL GetPatternName(UINT nPat, LPSTR lpszName, UINT cbSize=MAX_PATTERNNAME) const;
	CHANNELINDEX ReArrangeChannels(const vector<CHANNELINDEX>& fromToArray);
	bool MoveChannel(CHANNELINDEX chnFrom, CHANNELINDEX chnTo);

	bool InitChannel(CHANNELINDEX nch);
	void ResetChannelState(CHANNELINDEX chn, BYTE resetStyle);

	// Module Loaders
	bool ReadXM(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadS3M(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadMod(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadMed(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadMTM(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadSTM(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadIT(LPCBYTE lpStream, const DWORD dwMemLength);
	//bool ReadMPT(LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadITProject(LPCBYTE lpStream, const DWORD dwMemLength); // -> CODE#0023 -> DESC="IT project files (.itp)" -! NEW_FEATURE#0023
	bool Read669(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadUlt(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadWav(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadDSM(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadFAR(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadAMS(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadAMS2(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadMDL(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadOKT(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadDMF(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadPTM(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadDBM(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadAMF(LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMT2(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadPSM(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadJ2B(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadUMX(LPCBYTE lpStream, DWORD dwMemLength);
	bool ReadMO3(LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadGDM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadIMF(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMID(LPCBYTE lpStream, DWORD dwMemLength);
	// Save Functions
#ifndef MODPLUG_NO_FILESAVE
	UINT WriteSample(FILE *f, MODSAMPLE *pSmp, UINT nFlags, UINT nMaxLen=0);
	bool SaveXM(LPCSTR lpszFileName, UINT nPacking=0, const bool bCompatibilityExport = false);
	bool SaveS3M(LPCSTR lpszFileName, UINT nPacking=0);
	bool SaveMod(LPCSTR lpszFileName, UINT nPacking=0, const bool bCompatibilityExport = false);
	bool SaveIT(LPCSTR lpszFileName, UINT nPacking=0);
	bool SaveCompatIT(LPCSTR lpszFileName);
	bool SaveITProject(LPCSTR lpszFileName); // -> CODE#0023 -> DESC="IT project files (.itp)" -! NEW_FEATURE#0023
	UINT SaveMixPlugins(FILE *f=NULL, BOOL bUpdate=TRUE);
	void WriteInstrumentPropertyForAllInstruments(__int32 code,  __int16 size, FILE* f, MODINSTRUMENT* instruments[], UINT nInstruments);
	void SaveExtendedInstrumentProperties(MODINSTRUMENT *instruments[], UINT nInstruments, FILE* f);
	void SaveExtendedSongProperties(FILE* f);
	void LoadExtendedSongProperties(const MODTYPE modtype, LPCBYTE ptr, const LPCBYTE startpos, const size_t seachlimit, bool* pInterpretMptMade = NULL);

	// Reads extended instrument properties(XM/IT/MPTM). 
	// If no errors occur and song extension tag is found, returns pointer to the beginning
	// of the tag, else returns NULL.
	LPCBYTE LoadExtendedInstrumentProperties(const LPCBYTE pStart, const LPCBYTE pEnd, bool* pInterpretMptMade = NULL);

#endif // MODPLUG_NO_FILESAVE
	// MOD Convert function
	UINT GetBestSaveFormat() const;
	UINT GetSaveFormats() const;
	void ConvertModCommand(MODCOMMAND *) const;
	void S3MConvert(MODCOMMAND *m, BOOL bIT) const;
	void S3MSaveConvert(UINT *pcmd, UINT *pprm, BOOL bIT, BOOL bCompatibilityExport = false) const;
	WORD ModSaveCommand(const MODCOMMAND *m, const bool bXM, const bool bCompatibilityExport = false) const;
	
public:
	// Real-time sound functions
	VOID SuspendPlugins(); //rewbs.VSTCompliance
	VOID ResumePlugins();  //rewbs.VSTCompliance
	VOID StopAllVsti();    //rewbs.VSTCompliance
	VOID RecalculateGainForAllPlugs();
	VOID ResetChannels();
	UINT Read(LPVOID lpBuffer, UINT cbBuffer);
	UINT ReadMix(LPVOID lpBuffer, UINT cbBuffer, CSoundFile *, DWORD *, LPBYTE ps=NULL);
	UINT CreateStereoMix(int count);
	UINT GetResamplingFlag(const MODCHANNEL *pChannel);
	BOOL FadeSong(UINT msec);
	BOOL GlobalFadeSong(UINT msec);
	UINT GetTotalTickCount() const { return m_nTotalCount; }
	VOID ResetTotalTickCount() { m_nTotalCount = 0;}
	VOID ProcessPlugins(UINT nCount);

public:
	// Mixer Config
	static BOOL InitPlayer(BOOL bReset=FALSE);
	static BOOL SetWaveConfig(UINT nRate,UINT nBits,UINT nChannels,BOOL bMMX=FALSE);
	static BOOL SetDspEffects(BOOL bSurround,BOOL bReverb,BOOL xbass,BOOL dolbynr=FALSE,BOOL bEQ=FALSE);
	static BOOL SetResamplingMode(UINT nMode); // SRCMODE_XXXX
	static BOOL IsStereo() { return (gnChannels > 1) ? TRUE : FALSE; }
	static DWORD GetSampleRate() { return gdwMixingFreq; }
	static DWORD GetBitsPerSample() { return gnBitsPerSample; }
	static DWORD InitSysInfo();
	static DWORD GetSysInfo() { return gdwSysInfo; }
	static void EnableMMX(BOOL b) { if (b) gdwSoundSetup |= SNDMIX_ENABLEMMX; else gdwSoundSetup &= ~SNDMIX_ENABLEMMX; }
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
	UINT GetNNAChannel(UINT nChn) const;
	void CheckNNA(UINT nChn, UINT instr, int note, BOOL bForceCut);
	void NoteChange(UINT nChn, int note, bool bPorta = false, bool bResetEnv = true, bool bManual = false);
	void InstrumentChange(MODCHANNEL *pChn, UINT instr, BOOL bPorta=FALSE,BOOL bUpdVol=TRUE,BOOL bResetEnv=TRUE);

	// Channel Effects
	void KeyOff(UINT nChn);
	// Global Effects
	void SetTempo(UINT param, bool setAsNonModcommand = false);
	void SetSpeed(UINT param);

private:
	// Channel Effects
	void PortamentoUp(MODCHANNEL *pChn, UINT param, const bool fineAsRegular = false);
	void PortamentoDown(MODCHANNEL *pChn, UINT param, const bool fineAsRegular = false);
	void MidiPortamento(MODCHANNEL *pChn, int param);
	void FinePortamentoUp(MODCHANNEL *pChn, UINT param);
	void FinePortamentoDown(MODCHANNEL *pChn, UINT param);
	void ExtraFinePortamentoUp(MODCHANNEL *pChn, UINT param);
	void ExtraFinePortamentoDown(MODCHANNEL *pChn, UINT param);
	void NoteSlide(MODCHANNEL *pChn, UINT param, int sign);
	void TonePortamento(MODCHANNEL *pChn, UINT param);
	void Vibrato(MODCHANNEL *pChn, UINT param);
	void FineVibrato(MODCHANNEL *pChn, UINT param);
	void VolumeSlide(MODCHANNEL *pChn, UINT param);
	void PanningSlide(MODCHANNEL *pChn, UINT param);
	void ChannelVolSlide(MODCHANNEL *pChn, UINT param);
	void FineVolumeUp(MODCHANNEL *pChn, UINT param);
	void FineVolumeDown(MODCHANNEL *pChn, UINT param);
	void Tremolo(MODCHANNEL *pChn, UINT param);
	void Panbrello(MODCHANNEL *pChn, UINT param);
	void RetrigNote(UINT nChn, UINT param, UINT offset=0);  //rewbs.volOffset: added last param
	void SampleOffset(UINT nChn, UINT param, bool bPorta);	//rewbs.volOffset: moved offset code to own method
	void NoteCut(UINT nChn, UINT nTick);
	int PatternLoop(MODCHANNEL *, UINT param);
	void ExtendedMODCommands(UINT nChn, UINT param);
	void ExtendedS3MCommands(UINT nChn, UINT param);
	void ExtendedChannelEffect(MODCHANNEL *, UINT param);
	void ProcessMidiMacro(UINT nChn, LPCSTR pszMidiMacro, UINT param=0);
	void ProcessSmoothMidiMacro(UINT nChn, LPCSTR pszMidiMacro, UINT param=0); //rewbs.smoothVST
	void SetupChannelFilter(MODCHANNEL *pChn, bool bReset, int flt_modifier = 256) const;
	// Low-Level effect processing
	void DoFreqSlide(MODCHANNEL *pChn, LONG nFreqSlide);
	void GlobalVolSlide(UINT param, UINT * nOldGlobalVolSlide);
	DWORD IsSongFinished(UINT nOrder, UINT nRow) const;
	BOOL IsValidBackwardJump(UINT nStartOrder, UINT nStartRow, UINT nJumpOrder, UINT nJumpRow) const;
public:

	// Write pattern effect functions
	bool TryWriteEffect(PATTERNINDEX nPat, ROWINDEX nRow, BYTE nEffect, BYTE nParam, bool bIsVolumeEffect, CHANNELINDEX nChn = CHANNELINDEX_INVALID, bool bAllowMultipleEffects = true, bool bAllowNextRow = false, bool bRetry = true);
	
	// Read/Write sample functions
	char GetDeltaValue(char prev, UINT n) const { return (char)(prev + CompressionTable[n & 0x0F]); }
	UINT PackSample(int &sample, int next);
	bool CanPackSample(LPSTR pSample, UINT nLen, UINT nPacking, BYTE *result=NULL);
	UINT ReadSample(MODSAMPLE *pSmp, UINT nFlags, LPCSTR pMemFile, DWORD dwMemLength, const WORD format = 1);
	bool DestroySample(SAMPLEINDEX nSample);

// -> CODE#0020
// -> DESC="rearrange sample list"
	bool MoveSample(SAMPLEINDEX from, SAMPLEINDEX to);
// -! NEW_FEATURE#0020

// -> CODE#0003
// -> DESC="remove instrument's samples"
	//BOOL DestroyInstrument(UINT nInstr);
	bool DestroyInstrument(INSTRUMENTINDEX nInstr, char removeSamples = 0);
// -! BEHAVIOUR_CHANGE#0003
	bool IsSampleUsed(SAMPLEINDEX nSample);
	bool IsInstrumentUsed(INSTRUMENTINDEX nInstr);
	bool RemoveInstrumentSamples(INSTRUMENTINDEX nInstr);
	UINT DetectUnusedSamples(BYTE *); // bitmask
	bool RemoveSelectedSamples(bool *pbIns);
	void AdjustSampleLoop(MODSAMPLE *pSmp);
	// Samples file I/O
	bool ReadSampleFromFile(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadWAVSample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD *pdwWSMPOffset=NULL);
	bool ReadPATSample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadS3ISample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadAIFFSample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadXISample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength);

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
//	BOOL ReadITSSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD dwOffset=0);
	UINT ReadITSSample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD dwOffset=0);
// -! NEW_FEATURE#0027

	bool Read8SVXSample(UINT nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	bool SaveWAVSample(UINT nSample, LPCSTR lpszFileName);
	bool SaveRAWSample(UINT nSample, LPCSTR lpszFileName);
	// Instrument file I/O
	bool ReadInstrumentFromFile(INSTRUMENTINDEX nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadXIInstrument(INSTRUMENTINDEX nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadITIInstrument(INSTRUMENTINDEX nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadPATInstrument(INSTRUMENTINDEX nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	bool ReadSampleAsInstrument(INSTRUMENTINDEX nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	bool SaveXIInstrument(INSTRUMENTINDEX nInstr, LPCSTR lpszFileName);
	bool SaveITIInstrument(INSTRUMENTINDEX nInstr, LPCSTR lpszFileName);
	// I/O from another sound file
	bool ReadInstrumentFromSong(INSTRUMENTINDEX nInstr, CSoundFile *pSrcSong, UINT nSrcInstrument);
	bool ReadSampleFromSong(SAMPLEINDEX nSample, CSoundFile *pSrcSong, UINT nSrcSample);
	// Period/Note functions
	UINT GetNoteFromPeriod(UINT period) const;
	UINT GetPeriodFromNote(UINT note, int nFineTune, UINT nC5Speed) const;
	UINT GetFreqFromPeriod(UINT period, UINT nC5Speed, int nPeriodFrac=0) const;
	// Misc functions
	MODSAMPLE *GetSample(UINT n) { return Samples+n; }
	void ResetMidiCfg();
	UINT MapMidiInstrument(DWORD dwProgram, UINT nChannel, UINT nNote);
	long ITInstrToMPT(const void *p, MODINSTRUMENT *pIns, UINT trkvers); //change from BOOL for rewbs.modularInstData
	UINT LoadMixPlugins(const void *pData, UINT nLen);
//	PSNDMIXPLUGIN GetSndPlugMixPlug(IMixPlugin *pPlugin); //rewbs.plugDocAware
#ifndef NO_FILTER
	DWORD CutOffToFrequency(UINT nCutOff, int flt_modifier=256) const; // [0-255] => [1-10KHz]
#endif
#ifdef MODPLUG_TRACKER
	VOID ProcessMidiOut(UINT nChn, MODCHANNEL *pChn);		//rewbs.VSTdelay : added arg.
#endif
	VOID ApplyGlobalVolume(int SoundBuffer[], long lTotalSampleCount);

	// Static helper functions
public:
	static DWORD TransposeToFrequency(int transp, int ftune=0);
	static int FrequencyToTranspose(DWORD freq);
	static void FrequencyToTranspose(MODSAMPLE *psmp);

	// System-Dependant functions
public:
	static LPSTR AllocateSample(UINT nbytes);
	static void FreeSample(LPVOID p);
	static UINT Normalize24BitBuffer(LPBYTE pbuffer, UINT cbsizebytes, DWORD lmax24, DWORD dwByteInc);
	UINT GetBestPlugin(UINT nChn, UINT priority, bool respectMutes);
//private:
	static MODCOMMAND *AllocatePattern(UINT rows, UINT nchns);
	static void FreePattern(LPVOID pat);

public:
	int getVolEnvValueFromPosition(int position, MODINSTRUMENT* pIns);
    void resetEnvelopes(MODCHANNEL* pChn, int envToReset = ENV_RESET_ALL);
	void SetDefaultInstrumentValues(MODINSTRUMENT *pIns);
private:
	UINT  __cdecl GetChannelPlugin(UINT nChan, bool respectMutes);
	UINT  __cdecl GetActiveInstrumentPlugin(UINT nChan, bool respectMutes);
	UINT GetBestMidiChan(MODCHANNEL *pChn);

	void HandlePatternTransitionEvents();
	void BuildDefaultInstrument();
	long GetSampleOffset();
};

#pragma warning(default : 4324) //structure was padded due to __declspec(align())


inline uint32 MODSAMPLE::GetSampleRate(const MODTYPE type) const
//------------------------------------------------------------------
{
	uint32 nRate;
	if(type & (MOD_TYPE_MOD|MOD_TYPE_XM))
		nRate = CSoundFile::TransposeToFrequency(RelativeTone, nFineTune);
	else
		nRate = nC5Speed;
	return (nRate > 0) ? nRate : 8363;
}


inline IMixPlugin* CSoundFile::GetInstrumentPlugin(INSTRUMENTINDEX instr)
//----------------------------------------------------------------
{
	if(instr > 0 && instr < MAX_INSTRUMENTS && Instruments[instr] && Instruments[instr]->nMixPlug && Instruments[instr]->nMixPlug <= MAX_MIXPLUGINS)
		return m_MixPlugins[Instruments[instr]->nMixPlug-1].pMixPlugin;
	else
		return NULL;
}

// Ending swaps:
// BigEndian(x) may be used either to:
// -Convert DWORD x, which is in big endian format(for example read from file),
//		to endian format of current architecture.
// -Convert value x from endian format of current architecture to big endian format.
// Similarly LittleEndian(x) converts known little endian format to format of current
// endian architecture or value x in format of current architecture to little endian 
// format.
#ifdef PLATFORM_BIG_ENDIAN
// PPC
inline DWORD LittleEndian(DWORD x)	{ return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24); }
inline WORD LittleEndianW(WORD x)	{ return (WORD)(((x >> 8) & 0xFF) | ((x << 8) & 0xFF00)); }
#define BigEndian(x)				(x)
#define BigEndianW(x)				(x)
#else
// x86
inline DWORD BigEndian(DWORD x)	{ return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24); }
inline WORD BigEndianW(WORD x)	{ return (WORD)(((x >> 8) & 0xFF) | ((x << 8) & 0xFF00)); }
#define LittleEndian(x)			(x)
#define LittleEndianW(x)		(x)
#endif



//////////////////////////////////////////////////////////
// WAVE format information

#pragma pack(1)

// Standard IFF chunks IDs
#define IFFID_FORM		0x4d524f46
#define IFFID_RIFF		0x46464952
#define IFFID_WAVE		0x45564157
#define IFFID_LIST		0x5453494C
#define IFFID_INFO		0x4F464E49

// IFF Info fields
#define IFFID_ICOP		0x504F4349
#define IFFID_IART		0x54524149
#define IFFID_IPRD		0x44525049
#define IFFID_INAM		0x4D414E49
#define IFFID_ICMT		0x544D4349
#define IFFID_IENG		0x474E4549
#define IFFID_ISFT		0x54465349
#define IFFID_ISBJ		0x4A425349
#define IFFID_IGNR		0x524E4749
#define IFFID_ICRD		0x44524349

// Wave IFF chunks IDs
#define IFFID_wave		0x65766177
#define IFFID_fmt		0x20746D66
#define IFFID_wsmp		0x706D7377
#define IFFID_pcm		0x206d6370
#define IFFID_data		0x61746164
#define IFFID_smpl		0x6C706D73
#define IFFID_xtra		0x61727478

typedef struct WAVEFILEHEADER
{
	DWORD id_RIFF;		// "RIFF"
	DWORD filesize;		// file length-8
	DWORD id_WAVE;
} WAVEFILEHEADER;


typedef struct WAVEFORMATHEADER
{
	DWORD id_fmt;		// "fmt "
	DWORD hdrlen;		// 16
	WORD format;		// 1
	WORD channels;		// 1:mono, 2:stereo
	DWORD freqHz;		// sampling freq
	DWORD bytessec;		// bytes/sec=freqHz*samplesize
	WORD samplesize;	// sizeof(sample)
	WORD bitspersample;	// bits per sample (8/16)
} WAVEFORMATHEADER;


typedef struct WAVEDATAHEADER
{
	DWORD id_data;		// "data"
	DWORD length;		// length of data
} WAVEDATAHEADER;


typedef struct WAVESMPLHEADER
{
	// SMPL
	DWORD smpl_id;		// "smpl"	-> 0x6C706D73
	DWORD smpl_len;		// length of smpl: 3Ch	(54h with sustain loop)
	DWORD dwManufacturer;
	DWORD dwProduct;
	DWORD dwSamplePeriod;	// 1000000000/freqHz
	DWORD dwBaseNote;	// 3Ch = C-4 -> 60 + RelativeTone
	DWORD dwPitchFraction;
	DWORD dwSMPTEFormat;
	DWORD dwSMPTEOffset;
	DWORD dwSampleLoops;	// number of loops
	DWORD cbSamplerData;
} WAVESMPLHEADER;


typedef struct SAMPLELOOPSTRUCT
{
	DWORD dwIdentifier;
	DWORD dwLoopType;		// 0=normal, 1=bidi
	DWORD dwLoopStart;
	DWORD dwLoopEnd;		// Byte offset ?
	DWORD dwFraction;
	DWORD dwPlayCount;		// Loop Count, 0=infinite
} SAMPLELOOPSTRUCT;


typedef struct WAVESAMPLERINFO
{
	WAVESMPLHEADER wsiHdr;
	SAMPLELOOPSTRUCT wsiLoops[2];
} WAVESAMPLERINFO;


typedef struct WAVELISTHEADER
{
	DWORD list_id;	// "LIST" -> 0x5453494C
	DWORD list_len;
	DWORD info;		// "INFO"
} WAVELISTHEADER;


typedef struct WAVEEXTRAHEADER
{
	DWORD xtra_id;	// "xtra"	-> 0x61727478
	DWORD xtra_len;
	DWORD dwFlags;
	WORD  wPan;
	WORD  wVolume;
	WORD  wGlobalVol;
	WORD  wReserved;
	BYTE nVibType;
	BYTE nVibSweep;
	BYTE nVibDepth;
	BYTE nVibRate;
} WAVEEXTRAHEADER;

#pragma pack()

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
	DWORD dwFormatId;		// MOD_TYPE_XXXX
	LPCSTR lpszFormatName;	// "ProTracker"
	LPCSTR lpszExtension;	// ".xxx"
	DWORD dwPadding;
} MODFORMATINFO;

extern MODFORMATINFO gModFormatInfo[MAX_MODTYPE];

#endif


// Used in instrument/song extension reading to make sure the size field is valid.
bool IsValidSizeField(const LPCBYTE pData, const LPCBYTE pEnd, const int16 size);

// Read instrument property with 'code' and 'size' from 'ptr' to instrument 'pIns'.
// Note: (ptr, size) pair must be valid (e.g. can read 'size' bytes from 'ptr')
void ReadInstrumentExtensionField(MODINSTRUMENT* pIns, LPCBYTE& ptr, const int32 code, const int16 size);

// Read instrument property with 'code' from 'pData' to instrument 'pIns'.
void ReadExtendedInstrumentProperty(MODINSTRUMENT* pIns, const int32 code, LPCBYTE& pData, const LPCBYTE pEnd);

// Read extended instrument properties from 'pDataStart' to instrument 'pIns'.
void ReadExtendedInstrumentProperties(MODINSTRUMENT* pIns, const LPCBYTE pDataStart, const size_t nMemLength);


#endif
