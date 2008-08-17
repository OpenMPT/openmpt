/*
 * This program is  free software; you can redistribute it  and modify it
 * under the terms of the GNU  General Public License as published by the
 * Free Software Foundation; either version 2  of the license or (at your
 * option) any later version.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

#include "stdafx.h"
#include "../mptrack/mptrack.h"
#include "../mptrack/mainfrm.h"
#include "../mptrack/moddoc.h"
#include "../mptrack/version.h"
#include "sndfile.h"
#include "aeffectx.h"
#include "wavConverter.h"
#include "tuningcollection.h"
#include <vector>
#include <algorithm>


#ifndef NO_COPYRIGHT
#ifndef NO_MMCMP_SUPPORT
#define MMCMP_SUPPORT
#endif // NO_MMCMP_SUPPORT
#ifndef NO_ARCHIVE_SUPPORT
#define UNRAR_SUPPORT
#define UNLHA_SUPPORT
#define ZIPPED_MOD_SUPPORT
LPCSTR glpszModExtensions = "mod|s3m|xm|it|stm|nst|ult|669|wow|mtm|med|far|mdl|ams|dsm|amf|okt|dmf|ptm|psm|mt2|umx";
//Should there be mptm?
#endif // NO_ARCHIVE_SUPPORT
#else // NO_COPYRIGHT: EarSaver only loads mod/s3m/xm/it/wav
#define MODPLUG_BASIC_SUPPORT
#endif

#ifdef ZIPPED_MOD_SUPPORT
#include "unzip32.h"
#endif

#ifdef UNRAR_SUPPORT
#include "unrar32.h"
#endif

#ifdef UNLHA_SUPPORT
#include "../unlha/unlha32.h"
#endif

#ifdef MMCMP_SUPPORT
extern BOOL MMCMP_Unpack(LPCBYTE *ppMemFile, LPDWORD pdwMemLength);
#endif

// External decompressors
extern void AMSUnpack(const char *psrc, UINT inputlen, char *pdest, UINT dmax, char packcharacter);
extern WORD MDLReadBits(DWORD &bitbuf, UINT &bitnum, LPBYTE &ibuf, CHAR n);
extern int DMFUnpack(LPBYTE psample, LPBYTE ibuf, LPBYTE ibufmax, UINT maxlen);
extern DWORD ITReadBits(DWORD &bitbuf, UINT &bitnum, LPBYTE &ibuf, CHAR n);
extern void ITUnpack8Bit(LPSTR pSample, DWORD dwLen, LPBYTE lpMemFile, DWORD dwMemLength, BOOL b215);
extern void ITUnpack16Bit(LPSTR pSample, DWORD dwLen, LPBYTE lpMemFile, DWORD dwMemLength, BOOL b215);


#define MAX_PACK_TABLES		3


// Compression table
static char UnpackTable[MAX_PACK_TABLES][16] = 
//--------------------------------------------
{
	// CPU-generated dynamic table
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	// u-Law table
	0, 1, 2, 4, 8, 16, 32, 64,
	-1, -2, -4, -8, -16, -32, -48, -64,
	// Linear table
	0, 1, 2, 3, 5, 7, 12, 19,
	-1, -2, -3, -5, -7, -12, -19, -31,
};

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

/*---------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------
MODULAR (in/out) INSTRUMENTHEADER :
-----------------------------------------------------------------------------------------------

* to update:
------------

- both following functions need to be updated when adding a new member in INSTRUMENTHEADER :

void WriteInstrumentHeaderStruct(INSTRUMENTHEADER * input, FILE * file);
BYTE * GetInstrumentHeaderFieldPointer(INSTRUMENTHEADER * input, __int32 fcode, __int16 fsize);

- see below for body declaration.


* members:
----------

- 32bit identification CODE tag (must be unique)
- 16bit content SIZE in byte(s)
- member field


* CODE tag naming convention:
-----------------------------

- have a look below in current tag dictionnary
- take the initial ones of the field name
- 4 caracters code (not more, not less)
- must be filled with '.' caracters if code has less than 4 caracters
- for arrays, must include a '[' caracter following significant caracters ('.' not significant!!!)
- use only caracters used in full member name, ordered as they appear in it
- match caracter attribute (small,capital)

Exemple with "nPanLoopEnd" , "nPitchLoopEnd" & "VolEnv[MAX_ENVPOINTS]" members : 
- use 'PLE.' for nPanLoopEnd
- use 'PiLE' for nPitchLoopEnd
- use 'VE[.' for VolEnv[MAX_ENVPOINTS]


* In use CODE tag dictionnary (alphabetical order): [ see in Sndfile.cpp ]
--------------------------------------------------------------------------

						!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						!!! SECTION TO BE UPDATED !!!
						!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		[EXT]	means external (not related) to INSTRUMENTHEADER content

C...	[EXT]	nChannels
CS..			nCutSwing
CWV.	[EXT]	dwCreatedWithVersion
DCT.			nDCT;
dF..			dwFlags;
DGV.	[EXT]	nDefaultGlobalVolume
DT..	[EXT]	nDefaultTempo;			
DNA.			nDNA;
EBIH	[EXT]	embeded instrument header tag (ITP file format)
FM..			nFilterMode;
fn[.			filename[12];
FO..			nFadeOut;
GV..			nGlobalVol;
IFC.			nIFC;
IFR.			nIFR;
K[.				Keyboard[128];
LSWV	[EXT]	nPlugMixMode
MB..			wMidiBank;
MC..			nMidiChannel;
MDK.			nMidiDrumKey;
MIMA	[EXT]									MIdi MApping directives
MiP.			nMixPlug;
MP..			nMidiProgram;
MPTS	[EXT]									Extra song info tag
MPTX	[EXT]									EXTRA INFO tag
MSF.	[EXT]									Mod(Specific)Flags
n[..			name[32];
NNA.			nNNA;
NM[.			NoteMap[128];
P...			nPan;
PE..			nPanEnv;
PE[.			PanEnv[MAX_ENVPOINTS];
PiE.			nPitchEnv;
PiE[			PitchEnv[MAX_ENVPOINTS];
PiLE			nPitchLoopEnd;
PiLS			nPitchLoopStart;
PiP[			PitchPoints[MAX_ENVPOINTS];
PiSB			nPitchSustainBegin;
PiSE			nPitchSustainEnd;
PLE.			nPanLoopEnd;
PLS.			nPanLoopStart;
PMM.	[EXT]	nPlugMixMode;
PP[.			PanPoints[MAX_ENVPOINTS];
PPC.			nPPC;
PPS.			nPPS;
PS..			nPanSwing;
PSB.			nPanSustainBegin;
PSE.			nPanSustainEnd;
PTTL			wPitchToTempoLock;
PVEH			nPluginVelocityHandling;
PVOH			nPluginVolumeHandling;
R...			nResampling;
RP..	[EXT]	nRestartPos;
RPB.	[EXT]	nRowsPerBeat;
RPM.	[EXT]	nRowsPerMeasure;
RS..			nResSwing;
SEP@	[EXT]									chunk SEPARATOR tag
SPA.	[EXT]	m_nSamplePreAmp;
TM..	[EXT]	nTempoMode;
VE..			nVolEnv;
VE[.			VolEnv[MAX_ENVPOINTS];
VLE.			nVolLoopEnd;
VLS.			nVolLoopStart;
VP[.			VolPoints[MAX_ENVPOINTS];
VR..			nVolRamp;
VS..			nVolSwing;
VSB.			nVolSustainBegin;
VSE.			nVolSustainEnd;
VSTV	[EXT]	nVSTiVolume;
PERN			nPitchEnvReleaseNode
AERN			nPanEnvReleaseNode
VERN			nVolEnvReleaseNode
-----------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------*/

// --------------------------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for single type members ONLY (non-array)
// --------------------------------------------------------------------------------------------
#define WRITE_MPTHEADER_sized_member(name,type,code) \
fcode = #@code;\
fwrite(& fcode , 1 , sizeof( __int32 ) , file);\
fsize = sizeof( type );\
fwrite(& fsize , 1 , sizeof( __int16 ) , file);\
fwrite(&input-> name , 1 , fsize , file);

// --------------------------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for array members ONLY
// --------------------------------------------------------------------------------------------
#define WRITE_MPTHEADER_array_member(name,type,code,arraysize) \
fcode = #@code;\
fwrite(& fcode , 1 , sizeof( __int32 ) , file);\
fsize = sizeof( type ) * arraysize;\
fwrite(& fsize , 1 , sizeof( __int16 ) , file);\
fwrite(&input-> name , 1 , fsize , file);

// Write (in 'file') 'input' INSTRUMENTHEADER with 'code' & 'size' extra field infos for each member
void WriteInstrumentHeaderStruct(INSTRUMENTHEADER * input, FILE * file)
{
__int32 fcode;
__int16 fsize;
WRITE_MPTHEADER_sized_member(	nFadeOut			, UINT			, FO..							)
WRITE_MPTHEADER_sized_member(	dwFlags				, DWORD			, dF..							)
WRITE_MPTHEADER_sized_member(	nGlobalVol			, UINT			, GV..							)
WRITE_MPTHEADER_sized_member(	nPan				, UINT			, P...							)
WRITE_MPTHEADER_sized_member(	nVolEnv				, UINT			, VE..							)
WRITE_MPTHEADER_sized_member(	nPanEnv				, UINT			, PE..							)
WRITE_MPTHEADER_sized_member(	nPitchEnv			, UINT			, PiE.							)
WRITE_MPTHEADER_sized_member(	nVolLoopStart		, BYTE			, VLS.							)
WRITE_MPTHEADER_sized_member(	nVolLoopEnd			, BYTE			, VLE.							)
WRITE_MPTHEADER_sized_member(	nVolSustainBegin	, BYTE			, VSB.							)
WRITE_MPTHEADER_sized_member(	nVolSustainEnd		, BYTE			, VSE.							)
WRITE_MPTHEADER_sized_member(	nPanLoopStart		, BYTE			, PLS.							)
WRITE_MPTHEADER_sized_member(	nPanLoopEnd			, BYTE			, PLE.							)
WRITE_MPTHEADER_sized_member(	nPanSustainBegin	, BYTE			, PSB.							)
WRITE_MPTHEADER_sized_member(	nPanSustainEnd		, BYTE			, PSE.							)
WRITE_MPTHEADER_sized_member(	nPitchLoopStart		, BYTE			, PiLS							)
WRITE_MPTHEADER_sized_member(	nPitchLoopEnd		, BYTE			, PiLE							)
WRITE_MPTHEADER_sized_member(	nPitchSustainBegin	, BYTE			, PiSB							)
WRITE_MPTHEADER_sized_member(	nPitchSustainEnd	, BYTE			, PiSE							)
WRITE_MPTHEADER_sized_member(	nNNA				, BYTE			, NNA.							)
WRITE_MPTHEADER_sized_member(	nDCT				, BYTE			, DCT.							)
WRITE_MPTHEADER_sized_member(	nDNA				, BYTE			, DNA.							)
WRITE_MPTHEADER_sized_member(	nPanSwing			, BYTE			, PS..							)
WRITE_MPTHEADER_sized_member(	nVolSwing			, BYTE			, VS..							)
WRITE_MPTHEADER_sized_member(	nIFC				, BYTE			, IFC.							)
WRITE_MPTHEADER_sized_member(	nIFR				, BYTE			, IFR.							)
WRITE_MPTHEADER_sized_member(	wMidiBank			, WORD			, MB..							)
WRITE_MPTHEADER_sized_member(	nMidiProgram		, BYTE			, MP..							)
WRITE_MPTHEADER_sized_member(	nMidiChannel		, BYTE			, MC..							)
WRITE_MPTHEADER_sized_member(	nMidiDrumKey		, BYTE			, MDK.							)
WRITE_MPTHEADER_sized_member(	nPPS				, signed char	, PPS.							)
WRITE_MPTHEADER_sized_member(	nPPC				, unsigned char	, PPC.							)
WRITE_MPTHEADER_array_member(	VolPoints			, WORD			, VP[.		, MAX_ENVPOINTS		)
WRITE_MPTHEADER_array_member(	PanPoints			, WORD			, PP[.		, MAX_ENVPOINTS		)
WRITE_MPTHEADER_array_member(	PitchPoints			, WORD			, PiP[		, MAX_ENVPOINTS		)
WRITE_MPTHEADER_array_member(	VolEnv				, BYTE			, VE[.		, MAX_ENVPOINTS		)
WRITE_MPTHEADER_array_member(	PanEnv				, BYTE			, PE[.		, MAX_ENVPOINTS		)
WRITE_MPTHEADER_array_member(	PitchEnv			, BYTE			, PiE[		, MAX_ENVPOINTS		)
WRITE_MPTHEADER_array_member(	NoteMap				, BYTE			, NM[.		, 128				)
WRITE_MPTHEADER_array_member(	Keyboard			, WORD			, K[..		, 128				)
WRITE_MPTHEADER_array_member(	name				, CHAR			, n[..		, 32				)
WRITE_MPTHEADER_array_member(	filename			, CHAR			, fn[.		, 12				)
WRITE_MPTHEADER_sized_member(	nMixPlug			, BYTE			, MiP.							)
WRITE_MPTHEADER_sized_member(	nVolRamp			, USHORT		, VR..							)
WRITE_MPTHEADER_sized_member(	nResampling			, USHORT		, R...							)
WRITE_MPTHEADER_sized_member(	nCutSwing			, BYTE			, CS..							)
WRITE_MPTHEADER_sized_member(	nResSwing			, BYTE			, RS..							)
WRITE_MPTHEADER_sized_member(	nFilterMode			, BYTE			, FM..							)
WRITE_MPTHEADER_sized_member(  nPluginVelocityHandling	, BYTE			, PVEH							)
WRITE_MPTHEADER_sized_member(	nPluginVolumeHandling	, BYTE			, PVOH							)
WRITE_MPTHEADER_sized_member(	wPitchToTempoLock	, WORD			, PTTL							)
WRITE_MPTHEADER_sized_member(	nPitchEnvReleaseNode, BYTE			, PERN							)
WRITE_MPTHEADER_sized_member(	nPanEnvReleaseNode  , BYTE		    , AERN							)
WRITE_MPTHEADER_sized_member(	nVolEnvReleaseNode	, BYTE			, VERN							)
}

// --------------------------------------------------------------------------------------------
// Convenient macro to help GET_HEADER declaration for single type members ONLY (non-array)
// --------------------------------------------------------------------------------------------
#define GET_MPTHEADER_sized_member(name,type,code) \
case( #@code ):\
if( fsize <= sizeof( type ) ) pointer = (BYTE *)&input-> name ;\
break;

// --------------------------------------------------------------------------------------------
// Convenient macro to help GET_HEADER declaration for array members ONLY
// --------------------------------------------------------------------------------------------
#define GET_MPTHEADER_array_member(name,type,code,arraysize) \
case( #@code ):\
if( fsize <= sizeof( type ) * arraysize ) pointer = (BYTE *)&input-> name ;\
break;

// Return a pointer on the wanted field in 'input' INSTRUMENTHEADER given field code & size
BYTE * GetInstrumentHeaderFieldPointer(INSTRUMENTHEADER * input, __int32 fcode, __int16 fsize)
{
if(input == NULL) return NULL;
BYTE * pointer = NULL;

switch(fcode){
GET_MPTHEADER_sized_member(	nFadeOut			, UINT			, FO..							)
GET_MPTHEADER_sized_member(	dwFlags				, DWORD			, dF..							)
GET_MPTHEADER_sized_member(	nGlobalVol			, UINT			, GV..							)
GET_MPTHEADER_sized_member(	nPan				, UINT			, P...							)
GET_MPTHEADER_sized_member(	nVolEnv				, UINT			, VE..							)
GET_MPTHEADER_sized_member(	nPanEnv				, UINT			, PE..							)
GET_MPTHEADER_sized_member(	nPitchEnv			, UINT			, PiE.							)
GET_MPTHEADER_sized_member(	nVolLoopStart		, BYTE			, VLS.							)
GET_MPTHEADER_sized_member(	nVolLoopEnd			, BYTE			, VLE.							)
GET_MPTHEADER_sized_member(	nVolSustainBegin	, BYTE			, VSB.							)
GET_MPTHEADER_sized_member(	nVolSustainEnd		, BYTE			, VSE.							)
GET_MPTHEADER_sized_member(	nPanLoopStart		, BYTE			, PLS.							)
GET_MPTHEADER_sized_member(	nPanLoopEnd			, BYTE			, PLE.							)
GET_MPTHEADER_sized_member(	nPanSustainBegin	, BYTE			, PSB.							)
GET_MPTHEADER_sized_member(	nPanSustainEnd		, BYTE			, PSE.							)
GET_MPTHEADER_sized_member(	nPitchLoopStart		, BYTE			, PiLS							)
GET_MPTHEADER_sized_member(	nPitchLoopEnd		, BYTE			, PiLE							)
GET_MPTHEADER_sized_member(	nPitchSustainBegin	, BYTE			, PiSB							)
GET_MPTHEADER_sized_member(	nPitchSustainEnd	, BYTE			, PiSE							)
GET_MPTHEADER_sized_member(	nNNA				, BYTE			, NNA.							)
GET_MPTHEADER_sized_member(	nDCT				, BYTE			, DCT.							)
GET_MPTHEADER_sized_member(	nDNA				, BYTE			, DNA.							)
GET_MPTHEADER_sized_member(	nPanSwing			, BYTE			, PS..							)
GET_MPTHEADER_sized_member(	nVolSwing			, BYTE			, VS..							)
GET_MPTHEADER_sized_member(	nIFC				, BYTE			, IFC.							)
GET_MPTHEADER_sized_member(	nIFR				, BYTE			, IFR.							)
GET_MPTHEADER_sized_member(	wMidiBank			, WORD			, MB..							)
GET_MPTHEADER_sized_member(	nMidiProgram		, BYTE			, MP..							)
GET_MPTHEADER_sized_member(	nMidiChannel		, BYTE			, MC..							)
GET_MPTHEADER_sized_member(	nMidiDrumKey		, BYTE			, MDK.							)
GET_MPTHEADER_sized_member(	nPPS				, signed char	, PPS.							)
GET_MPTHEADER_sized_member(	nPPC				, unsigned char	, PPC.							)
GET_MPTHEADER_array_member(	VolPoints			, WORD			, VP[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PanPoints			, WORD			, PP[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PitchPoints			, WORD			, PiP[		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	VolEnv				, BYTE			, VE[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PanEnv				, BYTE			, PE[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PitchEnv			, BYTE			, PiE[		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	NoteMap				, BYTE			, NM[.		, 128				)
GET_MPTHEADER_array_member(	Keyboard			, WORD			, K[..		, 128				)
GET_MPTHEADER_array_member(	name				, CHAR			, n[..		, 32				)
GET_MPTHEADER_array_member(	filename			, CHAR			, fn[.		, 12				)
GET_MPTHEADER_sized_member(	nMixPlug			, BYTE			, MiP.							)
GET_MPTHEADER_sized_member(	nVolRamp			, USHORT		, VR..							)
GET_MPTHEADER_sized_member(	nResampling			, UINT			, R...							)
GET_MPTHEADER_sized_member(	nCutSwing			, BYTE			, CS..							)
GET_MPTHEADER_sized_member(	nResSwing			, BYTE			, RS..							)
GET_MPTHEADER_sized_member(	nFilterMode			, BYTE			, FM..							)
GET_MPTHEADER_sized_member(	wPitchToTempoLock	, WORD			, PTTL							)
GET_MPTHEADER_sized_member(	nPluginVelocityHandling, BYTE			, PVEH							)
GET_MPTHEADER_sized_member(	nPluginVolumeHandling	, BYTE			, PVOH							)
GET_MPTHEADER_sized_member(	nPitchEnvReleaseNode, BYTE			, PERN							)
GET_MPTHEADER_sized_member(	nPanEnvReleaseNode  , BYTE		    , AERN							)
GET_MPTHEADER_sized_member(	nVolEnvReleaseNode	, BYTE			, VERN							)
}

return pointer;
}

// -! NEW_FEATURE#0027


CTuning* INSTRUMENTHEADER::s_DefaultTuning = 0;

const ROWINDEX CPatternSizesMimic::operator [](const int i) const
//-----------------------------------------------------------
{
	return m_rSndFile.Patterns[i].GetNumRows();
}


//////////////////////////////////////////////////////////
// CSoundFile

CTuningCollection* CSoundFile::s_pTuningsSharedStandard(0);
CTuningCollection* CSoundFile::s_pTuningsSharedLocal(0);


CSoundFile::CSoundFile() :
	PatternSize(*this), Patterns(*this),
	Order(*this),
	m_PlaybackEventer(*this),
	m_pModSpecs(&IT_MPTEXT_SPECS),
	m_MIDIMapper(*this)
//----------------------
{
	m_nType = MOD_TYPE_NONE;
	m_dwSongFlags = 0;
	m_nChannels = 0;
	m_nMixChannels = 0;
	m_nSamples = 0;
	m_nInstruments = 0;
	m_nPatternNames = 0;
	m_lpszPatternNames = NULL;
	m_lpszSongComments = NULL;
	m_nFreqFactor = m_nTempoFactor = 128;
	m_nMasterVolume = 128;
	m_nMinPeriod = MIN_PERIOD;
	m_nMaxPeriod = 0x7FFF;
	m_nRepeatCount = 0;
	m_nSeqOverride = 0;
	m_bPatternTransitionOccurred = false;
	m_nRowsPerBeat = 4;
	m_nRowsPerMeasure = 16;
	m_nTempoMode = tempo_mode_classic;
	m_bIsRendering = false;
	m_nMaxSample = 0;

	m_ModFlags = 0;

	m_pModDoc = NULL;
	m_dwLastSavedWithVersion=0;
	m_dwCreatedWithVersion=0;
	memset(m_bChannelMuteTogglePending, 0, sizeof(m_bChannelMuteTogglePending));


// -> CODE#0023
// -> DESC="IT project files (.itp)"
	for(UINT i = 0 ; i < MAX_INSTRUMENTS ; i++){
		m_szInstrumentPath[i][0] = '\0';
		instrumentModified[i] = FALSE;
	}
// -! NEW_FEATURE#0023

	memset(Chn, 0, sizeof(Chn));
	memset(ChnMix, 0, sizeof(ChnMix));
	memset(Ins, 0, sizeof(Ins));
	memset(ChnSettings, 0, sizeof(ChnSettings));
	memset(Headers, 0, sizeof(Headers));
	Order.assign(MAX_ORDERS, Order.GetInvalidPatIndex());
	Patterns.ClearPatterns();
	memset(m_szNames, 0, sizeof(m_szNames));
	memset(m_MixPlugins, 0, sizeof(m_MixPlugins));
	memset(&m_SongEQ, 0, sizeof(m_SongEQ));
	m_lTotalSampleCount=0;

	m_pConfig = new CSoundFilePlayConfig();
	m_pTuningsTuneSpecific = new CTuningCollection("Tune specific tunings");
	
	BuildDefaultInstrument();
}


CSoundFile::~CSoundFile()
//-----------------------
{
	delete m_pConfig;
	delete m_pTuningsTuneSpecific;
	Destroy();
}


BOOL CSoundFile::Create(LPCBYTE lpStream, CModDoc *pModDoc, DWORD dwMemLength)
//---------------------------------------------------------------------------
{
	m_pModDoc=pModDoc;
	m_nType = MOD_TYPE_NONE;
	m_dwSongFlags = 0;
	m_nChannels = 0;
	m_nMixChannels = 0;
	m_nSamples = 0;
	m_nInstruments = 0;
	m_nFreqFactor = m_nTempoFactor = 128;
	m_nMasterVolume = 128;
	m_nDefaultGlobalVolume = 256;
	m_nGlobalVolume = 256;
	m_nOldGlbVolSlide = 0;
	m_nDefaultSpeed = 6;
	m_nDefaultTempo = 125;
	m_nPatternDelay = 0;
	m_nFrameDelay = 0;
	m_nNextRow = 0;
	m_nRow = 0;
	m_nPattern = 0;
	m_nCurrentPattern = 0;
	m_nNextPattern = 0;
	m_nSeqOverride = 0;
	m_nRestartPos = 0;
	m_nMinPeriod = 16;
	m_nMaxPeriod = 32767;
	m_nSamplePreAmp = 128;
	m_nVSTiVolume = 128;
	m_nPatternNames = 0;
	m_nMaxOrderPosition = 0;
	m_lpszPatternNames = NULL;
	m_lpszSongComments = NULL;
	m_nMixLevels = mixLevels_original;	// Will be overridden if appropriate.
	memset(Ins, 0, sizeof(Ins));
	memset(ChnMix, 0, sizeof(ChnMix));
	memset(Chn, 0, sizeof(Chn));
	memset(Headers, 0, sizeof(Headers));
	Order.assign(MAX_ORDERS, Order.GetInvalidPatIndex());
	Patterns.ClearPatterns();
	memset(m_szNames, 0, sizeof(m_szNames));
	memset(m_MixPlugins, 0, sizeof(m_MixPlugins));
	memset(&m_SongEQ, 0, sizeof(m_SongEQ));
	ResetMidiCfg();
	//for (UINT npt=0; npt<Patterns.Size(); npt++) PatternSize[npt] = 64;
	for (UINT nch=0; nch<MAX_BASECHANNELS; nch++)
	{
		ChnSettings[nch].nPan = 128;
		ChnSettings[nch].nVolume = 64;
		ChnSettings[nch].dwFlags = 0;
		ChnSettings[nch].szName[0] = 0;
	}
	if (lpStream)
	{
#ifdef ZIPPED_MOD_SUPPORT
		CZipArchive archive(glpszModExtensions);
		if (CZipArchive::IsArchive((LPBYTE)lpStream, dwMemLength))
		{
			if (archive.UnzipArchive((LPBYTE)lpStream, dwMemLength))
			{
				lpStream = archive.GetOutputFile();
				dwMemLength = archive.GetOutputFileLength();
			}
		}
#endif
#ifdef UNRAR_SUPPORT
		CRarArchive unrar((LPBYTE)lpStream, dwMemLength, glpszModExtensions);
		if (unrar.IsArchive())
		{
			if (unrar.ExtrFile())
			{
				lpStream = unrar.GetOutputFile();
				dwMemLength = unrar.GetOutputFileLength();
			}
		}
#endif
#ifdef UNLHA_SUPPORT
		CLhaArchive unlha((LPBYTE)lpStream, dwMemLength, glpszModExtensions);
		if (unlha.IsArchive())
		{
			if (unlha.ExtractFile())
			{
				lpStream = unlha.GetOutputFile();
				dwMemLength = unlha.GetOutputFileLength();
			}
		}
#endif
#ifdef MMCMP_SUPPORT
		BOOL bMMCmp = MMCMP_Unpack(&lpStream, &dwMemLength);
#endif
		if ((!ReadXM(lpStream, dwMemLength))
// -> CODE#0023
// -> DESC="IT project files (.itp)"
		 && (!ReadITProject(lpStream, dwMemLength))
// -! NEW_FEATURE#0023
		 && (!ReadIT(lpStream, dwMemLength))
		 /*&& (!ReadMPT(lpStream, dwMemLength))*/
		 && (!ReadS3M(lpStream, dwMemLength))
		 && (!ReadWav(lpStream, dwMemLength))
#ifndef MODPLUG_BASIC_SUPPORT
		 && (!ReadSTM(lpStream, dwMemLength))
		 && (!ReadMed(lpStream, dwMemLength))
#ifndef FASTSOUNDLIB
		 && (!ReadMTM(lpStream, dwMemLength))
		 && (!ReadMDL(lpStream, dwMemLength))
		 && (!ReadDBM(lpStream, dwMemLength))
		 && (!Read669(lpStream, dwMemLength))
		 && (!ReadFAR(lpStream, dwMemLength))
		 && (!ReadAMS(lpStream, dwMemLength))
		 && (!ReadOKT(lpStream, dwMemLength))
		 && (!ReadPTM(lpStream, dwMemLength))
		 && (!ReadUlt(lpStream, dwMemLength))
		 && (!ReadDMF(lpStream, dwMemLength))
		 && (!ReadDSM(lpStream, dwMemLength))
		 && (!ReadUMX(lpStream, dwMemLength))
		 && (!ReadAMF(lpStream, dwMemLength))
		 && (!ReadPSM(lpStream, dwMemLength))
		 && (!ReadMT2(lpStream, dwMemLength))
#ifdef MODPLUG_TRACKER
		 && (!ReadMID(lpStream, dwMemLength))
#endif // MODPLUG_TRACKER
#endif
#endif // MODPLUG_BASIC_SUPPORT
		 && (!ReadMod(lpStream, dwMemLength))) m_nType = MOD_TYPE_NONE;
#ifdef ZIPPED_MOD_SUPPORT
		if ((!m_lpszSongComments) && (archive.GetComments(FALSE)))
		{
			m_lpszSongComments = archive.GetComments(TRUE);
		}
#endif
#ifdef MMCMP_SUPPORT
		if (bMMCmp)
		{
			GlobalFreePtr(lpStream);
			lpStream = NULL;
		}
#endif
	} else {
		// New song
		m_dwCreatedWithVersion = MptVersion::num;
	}

	// Adjust song names
	for (UINT iSmp=0; iSmp<MAX_SAMPLES; iSmp++)
	{
		LPSTR p = m_szNames[iSmp];
		int j = 31;
		p[j] = 0;
		while ((j>=0) && (p[j]<=' ')) p[j--] = 0;
		while (j>=0)
		{
			if (((BYTE)p[j]) < ' ') p[j] = ' ';
			j--;
		}
	}
	// Adjust channels
	for (UINT ich=0; ich<MAX_BASECHANNELS; ich++)
	{
		if (ChnSettings[ich].nVolume > 64) ChnSettings[ich].nVolume = 64;
		if (ChnSettings[ich].nPan > 256) ChnSettings[ich].nPan = 128;
		Chn[ich].nPan = ChnSettings[ich].nPan;
		Chn[ich].nGlobalVol = ChnSettings[ich].nVolume;
		Chn[ich].dwFlags = ChnSettings[ich].dwFlags;
		Chn[ich].nVolume = 256;
		Chn[ich].nCutOff = 0x7F;
	}
	// Checking instruments
	MODINSTRUMENT *pins = Ins;
	for (UINT iIns=0; iIns<MAX_INSTRUMENTS; iIns++, pins++)
	{
		if (pins->pSample)
		{
			if (pins->nLoopEnd > pins->nLength) pins->nLoopEnd = pins->nLength;
			if (pins->nLoopStart + 3 >= pins->nLoopEnd)
			{
				pins->nLoopStart = 0;
				pins->nLoopEnd = 0;
			}
			if (pins->nSustainEnd > pins->nLength) pins->nSustainEnd = pins->nLength;
			if (pins->nSustainStart + 3 >= pins->nSustainEnd)
			{
				pins->nSustainStart = 0;
				pins->nSustainEnd = 0;
			}
		} else
		{
			pins->nLength = 0;
			pins->nLoopStart = 0;
			pins->nLoopEnd = 0;
			pins->nSustainStart = 0;
			pins->nSustainEnd = 0;
		}
		if (!pins->nLoopEnd) pins->uFlags &= ~CHN_LOOP;
		if (!pins->nSustainEnd) pins->uFlags &= ~CHN_SUSTAINLOOP;
		if (pins->nGlobalVol > 64) pins->nGlobalVol = 64;
	}
	// Check invalid instruments
	while ((m_nInstruments > 0) && (!Headers[m_nInstruments])) m_nInstruments--;
	// Set default values
	if (m_nDefaultTempo < 32) m_nDefaultTempo = 125;
	if (!m_nDefaultSpeed) m_nDefaultSpeed = 6;
	m_nMusicSpeed = m_nDefaultSpeed;
	m_nMusicTempo = m_nDefaultTempo;
	m_nGlobalVolume = m_nDefaultGlobalVolume;
	m_lHighResRampingGlobalVolume = m_nGlobalVolume<<VOLUMERAMPPRECISION;
	m_nGlobalVolumeDestination = m_nGlobalVolume;
	m_nSamplesToGlobalVolRampDest=0;
	m_nNextPattern = 0;
	m_nCurrentPattern = 0;
	m_nPattern = 0;
	m_nBufferCount = 0;
	m_dBufferDiff = 0;
	m_nTickCount = m_nMusicSpeed;
	m_nNextRow = 0;
	m_nRow = 0;

	switch(m_nTempoMode) {
		case tempo_mode_alternative: 
			m_nSamplesPerTick = gdwMixingFreq / m_nMusicTempo; break;
		case tempo_mode_modern: 
			m_nSamplesPerTick = gdwMixingFreq * (60/m_nMusicTempo / (m_nMusicSpeed * m_nRowsPerBeat)); break;
		case tempo_mode_classic: default:
			m_nSamplesPerTick = (gdwMixingFreq * 5 * m_nTempoFactor) / (m_nMusicTempo << 8);
	}

	if ((m_nRestartPos >= Order.size()) || (Order[m_nRestartPos] >= Patterns.Size())) m_nRestartPos = 0;
	// Load plugins only when m_pModDoc != 0.  (can be == 0 for example when examining module samples in treeview.
	if (gpMixPluginCreateProc && GetpModDoc())
	{
		for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
		{
			if ((m_MixPlugins[iPlug].Info.dwPluginId1)
			 || (m_MixPlugins[iPlug].Info.dwPluginId2))
			{
				gpMixPluginCreateProc(&m_MixPlugins[iPlug], this);
				if (m_MixPlugins[iPlug].pMixPlugin)
				{
					m_MixPlugins[iPlug].pMixPlugin->RestoreAllParameters(m_MixPlugins[iPlug].defaultProgram); //rewbs.plugDefaultProgram: added param
				}
			}
		}
	}

	// Set up mix levels
	m_pConfig->SetMixLevels(m_nMixLevels);
	RecalculateGainForAllPlugs();

	if (m_nType)
	{
		SetModSpecsPointer(m_pModSpecs, m_nType);
		return TRUE;
	}

	return FALSE;
}


BOOL CSoundFile::Destroy()
//------------------------
{
	size_t i;
	for (i=0; i<Patterns.Size(); i++) if (Patterns[i])
	{
		FreePattern(Patterns[i]);
		Patterns[i] = NULL;
	}
	m_nPatternNames = 0;

	delete[] m_lpszPatternNames;
	m_lpszPatternNames = NULL;

	delete[] m_lpszSongComments;
	m_lpszSongComments = NULL;

	for (i=1; i<MAX_SAMPLES; i++)
	{
		MODINSTRUMENT *pins = &Ins[i];
		if (pins->pSample)
		{
			FreeSample(pins->pSample);
			pins->pSample = NULL;
		}
	}
	for (i=0; i<MAX_INSTRUMENTS; i++)
	{
		delete Headers[i];
		Headers[i] = NULL;
	}
	for (i=0; i<MAX_MIXPLUGINS; i++)
	{
		if ((m_MixPlugins[i].nPluginDataSize) && (m_MixPlugins[i].pPluginData))
		{
			m_MixPlugins[i].nPluginDataSize = 0;
			delete[] m_MixPlugins[i].pPluginData;
			m_MixPlugins[i].pPluginData = NULL;
		}
		m_MixPlugins[i].pMixState = NULL;
		if (m_MixPlugins[i].pMixPlugin)
		{
			m_MixPlugins[i].pMixPlugin->Release();
			m_MixPlugins[i].pMixPlugin = NULL;
		}
	}
	m_nType = MOD_TYPE_NONE;
	m_nChannels = m_nSamples = m_nInstruments = 0;
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
// Memory Allocation

MODCOMMAND *CSoundFile::AllocatePattern(UINT rows, UINT nchns)
//------------------------------------------------------------
{
	MODCOMMAND *p = new MODCOMMAND[rows*nchns];
	if (p) memset(p, 0, rows*nchns*sizeof(MODCOMMAND));
	return p;
}


void CSoundFile::FreePattern(LPVOID pat)
//--------------------------------------
{
	
	if (pat) delete pat;
}


LPSTR CSoundFile::AllocateSample(UINT nbytes)
//-------------------------------------------
{
	if (nbytes>0xFFFFFFD6)
		return NULL;
	LPSTR p = (LPSTR)GlobalAllocPtr(GHND, (nbytes+39) & ~7);
	if (p) p += 16;
	return p;
}


void CSoundFile::FreeSample(LPVOID p)
//-----------------------------------
{
	if (p)
	{
		GlobalFreePtr(((LPSTR)p)-16);
	}
}


//////////////////////////////////////////////////////////////////////////
// Misc functions

void CSoundFile::ResetMidiCfg()
//-----------------------------
{
	memset(&m_MidiCfg, 0, sizeof(m_MidiCfg));
	lstrcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_START*32], "FF");
	lstrcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_STOP*32], "FC");
	lstrcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_NOTEON*32], "9c n v");
	lstrcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_NOTEOFF*32], "9c n 0");
	lstrcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_PROGRAM*32], "Cc p");
	lstrcpy(&m_MidiCfg.szMidiSFXExt[0], "F0F000z");
	for (int iz=0; iz<16; iz++) wsprintf(&m_MidiCfg.szMidiZXXExt[iz*32], "F0F001%02X", iz*8);
}


UINT CSoundFile::GetSongComments(LPSTR s, UINT len, UINT linesize)
//----------------------------------------------------------------
{
	LPCSTR p = m_lpszSongComments;
	if (!p) return 0;
	UINT i = 2, ln=0;
	if ((len) && (s)) s[0] = '\x0D';
	if ((len > 1) && (s)) s[1] = '\x0A';
	while ((*p)	&& (i+2 < len))
	{
		BYTE c = (BYTE)*p++;
		if ((c == 0x0D) || ((c == ' ') && (ln >= linesize)))
			{ if (s) { s[i++] = '\x0D'; s[i++] = '\x0A'; } else i+= 2; ln=0; }
		else
		if (c >= 0x20) { if (s) s[i++] = c; else i++; ln++; }
	}
	if (s) s[i] = 0;
	return i;
}


UINT CSoundFile::GetRawSongComments(LPSTR s, UINT len, UINT linesize)
//-------------------------------------------------------------------
{
	LPCSTR p = m_lpszSongComments;
	if (!p) return 0;
	UINT i = 0, ln=0;
	while ((*p)	&& (i < len-1))
	{
		BYTE c = (BYTE)*p++;
		if ((c == 0x0D)	|| (c == 0x0A))
		{
			if (ln) 
			{
				while (ln < linesize) { if (s) s[i] = ' '; i++; ln++; }
				ln = 0;
			}
		} else
		if ((c == ' ') && (!ln))
		{
			UINT k=0;
			while ((p[k]) && (p[k] >= ' '))	k++;
			if (k <= linesize)
			{
				if (s) s[i] = ' ';
				i++;
				ln++;
			}
		} else
		{
			if (s) s[i] = c;
			i++;
			ln++;
			if (ln == linesize) ln = 0;
		}
	}
	if (ln)
	{
		while ((ln < linesize) && (i < len))
		{
			if (s) s[i] = ' ';
			i++;
			ln++;
		}
	}
	if (s) s[i] = 0;
	return i;
}


BOOL CSoundFile::SetWaveConfig(UINT nRate,UINT nBits,UINT nChannels,BOOL bMMX)
//----------------------------------------------------------------------------
{
	BOOL bReset = FALSE;
	DWORD d = gdwSoundSetup & ~SNDMIX_ENABLEMMX;
	if (bMMX) d |= SNDMIX_ENABLEMMX;
	if ((gdwMixingFreq != nRate) || (gnBitsPerSample != nBits) || (gnChannels != nChannels) || (d != gdwSoundSetup)) bReset = TRUE;
	gnChannels = nChannels;
	gdwSoundSetup = d;
	gdwMixingFreq = nRate;
	gnBitsPerSample = nBits;
	InitPlayer(bReset);
	return TRUE;
}


BOOL CSoundFile::SetDspEffects(BOOL bSurround,BOOL bReverb,BOOL bMegaBass,BOOL bNR,BOOL bEQ)
//------------------------------------------------------------------------------------------
{
	DWORD d = gdwSoundSetup & ~(SNDMIX_SURROUND | SNDMIX_REVERB | SNDMIX_MEGABASS | SNDMIX_NOISEREDUCTION | SNDMIX_EQ);
	if (bSurround) d |= SNDMIX_SURROUND;
	if ((bReverb) && (gdwSysInfo & SYSMIX_ENABLEMMX)) d |= SNDMIX_REVERB;
	if (bMegaBass) d |= SNDMIX_MEGABASS;
	if (bNR) d |= SNDMIX_NOISEREDUCTION;
	if (bEQ) d |= SNDMIX_EQ;
	gdwSoundSetup = d;
	InitPlayer(FALSE);
	return TRUE;
}


BOOL CSoundFile::SetResamplingMode(UINT nMode)
//--------------------------------------------
{
	DWORD d = gdwSoundSetup & ~(SNDMIX_NORESAMPLING|SNDMIX_SPLINESRCMODE|SNDMIX_POLYPHASESRCMODE|SNDMIX_FIRFILTERSRCMODE);
	switch(nMode)
	{
	case SRCMODE_NEAREST:	d |= SNDMIX_NORESAMPLING; break;
	case SRCMODE_LINEAR:	break; // default
	//rewbs.resamplerConf
	//case SRCMODE_SPLINE:	d |= SNDMIX_HQRESAMPLER; break;
	//case SRCMODE_POLYPHASE:	d |= (SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE); break;
	case SRCMODE_SPLINE:	d |= SNDMIX_SPLINESRCMODE; break;
	case SRCMODE_POLYPHASE:	d |= SNDMIX_POLYPHASESRCMODE; break;
	case SRCMODE_FIRFILTER:	d |= SNDMIX_FIRFILTERSRCMODE; break;
	default: return FALSE;
	//end rewbs.resamplerConf
	}
	gdwSoundSetup = d;
	return TRUE;
}


BOOL CSoundFile::SetMasterVolume(UINT nVol, BOOL bAdjustAGC)
//----------------------------------------------------------
{
	if (nVol < 1) nVol = 1;
	if (nVol > 0x200) nVol = 0x200;	// x4 maximum
	if ((nVol < m_nMasterVolume) && (nVol) && (gdwSoundSetup & SNDMIX_AGC) && (bAdjustAGC))
	{
		gnAGC = gnAGC * m_nMasterVolume / nVol;
		if (gnAGC > AGC_UNITY) gnAGC = AGC_UNITY;
	}
	m_nMasterVolume = nVol;
	return TRUE;
}


void CSoundFile::SetAGC(BOOL b)
//-----------------------------
{
	if (b)
	{
		if (!(gdwSoundSetup & SNDMIX_AGC))
		{
			gdwSoundSetup |= SNDMIX_AGC;
			gnAGC = AGC_UNITY;
		}
	} else gdwSoundSetup &= ~SNDMIX_AGC;
}


UINT CSoundFile::GetNumPatterns() const
//-------------------------------------
{
	UINT i = 0;
	while ((i < Order.size()) && (Order[i] < Order.GetInvalidPatIndex())) i++;
	return i;
}

/*
UINT CSoundFile::GetNumInstruments() const
//----------------------------------------
{
	UINT n=0;
	for (UINT i=0; i<MAX_INSTRUMENTS; i++) if (Ins[i].pSample) n++;
	return n;
}
*/


UINT CSoundFile::GetMaxPosition() const
//-------------------------------------
{
	UINT max = 0;
	UINT i = 0;

	while ((i < Order.size()) && (Order[i] != Order.GetInvalidPatIndex()))
	{
		if (Order[i] < Patterns.Size()) max += PatternSize[Order[i]];
		i++;
	}
	return max;
}


UINT CSoundFile::GetCurrentPos() const
//------------------------------------
{
	UINT pos = 0;

	for (UINT i=0; i<m_nCurrentPattern; i++) if (Order[i] < Patterns.Size())
		pos += PatternSize[Order[i]];
	return pos + m_nRow; 
}

double  CSoundFile::GetCurrentBPM() const
//---------------------------------------
{
	double bpm;

	if (m_nTempoMode == tempo_mode_modern) {		// With modern mode, we trust that true bpm 
		bpm = static_cast<double>(m_nMusicTempo);	// is  be close enough to what user chose.
	}												// This avoids oscillation due to tick-to-tick corrections.

	else {												//with other modes, we calculate it:
		double ticksPerBeat = m_nMusicSpeed*m_nRowsPerBeat;		//ticks/beat = ticks/row  * rows/beat
		double samplesPerBeat = m_nSamplesPerTick*ticksPerBeat;	//samps/beat = samps/tick * ticks/beat
		bpm =  gdwMixingFreq/samplesPerBeat*60;					//beats/sec  = samps/sec  / samps/beat
	}															//beats/min  =  beats/sec * 60
	
	return bpm;
}

void CSoundFile::SetCurrentPos(UINT nPos)
//---------------------------------------
{
	UINT nPattern;
	BYTE resetMask = (!nPos) ? CHNRESET_SETPOS_FULL : CHNRESET_SETPOS_BASIC;

	for (CHANNELINDEX i=0; i<MAX_CHANNELS; i++)
		ResetChannelState(i, resetMask);
	
	if (!nPos)
	{
		m_nGlobalVolume = m_nDefaultGlobalVolume;
		m_nMusicSpeed = m_nDefaultSpeed;
		m_nMusicTempo = m_nDefaultTempo;
	}
	//m_dwSongFlags &= ~(SONG_PATTERNLOOP|SONG_CPUVERYHIGH|SONG_FADINGSONG|SONG_ENDREACHED|SONG_GLOBALFADE);
	m_dwSongFlags &= ~(SONG_CPUVERYHIGH|SONG_FADINGSONG|SONG_ENDREACHED|SONG_GLOBALFADE);
	for (nPattern = 0; nPattern < Order.size(); nPattern++)
	{
		UINT ord = Order[nPattern];
		if(ord == Order.GetIgnoreIndex()) continue;
		if (ord == Order.GetInvalidPatIndex()) break;
		if (ord < Patterns.Size())
		{
			if (nPos < (UINT)PatternSize[ord]) break;
			nPos -= PatternSize[ord];
		}
	}
	// Buggy position ?
	if ((nPattern >= Order.size())
	 || (Order[nPattern] >= Patterns.Size())
	 || (nPos >= PatternSize[Order[nPattern]]))
	{
		nPos = 0;
		nPattern = 0;
	}
	UINT nRow = nPos;
	if ((nRow) && (Order[nPattern] < Patterns.Size()))
	{
		MODCOMMAND *p = Patterns[Order[nPattern]];
		if ((p) && (nRow < PatternSize[Order[nPattern]]))
		{
			BOOL bOk = FALSE;
			while ((!bOk) && (nRow > 0))
			{
				UINT n = nRow * m_nChannels;
				for (UINT k=0; k<m_nChannels; k++, n++)
				{
					if (p[n].note)
					{
						bOk = TRUE;
						break;
					}
				}
				if (!bOk) nRow--;
			}
		}
	}
	m_nNextPattern = nPattern;
	m_nNextRow = nRow;
	m_nTickCount = m_nMusicSpeed;
	m_nBufferCount = 0;
	m_nPatternDelay = 0;
	m_nFrameDelay = 0;
	//m_nSeqOverride = 0;
}



void CSoundFile::SetCurrentOrder(UINT nPos)
//-----------------------------------------
{
	//while ((nPos < Order.size()) && (Order[nPos] == 0xFE)) nPos++;
	while ((nPos < Order.size()) && (Order[nPos] == Order.GetIgnoreIndex())) nPos++;
	if ((nPos >= Order.size()) || (Order[nPos] >= Patterns.Size())) return;
	for (UINT j=0; j<MAX_CHANNELS; j++)
	{
		Chn[j].nPeriod = 0;
		Chn[j].nNote = 0;
		Chn[j].nPortamentoDest = 0;
		Chn[j].nCommand = 0;
		Chn[j].nPatternLoopCount = 0;
		Chn[j].nPatternLoop = 0;
		Chn[j].nTremorCount = 0;
	}
	if (!nPos)
	{
		SetCurrentPos(0);
	} else
	{
		m_nNextPattern = nPos;
		m_nRow = m_nNextRow = 0;
		m_nPattern = 0;
		m_nTickCount = m_nMusicSpeed;
		m_nBufferCount = 0;
		m_nTotalCount = 0;
		m_nPatternDelay = 0;
		m_nFrameDelay = 0;
	}
	//m_dwSongFlags &= ~(SONG_PATTERNLOOP|SONG_CPUVERYHIGH|SONG_FADINGSONG|SONG_ENDREACHED|SONG_GLOBALFADE);
	m_dwSongFlags &= ~(SONG_CPUVERYHIGH|SONG_FADINGSONG|SONG_ENDREACHED|SONG_GLOBALFADE);
}

//rewbs.VSTCompliance
void CSoundFile::SuspendPlugins()	
//------------------------------
{
	for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
	{
		if (!m_MixPlugins[iPlug].pMixPlugin)	
			continue;  //most common branch
		
		IMixPlugin *pPlugin = m_MixPlugins[iPlug].pMixPlugin;
		if (m_MixPlugins[iPlug].pMixState)
		{
			pPlugin->NotifySongPlaying(false);
			pPlugin->HardAllNotesOff();
			pPlugin->Suspend();
		}
	}
	m_lTotalSampleCount=0;
}

void CSoundFile::ResumePlugins()	
//------------------------------
{
	for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
	{
		if (!m_MixPlugins[iPlug].pMixPlugin)	
			continue;  //most common branch

		if (m_MixPlugins[iPlug].pMixState)
		{
            IMixPlugin *pPlugin = m_MixPlugins[iPlug].pMixPlugin;
			pPlugin->NotifySongPlaying(true);
			pPlugin->Resume();
		}
	}
	m_lTotalSampleCount=GetSampleOffset();

}


void CSoundFile::StopAllVsti()
//----------------------------
{
	for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++) {
		if (!m_MixPlugins[iPlug].pMixPlugin)	
			continue;  //most common branch
		
		IMixPlugin *pPlugin = m_MixPlugins[iPlug].pMixPlugin;
		if (m_MixPlugins[iPlug].pMixState) {
			pPlugin->HardAllNotesOff();
		}
	}
	m_lTotalSampleCount=GetSampleOffset();
}


void CSoundFile::RecalculateGainForAllPlugs()
//------------------------------------------
{
	for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++) {
		if (!m_MixPlugins[iPlug].pMixPlugin)	
			continue;  //most common branch
		
		IMixPlugin *pPlugin = m_MixPlugins[iPlug].pMixPlugin;
		if (m_MixPlugins[iPlug].pMixState) {
			pPlugin->RecalculateGain();
		}
	}
}

//end rewbs.VSTCompliance

void CSoundFile::ResetChannels()
//------------------------------
{
	m_dwSongFlags &= ~(SONG_CPUVERYHIGH|SONG_FADINGSONG|SONG_ENDREACHED|SONG_GLOBALFADE);
	m_nBufferCount = 0;
	for (UINT i=0; i<MAX_CHANNELS; i++)
	{
		Chn[i].nROfs = Chn[i].nLOfs = 0;
	}
}



void CSoundFile::LoopPattern(int nPat, int nRow)
//----------------------------------------------
{
	if ((nPat < 0) || (nPat >= Patterns.Size()) || (!Patterns[nPat]))
	{
		m_dwSongFlags &= ~SONG_PATTERNLOOP;
	} else
	{
		if ((nRow < 0) || (nRow >= (int)PatternSize[nPat])) nRow = 0;
		m_nPattern = nPat;
		m_nRow = m_nNextRow = nRow;
		m_nTickCount = m_nMusicSpeed;
		m_nPatternDelay = 0;
		m_nFrameDelay = 0;
		m_nBufferCount = 0;
		m_dwSongFlags |= SONG_PATTERNLOOP;
	//	m_nSeqOverride = 0;
	}
}
//rewbs.playSongFromCursor
void CSoundFile::DontLoopPattern(int nPat, int nRow)
//----------------------------------------------
{
	if ((nPat < 0) || (nPat >= Patterns.Size()) || (!Patterns[nPat])) nPat = 0;
	if ((nRow < 0) || (nRow >= (int)PatternSize[nPat])) nRow = 0;
	m_nPattern = nPat;
	m_nRow = m_nNextRow = nRow;
	m_nTickCount = m_nMusicSpeed;
	m_nPatternDelay = 0;
	m_nFrameDelay = 0;
	m_nBufferCount = 0;
	m_dwSongFlags &= ~SONG_PATTERNLOOP;
	//m_nSeqOverride = 0;
}

int CSoundFile::FindOrder(PATTERNINDEX pat, UINT startFromOrder, bool direction)
//----------------------------------------------------------------------
{
	int foundAtOrder = -1;
	int candidateOrder = 0;

	for (UINT p=0; p<Order.size(); p++) 	{
		if (direction) {
			candidateOrder = (startFromOrder+p)%Order.size();		//wrap around MAX_ORDERS
		} else {
			candidateOrder = (startFromOrder-p+Order.size())%Order.size();	//wrap around 0 and MAX_ORDERS
		}
		if (Order[candidateOrder] == pat) {
			foundAtOrder = candidateOrder;
			break;
		}
	}

	return foundAtOrder;
}
//end rewbs.playSongFromCursor


UINT CSoundFile::GetBestSaveFormat() const
//----------------------------------------
{
	if ((!m_nSamples) || (!m_nChannels)) return MOD_TYPE_NONE;
	if (!m_nType) return MOD_TYPE_NONE;
	if (m_nType & (MOD_TYPE_MOD|MOD_TYPE_OKT))
		return MOD_TYPE_MOD;
	if (m_nType & (MOD_TYPE_S3M|MOD_TYPE_STM|MOD_TYPE_ULT|MOD_TYPE_FAR|MOD_TYPE_PTM))
		return MOD_TYPE_S3M;
	if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MED|MOD_TYPE_MTM|MOD_TYPE_MT2))
		return MOD_TYPE_XM;
	if(m_nType & MOD_TYPE_MPT)
		return MOD_TYPE_MPT;
	return MOD_TYPE_IT;
}


UINT CSoundFile::GetSaveFormats() const
//-------------------------------------
{
	UINT n = 0;
	if ((!m_nSamples) || (!m_nChannels) || (m_nType == MOD_TYPE_NONE)) return 0;
	switch(m_nType)
	{
	case MOD_TYPE_MOD:	n = MOD_TYPE_MOD;
	case MOD_TYPE_S3M:	n = MOD_TYPE_S3M;
	}
	n |= MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT;
	if (!m_nInstruments)
	{
		if (m_nSamples < 32) n |= MOD_TYPE_MOD;
		n |= MOD_TYPE_S3M;
	}
	return n;
}


CString CSoundFile::GetSampleName(UINT nSample) const
//--------------------------------------------------------
{
	if (nSample<MAX_SAMPLES) {
		return m_szNames[nSample];
	} else {
		return "";
	}
}


CString CSoundFile::GetInstrumentName(UINT nInstr) const
//-----------------------------------------------------------
{
	if ((nInstr >= MAX_INSTRUMENTS) || (!Headers[nInstr]))	{
		return "";
	}
	return Headers[nInstr]->name;
}


bool CSoundFile::InitChannel(UINT nch)
//-------------------------------------
{
	if(nch >= MAX_BASECHANNELS) return true;

	ChnSettings[nch].nPan = 128;
	ChnSettings[nch].nVolume = 64;
	ChnSettings[nch].dwFlags = 0;
	ChnSettings[nch].nMixPlugin = 0;
	ChnSettings[nch].szName[0] = 0;

	ResetChannelState(nch, CHNRESET_TOTAL);

	if(m_pModDoc)
	{
		m_pModDoc->Record1Channel(nch,FALSE);
		m_pModDoc->Record2Channel(nch,FALSE);
	}
	m_bChannelMuteTogglePending[nch] = false;

	return false;
}

void CSoundFile::ResetChannelState(CHANNELINDEX i, BYTE resetMask)
//-------------------------------------------------------
{
	if(i >= MAX_CHANNELS) return;

	if(resetMask & 2)
	{
		Chn[i].nNote = Chn[i].nNewNote = Chn[i].nNewIns = 0;
		Chn[i].pInstrument = NULL;
		Chn[i].pHeader = NULL;
		Chn[i].nPortamentoDest = 0;
		Chn[i].nCommand = 0;
		Chn[i].nPatternLoopCount = 0;
		Chn[i].nPatternLoop = 0;
		Chn[i].nFadeOutVol = 0;
		Chn[i].dwFlags |= CHN_KEYOFF|CHN_NOTEFADE;
		Chn[i].nTremorCount = 0;
	}

	if(resetMask & 4)
	{
		Chn[i].nPeriod = 0;
		Chn[i].nPos = Chn[i].nLength = 0;
		Chn[i].nLoopStart = 0;
		Chn[i].nLoopEnd = 0;
		Chn[i].nROfs = Chn[i].nLOfs = 0;
		Chn[i].pSample = NULL;
		Chn[i].pInstrument = NULL;
		Chn[i].pHeader = NULL;
		Chn[i].nCutOff = 0x7F;
		Chn[i].nResonance = 0;
		Chn[i].nFilterMode = 0;
		Chn[i].nLeftVol = Chn[i].nRightVol = 0;
		Chn[i].nNewLeftVol = Chn[i].nNewRightVol = 0;
		Chn[i].nLeftRamp = Chn[i].nRightRamp = 0;
		Chn[i].nVolume = 256;

		//-->Custom tuning related
		Chn[i].m_ReCalculateFreqOnFirstTick = false;
		Chn[i].m_CalculateFreq = false;
		Chn[i].m_PortamentoFineSteps = 0;
		Chn[i].m_PortamentoTickSlide = 0;
		Chn[i].m_Freq = 0;
		Chn[i].m_VibratoDepth = 0;
		//<--Custom tuning related.
	}

	if(resetMask & 1)
	{
		if(i < MAX_BASECHANNELS)
		{
			Chn[i].dwFlags = ChnSettings[i].dwFlags;
			Chn[i].nPan = ChnSettings[i].nPan;
			Chn[i].nGlobalVol = ChnSettings[i].nVolume;
		}
		else
		{
			Chn[i].dwFlags = 0;
			Chn[i].nPan = 128;
			Chn[i].nGlobalVol = 64;
		}
		Chn[i].nRestorePanOnNewNote = 0;
		Chn[i].nRestoreCutoffOnNewNote = 0;
		Chn[i].nRestoreResonanceOnNewNote = 0;
		
	}
}


CHANNELINDEX CSoundFile::ReArrangeChannels(const vector<CHANNELINDEX>& newOrder)
//-------------------------------------------------------------------
{
    //newOrder[i] tells which current channel should be placed to i:th position in
    //the new order, or if i is not an index of current channels, then new channel is
    //added to position i. If index of some current channel is missing from the
    //newOrder-vector, then the channel gets removed.
	
	UINT nRemainingChannels = newOrder.size();	

	if(nRemainingChannels > GetModSpecifications().channelsMax || nRemainingChannels < GetModSpecifications().channelsMin) 	
	{
		CString str = "Error: Bad newOrder vector in CSoundFile::ReArrangeChannels(...)";	
		CMainFrame::GetMainFrame()->MessageBox(str , "ReArrangeChannels", MB_OK | MB_ICONINFORMATION);
		return 0;
	}

	BEGIN_CRITICAL();
	UINT i = 0;
	for (i=0; i<Patterns.Size(); i++) 
	{
		if (Patterns[i])
		{
			MODCOMMAND *p = Patterns[i];
			MODCOMMAND *newp = CSoundFile::AllocatePattern(PatternSize[i], nRemainingChannels);
			if (!newp)
			{
				END_CRITICAL();
				CMainFrame::GetMainFrame()->MessageBox("ERROR: Pattern allocation failed in ReArrangechannels(...)" , "ReArrangeChannels", MB_OK | MB_ICONINFORMATION);
				return 0;
			}
			MODCOMMAND *tmpsrc = p, *tmpdest = newp;
			for (UINT j=0; j<PatternSize[i]; j++) //Scrolling rows
			{
				for (UINT k=0; k<nRemainingChannels; k++, tmpdest++) //Scrolling channels.
				{
					if(newOrder[k] < m_nChannels) //Case: getting old channel to the new channel order.
						*tmpdest = tmpsrc[j*m_nChannels+newOrder[k]];
					else //Case: figure newOrder[k] is not the index of any current channel, so adding a new channel.
						*tmpdest = MODCOMMAND();
							
				}
			}
			Patterns[i] = newp;
			CSoundFile::FreePattern(p);
		}
	}

	MODCHANNELSETTINGS settings[MAX_BASECHANNELS];
	MODCHANNEL chns[MAX_BASECHANNELS];		
	UINT recordStates[MAX_BASECHANNELS];
	bool chnMutePendings[MAX_BASECHANNELS];

	for(i = 0 ; i < m_nChannels ; i++)
	{
		settings[i] = ChnSettings[i];
		chns[i] = Chn[i];
		if(m_pModDoc)
			recordStates[i] = m_pModDoc->IsChannelRecord(i);
		chnMutePendings[i] = m_bChannelMuteTogglePending[i];
	}
	
	if(m_pModDoc)
		m_pModDoc->ReinitRecordState();

	for (UINT i=0; i<nRemainingChannels; i++)
	{
		if(newOrder[i] < m_nChannels)
		{
				ChnSettings[i] = settings[newOrder[i]];
				Chn[i] = chns[newOrder[i]];
				if(m_pModDoc)
				{
					if(recordStates[newOrder[i]] == 1) m_pModDoc->Record1Channel(i,TRUE);
					if(recordStates[newOrder[i]] == 2) m_pModDoc->Record2Channel(i,TRUE);
				}
				m_bChannelMuteTogglePending[i] = chnMutePendings[newOrder[i]];
		}
		else
		{
			InitChannel(i);
		}
	}

	m_nChannels = nRemainingChannels;
	END_CRITICAL();

	return static_cast<CHANNELINDEX>(m_nChannels);
}

bool CSoundFile::MoveChannel(UINT chnFrom, UINT chnTo)
//-----------------------------------------------------
{
    //Implementation of move channel using ReArrangeChannels(...). So this function
    //only creates correct newOrder-vector used in the ReArrangeChannels(...).
	if(chnFrom == chnTo) return false;
    if(chnFrom >= m_nChannels || chnTo >= m_nChannels)
    {
		CString str = "Error: Bad move indexes in CSoundFile::MoveChannel(...)";	
		CMainFrame::GetMainFrame()->MessageBox(str , "MoveChannel(...)", MB_OK | MB_ICONINFORMATION);
		return true;
    }
	vector<CHANNELINDEX> newOrder;
	//First creating new order identical to current order...
	for(CHANNELINDEX i = 0; i<GetNumChannels(); i++)
	{
		newOrder.push_back(i);
	}
	//...and then add the move channel effect.
	if(chnFrom < chnTo)
	{
		CHANNELINDEX temp = newOrder[chnFrom];
		for(UINT i = chnFrom; i<chnTo; i++)
		{
			newOrder[i] = newOrder[i+1];
		}
		newOrder[chnTo] = temp;
	}
	else //case chnFrom > chnTo(can't be equal, since it has been examined earlier.)
	{
		CHANNELINDEX temp = newOrder[chnFrom];
		for(UINT i = chnFrom; i>=chnTo+1; i--)
		{
			newOrder[i] = newOrder[i-1];
		}
		newOrder[chnTo] = temp;
     }

	if(newOrder.size() != ReArrangeChannels(newOrder))
	{
		CMainFrame::GetMainFrame()->MessageBox("BUG: Channel number changed in MoveChannel()" , "", MB_OK | MB_ICONINFORMATION);
	}
	return false;
}


CString CSoundFile::GetPatternViewInstrumentName(UINT nInstr, bool returnEmptyInsteadOfNoName) const
//-----------------------------------------------------------------
{
	//Default: returnEmptyInsteadOfNoName = false;
	if(nInstr >= MAX_INSTRUMENTS || m_nInstruments == 0 || Headers[nInstr] == 0) return "";

	CString displayName, instrumentName, pluginName = "";
					
	// Use instrument name
	instrumentName.Format("%s", Headers[nInstr]->name);

	if (instrumentName == "") {
		// if there's no instrument name, use name of sample associated with C-5.
 		//TODO: If there's no sample mapped to that note, we could check the other notes.
		if (Headers[nInstr]->Keyboard[60] && Ins[Headers[nInstr]->Keyboard[60]].pSample) {
			instrumentName.Format("s: %s", m_szNames[Headers[nInstr]->Keyboard[60]]); //60 is C-5
		}
	}

	//Get plugin name:
	UINT nPlug=Headers[nInstr]->nMixPlug;
	if (nPlug>0 && nPlug<MAX_MIXPLUGINS) {
		pluginName = m_MixPlugins[nPlug-1].Info.szName;
	}

	if (pluginName == "") {
		if(returnEmptyInsteadOfNoName && instrumentName == "") return "";
		if(instrumentName == "") instrumentName = "(no name)";
		displayName.Format("%02d: %s", nInstr, instrumentName);
	} else {
		displayName.Format("%02d: %s (%s)", nInstr, instrumentName, pluginName);
	}
	return displayName;
}


#ifndef NO_PACKING
UINT CSoundFile::PackSample(int &sample, int next)
//------------------------------------------------
{
	UINT i = 0;
	int delta = next - sample;
	if (delta >= 0)
	{
		for (i=0; i<7; i++) if (delta <= (int)CompressionTable[i+1]) break;
	} else
	{
		for (i=8; i<15; i++) if (delta >= (int)CompressionTable[i+1]) break;
	}
	sample += (int)CompressionTable[i];
	return i;
}


BOOL CSoundFile::CanPackSample(LPSTR pSample, UINT nLen, UINT nPacking, BYTE *result)
//-----------------------------------------------------------------------------------
{
	int pos, old, oldpos, besttable = 0;
	DWORD dwErr, dwTotal, dwResult;
	int i,j;
	
	if (result) *result = 0;
	if ((!pSample) || (nLen < 1024)) return FALSE;
	// Try packing with different tables
	dwResult = 0;
	for (j=1; j<MAX_PACK_TABLES; j++)
	{
		memcpy(CompressionTable, UnpackTable[j], 16);
		dwErr = 0;
		dwTotal = 1;
		old = pos = oldpos = 0;
		for (i=0; i<(int)nLen; i++)
		{
			int s = (int)pSample[i];
			PackSample(pos, s);
			dwErr += abs(pos - oldpos);
			dwTotal += abs(s - old);
			old = s;
			oldpos = pos;
		}
		dwErr = _muldiv(dwErr, 100, dwTotal);
		if (dwErr >= dwResult)
		{
			dwResult = dwErr;
			besttable = j;
		}
	}
	memcpy(CompressionTable, UnpackTable[besttable], 16);
	if (result)
	{
		if (dwResult > 100) *result	= 100; else *result = (BYTE)dwResult;
	}
	return (dwResult >= nPacking) ? TRUE : FALSE;
}
#endif // NO_PACKING

#ifndef MODPLUG_NO_FILESAVE

UINT CSoundFile::WriteSample(FILE *f, MODINSTRUMENT *pins, UINT nFlags, UINT nMaxLen)
//-----------------------------------------------------------------------------------
{
	UINT len = 0, bufcount;
	char buffer[4096];
	signed char *pSample = (signed char *)pins->pSample;
	UINT nLen = pins->nLength;
	
	if ((nMaxLen) && (nLen > nMaxLen)) nLen = nMaxLen;
// -> CODE#0023
// -> DESC="IT project files (.itp)"
//	if ((!pSample) || (f == NULL) || (!nLen)) return 0;
	if ((!pSample) || (!nLen)) return 0;
// NOTE : also added all needed 'if(f)' in this function
// -! NEW_FEATURE#0023
	switch(nFlags)
	{
#ifndef NO_PACKING
	// 3: 4-bit ADPCM data
	case RS_ADPCM4:
		{
			int pos; 
			len = (nLen + 1) / 2;
			if(f) fwrite(CompressionTable, 16, 1, f);
			bufcount = 0;
			pos = 0;
			for (UINT j=0; j<len; j++)
			{
				BYTE b;
				// Sample #1
				b = PackSample(pos, (int)pSample[j*2]);
				// Sample #2
				b |= PackSample(pos, (int)pSample[j*2+1]) << 4;
				buffer[bufcount++] = (char)b;
				if (bufcount >= sizeof(buffer))
				{
					if(f) fwrite(buffer, 1, bufcount, f);
					bufcount = 0;
				}
			}
			if (bufcount) if(f) fwrite(buffer, 1, bufcount, f);
			len += 16;
		}
		break;
#endif // NO_PACKING

	// 16-bit samples
	case RS_PCM16U:
	case RS_PCM16D:
	case RS_PCM16S:
		{
			short int *p = (short int *)pSample;
			int s_old = 0, s_ofs;
			len = nLen * 2;
			bufcount = 0;
			s_ofs = (nFlags == RS_PCM16U) ? 0x8000 : 0;
			for (UINT j=0; j<nLen; j++)
			{
				int s_new = *p;
				p++;
				if (pins->uFlags & CHN_STEREO)
				{
					s_new = (s_new + (*p) + 1) >> 1;
					p++;
				}
				if (nFlags == RS_PCM16D)
				{
					*((short *)(&buffer[bufcount])) = (short)(s_new - s_old);
					s_old = s_new;
				} else
				{
					*((short *)(&buffer[bufcount])) = (short)(s_new + s_ofs);
				}
				bufcount += 2;
				if (bufcount >= sizeof(buffer) - 1)
				{
					if(f) fwrite(buffer, 1, bufcount, f);
					bufcount = 0;
				}
			}
			if (bufcount) if(f) fwrite(buffer, 1, bufcount, f);
		}
		break;

	// 8-bit Stereo samples (not interleaved)
	case RS_STPCM8S:
	case RS_STPCM8U:
	case RS_STPCM8D:
		{
			int s_ofs = (nFlags == RS_STPCM8U) ? 0x80 : 0;
			for (UINT iCh=0; iCh<2; iCh++)
			{
				signed char *p = pSample + iCh;
				int s_old = 0;

				bufcount = 0;
				for (UINT j=0; j<nLen; j++)
				{
					int s_new = *p;
					p += 2;
					if (nFlags == RS_STPCM8D)
					{
						buffer[bufcount++] = (char)(s_new - s_old);
						s_old = s_new;
					} else
					{
						buffer[bufcount++] = (char)(s_new + s_ofs);
					}
					if (bufcount >= sizeof(buffer))
					{
						if(f) fwrite(buffer, 1, bufcount, f);
						bufcount = 0;
					}
				}
				if (bufcount) if(f) fwrite(buffer, 1, bufcount, f);
			}
		}
		len = nLen * 2;
		break;

	// 16-bit Stereo samples (not interleaved)
	case RS_STPCM16S:
	case RS_STPCM16U:
	case RS_STPCM16D:
		{
			int s_ofs = (nFlags == RS_STPCM16U) ? 0x8000 : 0;
			for (UINT iCh=0; iCh<2; iCh++)
			{
				signed short *p = ((signed short *)pSample) + iCh;
				int s_old = 0;

				bufcount = 0;
				for (UINT j=0; j<nLen; j++)
				{
					int s_new = *p;
					p += 2;
					if (nFlags == RS_STPCM16D)
					{
						*((short *)(&buffer[bufcount])) = (short)(s_new - s_old);
						s_old = s_new;
					} else
					{
						*((short *)(&buffer[bufcount])) = (short)(s_new + s_ofs);
					}
					bufcount += 2;
					if (bufcount >= sizeof(buffer))
					{
						if(f) fwrite(buffer, 1, bufcount, f);
						bufcount = 0;
					}
				}
				if (bufcount) if(f) fwrite(buffer, 1, bufcount, f);
			}
		}
		len = nLen*4;
		break;

	//	Stereo signed interleaved
	case RS_STIPCM8S:
	case RS_STIPCM16S:
		len = nLen * 2;
		if (nFlags == RS_STIPCM16S) len *= 2;
		if(f) fwrite(pSample, 1, len, f);
		break;

	// Default: assume 8-bit PCM data
	default:
		len = nLen;
		bufcount = 0;
		{
			signed char *p = pSample;
			int sinc = (pins->uFlags & CHN_16BIT) ? 2 : 1;
			int s_old = 0, s_ofs = (nFlags == RS_PCM8U) ? 0x80 : 0;
			if (pins->uFlags & CHN_16BIT) p++;
			for (UINT j=0; j<len; j++)
			{
				int s_new = (signed char)(*p);
				p += sinc;
				if (pins->uFlags & CHN_STEREO)
				{
					s_new = (s_new + ((int)*p) + 1) >> 1;
					p += sinc;
				}
				if (nFlags == RS_PCM8D)
				{
					buffer[bufcount++] = (char)(s_new - s_old);
					s_old = s_new;
				} else
				{
					buffer[bufcount++] = (char)(s_new + s_ofs);
				}
				if (bufcount >= sizeof(buffer))
				{
					if(f) fwrite(buffer, 1, bufcount, f);
					bufcount = 0;
				}
			}
			if (bufcount) if(f) fwrite(buffer, 1, bufcount, f);
		}
	}
	return len;
}

#endif // MODPLUG_NO_FILESAVE


// Flags:
//	0 = signed 8-bit PCM data (default)
//	1 = unsigned 8-bit PCM data
//	2 = 8-bit ADPCM data with linear table
//	3 = 4-bit ADPCM data
//	4 = 16-bit ADPCM data with linear table
//	5 = signed 16-bit PCM data
//	6 = unsigned 16-bit PCM data

UINT CSoundFile::ReadSample(MODINSTRUMENT *pIns, UINT nFlags, LPCSTR lpMemFile, DWORD dwMemLength, const WORD format)
//-----------------------------------------------------------------------------------------------------------------------
{
	UINT len = 0, mem = pIns->nLength+6;

	if ((!pIns) || (pIns->nLength < 4) || (!lpMemFile)) return 0;
	if (pIns->nLength > MAX_SAMPLE_LENGTH) pIns->nLength = MAX_SAMPLE_LENGTH;
	pIns->uFlags &= ~(CHN_16BIT|CHN_STEREO);
	if (nFlags & RSF_16BIT)
	{
		mem *= 2;
		pIns->uFlags |= CHN_16BIT;
	}
	if (nFlags & RSF_STEREO)
	{
		mem *= 2;
		pIns->uFlags |= CHN_STEREO;
	}
	if ((pIns->pSample = AllocateSample(mem)) == NULL)
	{
		pIns->nLength = 0;
		return 0;
	}
	switch(nFlags)
	{
	// 1: 8-bit unsigned PCM data
	case RS_PCM8U:
		{
			len = pIns->nLength;
			if (len > dwMemLength) len = pIns->nLength = dwMemLength;
			LPSTR pSample = pIns->pSample;
			for (UINT j=0; j<len; j++) pSample[j] = (char)(lpMemFile[j] - 0x80); 
		}
		break;

	// 2: 8-bit ADPCM data with linear table
	case RS_PCM8D:
		{
			len = pIns->nLength;
			if (len > dwMemLength) break;
			LPSTR pSample = pIns->pSample;
			const char *p = (const char *)lpMemFile;
			int delta = 0;
			for (UINT j=0; j<len; j++)
			{
				delta += p[j];
				*pSample++ = (char)delta;
			}
		}
		break;

	// 3: 4-bit ADPCM data
	case RS_ADPCM4:
		{
			len = (pIns->nLength + 1) / 2;
			if (len > dwMemLength - 16) break;
			memcpy(CompressionTable, lpMemFile, 16);
			lpMemFile += 16;
			LPSTR pSample = pIns->pSample;
			char delta = 0;
			for (UINT j=0; j<len; j++)
			{
				BYTE b0 = (BYTE)lpMemFile[j];
				BYTE b1 = (BYTE)(lpMemFile[j] >> 4);
				delta = (char)GetDeltaValue((int)delta, b0);
				pSample[0] = delta;
				delta = (char)GetDeltaValue((int)delta, b1);
				pSample[1] = delta;
				pSample += 2;
			}
			len += 16;
		}
		break;

	// 4: 16-bit ADPCM data with linear table
	case RS_PCM16D:
		{
			len = pIns->nLength * 2;
			if (len > dwMemLength) break;
			short int *pSample = (short int *)pIns->pSample;
			short int *p = (short int *)lpMemFile;
			int delta16 = 0;
			for (UINT j=0; j<len; j+=2)
			{
				delta16 += *p++;
				*pSample++ = (short int)delta16;
			}
		}
		break;

	// 5: 16-bit signed PCM data
	case RS_PCM16S:
		len = pIns->nLength * 2;
		if (len <= dwMemLength) memcpy(pIns->pSample, lpMemFile, len);
		break;

	// 16-bit signed mono PCM motorola byte order
	case RS_PCM16M:
		len = pIns->nLength * 2;
		if (len > dwMemLength) len = dwMemLength & ~1;
		if (len > 1)
		{
			char *pSample = (char *)pIns->pSample;
			char *pSrc = (char *)lpMemFile;
			for (UINT j=0; j<len; j+=2)
			{
				pSample[j] = pSrc[j+1];
				pSample[j+1] = pSrc[j];
			}
		}
		break;

	// 6: 16-bit unsigned PCM data
	case RS_PCM16U:
		{
			len = pIns->nLength * 2;
			if (len > dwMemLength) break;
			short int *pSample = (short int *)pIns->pSample;
			short int *pSrc = (short int *)lpMemFile;
			for (UINT j=0; j<len; j+=2) *pSample++ = (*(pSrc++)) - 0x8000;
		}
		break;

	// 16-bit signed stereo big endian
	case RS_STPCM16M:
		len = pIns->nLength * 2;
		if (len*2 <= dwMemLength)
		{
			char *pSample = (char *)pIns->pSample;
			char *pSrc = (char *)lpMemFile;
			for (UINT j=0; j<len; j+=2)
			{
				pSample[j*2] = pSrc[j+1];
				pSample[j*2+1] = pSrc[j];
				pSample[j*2+2] = pSrc[j+1+len];
				pSample[j*2+3] = pSrc[j+len];
			}
			len *= 2;
		}
		break;

	// 8-bit stereo samples
	case RS_STPCM8S:
	case RS_STPCM8U:
	case RS_STPCM8D:
		{
			len = pIns->nLength;
			char *psrc = (char *)lpMemFile;
			char *pSample = (char *)pIns->pSample;
			if (len*2 > dwMemLength) break;
			for (UINT c=0; c<2; c++)
			{
				int iadd = 0;
				if (nFlags == RS_STPCM8U) iadd = -128;
				if (nFlags == RS_STPCM8D)
				{
					for (UINT j=0; j<len; j++)
					{
						iadd += psrc[0];
						psrc++;
						pSample[j*2] = (char)iadd;
					}
				} else
				{
					for (UINT j=0; j<len; j++)
					{
						pSample[j*2] = (char)(psrc[0] + iadd);
						psrc++;
					}
				}
				pSample++;
			}
			len *= 2;
		}
		break;

	// 16-bit stereo samples
	case RS_STPCM16S:
	case RS_STPCM16U:
	case RS_STPCM16D:
		{
			len = pIns->nLength;
			short int *psrc = (short int *)lpMemFile;
			short int *pSample = (short int *)pIns->pSample;
			if (len*4 > dwMemLength) break;
			for (UINT c=0; c<2; c++)
			{
				int iadd = 0;
				if (nFlags == RS_STPCM16U) iadd = -0x8000;
				if (nFlags == RS_STPCM16D)
				{
					for (UINT j=0; j<len; j++)
					{
						iadd += psrc[0];
						psrc++;
						pSample[j*2] = (short int)iadd;
					}
				} else
				{
					for (UINT j=0; j<len; j++)
					{
						pSample[j*2] = (short int) (psrc[0] + iadd);
						psrc++;
					}
				}
				pSample++;
			}
			len *= 4;
		}
		break;

	// IT 2.14 compressed samples
	case RS_IT2148:
	case RS_IT21416:
	case RS_IT2158:
	case RS_IT21516:
		len = dwMemLength;
		if (len < 4) break;
		if ((nFlags == RS_IT2148) || (nFlags == RS_IT2158))
			ITUnpack8Bit(pIns->pSample, pIns->nLength, (LPBYTE)lpMemFile, dwMemLength, (nFlags == RS_IT2158));
		else
			ITUnpack16Bit(pIns->pSample, pIns->nLength, (LPBYTE)lpMemFile, dwMemLength, (nFlags == RS_IT21516));
		break;

#ifndef MODPLUG_BASIC_SUPPORT
#ifndef FASTSOUNDLIB
	// 8-bit interleaved stereo samples
	case RS_STIPCM8S:
	case RS_STIPCM8U:
		{
			int iadd = 0;
			if (nFlags == RS_STIPCM8U) { iadd = -0x80; }
			len = pIns->nLength;
			if (len*2 > dwMemLength) len = dwMemLength >> 1;
			LPBYTE psrc = (LPBYTE)lpMemFile;
			LPBYTE pSample = (LPBYTE)pIns->pSample;
			for (UINT j=0; j<len; j++)
			{
				pSample[j*2] = (char)(psrc[0] + iadd);
				pSample[j*2+1] = (char)(psrc[1] + iadd);
				psrc+=2;
			}
			len *= 2;
		}
		break;

	// 16-bit interleaved stereo samples
	case RS_STIPCM16S:
	case RS_STIPCM16U:
		{
			int iadd = 0;
			if (nFlags == RS_STIPCM16U) iadd = -32768;
			len = pIns->nLength;
			if (len*4 > dwMemLength) len = dwMemLength >> 2;
			short int *psrc = (short int *)lpMemFile;
			short int *pSample = (short int *)pIns->pSample;
			for (UINT j=0; j<len; j++)
			{
				pSample[j*2] = (short int)(psrc[0] + iadd);
				pSample[j*2+1] = (short int)(psrc[1] + iadd);
				psrc += 2;
			}
			len *= 4;
		}
		break;

	// AMS compressed samples
	case RS_AMS8:
	case RS_AMS16:
		len = 9;
		if (dwMemLength > 9)
		{
			const char *psrc = lpMemFile;
			char packcharacter = lpMemFile[8], *pdest = (char *)pIns->pSample;
			len += *((LPDWORD)(lpMemFile+4));
			if (len > dwMemLength) len = dwMemLength;
			UINT dmax = pIns->nLength;
			if (pIns->uFlags & CHN_16BIT) dmax <<= 1;
			AMSUnpack(psrc+9, len-9, pdest, dmax, packcharacter);
		}
		break;

	// PTM 8bit delta to 16-bit sample
	case RS_PTM8DTO16:
		{
			len = pIns->nLength * 2;
			if (len > dwMemLength) break;
			signed char *pSample = (signed char *)pIns->pSample;
			signed char delta8 = 0;
			for (UINT j=0; j<len; j++)
			{
				delta8 += lpMemFile[j];
				*pSample++ = delta8;
			}
		}
		break;

	// Huffman MDL compressed samples
	case RS_MDL8:
	case RS_MDL16:
		len = dwMemLength;
		if (len >= 4)
		{
			LPBYTE pSample = (LPBYTE)pIns->pSample;
			LPBYTE ibuf = (LPBYTE)lpMemFile;
			DWORD bitbuf = *((DWORD *)ibuf);
			UINT bitnum = 32;
			BYTE dlt = 0, lowbyte = 0;
			ibuf += 4;
			for (UINT j=0; j<pIns->nLength; j++)
			{
				BYTE hibyte;
				BYTE sign;
				if (nFlags == RS_MDL16) lowbyte = (BYTE)MDLReadBits(bitbuf, bitnum, ibuf, 8);
				sign = (BYTE)MDLReadBits(bitbuf, bitnum, ibuf, 1);
				if (MDLReadBits(bitbuf, bitnum, ibuf, 1))
				{
					hibyte = (BYTE)MDLReadBits(bitbuf, bitnum, ibuf, 3);
				} else
				{
					hibyte = 8;
					while (!MDLReadBits(bitbuf, bitnum, ibuf, 1)) hibyte += 0x10;
					hibyte += MDLReadBits(bitbuf, bitnum, ibuf, 4);
				}
				if (sign) hibyte = ~hibyte;
				dlt += hibyte;
				if (nFlags != RS_MDL16)
					pSample[j] = dlt;
				else
				{
					pSample[j<<1] = lowbyte;
					pSample[(j<<1)+1] = dlt;
				}
			}
		}
		break;

	case RS_DMF8:
	case RS_DMF16:
		len = dwMemLength;
		if (len >= 4)
		{
			UINT maxlen = pIns->nLength;
			if (pIns->uFlags & CHN_16BIT) maxlen <<= 1;
			LPBYTE ibuf = (LPBYTE)lpMemFile, ibufmax = (LPBYTE)(lpMemFile+dwMemLength);
			len = DMFUnpack((LPBYTE)pIns->pSample, ibuf, ibufmax, maxlen);
		}
		break;

#ifdef MODPLUG_TRACKER
	// Mono PCM 24/32-bit signed & 32 bit float -> load sample, and normalize it to 16-bit
	case RS_PCM24S:
	case RS_PCM32S:
		len = pIns->nLength * 3;
		if (nFlags == RS_PCM32S) len += pIns->nLength;
		if (len > dwMemLength) break;
		if (len > 4*8)
		{
			if(nFlags == RS_PCM24S)
			{
				char* pSrc = (char*)lpMemFile;
				char* pDest = (char*)pIns->pSample;
				CopyWavBuffer<3, 2, WavSigned24To16, MaxFinderSignedInt<3> >(pSrc, len, pDest, pIns->GetSampleSizeInBytes());
			}
			else //RS_PCM32S
			{
				char* pSrc = (char*)lpMemFile;
				char* pDest = (char*)pIns->pSample;
				if(format == 3)
					CopyWavBuffer<4, 2, WavFloat32To16, MaxFinderFloat32>(pSrc, len, pDest, pIns->GetSampleSizeInBytes());
				else
					CopyWavBuffer<4, 2, WavSigned32To16, MaxFinderSignedInt<4> >(pSrc, len, pDest, pIns->GetSampleSizeInBytes());
			}
		}
		break;

	// Stereo PCM 24/32-bit signed & 32 bit float -> convert sample to 16 bit
	case RS_STIPCM24S:
	case RS_STIPCM32S:
		if(format == 3 && nFlags == RS_STIPCM32S) //Microsoft IEEE float
		{
			len = pIns->nLength * 6;
			//pIns->nLength tells(?) the number of frames there
			//are. One 'frame' of 1 byte(== 8 bit) mono data requires
			//1 byte of space, while one frame of 3 byte(24 bit) 
			//stereo data requires 3*2 = 6 bytes. This is(?)
			//why there is factor 6.

			len += pIns->nLength * 2;
			//Compared to 24 stereo, 32 bit stereo needs 16 bits(== 2 bytes)
			//more per frame.

			if(len > dwMemLength) break;
			char* pSrc = (char*)lpMemFile;
			char* pDest = (char*)pIns->pSample;
			if (len > 8*8)
			{
				CopyWavBuffer<4, 2, WavFloat32To16, MaxFinderFloat32>(pSrc, len, pDest, pIns->GetSampleSizeInBytes());
			}
		}
		else
		{
			len = pIns->nLength * 6;
			if (nFlags == RS_STIPCM32S) len += pIns->nLength * 2;
			if (len > dwMemLength) break;
			if (len > 8*8)
			{
				char* pSrc = (char*)lpMemFile;
				char* pDest = (char*)pIns->pSample;
				if(nFlags == RS_STIPCM32S)
				{
					CopyWavBuffer<4,2,WavSigned32To16, MaxFinderSignedInt<4> >(pSrc, len, pDest, pIns->GetSampleSizeInBytes());
				}
				if(nFlags == RS_STIPCM24S)
				{
					CopyWavBuffer<3,2,WavSigned24To16, MaxFinderSignedInt<3> >(pSrc, len, pDest, pIns->GetSampleSizeInBytes());
				}

			}
		}
		break;

	// 16-bit signed big endian interleaved stereo
	case RS_STIPCM16M:
		{
			len = pIns->nLength;
			if (len*4 > dwMemLength) len = dwMemLength >> 2;
			LPCBYTE psrc = (LPCBYTE)lpMemFile;
			short int *pSample = (short int *)pIns->pSample;
			for (UINT j=0; j<len; j++)
			{
				pSample[j*2] = (signed short)(((UINT)psrc[0] << 8) | (psrc[1]));
				pSample[j*2+1] = (signed short)(((UINT)psrc[2] << 8) | (psrc[3]));
				psrc += 4;
			}
			len *= 4;
		}
		break;

#endif // MODPLUG_TRACKER
#endif // !FASTSOUNDLIB
#endif // !MODPLUG_BASIC_SUPPORT

	// Default: 8-bit signed PCM data
	default:
		len = pIns->nLength;
		if (len > dwMemLength) len = pIns->nLength = dwMemLength;
		memcpy(pIns->pSample, lpMemFile, len);
	}
	if (len > dwMemLength)
	{
		if (pIns->pSample)
		{
			pIns->nLength = 0;
			FreeSample(pIns->pSample);
			pIns->pSample = NULL;
		}
		return 0;
	}
	AdjustSampleLoop(pIns);
	return len;
}


void CSoundFile::AdjustSampleLoop(MODINSTRUMENT *pIns)
//----------------------------------------------------
{
	if ((!pIns->pSample) || (!pIns->nLength)) return;
	if (pIns->nLoopEnd > pIns->nLength) pIns->nLoopEnd = pIns->nLength;
	if (pIns->nLoopStart+2 >= pIns->nLoopEnd)
	{
		pIns->nLoopStart = pIns->nLoopEnd = 0;
		pIns->uFlags &= ~CHN_LOOP;
	}
	UINT len = pIns->nLength;
	if (pIns->uFlags & CHN_16BIT)
	{
		short int *pSample = (short int *)pIns->pSample;
		// Adjust end of sample
		if (pIns->uFlags & CHN_STEREO)
		{
			pSample[len*2+6] = pSample[len*2+4] = pSample[len*2+2] = pSample[len*2] = pSample[len*2-2];
			pSample[len*2+7] = pSample[len*2+5] = pSample[len*2+3] = pSample[len*2+1] = pSample[len*2-1];
		} else
		{
			pSample[len+4] = pSample[len+3] = pSample[len+2] = pSample[len+1] = pSample[len] = pSample[len-1];
		}
		if ((pIns->uFlags & (CHN_LOOP|CHN_PINGPONGLOOP|CHN_STEREO)) == CHN_LOOP)
		{
			// Fix bad loops
			if ((pIns->nLoopEnd+3 >= pIns->nLength) || (m_nType & MOD_TYPE_S3M))
			{
				pSample[pIns->nLoopEnd] = pSample[pIns->nLoopStart];
				pSample[pIns->nLoopEnd+1] = pSample[pIns->nLoopStart+1];
				pSample[pIns->nLoopEnd+2] = pSample[pIns->nLoopStart+2];
				pSample[pIns->nLoopEnd+3] = pSample[pIns->nLoopStart+3];
				pSample[pIns->nLoopEnd+4] = pSample[pIns->nLoopStart+4];
			}
		}
	} else
	{
		LPSTR pSample = pIns->pSample;
#ifndef FASTSOUNDLIB
		// Crappy samples (except chiptunes) ?
		if ((pIns->nLength > 0x100) && (m_nType & (MOD_TYPE_MOD|MOD_TYPE_S3M))
		 && (!(pIns->uFlags & CHN_STEREO)))
		{
			int smpend = pSample[pIns->nLength-1], smpfix = 0, kscan;
			for (kscan=pIns->nLength-1; kscan>0; kscan--)
			{
				smpfix = pSample[kscan-1];
				if (smpfix != smpend) break;
			}
			int delta = smpfix - smpend;
			if (((!(pIns->uFlags & CHN_LOOP)) || (kscan > (int)pIns->nLoopEnd))
			 && ((delta < -8) || (delta > 8)))
			{
				while (kscan<(int)pIns->nLength)
				{
					if (!(kscan & 7))
					{
						if (smpfix > 0) smpfix--;
						if (smpfix < 0) smpfix++;
					}
					pSample[kscan] = (char)smpfix;
					kscan++;
				}
			}
		}
#endif
		// Adjust end of sample
		if (pIns->uFlags & CHN_STEREO)
		{
			pSample[len*2+6] = pSample[len*2+4] = pSample[len*2+2] = pSample[len*2] = pSample[len*2-2];
			pSample[len*2+7] = pSample[len*2+5] = pSample[len*2+3] = pSample[len*2+1] = pSample[len*2-1];
		} else
		{
			pSample[len+4] = pSample[len+3] = pSample[len+2] = pSample[len+1] = pSample[len] = pSample[len-1];
		}
		if ((pIns->uFlags & (CHN_LOOP|CHN_PINGPONGLOOP|CHN_STEREO)) == CHN_LOOP)
		{
			if ((pIns->nLoopEnd+3 >= pIns->nLength) || (m_nType & (MOD_TYPE_MOD|MOD_TYPE_S3M)))
			{
				pSample[pIns->nLoopEnd] = pSample[pIns->nLoopStart];
				pSample[pIns->nLoopEnd+1] = pSample[pIns->nLoopStart+1];
				pSample[pIns->nLoopEnd+2] = pSample[pIns->nLoopStart+2];
				pSample[pIns->nLoopEnd+3] = pSample[pIns->nLoopStart+3];
				pSample[pIns->nLoopEnd+4] = pSample[pIns->nLoopStart+4];
			}
		}
	}
}


/////////////////////////////////////////////////////////////
// Transpose <-> Frequency conversions

// returns 8363*2^((transp*128+ftune)/(12*128))
DWORD CSoundFile::TransposeToFrequency(int transp, int ftune)
//-----------------------------------------------------------
{
	const float _fbase = 8363;
	const float _factor = 1.0f/(12.0f*128.0f);
	int result;
	DWORD freq;

	transp = (transp << 7) + ftune;
	_asm {
	fild transp
	fld _factor
	fmulp st(1), st(0)
	fist result
	fisub result
	f2xm1
	fild result
	fld _fbase
	fscale
	fstp st(1)
	fmul st(1), st(0)
	faddp st(1), st(0)
	fistp freq
	}
	UINT derr = freq % 11025;
	if (derr <= 8) freq -= derr;
	if (derr >= 11015) freq += 11025-derr;
	derr = freq % 1000;
	if (derr <= 5) freq -= derr;
	if (derr >= 995) freq += 1000-derr;
	return freq;
}


// returns 12*128*log2(freq/8363)
int CSoundFile::FrequencyToTranspose(DWORD freq)
//----------------------------------------------
{
	const float _f1_8363 = 1.0f / 8363.0f;
	const float _factor = 128 * 12;
	LONG result;
	
	if (!freq) return 0;
	_asm {
	fld _factor
	fild freq
	fld _f1_8363
	fmulp st(1), st(0)
	fyl2x
	fistp result
	}
	return result;
}


void CSoundFile::FrequencyToTranspose(MODINSTRUMENT *psmp)
//--------------------------------------------------------
{
	int f2t = FrequencyToTranspose(psmp->nC4Speed);
	int transp = f2t >> 7;
	int ftune = f2t & 0x7F; //0x7F == 111 1111
	if (ftune > 80)
	{
		transp++;
		ftune -= 128;
	}
	if (transp > 127) transp = 127;
	if (transp < -127) transp = -127;
	psmp->RelativeTone = transp;
	psmp->nFineTune = ftune;
}


void CSoundFile::CheckCPUUsage(UINT nCPU)
//---------------------------------------
{
	if (nCPU > 100) nCPU = 100;
	gnCPUUsage = nCPU;
	if (nCPU < 90)
	{
		m_dwSongFlags &= ~SONG_CPUVERYHIGH;
	} else
	if ((m_dwSongFlags & SONG_CPUVERYHIGH) && (nCPU >= 94))
	{
		UINT i=MAX_CHANNELS;
		while (i >= 8)
		{
			i--;
			if (Chn[i].nLength)
			{
				Chn[i].nLength = Chn[i].nPos = 0;
				nCPU -= 2;
				if (nCPU < 94) break;
			}
		}
	} else
	if (nCPU > 90)
	{
		m_dwSongFlags |= SONG_CPUVERYHIGH;
	}
}


BOOL CSoundFile::SetPatternName(UINT nPat, LPCSTR lpszName)
//---------------------------------------------------------
{
	CHAR szName[MAX_PATTERNNAME] = "";
	if (nPat >= Patterns.Size()) return FALSE;
	if (lpszName) lstrcpyn(szName, lpszName, MAX_PATTERNNAME);
	szName[MAX_PATTERNNAME-1] = 0;
	if (!m_lpszPatternNames) m_nPatternNames = 0;
	if (nPat >= m_nPatternNames)
	{
		if (!lpszName[0]) return TRUE;
		UINT len = (nPat+1)*MAX_PATTERNNAME;
		CHAR *p = new CHAR[len];
		if (!p) return FALSE;
		memset(p, 0, len);
		if (m_lpszPatternNames)
		{
			memcpy(p, m_lpszPatternNames, m_nPatternNames * MAX_PATTERNNAME);
			delete[] m_lpszPatternNames;
			m_lpszPatternNames = NULL;
		}
		m_lpszPatternNames = p;
		m_nPatternNames = nPat + 1;
	}
	memcpy(m_lpszPatternNames + nPat * MAX_PATTERNNAME, szName, MAX_PATTERNNAME);
	return TRUE;
}


BOOL CSoundFile::GetPatternName(UINT nPat, LPSTR lpszName, UINT cbSize) const
//---------------------------------------------------------------------------
{
	if ((!lpszName) || (!cbSize)) return FALSE;
	lpszName[0] = 0;
	if (cbSize > MAX_PATTERNNAME) cbSize = MAX_PATTERNNAME;
	if ((m_lpszPatternNames) && (nPat < m_nPatternNames))
	{
		memcpy(lpszName, m_lpszPatternNames + nPat * MAX_PATTERNNAME, cbSize);
		lpszName[cbSize-1] = 0;
		return TRUE;
	}
	return FALSE;
}


#ifndef FASTSOUNDLIB

UINT CSoundFile::DetectUnusedSamples(BYTE *pbIns)
//-----------------------------------------------
{
	UINT nExt = 0;

	if (!pbIns) return 0;
	if (m_nInstruments)
	{
		memset(pbIns, 0, (MAX_SAMPLES+7)/8);
		for (UINT ipat=0; ipat<Patterns.Size(); ipat++)
		{
			MODCOMMAND *p = Patterns[ipat];
			if (p)
			{
				UINT jmax = PatternSize[ipat] * m_nChannels;
				for (UINT j=0; j<jmax; j++, p++)
				{
					if ((p->note) && (p->note <= NOTE_MAX))
					{
						if ((p->instr) && (p->instr < MAX_INSTRUMENTS))
						{
							INSTRUMENTHEADER *penv = Headers[p->instr];
							if (penv)
							{
								UINT n = penv->Keyboard[p->note-1];
								if (n < MAX_SAMPLES) pbIns[n>>3] |= 1<<(n&7);
							}
						} else
						{
							for (UINT k=1; k<=m_nInstruments; k++)
							{
								INSTRUMENTHEADER *penv = Headers[k];
								if (penv)
								{
									UINT n = penv->Keyboard[p->note-1];
									if (n < MAX_SAMPLES) pbIns[n>>3] |= 1<<(n&7);
								}
							}
						}
					}
				}
			}
		}
		for (UINT ichk=1; ichk<=m_nSamples; ichk++)
		{
			if ((0 == (pbIns[ichk>>3]&(1<<(ichk&7)))) && (Ins[ichk].pSample)) nExt++;
		}
	}
	return nExt;
}


BOOL CSoundFile::RemoveSelectedSamples(BOOL *pbIns)
//-------------------------------------------------
{
	if (!pbIns) return FALSE;
	for (UINT j=1; j<MAX_SAMPLES; j++)
	{
		if ((!pbIns[j]) && (Ins[j].pSample))
		{
			DestroySample(j);
			if ((j == m_nSamples) && (j > 1)) m_nSamples--;
		}
	}
	return TRUE;
}


BOOL CSoundFile::DestroySample(UINT nSample)
//------------------------------------------
{
	if ((!nSample) || (nSample >= MAX_SAMPLES)) return FALSE;
	if (!Ins[nSample].pSample) return TRUE;
	MODINSTRUMENT *pins = &Ins[nSample];
	LPSTR pSample = pins->pSample;
	pins->pSample = NULL;
	pins->nLength = 0;
	pins->uFlags &= ~(CHN_16BIT);
	for (UINT i=0; i<MAX_CHANNELS; i++)
	{
		if (Chn[i].pSample == pSample)
		{
			Chn[i].nPos = Chn[i].nLength = 0;
			Chn[i].pSample = Chn[i].pCurrentSample = NULL;
		}
	}
	FreeSample(pSample);
	return TRUE;
}

// -> CODE#0020
// -> DESC="rearrange sample list"
BOOL CSoundFile::MoveSample(UINT from, UINT to)
//---------------------------------------------
{
	if (!from || from >= MAX_SAMPLES || !to || to >= MAX_SAMPLES) return FALSE;
	if (!Ins[from].pSample || Ins[to].pSample) return TRUE;

	MODINSTRUMENT *pinsf = &Ins[from];
	MODINSTRUMENT *pinst = &Ins[to];

	memcpy(pinst,pinsf,sizeof(MODINSTRUMENT));

	pinsf->pSample = NULL;
	pinsf->nLength = 0;
	pinsf->uFlags &= ~(CHN_16BIT);

	return TRUE;
}
// -! NEW_FEATURE#0020
#endif // FASTSOUNDLIB

//rewbs.plugDocAware
/*PSNDMIXPLUGIN CSoundFile::GetSndPlugMixPlug(IMixPlugin *pPlugin) 
{
	for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
	{
		if (m_MixPlugins[iPlug].pMixPlugin == pPlugin)
			return &(m_MixPlugins[iPlug]);
	}
	
	return NULL;
}*/
//end rewbs.plugDocAware


void CSoundFile::BuildDefaultInstrument() 
//---------------------------------------
{
// m_defaultInstrument is currently only used to get default values for extented properties. 
// In the future we can make better use of this.
	memset(&m_defaultInstrument, 0, sizeof(INSTRUMENTHEADER));
	m_defaultInstrument.nResampling = SRCMODE_DEFAULT;
	m_defaultInstrument.nFilterMode = FLTMODE_UNCHANGED;
	m_defaultInstrument.nPPC = 5*12;
	m_defaultInstrument.nGlobalVol=64;
	m_defaultInstrument.nPan = 0x20 << 2;
	m_defaultInstrument.nIFC = 0xFF;
	m_defaultInstrument.nPanEnvReleaseNode=ENV_RELEASE_NODE_UNSET;
	m_defaultInstrument.nPitchEnvReleaseNode=ENV_RELEASE_NODE_UNSET;
	m_defaultInstrument.nVolEnvReleaseNode=ENV_RELEASE_NODE_UNSET;
	m_defaultInstrument.wPitchToTempoLock = 0;
	m_defaultInstrument.pTuning = m_defaultInstrument.s_DefaultTuning;
	m_defaultInstrument.nPluginVelocityHandling = PLUGIN_VELOCITYHANDLING_CHANNEL;
	m_defaultInstrument.nPluginVolumeHandling = PLUGIN_VOLUMEHANDLING_DRYWET;
}


void CSoundFile::DeleteStaticdata()
//---------------------------------
{
	delete s_pTuningsSharedLocal; s_pTuningsSharedLocal = 0;
	delete s_pTuningsSharedStandard; s_pTuningsSharedStandard = 0;
}

bool CSoundFile::SaveStaticTunings()
//----------------------------------
{
	if(s_pTuningsSharedLocal->Serialize())
	{
		ErrorBox(IDS_ERR_TUNING_SERIALISATION, NULL);
		return true;
	}
	return false;
}

void SimpleMessageBox(const char* message, const char* title)
//-----------------------------------------------------------
{
	MessageBox(0, message, title, MB_ICONINFORMATION);
}

bool CSoundFile::LoadStaticTunings()
//-----------------------------------
{
	if(s_pTuningsSharedLocal || s_pTuningsSharedStandard) return true;
	//For now not allowing to reload tunings(one should be careful when reloading them
	//since various parts may use addresses of the tuningobjects).

	CTuning::MessageHandler = &SimpleMessageBox;
	
	srlztn::CSSBSerialization::s_DefaultReadLogMask = CMainFrame::GetPrivateProfileLong("Misc", "ReadLogMask", srlztn::SNT_FAILURE | (1<<14) | (1<<13), theApp.GetConfigFileName());
	srlztn::CSSBSerialization::s_DefaultWriteLogMask = CMainFrame::GetPrivateProfileLong("Misc", "WriteLogMask", srlztn::SNT_FAILURE | srlztn::SNT_NOTE | srlztn::SNT_WARNING, theApp.GetConfigFileName());
	
	s_pTuningsSharedStandard = new CTuningCollection("Standard tunings");
	s_pTuningsSharedLocal = new CTuningCollection("Local tunings");

	CTuning::AddCreationfunction(&CTuningRTI::CreateRTITuning);
	
	const string exeDir = CMainFrame::m_csExecutableDirectoryPath;
    const string baseDirectoryName = exeDir + "tunings\\";
	string filenameBase;
	string filename;

	s_pTuningsSharedStandard->SetSavefilePath(baseDirectoryName + string("standard\\std_tunings") + CTuningCollection::s_FileExtension);
	s_pTuningsSharedLocal->SetSavefilePath(baseDirectoryName + string("local_tunings") + CTuningCollection::s_FileExtension);

	s_pTuningsSharedStandard->Unserialize();
	s_pTuningsSharedLocal->Unserialize();

	//This condition should not be true.
	if(s_pTuningsSharedStandard->GetNumTunings() == 0)
	{
		CTuningRTI* pT = new CTuningRTI;
		//Note: Tuning collection class handles deleting.
		pT->CreateGeometric(1,1);
		if(s_pTuningsSharedStandard->AddTuning(pT))
			delete pT;
	}

	//Enabling adding/removing of tunings for standard collection
	//only for debug builds.
	#ifdef DEBUG
		s_pTuningsSharedStandard->SetConstStatus(CTuningCollection::EM_ALLOWALL);
	#else
		s_pTuningsSharedStandard->SetConstStatus(CTuningCollection::EM_CONST);
	#endif

	INSTRUMENTHEADER::s_DefaultTuning = NULL;

	return false;
}



void CSoundFile::SetDefaultInstrumentValues(INSTRUMENTHEADER *penv) 
//-----------------------------------------------------------------
{
	penv->nResampling = m_defaultInstrument.nResampling;
	penv->nFilterMode = m_defaultInstrument.nFilterMode;
	penv->nPitchEnvReleaseNode = m_defaultInstrument.nPitchEnvReleaseNode;
	penv->nPanEnvReleaseNode = m_defaultInstrument.nPanEnvReleaseNode;
	penv->nVolEnvReleaseNode = m_defaultInstrument.nVolEnvReleaseNode;
	penv->pTuning = m_defaultInstrument.pTuning;
	penv->nPluginVelocityHandling = m_defaultInstrument.nPluginVelocityHandling;
	penv->nPluginVolumeHandling = m_defaultInstrument.nPluginVolumeHandling;

}



long CSoundFile::GetSampleOffset() 
//-------------------------------
{
	//TODO: This is where we could inform patterns of the exact song position when playback starts.
	//order: m_nNextPattern
	//long ticksFromStartOfPattern = m_nRow*m_nMusicSpeed;
	//return ticksFromStartOfPattern*m_nSamplesPerTick;
	return 0;
}

string CSoundFile::GetNoteName(const CTuning::NOTEINDEXTYPE& note, const int inst) const
//----------------------------------------------------------------------------------
{
	if(inst >= MAX_INSTRUMENTS || inst < -1) return "BUG";
	if(inst == -1)
		return string(szNoteNames[abs(note-1)%12]) + Stringify((note-1)/12);
	
	if(m_nType == MOD_TYPE_MPT && Headers[inst] && Headers[inst]->pTuning)
		return Headers[inst]->pTuning->GetNoteName(note-NOTE_MIDDLEC);
	else
		return string(szNoteNames[abs(note-1)%12]) + Stringify((note-1)/12);
}


void CSoundFile::SetModSpecsPointer(const CModSpecifications*& pModSpecs, const MODTYPE type)
//------------------------------------------------------------------------------------------
{
	switch(type)
	{
		case MOD_TYPE_MPT:
			pModSpecs = &MPTM_SPECS;
		break;

		case MOD_TYPE_IT:
			pModSpecs = &IT_MPTEXT_SPECS;
		break;

		case MOD_TYPE_XM:
			pModSpecs = &XM_MPTEXT_SPECS;
		break;

		case MOD_TYPE_S3M:
			pModSpecs = &S3M_MPTEXT_SPECS;
		break;

		case MOD_TYPE_MOD:
		default:
			pModSpecs = &MOD_MPTEXT_SPECS;
			break;
	}
}

uint16 CSoundFile::GetModFlagMask(const MODTYPE oldtype, const MODTYPE newtype) const
//-----------------------------------------------------------------------------------
{
	if(oldtype == MOD_TYPE_IT)
	{
		if(newtype == MOD_TYPE_MPT) return 65535;
		if(newtype == MOD_TYPE_XM) return (1 << MSF_MIDICC_BUGEMULATION);
		return 0;
	}

	if(oldtype == MOD_TYPE_MPT)
	{
		if(newtype == MOD_TYPE_IT) return 65535;
		if(newtype == MOD_TYPE_XM) return (1 << MSF_MIDICC_BUGEMULATION);
		return 0;
	}

	if(oldtype == MOD_TYPE_XM && (newtype == MOD_TYPE_IT || newtype == MOD_TYPE_MPT))
		return (1 << MSF_MIDICC_BUGEMULATION);

	return 0;
}

void CSoundFile::ChangeModTypeTo(const MODTYPE& newType)
//---------------------------------------------------
{
	const MODTYPE oldtype = m_nType;
	m_nType = newType;
	SetModSpecsPointer(m_pModSpecs, m_nType);

	m_ModFlags = m_ModFlags & GetModFlagMask(oldtype, newType);

	Order.OnModTypeChanged(oldtype);
	Patterns.OnModTypeChanged(oldtype);
}


bool CSoundFile::SetTitle(const char* titleCandidate, size_t strSize)
//-------------------------------------------------------------------
{
	if(strcmp(m_szNames[0], titleCandidate))
	{
		memset(m_szNames[0], 0, sizeof(m_szNames[0]));
		memcpy(m_szNames[0], titleCandidate, min(sizeof(m_szNames[0])-1, strSize));
		return true;
	}
	return false;
}

double CSoundFile::GetPlaybackTimeAt(ORDERINDEX ord, ROWINDEX row)
//----------------------------------------------------------------
{
	bool targetReached = false;
	const double t = GetLength(targetReached, FALSE, TRUE, ord, row);
	if(targetReached) return t;
	else return -1; //Given position not found from play sequence.
}


const CModSpecifications& CSoundFile::GetModSpecifications(const MODTYPE type)
//----------------------------------------------------------------------------
{
	const CModSpecifications* p = 0;
	SetModSpecsPointer(p, type);
	return *p;
}

