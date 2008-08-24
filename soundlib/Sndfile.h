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

#include "../mptrack/SoundFilePlayConfig.h"
#include "tuning.h"
#include "modspecifications.h"
#include <vector>
#include <bitset>
#include "midi.h"

using std::bitset;


#ifndef LPCBYTE
	typedef const BYTE * LPCBYTE;
#endif

#define MOD_AMIGAC2			0x1AB
#define MAX_INFONAME		80

#define MAX_MIXPLUGINS		100	//50
#define MAX_PLUGPRESETS		1000 //rewbs.plugPresets
#define MAX_INSTRUMENTS		256	//200
#define MAX_SAMPLES			4000
#define MAX_PATTERNS		240 //Old maximum patterncount.
#define MAX_ORDERS			256	//Old maximum ordercount.

#ifdef FASTSOUNDLIB
	#define MAX_CHANNELS		80
#else
	#define MAX_CHANNELS		256	//200 //Note: MAX_BASECHANNELS defines max pattern channels
#endif
#ifdef FASTSOUNDLIB
	#define MAX_BASECHANNELS	64
#else
	#define MAX_BASECHANNELS	127	//Max pattern channels.
#endif



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
#define SONG_ITPROJECT		0x20000 //.itp
#define SONG_ITPEMBEDIH		0x40000 //.itp


#include "mixer.h"
#include "modsmp.h"
#include "modins.h"
#include "modchannel.h"
#include "modcommand.h"
#include "mixplug.h"
#include "pattern.h"
#include "patternContainer.h"
#include "ordertopatterntable.h"
#include "playbackEventer.h"

// Modular instrumentheader field access.
extern void WriteInstrumentHeaderStruct(INSTRUMENTHEADER * input, FILE * file);
extern BYTE * GetInstrumentHeaderFieldPointer(INSTRUMENTHEADER * input, __int32 fcode, __int16 fsize);


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


using ModSpecs::CModSpecifications;

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
	
	//Return value in seconds.
	double GetPlaybackTimeAt(ORDERINDEX, ROWINDEX);

	uint16 GetModFlags() const {return m_ModFlags;}
	void SetModFlags(const uint16 v) {m_ModFlags = v;}
	bool GetModFlag(BYTE i) const {return ((m_ModFlags & (1<<i)) != 0);}
	void SetModFlag(BYTE i, bool val) {if(i < 8*sizeof(m_ModFlags)) {m_ModFlags = (val) ? m_ModFlags |= (1 << i) : m_ModFlags &= ~(1 << i);}}
	
	//Tuning-->
public:
	static bool LoadStaticTunings();
	static bool SaveStaticTunings();
	static void DeleteStaticdata();
	static CTuningCollection& GetStandardTunings() {return *s_pTuningsSharedStandard;}
	static CTuningCollection& GetLocalTunings() {return *s_pTuningsSharedLocal;}
	CTuningCollection& GetTuneSpecificTunings() {return *m_pTuningsTuneSpecific;}

	string GetNoteName(const CTuning::NOTEINDEXTYPE&, const int inst = -1) const;
private:
	CTuningCollection* m_pTuningsTuneSpecific;
	static CTuningCollection* s_pTuningsSharedStandard;
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



public:	// for Editing
	CModDoc* m_pModDoc;
	UINT m_nType, m_nChannels, m_nSamples, m_nInstruments;
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
	//Note: m_nCurrentPattern and m_nNextPattern refer to orderindex - not patternindex.
	bool m_bPatternTransitionOccurred;
	UINT m_nMasterVolume, m_nGlobalVolume, m_nSamplesToGlobalVolRampDest,
		 m_nGlobalVolumeDestination, m_nSamplePreAmp, m_nVSTiVolume;
	long m_lHighResRampingGlobalVolume;
	UINT m_nFreqFactor, m_nTempoFactor, m_nOldGlbVolSlide;
	LONG m_nMinPeriod, m_nMaxPeriod, m_nRepeatCount;
	DWORD m_nGlobalFadeSamples, m_nGlobalFadeMaxSamples;
	UINT m_nMaxOrderPosition, m_nPatternNames;
	LPSTR m_lpszSongComments, m_lpszPatternNames;
	UINT ChnMix[MAX_CHANNELS];						// Channels to be mixed
	MODCHANNEL Chn[MAX_CHANNELS];					// Channels
	MODCHANNELSETTINGS ChnSettings[MAX_BASECHANNELS]; // Channels settings
	CPatternContainer Patterns;						//Patterns
	CPatternSizesMimic PatternSize;					// Mimics old PatternsSize-array(is read-only).
	COrderToPatternTable Order;						// Order[x] gives the index of the pattern located at order x.
	MODINSTRUMENT Ins[MAX_SAMPLES];					// Instruments
	INSTRUMENTHEADER *Headers[MAX_INSTRUMENTS];		// Instrument Headers
	INSTRUMENTHEADER m_defaultInstrument;			// Currently only used to get default values for extented properties. 
	CHAR m_szNames[MAX_SAMPLES][32];				// Song and sample names
	MODMIDICFG m_MidiCfg;							// Midi macro config table
	SNDMIXPLUGIN m_MixPlugins[MAX_MIXPLUGINS];		// Mix plugins
	SNDMIXSONGEQ m_SongEQ;							// Default song EQ preset
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
	UINT GetCurrentOrder() const { return m_nCurrentPattern; }
	UINT GetSongComments(LPSTR s, UINT cbsize, UINT linesize=32);
	UINT GetRawSongComments(LPSTR s, UINT cbsize, UINT linesize=32);
	UINT GetMaxPosition() const;

	IMixPlugin* GetInstrumentPlugin(INSTRUMENTINDEX instr);
	const CModSpecifications& GetModSpecifications() const {return *m_pModSpecs;}
	static const CModSpecifications& GetModSpecifications(const MODTYPE type);

	double GetCurrentBPM() const;
	int FindOrder(PATTERNINDEX pat, UINT startFromOrder=0, bool direction=true);	//rewbs.playSongFromCursor
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
	bool MoveChannel(UINT chn_from, UINT chn_to);

	bool InitChannel(UINT nch);
	void ResetChannelState(CHANNELINDEX chn, BYTE resetStyle);

	// Module Loaders
	BOOL ReadXM(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadS3M(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadMod(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadMed(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadMTM(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadSTM(LPCBYTE lpStream, DWORD dwMemLength);
	BOOL ReadIT(LPCBYTE lpStream, const DWORD dwMemLength);
	//BOOL ReadMPT(LPCBYTE lpStream, const DWORD dwMemLength);
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	BOOL ReadITProject(LPCBYTE lpStream, const DWORD dwMemLength);
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
	BOOL SaveCompatIT(LPCSTR lpszFileName);
	BOOL SaveCompatXM(LPCSTR lpszFileName);
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	BOOL SaveITProject(LPCSTR lpszFileName);
// -! NEW_FEATURE#0023
	UINT SaveMixPlugins(FILE *f=NULL, BOOL bUpdate=TRUE);
	void WriteInstrumentPropertyForAllInstruments(__int32 code,  __int16 size, FILE* f, INSTRUMENTHEADER* instruments[], UINT nInstruments);
	void SaveExtendedInstrumentProperties(INSTRUMENTHEADER *instruments[], UINT nInstruments, FILE* f);
	void SaveExtendedSongProperties(FILE* f);
	void LoadExtendedSongProperties(const MODTYPE modtype, BYTE*& ptr, const BYTE* startpos, const size_t seachlimit);

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
	void NoteChange(UINT nChn, int note, BOOL bPorta=FALSE, BOOL bResetEnv=TRUE, BOOL bManual=FALSE);
	void InstrumentChange(MODCHANNEL *pChn, UINT instr, BOOL bPorta=FALSE,BOOL bUpdVol=TRUE,BOOL bResetEnv=TRUE);
	// Channel Effects
	void PortamentoUp(MODCHANNEL *pChn, UINT param, const bool fineAsRegular = false);
	void PortamentoDown(MODCHANNEL *pChn, UINT param, const bool fineAsRegular = false);
	void MidiPortamento(MODCHANNEL *pChn, int param);
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
	void SetTempo(UINT param, bool setAsNonModcommand = false);
	void SetSpeed(UINT param);
	void GlobalVolSlide(UINT param);
	DWORD IsSongFinished(UINT nOrder, UINT nRow) const;
	BOOL IsValidBackwardJump(UINT nStartOrder, UINT nStartRow, UINT nJumpOrder, UINT nJumpRow) const;
	// Read/Write sample functions
	char GetDeltaValue(char prev, UINT n) const { return (char)(prev + CompressionTable[n & 0x0F]); }
	UINT PackSample(int &sample, int next);
	BOOL CanPackSample(LPSTR pSample, UINT nLen, UINT nPacking, BYTE *result=NULL);
	UINT ReadSample(MODINSTRUMENT *pIns, UINT nFlags, LPCSTR pMemFile, DWORD dwMemLength, const WORD format = 1);
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
	static void FrequencyToTranspose(MODINSTRUMENT *psmp);

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
	int getVolEnvValueFromPosition(int position, INSTRUMENTHEADER* penv);
    void resetEnvelopes(MODCHANNEL* pChn, int envToReset = ENV_RESET_ALL);
	void SetDefaultInstrumentValues(INSTRUMENTHEADER *penv);
private:
	UINT  __cdecl GetChannelPlugin(UINT nChan, bool respectMutes);
	UINT  __cdecl GetActiveInstrumentPlugin(UINT nChan, bool respectMutes);
	UINT GetBestMidiChan(MODCHANNEL *pChn);

	void HandlePatternTransitionEvents();
	void BuildDefaultInstrument();
	long GetSampleOffset();
};


inline IMixPlugin* CSoundFile::GetInstrumentPlugin(INSTRUMENTINDEX instr)
//----------------------------------------------------------------
{
	if(instr > 0 && instr < MAX_INSTRUMENTS && Headers[instr] && Headers[instr]->nMixPlug && Headers[instr]->nMixPlug <= MAX_MIXPLUGINS)
		return m_MixPlugins[Headers[instr]->nMixPlug-1].pMixPlugin;
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
