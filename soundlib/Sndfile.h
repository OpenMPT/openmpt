/*
 * This program is  free software; you can redistribute it  and modify it
 * under the terms of the GNU  General Public License as published by the
 * Free Software Foundation; either version 2  of the license or (at your
 * option) any later version.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

#ifndef __SNDFILE_H
#define __SNDFILE_H

#ifndef LPCBYTE
typedef const BYTE * LPCBYTE;
#endif

// -> CODE#0008
// -> DESC="#define to set pattern size"
#define MAX_PATTERN_ROWS	1024
// -! BEHAVIOUR_CHANGE#0008

#define MOD_AMIGAC2			0x1AB
// -> CODE#0006
// -> DESC="misc quantity changes"
#define MAX_SAMPLE_LENGTH	0x10000000	// 0x04000000 (64MB -> now 256MB)
// -! BEHAVIOUR_CHANGE#0006
#define MAX_SAMPLE_RATE		100000
#define MAX_ORDERS			256
#define MAX_PATTERNS		240
#define MAX_SAMPLES			4000
// -> CODE#0006
// -> DESC="misc quantity changes"
#define MAX_INSTRUMENTS		256	//200
// -! BEHAVIOUR_CHANGE#0006
#ifdef FASTSOUNDLIB
#define MAX_CHANNELS		80
#else
// -> CODE#0006
// -> DESC="misc quantity changes"
#define MAX_CHANNELS		256	//200
// -! BEHAVIOUR_CHANGE#0006
#endif
// -> CODE#0006
// -> DESC="misc quantity changes"
#ifdef FASTSOUNDLIB
#define MAX_BASECHANNELS	64
#else
#define MAX_BASECHANNELS	128	// Let 128 channels between MAX_BASECHANNELS & MAX_CHANNELS
#endif
// -! BEHAVIOUR_CHANGE#0006
#define MAX_ENVPOINTS		32
#define MIN_PERIOD			0x0020
#define MAX_PERIOD			0xFFFF
#define MAX_PATTERNNAME		32
#define MAX_CHANNELNAME		20
#define MAX_INFONAME		80
#define MAX_EQ_BANDS		6
// -> CODE#0006
// -> DESC="misc quantity changes"
#define MAX_MIXPLUGINS		100	//50
// -! BEHAVIOUR_CHANGE#0006
#define MAX_PLUGPRESETS		1000 //rewbs.plugPresets

#define MOD_TYPE_NONE		0x00
#define MOD_TYPE_MOD		0x01
#define MOD_TYPE_S3M		0x02
#define MOD_TYPE_XM			0x04
#define MOD_TYPE_MED		0x08
#define MOD_TYPE_MTM		0x10
#define MOD_TYPE_IT			0x20
#define MOD_TYPE_669		0x40
#define MOD_TYPE_ULT		0x80
#define MOD_TYPE_STM		0x100
#define MOD_TYPE_FAR		0x200
#define MOD_TYPE_WAV		0x400
#define MOD_TYPE_AMF		0x800
#define MOD_TYPE_AMS		0x1000
#define MOD_TYPE_DSM		0x2000
#define MOD_TYPE_MDL		0x4000
#define MOD_TYPE_OKT		0x8000
#define MOD_TYPE_MID		0x10000
#define MOD_TYPE_DMF		0x20000
#define MOD_TYPE_PTM		0x40000
#define MOD_TYPE_DBM		0x80000
#define MOD_TYPE_MT2		0x100000
#define MOD_TYPE_AMF0		0x200000
#define MOD_TYPE_PSM		0x400000
#define MOD_TYPE_J2B		0x800000
#define MOD_TYPE_UMX		0x80000000 // Fake type
#define MAX_MODTYPE			23



// Channel flags:
// Bits 0-7:	Sample Flags
#define CHN_16BIT			0x01
#define CHN_LOOP			0x02
#define CHN_PINGPONGLOOP	0x04
#define CHN_SUSTAINLOOP		0x08
#define CHN_PINGPONGSUSTAIN	0x10
#define CHN_PANNING			0x20
#define CHN_STEREO			0x40
#define CHN_PINGPONGFLAG	0x80
// Bits 8-31:	Channel Flags
#define CHN_MUTE			0x100
#define CHN_KEYOFF			0x200
#define CHN_NOTEFADE		0x400
#define CHN_SURROUND		0x800
#define CHN_NOIDO			0x1000
#define CHN_HQSRC			0x2000
#define CHN_FILTER			0x4000
#define CHN_VOLUMERAMP		0x8000
#define CHN_VIBRATO			0x10000
#define CHN_TREMOLO			0x20000
#define CHN_PANBRELLO		0x40000
#define CHN_PORTAMENTO		0x80000
#define CHN_GLISSANDO		0x100000
#define CHN_VOLENV			0x200000
#define CHN_PANENV			0x400000
#define CHN_PITCHENV		0x800000
#define CHN_FASTVOLRAMP		0x1000000
#define CHN_EXTRALOUD		0x2000000
#define CHN_REVERB			0x4000000
#define CHN_NOREVERB		0x8000000
// -> CODE#0012
// -> DESC="midi keyboard split"
#define CHN_SOLO			0x10000000
// -! NEW_FEATURE#0012

// -> CODE#0015
// -> DESC="channels management dlg"
#define CHN_NOFX			0x20000000
// -! NEW_FEATURE#0015


#define ENV_VOLUME			0x0001
#define ENV_VOLSUSTAIN		0x0002
#define ENV_VOLLOOP			0x0004
#define ENV_PANNING			0x0008
#define ENV_PANSUSTAIN		0x0010
#define ENV_PANLOOP			0x0020
#define ENV_PITCH			0x0040
#define ENV_PITCHSUSTAIN	0x0080
#define ENV_PITCHLOOP		0x0100
#define ENV_SETPANNING		0x0200
#define ENV_FILTER			0x0400
#define ENV_VOLCARRY		0x0800
#define ENV_PANCARRY		0x1000
#define ENV_PITCHCARRY		0x2000
#define ENV_MUTE			0x4000

#define CMD_NONE				0
#define CMD_ARPEGGIO			1
#define CMD_PORTAMENTOUP		2
#define CMD_PORTAMENTODOWN		3
#define CMD_TONEPORTAMENTO		4
#define CMD_VIBRATO				5
#define CMD_TONEPORTAVOL		6
#define CMD_VIBRATOVOL			7
#define CMD_TREMOLO				8
#define CMD_PANNING8			9
#define CMD_OFFSET				10
#define CMD_VOLUMESLIDE			11
#define CMD_POSITIONJUMP		12
#define CMD_VOLUME				13
#define CMD_PATTERNBREAK		14
#define CMD_RETRIG				15
#define CMD_SPEED				16
#define CMD_TEMPO				17
#define CMD_TREMOR				18
#define CMD_MODCMDEX			19
#define CMD_S3MCMDEX			20
#define CMD_CHANNELVOLUME		21
#define CMD_CHANNELVOLSLIDE		22
#define CMD_GLOBALVOLUME		23
#define CMD_GLOBALVOLSLIDE		24
#define CMD_KEYOFF				25
#define CMD_FINEVIBRATO			26
#define CMD_PANBRELLO			27
#define CMD_XFINEPORTAUPDOWN	28
#define CMD_PANNINGSLIDE		29
#define CMD_SETENVPOSITION		30
#define CMD_MIDI				31
#define CMD_SMOOTHMIDI			32 //rewbs.smoothVST
#define CMD_VELOCITY			33 //rewbs.velocity
// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
#define CMD_XPARAM				34
// -! NEW_FEATURE#0010

// Filter Modes
#define FLTMODE_LOWPASS			0
#define FLTMODE_HIGHPASS		1
#define FLTMODE_BANDPASS		2

// Volume Column commands
#define VOLCMD_VOLUME			1
#define VOLCMD_PANNING			2
#define VOLCMD_VOLSLIDEUP		3
#define VOLCMD_VOLSLIDEDOWN		4
#define VOLCMD_FINEVOLUP		5
#define VOLCMD_FINEVOLDOWN		6
#define VOLCMD_VIBRATOSPEED		7
#define VOLCMD_VIBRATO			8
#define VOLCMD_PANSLIDELEFT		9
#define VOLCMD_PANSLIDERIGHT	10
#define VOLCMD_TONEPORTAMENTO	11
#define VOLCMD_PORTAUP			12
#define VOLCMD_PORTADOWN		13
#define VOLCMD_VELOCITY			14 //rewbs.velocity
#define VOLCMD_OFFSET			15 //rewbs.volOff

#define RSF_16BIT		0x04
#define RSF_STEREO		0x08

#define RS_PCM8S		0	// 8-bit signed
#define RS_PCM8U		1	// 8-bit unsigned
#define RS_PCM8D		2	// 8-bit delta values
#define RS_ADPCM4		3	// 4-bit ADPCM-packed
#define RS_PCM16D		4	// 16-bit delta values
#define RS_PCM16S		5	// 16-bit signed
#define RS_PCM16U		6	// 16-bit unsigned
#define RS_PCM16M		7	// 16-bit motorola order
#define RS_STPCM8S		(RS_PCM8S|RSF_STEREO)	// stereo 8-bit signed
#define RS_STPCM8U		(RS_PCM8U|RSF_STEREO)	// stereo 8-bit unsigned
#define RS_STPCM8D		(RS_PCM8D|RSF_STEREO)	// stereo 8-bit delta values
#define RS_STPCM16S		(RS_PCM16S|RSF_STEREO)	// stereo 16-bit signed
#define RS_STPCM16U		(RS_PCM16U|RSF_STEREO)	// stereo 16-bit unsigned
#define RS_STPCM16D		(RS_PCM16D|RSF_STEREO)	// stereo 16-bit delta values
#define RS_STPCM16M		(RS_PCM16M|RSF_STEREO)	// stereo 16-bit signed big endian
// IT 2.14 compressed samples
#define RS_IT2148		0x10
#define RS_IT21416		0x14
#define RS_IT2158		0x12
#define RS_IT21516		0x16
// AMS Packed Samples
#define RS_AMS8			0x11
#define RS_AMS16		0x15
// DMF Huffman compression
#define RS_DMF8			0x13
#define RS_DMF16		0x17
// MDL Huffman compression
#define RS_MDL8			0x20
#define RS_MDL16		0x24
#define RS_PTM8DTO16	0x25
// Stereo Interleaved Samples
#define RS_STIPCM8S		(RS_PCM8S|0x40|RSF_STEREO)	// stereo 8-bit signed
#define RS_STIPCM8U		(RS_PCM8U|0x40|RSF_STEREO)	// stereo 8-bit unsigned
#define RS_STIPCM16S	(RS_PCM16S|0x40|RSF_STEREO)	// stereo 16-bit signed
#define RS_STIPCM16U	(RS_PCM16U|0x40|RSF_STEREO)	// stereo 16-bit unsigned
#define RS_STIPCM16M	(RS_PCM16M|0x40|RSF_STEREO)	// stereo 16-bit signed big endian
// 24-bit signed
#define RS_PCM24S		(RS_PCM16S|0x80)			// mono 24-bit signed
#define RS_STIPCM24S	(RS_PCM16S|0x80|RSF_STEREO)	// stereo 24-bit signed
#define RS_PCM32S		(RS_PCM16S|0xC0)			// mono 24-bit signed
#define RS_STIPCM32S	(RS_PCM16S|0xC0|RSF_STEREO)	// stereo 24-bit signed

// NNA types
#define NNA_NOTECUT		0
#define NNA_CONTINUE	1
#define NNA_NOTEOFF		2
#define NNA_NOTEFADE	3

// DCT types
#define DCT_NONE		0
#define DCT_NOTE		1
#define DCT_SAMPLE		2
#define DCT_INSTRUMENT	3
#define DCT_PLUGIN		4 //rewbs.VSTiNNA

// DNA types
#define DNA_NOTECUT		0
#define DNA_NOTEOFF		1
#define DNA_NOTEFADE	2

// Mixer Hardware-Dependent features
#define SYSMIX_ENABLEMMX	0x01
#define SYSMIX_SLOWCPU		0x02
#define SYSMIX_FASTCPU		0x04
#define SYSMIX_MMXEX		0x08
#define SYSMIX_3DNOW		0x10
#define SYSMIX_SSE			0x20

// Module flags
#define SONG_EMBEDMIDICFG	0x0001
#define SONG_FASTVOLSLIDES	0x0002
#define SONG_ITOLDEFFECTS	0x0004
#define SONG_ITCOMPATMODE	0x0008
#define SONG_LINEARSLIDES	0x0010
#define SONG_PATTERNLOOP	0x0020
#define SONG_STEP			0x0040
#define SONG_PAUSED			0x0080
#define SONG_FADINGSONG		0x0100
#define SONG_ENDREACHED		0x0200
#define SONG_GLOBALFADE		0x0400
#define SONG_CPUVERYHIGH	0x0800
#define SONG_FIRSTTICK		0x1000
#define SONG_MPTFILTERMODE	0x2000
#define SONG_SURROUNDPAN	0x4000
#define SONG_EXFILTERRANGE	0x8000
#define SONG_AMIGALIMITS	0x10000
// -> CODE#0023
// -> DESC="IT project files (.itp)"
#define SONG_ITPROJECT		0x20000
#define SONG_ITPEMBEDIH		0x40000
// -! NEW_FEATURE#0023

// Global Options (Renderer)
#define SNDMIX_REVERSESTEREO	0x0001
#define SNDMIX_NOISEREDUCTION	0x0002
#define SNDMIX_AGC				0x0004
#define SNDMIX_NORESAMPLING		0x0008
//      SNDMIX_NOLINEARSRCMODE is the default
//#define SNDMIX_HQRESAMPLER		0x0010	 //rewbs.resamplerConf: renamed SNDMIX_HQRESAMPLER to SNDMIX_SPLINESRCMODE
#define SNDMIX_SPLINESRCMODE	0x0010
#define SNDMIX_MEGABASS			0x0020
#define SNDMIX_SURROUND			0x0040
#define SNDMIX_REVERB			0x0080
#define SNDMIX_EQ				0x0100
#define SNDMIX_SOFTPANNING		0x0200
//#define SNDMIX_ULTRAHQSRCMODE	0x0400 	 //rewbs.resamplerConf: renamed SNDMIX_ULTRAHQSRCMODE to SNDMIX_POLYPHASESRCMODE
#define SNDMIX_POLYPHASESRCMODE	0x0400
#define SNDMIX_FIRFILTERSRCMODE	0x0800   //rewbs: added SNDMIX_FIRFILTERSRCMODE

//rewbs.resamplerConf: for stuff that applies to cubic spline, polyphase and FIR
#define SNDMIX_HQRESAMPLER (SNDMIX_SPLINESRCMODE|SNDMIX_POLYPHASESRCMODE|SNDMIX_FIRFILTERSRCMODE)

////rewbs.resamplerConf: for stuff that applies to polyphase and FIR
#define SNDMIX_ULTRAHQSRCMODE (SNDMIX_POLYPHASESRCMODE|SNDMIX_FIRFILTERSRCMODE)

// Misc Flags (can safely be turned on or off)
#define SNDMIX_DIRECTTODISK		0x10000
#define SNDMIX_ENABLEMMX		0x20000
#define SNDMIX_NOBACKWARDJUMPS	0x40000
#define SNDMIX_MAXDEFAULTPAN	0x80000	 // Used by the MOD loader
#define SNDMIX_MUTECHNMODE		0x100000 // Notes are not played on muted channels
#define SNDMIX_EMULATE_MIX_BUGS 0x200000 // rewbs.emulateMixBugs

// Resampling modes
enum {
	SRCMODE_NEAREST,
	SRCMODE_LINEAR,
	SRCMODE_SPLINE,
	SRCMODE_POLYPHASE,
	SRCMODE_FIRFILTER, //rewbs.resamplerConf
	NUM_SRC_MODES
};


// Sample Struct
typedef struct _MODINSTRUMENT
{
	UINT nLength,nLoopStart,nLoopEnd;
	UINT nSustainStart, nSustainEnd;
	LPSTR pSample;
	UINT nC4Speed;
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
	CHAR name[22];
} MODINSTRUMENT;


// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

/*---------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------
MODULAR STRUCT DECLARATIONS :
-----------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------*/

// Instrument Struct
typedef struct _INSTRUMENTHEADER
{
	UINT nFadeOut;
	DWORD dwFlags;
	UINT nGlobalVol;
	UINT nPan;
	UINT nVolEnv;
	UINT nPanEnv;
	UINT nPitchEnv;
	BYTE nVolLoopStart;
	BYTE nVolLoopEnd;
	BYTE nVolSustainBegin;
	BYTE nVolSustainEnd;
	BYTE nPanLoopStart;
	BYTE nPanLoopEnd;
	BYTE nPanSustainBegin;
	BYTE nPanSustainEnd;
	BYTE nPitchLoopStart;
	BYTE nPitchLoopEnd;
	BYTE nPitchSustainBegin;
	BYTE nPitchSustainEnd;
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
	signed char nPPS;
	unsigned char nPPC;
	WORD VolPoints[MAX_ENVPOINTS];
	WORD PanPoints[MAX_ENVPOINTS];
	WORD PitchPoints[MAX_ENVPOINTS];
	BYTE VolEnv[MAX_ENVPOINTS];
	BYTE PanEnv[MAX_ENVPOINTS];
	BYTE PitchEnv[MAX_ENVPOINTS];
	BYTE NoteMap[128];
	WORD Keyboard[128];
	CHAR name[32];
	CHAR filename[12];
	BYTE nMixPlug;							//rewbs.instroVSTi
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
	USHORT nVolRamp;
// -! NEW_FEATURE#0027

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// WHEN adding new members here, ALSO update Sndfile.cpp (instructions near the top of this file)!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

} INSTRUMENTHEADER;

// -----------------------------------------------------------------------------------------
// MODULAR INSTRUMENTHEADER FIELD ACCESS : body content at the (near) top of Sndfile.cpp !!!
// -----------------------------------------------------------------------------------------
extern void WriteInstrumentHeaderStruct(INSTRUMENTHEADER * input, FILE * file);
extern BYTE * GetInstrumentHeaderFieldPointer(INSTRUMENTHEADER * input, __int32 fcode, __int16 fsize);

// -! NEW_FEATURE#0027

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------


// Channel Struct
typedef struct _MODCHANNEL
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
	LONG nPeriod, nC4Speed, nPortamentoDest;
	INSTRUMENTHEADER *pHeader;
	MODINSTRUMENT *pInstrument;
	DWORD nVolEnvPosition, nPanEnvPosition, nPitchEnvPosition;
	DWORD nMasterChn, nVUMeter;
	LONG nGlobalVol, nInsVol;
	LONG nFineTune, nTranspose;
	LONG nPortamentoSlide, nAutoVibDepth;
	UINT nAutoVibPos, nVibratoPos, nTremoloPos, nPanbrelloPos;
	LONG nVolSwing, nPanSwing;
	// 8-bit members
	BYTE nNote, nNNA;
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
	BYTE nRetrigCount, nRetrigParam;
	BYTE nTremorCount, nTremorParam;
	BYTE nPatternLoop, nPatternLoopCount;
	BYTE nRowNote, nRowInstr;
	BYTE nRowVolCmd, nRowVolume;
	BYTE nRowCommand, nRowParam;
	BYTE nLeftVU, nRightVU;
	BYTE nActiveMacro, nFilterMode;

	float m_nPlugParamValueStep;  //rewbs.smoothVST 
	int m_nPlugInitialParamValue; //rewbs.smoothVST
} MODCHANNEL;


typedef struct _MODCHANNELSETTINGS
{
	UINT nPan;
	UINT nVolume;
	DWORD dwFlags;
	UINT nMixPlugin;
	CHAR szName[MAX_CHANNELNAME];
} MODCHANNELSETTINGS;


typedef struct _MODCOMMAND
{
	BYTE note;
	BYTE instr;
	BYTE volcmd;
	BYTE command;
	BYTE vol;
	BYTE param;
} MODCOMMAND, *LPMODCOMMAND;

////////////////////////////////////////////////////////////////////
// Mix Plugins
#define MIXPLUG_MIXREADY			0x01	// Set when cleared

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
	virtual void MidiCommand(UINT nMidiCh, UINT nMidiProg, WORD wMidiBank, UINT note, UINT vol, UINT trackChan) = 0;
	virtual void HardAllNotesOff() = 0;		//rewbs.VSTCompliance
	virtual bool isPlaying(UINT note, UINT midiChn, UINT trackerChn) = 0; //rewbs.VSTiNNA
	virtual bool MoveNote(UINT note, UINT midiChn, UINT sourceTrackerChn, UINT destTrackerChn) = 0; //rewbs.VSTiNNA
	virtual void SetZxxParameter(UINT nParam, UINT nValue) = 0;
	virtual UINT GetZxxParameter(UINT nParam) = 0; //rewbs.smoothVST 
	virtual long Dispatch(long opCode, long index, long value, void *ptr, float opt) =0; //rewbs.VSTCompliance
	virtual void NotifySongPlaying(bool)=0;	//rewbs.VSTCompliance
	virtual bool IsSongPlaying()=0;
	virtual bool IsResumed()=0;
	virtual void Resume()=0;
	virtual void Suspend()=0;
	virtual BOOL isInstrument()=0;
	virtual BOOL CanRecieveMidiEvents()=0;

};


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


typedef struct _SNDMIXPLUGINSTATE
{
	DWORD dwFlags;					// MIXPLUG_XXXX
	LONG nVolDecayL, nVolDecayR;	// Buffer click removal
	int *pMixBuffer;				// Stereo effect send buffer
	float *pOutBufferL;				// Temp storage for int -> float conversion
	float *pOutBufferR;
} SNDMIXPLUGINSTATE, *PSNDMIXPLUGINSTATE;

typedef struct _SNDMIXPLUGININFO
{
	DWORD dwPluginId1;
	DWORD dwPluginId2;
	DWORD dwInputRouting;	// MIXPLUG_INPUTF_XXXX
	DWORD dwOutputRouting;	// 0=mix 0x80+=fx
	DWORD dwReserved[4];	// Reserved for routing info
	CHAR szName[32];
	CHAR szLibraryName[64];	// original DLL name
} SNDMIXPLUGININFO, *PSNDMIXPLUGININFO; // Size should be 128 
										

typedef struct _SNDMIXPLUGIN
{
	IMixPlugin *pMixPlugin;
	PSNDMIXPLUGINSTATE pMixState;
	ULONG nPluginDataSize;
	PVOID pPluginData;
	SNDMIXPLUGININFO Info;
	float fDryRatio;		    // rewbs.dryRatio [20040123]
	long defaultProgram;		// rewbs.plugDefaultProgram
} SNDMIXPLUGIN, *PSNDMIXPLUGIN; // rewbs.dryRatio: Hopefully this doesn't need to be a fixed size.

typedef	BOOL (__cdecl *PMIXPLUGINCREATEPROC)(PSNDMIXPLUGIN);

typedef struct _SNDMIXSONGEQ
{
	ULONG nEQBands;
	ULONG EQFreq_Gains[MAX_EQ_BANDS];
} SNDMIXSONGEQ, *PSNDMIXSONGEQ;

////////////////////////////////////////////////////////////////////////
// Reverberation

typedef struct _SNDMIX_REVERB_PROPERTIES
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
} SNDMIX_REVERB_PROPERTIES, *PSNDMIX_REVERB_PROPERTIES;

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


typedef struct MODMIDICFG
{
	CHAR szMidiGlb[9*32];
	CHAR szMidiSFXExt[16*32];
	CHAR szMidiZXXExt[128*32];
} MODMIDICFG, *LPMODMIDICFG;


typedef VOID (__cdecl * LPSNDMIXHOOKPROC)(int *, unsigned long, unsigned long); // buffer, samples, channels



//==============
class CSoundFile
//==============
{
public:	// Static Members
	static UINT m_nXBassDepth, m_nXBassRange;
	static UINT m_nReverbDepth, gnReverbType;
	static UINT m_nProLogicDepth, m_nProLogicDelay;
	static UINT m_nStereoSeparation;
	static UINT m_nMaxMixChannels;
	static LONG m_nStreamVolume;
	static DWORD gdwSysInfo, gdwSoundSetup, gdwMixingFreq, gnBitsPerSample, gnChannels;
	static UINT gnAGC, gnVolumeRampSamples, gnCPUUsage;
	static LPSNDMIXHOOKPROC gpSndMixHook;
	static PMIXPLUGINCREATEPROC gpMixPluginCreateProc;

public:	// for Editing
	UINT m_nType, m_nChannels, m_nSamples, m_nInstruments;
	UINT m_nDefaultSpeed, m_nDefaultTempo, m_nDefaultGlobalVolume;
	DWORD m_dwSongFlags;							// Song flags SONG_XXXX
	UINT m_nMixChannels, m_nMixStat, m_nBufferCount;
	UINT m_nTickCount, m_nTotalCount, m_nPatternDelay, m_nFrameDelay;
	ULONG m_lTotalSampleCount;	// rewbs.VSTTimeInfo
	UINT m_nSamplesPerTick;	// rewbs.betterBPM
	UINT m_nMusicSpeed, m_nMusicTempo;
	UINT m_nNextRow, m_nRow;
	UINT m_nPattern,m_nCurrentPattern,m_nNextPattern,m_nRestartPos, m_nSeqOverride;
	UINT m_nMasterVolume, m_nGlobalVolume, m_nSongPreAmp;
	UINT m_nFreqFactor, m_nTempoFactor, m_nOldGlbVolSlide;
	LONG m_nMinPeriod, m_nMaxPeriod, m_nRepeatCount;
	DWORD m_nGlobalFadeSamples, m_nGlobalFadeMaxSamples;
	UINT m_nMaxOrderPosition, m_nPatternNames;
	LPSTR m_lpszSongComments, m_lpszPatternNames;
	UINT ChnMix[MAX_CHANNELS];						// Channels to be mixed
	MODCHANNEL Chn[MAX_CHANNELS];					// Channels
	MODCHANNELSETTINGS ChnSettings[MAX_BASECHANNELS]; // Channels settings
	MODCOMMAND *Patterns[MAX_PATTERNS];				// Patterns
	UINT PatternSize[MAX_PATTERNS];					// Patterns Lengths
	BYTE Order[MAX_ORDERS];							// Pattern Orders
	MODINSTRUMENT Ins[MAX_SAMPLES];					// Instruments
	INSTRUMENTHEADER *Headers[MAX_INSTRUMENTS];		// Instrument Headers
	CHAR m_szNames[MAX_SAMPLES][32];				// Song and sample names
	MODMIDICFG m_MidiCfg;							// Midi macro config table
	SNDMIXPLUGIN m_MixPlugins[MAX_MIXPLUGINS];		// Mix plugins
	SNDMIXSONGEQ m_SongEQ;							// Default song EQ preset
	CHAR CompressionTable[16];

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	CHAR m_szInstrumentPath[MAX_INSTRUMENTS][_MAX_PATH];
	BOOL instrumentModified[MAX_INSTRUMENTS];
// -! NEW_FEATURE#0023

public:
	CSoundFile();
	~CSoundFile();

public:
	BOOL Create(LPCBYTE lpStream, DWORD dwMemLength=0);
	BOOL Destroy();
	UINT GetType() const { return m_nType; }
	UINT GetNumChannels() const;
	UINT GetLogicalChannels() const { return m_nChannels; }
	BOOL SetMasterVolume(UINT vol, BOOL bAdjustAGC=FALSE);
	UINT GetMasterVolume() const { return m_nMasterVolume; }
	UINT GetNumPatterns() const;
	UINT GetNumInstruments() const;
	UINT GetNumSamples() const { return m_nSamples; }
	UINT GetCurrentPos() const;
	UINT GetCurrentPattern() const { return m_nPattern; }
	UINT GetCurrentOrder() const { return m_nCurrentPattern; }
	UINT GetSongComments(LPSTR s, UINT cbsize, UINT linesize=32);
	UINT GetRawSongComments(LPSTR s, UINT cbsize, UINT linesize=32);
	UINT GetMaxPosition() const;
	int FindOrder(BYTE pat);					//rewbs.playSongFromCursor
	void DontLoopPattern(int nPat, int nRow=0); //rewbs.playSongFromCursor
	void SetCurrentPos(UINT nPos);
	void SetCurrentOrder(UINT nOrder);
	void GetTitle(LPSTR s) const { lstrcpyn(s,m_szNames[0],32); }
	LPCSTR GetTitle() const { return m_szNames[0]; }
	UINT GetSampleName(UINT nSample,LPSTR s=NULL) const;
	UINT GetInstrumentName(UINT nInstr,LPSTR s=NULL) const;
	UINT GetMusicSpeed() const { return m_nMusicSpeed; }
	UINT GetMusicTempo() const { return m_nMusicTempo; }
	DWORD GetLength(BOOL bAdjust, BOOL bTotal=FALSE);
	DWORD GetSongTime() { return GetLength(FALSE, TRUE); }
	void SetRepeatCount(int n) { m_nRepeatCount = n; }
	int GetRepeatCount() const { return m_nRepeatCount; }
	BOOL IsPaused() const {	return (m_dwSongFlags & SONG_PAUSED) ? TRUE : FALSE; }
	void LoopPattern(int nPat, int nRow=0);
	void CheckCPUUsage(UINT nCPU);
	BOOL SetPatternName(UINT nPat, LPCSTR lpszName);
	BOOL GetPatternName(UINT nPat, LPSTR lpszName, UINT cbSize=MAX_PATTERNNAME) const;
	// Module Loaders
	BOOL ReadXM(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadS3M(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadMod(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadMed(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadMTM(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadSTM(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadIT(LPCBYTE lpStream, DWORD dwMemLength);
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	BOOL ReadITProject(LPCBYTE lpStream, DWORD dwMemLength);
// -! NEW_FEATURE#0023
	BOOL Read669(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadUlt(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadWav(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadDSM(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadFAR(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadAMS(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadAMS2(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadMDL(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadOKT(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadDMF(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadPTM(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadDBM(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadAMF(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadMT2(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadPSM(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadJ2B(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadUMX(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadMID(LPCBYTE lpStream, DWORD dwMemLength);
	// Save Functions
#ifndef MODPLUG_NO_FILESAVE
	UINT WriteSample(FILE *f, MODINSTRUMENT *pins, UINT nFlags, UINT nMaxLen=0);
	BOOL SaveXM(LPCSTR lpszFileName, UINT nPacking=0);
	BOOL SaveS3M(LPCSTR lpszFileName, UINT nPacking=0);
	BOOL SaveMod(LPCSTR lpszFileName, UINT nPacking=0);
	BOOL SaveIT(LPCSTR lpszFileName, UINT nPacking=0);
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	BOOL SaveITProject(LPCSTR lpszFileName);
// -! NEW_FEATURE#0023
	UINT SaveMixPlugins(FILE *f=NULL, BOOL bUpdate=TRUE);
#endif // MODPLUG_NO_FILESAVE
	// MOD Convert function
	UINT GetBestSaveFormat() const;
	UINT GetSaveFormats() const;
	void ConvertModCommand(MODCOMMAND *) const;
	void S3MConvert(MODCOMMAND *m, BOOL bIT) const;
	void S3MSaveConvert(UINT *pcmd, UINT *pprm, BOOL bIT) const;
	WORD ModSaveCommand(const MODCOMMAND *m, BOOL bXM) const;
	
public:
	// Real-time sound functions
	VOID SuspendPlugins(); //rewbs.VSTCompliance
	VOID ResumePlugins();  //rewbs.VSTCompliance
	VOID StopAllVsti();    //rewbs.VSTCompliance
	VOID ResetChannels();
	UINT Read(LPVOID lpBuffer, UINT cbBuffer);
	UINT ReadMix(LPVOID lpBuffer, UINT cbBuffer, CSoundFile *, DWORD *, LPBYTE ps=NULL);
	UINT CreateStereoMix(int count);
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
	static void EQStereo(int *pbuffer, UINT nCount);
	static void EQMono(int *pbuffer, UINT nCount);
#endif
	// Analyzer Functions
	static UINT WaveConvert(LPBYTE lpSrc, signed char *lpDest, UINT nSamples);
	static UINT WaveStereoConvert(LPBYTE lpSrc, signed char *lpDest, UINT nSamples);
	static LONG SpectrumAnalyzer(signed char *pBuffer, UINT nSamples, UINT nInc, UINT nChannels);
	// Float <-> Int conversion routines
	static VOID StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount);
	static VOID FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount);
	static VOID MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount);
	static VOID FloatToMonoMix(const float *pIn, int *pOut, UINT nCount);

public:
	BOOL ReadNote();
	BOOL ProcessRow();
	BOOL ProcessEffects();
	UINT GetNNAChannel(UINT nChn) const;
	void CheckNNA(UINT nChn, UINT instr, int note, BOOL bForceCut);
	void NoteChange(UINT nChn, int note, BOOL bPorta=FALSE, BOOL bResetEnv=TRUE, BOOL bManual=FALSE);
	void InstrumentChange(MODCHANNEL *pChn, UINT instr, BOOL bPorta=FALSE,BOOL bUpdVol=TRUE,BOOL bResetEnv=TRUE);
	// Channel Effects
	void PortamentoUp(MODCHANNEL *pChn, UINT param);
	void PortamentoDown(MODCHANNEL *pChn, UINT param);
	void FinePortamentoUp(MODCHANNEL *pChn, UINT param);
	void FinePortamentoDown(MODCHANNEL *pChn, UINT param);
	void ExtraFinePortamentoUp(MODCHANNEL *pChn, UINT param);
	void ExtraFinePortamentoDown(MODCHANNEL *pChn, UINT param);
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
	void KeyOff(UINT nChn);
	int PatternLoop(MODCHANNEL *, UINT param);
	void ExtendedMODCommands(UINT nChn, UINT param);
	void ExtendedS3MCommands(UINT nChn, UINT param);
	void ExtendedChannelEffect(MODCHANNEL *, UINT param);
	void ProcessMidiMacro(UINT nChn, LPCSTR pszMidiMacro, UINT param=0);
	void ProcessSmoothMidiMacro(UINT nChn, LPCSTR pszMidiMacro, UINT param=0); //rewbs.smoothVST
	void SetupChannelFilter(MODCHANNEL *pChn, BOOL bReset, int flt_modifier=256) const;
	// Low-Level effect processing
	void DoFreqSlide(MODCHANNEL *pChn, LONG nFreqSlide);
	// Global Effects
	void SetTempo(UINT param);
	void SetSpeed(UINT param);
	void GlobalVolSlide(UINT param);
	DWORD IsSongFinished(UINT nOrder, UINT nRow) const;
	BOOL IsValidBackwardJump(UINT nStartOrder, UINT nStartRow, UINT nJumpOrder, UINT nJumpRow) const;
	// Read/Write sample functions
	char GetDeltaValue(char prev, UINT n) const { return (char)(prev + CompressionTable[n & 0x0F]); }
	UINT PackSample(int &sample, int next);
	BOOL CanPackSample(LPSTR pSample, UINT nLen, UINT nPacking, BYTE *result=NULL);
	UINT ReadSample(MODINSTRUMENT *pIns, UINT nFlags, LPCSTR pMemFile, DWORD dwMemLength);
	BOOL DestroySample(UINT nSample);

// -> CODE#0020
// -> DESC="rearrange sample list"
	BOOL MoveSample(UINT from,UINT to);
// -! NEW_FEATURE#0020

// -> CODE#0003
// -> DESC="remove instrument's samples"
	//BOOL DestroyInstrument(UINT nInstr);
	BOOL DestroyInstrument(UINT nInstr, char removeSamples = 0);
// -! BEHAVIOUR_CHANGE#0003
	BOOL IsSampleUsed(UINT nSample);
	BOOL IsInstrumentUsed(UINT nInstr);
	BOOL RemoveInstrumentSamples(UINT nInstr);
	UINT DetectUnusedSamples(BYTE *); // bitmask
	BOOL RemoveSelectedSamples(BOOL *);
	void AdjustSampleLoop(MODINSTRUMENT *pIns);
	// Samples file I/O
	BOOL ReadSampleFromFile(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength);
	BOOL ReadWAVSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD *pdwWSMPOffset=NULL);
	BOOL ReadPATSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength);
	BOOL ReadS3ISample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength);
	BOOL ReadAIFFSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength);
	BOOL ReadXISample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength);

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
//	BOOL ReadITSSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD dwOffset=0);
	UINT ReadITSSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD dwOffset=0);
// -! NEW_FEATURE#0027

	BOOL ReadITISample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength);
	BOOL Read8SVXSample(UINT nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	BOOL SaveWAVSample(UINT nSample, LPCSTR lpszFileName);
	// Instrument file I/O
	BOOL ReadInstrumentFromFile(UINT nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	BOOL ReadXIInstrument(UINT nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	BOOL ReadITIInstrument(UINT nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	BOOL ReadPATInstrument(UINT nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	BOOL ReadSampleAsInstrument(UINT nInstr, LPBYTE lpMemFile, DWORD dwFileLength);
	BOOL SaveXIInstrument(UINT nInstr, LPCSTR lpszFileName);
	BOOL SaveITIInstrument(UINT nInstr, LPCSTR lpszFileName);
	// I/O from another sound file
	BOOL ReadInstrumentFromSong(UINT nInstr, CSoundFile *, UINT nSrcInstrument);
	BOOL ReadSampleFromSong(UINT nSample, CSoundFile *, UINT nSrcSample);
	// Period/Note functions
	UINT GetNoteFromPeriod(UINT period) const;
	UINT GetPeriodFromNote(UINT note, int nFineTune, UINT nC4Speed) const;
	UINT GetFreqFromPeriod(UINT period, UINT nC4Speed, int nPeriodFrac=0) const;
	// Misc functions
	MODINSTRUMENT *GetSample(UINT n) { return Ins+n; }
	void ResetMidiCfg();
	UINT MapMidiInstrument(DWORD dwProgram, UINT nChannel, UINT nNote);
	long ITInstrToMPT(const void *p, INSTRUMENTHEADER *penv, UINT trkvers); //change from BOOL for rewbs.modularInstData
	UINT LoadMixPlugins(const void *pData, UINT nLen);
#ifndef NO_FILTER
	DWORD CutOffToFrequency(UINT nCutOff, int flt_modifier=256) const; // [0-255] => [1-10KHz]
#endif
#ifdef MODPLUG_TRACKER
	VOID ProcessMidiOut(UINT nChn, MODCHANNEL *pChn);		//rewbs.VSTdelay : added arg.
#endif

	// Static helper functions
public:
	static DWORD TransposeToFrequency(int transp, int ftune=0);
	static int FrequencyToTranspose(DWORD freq);
	static void FrequencyToTranspose(MODINSTRUMENT *psmp);

	// System-Dependant functions
public:
	static MODCOMMAND *AllocatePattern(UINT rows, UINT nchns);
	static LPSTR AllocateSample(UINT nbytes);
	static void FreePattern(LPVOID pat);
	static void FreeSample(LPVOID p);
	static UINT Normalize24BitBuffer(LPBYTE pbuffer, UINT cbsizebytes, DWORD lmax24, DWORD dwByteInc);
};


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
#define MIXING_CLIPMIN		(-0x07FFFFFF)
#define MIXING_CLIPMAX		(0x07FFFFFF)
#define VOLUMERAMPPRECISION	12				//rewbs.soundq exp (was 12)
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


#endif
