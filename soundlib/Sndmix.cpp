/*
 * Sndmix.cpp
 * -----------
 * Purpose: Pattern playback, effect processing
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

// -> CODE#0022
// -> DESC="alternative BPM/Speed interpretation method"
#include "../mptrack/mptrack.h"
#include "../mptrack/moddoc.h"
#include "../mptrack/MainFrm.h"
#include "../common/misc_util.h"
// -! NEW_FEATURE#0022
#include "sndfile.h"
#include "midi.h"
#include "tuning.h"

#ifdef MODPLUG_TRACKER
#define ENABLE_STEREOVU
#endif

// Volume ramp length, in 1/10 ms
#define VOLUMERAMPLEN	0	// 1.46ms = 64 samples at 44.1kHz		//rewbs.soundQ exp - was 146

// VU-Meter
#define VUMETER_DECAY		4

// SNDMIX: These are global flags for playback control
UINT CSoundFile::m_nStereoSeparation = 128;
LONG CSoundFile::m_nStreamVolume = 0x8000;
UINT CSoundFile::m_nMaxMixChannels = MAX_CHANNELS;
// Mixing Configuration (SetWaveConfig)
DWORD CSoundFile::gdwSysInfo = 0;
DWORD CSoundFile::gnChannels = 1;
DWORD CSoundFile::gdwSoundSetup = 0;
DWORD CSoundFile::gdwMixingFreq = 44100;
DWORD CSoundFile::gnBitsPerSample = 16;
// Mixing data initialized in
UINT CSoundFile::gnAGC = AGC_UNITY;
UINT CSoundFile::gnVolumeRampUpSamples = 42;		//default value
UINT CSoundFile::gnVolumeRampDownSamples = 42;		//default value
UINT CSoundFile::gnCPUUsage = 0;
LPSNDMIXHOOKPROC CSoundFile::gpSndMixHook = NULL;
PMIXPLUGINCREATEPROC CSoundFile::gpMixPluginCreateProc = NULL;
LONG gnDryROfsVol = 0;
LONG gnDryLOfsVol = 0;
int gbInitPlugins = 0;
bool gbInitTables = 0;

typedef DWORD (MPPASMCALL * LPCONVERTPROC)(LPVOID, int *, DWORD);

extern DWORD MPPASMCALL X86_Convert32To8(LPVOID lpBuffer, int *, DWORD nSamples);
extern DWORD MPPASMCALL X86_Convert32To16(LPVOID lpBuffer, int *, DWORD nSamples);
extern DWORD MPPASMCALL X86_Convert32To24(LPVOID lpBuffer, int *, DWORD nSamples);
extern DWORD MPPASMCALL X86_Convert32To32(LPVOID lpBuffer, int *, DWORD nSamples);
extern UINT MPPASMCALL X86_AGC(int *pBuffer, UINT nSamples, UINT nAGC);
extern VOID MPPASMCALL X86_Dither(int *pBuffer, UINT nSamples, UINT nBits);
extern VOID MPPASMCALL X86_InterleaveFrontRear(int *pFrontBuf, int *pRearBuf, DWORD nSamples);
extern VOID MPPASMCALL X86_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs);
extern VOID MPPASMCALL X86_MonoFromStereo(int *pMixBuf, UINT nSamples);
extern void SndMixInitializeTables();

extern short int ModSinusTable[64];
extern short int ModRampDownTable[64];
extern short int ModSquareTable[64];
extern short int ModRandomTable[64];
extern short int ITSinusTable[256];
extern short int ITRampDownTable[256];
extern short int ITSquareTable[256];
extern DWORD LinearSlideUpTable[256];
extern DWORD LinearSlideDownTable[256];
extern DWORD FineLinearSlideUpTable[16];
extern DWORD FineLinearSlideDownTable[16];
extern signed char ft2VibratoTable[256];	// -64 .. +64
extern int MixSoundBuffer[MIXBUFFERSIZE * 4];
extern int MixRearBuffer[MIXBUFFERSIZE * 2];

#ifndef NO_REVERB
extern UINT gnReverbSend;
extern LONG gnRvbROfsVol;
extern LONG gnRvbLOfsVol;

extern void ProcessReverb(UINT nSamples);
#endif

// Log tables for pre-amp
// Pre-amp (or more precisely: Pre-attenuation) depends on the number of channels,
// Which this table takes care of.
const UINT PreAmpTable[16] =
{
	0x60, 0x60, 0x60, 0x70,	// 0-7
	0x80, 0x88, 0x90, 0x98,	// 8-15
	0xA0, 0xA4, 0xA8, 0xAC,	// 16-23
	0xB0, 0xB4, 0xB8, 0xBC,	// 24-31
};

const UINT PreAmpAGCTable[16] =
{
	0x60, 0x60, 0x60, 0x64,
	0x68, 0x70, 0x78, 0x80,
	0x84, 0x88, 0x8C, 0x90,
	0x92, 0x94, 0x96, 0x98,
};

typedef CTuning::RATIOTYPE RATIOTYPE;

static const RATIOTYPE TwoToPowerXOver12Table[16] =
{
	1.0F		   , 1.059463094359F, 1.122462048309F, 1.1892071150027F,
	1.259921049895F, 1.334839854170F, 1.414213562373F, 1.4983070768767F,
	1.587401051968F, 1.681792830507F, 1.781797436281F, 1.8877486253634F,
	2.0F		   , 2.118926188719F, 2.244924096619F, 2.3784142300054F
};

inline RATIOTYPE TwoToPowerXOver12(const BYTE i)
//-------------------------------------------
{
	return (i < 16) ? TwoToPowerXOver12Table[i] : 1;
}


// Return (a*b)/c - no divide error
int _muldiv(long a, long b, long c)
{
	int sign, result;
	_asm {
	mov eax, a
	mov ebx, b
	or eax, eax
	mov edx, eax
	jge aneg
	neg eax
aneg:
	xor edx, ebx
	or ebx, ebx
	mov ecx, c
	jge bneg
	neg ebx
bneg:
	xor edx, ecx
	or ecx, ecx
	mov sign, edx
	jge cneg
	neg ecx
cneg:
	mul ebx
	cmp edx, ecx
	jae diverr
	div ecx
	jmp ok
diverr:
	mov eax, 0x7fffffff
ok:
	mov edx, sign
	or edx, edx
	jge rneg
	neg eax
rneg:
	mov result, eax
	}
	return result;
}


// Return (a*b+c/2)/c - no divide error
int _muldivr(long a, long b, long c)
{
	int sign, result;
	_asm {
	mov eax, a
	mov ebx, b
	or eax, eax
	mov edx, eax
	jge aneg
	neg eax
aneg:
	xor edx, ebx
	or ebx, ebx
	mov ecx, c
	jge bneg
	neg ebx
bneg:
	xor edx, ecx
	or ecx, ecx
	mov sign, edx
	jge cneg
	neg ecx
cneg:
	mul ebx
	mov ebx, ecx
	shr ebx, 1
	add eax, ebx
	adc edx, 0
	cmp edx, ecx
	jae diverr
	div ecx
	jmp ok
diverr:
	mov eax, 0x7fffffff
ok:
	mov edx, sign
	or edx, edx
	jge rneg
	neg eax
rneg:
	mov result, eax
	}
	return result;
}


BOOL CSoundFile::InitPlayer(BOOL bReset)
//--------------------------------------
{
#ifndef FASTSOUNDLIB
	if (!gbInitTables)
	{
		SndMixInitializeTables();
		gbInitTables = true;
	}
#endif
	if (m_nMaxMixChannels > MAX_CHANNELS) m_nMaxMixChannels = MAX_CHANNELS;
	if (gdwMixingFreq < 4000) gdwMixingFreq = 4000;
	if (gdwMixingFreq > MAX_SAMPLE_RATE) gdwMixingFreq = MAX_SAMPLE_RATE;
	// Start with ramping disabled to avoid clicks on first read.
	// Ramping is now set after the first read in CSoundFile::Read();
	gnVolumeRampUpSamples = 0; 
	gnVolumeRampDownSamples = CMainFrame::GetSettings().glVolumeRampDownSamples;
	gnDryROfsVol = gnDryLOfsVol = 0;
#ifndef NO_REVERB
	gnRvbROfsVol = gnRvbLOfsVol = 0;
#endif
	if (bReset) gnCPUUsage = 0;
	InitializeDSP(bReset);
#ifdef ENABLE_EQ
	InitializeEQ(bReset);
#endif
	gbInitPlugins = (bReset) ? 3 : 1;
	return TRUE;
}


BOOL CSoundFile::FadeSong(UINT msec)
//----------------------------------
{
	LONG nsamples = _muldiv(msec, gdwMixingFreq, 1000);
	if (nsamples <= 0) return FALSE;
	if (nsamples > 0x100000) nsamples = 0x100000;
	m_nBufferCount = nsamples;
	LONG nRampLength = m_nBufferCount;
	// Ramp everything down
	for (UINT noff=0; noff < m_nMixChannels; noff++)
	{
		ModChannel *pramp = &Chn[ChnMix[noff]];
		if (!pramp) continue;
		pramp->nNewLeftVol = pramp->nNewRightVol = 0;
		pramp->nRightRamp = (-pramp->nRightVol << VOLUMERAMPPRECISION) / nRampLength;
		pramp->nLeftRamp = (-pramp->nLeftVol << VOLUMERAMPPRECISION) / nRampLength;
		pramp->nRampRightVol = pramp->nRightVol << VOLUMERAMPPRECISION;
		pramp->nRampLeftVol = pramp->nLeftVol << VOLUMERAMPPRECISION;
		pramp->nRampLength = nRampLength;
		pramp->dwFlags |= CHN_VOLUMERAMP;
	}
	m_dwSongFlags |= SONG_FADINGSONG;
	return TRUE;
}


BOOL CSoundFile::GlobalFadeSong(UINT msec)
//----------------------------------------
{
	if (m_dwSongFlags & SONG_GLOBALFADE) return FALSE;
	m_nGlobalFadeMaxSamples = _muldiv(msec, gdwMixingFreq, 1000);
	m_nGlobalFadeSamples = m_nGlobalFadeMaxSamples;
	m_dwSongFlags |= SONG_GLOBALFADE;
	return TRUE;
}


UINT CSoundFile::Read(LPVOID lpDestBuffer, UINT cbBuffer)
//-------------------------------------------------------
{
	LPBYTE lpBuffer = (LPBYTE)lpDestBuffer;
	LPCONVERTPROC pCvt = X86_Convert32To8;
	UINT lRead, lMax, lSampleSize, lCount, lSampleCount, nStat=0;
	UINT nMaxPlugins;

	nMaxPlugins = MAX_MIXPLUGINS;
	while ((nMaxPlugins > 0) && (!m_MixPlugins[nMaxPlugins-1].pMixPlugin)) nMaxPlugins--;
	m_nMixStat = 0;

	lSampleSize = gnChannels;
	if (gnBitsPerSample == 16) { lSampleSize *= 2; pCvt = X86_Convert32To16; }
#ifndef FASTSOUNDLIB
	else if (gnBitsPerSample == 24) { lSampleSize *= 3; pCvt = X86_Convert32To24; } 
	else if (gnBitsPerSample == 32) { lSampleSize *= 4; pCvt = X86_Convert32To32; } 
#endif // FASTSOUNDLIB

	lMax = cbBuffer / lSampleSize;
	if ((!lMax) || (!lpBuffer) || (!m_nChannels)) return 0;
	lRead = lMax;
	if (m_dwSongFlags & SONG_ENDREACHED) 
		goto MixDone;

	while (lRead > 0)
	{
		// Update Channel Data
		if (!m_nBufferCount)
		{

#ifndef FASTSOUNDLIB
			if (m_dwSongFlags & SONG_FADINGSONG)
			{
				m_dwSongFlags |= SONG_ENDREACHED;
				m_nBufferCount = lRead;
			} else
#endif // FASTSOUNDLIB

			if (ReadNote())
			{
				// Save pattern cue points for WAV rendering here (if we reached a new pattern, that is.)
				if(m_bIsRendering && (m_PatternCuePoints.empty() || m_nCurrentOrder != m_PatternCuePoints.back().order))
				{
					PatternCuePoint cue;
					cue.offset = lMax - lRead;
					cue.order = m_nCurrentOrder;
					cue.processed = false;	// We don't know the base offset in the file here. It has to be added in the main conversion loop.
					m_PatternCuePoints.push_back(cue);
				}
			} else 
			{
#ifdef MODPLUG_TRACKER
				if ((m_nMaxOrderPosition) && (m_nCurrentOrder >= m_nMaxOrderPosition))
				{
					m_dwSongFlags |= SONG_ENDREACHED;
					break;
				}
#endif // MODPLUG_TRACKER

#ifndef FASTSOUNDLIB
				if (!FadeSong(FADESONGDELAY) || m_bIsRendering)	//rewbs: disable song fade when rendering.
#endif // FASTSOUNDLIB
				{
					m_dwSongFlags |= SONG_ENDREACHED;
					if (lRead == lMax || m_bIsRendering)		//rewbs: don't complete buffer when rendering
						goto MixDone;
					m_nBufferCount = lRead;
				}
			}
		}

		lCount = m_nBufferCount;
		if (lCount > MIXBUFFERSIZE) lCount = MIXBUFFERSIZE;
		if (lCount > lRead) lCount = lRead;
		if (!lCount) 
			break;

		lSampleCount = lCount;

#ifndef NO_REVERB
		gnReverbSend = 0;
#endif // NO_REVERB

		// Resetting sound buffer
		X86_StereoFill(MixSoundBuffer, lSampleCount, &gnDryROfsVol, &gnDryLOfsVol);
		
		ASSERT(lCount<=MIXBUFFERSIZE);		// ensure MIXBUFFERSIZE really is our max buffer size
		if (gnChannels >= 2)
		{
			lSampleCount *= 2;
			m_nMixStat += CreateStereoMix(lCount);

#ifndef NO_REVERB
			ProcessReverb(lCount);
#endif // NO_REVERB

			if (nMaxPlugins) ProcessPlugins(lCount);

			// Apply global volume
			if (m_pConfig->getGlobalVolumeAppliesToMaster())
			{
				ApplyGlobalVolume(MixSoundBuffer, MixRearBuffer, lSampleCount);
			}

			ProcessStereoDSP(lCount);
		} else
		{
			m_nMixStat += CreateStereoMix(lCount);

#ifndef NO_REVERB
			ProcessReverb(lCount);
#endif // NO_REVERB

			if (nMaxPlugins) ProcessPlugins(lCount);
			X86_MonoFromStereo(MixSoundBuffer, lCount);

			// Apply global volume
			if (m_pConfig->getGlobalVolumeAppliesToMaster())
			{
				ApplyGlobalVolume(MixSoundBuffer, nullptr, lSampleCount);
			}

			ProcessMonoDSP(lCount);
		}

#ifdef ENABLE_EQ
		// Graphic Equalizer
		if (gdwSoundSetup & SNDMIX_EQ)
		{
			if (gnChannels >= 2)
				EQStereo(MixSoundBuffer, lCount);
			else
				EQMono(MixSoundBuffer, lCount);
		}
#endif // ENABLE_EQ

		nStat++;

#ifndef NO_AGC
		// Automatic Gain Control
		if (gdwSoundSetup & SNDMIX_AGC) ProcessAGC(lSampleCount);
#endif // NO_AGC

		UINT lTotalSampleCount = lSampleCount;	// Including rear channels

#ifndef FASTSOUNDLIB
		// Multichannel
		if (gnChannels > 2)
		{
			X86_InterleaveFrontRear(MixSoundBuffer, MixRearBuffer, lSampleCount);
			lTotalSampleCount *= 2;
		}
#endif // FASTSOUNDLIB

#ifndef FASTSOUNDLIB
		// Noise Shaping
		if (gnBitsPerSample <= 16)
		{
			if ((gdwSoundSetup & SNDMIX_HQRESAMPLER)
			 && ((gnCPUUsage < 25) || (gdwSoundSetup & SNDMIX_DIRECTTODISK)))
				X86_Dither(MixSoundBuffer, lTotalSampleCount, gnBitsPerSample);
		}

		// Hook Function
		if (gpSndMixHook)
		{
			//Currently only used for VU Meter, so it's OK to do it after global Vol.
			gpSndMixHook(MixSoundBuffer, lTotalSampleCount, gnChannels);
		}
#endif // FASTSOUNDLIB

		// Perform clipping
		lpBuffer += pCvt(lpBuffer, MixSoundBuffer, lTotalSampleCount);

		// Buffer ready
		lRead -= lCount;
		m_nBufferCount -= lCount;
		m_lTotalSampleCount += lCount;		// increase sample count for VSTTimeInfo.
		// Turn on ramping after first read (fix http://forum.openmpt.org/index.php?topic=523.0 )
		gnVolumeRampUpSamples = CMainFrame::GetSettings().glVolumeRampUpSamples;
	}
MixDone:
	if (lRead) memset(lpBuffer, (gnBitsPerSample == 8) ? 0x80 : 0, lRead * lSampleSize);
	if (nStat) { m_nMixStat += nStat-1; m_nMixStat /= nStat; }
	return lMax - lRead;
}


#ifdef MODPLUG_PLAYER
UINT CSoundFile::ReadMix(LPVOID lpDestBuffer, UINT cbBuffer, CSoundFile *pSndFile, DWORD *pstatus, LPBYTE pstream)
//----------------------------------------------------------------------------------------------------------------
{
	LPBYTE lpBuffer = (LPBYTE)lpDestBuffer;
	LPCONVERTPROC pCvt = X86_Convert32To8;
	UINT lRead, lMax, lSampleSize, lCount, lSampleCount, nStat1=0, nStat2=0;
	BOOL active1, active2, bstream = FALSE, b16stream = FALSE;
	DWORD dwenable = *pstatus;
	UINT nMaxPlugins;
		
	{
		nMaxPlugins = MAX_MIXPLUGINS;
		while ((nMaxPlugins > 0) && (!m_MixPlugins[nMaxPlugins-1].pMixPlugin)) nMaxPlugins--;
	}
	lSampleSize = gnChannels;
	if (gnBitsPerSample == 16) { lSampleSize *= 2; pCvt = X86_Convert32To16; }
	else if (gnBitsPerSample == 24) { lSampleSize *= 3; pCvt = X86_Convert32To24; } 
	else if (gnBitsPerSample == 32) { lSampleSize *= 4; pCvt = X86_Convert32To32; } 
	lMax = cbBuffer / lSampleSize;
	if ((!lMax) || (!lpBuffer) || (!pSndFile) || (!pstatus)) return 0;
	if (*pstatus & 4) bstream = TRUE;
	if (*pstatus & 8) b16stream = TRUE;
	m_nMixStat = 0;
	pSndFile->m_nMixStat = 0;
	lRead = lMax;
	*pstatus = 0;
	while (lRead > 0)
	{
		active1 = active2 = FALSE;
		lCount = 0;
		if ((m_nChannels) && (dwenable & 1))
		{
			if (!m_nBufferCount)
			{
				if (m_dwSongFlags & SONG_ENDREACHED)
				{
					m_nBufferCount = 0;
					*pstatus &= ~1;
				} else
				if (m_dwSongFlags & SONG_FADINGSONG)
				{
					m_dwSongFlags |= SONG_ENDREACHED;
					m_nBufferCount = lRead;
					*pstatus &= ~1;
				} else
				if (!ReadNote())
				{
					if (!FadeSong(FADESONGDELAY))
					{
						m_dwSongFlags |= SONG_ENDREACHED;
						m_nBufferCount = lRead;
						*pstatus &= ~1;
					}
				}
			}
			lCount = m_nBufferCount;
			if (lCount)
			{
				*pstatus |= 1;
				active1 = TRUE;
			}
		}
		if ((pSndFile->m_nChannels) && (dwenable & 2))
		{
			if (!pSndFile->m_nBufferCount)
			{
				if (pSndFile->m_dwSongFlags & SONG_ENDREACHED)
				{
					pSndFile->m_nBufferCount = 0;
					*pstatus &= ~2;
				} else
				if (pSndFile->m_dwSongFlags & SONG_FADINGSONG)
				{
					pSndFile->m_dwSongFlags |= SONG_ENDREACHED;
					pSndFile->m_nBufferCount = lRead;
					*pstatus &= ~2;
				} else
				if (!pSndFile->ReadNote())
				{
					if (!pSndFile->FadeSong(FADESONGDELAY))
					{
						pSndFile->m_dwSongFlags |= SONG_ENDREACHED;
						pSndFile->m_nBufferCount = lRead;
						*pstatus &= ~2;
					}
				}
			}
			if (pSndFile->m_nBufferCount)
			{
				*pstatus |= 2;
				active2 = TRUE;
				if ((!lCount) || (pSndFile->m_nBufferCount < lCount)) lCount = pSndFile->m_nBufferCount;
			}
		}
		// No sound?
		if (!lCount)
		{
			if ((lRead == lMax) && (!bstream)) break;
			lCount = lRead;
		}
		if (lCount > MIXBUFFERSIZE) lCount = MIXBUFFERSIZE;
		if (lCount > lRead) lCount = lRead;
		lSampleCount = lCount;
		if (!lCount) break;
		// Resetting sound buffer
	#ifndef NO_REVERB
		gnReverbSend = 0;
	#endif
		X86_StereoFill(MixSoundBuffer, lSampleCount, &gnDryROfsVol, &gnDryLOfsVol);
		if (pstream)
		{
			if (b16stream)
			{
				signed short *p = (signed short *)pstream;
				for (UINT i=0; i<lSampleCount; i++, p++)
				{
					int k = (int)(*p) * (m_nStreamVolume >> MIXING_ATTENUATION);
					MixSoundBuffer[i*2] += k;
					MixSoundBuffer[i*2+1] += k;
				}
				pstream = (LPBYTE)p;
			} else
			{
				BYTE *p = (BYTE *)pstream;
				for (UINT i=0; i<lSampleCount; i++, p++)
				{
					int k = ((((int)(*p)) - 0x80) << 8) * (m_nStreamVolume >> MIXING_ATTENUATION);
					MixSoundBuffer[i*2] += k;
					MixSoundBuffer[i*2+1] += k;
				}
				pstream = (LPBYTE)p;
			}
		}
		if (gnChannels >= 2)
		{
			lSampleCount *= 2;
			if (active1) { m_nMixStat += CreateStereoMix(lCount); nStat1++; }
			if (active2) { pSndFile->m_nMixStat += pSndFile->CreateStereoMix(lCount); nStat2++; }
		#ifndef NO_REVERB
			ProcessReverb(lCount);
		#endif
			ProcessStereoDSP(lCount);
		} else
		{
			if (active1) { m_nMixStat += CreateStereoMix(lCount); nStat1++; }
			if (active2) { pSndFile->m_nMixStat += pSndFile->CreateStereoMix(lCount); nStat2++; }
		#ifndef NO_REVERB
			ProcessReverb(lCount);
		#endif
			X86_MonoFromStereo(MixSoundBuffer, lCount);
			ProcessMonoDSP(lCount);
		}
#ifdef ENABLE_EQ
		if (gdwSoundSetup & SNDMIX_EQ)
		{
			if (gnChannels >= 2)
				EQStereo(MixSoundBuffer, lCount);
			else
				EQMono(MixSoundBuffer, lCount);
		}
#endif
#ifndef NO_AGC
		// Automatic Gain Control
		if (gdwSoundSetup & SNDMIX_AGC) ProcessAGC(lSampleCount);
#endif
		UINT lTotalSampleCount = lSampleCount;
		// Multichannel
		if (gnChannels > 2)
		{
			X86_InterleaveFrontRear(MixSoundBuffer, MixRearBuffer, lSampleCount);
			lTotalSampleCount *= 2;
		}
		// Noise Shaping
		if (gnBitsPerSample <= 24)
		{
			if ((gdwSoundSetup & SNDMIX_HQRESAMPLER)
			 && ((gnCPUUsage < 25) || (gdwSoundSetup & SNDMIX_DIRECTTODISK)))
				X86_Dither(MixSoundBuffer, lTotalSampleCount, gnBitsPerSample);
		}

		// Perform clipping + VU-Meter
		lpBuffer += pCvt(lpBuffer, MixSoundBuffer, lTotalSampleCount);
		// Buffer ready
		lRead -= lCount;
		if (active1) m_nBufferCount -= lCount;
		if (active2) pSndFile->m_nBufferCount -= lCount;
	}
	if (lRead) memset(lpBuffer, (gnBitsPerSample == 8) ? 0x80 : 0, lRead * lSampleSize);
	if (nStat1) { m_nMixStat += nStat1 - 1; m_nMixStat /= nStat1; }
	if (nStat2) { pSndFile->m_nMixStat += nStat2 - 1; pSndFile->m_nMixStat /= nStat2; }
	return lMax - lRead;
}
#endif // FASTSOUNDLIB


/////////////////////////////////////////////////////////////////////////////
// Handles navigation/effects

BOOL CSoundFile::ProcessRow()
//---------------------------
{
	if (++m_nTickCount >= GetNumTicksOnCurrentRow())
	{
		HandlePatternTransitionEvents();
		m_nPatternDelay = 0;
		m_nFrameDelay = 0;
		m_nTickCount = 0;
		m_nRow = m_nNextRow;
		// Reset Pattern Loop Effect
		if (m_nCurrentOrder != m_nNextOrder) 
			m_nCurrentOrder = m_nNextOrder;
		// Check if pattern is valid
		if (!(m_dwSongFlags & SONG_PATTERNLOOP))
		{
			m_nPattern = (m_nCurrentOrder < Order.size()) ? Order[m_nCurrentOrder] : Order.GetInvalidPatIndex();
			if ((m_nPattern < Patterns.Size()) && (!Patterns[m_nPattern])) m_nPattern = Order.GetIgnoreIndex();
			while (m_nPattern >= Patterns.Size())
			{
				// End of song?
				if ((m_nPattern == Order.GetInvalidPatIndex()) || (m_nCurrentOrder >= Order.size()))
				{

					//if (!m_nRepeatCount) return FALSE;

					ORDERINDEX nRestartPosOverride = m_nRestartPos;
					if(!m_nRestartPos && m_nCurrentOrder <= Order.size() && m_nCurrentOrder > 0)
					{
						/* Subtune detection. Subtunes are separated by "---" order items, so if we're in a
						   subtune and there's no restart position, we go to the first order of the subtune
						   (i.e. the first order after the previous "---" item) */
						for(ORDERINDEX iOrd = m_nCurrentOrder - 1; iOrd > 0; iOrd--)
						{
							if(Order[iOrd] == Order.GetInvalidPatIndex())
							{
								// Jump back to first order of this subtune
								nRestartPosOverride = iOrd + 1;
								break;
							}
						}
					}

					// If channel resetting is disabled in MPT, we will emulate a pattern break (and we always do it if we're not in MPT)
#ifdef MODPLUG_TRACKER
					if(!(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_RESETCHANNELS))
#endif // MODPLUG_TRACKER
					{
						m_dwSongFlags |= SONG_BREAKTOROW;
					}

					if (!nRestartPosOverride && !(m_dwSongFlags & SONG_BREAKTOROW))
					{
						//rewbs.instroVSTi: stop all VSTi at end of song, if looping.
						StopAllVsti();
						m_nMusicSpeed = m_nDefaultSpeed;
						m_nMusicTempo = m_nDefaultTempo;
						m_nGlobalVolume = m_nDefaultGlobalVolume;
						for (UINT i=0; i<MAX_CHANNELS; i++)
						{
							Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
							Chn[i].nFadeOutVol = 0;

							if (i < m_nChannels)
							{
								Chn[i].nGlobalVol = ChnSettings[i].nVolume;
								Chn[i].nVolume = ChnSettings[i].nVolume;
								Chn[i].nPan = ChnSettings[i].nPan;
								Chn[i].nPanSwing = Chn[i].nVolSwing = 0;
								Chn[i].nCutSwing = Chn[i].nResSwing = 0;
								Chn[i].nOldVolParam = 0;
								Chn[i].nOldOffset = 0;
								Chn[i].nOldHiOffset = 0;
								Chn[i].nPortamentoDest = 0;

								if (!Chn[i].nLength)
								{
									Chn[i].dwFlags = ChnSettings[i].dwFlags;
									Chn[i].nLoopStart = 0;
									Chn[i].nLoopEnd = 0;
									Chn[i].pModInstrument = nullptr;
									Chn[i].pSample = nullptr;
									Chn[i].pModSample = nullptr;
								}
							}
						}


					}

					//Handle Repeat position
					if (m_nRepeatCount > 0) m_nRepeatCount--;
					m_nCurrentOrder = nRestartPosOverride;
					m_dwSongFlags &= ~SONG_BREAKTOROW;
					//If restart pos points to +++, move along
					while (Order[m_nCurrentOrder] == Order.GetIgnoreIndex())
					{
						m_nCurrentOrder++;
					}
					//Check for end of song or bad pattern
					if (m_nCurrentOrder >= Order.size()
						|| (Order[m_nCurrentOrder] >= Patterns.Size()) 
						|| (!Patterns[Order[m_nCurrentOrder]]) )
					{
						InitializeVisitedRows(true);
						return FALSE;
					}

				} else
				{
					m_nCurrentOrder++;
				}

				if (m_nCurrentOrder < Order.size())
					m_nPattern = Order[m_nCurrentOrder];
				else
					m_nPattern = Order.GetInvalidPatIndex();

				if ((m_nPattern < Patterns.Size()) && (!Patterns[m_nPattern]))
					m_nPattern = Order.GetIgnoreIndex();
			}
			m_nNextOrder = m_nCurrentOrder;

#ifdef MODPLUG_TRACKER
			if ((m_nMaxOrderPosition) && (m_nCurrentOrder >= m_nMaxOrderPosition)) return FALSE;
#endif // MODPLUG_TRACKER
		}

		// Weird stuff?
		if (!Patterns.IsValidPat(m_nPattern)) return FALSE;
		// Should never happen
		if (m_nRow >= Patterns[m_nPattern].GetNumRows()) m_nRow = 0;

		// Has this row been visited before? We might want to stop playback now.
		// But: We will not mark the row as modified if the song is not in loop mode but
		// the pattern loop (editor flag, not to be confused with the pattern loop effect)
		// flag is set - because in that case, the module would stop after the first pattern loop...
		const bool overrideLoopCheck = (m_nRepeatCount != -1) && (m_dwSongFlags & SONG_PATTERNLOOP);
		if(!overrideLoopCheck && IsRowVisited(m_nCurrentOrder, m_nRow, true))
		{
			if(m_nRepeatCount)
			{
				// repeat count == -1 means repeat infinitely.
				if (m_nRepeatCount > 0)
				{
					m_nRepeatCount--;
				}
				// Forget all but the current row.
				InitializeVisitedRows(true);
				SetRowVisited(m_nCurrentOrder, m_nRow);
			} else
			{
#ifdef MODPLUG_TRACKER
				// Let's check again if this really is the end of the song.
				// The visited rows vector might have been screwed up while editing...
				// This is of course not possible during rendering to WAV, so we ignore that case.
				GetLengthType t = GetLength(eNoAdjust);
				if((gdwSoundSetup & SNDMIX_DIRECTTODISK) || (t.lastOrder == m_nCurrentOrder && t.lastRow == m_nRow))
#else
				if(1)
#endif // MODPLUG_TRACKER
				{
					// This is really the song's end!
					InitializeVisitedRows(true);
					return FALSE;
				} else
				{
					// Ok, this is really dirty, but we have to update the visited rows vector...
					GetLength(eAdjustOnSuccess, m_nCurrentOrder, m_nRow);
				}
			}
		}

		m_nNextRow = m_nRow + 1;
		if (m_nNextRow >= Patterns[m_nPattern].GetNumRows())
		{
			if (!(m_dwSongFlags & SONG_PATTERNLOOP)) m_nNextOrder = m_nCurrentOrder + 1;
			m_bPatternTransitionOccurred = true;
			m_nNextRow = 0;

			// FT2 idiosyncrasy: When E60 is used on a pattern row x, the following pattern also starts from row x
			// instead of the beginning of the pattern, unless there was a Bxx or Dxx effect.
			if(IsCompatibleMode(TRK_FASTTRACKER2))
			{
				m_nNextRow = m_nNextPatStartRow;
				m_nNextPatStartRow = 0;
			}
		}
		// Reset channel values
		ModChannel *pChn = Chn;
		ModCommand *m = Patterns[m_nPattern].GetRow(m_nRow);
		for (CHANNELINDEX nChn=0; nChn<m_nChannels; pChn++, nChn++, m++)
		{
			pChn->rowCommand = *m;

			pChn->nLeftVol = pChn->nNewLeftVol;
			pChn->nRightVol = pChn->nNewRightVol;
			pChn->dwFlags &= ~(CHN_PORTAMENTO | CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO);
			pChn->nCommand = 0;
			pChn->m_plugParamValueStep = 0;
		}

		// Now that we know which pattern we're on, we can update time signatures (global or pattern-specific)
		UpdateTimeSignature();

	}
	// Should we process tick0 effects?
	if (!m_nMusicSpeed) m_nMusicSpeed = 1;
	m_dwSongFlags |= SONG_FIRSTTICK;

	//End of row? stop pattern step (aka "play row").
#ifdef MODPLUG_TRACKER
	if (m_nTickCount >= GetNumTicksOnCurrentRow() - 1)
	{
		if (m_dwSongFlags & SONG_STEP)
		{
			m_dwSongFlags &= ~SONG_STEP;
			m_dwSongFlags |= SONG_PAUSED;
		}
	}
#endif // MODPLUG_TRACKER

	if (m_nTickCount)
	{
		m_dwSongFlags &= ~SONG_FIRSTTICK;
		if ((!(m_nType & MOD_TYPE_XM)) && (m_nTickCount < GetNumTicksOnCurrentRow()))
		{
			if (!(m_nTickCount % m_nMusicSpeed)) m_dwSongFlags |= SONG_FIRSTTICK;
		}
	}

	// Update Effects
	return ProcessEffects();
}


////////////////////////////////////////////////////////////////////////////////////////////
// Channel effect processing


// Calculate delta for Vibrato / Tremolo / Panbrello effect
int CSoundFile::GetVibratoDelta(int type, int position) const
//-----------------------------------------------------------
{
	switch(type & 0x03)
	{
	case 0:
	default:
		// IT compatibility: IT has its own, more precise tables
		return IsCompatibleMode(TRK_IMPULSETRACKER) ? ITSinusTable[position] : ModSinusTable[position];

	case 1:
		// IT compatibility: IT has its own, more precise tables
		return IsCompatibleMode(TRK_IMPULSETRACKER) ? ITRampDownTable[position] : ModRampDownTable[position];

	case 2:
		// IT compatibility: IT has its own, more precise tables
		return IsCompatibleMode(TRK_IMPULSETRACKER) ? ITSquareTable[position] : ModSquareTable[position];

	case 3:
		//IT compatibility 19. Use random values
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
			// TODO delay is not taken into account!
			return (rand() & 0x7F) - 0x40;
		else
			return ModRandomTable[position];
	}
}


void CSoundFile::ProcessVolumeSwing(ModChannel *pChn, int &vol)
//-------------------------------------------------------------
{
	if(IsCompatibleMode(TRK_IMPULSETRACKER))
	{
		vol += pChn->nVolSwing;
		Limit(vol, 0, 64);
	} else if(GetModFlag(MSF_OLDVOLSWING))
	{
		vol += pChn->nVolSwing;
		Limit(vol, 0, 256);
	} else
	{
		pChn->nVolume += pChn->nVolSwing;
		Limit(pChn->nVolume, 0, 256);
		vol = pChn->nVolume;
		pChn->nVolSwing = 0;
	}
}


void CSoundFile::ProcessPanningSwing(ModChannel *pChn)
//----------------------------------------------------
{
	if(IsCompatibleMode(TRK_IMPULSETRACKER) || GetModFlag(MSF_OLDVOLSWING))
	{
		pChn->nRealPan = pChn->nPan + pChn->nPanSwing;
		Limit(pChn->nRealPan, 0, 256);
	} else
	{
		pChn->nPan += pChn->nPanSwing;
		Limit(pChn->nPan, 0, 256);
		pChn->nPanSwing = 0;
		pChn->nRealPan = pChn->nPan;
	}
}


void CSoundFile::ProcessTremolo(ModChannel *pChn, int &vol)
//---------------------------------------------------------
{
	if (pChn->dwFlags & CHN_TREMOLO)
	{
		UINT trempos = pChn->nTremoloPos;
		// IT compatibility: Why would you not want to execute tremolo at volume 0?
		if (vol > 0 || IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			// IT compatibility: We don't need a different attenuation here because of the different tables we're going to use
			const int tremattn = ((GetType() & MOD_TYPE_XM) || IsCompatibleMode(TRK_IMPULSETRACKER)) ? 5 : 6;

			vol += (GetVibratoDelta(pChn->nTremoloType, trempos) * (int)pChn->nTremoloDepth) >> tremattn;
		}
		if (!(m_dwSongFlags & SONG_FIRSTTICK) || ((GetType() & (MOD_TYPE_STM|MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)) && (!(m_dwSongFlags & SONG_ITOLDEFFECTS))))
		{
			// IT compatibility: IT has its own, more precise tables
			if(IsCompatibleMode(TRK_IMPULSETRACKER))
				pChn->nTremoloPos = (pChn->nTremoloPos + 4 * pChn->nTremoloSpeed) & 0xFF;
			else
				pChn->nTremoloPos = (pChn->nTremoloPos + pChn->nTremoloSpeed) & 0x3F;
		}
	}
}


void CSoundFile::ProcessTremor(ModChannel *pChn, int &vol)
//--------------------------------------------------------
{
	if(pChn->nCommand == CMD_TREMOR)
	{
		// IT compatibility 12. / 13.: Tremor
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			if ((pChn->nTremorCount & 128) && pChn->nLength)
			{
				if (pChn->nTremorCount == 128)
					pChn->nTremorCount = (pChn->nTremorParam >> 4) | 192;
				else if (pChn->nTremorCount == 192)
					pChn->nTremorCount = (pChn->nTremorParam & 0x0F) | 128;
				else
					pChn->nTremorCount--;
			}

			if ((pChn->nTremorCount & 192) == 128)
				vol = 0;
		}
		else
		{
			UINT ontime = pChn->nTremorParam >> 4;
			UINT n = ontime + (pChn->nTremorParam & 0x0F);	// Total tremor cycle time (On + Off)
			if ((!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) || (m_dwSongFlags & SONG_ITOLDEFFECTS))
			{
				n += 2;
				ontime++;
			}
			UINT tremcount = (UINT)pChn->nTremorCount;
			if(!(GetType() & MOD_TYPE_XM))
			{
				if (tremcount >= n) tremcount = 0;
				if (tremcount >= ontime) vol = 0;
				pChn->nTremorCount = (BYTE)(tremcount + 1);
			} else
			{
				if(m_dwSongFlags & SONG_FIRSTTICK)
				{
					// tremcount is only 0 on the first tremor tick after triggering a note.
					if(tremcount > 0)
					{
						tremcount--;
					}
				} else
				{
					pChn->nTremorCount = (BYTE)(tremcount + 1);
				}
				if (tremcount % n >= ontime) vol = 0;
			}
		}
		pChn->dwFlags |= CHN_FASTVOLRAMP;
	}
}


bool CSoundFile::IsEnvelopeProcessed(const ModChannel *pChn, enmEnvelopeTypes env) const
//--------------------------------------------------------------------------------------
{
	if(pChn->pModInstrument == nullptr)
	{
		return false;
	}
	const InstrumentEnvelope &insEnv = pChn->pModInstrument->GetEnvelope(env);

	// IT Compatibility: S77/S79/S7B do not disable the envelope, they just pause the counter
	return (((pChn->GetEnvelope(env).flags & ENV_ENABLED) || ((insEnv.dwFlags & ENV_ENABLED) && IsCompatibleMode(TRK_IMPULSETRACKER)))
		&& insEnv.nNodes != 0);
}


void CSoundFile::ProcessVolumeEnvelope(ModChannel *pChn, int &vol)
//----------------------------------------------------------------
{
	if(IsEnvelopeProcessed(pChn, ENV_VOLUME))
	{
		const ModInstrument *pIns = pChn->pModInstrument;

		if(IsCompatibleMode(TRK_IMPULSETRACKER) && pChn->VolEnv.nEnvPosition == 0)
		{
			// If the envelope is disabled at the very same moment as it is triggered, we do not process anything.
			return;
		}
		const int envpos = pChn->VolEnv.nEnvPosition - (IsCompatibleMode(TRK_IMPULSETRACKER) ? 1 : 0);
		int envvol = GetVolEnvValueFromPosition(envpos, pIns->VolEnv);

		// if we are in the release portion of the envelope,
		// rescale envelope factor so that it is proportional to the release point
		// and release envelope beginning.
		if (pIns->VolEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET
			&& envpos >= pIns->VolEnv.Ticks[pIns->VolEnv.nReleaseNode]
		&& pChn->VolEnv.nEnvValueAtReleaseJump != NOT_YET_RELEASED)
		{
			int envValueAtReleaseJump = pChn->VolEnv.nEnvValueAtReleaseJump;
			int envValueAtReleaseNode = pIns->VolEnv.Values[pIns->VolEnv.nReleaseNode] << 2;

			//If we have just hit the release node, force the current env value
			//to be that of the release node. This works around the case where 
			// we have another node at the same position as the release node.
			if (envpos == pIns->VolEnv.Ticks[pIns->VolEnv.nReleaseNode])
				envvol = envValueAtReleaseNode;

			int relativeVolumeChange = (envvol - envValueAtReleaseNode) * 2;
			envvol = envValueAtReleaseJump + relativeVolumeChange;
		}
		vol = (vol * Clamp(envvol, 0, 512)) >> 8;
	}

}


void CSoundFile::ProcessPanningEnvelope(ModChannel *pChn)
//-------------------------------------------------------
{
	if(IsEnvelopeProcessed(pChn, ENV_PANNING))
	{
		const ModInstrument *pIns = pChn->pModInstrument;

		if(IsCompatibleMode(TRK_IMPULSETRACKER) && pChn->PanEnv.nEnvPosition == 0)
		{
			// If the envelope is disabled at the very same moment as it is triggered, we do not process anything.
			return;
		}
		const int envpos = pChn->PanEnv.nEnvPosition - (IsCompatibleMode(TRK_IMPULSETRACKER) ? 1 : 0);

		UINT pt = pIns->PanEnv.nNodes - 1;
		for (UINT i = 0; i < (UINT)(pIns->PanEnv.nNodes - 1); i++)
		{
			if (envpos <= pIns->PanEnv.Ticks[i])
			{
				pt = i;
				break;
			}
		}
		int x2 = pIns->PanEnv.Ticks[pt], y2 = pIns->PanEnv.Values[pt];
		int x1, envpan;
		if (envpos >= x2)
		{
			envpan = y2;
			x1 = x2;
		} else if (pt)
		{
			envpan = pIns->PanEnv.Values[pt - 1];
			x1 = pIns->PanEnv.Ticks[pt - 1];
		} else
		{
			envpan = 128;
			x1 = 0;
		}
		if ((x2 > x1) && (envpos > x1))
		{
			envpan += ((envpos - x1) * (y2 - envpan)) / (x2 - x1);
		}

		envpan = Clamp(envpan, 0, 64);
		int pan = pChn->nPan;
		if (pan >= 128)
		{
			pan += ((envpan - 32) * (256 - pan)) / 32;
		} else
		{
			pan += ((envpan - 32) * (pan)) / 32;
		}

		pChn->nRealPan = Clamp(pan, 0, 256);
	}
}


void CSoundFile::ProcessPitchFilterEnvelope(ModChannel *pChn, int &period)
//------------------------------------------------------------------------
{
	if(IsEnvelopeProcessed(pChn, ENV_PITCH))
	{
		const ModInstrument *pIns = pChn->pModInstrument;

		if(IsCompatibleMode(TRK_IMPULSETRACKER) && pChn->PitchEnv.nEnvPosition == 0)
		{
			// If the envelope is disabled at the very same moment as it is triggered, we do not process anything.
			return;
		}
		int envpos = pChn->PitchEnv.nEnvPosition - (IsCompatibleMode(TRK_IMPULSETRACKER) ? 1 : 0);

		UINT pt = pIns->PitchEnv.nNodes - 1;
		for (UINT i=0; i<(UINT)(pIns->PitchEnv.nNodes-1); i++)
		{
			if (envpos <= pIns->PitchEnv.Ticks[i])
			{
				pt = i;
				break;
			}
		}
		int x2 = pIns->PitchEnv.Ticks[pt];
		int x1, envpitch;
		if (envpos >= x2)
		{
			envpitch = (((int)pIns->PitchEnv.Values[pt]) - ENVELOPE_MID) * 8;
			x1 = x2;
		} else if (pt)
		{
			envpitch = (((int)pIns->PitchEnv.Values[pt-1]) - ENVELOPE_MID) * 8;
			x1 = pIns->PitchEnv.Ticks[pt-1];
		} else
		{
			envpitch = 0;
			x1 = 0;
		}
		if (envpos > x2) envpos = x2;
		if ((x2 > x1) && (envpos > x1))
		{
			int envpitchdest = (((int)pIns->PitchEnv.Values[pt]) - 32) * 8;
			envpitch += ((envpos - x1) * (envpitchdest - envpitch)) / (x2 - x1);
		}
		Limit(envpitch, -256, 256);

		//if (pIns->PitchEnv.dwFlags & ENV_FILTER)
		if (pChn->PitchEnv.flags & ENV_FILTER)
		{
			// Filter Envelope: controls cutoff frequency
#ifndef NO_FILTER
			SetupChannelFilter(pChn, !(pChn->dwFlags & CHN_FILTER), envpitch);
#endif // NO_FILTER
		} else
		{
			// Pitch Envelope
			if(m_nType == MOD_TYPE_MPT && pChn->pModInstrument && pChn->pModInstrument->pTuning)
			{
				if(pChn->nFineTune != envpitch)
				{
					pChn->nFineTune = envpitch;
					pChn->m_CalculateFreq = true;
					//Preliminary tests indicated that this behavior
					//is very close to original(with 12TET) when finestep count
					//is 15.
				}
			}
			else //Original behavior
			{
				int l = envpitch;
				if (l < 0)
				{
					l = -l;
					if (l > 255) l = 255;
					period = _muldiv(period, LinearSlideUpTable[l], 0x10000);
				} else
				{
					if (l > 255) l = 255;
					period = _muldiv(period, LinearSlideDownTable[l], 0x10000);
				}
			} //End: Original behavior.
		}
	}
}


void CSoundFile::IncrementVolumeEnvelopePosition(ModChannel *pChn)
//----------------------------------------------------------------
{
	const ModInstrument *pIns = pChn->pModInstrument;

	if (pChn->VolEnv.flags & ENV_ENABLED)
	{
		// Increase position
		UINT position = pChn->VolEnv.nEnvPosition + (IsCompatibleMode(TRK_IMPULSETRACKER) ? 0 : 1);
		// Volume Loop ?
		if (pIns->VolEnv.dwFlags & ENV_LOOP)
		{
			UINT volloopend = pIns->VolEnv.Ticks[pIns->VolEnv.nLoopEnd];
			if (GetType() != MOD_TYPE_XM) volloopend++;
			if (position == volloopend)
			{
				position = pIns->VolEnv.Ticks[pIns->VolEnv.nLoopStart];
				if ((pIns->VolEnv.nLoopEnd == pIns->VolEnv.nLoopStart) && (!pIns->VolEnv.Values[pIns->VolEnv.nLoopStart])
					&& ((!(GetType() & MOD_TYPE_XM)) || (pIns->VolEnv.nLoopEnd+1 == (int)pIns->VolEnv.nNodes)))
				{
					pChn->dwFlags |= CHN_NOTEFADE;
					pChn->nFadeOutVol = 0;
				}
			}
		}
		// Volume Sustain ?
		if ((pIns->VolEnv.dwFlags & ENV_SUSTAIN) && (!(pChn->dwFlags & CHN_KEYOFF)))
		{
			if (position == (UINT)pIns->VolEnv.Ticks[pIns->VolEnv.nSustainEnd] + 1)
				position = pIns->VolEnv.Ticks[pIns->VolEnv.nSustainStart];
		} else
		{
			// End of Envelope ?
			if (position > pIns->VolEnv.Ticks[pIns->VolEnv.nNodes - 1])
			{
				if ((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || (pChn->dwFlags & CHN_KEYOFF)) pChn->dwFlags |= CHN_NOTEFADE;
				position = pIns->VolEnv.Ticks[pIns->VolEnv.nNodes - 1];
				if ((!pIns->VolEnv.Values[pIns->VolEnv.nNodes - 1]) && ((pChn->nMasterChn > 0) || (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))))
				{
					pChn->dwFlags |= CHN_NOTEFADE;
					pChn->nFadeOutVol = 0;
					pChn->nRealVolume = 0;
					pChn->nCalcVolume = 0;
				}
			}
		}

		pChn->VolEnv.nEnvPosition = position + (IsCompatibleMode(TRK_IMPULSETRACKER) ? 1 : 0);
	}
}


void CSoundFile::IncrementPanningEnvelopePosition(ModChannel *pChn)
//-----------------------------------------------------------------
{
	const ModInstrument *pIns = pChn->pModInstrument;

	if (pChn->PanEnv.flags & ENV_ENABLED)
	{
		// Increase position
		UINT position = pChn->PanEnv.nEnvPosition + (IsCompatibleMode(TRK_IMPULSETRACKER) ? 0 : 1);

		if (pIns->PanEnv.dwFlags & ENV_LOOP)
		{
			UINT panloopend = pIns->PanEnv.Ticks[pIns->PanEnv.nLoopEnd];
			if (GetType() != MOD_TYPE_XM) panloopend++;
			if (position == panloopend)
				position = pIns->PanEnv.Ticks[pIns->PanEnv.nLoopStart];
		}

		// Panning Sustain ?
		if ((pIns->PanEnv.dwFlags & ENV_SUSTAIN) && (position == (UINT)pIns->PanEnv.Ticks[pIns->PanEnv.nSustainEnd] + 1)
			&& (!(pChn->dwFlags & CHN_KEYOFF)))
		{
			// Panning sustained
			position = pIns->PanEnv.Ticks[pIns->PanEnv.nSustainStart];
		} else
		{
			if (position > pIns->PanEnv.Ticks[pIns->PanEnv.nNodes - 1])
				position = pIns->PanEnv.Ticks[pIns->PanEnv.nNodes - 1];
		}

		pChn->PanEnv.nEnvPosition = position + (IsCompatibleMode(TRK_IMPULSETRACKER) ? 1 : 0);
	}
}


void CSoundFile::IncrementPitchFilterEnvelopePosition(ModChannel *pChn)
//---------------------------------------------------------------------
{
	const ModInstrument *pIns = pChn->pModInstrument;

	if (pChn->PitchEnv.flags & ENV_ENABLED)
	{
		// Increase position
		UINT position = pChn->PitchEnv.nEnvPosition + (IsCompatibleMode(TRK_IMPULSETRACKER) ? 0 : 1);

		// Pitch Loop ?
		if (pIns->PitchEnv.dwFlags & ENV_LOOP)
		{
			UINT pitchloopend = pIns->PitchEnv.Ticks[pIns->PitchEnv.nLoopEnd];
			if (GetType() != MOD_TYPE_XM) pitchloopend++;
			if (position >= pitchloopend)
				position = pIns->PitchEnv.Ticks[pIns->PitchEnv.nLoopStart];
		}
		// Pitch Sustain ?
		if ((pIns->PitchEnv.dwFlags & ENV_SUSTAIN) && (!(pChn->dwFlags & CHN_KEYOFF)))
		{
			if (position == (UINT)pIns->PitchEnv.Ticks[pIns->PitchEnv.nSustainEnd]+1)
				position = pIns->PitchEnv.Ticks[pIns->PitchEnv.nSustainStart];
		} else
		{
			if (position > pIns->PitchEnv.Ticks[pIns->PitchEnv.nNodes - 1])
				position = pIns->PitchEnv.Ticks[pIns->PitchEnv.nNodes - 1];
		}

		pChn->PitchEnv.nEnvPosition = position + (IsCompatibleMode(TRK_IMPULSETRACKER) ? 1 : 0);
	}
}


void CSoundFile::ProcessInstrumentFade(ModChannel *pChn, int &vol)
//----------------------------------------------------------------
{
	// FadeOut volume
	if (pChn->dwFlags & CHN_NOTEFADE)
	{
		const ModInstrument *pIns = pChn->pModInstrument;

		UINT fadeout = pIns->nFadeOut;
		if (fadeout)
		{
			pChn->nFadeOutVol -= fadeout << 1;
			if (pChn->nFadeOutVol <= 0) pChn->nFadeOutVol = 0;
			vol = (vol * pChn->nFadeOutVol) >> 16;
		} else if (!pChn->nFadeOutVol)
		{
			vol = 0;
		}
	}
}


void CSoundFile::ProcessPitchPanSeparation(ModChannel *pChn)
//----------------------------------------------------------
{
	const ModInstrument *pIns = pChn->pModInstrument;

	if ((pIns->nPPS) && (pChn->nRealPan) && (pChn->nNote))
	{
		// PPS value is 1/512, i.e. PPS=1 will adjust by 8/512 = 1/64 for each 8 semitones
		// with PPS = 32 / PPC = C-5, E-6 will pan hard right (and D#6 will not)
		int pandelta = (int)pChn->nRealPan + (int)((int)(pChn->nNote - pIns->nPPC - 1) * (int)pIns->nPPS) / 4;
		pChn->nRealPan = Clamp(pandelta, 0, 256);
	}
}


void CSoundFile::ProcessPanbrello(ModChannel *pChn)
//-------------------------------------------------
{
	if (pChn->dwFlags & CHN_PANBRELLO)
	{
		UINT panpos;
		// IT compatibility: IT has its own, more precise tables
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
			panpos = pChn->nPanbrelloPos & 0xFF;
		else
			panpos = ((pChn->nPanbrelloPos + 0x10) >> 2) & 0x3F;

		LONG pdelta = GetVibratoDelta(pChn->nPanbrelloType, panpos);

		pChn->nPanbrelloPos += pChn->nPanbrelloSpeed;
		pdelta = ((pdelta * (int)pChn->nPanbrelloDepth) + 2) >> 3;
		pdelta += pChn->nRealPan;

		pChn->nRealPan = Clamp(pdelta, 0, 256);
		//if(IsCompatibleMode(TRK_IMPULSETRACKER)) pChn->nPan = pChn->nRealPan; // TODO
	}
}


void CSoundFile::ProcessArpeggio(ModChannel *pChn, int &period, CTuning::NOTEINDEXTYPE &arpeggioSteps)
//----------------------------------------------------------------------------------------------------
{
	if (pChn->nCommand == CMD_ARPEGGIO)
	{
#ifndef NO_VST
#if 0
		// EXPERIMENTAL VSTi arpeggio. Far from perfect!
		if(pChn->pModInstrument && pChn->pModInstrument->nMixPlug && !(m_dwSongFlags & SONG_FIRSTTICK))
		{
			const ModInstrument *pIns = pChn->pModInstrument;
			IMixPlugin *pPlugin =  m_MixPlugins[pIns->nMixPlug - 1].pMixPlugin;
			if(pPlugin)
			{
				// Temporary logic: This ensures that the first and last tick are both playing the base note.
				int nCount = (int)m_nTickCount - (int)(m_nMusicSpeed * (m_nPatternDelay + 1) + m_nFrameDelay - 1);
				int nStep = 0, nLastStep = 0;
				nCount = -nCount;
				switch(nCount % 3)
				{
				case 0:
					nStep = 0;
					nLastStep = pChn->nArpeggio & 0x0F;
					break;
				case 1: 
					nStep = pChn->nArpeggio >> 4;
					nLastStep = 0;
					break;
				case 2:
					nStep = pChn->nArpeggio & 0x0F;
					nLastStep = pChn->nArpeggio >> 4;
					break;
				}
				// First tick is always 0
				if(m_nTickCount == 1)
					nLastStep = 0;

				pPlugin->MidiCommand(pIns->nMidiChannel, pIns->nMidiProgram, pIns->wMidiBank, pChn->nNote + nStep, pChn->nVolume, nChn);
				pPlugin->MidiCommand(pIns->nMidiChannel, pIns->nMidiProgram, pIns->wMidiBank, pChn->nNote + nLastStep + NOTE_KEYOFF, 0, nChn);
			}
		}
#endif // 0
#endif // NO_VST

		if((m_nType & MOD_TYPE_MPT) && pChn->pModInstrument && pChn->pModInstrument->pTuning)
		{
			switch(m_nTickCount % 3)
			{
			case 0:
				arpeggioSteps = 0;
				break;
			case 1: 
				arpeggioSteps = pChn->nArpeggio >> 4; // >> 4 <-> division by 16. This gives the first number in the parameter.
				break;
			case 2:
				arpeggioSteps = pChn->nArpeggio & 0x0F; //Gives the latter number in the parameter.
				break;
			}
			pChn->m_CalculateFreq = true;
			pChn->m_ReCalculateFreqOnFirstTick = true;
		}
		else
		{
			//IT playback compatibility 01 & 02
			if(IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				if(pChn->nArpeggio >> 4 != 0 || (pChn->nArpeggio & 0x0F) != 0)
				{
					switch(m_nTickCount % 3)
					{
					case 1:	period = Util::Round<int>(period / TwoToPowerXOver12(pChn->nArpeggio >> 4)); break;
					case 2:	period = Util::Round<int>(period / TwoToPowerXOver12(pChn->nArpeggio & 0x0F)); break;
					}
				}
			}
			// FastTracker 2: Swedish tracker logic (TM) arpeggio
			else if(IsCompatibleMode(TRK_FASTTRACKER2))
			{
				BYTE note = pChn->nNote;
				int arpPos = 0;

				if (!(m_dwSongFlags & SONG_FIRSTTICK))
				{
					arpPos = ((int)m_nMusicSpeed - (int)m_nTickCount) % 3;
					if((m_nMusicSpeed > 18) && (m_nMusicSpeed - m_nTickCount > 16)) arpPos = 2; // swedish tracker logic, I love it
					switch(arpPos)
					{
					case 1:	note += (pChn->nArpeggio >> 4); break;
					case 2:	note += (pChn->nArpeggio & 0x0F); break;
					}
				}

				if (note > 109 && arpPos != 0)
					note = 109; // FT2's note limit

				period = GetPeriodFromNote(note, pChn->nFineTune, pChn->nC5Speed);

			}
			// Other trackers
			else
			{
				switch(m_nTickCount % 3)
				{
				case 1:	period = GetPeriodFromNote(pChn->nNote + (pChn->nArpeggio >> 4), pChn->nFineTune, pChn->nC5Speed); break;
				case 2:	period = GetPeriodFromNote(pChn->nNote + (pChn->nArpeggio & 0x0F), pChn->nFineTune, pChn->nC5Speed); break;
				}
			}
		}
	}
}


void CSoundFile::ProcessVibrato(ModChannel *pChn, int &period, CTuning::RATIOTYPE &vibratoFactor)
//-----------------------------------------------------------------------------------------------
{
	if (pChn->dwFlags & CHN_VIBRATO)
	{
		UINT vibpos = pChn->nVibratoPos;

		LONG vdelta = GetVibratoDelta(pChn->nVibratoType, vibpos);

		if(GetType() == MOD_TYPE_MPT && pChn->pModInstrument && pChn->pModInstrument->pTuning)
		{
			//Hack implementation: Scaling vibratofactor to [0.95; 1.05]
			//using figure from above tables and vibratodepth parameter
			vibratoFactor += 0.05F * vdelta * pChn->m_VibratoDepth / 128.0F;
			pChn->m_CalculateFreq = true;
			pChn->m_ReCalculateFreqOnFirstTick = false;

			if(m_nTickCount + 1 == m_nMusicSpeed)
				pChn->m_ReCalculateFreqOnFirstTick = true;
		}
		else
		{
			// Original behaviour

			UINT vdepth;
			// IT compatibility: correct vibrato depth
			if(IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				// Yes, vibrato goes backwards with old effects enabled!
				if(m_dwSongFlags & SONG_ITOLDEFFECTS)
				{
					// Test case: vibrato-oldfx.it
					vdepth = 5;
				} else
				{
					// Test case: vibrato.it
					vdepth = 6;
					vdelta = -vdelta;
				}
			}
			else
			{
				vdepth = ((!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) || (m_dwSongFlags & SONG_ITOLDEFFECTS)) ? 6 : 7;
			}
			vdelta = (vdelta * (int)pChn->nVibratoDepth) >> vdepth;
			if ((m_dwSongFlags & SONG_LINEARSLIDES) && (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)))
			{
				LONG l = vdelta;
				if (l < 0)
				{
					l = -l;
					vdelta = _muldiv(period, LinearSlideDownTable[l >> 2], 0x10000) - period;
					if (l & 0x03) vdelta += _muldiv(period, FineLinearSlideDownTable[l & 0x03], 0x10000) - period;
				} else
				{
					vdelta = _muldiv(period, LinearSlideUpTable[l >> 2], 0x10000) - period;
					if (l & 0x03) vdelta += _muldiv(period, FineLinearSlideUpTable[l & 0x03], 0x10000) - period;
				}
			}
			period += vdelta;
		}
		if ((m_nTickCount) || ((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (!(m_dwSongFlags & SONG_ITOLDEFFECTS))))
		{
			// IT compatibility: IT has its own, more precise tables
			if(IsCompatibleMode(TRK_IMPULSETRACKER))
				pChn->nVibratoPos = (vibpos + 4 * pChn->nVibratoSpeed) & 0xFF;
			else
				pChn->nVibratoPos = (vibpos + pChn->nVibratoSpeed) & 0x3F;
		}
	}
}


void CSoundFile::ProcessSampleAutoVibrato(ModChannel *pChn, int &period, CTuning::RATIOTYPE &vibratoFactor, int &nPeriodFrac)
//---------------------------------------------------------------------------------------------------------------------------
{
	// Sample Auto-Vibrato
	if ((pChn->pModSample) && (pChn->pModSample->nVibDepth))
	{
		const ModSample *pSmp = pChn->pModSample;
		const bool alternativeTuning = pChn->pModInstrument && pChn->pModInstrument->pTuning;

		// IT compatibility: Autovibrato is so much different in IT that I just put this in a separate code block, to get rid of a dozen IsCompatibilityMode() calls.
		if(IsCompatibleMode(TRK_IMPULSETRACKER) && !alternativeTuning)
		{
			// Schism's autovibrato code

			/*
			X86 Assembler from ITTECH.TXT:
			1) Mov AX, [SomeVariableNameRelatingToVibrato]
			2) Add AL, Rate
			3) AdC AH, 0
			4) AH contains the depth of the vibrato as a fine-linear slide.
			5) Mov [SomeVariableNameRelatingToVibrato], AX  ; For the next cycle.
			*/
			const int vibpos = pChn->nAutoVibPos & 0xFF;
			int adepth = pChn->nAutoVibDepth; // (1)
			adepth += pSmp->nVibSweep; // (2 & 3)
			adepth = min(adepth, (int)(pSmp->nVibDepth << 8));
			pChn->nAutoVibDepth = adepth; // (5)
			adepth >>= 8; // (4)

			pChn->nAutoVibPos += pSmp->nVibRate;

			int vdelta;
			switch(pSmp->nVibType)
			{
			case VIB_RANDOM:
				vdelta = (rand() & 0x7F) - 0x40;
				break;
			case VIB_RAMP_DOWN:
				vdelta = ITRampDownTable[vibpos];
				break;
			case VIB_RAMP_UP:
				vdelta = -ITRampDownTable[vibpos];
				break;
			case VIB_SQUARE:
				vdelta = ITSquareTable[vibpos];
				break;
			case VIB_SINE:
			default:
				vdelta = ITSinusTable[vibpos];
				break;
			}

			vdelta = (vdelta * adepth) >> 6;
			int l = abs(vdelta);
			if(vdelta < 0)
			{
				vdelta = _muldiv(period, LinearSlideDownTable[l >> 2], 0x10000) - period;
				if (l & 0x03)
				{
					vdelta += _muldiv(period, FineLinearSlideDownTable[l & 0x03], 0x10000) - period;
				}
			} else
			{
				vdelta = _muldiv(period, LinearSlideUpTable[l >> 2], 0x10000) - period;
				if (l & 0x03)
				{
					vdelta += _muldiv(period, FineLinearSlideUpTable[l & 0x03], 0x10000) - period;
				}
			}
			period -= vdelta;

		} else
		{
			// MPT's autovibrato code
			if (pSmp->nVibSweep == 0 && !(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)))
			{
				pChn->nAutoVibDepth = pSmp->nVibDepth << 8;
			} else
			{
				// Calculate current autovibrato depth using vibsweep
				if (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
				{
					// Note: changed bitshift from 3 to 1 as the variable is not divided by 4 in the IT loader anymore
					// - so we divide sweep by 4 here.
					pChn->nAutoVibDepth += pSmp->nVibSweep << 1;
				} else
				{
					if (!(pChn->dwFlags & CHN_KEYOFF))
					{
						pChn->nAutoVibDepth += (pSmp->nVibDepth << 8) /	pSmp->nVibSweep;
					}
				}
				if ((pChn->nAutoVibDepth >> 8) > pSmp->nVibDepth)
					pChn->nAutoVibDepth = pSmp->nVibDepth << 8;
			}
			pChn->nAutoVibPos += pSmp->nVibRate;
			int vdelta;
			switch(pSmp->nVibType)
			{
			case VIB_RANDOM:
				vdelta = ModRandomTable[pChn->nAutoVibPos & 0x3F];
				pChn->nAutoVibPos++;
				break;
			case VIB_RAMP_DOWN:
				vdelta = ((0x40 - (pChn->nAutoVibPos >> 1)) & 0x7F) - 0x40;
				break;
			case VIB_RAMP_UP:
				vdelta = ((0x40 + (pChn->nAutoVibPos >> 1)) & 0x7F) - 0x40;
				break;
			case VIB_SQUARE:
				vdelta = (pChn->nAutoVibPos & 128) ? +64 : -64;
				break;
			case VIB_SINE:
			default:
				vdelta = ft2VibratoTable[pChn->nAutoVibPos & 0xFF];
			}
			int n;
			n =	((vdelta * pChn->nAutoVibDepth) >> 8);

			if(alternativeTuning)
			{
				//Vib sweep is not taken into account here.
				vibratoFactor += 0.05F * pSmp->nVibDepth * vdelta / 4096.0f; //4096 == 64^2
				//See vibrato for explanation.
				pChn->m_CalculateFreq = true;
				/*
				Finestep vibrato:
				const float autoVibDepth = pSmp->nVibDepth * val / 4096.0f; //4096 == 64^2
				vibratoFineSteps += static_cast<CTuning::FINESTEPTYPE>(pChn->pModInstrument->pTuning->GetFineStepCount() *  autoVibDepth);
				pChn->m_CalculateFreq = true;
				*/
			}
			else //Original behavior
			{
				if (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
				{
					int df1, df2;
					if (n < 0)
					{
						n = -n;
						UINT n1 = n >> 8;
						df1 = LinearSlideUpTable[n1];
						df2 = LinearSlideUpTable[n1+1];
					} else
					{
						UINT n1 = n >> 8;
						df1 = LinearSlideDownTable[n1];
						df2 = LinearSlideDownTable[n1+1];
					}
					n >>= 2;
					period = _muldiv(period, df1 + ((df2 - df1) * (n & 0x3F) >> 6), 256);
					nPeriodFrac = period & 0xFF;
					period >>= 8;
				} else
				{
					period += (n >> 6);
				}
			} //Original MPT behavior
		}
	}
}


void CSoundFile::ProcessRamping(ModChannel *pChn)
//-----------------------------------------------
{
	pChn->nRightRamp = pChn->nLeftRamp = 0;
	if((pChn->dwFlags & CHN_VOLUMERAMP) && ((pChn->nRightVol != pChn->nNewRightVol) || (pChn->nLeftVol != pChn->nNewLeftVol)))
	{
		const bool rampUp = (pChn->nNewRightVol > pChn->nRightVol) || (pChn->nNewLeftVol > pChn->nLeftVol);
		LONG rampLength, globalRampLength, instrRampLength = 0;
		rampLength = globalRampLength = (rampUp ? gnVolumeRampUpSamples : gnVolumeRampDownSamples);
		//XXXih: add real support for bidi ramping here	
		
		if(pChn->pModInstrument != nullptr && rampUp)
		{
			instrRampLength = pChn->pModInstrument->nVolRampUp;
			rampLength = instrRampLength ? (gdwMixingFreq * instrRampLength / 100000) : globalRampLength;
		}
		const bool enableCustomRamp = (instrRampLength > 0);

		if(!rampLength)
		{
			rampLength = 1;
		}

		LONG nRightDelta = ((pChn->nNewRightVol - pChn->nRightVol) << VOLUMERAMPPRECISION);
		LONG nLeftDelta = ((pChn->nNewLeftVol - pChn->nLeftVol) << VOLUMERAMPPRECISION);
#ifndef FASTSOUNDLIB
//		if ((gdwSoundSetup & SNDMIX_DIRECTTODISK)
//			|| ((gdwSysInfo & (SYSMIX_ENABLEMMX|SYSMIX_FASTCPU))
//			&& (gdwSoundSetup & SNDMIX_HQRESAMPLER) && (gnCPUUsage <= 50)))
		if((gdwSoundSetup & SNDMIX_DIRECTTODISK)
			|| ((gdwSysInfo & (SYSMIX_ENABLEMMX | SYSMIX_FASTCPU)) && (gdwSoundSetup & SNDMIX_HQRESAMPLER) && (gnCPUUsage <= 50) && !enableCustomRamp))
		{
			if((pChn->nRightVol | pChn->nLeftVol) && (pChn->nNewRightVol | pChn->nNewLeftVol) && (!(pChn->dwFlags & CHN_FASTVOLRAMP)))
			{
				rampLength = m_nBufferCount;
				if(rampLength > (1 << (VOLUMERAMPPRECISION-1)))
				{
					rampLength = (1 << (VOLUMERAMPPRECISION-1));
				}
				if(rampLength < globalRampLength)
				{
					rampLength = globalRampLength;
				}
			}
		}
#endif // FASTSOUNDLIB

		pChn->nRightRamp = nRightDelta / rampLength;
		pChn->nLeftRamp = nLeftDelta / rampLength;
		pChn->nRightVol = pChn->nNewRightVol - ((pChn->nRightRamp * rampLength) >> VOLUMERAMPPRECISION);
		pChn->nLeftVol = pChn->nNewLeftVol - ((pChn->nLeftRamp * rampLength) >> VOLUMERAMPPRECISION);

		if (pChn->nRightRamp|pChn->nLeftRamp)
		{
			pChn->nRampLength = rampLength;
		} else
		{
			pChn->dwFlags &= ~CHN_VOLUMERAMP;
			pChn->nRightVol = pChn->nNewRightVol;
			pChn->nLeftVol = pChn->nNewLeftVol;
		}
	} else
	{
		pChn->dwFlags &= ~CHN_VOLUMERAMP;
		pChn->nRightVol = pChn->nNewRightVol;
		pChn->nLeftVol = pChn->nNewLeftVol;
	}
	pChn->nRampRightVol = pChn->nRightVol << VOLUMERAMPPRECISION;
	pChn->nRampLeftVol = pChn->nLeftVol << VOLUMERAMPPRECISION;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Handles envelopes & mixer setup

BOOL CSoundFile::ReadNote()
//-------------------------
{
#ifdef MODPLUG_TRACKER
	// Checking end of row ?
	if (m_dwSongFlags & SONG_PAUSED)
	{
		m_nTickCount = 0;
		if (!m_nMusicSpeed) m_nMusicSpeed = 6;
		if (!m_nMusicTempo) m_nMusicTempo = 125;
	} else
#endif // MODPLUG_TRACKER
	{
		if (!ProcessRow()) 
			return FALSE;
	}
	////////////////////////////////////////////////////////////////////////////////////
	m_nTotalCount++;
	if (!m_nMusicTempo) return FALSE;

	switch(m_nTempoMode)
	{

		case tempo_mode_alternative: 
			m_nBufferCount = gdwMixingFreq / m_nMusicTempo;
			break;

		case tempo_mode_modern:
			{
				double accurateBufferCount = (double)gdwMixingFreq * (60.0 / (double)m_nMusicTempo / ((double)m_nMusicSpeed * (double)m_nCurrentRowsPerBeat));
				m_nBufferCount = static_cast<int>(accurateBufferCount);
				m_dBufferDiff += accurateBufferCount-m_nBufferCount;
				//tick-to-tick tempo correction:
				if (m_dBufferDiff >= 1)
				{ 
					m_nBufferCount++;
					m_dBufferDiff--;
				} else if (m_dBufferDiff <= -1)
				{ 
					m_nBufferCount--;
					m_dBufferDiff++;
				}
				ASSERT(abs(m_dBufferDiff) < 1);
				break;
			}

		case tempo_mode_classic:
		default:
			m_nBufferCount = (gdwMixingFreq * 5 * m_nTempoFactor) / (m_nMusicTempo << 8);
	}
	
	m_nSamplesPerTick = m_nBufferCount; //rewbs.flu


	// Master Volume + Pre-Amplification / Attenuation setup
	DWORD nMasterVol;
	{
		int nchn32 = CLAMP(m_nChannels, 1, 31);
		
		DWORD mastervol;

		if (m_pConfig->getUseGlobalPreAmp())
		{
			int realmastervol = m_nMasterVolume;
			if (realmastervol > 0x80)
			{
				//Attenuate global pre-amp depending on num channels
				realmastervol = 0x80 + ((realmastervol - 0x80) * (nchn32+4)) / 16;
			}
			mastervol = (realmastervol * (m_nSamplePreAmp)) >> 6;
		} else
		{
			//Preferred option: don't use global pre-amp at all.
			mastervol = m_nSamplePreAmp;
		}

		if ((m_dwSongFlags & SONG_GLOBALFADE) && (m_nGlobalFadeMaxSamples))
		{
			mastervol = _muldiv(mastervol, m_nGlobalFadeSamples, m_nGlobalFadeMaxSamples);
		}

		if (m_pConfig->getUseGlobalPreAmp())
		{
			UINT attenuation = (gdwSoundSetup & SNDMIX_AGC) ? PreAmpAGCTable[nchn32>>1] : PreAmpTable[nchn32>>1];
			if(attenuation < 1) attenuation = 1;
			nMasterVol = (mastervol << 7) / attenuation;
		} else
		{
			nMasterVol = mastervol;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Update channels data
	m_nMixChannels = 0;
	ModChannel *pChn = Chn;
	for (CHANNELINDEX nChn = 0; nChn < MAX_CHANNELS; nChn++, pChn++)
	{
		// XM Compatibility: Prevent notes to be stopped after a fadeout. This way, a portamento effect can pick up a faded instrument which is long enough.
		// This occours for example in the bassline (channel 11) of jt_burn.xm. I hope this won't break anything else...
		// I also suppose this could decrease mixing performance a bit, but hey, which CPU can't handle 32 muted channels these days... :-)
		if ((pChn->dwFlags & CHN_NOTEFADE) && (!(pChn->nFadeOutVol|pChn->nRightVol|pChn->nLeftVol)) && (!IsCompatibleMode(TRK_FASTTRACKER2)))
		{
			pChn->nLength = 0;
			pChn->nROfs = pChn->nLOfs = 0;
		}
		// Check for unused channel
		if ((pChn->dwFlags & CHN_MUTE) || ((nChn >= m_nChannels) && (!pChn->nLength)))
		{
			if(nChn < m_nChannels)
			{
				// Process MIDI macros on channels that are currently muted.
				ProcessMacroOnChannel(nChn);
			}

			pChn->nVUMeter = 0;
#ifdef ENABLE_STEREOVU
			pChn->nLeftVU = pChn->nRightVU = 0;
#endif

			continue;
		}
		// Reset channel data
		pChn->nInc = 0;
		pChn->nRealVolume = 0;
		pChn->nCalcVolume = 0;

		pChn->nRampLength = 0;

		//Aux variables
		CTuning::RATIOTYPE vibratoFactor = 1;
		CTuning::NOTEINDEXTYPE arpeggioSteps = 0;

		ModInstrument *pIns = pChn->pModInstrument;

		// Calc Frequency
		int period;

		// Also process envelopes etc. when there's a plugin on this channel, for possible fake automation using volume and pan data.
		// We only care about master channels, though, since automation only "happens" on them.
		const bool samplePlaying = (pChn->nPeriod && pChn->nLength);
		const bool plugAssigned = (nChn < m_nChannels) && (ChnSettings[nChn].nMixPlugin || (pChn->pModInstrument != nullptr && pChn->pModInstrument->nMixPlug));
		if (samplePlaying || plugAssigned)
		{
			int vol = pChn->nVolume;

			if(!IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				ProcessVolumeSwing(pChn, vol);
			}
			ProcessPanningSwing(pChn);
			ProcessTremolo(pChn, vol);
			ProcessTremor(pChn, vol);

			// Clip volume and multiply (extend to 14 bits)
			Limit(vol, 0, 256);
			vol <<= 6;

			// Process Envelopes
			if (pIns)
			{
				if(IsCompatibleMode(TRK_IMPULSETRACKER))
				{
					// In IT compatible mode, envelope position indices are shifted by one for proper envelope pausing,
					// so we have to update the position before we actually process the envelopes.
					// When using MPT behaviour, we get the envelope position for the next tick while we are still calculating the current tick,
					// which then results in wrong position information when the envelope is paused on the next row.
					// Test case: s77.it
					IncrementVolumeEnvelopePosition(pChn);
					IncrementPanningEnvelopePosition(pChn);
					IncrementPitchFilterEnvelopePosition(pChn);
				}
				ProcessVolumeEnvelope(pChn, vol);
				ProcessInstrumentFade(pChn, vol);
				ProcessPanningEnvelope(pChn);
				ProcessPitchPanSeparation(pChn);
			} else
			{
				// No Envelope: key off => note cut
				if (pChn->dwFlags & CHN_NOTEFADE) // 1.41-: CHN_KEYOFF|CHN_NOTEFADE
				{
					pChn->nFadeOutVol = 0;
					vol = 0;
				}
			}
			// vol is 14-bits
			if (vol)
			{
				int insVol = pChn->nInsVol;	// This is the "SV * IV" value in ITTECH.TXT
				if(IsCompatibleMode(TRK_IMPULSETRACKER))
				{
					ProcessVolumeSwing(pChn, insVol);
				}

				// IMPORTANT: pChn->nRealVolume is 14 bits !!!
				// -> _muldiv( 14+8, 6+6, 18); => RealVolume: 14-bit result (22+12-20)
				
				// Don't let global volume affect level of sample if
				// Global volume is going to be applied to master output anyway.
				if (pChn->dwFlags & CHN_SYNCMUTE)
				{
					pChn->nRealVolume = 0;
				} else if (m_pConfig->getGlobalVolumeAppliesToMaster())
				{
					pChn->nRealVolume = _muldiv(vol * MAX_GLOBAL_VOLUME, pChn->nGlobalVol * insVol, 1 << 20);
				} else
				{
					pChn->nRealVolume = _muldiv(vol * m_nGlobalVolume, pChn->nGlobalVol * insVol, 1 << 20);
				}
			}

			pChn->nCalcVolume = vol;	// Update calculated volume for MIDI macros

			if (pChn->nPeriod < m_nMinPeriod) pChn->nPeriod = m_nMinPeriod;
			period = pChn->nPeriod;
			// TODO Glissando effect is reset after portamento! What would this sound like without the CHN_PORTAMENTO flag?
			if ((pChn->dwFlags & (CHN_GLISSANDO|CHN_PORTAMENTO)) ==	(CHN_GLISSANDO|CHN_PORTAMENTO))
			{
				period = GetPeriodFromNote(GetNoteFromPeriod(period), pChn->nFineTune, pChn->nC5Speed);
			}

			ProcessArpeggio(pChn, period, arpeggioSteps);

			// Preserve Amiga freq limits
			if (m_dwSongFlags & (SONG_AMIGALIMITS|SONG_PT1XMODE))
			{
				Limit(period, 113 * 4, 856 * 4);
			}

			ProcessPanbrello(pChn);

		}

		// IT Compatibility: Ensure that there is no pan swing, panbrello, panning envelopes, etc. applied on surround channels.
		// Test case: surround-pan.it
		if((pChn->dwFlags & CHN_SURROUND) && !(m_dwSongFlags & SONG_SURROUNDPAN) && IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			pChn->nRealPan = 128;
		}

		// Now that all relevant envelopes etc. have been processed, we can parse the MIDI macro data.
		ProcessMacroOnChannel(nChn);

		// After MIDI macros have been processed, we can also process the pitch / filter envelope and other pitch-related things.
		if (samplePlaying)
		{
			int nPeriodFrac = 0;

			ProcessPitchFilterEnvelope(pChn, period);
			ProcessVibrato(pChn, period, vibratoFactor);
			ProcessSampleAutoVibrato(pChn, period, vibratoFactor, nPeriodFrac);

			// Final Period
			if (period <= m_nMinPeriod)
			{
				if (GetType() & MOD_TYPE_S3M) pChn->nLength = 0;
				period = m_nMinPeriod;
			}
			//rewbs: temporarily commenting out block to allow notes below A-0.
			/*if (period > m_nMaxPeriod)
			{
				if ((m_nType & MOD_TYPE_IT) || (period >= 0x100000))
				{
					pChn->nFadeOutVol = 0;
					pChn->dwFlags |= CHN_NOTEFADE;
					pChn->nRealVolume = 0;
				}
				period = m_nMaxPeriod;
				nPeriodFrac = 0;
			}*/
			UINT freq = 0;

			if(GetType() != MOD_TYPE_MPT || pIns == nullptr || pIns->pTuning == nullptr)
			{
				freq = GetFreqFromPeriod(period, pChn->nC5Speed, nPeriodFrac);
			} else
			{
				// In this case: GetType() == MOD_TYPE_MPT and using custom tunings.
				if(pChn->m_CalculateFreq || (pChn->m_ReCalculateFreqOnFirstTick && m_nTickCount == 0))
				{
					pChn->m_Freq = Util::Round<UINT>(pChn->nC5Speed * vibratoFactor * pIns->pTuning->GetRatio(pChn->nNote - NOTE_MIDDLEC + arpeggioSteps, pChn->nFineTune+pChn->m_PortamentoFineSteps));
					if(!pChn->m_CalculateFreq)
						pChn->m_ReCalculateFreqOnFirstTick = false;
					else
						pChn->m_CalculateFreq = false;
				}

				freq = pChn->m_Freq;
			}

			// Applying Pitch/Tempo lock.
            if(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT) && pIns && pIns->wPitchToTempoLock)
			{
				freq = _muldivr(freq, m_nMusicTempo, pIns->wPitchToTempoLock);
			}

			if ((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (freq < 256))
			{
				pChn->nFadeOutVol = 0;
				pChn->dwFlags |= CHN_NOTEFADE;
				pChn->nRealVolume = 0;
				pChn->nCalcVolume = 0;
			}

			UINT ninc = _muldiv(freq, 0x10000, gdwMixingFreq);
			if ((ninc >= 0xFFB0) && (ninc <= 0x10090)) ninc = 0x10000;
			if (m_nFreqFactor != 128) ninc = (ninc * m_nFreqFactor) >> 7;
			if (ninc > 0xFF0000) ninc = 0xFF0000;
			pChn->nInc = (ninc + 1) & ~3;
		} else
		{
			// Avoid nasty noises...
			// This could have been != 0 if a plugin was assigned to the channel, for macro purposes.
			pChn->nRealVolume = 0;
		}

		// Increment envelope positions
		if (pIns != nullptr && !IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			// In IT compatible mode, envelope positions are updated above.
			// Test case: s77.it
			IncrementVolumeEnvelopePosition(pChn);
			IncrementPanningEnvelopePosition(pChn);
			IncrementPitchFilterEnvelopePosition(pChn);
		}

#ifdef MODPLUG_PLAYER
		// Limit CPU -> > 80% -> don't ramp
		if ((gnCPUUsage >= 80) && (!pChn->nRealVolume))
		{
			pChn->nLeftVol = pChn->nRightVol = 0;
		}
#endif // MODPLUG_PLAYER

		// Volume ramping
		pChn->dwFlags &= ~CHN_VOLUMERAMP;
		if ((pChn->nRealVolume) || (pChn->nLeftVol) || (pChn->nRightVol))
			pChn->dwFlags |= CHN_VOLUMERAMP;

#ifdef MODPLUG_PLAYER
		// Decrease VU-Meter
		if (pChn->nVUMeter > VUMETER_DECAY)	pChn->nVUMeter -= VUMETER_DECAY; else pChn->nVUMeter = 0;
#endif // MODPLUG_PLAYER

#ifdef ENABLE_STEREOVU
		if (pChn->nLeftVU > VUMETER_DECAY) pChn->nLeftVU -= VUMETER_DECAY; else pChn->nLeftVU = 0;
		if (pChn->nRightVU > VUMETER_DECAY) pChn->nRightVU -= VUMETER_DECAY; else pChn->nRightVU = 0;
#endif

		// Check for too big nInc
		if (((pChn->nInc >> 16) + 1) >= (LONG)(pChn->nLoopEnd - pChn->nLoopStart)) pChn->dwFlags &= ~CHN_LOOP;
		pChn->nNewRightVol = pChn->nNewLeftVol = 0;
		pChn->pCurrentSample = ((pChn->pSample) && (pChn->nLength) && (pChn->nInc)) ? pChn->pSample : NULL;
		if (pChn->pCurrentSample)
		{
			// Update VU-Meter (nRealVolume is 14-bit)
#ifdef MODPLUG_PLAYER
			UINT vutmp = pChn->nRealVolume >> (14 - 8);
			if (vutmp > 0xFF) vutmp = 0xFF;
			if (pChn->nVUMeter >= 0x100) pChn->nVUMeter = vutmp;
			vutmp >>= 1;
			if (pChn->nVUMeter < vutmp)	pChn->nVUMeter = vutmp;
#endif // MODPLUG_PLAYER

#ifdef ENABLE_STEREOVU
			UINT vul = (pChn->nRealVolume * pChn->nRealPan) >> 14;
			if (vul > 127) vul = 127;
			if (pChn->nLeftVU > 127) pChn->nLeftVU = (BYTE)vul;
			vul >>= 1;
			if (pChn->nLeftVU < vul) pChn->nLeftVU = (BYTE)vul;
			UINT vur = (pChn->nRealVolume * (256-pChn->nRealPan)) >> 14;
			if (vur > 127) vur = 127;
			if (pChn->nRightVU > 127) pChn->nRightVU = (BYTE)vur;
			vur >>= 1;
			if (pChn->nRightVU < vur) pChn->nRightVU = (BYTE)vur;
#endif

#ifdef MODPLUG_TRACKER
			const UINT kChnMasterVol = (pChn->dwFlags & CHN_EXTRALOUD) ? 0x100 : nMasterVol;
#else
#define		kChnMasterVol	nMasterVol
#endif // MODPLUG_TRACKER

			// Adjusting volumes
			if (gnChannels >= 2)
			{
				int pan = ((int)pChn->nRealPan) - 128;
				pan *= (int)m_nStereoSeparation;
				pan /= 128;
				pan += 128;
				Limit(pan, 0, 256);
#ifndef FASTSOUNDLIB
				if (gdwSoundSetup & SNDMIX_REVERSESTEREO) pan = 256 - pan;
#endif

				LONG realvol;
				if (m_pConfig->getUseGlobalPreAmp())
				{
					realvol = (pChn->nRealVolume * kChnMasterVol) >> 7;
				} else
				{
					// Extra attenuation required here if we're bypassing pre-amp.
					realvol = (pChn->nRealVolume * kChnMasterVol) >> 8;
				}
				
				const forcePanningMode panningMode = m_pConfig->getForcePanningMode(); 				
				if (panningMode == forceSoftPanning || (panningMode == dontForcePanningMode && (gdwSoundSetup & SNDMIX_SOFTPANNING)))
				{
					if (pan < 128)
					{
						pChn->nNewLeftVol = (realvol * pan) >> 8;
						pChn->nNewRightVol = (realvol * 128) >> 8;
					} else
					{
						pChn->nNewLeftVol = (realvol * 128) >> 8;
						pChn->nNewRightVol = (realvol * (256 - pan)) >> 8;
					}
				} else
				{
					pChn->nNewLeftVol = (realvol * pan) >> 8;
					pChn->nNewRightVol = (realvol * (256 - pan)) >> 8;
				}

			} else
			{
				pChn->nNewRightVol = (pChn->nRealVolume * kChnMasterVol) >> 8;
				pChn->nNewLeftVol = pChn->nNewRightVol;
			}
			// Clipping volumes
			//if (pChn->nNewRightVol > 0xFFFF) pChn->nNewRightVol = 0xFFFF;
			//if (pChn->nNewLeftVol > 0xFFFF) pChn->nNewLeftVol = 0xFFFF;
			// Check IDO
			if (gdwSoundSetup & SNDMIX_NORESAMPLING)
			{
				pChn->dwFlags |= CHN_NOIDO;
			} else
			{
				pChn->dwFlags &= ~(CHN_NOIDO|CHN_HQSRC);
#ifndef FASTSOUNDLIB
				if (pChn->nInc == 0x10000)
				{
					pChn->dwFlags |= CHN_NOIDO;
				} else
				if ((gdwSoundSetup & SNDMIX_HQRESAMPLER) && ((gnCPUUsage < 80) || (gdwSoundSetup & SNDMIX_DIRECTTODISK) || (m_nMixChannels < 8)))
				{
					if ((!(gdwSoundSetup & SNDMIX_DIRECTTODISK)) && (!(gdwSoundSetup & SNDMIX_ULTRAHQSRCMODE)))
					{
						int fmax = 0x20000;
						if (gdwSysInfo & SYSMIX_SLOWCPU)
						{
							fmax = 0xFE00;
						} else
						if (!(gdwSysInfo & (SYSMIX_ENABLEMMX|SYSMIX_FASTCPU))) 
						{
							fmax = 0x18000;
						}
						if ((pChn->nNewLeftVol < 0x80) && (pChn->nNewRightVol < 0x80)
						 && (pChn->nLeftVol < 0x80) && (pChn->nRightVol < 0x80))
						{
							if (pChn->nInc >= 0xFF00) pChn->dwFlags |= CHN_NOIDO;
						} else
						{
							pChn->dwFlags |= (pChn->nInc >= fmax) ? CHN_NOIDO : CHN_HQSRC;
						}
					} else
					{
						pChn->dwFlags |= CHN_HQSRC;
					}
					
				} else
				{
					if ((pChn->nInc >= 0x14000)
					|| ((pChn->nInc >= 0xFF00) && ((pChn->nInc < 0x10100) || (gdwSysInfo & SYSMIX_SLOWCPU)))
					|| ((gnCPUUsage > 80) && (pChn->nNewLeftVol < 0x100) && (pChn->nNewRightVol < 0x100)))
						pChn->dwFlags |= CHN_NOIDO;
				}
#else
				if (pChn->nInc >= 0xFE00) pChn->dwFlags |= CHN_NOIDO;
#endif // FASTSOUNDLIB
			}

			/*if (m_pConfig->getUseGlobalPreAmp())
			{
				pChn->nNewRightVol >>= MIXING_ATTENUATION;
				pChn->nNewLeftVol >>= MIXING_ATTENUATION;
			}*/
			const int extraAttenuation = m_pConfig->getExtraSampleAttenuation();
			pChn->nNewRightVol >>= extraAttenuation;
			pChn->nNewLeftVol >>= extraAttenuation;

			// Dolby Pro-Logic Surround
			if ((pChn->dwFlags & CHN_SURROUND) && (gnChannels == 2)) pChn->nNewLeftVol = - pChn->nNewLeftVol;
			// Checking Ping-Pong Loops
			if (pChn->dwFlags & CHN_PINGPONGFLAG) pChn->nInc = -pChn->nInc;
			// Setting up volume ramp
			ProcessRamping(pChn);

			// Adding the channel in the channel list
			ChnMix[m_nMixChannels++] = nChn;
		} else
		{
#ifdef ENABLE_STEREOVU
			// Note change but no sample
			if (pChn->nLeftVU > 128) pChn->nLeftVU = 0;
			if (pChn->nRightVU > 128) pChn->nRightVU = 0;
#endif // ENABLE_STEREOVU
			if (pChn->nVUMeter > 0xFF) pChn->nVUMeter = 0;
			pChn->nLeftVol = pChn->nRightVol = 0;
			pChn->nLength = 0;
		}
	}

	// Checking Max Mix Channels reached: ordering by volume
	if ((m_nMixChannels >= m_nMaxMixChannels) && (!(gdwSoundSetup & SNDMIX_DIRECTTODISK)))
	{
		for (UINT i=0; i<m_nMixChannels; i++)
		{
			UINT j=i;
			while ((j+1<m_nMixChannels) && (Chn[ChnMix[j]].nRealVolume < Chn[ChnMix[j+1]].nRealVolume))
			{
				UINT n = ChnMix[j];
				ChnMix[j] = ChnMix[j+1];
				ChnMix[j+1] = n;
				j++;
			}
		}
	}
	if (m_dwSongFlags & SONG_GLOBALFADE)
	{
		if (!m_nGlobalFadeSamples)
		{
			m_dwSongFlags |= SONG_ENDREACHED;
			return FALSE;
		}
		if (m_nGlobalFadeSamples > m_nBufferCount)
			m_nGlobalFadeSamples -= m_nBufferCount;
		else
			m_nGlobalFadeSamples = 0;
	}
	return TRUE;
}


void CSoundFile::ProcessMacroOnChannel(CHANNELINDEX nChn)
//-------------------------------------------------------
{
	ModChannel *pChn = &Chn[nChn];
	if(nChn < m_nChannels)
	{
		// TODO evaluate per-plugin macros here
		//ProcessMIDIMacro(nChn, false, m_MidiCfg.szMidiGlb[MIDIOUT_PAN]);
		//ProcessMIDIMacro(nChn, false, m_MidiCfg.szMidiGlb[MIDIOUT_VOLUME]);

		if(pChn->rowCommand.command == CMD_MIDI || pChn->rowCommand.command == CMD_SMOOTHMIDI)
		{
			// Also non-smooth MIDI Macros are processed on every row to update macros with volume or panning variables.
			if(pChn->rowCommand.param < 0x80)
				ProcessMIDIMacro(nChn, (pChn->rowCommand.command == CMD_SMOOTHMIDI), m_MidiCfg.szMidiSFXExt[pChn->nActiveMacro], pChn->rowCommand.param);
			else
				ProcessMIDIMacro(nChn, (pChn->rowCommand.command == CMD_SMOOTHMIDI), m_MidiCfg.szMidiZXXExt[(pChn->rowCommand.param & 0x7F)], 0);
		}
	}
}


#ifdef MODPLUG_TRACKER

void CSoundFile::ProcessMidiOut(CHANNELINDEX nChn)
//------------------------------------------------
{
	ModChannel *pChn = &Chn[nChn];

	// Do we need to process midi?
	// For now there is no difference between mute and sync mute with VSTis.
	if (pChn->dwFlags & (CHN_MUTE|CHN_SYNCMUTE)) return;
	if ((!m_nInstruments) || (m_nPattern >= Patterns.Size())
		 || (m_nRow >= Patterns[m_nPattern].GetNumRows()) || (!Patterns[m_nPattern])) return;

	const ModCommand::NOTE note = pChn->rowCommand.note;
	const ModCommand::VOL vol = pChn->rowCommand.vol;
	const ModCommand::VOLCMD volcmd = pChn->rowCommand.volcmd;

	// Get instrument info and plugin reference
	ModInstrument *pIns = pChn->pModInstrument;
	PLUGINDEX nPlugin = 0;
	IMixPlugin *pPlugin = nullptr;

	if (pIns)
	{
		// Check instrument plugins
		if (pIns->HasValidMIDIChannel())
		{
			nPlugin = GetBestPlugin(nChn, PrioritiseInstrument, RespectMutes);
			if ((nPlugin) && (nPlugin <= MAX_MIXPLUGINS))
			{
				pPlugin = m_MixPlugins[nPlugin-1].pMixPlugin;
			}
		}

		// Muted instrument?
		if (pIns && (pIns->dwFlags & INS_MUTE)) return;
	}

	// Couldn't find a valid plugin
	if (pPlugin == nullptr) return;

	if(GetModFlag(MSF_MIDICC_BUGEMULATION))
	{
		if(note != NOTE_NONE)
		{
			ModCommand::NOTE realNote = note;
			if(ModCommand::IsNote(note))
				realNote = pIns->NoteMap[note - 1];
			pPlugin->MidiCommand(GetBestMidiChannel(nChn), pIns->nMidiProgram, pIns->wMidiBank, realNote, pChn->nVolume, nChn);
		} else if (volcmd == VOLCMD_VOLUME)
		{
			pPlugin->MidiCC(GetBestMidiChannel(nChn), MIDICC_Volume_Fine, vol, nChn);
		}
		return;
	}


	const bool hasVolCommand = (volcmd == VOLCMD_VOLUME);
	const UINT defaultVolume = pIns->nGlobalVol;

	
	//If new note, determine notevelocity to use.
	if(note != NOTE_NONE)
	{
		UINT velocity = 4 * defaultVolume;
		switch(pIns->nPluginVelocityHandling)
		{
			case PLUGIN_VELOCITYHANDLING_CHANNEL:
				velocity = pChn->nVolume;
			break;
		}

		ModCommand::NOTE realNote = note;
		if(ModCommand::IsNote(note))
			realNote = pIns->NoteMap[note - 1];
		// Experimental VST panning
		//ProcessMIDIMacro(nChn, false, m_MidiCfg.szMidiGlb[MIDIOUT_PAN], 0, nPlugin);
		pPlugin->MidiCommand(GetBestMidiChannel(nChn), pIns->nMidiProgram, pIns->wMidiBank, realNote, velocity, nChn);
	}

	
	const bool processVolumeAlsoOnNote = (pIns->nPluginVelocityHandling == PLUGIN_VELOCITYHANDLING_VOLUME);

	if((hasVolCommand && !note) || (note && processVolumeAlsoOnNote))
	{
		switch(pIns->nPluginVolumeHandling)
		{
			case PLUGIN_VOLUMEHANDLING_DRYWET:
				if(hasVolCommand) pPlugin->SetDryRatio(2*vol);
				else pPlugin->SetDryRatio(2 * defaultVolume);
				break;

			case PLUGIN_VOLUMEHANDLING_MIDI:
				if(hasVolCommand) pPlugin->MidiCC(GetBestMidiChannel(nChn), MIDICC_Volume_Coarse, min(127, 2 * vol), nChn);
				else pPlugin->MidiCC(GetBestMidiChannel(nChn), MIDICC_Volume_Coarse, min(127, 2 * defaultVolume), nChn);
				break;

		}
	}
}


int CSoundFile::GetVolEnvValueFromPosition(int position, const InstrumentEnvelope &env) const
//-------------------------------------------------------------------------------------------
{
	UINT pt = env.nNodes - 1;

	// Checking where current 'tick' is relative to the 
	// envelope points.
	for (UINT i = 0; i < (UINT)(env.nNodes-1); i++)
	{
		if (position <= env.Ticks[i])
		{
			pt = i;
			break;
		}
	}

	int x2 = env.Ticks[pt];
	int x1, envvol;
	if (position >= x2) // Case: current 'tick' is on a envelope point.
	{
		envvol = env.Values[pt] * 4;
	} else // Case: current 'tick' is between two envelope points.
	{
		if (pt)
		{
			envvol = env.Values[pt - 1] * 4;
			x1 = env.Ticks[pt - 1];
		} else
		{
			envvol = 0;
			x1 = 0;
		}

		if(x2 > x1 && position > x1)
		{
			// Linear approximation between the points;
			// f(x+d) ~ f(x) + f'(x)*d, where f'(x) = (y2-y1)/(x2-x1)
			envvol += ((position - x1) * (((int)env.Values[pt] * 4) - envvol)) / (x2 - x1);
		}
	}
	return envvol;
}

#endif


void CSoundFile::ApplyGlobalVolume(int SoundBuffer[], int RearBuffer[], long lTotalSampleCount)
//---------------------------------------------------------------------------------------------
{
	long step = 0;

	if (m_nGlobalVolumeDestination != m_nGlobalVolume)
	{
		// User has provided new global volume
		const bool rampUp = m_nGlobalVolumeDestination > m_nGlobalVolume;
		m_nGlobalVolumeDestination = m_nGlobalVolume;
		m_nSamplesToGlobalVolRampDest = m_nGlobalVolumeRampAmount = rampUp ? gnVolumeRampUpSamples : gnVolumeRampDownSamples;
	} 

	if (m_nSamplesToGlobalVolRampDest > 0)
	{
		// Still some ramping left to do.
		long highResGlobalVolumeDestination = static_cast<long>(m_nGlobalVolumeDestination) << VOLUMERAMPPRECISION;

		const long delta = highResGlobalVolumeDestination - m_lHighResRampingGlobalVolume;
		step = delta / static_cast<long>(m_nSamplesToGlobalVolRampDest);

		// Define max step size as some factor of user defined ramping value: the lower the value, the more likely the click.
		// If step is too big (might cause click), extend ramp length.
		UINT maxStep = max(50, (10000 / (m_nGlobalVolumeRampAmount + 1)));
		while(static_cast<UINT>(abs(step)) > maxStep)
		{
			m_nSamplesToGlobalVolRampDest += m_nGlobalVolumeRampAmount;
			step = delta / static_cast<long>(m_nSamplesToGlobalVolRampDest);
		}
	}

	const long highResVolume = m_lHighResRampingGlobalVolume;
	const UINT samplesToRamp = m_nSamplesToGlobalVolRampDest;

	// SoundBuffer has interleaved left/right channels for the front channels; RearBuffer has the rear left/right channels.
	// So we process the pairs independently for ramping.
	for (int pairs = max(gnChannels / 2, 1); pairs > 0; pairs--)
	{
		int *sample = (pairs == 1) ? SoundBuffer : RearBuffer;
		m_lHighResRampingGlobalVolume = highResVolume;
		m_nSamplesToGlobalVolRampDest = samplesToRamp;

		for (int pos = lTotalSampleCount; pos > 0; pos--, sample++)
		{
			if (m_nSamplesToGlobalVolRampDest > 0)
			{
				// Ramping required
				m_lHighResRampingGlobalVolume += step;
				*sample = _muldiv(*sample, m_lHighResRampingGlobalVolume, MAX_GLOBAL_VOLUME << VOLUMERAMPPRECISION);
				m_nSamplesToGlobalVolRampDest--;
			} else
			{
				*sample = _muldiv(*sample, m_nGlobalVolume, MAX_GLOBAL_VOLUME);
				m_lHighResRampingGlobalVolume = m_nGlobalVolume << VOLUMERAMPPRECISION;
			}
		}
	}
}
