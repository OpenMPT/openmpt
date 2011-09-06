/*
 * OpenMPT
 *
 * Sndfile.cpp
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *          OpenMPT devs
*/

#include "stdafx.h"
#include "../mptrack/mptrack.h"
#include "../mptrack/mainfrm.h"
#include "../mptrack/moddoc.h"
#include "../mptrack/version.h"
#include "../mptrack/serialization_utils.h"
#include "sndfile.h"
#include "wavConverter.h"
#include "tuningcollection.h"
#include <vector>
#include <list>
#include <algorithm>

#define str_SampleAllocationError	(GetStrI18N(_TEXT("Sample allocation error")))
#define str_Error					(GetStrI18N(_TEXT("Error")))

#ifndef NO_COPYRIGHT
#ifndef NO_MMCMP_SUPPORT
#define MMCMP_SUPPORT
#endif // NO_MMCMP_SUPPORT
#ifndef NO_ARCHIVE_SUPPORT
#define UNRAR_SUPPORT
#define UNLHA_SUPPORT
#define UNGZIP_SUPPORT
#define ZIPPED_MOD_SUPPORT
LPCSTR glpszModExtensions = "mod|s3m|xm|it|stm|nst|ult|669|wow|mtm|med|far|mdl|ams|dsm|amf|okt|dmf|ptm|psm|mt2|umx|gdm|imf|j2b"
#ifndef NO_UNMO3_SUPPORT
"|mo3"
#endif
;
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

#ifdef UNGZIP_SUPPORT
#include "../ungzip/ungzip.h"
#endif

#ifdef MMCMP_SUPPORT
extern BOOL MMCMP_Unpack(LPCBYTE *ppMemFile, LPDWORD pdwMemLength);
#endif

// External decompressors
extern void AMSUnpack(const char *psrc, UINT inputlen, char *pdest, UINT dmax, char packcharacter);
extern WORD MDLReadBits(DWORD &bitbuf, UINT &bitnum, LPBYTE &ibuf, CHAR n);
extern int DMFUnpack(LPBYTE psample, uint8 *ibuf, uint8 *ibufmax, UINT maxlen);
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
MODULAR (in/out) MODINSTRUMENT :
-----------------------------------------------------------------------------------------------

* to update:
------------

- both following functions need to be updated when adding a new member in MODINSTRUMENT :

void WriteInstrumentHeaderStruct(MODINSTRUMENT * input, FILE * file);
BYTE * GetInstrumentHeaderFieldPointer(MODINSTRUMENT * input, __int32 fcode, __int16 fsize);

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

Example with "PanEnv.nLoopEnd" , "PitchEnv.nLoopEnd" & "VolEnv.Values[MAX_ENVPOINTS]" members : 
- use 'PLE.' for PanEnv.nLoopEnd
- use 'PiLE' for PitchEnv.nLoopEnd
- use 'VE[.' for VolEnv.Values[MAX_ENVPOINTS]


* In use CODE tag dictionary (alphabetical order): [ see in Sndfile.cpp ]
-------------------------------------------------------------------------

						!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						!!! SECTION TO BE UPDATED !!!
						!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		[EXT]	means external (not related) to MODINSTRUMENT content

C...	[EXT]	nChannels
ChnS	[EXT]	IT/MPTM: Channel settings for channels 65-127 if needed (doesn't fit to IT header).
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
LSWV	[EXT]	Last Saved With Version
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
PE..			PanEnv.nNodes;
PE[.			PanEnv.Values[MAX_ENVPOINTS];
PiE.			PitchEnv.nNodes;
PiE[			PitchEnv.Values[MAX_ENVPOINTS];
PiLE			PitchEnv.nLoopEnd;
PiLS			PitchEnv.nLoopStart;
PiP[			PitchEnv.Ticks[MAX_ENVPOINTS];
PiSB			PitchEnv.nSustainStart;
PiSE			PitchEnv.nSustainEnd;
PLE.			PanEnv.nLoopEnd;
PLS.			PanEnv.nLoopStart;
PMM.	[EXT]	nPlugMixMode;
PP[.			PanEnv.Ticks[MAX_ENVPOINTS];
PPC.			nPPC;
PPS.			nPPS;
PS..			nPanSwing;
PSB.			PanEnv.nSustainStart;
PSE.			PanEnv.nSustainEnd;
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
VE..			VolEnv.nNodes;
VE[.			VolEnv.Values[MAX_ENVPOINTS];
VLE.			VolEnv.nLoopEnd;
VLS.			VolEnv.nLoopStart;
VP[.			VolEnv.Ticks[MAX_ENVPOINTS];
VR..			nVolRamp;
VS..			nVolSwing;
VSB.			VolEnv.nSustainStart;
VSE.			VolEnv.nSustainEnd;
VSTV	[EXT]	nVSTiVolume;
PERN			PitchEnv.nReleaseNode
AERN			PanEnv.nReleaseNode
VERN			VolEnv.nReleaseNode
PFLG			PitchEnv.dwFlag
AFLG			PanEnv.dwFlags
VFLG			VolEnv.dwFlags
-----------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------*/

// --------------------------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for single type members ONLY (non-array)
// --------------------------------------------------------------------------------------------
#define WRITE_MPTHEADER_sized_member(name,type,code) \
static_assert(sizeof(input->name) >= sizeof(type), "");\
fcode = #@code;\
fwrite(& fcode , 1 , sizeof( __int32 ) , file);\
fsize = sizeof( type );\
fwrite(& fsize , 1 , sizeof( __int16 ) , file);\
fwrite(&input-> name , 1 , fsize , file);

// --------------------------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for array members ONLY
// --------------------------------------------------------------------------------------------
#define WRITE_MPTHEADER_array_member(name,type,code,arraysize) \
ASSERT(sizeof(input->name) >= sizeof(type) * arraysize);\
fcode = #@code;\
fwrite(& fcode , 1 , sizeof( __int32 ) , file);\
fsize = sizeof( type ) * arraysize;\
fwrite(& fsize , 1 , sizeof( __int16 ) , file);\
fwrite(&input-> name , 1 , fsize , file);

namespace {
// Create 'dF..' entry.
DWORD CreateExtensionFlags(const MODINSTRUMENT& ins)
//--------------------------------------------------
{
	DWORD dwFlags = 0;
	if (ins.VolEnv.dwFlags & ENV_ENABLED)	dwFlags |= dFdd_VOLUME;
	if (ins.VolEnv.dwFlags & ENV_SUSTAIN)	dwFlags |= dFdd_VOLSUSTAIN;
	if (ins.VolEnv.dwFlags & ENV_LOOP)		dwFlags |= dFdd_VOLLOOP;
	if (ins.PanEnv.dwFlags & ENV_ENABLED)	dwFlags |= dFdd_PANNING;
	if (ins.PanEnv.dwFlags & ENV_SUSTAIN)	dwFlags |= dFdd_PANSUSTAIN;
	if (ins.PanEnv.dwFlags & ENV_LOOP)		dwFlags |= dFdd_PANLOOP;
	if (ins.PitchEnv.dwFlags & ENV_ENABLED)	dwFlags |= dFdd_PITCH;
	if (ins.PitchEnv.dwFlags & ENV_SUSTAIN)	dwFlags |= dFdd_PITCHSUSTAIN;
	if (ins.PitchEnv.dwFlags & ENV_LOOP)	dwFlags |= dFdd_PITCHLOOP;
	if (ins.dwFlags & INS_SETPANNING)		dwFlags |= dFdd_SETPANNING;
	if (ins.PitchEnv.dwFlags & ENV_FILTER)	dwFlags |= dFdd_FILTER;
	if (ins.VolEnv.dwFlags & ENV_CARRY)		dwFlags |= dFdd_VOLCARRY;
	if (ins.PanEnv.dwFlags & ENV_CARRY)		dwFlags |= dFdd_PANCARRY;
	if (ins.PitchEnv.dwFlags & ENV_CARRY)	dwFlags |= dFdd_PITCHCARRY;
	if (ins.dwFlags & INS_MUTE)				dwFlags |= dFdd_MUTE;
	return dwFlags;
}
} // unnamed namespace.

// Write (in 'file') 'input' MODINSTRUMENT with 'code' & 'size' extra field infos for each member
void WriteInstrumentHeaderStruct(MODINSTRUMENT * input, FILE * file)
{
__int32 fcode;
__int16 fsize;
WRITE_MPTHEADER_sized_member(	nFadeOut				, UINT			, FO..							)

{ // dwFlags needs to be constructed so write it manually.
	//WRITE_MPTHEADER_sized_member(	dwFlags					, DWORD			, dF..							)
	const DWORD dwFlags = CreateExtensionFlags(*input);
	fcode = 'dF..';
	fwrite(&fcode, 1, sizeof(int32), file);
	fsize = sizeof(dwFlags);
	fwrite(&fsize, 1, sizeof(int16), file);
	fwrite(&dwFlags, 1, fsize, file);
}

WRITE_MPTHEADER_sized_member(	nGlobalVol				, UINT			, GV..							)
WRITE_MPTHEADER_sized_member(	nPan					, UINT			, P...							)
WRITE_MPTHEADER_sized_member(	VolEnv.nNodes			, UINT			, VE..							)
WRITE_MPTHEADER_sized_member(	PanEnv.nNodes			, UINT			, PE..							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nNodes			, UINT			, PiE.							)
WRITE_MPTHEADER_sized_member(	VolEnv.nLoopStart		, BYTE			, VLS.							)
WRITE_MPTHEADER_sized_member(	VolEnv.nLoopEnd			, BYTE			, VLE.							)
WRITE_MPTHEADER_sized_member(	VolEnv.nSustainStart	, BYTE			, VSB.							)
WRITE_MPTHEADER_sized_member(	VolEnv.nSustainEnd		, BYTE			, VSE.							)
WRITE_MPTHEADER_sized_member(	PanEnv.nLoopStart		, BYTE			, PLS.							)
WRITE_MPTHEADER_sized_member(	PanEnv.nLoopEnd			, BYTE			, PLE.							)
WRITE_MPTHEADER_sized_member(	PanEnv.nSustainStart	, BYTE			, PSB.							)
WRITE_MPTHEADER_sized_member(	PanEnv.nSustainEnd		, BYTE			, PSE.							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nLoopStart		, BYTE			, PiLS							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nLoopEnd		, BYTE			, PiLE							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nSustainStart	, BYTE			, PiSB							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nSustainEnd	, BYTE			, PiSE							)
WRITE_MPTHEADER_sized_member(	nNNA					, BYTE			, NNA.							)
WRITE_MPTHEADER_sized_member(	nDCT					, BYTE			, DCT.							)
WRITE_MPTHEADER_sized_member(	nDNA					, BYTE			, DNA.							)
WRITE_MPTHEADER_sized_member(	nPanSwing				, BYTE			, PS..							)
WRITE_MPTHEADER_sized_member(	nVolSwing				, BYTE			, VS..							)
WRITE_MPTHEADER_sized_member(	nIFC					, BYTE			, IFC.							)
WRITE_MPTHEADER_sized_member(	nIFR					, BYTE			, IFR.							)
WRITE_MPTHEADER_sized_member(	wMidiBank				, WORD			, MB..							)
WRITE_MPTHEADER_sized_member(	nMidiProgram			, BYTE			, MP..							)
WRITE_MPTHEADER_sized_member(	nMidiChannel			, BYTE			, MC..							)
WRITE_MPTHEADER_sized_member(	nMidiDrumKey			, BYTE			, MDK.							)
WRITE_MPTHEADER_sized_member(	nPPS					, signed char	, PPS.							)
WRITE_MPTHEADER_sized_member(	nPPC					, unsigned char	, PPC.							)
WRITE_MPTHEADER_array_member(	VolEnv.Ticks			, WORD			, VP[.		, ((input->VolEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	PanEnv.Ticks			, WORD			, PP[.		, ((input->PanEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	PitchEnv.Ticks			, WORD			, PiP[		, ((input->PitchEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	VolEnv.Values			, BYTE			, VE[.		, ((input->VolEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	PanEnv.Values			, BYTE			, PE[.		, ((input->PanEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	PitchEnv.Values			, BYTE			, PiE[		, ((input->PitchEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	NoteMap					, BYTE			, NM[.		, 128				)
WRITE_MPTHEADER_array_member(	Keyboard				, WORD			, K[..		, 128				)
WRITE_MPTHEADER_array_member(	name					, CHAR			, n[..		, 32				)
WRITE_MPTHEADER_array_member(	filename				, CHAR			, fn[.		, 12				)
WRITE_MPTHEADER_sized_member(	nMixPlug				, BYTE			, MiP.							)
WRITE_MPTHEADER_sized_member(	nVolRamp				, USHORT		, VR..							)
WRITE_MPTHEADER_sized_member(	nResampling				, USHORT		, R...							)
WRITE_MPTHEADER_sized_member(	nCutSwing				, BYTE			, CS..							)
WRITE_MPTHEADER_sized_member(	nResSwing				, BYTE			, RS..							)
WRITE_MPTHEADER_sized_member(	nFilterMode				, BYTE			, FM..							)
WRITE_MPTHEADER_sized_member(	nPluginVelocityHandling	, BYTE			, PVEH							)
WRITE_MPTHEADER_sized_member(	nPluginVolumeHandling	, BYTE			, PVOH							)
WRITE_MPTHEADER_sized_member(	wPitchToTempoLock		, WORD			, PTTL							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nReleaseNode	, BYTE			, PERN							)
WRITE_MPTHEADER_sized_member(	PanEnv.nReleaseNode		, BYTE		    , AERN							)
WRITE_MPTHEADER_sized_member(	VolEnv.nReleaseNode		, BYTE			, VERN							)
WRITE_MPTHEADER_sized_member(	PitchEnv.dwFlags		, DWORD			, PFLG							)
WRITE_MPTHEADER_sized_member(	PanEnv.dwFlags			, DWORD		    , AFLG							)
WRITE_MPTHEADER_sized_member(	VolEnv.dwFlags			, DWORD			, VFLG							)
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

// Return a pointer on the wanted field in 'input' MODINSTRUMENT given field code & size
BYTE * GetInstrumentHeaderFieldPointer(MODINSTRUMENT * input, __int32 fcode, __int16 fsize)
{
if(input == NULL) return NULL;
BYTE * pointer = NULL;

switch(fcode){
GET_MPTHEADER_sized_member(	nFadeOut				, UINT			, FO..							)
GET_MPTHEADER_sized_member(	dwFlags					, DWORD			, dF..							)
GET_MPTHEADER_sized_member(	nGlobalVol				, UINT			, GV..							)
GET_MPTHEADER_sized_member(	nPan					, UINT			, P...							)
GET_MPTHEADER_sized_member(	VolEnv.nNodes			, UINT			, VE..							)
GET_MPTHEADER_sized_member(	PanEnv.nNodes			, UINT			, PE..							)
GET_MPTHEADER_sized_member(	PitchEnv.nNodes			, UINT			, PiE.							)
GET_MPTHEADER_sized_member(	VolEnv.nLoopStart		, BYTE			, VLS.							)
GET_MPTHEADER_sized_member(	VolEnv.nLoopEnd			, BYTE			, VLE.							)
GET_MPTHEADER_sized_member(	VolEnv.nSustainStart	, BYTE			, VSB.							)
GET_MPTHEADER_sized_member(	VolEnv.nSustainEnd		, BYTE			, VSE.							)
GET_MPTHEADER_sized_member(	PanEnv.nLoopStart		, BYTE			, PLS.							)
GET_MPTHEADER_sized_member(	PanEnv.nLoopEnd			, BYTE			, PLE.							)
GET_MPTHEADER_sized_member(	PanEnv.nSustainStart	, BYTE			, PSB.							)
GET_MPTHEADER_sized_member(	PanEnv.nSustainEnd		, BYTE			, PSE.							)
GET_MPTHEADER_sized_member(	PitchEnv.nLoopStart		, BYTE			, PiLS							)
GET_MPTHEADER_sized_member(	PitchEnv.nLoopEnd		, BYTE			, PiLE							)
GET_MPTHEADER_sized_member(	PitchEnv.nSustainStart	, BYTE			, PiSB							)
GET_MPTHEADER_sized_member(	PitchEnv.nSustainEnd	, BYTE			, PiSE							)
GET_MPTHEADER_sized_member(	nNNA					, BYTE			, NNA.							)
GET_MPTHEADER_sized_member(	nDCT					, BYTE			, DCT.							)
GET_MPTHEADER_sized_member(	nDNA					, BYTE			, DNA.							)
GET_MPTHEADER_sized_member(	nPanSwing				, BYTE			, PS..							)
GET_MPTHEADER_sized_member(	nVolSwing				, BYTE			, VS..							)
GET_MPTHEADER_sized_member(	nIFC					, BYTE			, IFC.							)
GET_MPTHEADER_sized_member(	nIFR					, BYTE			, IFR.							)
GET_MPTHEADER_sized_member(	wMidiBank				, WORD			, MB..							)
GET_MPTHEADER_sized_member(	nMidiProgram			, BYTE			, MP..							)
GET_MPTHEADER_sized_member(	nMidiChannel			, BYTE			, MC..							)
GET_MPTHEADER_sized_member(	nMidiDrumKey			, BYTE			, MDK.							)
GET_MPTHEADER_sized_member(	nPPS					, signed char	, PPS.							)
GET_MPTHEADER_sized_member(	nPPC					, unsigned char	, PPC.							)
GET_MPTHEADER_array_member(	VolEnv.Ticks			, WORD			, VP[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PanEnv.Ticks			, WORD			, PP[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PitchEnv.Ticks			, WORD			, PiP[		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	VolEnv.Values			, BYTE			, VE[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PanEnv.Values			, BYTE			, PE[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PitchEnv.Values			, BYTE			, PiE[		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	NoteMap					, BYTE			, NM[.		, 128				)
GET_MPTHEADER_array_member(	Keyboard				, WORD			, K[..		, 128				)
GET_MPTHEADER_array_member(	name					, CHAR			, n[..		, 32				)
GET_MPTHEADER_array_member(	filename				, CHAR			, fn[.		, 12				)
GET_MPTHEADER_sized_member(	nMixPlug				, BYTE			, MiP.							)
GET_MPTHEADER_sized_member(	nVolRamp				, USHORT		, VR..							)
GET_MPTHEADER_sized_member(	nResampling				, UINT			, R...							)
GET_MPTHEADER_sized_member(	nCutSwing				, BYTE			, CS..							)
GET_MPTHEADER_sized_member(	nResSwing				, BYTE			, RS..							)
GET_MPTHEADER_sized_member(	nFilterMode				, BYTE			, FM..							)
GET_MPTHEADER_sized_member(	wPitchToTempoLock		, WORD			, PTTL							)
GET_MPTHEADER_sized_member(	nPluginVelocityHandling	, BYTE			, PVEH							)
GET_MPTHEADER_sized_member(	nPluginVolumeHandling	, BYTE			, PVOH							)
GET_MPTHEADER_sized_member(	PitchEnv.nReleaseNode	, BYTE			, PERN							)
GET_MPTHEADER_sized_member(	PanEnv.nReleaseNode		, BYTE		    , AERN							)
GET_MPTHEADER_sized_member(	VolEnv.nReleaseNode		, BYTE			, VERN							)
GET_MPTHEADER_sized_member(	PitchEnv.dwFlags     	, DWORD			, PFLG							)
GET_MPTHEADER_sized_member(	PanEnv.dwFlags     		, DWORD		    , AFLG							)
GET_MPTHEADER_sized_member(	VolEnv.dwFlags     		, DWORD			, VFLG							)
}

return pointer;
}

// -! NEW_FEATURE#0027


CTuning* MODINSTRUMENT::s_DefaultTuning = 0;


//////////////////////////////////////////////////////////
// CSoundFile

CTuningCollection* CSoundFile::s_pTuningsSharedBuiltIn(0);
CTuningCollection* CSoundFile::s_pTuningsSharedLocal(0);
uint8 CSoundFile::s_DefaultPlugVolumeHandling = PLUGIN_VOLUMEHANDLING_IGNORE;

#pragma warning(disable : 4355) // "'this' : used in base member initializer list"
CSoundFile::CSoundFile() :
	Patterns(*this),
	Order(*this),
	m_PlaybackEventer(*this),
	m_pModSpecs(&ModSpecs::itEx),
	m_MIDIMapper(*this)
#pragma warning(default : 4355) // "'this' : used in base member initializer list"
//----------------------
{
	m_nType = MOD_TYPE_NONE;
	m_dwSongFlags = 0;
	m_nChannels = 0;
	m_nMixChannels = 0;
	m_nSamples = 0;
	m_nInstruments = 0;
	m_lpszSongComments = nullptr;
	m_nFreqFactor = m_nTempoFactor = 128;
	m_nMasterVolume = 128;
	m_nMinPeriod = MIN_PERIOD;
	m_nMaxPeriod = 0x7FFF;
	m_nRepeatCount = 0;
	m_nSeqOverride = 0;
	m_bPatternTransitionOccurred = false;
	m_nDefaultRowsPerBeat = m_nCurrentRowsPerBeat = (CMainFrame::m_nRowSpacing2) ? CMainFrame::m_nRowSpacing2 : 4;
	m_nDefaultRowsPerMeasure = m_nCurrentRowsPerMeasure = (CMainFrame::m_nRowSpacing >= m_nDefaultRowsPerBeat) ? CMainFrame::m_nRowSpacing : m_nDefaultRowsPerBeat * 4;
	m_nTempoMode = tempo_mode_classic;
	m_bIsRendering = false;
	m_nMaxSample = 0;

	m_ModFlags = 0;
	m_bITBidiMode = false;

	m_pModDoc = NULL;
	m_dwLastSavedWithVersion=0;
	m_dwCreatedWithVersion=0;
	MemsetZero(m_bChannelMuteTogglePending);


// -> CODE#0023
// -> DESC="IT project files (.itp)"
	for(UINT i = 0; i < MAX_INSTRUMENTS; i++)
	{
		MemsetZero(m_szInstrumentPath[i]);
	}
// -! NEW_FEATURE#0023

	MemsetZero(Chn);
	MemsetZero(ChnMix);
	MemsetZero(Samples);
	MemsetZero(ChnSettings);
	MemsetZero(Instruments);
	MemsetZero(m_szNames);
	MemsetZero(m_MixPlugins);
	MemsetZero(m_SongEQ);
	Order.Init();
	Patterns.ClearPatterns();
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
//----------------------------------------------------------------------------
{
	m_pModDoc = pModDoc;
	m_nType = MOD_TYPE_NONE;
	m_dwSongFlags = 0;
	m_nChannels = 0;
	m_nMixChannels = 0;
	m_nSamples = 0;
	m_nInstruments = 0;
	m_nFreqFactor = m_nTempoFactor = 128;
	m_nMasterVolume = 128;
	m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	m_nGlobalVolume = MAX_GLOBAL_VOLUME;
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
	m_nNextPatStartRow = 0;
	m_nSeqOverride = 0;
	m_nRestartPos = 0;
	m_nMinPeriod = 16;
	m_nMaxPeriod = 32767;
	m_nSamplePreAmp = 48;
	m_nVSTiVolume = 48;
	m_nMaxOrderPosition = 0;
	m_lpszSongComments = nullptr;
	m_nMixLevels = mixLevels_compatible;	// Will be overridden if appropriate.
	MemsetZero(Samples);
	MemsetZero(ChnMix);
	MemsetZero(Chn);
	MemsetZero(Instruments);
	MemsetZero(m_szNames);
	MemsetZero(m_MixPlugins);
	MemsetZero(m_SongEQ);
	//Order.assign(MAX_ORDERS, Order.GetInvalidPatIndex());
	Order.resize(1);
	Patterns.ClearPatterns();
	ResetMidiCfg();

	for (CHANNELINDEX nChn = 0; nChn < MAX_BASECHANNELS; nChn++)
	{
		InitChannel(nChn);
	}
	if (lpStream)
	{
#ifdef ZIPPED_MOD_SUPPORT
		CZipArchive archive(glpszModExtensions);
		if (CZipArchive::IsArchive((LPBYTE)lpStream, dwMemLength))
		{
			if (archive.UnzipArchive((LPBYTE)lpStream, dwMemLength) && archive.GetOutputFile())
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
			if (unrar.ExtrFile() && unrar.GetOutputFile())
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
			if (unlha.ExtractFile() && unlha.GetOutputFile())
			{
				lpStream = unlha.GetOutputFile();
				dwMemLength = unlha.GetOutputFileLength();
			}
		}
#endif
#ifdef UNGZIP_SUPPORT
		CGzipArchive ungzip((LPBYTE)lpStream, dwMemLength);
		if (ungzip.IsArchive())
		{
			if (ungzip.ExtractFile() && ungzip.GetOutputFile())
			{
				lpStream = ungzip.GetOutputFile();
				dwMemLength = ungzip.GetOutputFileLength();
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
		 && (!ReadGDM(lpStream, dwMemLength))
		 && (!ReadIMF(lpStream, dwMemLength))
		 && (!ReadAM(lpStream, dwMemLength))
		 && (!ReadJ2B(lpStream, dwMemLength))
		 && (!ReadMO3(lpStream, dwMemLength))
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
		Chn[ich].nEFxSpeed = 0;
		//IT compatibility 15. Retrigger
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			Chn[ich].nRetrigParam = Chn[ich].nRetrigCount = 1;
		}
	}
	// Checking instruments
	MODSAMPLE *pSmp = Samples;
	for (UINT iIns=0; iIns<MAX_INSTRUMENTS; iIns++, pSmp++)
	{
		if (pSmp->pSample)
		{
			if (pSmp->nLoopEnd > pSmp->nLength) pSmp->nLoopEnd = pSmp->nLength;
			if (pSmp->nLoopStart >= pSmp->nLoopEnd)
			{
				pSmp->nLoopStart = 0;
				pSmp->nLoopEnd = 0;
			}
			if (pSmp->nSustainEnd > pSmp->nLength) pSmp->nSustainEnd = pSmp->nLength;
			if (pSmp->nSustainStart >= pSmp->nSustainEnd)
			{
				pSmp->nSustainStart = 0;
				pSmp->nSustainEnd = 0;
			}
		} else
		{
			pSmp->nLength = 0;
			pSmp->nLoopStart = 0;
			pSmp->nLoopEnd = 0;
			pSmp->nSustainStart = 0;
			pSmp->nSustainEnd = 0;
		}
		if (!pSmp->nLoopEnd) pSmp->uFlags &= ~(CHN_LOOP|CHN_PINGPONGLOOP);
		if (!pSmp->nSustainEnd) pSmp->uFlags &= ~(CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN);
		if (pSmp->nGlobalVol > 64) pSmp->nGlobalVol = 64;
	}
	// Check invalid instruments
	while ((m_nInstruments > 0) && (!Instruments[m_nInstruments])) m_nInstruments--;
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

	InitializeVisitedRows(true);

	switch(m_nTempoMode)
	{
		case tempo_mode_alternative: 
			m_nSamplesPerTick = gdwMixingFreq / m_nMusicTempo; break;
		case tempo_mode_modern: 
			m_nSamplesPerTick = gdwMixingFreq * (60/m_nMusicTempo / (m_nMusicSpeed * m_nCurrentRowsPerBeat)); break;
		case tempo_mode_classic: default:
			m_nSamplesPerTick = (gdwMixingFreq * 5 * m_nTempoFactor) / (m_nMusicTempo << 8);
	}

	if ((m_nRestartPos >= Order.size()) || (Order[m_nRestartPos] >= Patterns.Size())) m_nRestartPos = 0;

	// plugin loader
	string sNotFound;
	std::list<PLUGINDEX> notFoundIDs;

	// Load plugins only when m_pModDoc != 0.  (can be == 0 for example when examining module samples in treeview.
	if (gpMixPluginCreateProc && GetpModDoc())
	{
		for (PLUGINDEX iPlug = 0; iPlug < MAX_MIXPLUGINS; iPlug++)
		{
			if ((m_MixPlugins[iPlug].Info.dwPluginId1)
			 || (m_MixPlugins[iPlug].Info.dwPluginId2))
			{
				gpMixPluginCreateProc(&m_MixPlugins[iPlug], this);
				if (m_MixPlugins[iPlug].pMixPlugin)
				{
					// plugin has been found
					m_MixPlugins[iPlug].pMixPlugin->RestoreAllParameters(m_MixPlugins[iPlug].defaultProgram); //rewbs.plugDefaultProgram: added param
				}
				else
				{
					// plugin not found - add to list
					bool bFound = false;
					for(std::list<PLUGINDEX>::iterator i = notFoundIDs.begin(); i != notFoundIDs.end(); ++i)
					{
						if(m_MixPlugins[*i].Info.dwPluginId2 == m_MixPlugins[iPlug].Info.dwPluginId2)
						{
							bFound = true;
							break;
						}
					}

					if(bFound == false)
					{
						sNotFound = sNotFound + m_MixPlugins[iPlug].Info.szLibraryName + "\n";
						notFoundIDs.push_back(iPlug); // add this to the list of missing IDs so we will find the needed plugins later when calling KVRAudio
					}
				}
			}
		}
	}

	// Display a nice message so the user sees what plugins are missing
	// TODO: Use IDD_MODLOADING_WARNINGS dialog (NON-MODAL!) to display all warnings that are encountered when loading a module.
	if(!notFoundIDs.empty())
	{
		if(notFoundIDs.size() == 1)
		{
			sNotFound = "The following plugin has not been found:\n\n" + sNotFound + "\nDo you want to search for it on KVRAudio?";
		}
		else
		{
			sNotFound =	"The following plugins have not been found:\n\n" + sNotFound + "\nDo you want to search for them on KVRAudio?"
						"\nWARNING: A browser window / tab is opened for every plugin. If you do not want that, you can visit http://www.kvraudio.com/search.php";
		}
		if (::MessageBox(0, sNotFound.c_str(), "OpenMPT - Plugins missing", MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) == IDYES)
			for(std::list<PLUGINDEX>::iterator i = notFoundIDs.begin(); i != notFoundIDs.end(); ++i)
			{
				CString sUrl;
				sUrl.Format("http://www.kvraudio.com/search.php?lq=inurl%3Aget&q=%s", m_MixPlugins[*i].Info.szLibraryName);
				CTrackApp::OpenURL(sUrl);
			}
	}

	// Set up mix levels
	m_pConfig->SetMixLevels(m_nMixLevels);
	RecalculateGainForAllPlugs();

	if (m_nType)
	{
		SetModSpecsPointer(m_pModSpecs, m_nType);
		const ORDERINDEX nMinLength = (std::min)(ModSequenceSet::s_nCacheSize, GetModSpecifications().ordersMax);
		if (Order.GetLength() < nMinLength)
			Order.resize(nMinLength);
		return TRUE;
	}

	return FALSE;
}


BOOL CSoundFile::Destroy()
//------------------------
{
	size_t i;
	Patterns.DestroyPatterns();

	FreeMessage();

	for (i=1; i<MAX_SAMPLES; i++)
	{
		MODSAMPLE *pSmp = &Samples[i];
		if (pSmp->pSample)
		{
			FreeSample(pSmp->pSample);
			pSmp->pSample = nullptr;
		}
	}
	for (i = 0; i < MAX_INSTRUMENTS; i++)
	{
		delete Instruments[i];
		Instruments[i] = nullptr;
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
	MemsetZero(m_MidiCfg);
	lstrcpy(m_MidiCfg.szMidiGlb[MIDIOUT_START], "FF");
	lstrcpy(m_MidiCfg.szMidiGlb[MIDIOUT_STOP], "FC");
	lstrcpy(m_MidiCfg.szMidiGlb[MIDIOUT_NOTEON], "9c n v");
	lstrcpy(m_MidiCfg.szMidiGlb[MIDIOUT_NOTEOFF], "9c n 0");
	lstrcpy(m_MidiCfg.szMidiGlb[MIDIOUT_PROGRAM], "Cc p");
	lstrcpy(m_MidiCfg.szMidiSFXExt[0], "F0F000z");
	for (int iz=0; iz<16; iz++) wsprintf(m_MidiCfg.szMidiZXXExt[iz], "F0F001%02X", iz*8);
}


// Set null terminator for all MIDI macros
void CSoundFile::SanitizeMacros()
//-------------------------------
{
	for(size_t i = 0; i < CountOf(m_MidiCfg.szMidiGlb); i++)
	{
		SetNullTerminator(m_MidiCfg.szMidiGlb[i]);
	}
	for(size_t i = 0; i < CountOf(m_MidiCfg.szMidiSFXExt); i++)
	{
		SetNullTerminator(m_MidiCfg.szMidiSFXExt[i]);
	}
	for(size_t i = 0; i < CountOf(m_MidiCfg.szMidiZXXExt); i++)
	{
		SetNullTerminator(m_MidiCfg.szMidiZXXExt[i]);
	}
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


void CSoundFile::SetMasterVolume(UINT nVol, bool adjustAGC)
//---------------------------------------------------------
{
	if (nVol < 1) nVol = 1;
	if (nVol > 0x200) nVol = 0x200;	// x4 maximum
	if ((nVol < m_nMasterVolume) && (nVol) && (gdwSoundSetup & SNDMIX_AGC) && (adjustAGC))
	{
		gnAGC = gnAGC * m_nMasterVolume / nVol;
		if (gnAGC > AGC_UNITY) gnAGC = AGC_UNITY;
	}
	m_nMasterVolume = nVol;
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


PATTERNINDEX CSoundFile::GetNumPatterns() const
//---------------------------------------------
{
	PATTERNINDEX max = 0;
	for(PATTERNINDEX i = 0; i < Patterns.Size(); i++)
	{
		if(Patterns.IsValidPat(i))
			max = i + 1;
	}
	return max;
}


UINT CSoundFile::GetMaxPosition() const
//-------------------------------------
{
	UINT max = 0;
	UINT i = 0;

	while ((i < Order.size()) && (Order[i] != Order.GetInvalidPatIndex()))
	{
		if (Order[i] < Patterns.Size()) max += Patterns[Order[i]].GetNumRows();
		i++;
	}
	return max;
}


UINT CSoundFile::GetCurrentPos() const
//------------------------------------
{
	UINT pos = 0;

	for (UINT i=0; i<m_nCurrentPattern; i++) if (Order[i] < Patterns.Size())
		pos += Patterns[Order[i]].GetNumRows();
	return pos + m_nRow; 
}

double  CSoundFile::GetCurrentBPM() const
//---------------------------------------
{
	double bpm;

	if (m_nTempoMode == tempo_mode_modern)			// With modern mode, we trust that true bpm
	{												// is close enough to what user chose.
		bpm = static_cast<double>(m_nMusicTempo);	// This avoids oscillation due to tick-to-tick corrections.
	} else
	{																	//with other modes, we calculate it:
		double ticksPerBeat = m_nMusicSpeed * m_nCurrentRowsPerBeat;	//ticks/beat = ticks/row  * rows/beat
		double samplesPerBeat = m_nSamplesPerTick * ticksPerBeat;		//samps/beat = samps/tick * ticks/beat
		bpm =  gdwMixingFreq/samplesPerBeat * 60;						//beats/sec  = samps/sec  / samps/beat
	}																	//beats/min  =  beats/sec * 60
	
	return bpm;
}

void CSoundFile::SetCurrentPos(UINT nPos)
//---------------------------------------
{
	ORDERINDEX nPattern;
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
			if (nPos < (UINT)Patterns[ord].GetNumRows()) break;
			nPos -= Patterns[ord].GetNumRows();
		}
	}
	// Buggy position ?
	if ((nPattern >= Order.size())
	 || (Order[nPattern] >= Patterns.Size())
	 || (nPos >= Patterns[Order[nPattern]].GetNumRows()))
	{
		nPos = 0;
		nPattern = 0;
	}
	UINT nRow = nPos;
	if ((nRow) && (Order[nPattern] < Patterns.Size()))
	{
		MODCOMMAND *p = Patterns[Order[nPattern]];
		if ((p) && (nRow < Patterns[Order[nPattern]].GetNumRows()))
		{
			bool bOk = false;
			while ((!bOk) && (nRow > 0))
			{
				UINT n = nRow * m_nChannels;
				for (UINT k=0; k<m_nChannels; k++, n++)
				{
					if (p[n].note)
					{
						bOk = true;
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
	m_nNextPatStartRow = 0;
	//m_nSeqOverride = 0;
}



void CSoundFile::SetCurrentOrder(ORDERINDEX nOrder)
//-------------------------------------------------
{
	while ((nOrder < Order.size()) && (Order[nOrder] == Order.GetIgnoreIndex())) nOrder++;
	if ((nOrder >= Order.size()) || (Order[nOrder] >= Patterns.Size())) return;
	for (CHANNELINDEX j = 0; j < MAX_CHANNELS; j++)
	{
		Chn[j].nPeriod = 0;
		Chn[j].nNote = NOTE_NONE;
		Chn[j].nPortamentoDest = 0;
		Chn[j].nCommand = 0;
		Chn[j].nPatternLoopCount = 0;
		Chn[j].nPatternLoop = 0;
		Chn[j].nVibratoPos = Chn[j].nTremoloPos = Chn[j].nPanbrelloPos = 0;
		//IT compatibility 15. Retrigger
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			Chn[j].nRetrigCount = 0;
			Chn[j].nRetrigParam = 1;
		}
		Chn[j].nTremorCount = 0;
	}

#ifndef NO_VST
	// Stop hanging notes from VST instruments as well
	StopAllVsti();
#endif // NO_VST

	if (!nOrder)
	{
		SetCurrentPos(0);
	} else
	{
		m_nNextPattern = nOrder;
		m_nRow = m_nNextRow = 0;
		m_nPattern = 0;
		m_nTickCount = m_nMusicSpeed;
		m_nBufferCount = 0;
		m_nTotalCount = 0;
		m_nPatternDelay = 0;
		m_nFrameDelay = 0;
		m_nNextPatStartRow = 0;
	}
	//m_dwSongFlags &= ~(SONG_PATTERNLOOP|SONG_CPUVERYHIGH|SONG_FADINGSONG|SONG_ENDREACHED|SONG_GLOBALFADE);
	m_dwSongFlags &= ~(SONG_CPUVERYHIGH|SONG_FADINGSONG|SONG_ENDREACHED|SONG_GLOBALFADE);
}

//rewbs.VSTCompliance
void CSoundFile::SuspendPlugins()
//-------------------------------
{
	for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
	{
		if (!m_MixPlugins[iPlug].pMixPlugin)	
			continue;  //most common branch
		
		IMixPlugin *pPlugin = m_MixPlugins[iPlug].pMixPlugin;
		if (m_MixPlugins[iPlug].pMixState && pPlugin->IsResumed())
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
	for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
	{
		if (!m_MixPlugins[iPlug].pMixPlugin)	
			continue;  //most common branch
		
		IMixPlugin *pPlugin = m_MixPlugins[iPlug].pMixPlugin;
		if (m_MixPlugins[iPlug].pMixState)
		{
			pPlugin->HardAllNotesOff();
		}
	}
	m_lTotalSampleCount = GetSampleOffset();
}


void CSoundFile::RecalculateGainForAllPlugs()
//-------------------------------------------
{
	for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
	{
		if (!m_MixPlugins[iPlug].pMixPlugin)	
			continue;  //most common branch
		
		IMixPlugin *pPlugin = m_MixPlugins[iPlug].pMixPlugin;
		if (m_MixPlugins[iPlug].pMixState)
		{
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
		Chn[i].nROfs = Chn[i].nLOfs = Chn[i].nLength = 0;
	}
}



void CSoundFile::LoopPattern(PATTERNINDEX nPat, ROWINDEX nRow)
//------------------------------------------------------------
{
	if ((nPat < 0) || (nPat >= Patterns.Size()) || (!Patterns[nPat]))
	{
		m_dwSongFlags &= ~SONG_PATTERNLOOP;
	} else
	{
		if ((nRow < 0) || (nRow >= (int)Patterns[nPat].GetNumRows())) nRow = 0;
		m_nPattern = nPat;
		m_nRow = m_nNextRow = nRow;
		m_nTickCount = m_nMusicSpeed;
		m_nPatternDelay = 0;
		m_nFrameDelay = 0;
		m_nBufferCount = 0;
		m_nNextPatStartRow = 0;
		m_dwSongFlags |= SONG_PATTERNLOOP;
	//	m_nSeqOverride = 0;
	}
}
//rewbs.playSongFromCursor
void CSoundFile::DontLoopPattern(PATTERNINDEX nPat, ROWINDEX nRow)
//----------------------------------------------------------------
{
	if ((nPat < 0) || (nPat >= Patterns.Size()) || (!Patterns[nPat])) nPat = 0;
	if ((nRow < 0) || (nRow >= (int)Patterns[nPat].GetNumRows())) nRow = 0;
	m_nPattern = nPat;
	m_nRow = m_nNextRow = nRow;
	m_nTickCount = m_nMusicSpeed;
	m_nPatternDelay = 0;
	m_nFrameDelay = 0;
	m_nBufferCount = 0;
	m_nNextPatStartRow = 0;
	m_dwSongFlags &= ~SONG_PATTERNLOOP;
	//m_nSeqOverride = 0;
}

ORDERINDEX CSoundFile::FindOrder(PATTERNINDEX nPat, UINT startFromOrder, bool direction)
//--------------------------------------------------------------------------------------
{
	ORDERINDEX foundAtOrder = ORDERINDEX_INVALID;
	ORDERINDEX candidateOrder = 0;

	for (ORDERINDEX p = 0; p < Order.size(); p++)
	{
		if (direction)
		{
			candidateOrder = (startFromOrder + p) % Order.size();			//wrap around MAX_ORDERS
		} else {
			candidateOrder = (startFromOrder - p + Order.size()) % Order.size();	//wrap around 0 and MAX_ORDERS
		}
		if (Order[candidateOrder] == nPat) {
			foundAtOrder = candidateOrder;
			break;
		}
	}

	return foundAtOrder;
}
//end rewbs.playSongFromCursor


MODTYPE CSoundFile::GetBestSaveFormat() const
//-------------------------------------------
{
	if ((!m_nSamples) || (!m_nChannels)) return MOD_TYPE_NONE;
	if (!m_nType) return MOD_TYPE_NONE;
	if (m_nType & (MOD_TYPE_MOD/*|MOD_TYPE_OKT*/))
		return MOD_TYPE_MOD;
	if (m_nType & (MOD_TYPE_S3M|MOD_TYPE_STM|MOD_TYPE_ULT|MOD_TYPE_FAR|MOD_TYPE_PTM|MOD_TYPE_MTM))
		return MOD_TYPE_S3M;
	if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MED/*|MOD_TYPE_MT2*/))
		return MOD_TYPE_XM;
	if(m_nType & MOD_TYPE_MPT)
		return MOD_TYPE_MPT;
	return MOD_TYPE_IT;
}


MODTYPE CSoundFile::GetSaveFormats() const
//----------------------------------------
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


LPCTSTR CSoundFile::GetSampleName(UINT nSample) const
//---------------------------------------------------
{
	if (nSample<MAX_SAMPLES) {
		return m_szNames[nSample];
	} else {
		return gszEmpty;
	}
}


CString CSoundFile::GetInstrumentName(UINT nInstr) const
//------------------------------------------------------
{
	if ((nInstr >= MAX_INSTRUMENTS) || (!Instruments[nInstr]))
		return TEXT("");

	const size_t nSize = ARRAYELEMCOUNT(Instruments[nInstr]->name);
	CString str;
	LPTSTR p = str.GetBuffer(nSize + 1);
	ArrayCopy(p, Instruments[nInstr]->name, nSize);
	p[nSize] = 0;
	str.ReleaseBuffer();
	return str;
}


bool CSoundFile::InitChannel(CHANNELINDEX nChn)
//---------------------------------------------
{
	if(nChn >= MAX_BASECHANNELS) return true;

	ChnSettings[nChn].nPan = 128;
	ChnSettings[nChn].nVolume = 64;
	ChnSettings[nChn].dwFlags = 0;
	ChnSettings[nChn].nMixPlugin = 0;
	strcpy(ChnSettings[nChn].szName, "");

	ResetChannelState(nChn, CHNRESET_TOTAL);

	if(m_pModDoc)
	{
		m_pModDoc->Record1Channel(nChn, false);
		m_pModDoc->Record2Channel(nChn, false);
	}
	m_bChannelMuteTogglePending[nChn] = false;

	return false;
}

void CSoundFile::ResetChannelState(CHANNELINDEX i, BYTE resetMask)
//----------------------------------------------------------------
{
	if(i >= MAX_CHANNELS) return;

	if(resetMask & 2)
	{
		Chn[i].nNote = Chn[i].nNewNote = Chn[i].nNewIns = 0;
		Chn[i].pModSample = nullptr;
		Chn[i].pModInstrument = nullptr;
		Chn[i].nPortamentoDest = 0;
		Chn[i].nCommand = 0;
		Chn[i].nPatternLoopCount = 0;
		Chn[i].nPatternLoop = 0;
		Chn[i].nFadeOutVol = 0;
		Chn[i].dwFlags |= CHN_KEYOFF|CHN_NOTEFADE;
		//IT compatibility 15. Retrigger
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			Chn[i].nRetrigParam = 1;
			Chn[i].nRetrigCount = 0;
		}
		Chn[i].nTremorCount = 0;
		Chn[i].nEFxSpeed = 0;
	}

	if(resetMask & 4)
	{
		Chn[i].nPeriod = 0;
		Chn[i].nPos = Chn[i].nLength = 0;
		Chn[i].nLoopStart = 0;
		Chn[i].nLoopEnd = 0;
		Chn[i].nROfs = Chn[i].nLOfs = 0;
		Chn[i].pSample = nullptr;
		Chn[i].pModSample = nullptr;
		Chn[i].pModInstrument = nullptr;
		Chn[i].nCutOff = 0x7F;
		Chn[i].nResonance = 0;
		Chn[i].nFilterMode = 0;
		Chn[i].nLeftVol = Chn[i].nRightVol = 0;
		Chn[i].nNewLeftVol = Chn[i].nNewRightVol = 0;
		Chn[i].nLeftRamp = Chn[i].nRightRamp = 0;
		Chn[i].nVolume = 256;
		Chn[i].nVibratoPos = Chn[i].nTremoloPos = Chn[i].nPanbrelloPos = 0;

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


bool CSoundFile::CanPackSample(LPSTR pSample, UINT nLen, UINT nPacking, BYTE *result/*=NULL*/)
//--------------------------------------------------------------------------------------------
{
	int pos, old, oldpos, besttable = 0;
	DWORD dwErr, dwTotal, dwResult;
	int i,j;
	
	if (result) *result = 0;
	if ((!pSample) || (nLen < 1024)) return false;
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
	return (dwResult >= nPacking) ? true : false;
}
#endif // NO_PACKING

#ifndef MODPLUG_NO_FILESAVE

UINT CSoundFile::WriteSample(FILE *f, MODSAMPLE *pSmp, UINT nFlags, UINT nMaxLen)
//-------------------------------------------------------------------------------
{
	UINT len = 0, bufcount;
	char buffer[4096];
	signed char *pSample = (signed char *)pSmp->pSample;
	UINT nLen = pSmp->nLength;
	
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
			int16 *p = (int16 *)pSample;
			int s_old = 0, s_ofs;
			len = nLen * 2;
			bufcount = 0;
			s_ofs = (nFlags == RS_PCM16U) ? 0x8000 : 0;
			for (UINT j=0; j<nLen; j++)
			{
				int s_new = *p;
				p++;
				if (pSmp->uFlags & CHN_STEREO)
				{
					s_new = (s_new + (*p) + 1) >> 1;
					p++;
				}
				if (nFlags == RS_PCM16D)
				{
					*((int16 *)(&buffer[bufcount])) = (int16)(s_new - s_old);
					s_old = s_new;
				} else
				{
					*((int16 *)(&buffer[bufcount])) = (int16)(s_new + s_ofs);
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
				int8 *p = ((int8 *)pSample) + iCh;
				int s_old = 0;

				bufcount = 0;
				for (UINT j=0; j<nLen; j++)
				{
					int s_new = *p;
					p += 2;
					if (nFlags == RS_STPCM8D)
					{
						buffer[bufcount++] = (int8)(s_new - s_old);
						s_old = s_new;
					} else
					{
						buffer[bufcount++] = (int8)(s_new + s_ofs);
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
				int16 *p = ((int16 *)pSample) + iCh;
				int s_old = 0;

				bufcount = 0;
				for (UINT j=0; j<nLen; j++)
				{
					int s_new = *p;
					p += 2;
					if (nFlags == RS_STPCM16D)
					{
						*((int16 *)(&buffer[bufcount])) = (int16)(s_new - s_old);
						s_old = s_new;
					} else
					{
						*((int16 *)(&buffer[bufcount])) = (int16)(s_new + s_ofs);
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

	//	Stereo unsigned interleaved
	case RS_STIPCM8U:
		len = nLen * 2;
		bufcount = 0;
		for (UINT j=0; j<len; j++)
		{
			*((uint8 *)(&buffer[bufcount])) = *((uint8 *)(&pSample[j])) + 0x80;
			bufcount++;
			if (bufcount >= sizeof(buffer))
			{
				if(f) fwrite(buffer, 1, bufcount, f);
				bufcount = 0;
			}
		}
		if (bufcount) if(f) fwrite(buffer, 1, bufcount, f);
		break;

	// Default: assume 8-bit PCM data
	default:
		len = nLen;
		bufcount = 0;
		{
			int8 *p = (int8 *)pSample;
			int sinc = (pSmp->uFlags & CHN_16BIT) ? 2 : 1;
			int s_old = 0, s_ofs = (nFlags == RS_PCM8U) ? 0x80 : 0;
			if (pSmp->uFlags & CHN_16BIT) p++;
			for (UINT j=0; j<len; j++)
			{
				int s_new = (int8)(*p);
				p += sinc;
				if (pSmp->uFlags & CHN_STEREO)
				{
					s_new = (s_new + ((int)*p) + 1) >> 1;
					p += sinc;
				}
				if (nFlags == RS_PCM8D)
				{
					buffer[bufcount++] = (int8)(s_new - s_old);
					s_old = s_new;
				} else
				{
					buffer[bufcount++] = (int8)(s_new + s_ofs);
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

UINT CSoundFile::ReadSample(MODSAMPLE *pSmp, UINT nFlags, LPCSTR lpMemFile, DWORD dwMemLength, const WORD format)
//---------------------------------------------------------------------------------------------------------------
{
	if ((!pSmp) || (pSmp->nLength < 2) || (!lpMemFile)) return 0;

	if(pSmp->nLength > MAX_SAMPLE_LENGTH)
		pSmp->nLength = MAX_SAMPLE_LENGTH;
	
	UINT len = 0, mem = pSmp->nLength+6;

	pSmp->uFlags &= ~(CHN_16BIT|CHN_STEREO);
	if (nFlags & RSF_16BIT)
	{
		mem *= 2;
		pSmp->uFlags |= CHN_16BIT;
	}
	if (nFlags & RSF_STEREO)
	{
		mem *= 2;
		pSmp->uFlags |= CHN_STEREO;
	}

	if ((pSmp->pSample = AllocateSample(mem)) == NULL)
	{
		pSmp->nLength = 0;
		return 0;
	}

	// Check that allocated memory size is not less than what the modinstrument itself
	// thinks it is.
	if( mem < pSmp->GetSampleSizeInBytes() )
	{
		pSmp->nLength = 0;
		FreeSample(pSmp->pSample);
		pSmp->pSample = nullptr;
		MessageBox(0, str_SampleAllocationError, str_Error, MB_ICONERROR);
		return 0;
	}

	switch(nFlags)
	{
	// 1: 8-bit unsigned PCM data
	case RS_PCM8U:
		{
			len = pSmp->nLength;
			if (len > dwMemLength) len = pSmp->nLength = dwMemLength;
			LPSTR pSample = pSmp->pSample;
			for (UINT j=0; j<len; j++) pSample[j] = (char)(lpMemFile[j] - 0x80); 
		}
		break;

	// 2: 8-bit ADPCM data with linear table
	case RS_PCM8D:
		{
			len = pSmp->nLength;
			if (len > dwMemLength) break;
			LPSTR pSample = pSmp->pSample;
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
			len = (pSmp->nLength + 1) / 2;
			if (len > dwMemLength - 16) break;
			memcpy(CompressionTable, lpMemFile, 16);
			lpMemFile += 16;
			LPSTR pSample = pSmp->pSample;
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
			len = pSmp->nLength * 2;
			if (len > dwMemLength) break;
			short int *pSample = (short int *)pSmp->pSample;
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
		len = pSmp->nLength * 2;
		if (len <= dwMemLength) memcpy(pSmp->pSample, lpMemFile, len);
		break;

	// 16-bit signed mono PCM motorola byte order
	case RS_PCM16M:
		len = pSmp->nLength * 2;
		if (len > dwMemLength) len = dwMemLength & ~1;
		if (len > 1)
		{
			char *pSample = (char *)pSmp->pSample;
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
			len = pSmp->nLength * 2;
			if (len > dwMemLength) break;
			short int *pSample = (short int *)pSmp->pSample;
			short int *pSrc = (short int *)lpMemFile;
			for (UINT j=0; j<len; j+=2) *pSample++ = (*(pSrc++)) - 0x8000;
		}
		break;

	// 16-bit signed stereo big endian
	case RS_STPCM16M:
		len = pSmp->nLength * 2;
		if (len*2 <= dwMemLength)
		{
			char *pSample = (char *)pSmp->pSample;
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
			len = pSmp->nLength;
			char *psrc = (char *)lpMemFile;
			char *pSample = (char *)pSmp->pSample;
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
			len = pSmp->nLength;
			short int *psrc = (short int *)lpMemFile;
			short int *pSample = (short int *)pSmp->pSample;
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
			ITUnpack8Bit(pSmp->pSample, pSmp->nLength, (LPBYTE)lpMemFile, dwMemLength, (nFlags == RS_IT2158));
		else
			ITUnpack16Bit(pSmp->pSample, pSmp->nLength, (LPBYTE)lpMemFile, dwMemLength, (nFlags == RS_IT21516));
		break;

#ifndef MODPLUG_BASIC_SUPPORT
#ifndef FASTSOUNDLIB
	// 8-bit interleaved stereo samples
	case RS_STIPCM8S:
	case RS_STIPCM8U:
		{
			int iadd = 0;
			if (nFlags == RS_STIPCM8U) { iadd = -0x80; }
			len = pSmp->nLength;
			if (len*2 > dwMemLength) len = dwMemLength >> 1;
			LPBYTE psrc = (LPBYTE)lpMemFile;
			LPBYTE pSample = (LPBYTE)pSmp->pSample;
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
			len = pSmp->nLength;
			if (len*4 > dwMemLength) len = dwMemLength >> 2;
			short int *psrc = (short int *)lpMemFile;
			short int *pSample = (short int *)pSmp->pSample;
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
			char packcharacter = lpMemFile[8], *pdest = (char *)pSmp->pSample;
			len += *((LPDWORD)(lpMemFile+4));
			if (len > dwMemLength) len = dwMemLength;
			UINT dmax = pSmp->nLength;
			if (pSmp->uFlags & CHN_16BIT) dmax <<= 1;
			AMSUnpack(psrc+9, len-9, pdest, dmax, packcharacter);
		}
		break;

	// PTM 8bit delta to 16-bit sample
	case RS_PTM8DTO16:
		{
			len = pSmp->nLength * 2;
			if (len > dwMemLength) break;
			signed char *pSample = (signed char *)pSmp->pSample;
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
			LPBYTE pSample = (LPBYTE)pSmp->pSample;
			LPBYTE ibuf = (LPBYTE)lpMemFile;
			DWORD bitbuf = *((DWORD *)ibuf);
			UINT bitnum = 32;
			BYTE dlt = 0, lowbyte = 0;
			ibuf += 4;
			for (UINT j=0; j<pSmp->nLength; j++)
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
			UINT maxlen = pSmp->nLength;
			if (pSmp->uFlags & CHN_16BIT) maxlen <<= 1;
			uint8 *ibuf = (uint8 *)lpMemFile, *ibufmax = (uint8 *)(lpMemFile + dwMemLength);
			len = DMFUnpack((LPBYTE)pSmp->pSample, ibuf, ibufmax, maxlen);
		}
		break;

#ifdef MODPLUG_TRACKER
	// Mono PCM 24/32-bit signed & 32 bit float -> load sample, and normalize it to 16-bit
	case RS_PCM24S:
	case RS_PCM32S:
		len = pSmp->nLength * 3;
		if (nFlags == RS_PCM32S) len += pSmp->nLength;
		if (len > dwMemLength) break;
		if (len > 4*8)
		{
			if(nFlags == RS_PCM24S)
			{
				char* pSrc = (char*)lpMemFile;
				char* pDest = (char*)pSmp->pSample;
				CopyWavBuffer<3, 2, WavSigned24To16, MaxFinderSignedInt<3> >(pSrc, len, pDest, pSmp->GetSampleSizeInBytes());
			}
			else //RS_PCM32S
			{
				char* pSrc = (char*)lpMemFile;
				char* pDest = (char*)pSmp->pSample;
				if(format == 3)
					CopyWavBuffer<4, 2, WavFloat32To16, MaxFinderFloat32>(pSrc, len, pDest, pSmp->GetSampleSizeInBytes());
				else
					CopyWavBuffer<4, 2, WavSigned32To16, MaxFinderSignedInt<4> >(pSrc, len, pDest, pSmp->GetSampleSizeInBytes());
			}
		}
		break;

	// Stereo PCM 24/32-bit signed & 32 bit float -> convert sample to 16 bit
	case RS_STIPCM24S:
	case RS_STIPCM32S:
		if(format == 3 && nFlags == RS_STIPCM32S) //Microsoft IEEE float
		{
			len = pSmp->nLength * 6;
			//pSmp->nLength tells(?) the number of frames there
			//are. One 'frame' of 1 byte(== 8 bit) mono data requires
			//1 byte of space, while one frame of 3 byte(24 bit) 
			//stereo data requires 3*2 = 6 bytes. This is(?)
			//why there is factor 6.

			len += pSmp->nLength * 2;
			//Compared to 24 stereo, 32 bit stereo needs 16 bits(== 2 bytes)
			//more per frame.

			if(len > dwMemLength) break;
			char* pSrc = (char*)lpMemFile;
			char* pDest = (char*)pSmp->pSample;
			if (len > 8*8)
			{
				CopyWavBuffer<4, 2, WavFloat32To16, MaxFinderFloat32>(pSrc, len, pDest, pSmp->GetSampleSizeInBytes());
			}
		}
		else
		{
			len = pSmp->nLength * 6;
			if (nFlags == RS_STIPCM32S) len += pSmp->nLength * 2;
			if (len > dwMemLength) break;
			if (len > 8*8)
			{
				char* pSrc = (char*)lpMemFile;
				char* pDest = (char*)pSmp->pSample;
				if(nFlags == RS_STIPCM32S)
				{
					CopyWavBuffer<4,2,WavSigned32To16, MaxFinderSignedInt<4> >(pSrc, len, pDest, pSmp->GetSampleSizeInBytes());
				}
				if(nFlags == RS_STIPCM24S)
				{
					CopyWavBuffer<3,2,WavSigned24To16, MaxFinderSignedInt<3> >(pSrc, len, pDest, pSmp->GetSampleSizeInBytes());
				}

			}
		}
		break;

	// 16-bit signed big endian interleaved stereo
	case RS_STIPCM16M:
		{
			len = pSmp->nLength;
			if (len*4 > dwMemLength) len = dwMemLength >> 2;
			LPCBYTE psrc = (LPCBYTE)lpMemFile;
			short int *pSample = (short int *)pSmp->pSample;
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
		len = pSmp->nLength;
		if (len > dwMemLength) len = pSmp->nLength = dwMemLength;
		memcpy(pSmp->pSample, lpMemFile, len);
	}
	if (len > dwMemLength)
	{
		if (pSmp->pSample)
		{
			pSmp->nLength = 0;
			FreeSample(pSmp->pSample);
			pSmp->pSample = nullptr;
		}
		return 0;
	}
	AdjustSampleLoop(pSmp);
	return len;
}


void CSoundFile::AdjustSampleLoop(MODSAMPLE *pSmp)
//------------------------------------------------
{
	if ((!pSmp->pSample) || (!pSmp->nLength)) return;
	if (pSmp->nLoopEnd > pSmp->nLength) pSmp->nLoopEnd = pSmp->nLength;
	if (pSmp->nLoopStart >= pSmp->nLoopEnd)
	{
		pSmp->nLoopStart = pSmp->nLoopEnd = 0;
		pSmp->uFlags &= ~CHN_LOOP;
	}
	UINT len = pSmp->nLength;
	if (pSmp->uFlags & CHN_16BIT)
	{
		short int *pSample = (short int *)pSmp->pSample;
		// Adjust end of sample
		if (pSmp->uFlags & CHN_STEREO)
		{
			pSample[len*2+6] = pSample[len*2+4] = pSample[len*2+2] = pSample[len*2] = pSample[len*2-2];
			pSample[len*2+7] = pSample[len*2+5] = pSample[len*2+3] = pSample[len*2+1] = pSample[len*2-1];
		} else
		{
			pSample[len+4] = pSample[len+3] = pSample[len+2] = pSample[len+1] = pSample[len] = pSample[len-1];
		}
		if ((pSmp->uFlags & (CHN_LOOP|CHN_PINGPONGLOOP|CHN_STEREO)) == CHN_LOOP)
		{
			// Fix bad loops
			if ((pSmp->nLoopEnd+3 >= pSmp->nLength) || (m_nType & MOD_TYPE_S3M))
			{
				pSample[pSmp->nLoopEnd] = pSample[pSmp->nLoopStart];
				pSample[pSmp->nLoopEnd+1] = pSample[pSmp->nLoopStart+1];
				pSample[pSmp->nLoopEnd+2] = pSample[pSmp->nLoopStart+2];
				pSample[pSmp->nLoopEnd+3] = pSample[pSmp->nLoopStart+3];
				pSample[pSmp->nLoopEnd+4] = pSample[pSmp->nLoopStart+4];
			}
		}
	} else
	{
		LPSTR pSample = pSmp->pSample;
#ifndef FASTSOUNDLIB
		// Crappy samples (except chiptunes) ?
		if ((pSmp->nLength > 0x100) && (m_nType & (MOD_TYPE_MOD|MOD_TYPE_S3M))
		 && (!(pSmp->uFlags & CHN_STEREO)))
		{
			int smpend = pSample[pSmp->nLength-1], smpfix = 0, kscan;
			for (kscan=pSmp->nLength-1; kscan>0; kscan--)
			{
				smpfix = pSample[kscan-1];
				if (smpfix != smpend) break;
			}
			int delta = smpfix - smpend;
			if (((!(pSmp->uFlags & CHN_LOOP)) || (kscan > (int)pSmp->nLoopEnd))
			 && ((delta < -8) || (delta > 8)))
			{
				while (kscan<(int)pSmp->nLength)
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
		if (pSmp->uFlags & CHN_STEREO)
		{
			pSample[len*2+6] = pSample[len*2+4] = pSample[len*2+2] = pSample[len*2] = pSample[len*2-2];
			pSample[len*2+7] = pSample[len*2+5] = pSample[len*2+3] = pSample[len*2+1] = pSample[len*2-1];
		} else
		{
			pSample[len+4] = pSample[len+3] = pSample[len+2] = pSample[len+1] = pSample[len] = pSample[len-1];
		}
		if ((pSmp->uFlags & (CHN_LOOP|CHN_PINGPONGLOOP|CHN_STEREO)) == CHN_LOOP)
		{
			if ((pSmp->nLoopEnd+3 >= pSmp->nLength) || (m_nType & (MOD_TYPE_MOD|MOD_TYPE_S3M)))
			{
				pSample[pSmp->nLoopEnd] = pSample[pSmp->nLoopStart];
				pSample[pSmp->nLoopEnd+1] = pSample[pSmp->nLoopStart+1];
				pSample[pSmp->nLoopEnd+2] = pSample[pSmp->nLoopStart+2];
				pSample[pSmp->nLoopEnd+3] = pSample[pSmp->nLoopStart+3];
				pSample[pSmp->nLoopEnd+4] = pSample[pSmp->nLoopStart+4];
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


void CSoundFile::FrequencyToTranspose(MODSAMPLE *psmp)
//----------------------------------------------------
{
	int f2t = FrequencyToTranspose(psmp->nC5Speed);
	int transp = f2t >> 7;
	int ftune = f2t & 0x7F; //0x7F == 111 1111
	if (ftune > 80)
	{
		transp++;
		ftune -= 128;
	}
	Limit(transp, -127, 127);
	psmp->RelativeTone = static_cast<int8>(transp);
	psmp->nFineTune = static_cast<int8>(ftune);
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


#ifndef FASTSOUNDLIB

// Check whether a sample is used.
// In sample mode, the sample numbers in all patterns are checked.
// In instrument mode, it is only checked if a sample is referenced by an instrument (but not if the sample is actually played anywhere)
bool CSoundFile::IsSampleUsed(SAMPLEINDEX nSample) const
//------------------------------------------------------
{
	if ((!nSample) || (nSample > GetNumSamples())) return false;
	if (GetNumInstruments())
	{
		for (UINT i = 1; i <= GetNumInstruments(); i++) if (Instruments[i])
		{
			MODINSTRUMENT *pIns = Instruments[i];
			for (UINT j = 0; j < CountOf(pIns->Keyboard); j++)
			{
				if (pIns->Keyboard[j] == nSample) return true;
			}
		}
	} else
	{
		for (UINT i=0; i<Patterns.Size(); i++) if (Patterns[i])
		{
			const MODCOMMAND *m = Patterns[i];
			for (UINT j=m_nChannels*Patterns[i].GetNumRows(); j; m++, j--)
			{
				if (m->instr == nSample && !m->IsPcNote()) return true;
			}
		}
	}
	return false;
}


bool CSoundFile::IsInstrumentUsed(INSTRUMENTINDEX nInstr) const
//-------------------------------------------------------------
{
	if ((!nInstr) || (nInstr > GetNumInstruments()) || (!Instruments[nInstr])) return false;
	for (UINT i=0; i<Patterns.Size(); i++) if (Patterns[i])
	{
		const MODCOMMAND *m = Patterns[i];
		for (UINT j=m_nChannels*Patterns[i].GetNumRows(); j; m++, j--)
		{
			if (m->instr == nInstr && !m->IsPcNote()) return true;
		}
	}
	return false;
}


// Detect samples that are referenced by an instrument, but actually not used in a song.
// Only works in instrument mode. Unused samples are marked as false in the vector.
SAMPLEINDEX CSoundFile::DetectUnusedSamples(vector<bool> &sampleUsed) const
//-------------------------------------------------------------------------
{
	sampleUsed.assign(GetNumSamples() + 1, false);

	if(GetNumInstruments() == 0)
	{
		return 0;
	}
	SAMPLEINDEX nExt = 0;

	for (PATTERNINDEX nPat = 0; nPat < GetNumPatterns(); nPat++)
	{
		const MODCOMMAND *p = Patterns[nPat];
		if(p == nullptr)
		{
			continue;
		}

		UINT jmax = Patterns[nPat].GetNumRows() * GetNumChannels();
		for (UINT j=0; j<jmax; j++, p++)
		{
			if ((p->note) && (p->note <= NOTE_MAX) && (!p->IsPcNote()))
			{
				if ((p->instr) && (p->instr < MAX_INSTRUMENTS))
				{
					MODINSTRUMENT *pIns = Instruments[p->instr];
					if (pIns)
					{
						SAMPLEINDEX n = pIns->Keyboard[p->note-1];
						if (n <= GetNumSamples()) sampleUsed[n] = true;
					}
				} else
				{
					for (INSTRUMENTINDEX k = GetNumInstruments(); k >= 1; k--)
					{
						MODINSTRUMENT *pIns = Instruments[k];
						if (pIns)
						{
							SAMPLEINDEX n = pIns->Keyboard[p->note-1];
							if (n <= GetNumSamples()) sampleUsed[n] = true;
						}
					}
				}
			}
		}
	}
	for (SAMPLEINDEX ichk = GetNumSamples(); ichk >= 1; ichk--)
	{
		if ((!sampleUsed[ichk]) && (Samples[ichk].pSample)) nExt++;
	}

	return nExt;
}


// Destroy samples where keepSamples index is false. First sample is keepSamples[1]!
SAMPLEINDEX CSoundFile::RemoveSelectedSamples(const vector<bool> &keepSamples)
//----------------------------------------------------------------------------
{
	if(keepSamples.empty())
	{
		return 0;
	}
	SAMPLEINDEX nRemoved = 0;
	for(SAMPLEINDEX nSmp = (SAMPLEINDEX)min(GetNumSamples(), keepSamples.size() - 1); nSmp >= 1; nSmp--)
	{
		if(!keepSamples[nSmp])
		{
			if(DestroySample(nSmp))
			{
				nRemoved++;
			}
			if((nSmp == GetNumSamples()) && (nSmp > 1)) m_nSamples--;
		}
	}
	return nRemoved;
}


bool CSoundFile::DestroySample(SAMPLEINDEX nSample)
//-------------------------------------------------
{
	if ((!nSample) || (nSample >= MAX_SAMPLES)) return false;
	if (!Samples[nSample].pSample) return true;
	MODSAMPLE *pSmp = &Samples[nSample];
	LPSTR pSample = pSmp->pSample;
	pSmp->pSample = nullptr;
	pSmp->nLength = 0;
	pSmp->uFlags &= ~(CHN_16BIT);
	for (UINT i=0; i<MAX_CHANNELS; i++)
	{
		if (Chn[i].pSample == pSample)
		{
			Chn[i].nPos = Chn[i].nLength = 0;
			Chn[i].pSample = Chn[i].pCurrentSample = NULL;
		}
	}
	FreeSample(pSample);
	return true;
}

// -> CODE#0020
// -> DESC="rearrange sample list"
bool CSoundFile::MoveSample(SAMPLEINDEX from, SAMPLEINDEX to)
//-----------------------------------------------------------
{
	if (!from || from >= MAX_SAMPLES || !to || to >= MAX_SAMPLES) return false;
	if (/*!Ins[from].pSample ||*/ Samples[to].pSample) return true;

	MODSAMPLE *pFrom = &Samples[from];
	MODSAMPLE *pTo = &Samples[to];

	memcpy(pTo, pFrom, sizeof(MODSAMPLE));

	pFrom->pSample = nullptr;
	pFrom->nLength = 0;
	pFrom->uFlags &= ~(CHN_16BIT);

	return true;
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
	MemsetZero(m_defaultInstrument);
	m_defaultInstrument.nResampling = SRCMODE_DEFAULT;
	m_defaultInstrument.nFilterMode = FLTMODE_UNCHANGED;
	m_defaultInstrument.nPPC = 5*12;
	m_defaultInstrument.nGlobalVol=64;
	m_defaultInstrument.nPan = 0x20 << 2;
	//m_defaultInstrument.nIFC = 0xFF;
	m_defaultInstrument.PanEnv.nReleaseNode=ENV_RELEASE_NODE_UNSET;
	m_defaultInstrument.PitchEnv.nReleaseNode=ENV_RELEASE_NODE_UNSET;
	m_defaultInstrument.VolEnv.nReleaseNode=ENV_RELEASE_NODE_UNSET;
	m_defaultInstrument.wPitchToTempoLock = 0;
	m_defaultInstrument.pTuning = m_defaultInstrument.s_DefaultTuning;
	m_defaultInstrument.nPluginVelocityHandling = PLUGIN_VELOCITYHANDLING_CHANNEL;
	m_defaultInstrument.nPluginVolumeHandling = CSoundFile::s_DefaultPlugVolumeHandling;
}


void CSoundFile::DeleteStaticdata()
//---------------------------------
{
	delete s_pTuningsSharedLocal; s_pTuningsSharedLocal = nullptr;
	delete s_pTuningsSharedBuiltIn; s_pTuningsSharedBuiltIn = nullptr;
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
//----------------------------------
{
	if(s_pTuningsSharedLocal || s_pTuningsSharedBuiltIn) return true;
	//For now not allowing to reload tunings(one should be careful when reloading them
	//since various parts may use addresses of the tuningobjects).

	CTuning::MessageHandler = &SimpleMessageBox;

	s_pTuningsSharedBuiltIn = new CTuningCollection;
	s_pTuningsSharedLocal = new CTuningCollection("Local tunings");

	// Load built-in tunings.
	const char* pData = nullptr;
	HGLOBAL hglob = nullptr;
	size_t nSize = 0;
	if (LoadResource(MAKEINTRESOURCE(IDR_BUILTIN_TUNINGS), TEXT("TUNING"), pData, nSize, hglob) != nullptr)
	{
		std::istrstream iStrm(pData, nSize);
		s_pTuningsSharedBuiltIn->Deserialize(iStrm);
		FreeResource(hglob);
	}
	if(s_pTuningsSharedBuiltIn->GetNumTunings() == 0)
	{
		ASSERT(false);
		CTuningRTI* pT = new CTuningRTI;
		//Note: Tuning collection class handles deleting.
		pT->CreateGeometric(1,1);
		if(s_pTuningsSharedBuiltIn->AddTuning(pT))
			delete pT;
	}
		
	// Load local tunings.
	CString sPath;
	sPath.Format(TEXT("%slocal_tunings%s"), CMainFrame::GetDefaultDirectory(DIR_TUNING), CTuningCollection::s_FileExtension);
	s_pTuningsSharedLocal->SetSavefilePath(sPath);
	s_pTuningsSharedLocal->Deserialize();

	// Enabling adding/removing of tunings for standard collection
	// only for debug builds.
	#ifdef DEBUG
		s_pTuningsSharedBuiltIn->SetConstStatus(CTuningCollection::EM_ALLOWALL);
	#else
		s_pTuningsSharedBuiltIn->SetConstStatus(CTuningCollection::EM_CONST);
	#endif

	MODINSTRUMENT::s_DefaultTuning = NULL;

	return false;
}



void CSoundFile::SetDefaultInstrumentValues(MODINSTRUMENT *pIns)
//--------------------------------------------------------------
{
	pIns->nResampling = m_defaultInstrument.nResampling;
	pIns->nFilterMode = m_defaultInstrument.nFilterMode;
	pIns->PitchEnv.nReleaseNode = m_defaultInstrument.PitchEnv.nReleaseNode;
	pIns->PanEnv.nReleaseNode = m_defaultInstrument.PanEnv.nReleaseNode;
	pIns->VolEnv.nReleaseNode = m_defaultInstrument.VolEnv.nReleaseNode;
	pIns->pTuning = m_defaultInstrument.pTuning;
	pIns->nPluginVelocityHandling = m_defaultInstrument.nPluginVelocityHandling;
	pIns->nPluginVolumeHandling = m_defaultInstrument.nPluginVolumeHandling;

}


long CSoundFile::GetSampleOffset() 
//--------------------------------
{
	//TODO: This is where we could inform patterns of the exact song position when playback starts.
	//order: m_nNextPattern
	//long ticksFromStartOfPattern = m_nRow*m_nMusicSpeed;
	//return ticksFromStartOfPattern*m_nSamplesPerTick;
	return 0;
}

string CSoundFile::GetNoteName(const CTuning::NOTEINDEXTYPE& note, const INSTRUMENTINDEX inst) const
//--------------------------------------------------------------------------------------------------
{
	if((inst >= MAX_INSTRUMENTS && inst != INSTRUMENTINDEX_INVALID) || note < 1 || note > NOTE_MAX) return "BUG";

	// For MPTM instruments with custom tuning, find the appropriate note name. Else, use default note names.
	if(inst != INSTRUMENTINDEX_INVALID && m_nType == MOD_TYPE_MPT && Instruments[inst] && Instruments[inst]->pTuning)
		return Instruments[inst]->pTuning->GetNoteName(note - NOTE_MIDDLEC);
	else
		return szDefaultNoteNames[note - 1];
}


void CSoundFile::SetModSpecsPointer(const CModSpecifications*& pModSpecs, const MODTYPE type)
//------------------------------------------------------------------------------------------
{
	switch(type)
	{
		case MOD_TYPE_MPT:
			pModSpecs = &ModSpecs::mptm;
		break;

		case MOD_TYPE_IT:
			pModSpecs = &ModSpecs::itEx;
		break;

		case MOD_TYPE_XM:
			pModSpecs = &ModSpecs::xmEx;
		break;

		case MOD_TYPE_S3M:
			pModSpecs = &ModSpecs::s3mEx;
		break;

		case MOD_TYPE_MOD:
		default:
			pModSpecs = &ModSpecs::modEx;
			break;
	}
}

uint16 CSoundFile::GetModFlagMask(const MODTYPE oldtype, const MODTYPE newtype) const
//-----------------------------------------------------------------------------------
{
	const MODTYPE combined = oldtype | newtype;

	// XM <-> IT/MPT conversion.
	if(combined == (MOD_TYPE_IT|MOD_TYPE_XM) || combined == (MOD_TYPE_MPT|MOD_TYPE_XM))
		return (1 << MSF_COMPATIBLE_PLAY) | (1 << MSF_MIDICC_BUGEMULATION);

	// IT <-> MPT conversion.
	if(combined == (MOD_TYPE_IT|MOD_TYPE_MPT))
		return uint16_max;

	return (1 << MSF_COMPATIBLE_PLAY);
}

void CSoundFile::ChangeModTypeTo(const MODTYPE& newType)
//------------------------------------------------------
{
	const MODTYPE oldtype = m_nType;
	if (oldtype == newType)
		return;
	m_nType = newType;
	SetModSpecsPointer(m_pModSpecs, m_nType);
	SetupMODPanning(); // Setup LRRL panning scheme if needed
	SetupITBidiMode(); // Setup IT bidi mode

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


double CSoundFile::GetPlaybackTimeAt(ORDERINDEX ord, ROWINDEX row, bool updateVars)
//---------------------------------------------------------------------------------
{
	const GetLengthType t = GetLength(updateVars ? eAdjust : eNoAdjust, ord, row);
	if(t.targetReached) return t.duration;
	else return -1; //Given position not found from play sequence.
}


const CModSpecifications& CSoundFile::GetModSpecifications(const MODTYPE type)
//----------------------------------------------------------------------------
{
	const CModSpecifications* p = nullptr;
	SetModSpecsPointer(p, type);
	return *p;
}


// Set up channel panning and volume suitable for MOD + similar files. If the current mod type is not MOD, bForceSetup has to be set to true.
void CSoundFile::SetupMODPanning(bool bForceSetup)
//------------------------------------------------
{
	// Setup LRRL panning, max channel volume
	if((m_nType & MOD_TYPE_MOD) == 0 && bForceSetup == false) return;

	for(CHANNELINDEX nChn = 0; nChn < MAX_BASECHANNELS; nChn++)
	{
		ChnSettings[nChn].nVolume = 64;
		if(gdwSoundSetup & SNDMIX_MAXDEFAULTPAN)
			ChnSettings[nChn].nPan = (((nChn & 3) == 1) || ((nChn & 3) == 2)) ? 256 : 0;
		else
			ChnSettings[nChn].nPan = (((nChn & 3) == 1) || ((nChn & 3) == 2)) ? 0xC0 : 0x40;
	}
}


// Set or unset the IT bidi loop mode (see Fastmix.cpp for an explanation). The variable has to be static...
void CSoundFile::SetupITBidiMode()
//--------------------------------
{
	m_bITBidiMode = IsCompatibleMode(TRK_IMPULSETRACKER);
}
