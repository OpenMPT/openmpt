/*
 * This program is  free software; you can redistribute it  and modify it
 * under the terms of the GNU  General Public License as published by the
 * Free Software Foundation; either version 2  of the license or (at your
 * option) any later version.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

#include "stdafx.h"

// -> CODE#0022
// -> DESC="alternative BPM/Speed interpretation method"
#include "../mptrack/mptrack.h"
#include "../mptrack/moddoc.h"
#include "../mptrack/MainFrm.h"
// -! NEW_FEATURE#0022
#include "sndfile.h"
#include "midi.h"

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
UINT CSoundFile::m_nMaxMixChannels = 32;
// Mixing Configuration (SetWaveConfig)
DWORD CSoundFile::gdwSysInfo = 0;
DWORD CSoundFile::gnChannels = 1;
DWORD CSoundFile::gdwSoundSetup = 0;
DWORD CSoundFile::gdwMixingFreq = 44100;
DWORD CSoundFile::gnBitsPerSample = 16;
// Mixing data initialized in
UINT CSoundFile::gnAGC = AGC_UNITY;
UINT CSoundFile::gnVolumeRampSamples = 42;		//default value
UINT CSoundFile::gnCPUUsage = 0;
LPSNDMIXHOOKPROC CSoundFile::gpSndMixHook = NULL;
PMIXPLUGINCREATEPROC CSoundFile::gpMixPluginCreateProc = NULL;
LONG gnDryROfsVol = 0;
LONG gnDryLOfsVol = 0;
int gbInitPlugins = 0;
int gbInitTables = 0;

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
extern VOID SndMixInitializeTables();

extern short int ModSinusTable[64];
extern short int ModRampDownTable[64];
extern short int ModSquareTable[64];
extern short int ModRandomTable[64];
extern DWORD LinearSlideUpTable[256];
extern DWORD LinearSlideDownTable[256];
extern DWORD FineLinearSlideUpTable[16];
extern DWORD FineLinearSlideDownTable[16];
extern signed char ft2VibratoTable[256];	// -64 .. +64
extern int MixSoundBuffer[MIXBUFFERSIZE*4];
extern int MixRearBuffer[MIXBUFFERSIZE*2];

#ifndef NO_REVERB
extern UINT gnReverbSend;
extern LONG gnRvbROfsVol;
extern LONG gnRvbLOfsVol;

extern void ProcessReverb(UINT nSamples);
#endif

// Log tables for pre-amp
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
		gbInitTables = 1;
	}
#endif
	if (m_nMaxMixChannels > MAX_CHANNELS) m_nMaxMixChannels = MAX_CHANNELS;
	if (gdwMixingFreq < 4000) gdwMixingFreq = 4000;
	if (gdwMixingFreq > MAX_SAMPLE_RATE) gdwMixingFreq = MAX_SAMPLE_RATE;
	// Start with ramping disabled to avoid clicks on first read.
	// Ramping is now set after the first read in CSoundFile::Read();
	gnVolumeRampSamples = 0; 
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
		MODCHANNEL *pramp = &Chn[ChnMix[noff]];
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
#endif
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
#endif
			if (!ReadNote())
			{
#ifdef MODPLUG_TRACKER
				if ((m_nMaxOrderPosition) && (m_nCurrentPattern >= m_nMaxOrderPosition))
				{
					m_dwSongFlags |= SONG_ENDREACHED;
					break;
				}
#endif // MODPLUG_TRACKER
#ifndef FASTSOUNDLIB
				if (!FadeSong(FADESONGDELAY) || m_bIsRendering)	//rewbs: disable song fade when rendering.
#endif
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
	#endif
		// Resetting sound buffer
		X86_StereoFill(MixSoundBuffer, lSampleCount, &gnDryROfsVol, &gnDryLOfsVol);
		
		ASSERT(lCount<=MIXBUFFERSIZE);		// ensure MIXBUFFERSIZE really is our max buffer size
		if (gnChannels >= 2)
		{
			lSampleCount *= 2;
			m_nMixStat += CreateStereoMix(lCount);
		#ifndef NO_REVERB
			ProcessReverb(lCount);
		#endif
			if (nMaxPlugins) ProcessPlugins(lCount);
			ProcessStereoDSP(lCount);
		} else
		{
			m_nMixStat += CreateStereoMix(lCount);
		#ifndef NO_REVERB
			ProcessReverb(lCount);
		#endif
			if (nMaxPlugins) ProcessPlugins(lCount);
			X86_MonoFromStereo(MixSoundBuffer, lCount);
			ProcessMonoDSP(lCount);
		}
		// Graphic Equalizer
#ifdef ENABLE_EQ
		if (gdwSoundSetup & SNDMIX_EQ)
		{
			if (gnChannels >= 2)
				EQStereo(MixSoundBuffer, lCount);
			else
				EQMono(MixSoundBuffer, lCount);
		}
#endif
		nStat++;
#ifndef NO_AGC
		// Automatic Gain Control
		if (gdwSoundSetup & SNDMIX_AGC) ProcessAGC(lSampleCount);
#endif
		UINT lTotalSampleCount = lSampleCount;
#ifndef FASTSOUNDLIB
		// Multichannel
		if (gnChannels > 2)
		{
			X86_InterleaveFrontRear(MixSoundBuffer, MixRearBuffer, lSampleCount);
			lTotalSampleCount *= 2;
		}
		// Noise Shaping
		if (gnBitsPerSample <= 16)
		{
			if ((gdwSoundSetup & SNDMIX_HQRESAMPLER)
			 && ((gnCPUUsage < 25) || (gdwSoundSetup & SNDMIX_DIRECTTODISK)))
				X86_Dither(MixSoundBuffer, lTotalSampleCount, gnBitsPerSample);
		}

		//Apply global volume
		if (m_pConfig->getGlobalVolumeAppliesToMaster()) {
			ApplyGlobalVolume(MixSoundBuffer, lTotalSampleCount);
		}

		// Hook Function
		if (gpSndMixHook) {	//Currently only used for VU Meter, so it's OK to do it after global Vol.
			gpSndMixHook(MixSoundBuffer, lTotalSampleCount, gnChannels);
		}
#endif

		// Perform clipping
		lpBuffer += pCvt(lpBuffer, MixSoundBuffer, lTotalSampleCount);

		// Buffer ready
		lRead -= lCount;
		m_nBufferCount -= lCount;
		m_lTotalSampleCount += lCount;		// increase sample count for VSTTimeInfo.
		// Turn on ramping after first read (fix http://lpchip.com/modplug/viewtopic.php?t=523 )
		gnVolumeRampSamples = CMainFrame::glVolumeRampSamples;
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
	if (++m_nTickCount >= m_nMusicSpeed * (m_nPatternDelay+1) + m_nFrameDelay)
	{
		HandlePatternTransitionEvents();
		m_nPatternDelay = 0;
		m_nFrameDelay = 0;
		m_nTickCount = 0;
		m_nRow = m_nNextRow;
		// Reset Pattern Loop Effect
		if (m_nCurrentPattern != m_nNextPattern) 
			m_nCurrentPattern = m_nNextPattern;
		// Check if pattern is valid
		if (!(m_dwSongFlags & SONG_PATTERNLOOP))
		{
			m_nPattern = (m_nCurrentPattern < Order.size()) ? Order[m_nCurrentPattern] : Order.GetInvalidPatIndex();
			if ((m_nPattern < Patterns.Size()) && (!Patterns[m_nPattern])) m_nPattern = Order.GetIgnoreIndex();
			while (m_nPattern >= Patterns.Size())
			{
				// End of song ?
				if ((m_nPattern == Order.GetInvalidPatIndex()) || (m_nCurrentPattern >= Order.size()))
				{

					if (!m_nRepeatCount) return FALSE;
					if (!m_nRestartPos)
					{

						//rewbs.instroVSTi: stop all VSTi at end of song, if looping.
						StopAllVsti();
						m_nMusicSpeed = m_nDefaultSpeed;
						m_nMusicTempo = m_nDefaultTempo;
						m_nGlobalVolume = m_nDefaultGlobalVolume;
						for (UINT i=0; i<MAX_CHANNELS; i++)	{
							Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
							Chn[i].nFadeOutVol = 0;

							if (i < m_nChannels) {
								Chn[i].nGlobalVol = ChnSettings[i].nVolume;
								Chn[i].nVolume = ChnSettings[i].nVolume;
								Chn[i].nPan = ChnSettings[i].nPan;
								Chn[i].nPanSwing = Chn[i].nVolSwing = 0;
								Chn[i].nCutSwing = Chn[i].nResSwing = 0;
								Chn[i].nOldVolParam = 0;
								Chn[i].nOldOffset = 0;
								Chn[i].nOldHiOffset = 0;
								Chn[i].nPortamentoDest = 0;

								if (!Chn[i].nLength) {
									Chn[i].dwFlags = ChnSettings[i].dwFlags;
									Chn[i].nLoopStart = 0;
									Chn[i].nLoopEnd = 0;
									Chn[i].pHeader = NULL;
									Chn[i].pSample = NULL;
									Chn[i].pInstrument = NULL;
								}
							}
						}


					}

					//Handle Repeat position
					if (m_nRepeatCount > 0) m_nRepeatCount--;
					m_nCurrentPattern = m_nRestartPos;
					m_nRow = 0;					
					//If restart pos points to +++, move along
					while (Order[m_nCurrentPattern] == Order.GetIgnoreIndex()) {
						m_nCurrentPattern++;
					}
					//Check for end of song or bad pattern
					if (m_nCurrentPattern >= Order.size()
						||
						(Order[m_nCurrentPattern] >= Patterns.Size()) 
						|| (!Patterns[Order[m_nCurrentPattern]]) ) 	{
						return FALSE;
					}

				} else {
					m_nCurrentPattern++;
				}

				if (m_nCurrentPattern < Order.size()) {
					m_nPattern = Order[m_nCurrentPattern];
				} else {
					m_nPattern = Order.GetInvalidPatIndex();
				}

				if ((m_nPattern < Patterns.Size()) && (!Patterns[m_nPattern])) {
					m_nPattern = Order.GetIgnoreIndex();
				}
			}
			m_nNextPattern = m_nCurrentPattern;

	#ifdef MODPLUG_TRACKER
		if ((m_nMaxOrderPosition) && (m_nCurrentPattern >= m_nMaxOrderPosition)) return FALSE;
	#endif // MODPLUG_TRACKER
		}

		// Weird stuff?
		if ((m_nPattern >= Patterns.Size()) || (!Patterns[m_nPattern])) return FALSE;
		// Should never happen
		if (m_nRow >= PatternSize[m_nPattern]) m_nRow = 0;
		m_nNextRow = m_nRow + 1;
		if (m_nNextRow >= PatternSize[m_nPattern])
		{
			if (!(m_dwSongFlags & SONG_PATTERNLOOP)) m_nNextPattern = m_nCurrentPattern + 1;
			m_nNextRow = 0;
			m_bPatternTransitionOccurred=true;
		}
		// Reset channel values
		MODCHANNEL *pChn = Chn;
		MODCOMMAND *m = Patterns[m_nPattern] + m_nRow * m_nChannels;
		for (UINT nChn=0; nChn<m_nChannels; pChn++, nChn++, m++)
		{
			pChn->nRowNote = m->note;
			pChn->nRowInstr = m->instr;
			pChn->nRowVolCmd = m->volcmd;
			pChn->nRowVolume = m->vol;
			pChn->nRowCommand = m->command;
			pChn->nRowParam = m->param;

			pChn->nLeftVol = pChn->nNewLeftVol;
			pChn->nRightVol = pChn->nNewRightVol;
			pChn->dwFlags &= ~(CHN_PORTAMENTO | CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO);
			pChn->nCommand = 0;
			pChn->m_nPlugParamValueStep = 0;
		}

	}
	// Should we process tick0 effects?
	if (!m_nMusicSpeed) m_nMusicSpeed = 1;
	m_dwSongFlags |= SONG_FIRSTTICK;

	//End of row? stop pattern step (aka "play row").
	if (m_nTickCount >= m_nMusicSpeed * (m_nPatternDelay+1) + m_nFrameDelay - 1) {
		#ifdef MODPLUG_TRACKER
		if (m_dwSongFlags & SONG_STEP) {
			m_dwSongFlags &= ~SONG_STEP;
			m_dwSongFlags |= SONG_PAUSED;
		}
		#endif // MODPLUG_TRACKER
	}

	if (m_nTickCount)
	{
		m_dwSongFlags &= ~SONG_FIRSTTICK;
		if ((!(m_nType & MOD_TYPE_XM)) && (m_nTickCount < m_nMusicSpeed * (1 + m_nPatternDelay)))
		{
			if (!(m_nTickCount % m_nMusicSpeed)) m_dwSongFlags |= SONG_FIRSTTICK;
		}
	}
	// Update Effects
	return ProcessEffects();
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

	switch(m_nTempoMode) {

		case tempo_mode_alternative: 
			m_nBufferCount = gdwMixingFreq / m_nMusicTempo;
			break;

		case tempo_mode_modern: {
			double accurateBufferCount = (double)gdwMixingFreq * (60.0/(double)m_nMusicTempo / ((double)m_nMusicSpeed * (double)m_nRowsPerBeat));
			m_nBufferCount = static_cast<int>(accurateBufferCount);
			m_dBufferDiff += accurateBufferCount-m_nBufferCount;
			//tick-to-tick tempo correction:
			if (m_dBufferDiff>=1) { 
				m_nBufferCount++;
				m_dBufferDiff--;
			} else if (m_dBufferDiff<=-1) { 
				m_nBufferCount--;
				m_dBufferDiff++;
			}
			ASSERT(abs(m_dBufferDiff)<1);
			break;
		}

		case tempo_mode_classic:
		default:
			m_nBufferCount = (gdwMixingFreq * 5 * m_nTempoFactor) / (m_nMusicTempo << 8);
	}
	
	m_nSamplesPerTick = m_nBufferCount; //rewbs.flu


// robinf: this block causes envelopes to behave incorrectly when 
// playback is triggered from instrument panel. 
// I can't see why it would be useful. Dissabling for now.
//
//	if (m_dwSongFlags & SONG_PAUSED) {
//		m_nBufferCount = gdwMixingFreq / 64; // 1/64 seconds
//	}


	// Master Volume + Pre-Amplification / Attenuation setup
	DWORD nMasterVol;
	{
		int nchn32 = 0;
		MODCHANNEL *pChn = Chn;
		for (UINT nChn=0; nChn<m_nChannels; nChn++,pChn++) {
			//if(!(pChn->dwFlags & CHN_MUTE))	//removed by rewbs: fix http://www.modplug.com/forum/viewtopic.php?t=3358
				nchn32++;
		}
		if(nchn32 < 1) nchn32 = 1;
		if(nchn32 > 31) nchn32 = 31;

		
		DWORD mastervol;

		if (m_pConfig->getUseGlobalPreAmp()) {
			int realmastervol = m_nMasterVolume;
			if (realmastervol > 0x80) {
				//Attenuate global pre-amp depending on num channels
				realmastervol = 0x80 + ((realmastervol - 0x80) * (nchn32+4)) / 16;
			}
			mastervol = (realmastervol * (m_nSamplePreAmp)) >> 6;
		} else {
			//Preferred option: don't use global pre-amp at all.
			mastervol = m_nSamplePreAmp;
		}

		if ((m_dwSongFlags & SONG_GLOBALFADE) && (m_nGlobalFadeMaxSamples))
		{
			mastervol = _muldiv(mastervol, m_nGlobalFadeSamples, m_nGlobalFadeMaxSamples);
		}

		if (m_pConfig->getUseGlobalPreAmp()) {
			UINT attenuation = (gdwSoundSetup & SNDMIX_AGC) ? PreAmpAGCTable[nchn32>>1] : PreAmpTable[nchn32>>1];
			if(attenuation < 1) attenuation = 1;
			nMasterVol = (mastervol << 7) / attenuation;
		} else {
			nMasterVol = mastervol;
		}
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Update channels data
	m_nMixChannels = 0;
	MODCHANNEL *pChn = Chn;
	for (UINT nChn=0; nChn<MAX_CHANNELS; nChn++,pChn++)
	{
	skipchn:
		if ((pChn->dwFlags & CHN_NOTEFADE) && (!(pChn->nFadeOutVol|pChn->nRightVol|pChn->nLeftVol)))
		{
			pChn->nLength = 0;
			pChn->nROfs = pChn->nLOfs = 0;
		}
		// Check for unused channel
		if ((pChn->dwFlags & CHN_MUTE) || ((nChn >= m_nChannels) && (!pChn->nLength)))
		{
			pChn->nVUMeter = 0;
#ifdef ENABLE_STEREOVU
			pChn->nLeftVU = pChn->nRightVU = 0;
#endif
			nChn++;
			pChn++;
			if (nChn >= m_nChannels)
			{
				while ((nChn < MAX_CHANNELS) && (!pChn->nLength))
				{
					nChn++;
					pChn++;
				}
			}
			if (nChn < MAX_CHANNELS) goto skipchn;
			goto done;
		}
		// Reset channel data
		pChn->nInc = 0;
		pChn->nRealVolume = 0;

		if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || GetModFlag(MSF_OLDVOLSWING))
		{
			pChn->nRealPan = pChn->nPan + pChn->nPanSwing;
		}
		else
		{
			pChn->nPan += pChn->nPanSwing;
			if(pChn->nPan > 256) pChn->nPan = 256;
			if(pChn->nPan < 0) pChn->nPan = 0;
			pChn->nPanSwing = 0;
			pChn->nRealPan = pChn->nPan;
		}

		if (pChn->nRealPan < 0) pChn->nRealPan = 0;
		if (pChn->nRealPan > 256) pChn->nRealPan = 256;
		pChn->nRampLength = 0;

		//Aux variables
		CTuning::RATIOTYPE vibratoFactor = 1;
		CTuning::NOTEINDEXTYPE arpeggioSteps = 0;

		// Calc Frequency
		if ((pChn->nPeriod)	&& (pChn->nLength))
		{
			int vol = pChn->nVolume;

			if(pChn->nVolSwing)
			{
				if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || GetModFlag(MSF_OLDVOLSWING))
				{
					vol = pChn->nVolume + pChn->nVolSwing;
				}
				else
				{
					pChn->nVolume += pChn->nVolSwing;
					if(pChn->nVolume > 256) pChn->nVolume = 256;
					if(pChn->nVolume < 0) pChn->nVolume = 0;
					vol = pChn->nVolume;
					pChn->nVolSwing = 0;
				}
			}

			if (vol < 0) vol = 0;
			if (vol > 256) vol = 256;

			// Tremolo
			if (pChn->dwFlags & CHN_TREMOLO)
			{
				UINT trempos = pChn->nTremoloPos & 0x3F;
				if (vol > 0)
				{
					int tremattn = (m_nType & MOD_TYPE_XM) ? 5 : 6;
					switch (pChn->nTremoloType & 0x03)
					{
					case 1:
						vol += (ModRampDownTable[trempos] * (int)pChn->nTremoloDepth) >> tremattn;
						break;
					case 2:
						vol += (ModSquareTable[trempos] * (int)pChn->nTremoloDepth) >> tremattn;
						break;
					case 3:
						vol += (ModRandomTable[trempos] * (int)pChn->nTremoloDepth) >> tremattn;
						break;
					default:
						vol += (ModSinusTable[trempos] * (int)pChn->nTremoloDepth) >> tremattn;
					}
				}
				if ((m_nTickCount) || ((m_nType & (MOD_TYPE_STM|MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)) && (!(m_dwSongFlags & SONG_ITOLDEFFECTS))))
				{
					pChn->nTremoloPos = (trempos + pChn->nTremoloSpeed) & 0x3F;
				}
			}

			// Tremor
			if (pChn->nCommand == CMD_TREMOR)
			{
				UINT n = (pChn->nTremorParam >> 4) + (pChn->nTremorParam & 0x0F);
				UINT ontime = pChn->nTremorParam >> 4;
				if ((!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) || (m_dwSongFlags & SONG_ITOLDEFFECTS)) { n += 2; ontime++; }
				UINT tremcount = (UINT)pChn->nTremorCount;
				if (tremcount >= n) tremcount = 0;
				if ((m_nTickCount) || (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)))
				{
					if (tremcount >= ontime) vol = 0;
					pChn->nTremorCount = (BYTE)(tremcount + 1);
				}
				pChn->dwFlags |= CHN_FASTVOLRAMP;
			}

			// Clip volume
			if (vol < 0) vol = 0;
			if (vol > 0x100) vol = 0x100;
			vol <<= 6;

			// Process Envelopes
			if (pChn->pHeader)
			{
				INSTRUMENTHEADER *penv = pChn->pHeader;
				// Volume Envelope
				if ((pChn->dwFlags & CHN_VOLENV) && (penv->nVolEnv))
				{
					int envvol = getVolEnvValueFromPosition(pChn->nVolEnvPosition, penv);
					
					// if we are in the release portion of the envelope,
					// rescale envelope factor so that it is proportional to the release point
					// and release envelope beginning.
					if (penv->nVolEnvReleaseNode != ENV_RELEASE_NODE_UNSET
						&& pChn->nVolEnvPosition>=penv->VolPoints[penv->nVolEnvReleaseNode]
						&& pChn->nVolEnvValueAtReleaseJump != NOT_YET_RELEASED) {
						int envValueAtReleaseJump = pChn->nVolEnvValueAtReleaseJump;
						int envValueAtReleaseNode = penv->VolEnv[penv->nVolEnvReleaseNode] << 2;

						//If we have just hit the release node, force the current env value
						//to be that of the release node. This works around the case where 
						// we have another node at the same position as the release node.
						if (pChn->nVolEnvPosition==penv->VolPoints[penv->nVolEnvReleaseNode]) {
							envvol=envValueAtReleaseNode;
						}

						int relativeVolumeChange = (envvol-envValueAtReleaseNode)*2;
						envvol = envValueAtReleaseJump + relativeVolumeChange;
					}
					if (envvol < 0) envvol = 0;
					if (envvol > 512) 
						envvol = 512;
					vol = (vol * envvol) >> 8;
				}
				// Panning Envelope
				if ((pChn->dwFlags & CHN_PANENV) && (penv->nPanEnv))
				{
					int envpos = pChn->nPanEnvPosition;
					UINT pt = penv->nPanEnv - 1;
					for (UINT i=0; i<(UINT)(penv->nPanEnv-1); i++)
					{
						if (envpos <= penv->PanPoints[i])
						{
							pt = i;
							break;
						}
					}
					int x2 = penv->PanPoints[pt], y2 = penv->PanEnv[pt];
					int x1, envpan;
					if (envpos >= x2)
					{
						envpan = y2;
						x1 = x2;
					} else
					if (pt)
					{
						envpan = penv->PanEnv[pt-1];
						x1 = penv->PanPoints[pt-1];
					} else
					{
						envpan = 128;
						x1 = 0;
					}
					if ((x2 > x1) && (envpos > x1))
					{
						envpan += ((envpos - x1) * (y2 - envpan)) / (x2 - x1);
					}
					if (envpan < 0) envpan = 0;
					if (envpan > 64) envpan = 64;
					int pan = pChn->nPan;
					if (pan >= 128)
					{
						pan += ((envpan - 32) * (256 - pan)) / 32;
					} else
					{
						pan += ((envpan - 32) * (pan)) / 32;
					}
					if (pan < 0) pan = 0;
					if (pan > 256) pan = 256;
					pChn->nRealPan = pan;
				}
				// FadeOut volume
				if (pChn->dwFlags & CHN_NOTEFADE)
				{
					UINT fadeout = penv->nFadeOut;
					if (fadeout)
					{
						pChn->nFadeOutVol -= fadeout << 1;
						if (pChn->nFadeOutVol <= 0) pChn->nFadeOutVol = 0;
						vol = (vol * pChn->nFadeOutVol) >> 16;
					} else
					if (!pChn->nFadeOutVol)
					{
						vol = 0;
					}
				}
				// Pitch/Pan separation
				if ((penv->nPPS) && (pChn->nRealPan) && (pChn->nNote))
				{
					int pandelta = (int)pChn->nRealPan + (int)((int)(pChn->nNote - penv->nPPC - 1) * (int)penv->nPPS) / (int)8;
					if (pandelta < 0) pandelta = 0;
					if (pandelta > 256) pandelta = 256;
					pChn->nRealPan = pandelta;
				}
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
			if (vol) {
				// IMPORTANT: pChn->nRealVolume is 14 bits !!!
				// -> _muldiv( 14+8, 6+6, 18); => RealVolume: 14-bit result (22+12-20)
				
				//UINT nPlugin = GetBestPlugin(nChn, PRIORITISE_INSTRUMENT, RESPECT_MUTES);

				//Don't let global volume affect level of sample if
				//global volume is going to be applied to master output anyway.
				if (pChn->dwFlags&CHN_SYNCMUTE) {
					pChn->nRealVolume = 0;
				} else if (m_pConfig->getGlobalVolumeAppliesToMaster()) {
					pChn->nRealVolume = _muldiv(vol*MAX_GLOBAL_VOLUME, pChn->nGlobalVol * pChn->nInsVol, 1 << 20);
				} else {
					pChn->nRealVolume = _muldiv(vol * m_nGlobalVolume, pChn->nGlobalVol * pChn->nInsVol, 1 << 20);
				}
			}
			if (pChn->nPeriod < m_nMinPeriod) pChn->nPeriod = m_nMinPeriod;
			int period = pChn->nPeriod;
			if ((pChn->dwFlags & (CHN_GLISSANDO|CHN_PORTAMENTO)) ==	(CHN_GLISSANDO|CHN_PORTAMENTO))
			{
				period = GetPeriodFromNote(GetNoteFromPeriod(period), pChn->nFineTune, pChn->nC4Speed);
			}

			// Arpeggio ?
			if (pChn->nCommand == CMD_ARPEGGIO)
			{
				if(m_nType == MOD_TYPE_MPT && pChn->pHeader && pChn->pHeader->pTuning)
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
							arpeggioSteps = pChn->nArpeggio % 16; //Gives the latter number in the parameter.
							break;
					}
					pChn->m_CalculateFreq = true;
					pChn->m_ReCalculateFreqOnFirstTick = true;
				}
				else
				{
					//IT playback compatibility 01 & 02
					if(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT) && GetModFlag(MSF_IT_COMPATIBLE_PLAY))
					{
						if(pChn->nArpeggio >> 4 != 0 || (pChn->nArpeggio & 0x0F) != 0)
						{
							switch(m_nTickCount % 3)
							{
								case 1:	period = period / TwoToPowerXOver12(pChn->nArpeggio >> 4); break;
								case 2:	period = period / TwoToPowerXOver12(pChn->nArpeggio & 0x0F); break;
							}
						}
					}
					else //Original
					{
						BYTE note = pChn->nNote;
						switch(m_nTickCount % 3)
						{
							case 1:	period = GetPeriodFromNote(note + (pChn->nArpeggio >> 4), pChn->nFineTune, pChn->nC4Speed); break;
							case 2:	period = GetPeriodFromNote(note + (pChn->nArpeggio & 0x0F), pChn->nFineTune, pChn->nC4Speed); break;
						}
					}
				}
			}

			if (m_dwSongFlags & SONG_AMIGALIMITS)
			{
				if (period < 113*4) period = 113*4;
				if (period > 856*4) period = 856*4;
			}

			// Pitch/Filter Envelope
			if ((pChn->pHeader) && (pChn->dwFlags & CHN_PITCHENV) && (pChn->pHeader->nPitchEnv))
			{
				INSTRUMENTHEADER *penv = pChn->pHeader;
				int envpos = pChn->nPitchEnvPosition;
				UINT pt = penv->nPitchEnv - 1;
				for (UINT i=0; i<(UINT)(penv->nPitchEnv-1); i++)
				{
					if (envpos <= penv->PitchPoints[i])
					{
						pt = i;
						break;
					}
				}
				int x2 = penv->PitchPoints[pt];
				int x1, envpitch;
				if (envpos >= x2)
				{
					envpitch = (((int)penv->PitchEnv[pt]) - 32) * 8;
					x1 = x2;
				} else
				if (pt)
				{
					envpitch = (((int)penv->PitchEnv[pt-1]) - 32) * 8;
					x1 = penv->PitchPoints[pt-1];
				} else
				{
					envpitch = 0;
					x1 = 0;
				}
				if (envpos > x2) envpos = x2;
				if ((x2 > x1) && (envpos > x1))
				{
					int envpitchdest = (((int)penv->PitchEnv[pt]) - 32) * 8;
					envpitch += ((envpos - x1) * (envpitchdest - envpitch)) / (x2 - x1);
				}
				if (envpitch < -256) envpitch = -256;
				if (envpitch > 256) envpitch = 256;
				// Filter Envelope: controls cutoff frequency
				if (penv->dwFlags & ENV_FILTER)
				{
#ifndef NO_FILTER
					SetupChannelFilter(pChn, (pChn->dwFlags & CHN_FILTER) ? FALSE : TRUE, envpitch);
#endif // NO_FILTER
				} else
				// Pitch Envelope
				{
					if(m_nType == MOD_TYPE_MPT && pChn->pHeader && pChn->pHeader->pTuning)
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
			
			// Vibrato
			if (pChn->dwFlags & CHN_VIBRATO)
			{
				UINT vibpos = pChn->nVibratoPos;
				LONG vdelta;
				switch (pChn->nVibratoType & 0x03)
				{
				case 1:
					vdelta = ModRampDownTable[vibpos];
					break;
				case 2:
					vdelta = ModSquareTable[vibpos];
					break;
				case 3:
					vdelta = ModRandomTable[vibpos];
					break;
				default:
					vdelta = ModSinusTable[vibpos];
				}

				if(m_nType == MOD_TYPE_MPT && pChn->pHeader && pChn->pHeader->pTuning)
				{
					//Hack implementation: Scaling vibratofactor to [0.95; 1.05]
					//using figure from above tables and vibratodepth parameter
					vibratoFactor += 0.05F * vdelta * pChn->m_VibratoDepth / 128.0F;
					pChn->m_CalculateFreq = true;
					pChn->m_ReCalculateFreqOnFirstTick = false;

					if(m_nTickCount + 1 == m_nMusicSpeed)
						pChn->m_ReCalculateFreqOnFirstTick = true;
				}
				else //Original behavior
				{
					UINT vdepth = ((!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) || (m_dwSongFlags & SONG_ITOLDEFFECTS)) ? 6 : 7;
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
				if ((m_nTickCount) || ((m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (!(m_dwSongFlags & SONG_ITOLDEFFECTS))))
				{
					pChn->nVibratoPos = (vibpos + pChn->nVibratoSpeed) & 0x3F;
				}
			}
			// Panbrello
			if (pChn->dwFlags & CHN_PANBRELLO)
			{
				UINT panpos = ((pChn->nPanbrelloPos+0x10) >> 2) & 0x3F;
				LONG pdelta;
				switch (pChn->nPanbrelloType & 0x03)
				{
				case 1:
					pdelta = ModRampDownTable[panpos];
					break;
				case 2:
					pdelta = ModSquareTable[panpos];
					break;
				case 3:
					pdelta = ModRandomTable[panpos];
					break;
				default:
					pdelta = ModSinusTable[panpos];
				}
				pChn->nPanbrelloPos += pChn->nPanbrelloSpeed;
				pdelta = ((pdelta * (int)pChn->nPanbrelloDepth) + 2) >> 3;
				pdelta += pChn->nRealPan;
				if (pdelta < 0) pdelta = 0;
				if (pdelta > 256) pdelta = 256;
				pChn->nRealPan = pdelta;
			}
			int nPeriodFrac = 0;
			// Instrument Auto-Vibrato
			if ((pChn->pInstrument) && (pChn->pInstrument->nVibDepth))
			{
				MODINSTRUMENT *pins = pChn->pInstrument;

				if (pins->nVibSweep == 0)
				{
					pChn->nAutoVibDepth = pins->nVibDepth << 8;
				} else
				{
					if (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT))
					{
						pChn->nAutoVibDepth += pins->nVibSweep << 3;
					} else
					if (!(pChn->dwFlags & CHN_KEYOFF))
					{
						pChn->nAutoVibDepth += (pins->nVibDepth << 8) /	pins->nVibSweep;
					}
					if ((pChn->nAutoVibDepth >> 8) > pins->nVibDepth)
						pChn->nAutoVibDepth = pins->nVibDepth << 8;
				}
				pChn->nAutoVibPos += pins->nVibRate;
				int val;
				switch(pins->nVibType)
				{
				case 4:	// Random
					val = ModRandomTable[pChn->nAutoVibPos & 0x3F];
					pChn->nAutoVibPos++;
					break;
				case 3:	// Ramp Down
					val = ((0x40 - (pChn->nAutoVibPos >> 1)) & 0x7F) - 0x40;
					break;
				case 2:	// Ramp Up
					val = ((0x40 + (pChn->nAutoVibPos >> 1)) & 0x7f) - 0x40;
					break;
				case 1:	// Square
					val = (pChn->nAutoVibPos & 128) ? +64 : -64;
					break;
				default:	// Sine
					val = ft2VibratoTable[pChn->nAutoVibPos & 255];
				}
				int n =	((val * pChn->nAutoVibDepth) >> 8);

				if(pChn->pHeader && pChn->pHeader->pTuning)
				{
					//Vib sweep is not taken into account here.
					vibratoFactor += 0.05F * pins->nVibDepth * val / 4096.0F; //4096 == 64^2
					//See vibrato for explanation.
					pChn->m_CalculateFreq = true;
					/*
					Finestep vibrato:
					const float autoVibDepth = pins->nVibDepth * val / 4096.0F; //4096 == 64^2
					vibratoFineSteps += static_cast<CTuning::FINESTEPTYPE>(pChn->pHeader->pTuning->GetFineStepCount() *  autoVibDepth);
					pChn->m_CalculateFreq = true;
					*/
				}
				else //Original behavior
				{
					if (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT))
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
						period = _muldiv(period, df1 + ((df2-df1)*(n&0x3F)>>6), 256);
						nPeriodFrac = period & 0xFF;
						period >>= 8;
					} else
					{
						period += (n >> 6);
					}
				} //Original MPT behavior
			} //End: AutoVibrato
			// Final Period
			if (period <= m_nMinPeriod)
			{
				if (m_nType & MOD_TYPE_S3M) pChn->nLength = 0;
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

			if(m_nType != MOD_TYPE_MPT || !pChn->pHeader || pChn->pHeader->pTuning == NULL)
			{
				freq = GetFreqFromPeriod(period, pChn->nC4Speed, nPeriodFrac);
			}
			else //In this case: m_nType == MOD_TYPE_MPT and using custom tunings.
			{
				if(pChn->m_CalculateFreq || (pChn->m_ReCalculateFreqOnFirstTick && m_nTickCount == 0))
				{
					pChn->m_Freq = pChn->nC4Speed * vibratoFactor * pChn->pHeader->pTuning->GetRatio(pChn->nNote - NOTE_MIDDLEC + arpeggioSteps, pChn->nFineTune+pChn->m_PortamentoFineSteps);
					if(!pChn->m_CalculateFreq)
						pChn->m_ReCalculateFreqOnFirstTick = false;
					else
						pChn->m_CalculateFreq = false;
				}

				freq = pChn->m_Freq;
			}

			//Applying Pitch/Tempo lock.
            if(m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT) && pChn->pHeader && pChn->pHeader->wPitchToTempoLock)
				freq *= (float)m_nMusicTempo / (float)pChn->pHeader->wPitchToTempoLock;


			if ((m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (freq < 256))
			{
				pChn->nFadeOutVol = 0;
				pChn->dwFlags |= CHN_NOTEFADE;
				pChn->nRealVolume = 0;
			}

			UINT ninc = _muldiv(freq, 0x10000, gdwMixingFreq);
			if ((ninc >= 0xFFB0) && (ninc <= 0x10090)) ninc = 0x10000;
			if (m_nFreqFactor != 128) ninc = (ninc * m_nFreqFactor) >> 7;
			if (ninc > 0xFF0000) ninc = 0xFF0000;
			pChn->nInc = (ninc+1) & ~3;
		}
		
		// Increment envelope position
		if (pChn->pHeader)
		{
			INSTRUMENTHEADER *penv = pChn->pHeader;
			// Volume Envelope
			if (pChn->dwFlags & CHN_VOLENV)
			{
				// Increase position
				pChn->nVolEnvPosition++;
				// Volume Loop ?
				if (penv->dwFlags & ENV_VOLLOOP)
				{
					UINT volloopend = penv->VolPoints[penv->nVolLoopEnd];
					if (m_nType != MOD_TYPE_XM) volloopend++;
					if (pChn->nVolEnvPosition == volloopend)
					{
						pChn->nVolEnvPosition = penv->VolPoints[penv->nVolLoopStart];
						if ((penv->nVolLoopEnd == penv->nVolLoopStart) && (!penv->VolEnv[penv->nVolLoopStart])
						 && ((!(m_nType & MOD_TYPE_XM)) || (penv->nVolLoopEnd+1 == (int)penv->nVolEnv)))
						{
							pChn->dwFlags |= CHN_NOTEFADE;
							pChn->nFadeOutVol = 0;
						}
					}
				}
				// Volume Sustain ?
				if ((penv->dwFlags & ENV_VOLSUSTAIN) && (!(pChn->dwFlags & CHN_KEYOFF)))
				{
					if (pChn->nVolEnvPosition == (UINT)penv->VolPoints[penv->nVolSustainEnd]+1)
						pChn->nVolEnvPosition = penv->VolPoints[penv->nVolSustainBegin];
				} else
				// End of Envelope ?
				if (pChn->nVolEnvPosition > penv->VolPoints[penv->nVolEnv - 1])
				{
					if ((m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) || (pChn->dwFlags & CHN_KEYOFF)) pChn->dwFlags |= CHN_NOTEFADE;
					pChn->nVolEnvPosition = penv->VolPoints[penv->nVolEnv - 1];
					if ((!penv->VolEnv[penv->nVolEnv-1]) && ((nChn >= m_nChannels) || (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT))))
					{
						pChn->dwFlags |= CHN_NOTEFADE;
						pChn->nFadeOutVol = 0;
						pChn->nRealVolume = 0;
					}
				}
			}
			// Panning Envelope
			if (pChn->dwFlags & CHN_PANENV)
			{
				pChn->nPanEnvPosition++;
				if (penv->dwFlags & ENV_PANLOOP)
				{
					UINT panloopend = penv->PanPoints[penv->nPanLoopEnd];
					if (m_nType != MOD_TYPE_XM) panloopend++;
					if (pChn->nPanEnvPosition == panloopend)
						pChn->nPanEnvPosition = penv->PanPoints[penv->nPanLoopStart];
				}
				// Panning Sustain ?
				if ((penv->dwFlags & ENV_PANSUSTAIN) && (pChn->nPanEnvPosition == (UINT)penv->PanPoints[penv->nPanSustainEnd]+1)
				 && (!(pChn->dwFlags & CHN_KEYOFF)))
				{
					// Panning sustained
					pChn->nPanEnvPosition = penv->PanPoints[penv->nPanSustainBegin];
				} else
				{
					if (pChn->nPanEnvPosition > penv->PanPoints[penv->nPanEnv - 1])
						pChn->nPanEnvPosition = penv->PanPoints[penv->nPanEnv - 1];
				}
			}
			// Pitch Envelope
			if (pChn->dwFlags & CHN_PITCHENV)
			{
				// Increase position
				pChn->nPitchEnvPosition++;
				// Pitch Loop ?
				if (penv->dwFlags & ENV_PITCHLOOP)
				{
					if (pChn->nPitchEnvPosition >= penv->PitchPoints[penv->nPitchLoopEnd])
						pChn->nPitchEnvPosition = penv->PitchPoints[penv->nPitchLoopStart];
				}
				// Pitch Sustain ?
				if ((penv->dwFlags & ENV_PITCHSUSTAIN) && (!(pChn->dwFlags & CHN_KEYOFF)))
				{
					if (pChn->nPitchEnvPosition == (UINT)penv->PitchPoints[penv->nPitchSustainEnd]+1)
						pChn->nPitchEnvPosition = penv->PitchPoints[penv->nPitchSustainBegin];
				} else
				{
					if (pChn->nPitchEnvPosition > penv->PitchPoints[penv->nPitchEnv - 1])
						pChn->nPitchEnvPosition = penv->PitchPoints[penv->nPitchEnv - 1];
				}
			}
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
			UINT kChnMasterVol = (pChn->dwFlags & CHN_EXTRALOUD) ? 0x100 : nMasterVol;
#else
#define		kChnMasterVol	nMasterVol
#endif // MODPLUG_TRACKER
			// Adjusting volumes
			if (gnChannels >= 2) {
				int pan = ((int)pChn->nRealPan) - 128;
				pan *= (int)m_nStereoSeparation;
				pan /= 128;
				pan += 128;
				if (pan < 0) pan = 0;
				if (pan > 256) pan = 256;
#ifndef FASTSOUNDLIB
				if (gdwSoundSetup & SNDMIX_REVERSESTEREO) pan = 256 - pan;
#endif

				LONG realvol;
				if (m_pConfig->getUseGlobalPreAmp()) {
					realvol = (pChn->nRealVolume * kChnMasterVol) >> 7;
				} else {
					//Extra attenuation required here if we're bypassing pre-amp.
					realvol = (pChn->nRealVolume * kChnMasterVol) >> 8;
				}
				
				if (m_pConfig->getForceSoftPanning() || gdwSoundSetup & SNDMIX_SOFTPANNING) {
					if (pan < 128) {
						pChn->nNewLeftVol = (realvol * pan) >> 8;
						pChn->nNewRightVol = (realvol * 128) >> 8;
					} else {
						pChn->nNewLeftVol = (realvol * 128) >> 8;
						pChn->nNewRightVol = (realvol * (256 - pan)) >> 8;
					}
				} else {
					pChn->nNewLeftVol = (realvol * pan) >> 8;
					pChn->nNewRightVol = (realvol * (256 - pan)) >> 8;
				}

			} else {
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
			if (m_pConfig->getUseGlobalPreAmp()) {
				pChn->nNewRightVol >>= MIXING_ATTENUATION;
				pChn->nNewLeftVol >>= MIXING_ATTENUATION;
			}

			pChn->nRightRamp = pChn->nLeftRamp = 0;
			// Dolby Pro-Logic Surround
			if ((pChn->dwFlags & CHN_SURROUND) && (gnChannels == 2)) pChn->nNewLeftVol = - pChn->nNewLeftVol;
			// Checking Ping-Pong Loops
			if (pChn->dwFlags & CHN_PINGPONGFLAG) pChn->nInc = -pChn->nInc;
			// Setting up volume ramp
			if ((pChn->dwFlags & CHN_VOLUMERAMP) // && gnVolumeRampSamples //rewbs: this allows us to use non ramping mix functions if ramping is 0
			 && ((pChn->nRightVol != pChn->nNewRightVol) || (pChn->nLeftVol != pChn->nNewLeftVol)))
			{
				LONG nRampLength = gnVolumeRampSamples;
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup"
				BOOL enableCustomRamp = pChn->pHeader && (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM));
				if(enableCustomRamp) nRampLength = pChn->pHeader->nVolRamp ? (gdwMixingFreq * pChn->pHeader->nVolRamp / 100000) : gnVolumeRampSamples;
				if(!nRampLength) nRampLength = 1;
// -! NEW_FEATURE#0027
				LONG nRightDelta = ((pChn->nNewRightVol - pChn->nRightVol) << VOLUMERAMPPRECISION);
				LONG nLeftDelta = ((pChn->nNewLeftVol - pChn->nLeftVol) << VOLUMERAMPPRECISION);
#ifndef FASTSOUNDLIB
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup "
//				if ((gdwSoundSetup & SNDMIX_DIRECTTODISK)
//				 || ((gdwSysInfo & (SYSMIX_ENABLEMMX|SYSMIX_FASTCPU))
//				  && (gdwSoundSetup & SNDMIX_HQRESAMPLER) && (gnCPUUsage <= 50)))
				if ((gdwSoundSetup & SNDMIX_DIRECTTODISK)
				 || ((gdwSysInfo & (SYSMIX_ENABLEMMX|SYSMIX_FASTCPU))
				  && (gdwSoundSetup & SNDMIX_HQRESAMPLER) && (gnCPUUsage <= 50) && !(enableCustomRamp && pChn->pHeader->nVolRamp)))
// -! NEW_FEATURE#0027
				{
					if ((pChn->nRightVol|pChn->nLeftVol) && (pChn->nNewRightVol|pChn->nNewLeftVol) && (!(pChn->dwFlags & CHN_FASTVOLRAMP)))
					{
						nRampLength = m_nBufferCount;
						if (nRampLength > (1 << (VOLUMERAMPPRECISION-1))) nRampLength = (1 << (VOLUMERAMPPRECISION-1));
						if (nRampLength < (LONG)gnVolumeRampSamples) nRampLength = gnVolumeRampSamples;
					}
				}
#endif
				pChn->nRightRamp = nRightDelta / nRampLength;
				pChn->nLeftRamp = nLeftDelta / nRampLength;
				pChn->nRightVol = pChn->nNewRightVol - ((pChn->nRightRamp * nRampLength) >> VOLUMERAMPPRECISION);
				pChn->nLeftVol = pChn->nNewLeftVol - ((pChn->nLeftRamp * nRampLength) >> VOLUMERAMPPRECISION);
				if (pChn->nRightRamp|pChn->nLeftRamp)
				{
					pChn->nRampLength = nRampLength;
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
			// Adding the channel in the channel list
			ChnMix[m_nMixChannels++] = nChn;
		} else
		{
#ifdef ENABLE_STEREOVU
			// Note change but no sample
			if (pChn->nLeftVU > 128) pChn->nLeftVU = 0;
			if (pChn->nRightVU > 128) pChn->nRightVU = 0;
#endif
			if (pChn->nVUMeter > 0xFF) pChn->nVUMeter = 0;
			pChn->nLeftVol = pChn->nRightVol = 0;
			pChn->nLength = 0;
		}
	}
done:
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


#ifdef MODPLUG_TRACKER

VOID CSoundFile::ProcessMidiOut(UINT nChn, MODCHANNEL *pChn)	//rewbs.VSTdelay: added arg
//----------------------------------------------------------
{
	// Do we need to process midi?
	// For now there is no difference between mute and sync mute with VSTis.
	if (pChn->dwFlags & (CHN_MUTE|CHN_SYNCMUTE)) return;
	if ((!m_nInstruments) || (m_nPattern >= Patterns.Size())
		 || (m_nRow >= PatternSize[m_nPattern]) || (!Patterns[m_nPattern])) return;
	
	const MODCOMMAND::NOTE note = pChn->nRowNote;
	const MODCOMMAND::INSTR instr = pChn->nRowInstr;
	const MODCOMMAND::VOL vol = pChn->nRowVolume;
	const MODCOMMAND::VOLCMD volcmd = pChn->nRowVolCmd;
	ASSERT(Patterns[m_nPattern].GetpModCommand(m_nRow, nChn)->note == NOTE_PC ||
		   Patterns[m_nPattern].GetpModCommand(m_nRow, nChn)->note == NOTE_PCS ||
           ( note == Patterns[m_nPattern].GetpModCommand(m_nRow, nChn)->note 
			 &&
			 instr == Patterns[m_nPattern].GetpModCommand(m_nRow, nChn)->instr
			 &&
             volcmd == Patterns[m_nPattern].GetpModCommand(m_nRow, nChn)->volcmd
			 &&
             vol == Patterns[m_nPattern].GetpModCommand(m_nRow, nChn)->vol )
		  );
	// Previously this function took modcommand directly from pattern. ASSERT is there
	// to detect possible change of behavior now that the data is accessed from 
	// channel rowdatas. There should not be any change.


	// Get instrument info and plugin reference
	INSTRUMENTHEADER *penv = pChn->pHeader;
	IMixPlugin *pPlugin = NULL;

	if ((instr) && (instr < MAX_INSTRUMENTS)) {
		penv = Headers[instr];
	}
	if ((penv) && (penv->nMidiChannel >= 1) && (penv->nMidiChannel <= 16)) {
		UINT nPlugin = GetBestPlugin(nChn, PRIORITISE_INSTRUMENT, RESPECT_MUTES);
		if ((nPlugin) && (nPlugin <= MAX_MIXPLUGINS)) {
			pPlugin = m_MixPlugins[nPlugin-1].pMixPlugin;
		}
	}

	//Do couldn't find a valid plugin
	if (pPlugin == NULL) return;

	if(GetModFlag(MSF_MIDICC_BUGEMULATION))
	{
		if (note) {
			pPlugin->MidiCommand(penv->nMidiChannel, penv->nMidiProgram, penv->wMidiBank, note, pChn->nVolume, nChn);
		} else if (volcmd == VOLCMD_VOLUME) {
			pPlugin->MidiCC(penv->nMidiChannel, MIDICC_Volume_Fine, vol, nChn);
		}
		return;
	}


	const bool hasVolCommand = (volcmd == VOLCMD_VOLUME);
	const UINT defaultVolume = penv->nGlobalVol;

	
	//If new note, determine notevelocity to use.
	if(note)
	{
		UINT velocity = 4*defaultVolume;
		switch(penv->nPluginVelocityHandling)
		{
			case PLUGIN_VELOCITYHANDLING_CHANNEL:
				velocity = pChn->nVolume;
			break;
		}

		pPlugin->MidiCommand(penv->nMidiChannel, penv->nMidiProgram, penv->wMidiBank, note, velocity, nChn);
	}

	
	const bool processVolumeAlsoOnNote = (penv->nPluginVelocityHandling == PLUGIN_VELOCITYHANDLING_VOLUME);

	if((hasVolCommand && !note) || (note && processVolumeAlsoOnNote))
	{
		switch(penv->nPluginVolumeHandling)
		{
			case PLUGIN_VOLUMEHANDLING_DRYWET:
				if(hasVolCommand) pPlugin->SetDryRatio(2*vol);
				else pPlugin->SetDryRatio(2*defaultVolume);
			break;

			case PLUGIN_VOLUMEHANDLING_MIDI:
				if(hasVolCommand) pPlugin->MidiCC(penv->nMidiChannel, MIDICC_Volume_Coarse, min(127, 2*vol), nChn);
				else pPlugin->MidiCC(penv->nMidiChannel, MIDICC_Volume_Coarse, min(127, 2*defaultVolume), nChn);
			break;
		}
	}
}

int CSoundFile::getVolEnvValueFromPosition(int position, INSTRUMENTHEADER* penv)
//------------------------------------------------------------------------------
{
	UINT pt = penv->nVolEnv - 1;

	//Checking where current 'tick' is relative to the 
	//envelope points.
	for (UINT i=0; i<(UINT)(penv->nVolEnv-1); i++)
	{
		if (position <= penv->VolPoints[i])
		{
			pt = i;
			break;
		}
	}

	int x2 = penv->VolPoints[pt];
	int x1, envvol;
	if (position >= x2) //Case: current 'tick' is on a envelope point.
	{
		envvol = penv->VolEnv[pt] << 2;
		x1 = x2;
	}
	else //Case: current 'tick' is between two envelope points.
	{
		if (pt)
		{
			envvol = penv->VolEnv[pt-1] << 2;
			x1 = penv->VolPoints[pt-1];
		} else
		{
			envvol = 0;
			x1 = 0;
		}

		if(x2 > x1 && position > x1)
		{
			//Linear approximation between the points;
			//f(x+d) ~ f(x) + f'(x)*d, where f'(x) = (y2-y1)/(x2-x1)
			envvol += ((position - x1) * (((int)penv->VolEnv[pt]<<2) - envvol)) / (x2 - x1);
		}
	}
	return envvol;
}

#endif


VOID CSoundFile::ApplyGlobalVolume(int SoundBuffer[], long lTotalSampleCount)
//---------------------------------------------------------------------------
{
		long delta=0;
		long step=0;

		if (m_nGlobalVolumeDestination != m_nGlobalVolume) { //user has provided new global volume
			m_nGlobalVolumeDestination = m_nGlobalVolume;
			m_nSamplesToGlobalVolRampDest = gnVolumeRampSamples;
		} 

		if (m_nSamplesToGlobalVolRampDest>0) {	// still some ramping left to do.
			long highResGlobalVolumeDestination = static_cast<long>(m_nGlobalVolumeDestination)<<VOLUMERAMPPRECISION;
			
			delta = highResGlobalVolumeDestination-m_lHighResRampingGlobalVolume;
			step = delta/static_cast<long>(m_nSamplesToGlobalVolRampDest);
			
			UINT maxStep = max(50, (10000/gnVolumeRampSamples+1)); //define max step size as some factor of user defined ramping value: the lower the value, the more likely the click.
			while (abs(step)>maxStep) {					 //if step is too big (might cause click), extend ramp length.
				m_nSamplesToGlobalVolRampDest += gnVolumeRampSamples;
				step = delta/static_cast<long>(m_nSamplesToGlobalVolRampDest);
			}
		}

		for (int pos=0; pos<lTotalSampleCount; pos++) {

			if (m_nSamplesToGlobalVolRampDest>0) { //ramping required
				m_lHighResRampingGlobalVolume += step;
				SoundBuffer[pos] = _muldiv(SoundBuffer[pos], m_lHighResRampingGlobalVolume, MAX_GLOBAL_VOLUME<<VOLUMERAMPPRECISION);
				m_nSamplesToGlobalVolRampDest--;
			} else {
				SoundBuffer[pos] = _muldiv(SoundBuffer[pos], m_nGlobalVolume, MAX_GLOBAL_VOLUME);
				m_lHighResRampingGlobalVolume = m_nGlobalVolume<<VOLUMERAMPPRECISION;
			}

		}
}

