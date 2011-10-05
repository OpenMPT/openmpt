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
#include "../common/misc_util.h"
#include "mod_specifications.h"
#include <vector>
#include <bitset>
#include "midi.h"
#include "Snd_defs.h"
#include "Endianness.h"
#include "tuning.h"

// For VstInt32 and stuff - a stupid workaround for IMixPlugin.
#ifndef NO_VST
#define VST_FORCE_DEPRECATED 0
#include <aeffect.h>			// VST
#else
typedef int32 VstInt32;
typedef intptr_t VstIntPtr;
#endif

//class CTuningBase;
//typedef CTuningBase CTuning;


// Sample Struct
struct MODSAMPLE
{
	UINT nLength, nLoopStart, nLoopEnd;	// In samples, not bytes
	UINT nSustainStart, nSustainEnd;	// Dito
	LPSTR pSample;						// Pointer to sample data
	UINT nC5Speed;						// Frequency of middle c, in Hz (for IT/S3M/MPTM)
	WORD nPan;							// Default sample panning (if pan flag is set)
	WORD nVolume;						// Default volume
	WORD nGlobalVol;					// Global volume (sample volume is multiplied by this)
	WORD uFlags;						// Sample flags
	signed char RelativeTone;			// Relative note to middle c (for MOD/XM)
	signed char nFineTune;				// Finetune period (for MOD/XM)
	BYTE nVibType;						// Auto vibrato type
	BYTE nVibSweep;						// Auto vibrato sweep (i.e. how long it takes until the vibrato effect reaches its full strength)
	BYTE nVibDepth;						// Auto vibrato depth
	BYTE nVibRate;						// Auto vibrato rate (speed)
	//CHAR name[MAX_SAMPLENAME];		// Maybe it would be nicer to have sample names here, but that would require some refactoring. Also, would this slow down the mixer (cache misses)?
	CHAR filename[MAX_SAMPLEFILENAME];

	// Return the size of one (elementary) sample in bytes.
	uint8 GetElementarySampleSize() const {return (uFlags & CHN_16BIT) ? 2 : 1;}

	// Return the number of channels in the sample.
	uint8 GetNumChannels() const {return (uFlags & CHN_STEREO) ? 2 : 1;}

	// Return the number of bytes per sampling point. (Channels * Elementary Sample Size)
	uint8 GetBytesPerSample() const {return GetElementarySampleSize() * GetNumChannels();}

	// Return the size which pSample is at least.
	DWORD GetSampleSizeInBytes() const {return nLength * GetBytesPerSample();}

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
	DWORD dwFlags;				// envelope flags
	UINT nNodes;				// amount of nodes used
	BYTE nLoopStart;			// loop start node
	BYTE nLoopEnd;				// loop end node
	BYTE nSustainStart;			// sustain start node
	BYTE nSustainEnd;			// sustain end node
	BYTE nReleaseNode;			// release node
	WORD Ticks[MAX_ENVPOINTS];	// envelope point position (x axis)
	BYTE Values[MAX_ENVPOINTS];	// envelope point value (y axis)

	INSTRUMENTENVELOPE()
	{
		dwFlags = 0;
		nNodes = 0;
		nLoopStart = nLoopEnd = 0;
		nSustainStart = nSustainEnd = 0;
		nReleaseNode = ENV_RELEASE_NODE_UNSET;
		MemsetZero(Ticks);
		MemsetZero(Values);
	}
};

// Instrument Struct
struct MODINSTRUMENT
{
	UINT nFadeOut;		// Instrument fadeout speed
	DWORD dwFlags;		// Instrument flags
	UINT nGlobalVol;	// Global volume (0...64, all sample volumes are multiplied with this - TODO: This is 0...128 in Impulse Tracker)
	UINT nPan;			// Default pan (0...256), if the appropriate flag is set. Sample panning overrides instrument panning.

	BYTE nNNA;			// New note action
	BYTE nDCT;			// Duplicate check type	(i.e. which condition will trigger the duplicate note action)
	BYTE nDNA;			// Duplicate note action
	BYTE nPanSwing;		// Random panning factor (0...64)
	BYTE nVolSwing;		// Random volume factor (0...100)
	BYTE nIFC;			// Default filter cutoff (0...127). Used if the high bit is set
	BYTE nIFR;			// Default filter resonance (0...127). Used if the high bit is set

	WORD wMidiBank;		// MIDI bank
	BYTE nMidiProgram;	// MIDI program
	BYTE nMidiChannel;	// MIDI channel
	BYTE nMidiDrumKey;	// Drum set note mapping (currently only used by the .MID loader)

	signed char nPPS;	//Pitch/Pan separation (i.e. how wide the panning spreads)
	unsigned char nPPC;	//Pitch/Pan centre

	PLUGINDEX nMixPlug;				// Plugin assigned to this instrument
	uint16 nVolRampUp;				// Default sample ramping up
	UINT nResampling;				// Resampling mode
	BYTE nCutSwing;					// Random cutoff factor (0...64)
	BYTE nResSwing;					// Random resonance factor (0...64)
	BYTE nFilterMode;				// Default filter mode
	WORD wPitchToTempoLock;			// BPM at which the samples assigned to this instrument loop correctly
	BYTE nPluginVelocityHandling;	// How to deal with plugin velocity
	BYTE nPluginVolumeHandling;		// How to deal with plugin volume
	CTuning *pTuning;				// sample tuning assigned to this instrument
	static CTuning *s_DefaultTuning;

	INSTRUMENTENVELOPE VolEnv;		// Volume envelope data
	INSTRUMENTENVELOPE PanEnv;		// Panning envelope data
	INSTRUMENTENVELOPE PitchEnv;	// Pitch / filter envelope data

	BYTE NoteMap[128];			// Note mapping, f.e. C-5 => D-5.
	SAMPLEINDEX Keyboard[128];	// Sample mapping, f.e. C-5 => Sample 1

	CHAR name[MAX_INSTRUMENTNAME];		// Note: not guaranteed to be null-terminated.
	CHAR filename[MAX_INSTRUMENTFILENAME];

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// WHEN adding new members here, ALSO update Sndfile.cpp (instructions near the top of this file)!
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	void SetTuning(CTuning* pT)
	{
		pTuning = pT;
	}

	MODINSTRUMENT(SAMPLEINDEX sample = 0)
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
		nIFC = 0;
		nIFR = 0;

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

		for(size_t n = 0; n < CountOf(Keyboard); n++)
		{
			Keyboard[n] = sample;
			NoteMap[n] = (BYTE)(n + 1);
		}

		MemsetZero(name);
		MemsetZero(filename);
	}

};

//MODINSTRUMENT;

// -----------------------------------------------------------------------------------------
// MODULAR MODINSTRUMENT FIELD ACCESS : body content at the (near) top of Sndfile.cpp !!!
// -----------------------------------------------------------------------------------------
extern void WriteInstrumentHeaderStruct(MODINSTRUMENT * input, FILE * file);
extern BYTE * GetInstrumentHeaderFieldPointer(const MODINSTRUMENT * input, __int32 fcode, __int16 fsize);

// -! NEW_FEATURE#0027

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------


// Envelope playback info for each MODCHANNEL
struct  MODCHANNEL_ENVINFO
{
	DWORD nEnvPosition;
	DWORD flags;
	LONG nEnvValueAtReleaseJump;
};


#pragma warning(disable : 4324) //structure was padded due to __declspec(align())

// Channel Struct
typedef struct __declspec(align(32)) _MODCHANNEL
{
	// First 32-bytes: Most used mixing information: don't change it
	// These fields are accessed directly by the MMX mixing code (look out for CHNOFS_PCURRENTSAMPLE), so the order is crucial
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
	float nFilter_Y1, nFilter_Y2, nFilter_Y3, nFilter_Y4;
	float nFilter_A0, nFilter_B0, nFilter_B1;
	int nFilter_HP;
	LONG nROfs, nLOfs;
	LONG nRampLength;
	// Information not used in the mixer
	LPSTR pSample;
	LONG nNewRightVol, nNewLeftVol;
	LONG nRealVolume, nRealPan;
	LONG nVolume, nPan, nFadeOutVol;
	LONG nPeriod, nC5Speed, nPortamentoDest;
	int nCalcVolume;								// Calculated channel volume, 14-Bit (without global volume, pre-amp etc applied) - for MIDI macros
	MODINSTRUMENT *pModInstrument;					// Currently assigned instrument slot
	MODCHANNEL_ENVINFO VolEnv, PanEnv, PitchEnv;	// Envelope playback info
	MODSAMPLE *pModSample;							// Currently assigned sample slot
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

	MODCOMMAND rowCommand;

	//NOTE_PCs memory.
	uint16 m_RowPlugParam;
	float m_plugParamValueStep, m_plugParamTargetValue;
	PLUGINDEX m_RowPlug;
	
	void ClearRowCmd() { rowCommand = MODCOMMAND::Empty(); }

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
} MODCHANNEL;

#define CHNRESET_CHNSETTINGS	1 //  1 b 
#define CHNRESET_SETPOS_BASIC	2 // 10 b
#define	CHNRESET_SETPOS_FULL	7 //111 b
#define CHNRESET_TOTAL			255 //11111111b


struct MODCHANNELSETTINGS
{
	UINT nPan;
	UINT nVolume;
	DWORD dwFlags;
	PLUGINDEX nMixPlugin;
	CHAR szName[MAX_CHANNELNAME];
};

#include "modcommand.h"


////////////////////////////////////////////////////////////////////
// Mix Plugins
#define MIXPLUG_MIXREADY			0x01	// Set when cleared

typedef VstInt32 PlugParamIndex;
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
	virtual VstIntPtr Dispatch(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt) =0; //rewbs.VSTCompliance
	virtual void NotifySongPlaying(bool)=0;	//rewbs.VSTCompliance
	virtual bool IsSongPlaying()=0;
	virtual bool IsResumed()=0;
	virtual void Resume()=0;
	virtual void Suspend()=0;
	virtual bool isInstrument()=0;
	virtual bool CanRecieveMidiEvents()=0;
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
	DWORD dwInputRouting;	// MIXPLUG_INPUTF_XXXX, bits 16-23 = gain
	DWORD dwOutputRouting;	// 0=mix 0x80+=fx
	DWORD dwReserved[4];	// Reserved for routing info
	CHAR szName[32];
	CHAR szLibraryName[64];	// original DLL name
}; // Size should be 128 							
typedef SNDMIXPLUGININFO* PSNDMIXPLUGININFO;
STATIC_ASSERT(sizeof(SNDMIXPLUGININFO) == 128);	// this is directly written to files, so the size must be correct!

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

// Global MIDI macros
enum
{
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


#define NUM_MACROS 16	// number of parametered macros
#define MACRO_LENGTH 32	// max number of chars per macro
struct MODMIDICFG
{
	char szMidiGlb[9][MACRO_LENGTH];		// Global MIDI macros
	char szMidiSFXExt[16][MACRO_LENGTH];	// Parametric MIDI macros
	char szMidiZXXExt[128][MACRO_LENGTH];	// Fixed MIDI macros
};
STATIC_ASSERT(sizeof(MODMIDICFG) == 4896); // this is directly written to files, so the size must be correct!

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

// Data type for the visited rows routines.
typedef vector<bool> VisitedRowsBaseType;
typedef vector<VisitedRowsBaseType> VisitedRowsType;


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


// Row advance mode for TryWriteEffect()
enum writeEffectAllowRowChange
{
	weIgnore,			// If effect can't be written, abort.
	weTryNextRow,		// If effect can't be written, try next row.
	weTryPreviousRow,	// If effect can't be written, try previous row.
};


// Delete samples assigned to instrument
enum deleteInstrumentSamples
{
	deleteAssociatedSamples,
	doNoDeleteAssociatedSamples,
	askdeleteAssociatedSamples,
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
		return ((GetType() & type) && GetModFlag(MSF_COMPATIBLE_PLAY)) ? true : false;
	}

	// Check whether a filter algorithm closer to IT's should be used.
	bool UseITFilterMode() const { return IsCompatibleMode(TRK_IMPULSETRACKER) && !(m_dwSongFlags & SONG_EXFILTERRANGE); }
	
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
	bool m_bITBidiMode;	// Process bidi loops like Impulse Tracker (see Fastmix.cpp for an explanation)

	// For handling backwards jumps and stuff to prevent infinite loops when counting the mod length or rendering to wav.
	VisitedRowsType m_VisitedRows;


public:	// Static Members
	static UINT m_nXBassDepth, m_nXBassRange;
	static float m_nMaxSample;
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


public:	// for Editing
	CModDoc *m_pModDoc;		// Can be a null pointer f.e. when previewing samples from the treeview.
	MODTYPE m_nType;
	CHANNELINDEX m_nChannels;
	SAMPLEINDEX m_nSamples;
	INSTRUMENTINDEX m_nInstruments;
	UINT m_nDefaultSpeed, m_nDefaultTempo, m_nDefaultGlobalVolume;
	DWORD m_dwSongFlags;							// Song flags SONG_XXXX
	bool m_bIsRendering;
	UINT m_nMixChannels, m_nMixStat, m_nBufferCount;
	double m_dBufferDiff;
	UINT m_nTickCount, m_nTotalCount;
	UINT m_nPatternDelay, m_nFrameDelay;	// m_nPatternDelay = pattern delay (rows), m_nFrameDelay = fine pattern delay (ticks)
	ULONG m_lTotalSampleCount;	// rewbs.VSTTimeInfo
	UINT m_nSamplesPerTick;		// rewbs.betterBPM
	ROWINDEX m_nDefaultRowsPerBeat, m_nDefaultRowsPerMeasure;	// default rows per beat and measure for this module // rewbs.betterBPM
	ROWINDEX m_nCurrentRowsPerBeat, m_nCurrentRowsPerMeasure;	// current rows per beat and measure for this module
	BYTE m_nTempoMode;			// rewbs.betterBPM
	BYTE m_nMixLevels;
    UINT m_nMusicSpeed, m_nMusicTempo;	// Current speed and tempo
	ROWINDEX m_nNextRow, m_nRow;
	ROWINDEX m_nNextPatStartRow; // for FT2's E60 bug
	PATTERNINDEX m_nPattern;
	ORDERINDEX m_nCurrentPattern, m_nNextPattern, m_nRestartPos, m_nSeqOverride;
	//NOTE: m_nCurrentPattern and m_nNextPattern refer to order index - not pattern index.
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
	MODCHANNEL Chn[MAX_CHANNELS];						// Mixing channels... First m_nChannel channels are master channels (i.e. they are never NNA channels)!
	MODCHANNELSETTINGS ChnSettings[MAX_BASECHANNELS];	// Initial channels settings
	CPatternContainer Patterns;							// Patterns
	ModSequenceSet Order;								// Modsequences. Order[x] returns an index of a pattern located at order x of the current sequence.
protected:
	MODSAMPLE Samples[MAX_SAMPLES];						// Sample Headers
public:
	MODINSTRUMENT *Instruments[MAX_INSTRUMENTS];		// Instrument Headers
	CHAR m_szNames[MAX_SAMPLES][MAX_SAMPLENAME];		// Song and sample names
	MODMIDICFG m_MidiCfg;								// Midi macro config table
	SNDMIXPLUGIN m_MixPlugins[MAX_MIXPLUGINS];			// Mix plugins
	CHAR CompressionTable[16];							// ADPCM compression LUT
	bool m_bChannelMuteTogglePending[MAX_BASECHANNELS];

	CSoundFilePlayConfig* m_pConfig;
	DWORD m_dwCreatedWithVersion;
	DWORD m_dwLastSavedWithVersion;

	vector<PatternCuePoint> m_PatternCuePoints;			// For WAV export (writing pattern positions to file)

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
	CModDoc* GetpModDoc() const { return m_pModDoc; }

	void SetMasterVolume(UINT vol, bool adjustAGC = false);
	UINT GetMasterVolume() const { return m_nMasterVolume; }

	INSTRUMENTINDEX GetNumInstruments() const { return m_nInstruments; } 
	SAMPLEINDEX GetNumSamples() const { return m_nSamples; }
	UINT GetCurrentPos() const;
	PATTERNINDEX GetCurrentPattern() const { return m_nPattern; }
	ORDERINDEX GetCurrentOrder() const { return static_cast<ORDERINDEX>(m_nCurrentPattern); }
	CHANNELINDEX GetNumChannels() const { return m_nChannels; }

	IMixPlugin* GetInstrumentPlugin(INSTRUMENTINDEX instr);
	const CModSpecifications& GetModSpecifications() const {return *m_pModSpecs;}
	static const CModSpecifications& GetModSpecifications(const MODTYPE type);

	double GetCurrentBPM() const;
	ORDERINDEX FindOrder(PATTERNINDEX nPat, ORDERINDEX startFromOrder = 0, bool direction = true);	//rewbs.playSongFromCursor
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
	DWORD GetSongTime() { return static_cast<DWORD>((m_nTempoMode == tempo_mode_alternative) ? GetLength(eNoAdjust).duration + 1.0 : GetLength(eNoAdjust).duration + 0.5); }

	// A repeat count value of -1 means infinite loop
	void SetRepeatCount(int n) { m_nRepeatCount = n; }
	int GetRepeatCount() const { return m_nRepeatCount; }
	bool IsPaused() const {	return (m_dwSongFlags & (SONG_PAUSED|SONG_STEP)) ? true : false; }	// Added SONG_STEP as it seems to be desirable in most cases to check for this as well.
	void LoopPattern(PATTERNINDEX nPat, ROWINDEX nRow = 0);
	void CheckCPUUsage(UINT nCPU);

	void SetupITBidiMode();

	bool InitChannel(CHANNELINDEX nChn);
	void ResetChannelState(CHANNELINDEX chn, BYTE resetStyle);

	// Module Loaders
	bool ReadXM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadS3M(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMod(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMed(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMTM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadSTM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadIT(const LPCBYTE lpStream, const DWORD dwMemLength);
	//bool ReadMPT(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadITProject(const LPCBYTE lpStream, const DWORD dwMemLength); // -> CODE#0023 -> DESC="IT project files (.itp)" -! NEW_FEATURE#0023
	bool Read669(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadUlt(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadWav(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadDSM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadFAR(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadAMS(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadAMS2(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMDL(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadOKT(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadDMF(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadPTM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadDBM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadAMF(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMT2(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadPSM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadPSM16(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadUMX(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMO3(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadGDM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadIMF(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadAM(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadJ2B(const LPCBYTE lpStream, const DWORD dwMemLength);
	bool ReadMID(const LPCBYTE lpStream, DWORD dwMemLength);

	void UpgradeModFlags();
	void UpgradeSong();
	static void FixMIDIConfigStrings(MODMIDICFG &midiCfg);

	// Save Functions
#ifndef MODPLUG_NO_FILESAVE
	UINT WriteSample(FILE *f, const MODSAMPLE *pSmp, UINT nFlags, UINT nMaxLen=0) const;
	bool SaveXM(LPCSTR lpszFileName, UINT nPacking=0, const bool bCompatibilityExport = false);
	bool SaveS3M(LPCSTR lpszFileName, UINT nPacking=0);
	bool SaveMod(LPCSTR lpszFileName, UINT nPacking=0, const bool bCompatibilityExport = false);
	bool SaveIT(LPCSTR lpszFileName, UINT nPacking=0, const bool compatExport = false);
	bool SaveITProject(LPCSTR lpszFileName); // -> CODE#0023 -> DESC="IT project files (.itp)" -! NEW_FEATURE#0023
	UINT SaveMixPlugins(FILE *f=NULL, BOOL bUpdate=TRUE);
	void WriteInstrumentPropertyForAllInstruments(__int32 code,  __int16 size, FILE* f, UINT nInstruments) const;
	void SaveExtendedInstrumentProperties(UINT nInstruments, FILE* f) const;
	void SaveExtendedSongProperties(FILE* f) const;
	void LoadExtendedSongProperties(const MODTYPE modtype, LPCBYTE ptr, const LPCBYTE startpos, const size_t seachlimit, bool* pInterpretMptMade = nullptr);
#endif // MODPLUG_NO_FILESAVE

	// Reads extended instrument properties(XM/IT/MPTM). 
	// If no errors occur and song extension tag is found, returns pointer to the beginning
	// of the tag, else returns NULL.
	LPCBYTE LoadExtendedInstrumentProperties(const LPCBYTE pStart, const LPCBYTE pEnd, bool* pInterpretMptMade = nullptr);

	// MOD Convert function
	MODTYPE GetBestSaveFormat() const;
	MODTYPE GetSaveFormats() const;
	void ConvertModCommand(MODCOMMAND *) const;
	void S3MConvert(MODCOMMAND *m, bool bIT) const;
	void S3MSaveConvert(UINT *pcmd, UINT *pprm, bool bIT, bool bCompatibilityExport = false) const;
	WORD ModSaveCommand(const MODCOMMAND *m, const bool bXM, const bool bCompatibilityExport = false) const;
	
	static void ConvertCommand(MODCOMMAND *m, MODTYPE nOldType, MODTYPE nNewType); // Convert a complete MODCOMMAND item from one format to another
	static void MODExx2S3MSxx(MODCOMMAND *m); // Convert Exx to Sxx
	static void S3MSxx2MODExx(MODCOMMAND *m); // Convert Sxx to Exx
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
	UINT GetResamplingFlag(const MODCHANNEL *pChannel);
	BOOL FadeSong(UINT msec);
	BOOL GlobalFadeSong(UINT msec);
	UINT GetTotalTickCount() const { return m_nTotalCount; }
	void ResetTotalTickCount() { m_nTotalCount = 0;}
	void ProcessPlugins(UINT nCount);

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
	CHANNELINDEX GetNNAChannel(CHANNELINDEX nChn) const;
	void CheckNNA(CHANNELINDEX nChn, UINT instr, int note, BOOL bForceCut);
	void NoteChange(CHANNELINDEX nChn, int note, bool bPorta = false, bool bResetEnv = true, bool bManual = false);
	void InstrumentChange(MODCHANNEL *pChn, UINT instr, bool bPorta = false, bool bUpdVol = true, bool bResetEnv = true);

	// Channel Effects
	void KeyOff(CHANNELINDEX nChn);
	// Global Effects
	void SetTempo(UINT param, bool setAsNonModcommand = false);
	void SetSpeed(UINT param);

protected:
	// Channel effect processing
	void ProcessVolumeSwing(MODCHANNEL *pChn, int &vol);
	void ProcessPanningSwing(MODCHANNEL *pChn);
	void ProcessTremolo(MODCHANNEL *pChn, int &vol);
	void ProcessTremor(MODCHANNEL *pChn, int &vol);

	void ProcessVolumeEnvelope(MODCHANNEL *pChn, int &vol);
	void ProcessPanningEnvelope(MODCHANNEL *pChn);
	void ProcessPitchFilterEnvelope(MODCHANNEL *pChn, int &period);

	void IncrementVolumeEnvelopePosition(MODCHANNEL *pChn);
	void IncrementPanningEnvelopePosition(MODCHANNEL *pChn);
	void IncrementPitchFilterEnvelopePosition(MODCHANNEL *pChn);

	void ProcessInstrumentFade(MODCHANNEL *pChn, int &vol);

	void ProcessPitchPanSeparation(MODCHANNEL *pChn);
	void ProcessPanbrello(MODCHANNEL *pChn);

	void ProcessArpeggio(MODCHANNEL *pChn, int &period, CTuning::NOTEINDEXTYPE &arpeggioSteps);
	void ProcessVibrato(MODCHANNEL *pChn, int &period, CTuning::RATIOTYPE &vibratoFactor);
	void ProcessSampleAutoVibrato(MODCHANNEL *pChn, int &period, CTuning::RATIOTYPE &vibratoFactor, int &nPeriodFrac);

	void ProcessRamping(MODCHANNEL *pChn);

protected:
	// Channel Effects
	void UpdateS3MEffectMemory(MODCHANNEL *pChn, UINT param) const;
	void PortamentoUp(MODCHANNEL *pChn, UINT param, const bool fineAsRegular = false);
	void PortamentoDown(MODCHANNEL *pChn, UINT param, const bool fineAsRegular = false);
	void MidiPortamento(MODCHANNEL *pChn, int param);
	void FinePortamentoUp(MODCHANNEL *pChn, UINT param);
	void FinePortamentoDown(MODCHANNEL *pChn, UINT param);
	void ExtraFinePortamentoUp(MODCHANNEL *pChn, UINT param);
	void ExtraFinePortamentoDown(MODCHANNEL *pChn, UINT param);
	void NoteSlide(MODCHANNEL *pChn, UINT param, bool slideUp);
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
	void RetrigNote(CHANNELINDEX nChn, int param, UINT offset=0);  //rewbs.volOffset: added last param
	void SampleOffset(CHANNELINDEX nChn, UINT param, bool bPorta);	//rewbs.volOffset: moved offset code to own method
	void NoteCut(CHANNELINDEX nChn, UINT nTick);
	int PatternLoop(MODCHANNEL *, UINT param);
	void ExtendedMODCommands(CHANNELINDEX nChn, UINT param);
	void ExtendedS3MCommands(CHANNELINDEX nChn, UINT param);
	void ExtendedChannelEffect(MODCHANNEL *, UINT param);
	inline void InvertLoop(MODCHANNEL* pChn);
	void ProcessMacroOnChannel(CHANNELINDEX nChn);
	void ProcessMIDIMacro(CHANNELINDEX nChn, bool isSmooth, char *macro, uint8 param = 0, PLUGINDEX plugin = 0);
	float CalculateSmoothParamChange(float currentValue, float param) const;
	size_t SendMIDIData(CHANNELINDEX nChn, bool isSmooth, const unsigned char *macro, size_t macroLen, PLUGINDEX plugin);

	void SetupChannelFilter(MODCHANNEL *pChn, bool bReset, int flt_modifier = 256) const;
	// Low-Level effect processing
	void DoFreqSlide(MODCHANNEL *pChn, LONG nFreqSlide);
	void GlobalVolSlide(UINT param, UINT &nOldGlobalVolSlide);
	DWORD IsSongFinished(UINT nOrder, UINT nRow) const;
	void UpdateTimeSignature();

	UINT GetNumTicksOnCurrentRow() const { return m_nMusicSpeed * (m_nPatternDelay + 1) + m_nFrameDelay; };
public:
	// Write pattern effect functions
	bool TryWriteEffect(PATTERNINDEX nPat, ROWINDEX nRow, BYTE nEffect, BYTE nParam, bool bIsVolumeEffect, CHANNELINDEX nChn = CHANNELINDEX_INVALID, bool bAllowMultipleEffects = true, writeEffectAllowRowChange allowRowChange = weIgnore, bool bRetry = true);
	
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

	bool DestroyInstrument(INSTRUMENTINDEX nInstr, deleteInstrumentSamples removeSamples);
	bool IsSampleUsed(SAMPLEINDEX nSample) const;
	bool IsInstrumentUsed(INSTRUMENTINDEX nInstr) const;
	bool RemoveInstrumentSamples(INSTRUMENTINDEX nInstr);
	SAMPLEINDEX DetectUnusedSamples(vector<bool> &sampleUsed) const;
	SAMPLEINDEX RemoveSelectedSamples(const vector<bool> &keepSamples);
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
	MODSAMPLE &GetSample(SAMPLEINDEX sample) { ASSERT(sample <= m_nSamples && sample < CountOf(Samples)); return Samples[sample]; }
	const MODSAMPLE &GetSample(SAMPLEINDEX sample) const { ASSERT(sample <= m_nSamples && sample < CountOf(Samples)); return Samples[sample]; }
	static void ResetMidiCfg(MODMIDICFG &midiConfig);
	void SanitizeMacros();
	UINT MapMidiInstrument(DWORD dwProgram, UINT nChannel, UINT nNote);
	long ITInstrToMPT(const void *p, MODINSTRUMENT *pIns, UINT trkvers); //change from BOOL for rewbs.modularInstData
	UINT LoadMixPlugins(const void *pData, UINT nLen);
//	PSNDMIXPLUGIN GetSndPlugMixPlug(IMixPlugin *pPlugin); //rewbs.plugDocAware
#ifndef NO_FILTER
	DWORD CutOffToFrequency(UINT nCutOff, int flt_modifier=256) const; // [0-127] => [1-10KHz]
#endif
#ifdef MODPLUG_TRACKER
	void ProcessMidiOut(CHANNELINDEX nChn, MODCHANNEL *pChn);		//rewbs.VSTdelay : added arg.
#endif
	void ApplyGlobalVolume(int SoundBuffer[], int RearBuffer[], long lTotalSampleCount);

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
	bool ReadMessage(const BYTE *data, const size_t length, enmLineEndings lineEnding, void (*pTextConverter)(char &) = nullptr);

	// Read comments with fixed line length from a mapped file.
	// [in]  data: pointer to the data in memory that is going to be read
	// [in]  length: number of characters that should be read, not including a possible trailing null terminator (it is automatically appended).
	// [in]  lineLength: The fixed length of a line.
	// [in]  lineEndingLength: The padding space between two fixed lines. (there could for example be a null char after every line)
	// [in]  pTextConverter: Pointer to a callback function which can be used to pre-process the read characters, if necessary (nullptr otherwise).
	// [out] returns true on success.
	bool ReadFixedLineLengthMessage(const BYTE *data, const size_t length, const size_t lineLength, const size_t lineEndingLength, void (*pTextConverter)(char &) = nullptr);

public:
	int GetVolEnvValueFromPosition(int position, const MODINSTRUMENT* pIns) const;
    void ResetChannelEnvelopes(MODCHANNEL *pChn) const;
	void ResetChannelEnvelope(MODCHANNEL_ENVINFO &env) const;
private:
	PLUGINDEX __cdecl GetChannelPlugin(CHANNELINDEX nChn, PluginMutePriority respectMutes) const;
	PLUGINDEX __cdecl GetActiveInstrumentPlugin(CHANNELINDEX, PluginMutePriority respectMutes) const;
	UINT GetBestMidiChan(const MODCHANNEL *pChn) const;

	void HandlePatternTransitionEvents();
	long GetSampleOffset();

public:
	PLUGINDEX GetBestPlugin(CHANNELINDEX nChn, PluginPriority priority, PluginMutePriority respectMutes) const;

// A couple of functions for handling backwards jumps and stuff to prevent infinite loops when counting the mod length or rendering to wav.
public:
	void InitializeVisitedRows(const bool bReset = true, VisitedRowsType *pRowVector = nullptr);
private:
	void SetRowVisited(const ORDERINDEX nOrd, const ROWINDEX nRow, const bool bVisited = true, VisitedRowsType *pRowVector = nullptr);
	bool IsRowVisited(const ORDERINDEX nOrd, const ROWINDEX nRow, const bool bAutoSet = true, VisitedRowsType *pRowVector = nullptr);
	size_t GetVisitedRowsVectorSize(const PATTERNINDEX nPat) const;

public:
	// "importance" of every FX command. Table is used for importing from formats with multiple effect columns
	// and is approximately the same as in SchismTracker.
	static size_t CSoundFile::GetEffectWeight(MODCOMMAND::COMMAND cmd);
	// try to convert a an effect into a volume column effect.
	static bool ConvertVolEffect(uint8 *e, uint8 *p, bool bForce);
};

#pragma warning(default : 4324) //structure was padded due to __declspec(align())


inline uint32 MODSAMPLE::GetSampleRate(const MODTYPE type) const
//--------------------------------------------------------------
{
	uint32 nRate;
	if(type & (MOD_TYPE_MOD|MOD_TYPE_XM))
		nRate = CSoundFile::TransposeToFrequency(RelativeTone, nFineTune);
	else
		nRate = nC5Speed;
	return (nRate > 0) ? nRate : 8363;
}


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
void ReadInstrumentExtensionField(MODINSTRUMENT* pIns, LPCBYTE& ptr, const int32 code, const int16 size);

// Read instrument property with 'code' from 'pData' to instrument 'pIns'.
void ReadExtendedInstrumentProperty(MODINSTRUMENT* pIns, const int32 code, LPCBYTE& pData, const LPCBYTE pEnd);

// Read extended instrument properties from 'pDataStart' to instrument 'pIns'.
void ReadExtendedInstrumentProperties(MODINSTRUMENT* pIns, const LPCBYTE pDataStart, const size_t nMemLength);

// Convert instrument flags which were read from 'dF..' extension to proper internal representation.
void ConvertReadExtendedFlags(MODINSTRUMENT* pIns);


#endif
