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

#include "sndfile.h"
#include "MIDIEvents.h"
#include "tuning.h"
#include "Tables.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/TrackerSettings.h"
#endif

#ifdef MODPLUG_TRACKER
#define ENABLE_STEREOVU
#endif

// VU-Meter
#define VUMETER_DECAY		4

// Mixing data initialized in
MixerSettings CSoundFile::m_MixerSettings;
CResampler CSoundFile::m_Resampler;
#ifndef NO_REVERB
CReverb CSoundFile::m_Reverb;
#endif
#ifndef NO_DSP
CDSP CSoundFile::m_DSP;
#endif
#ifndef NO_EQ
CEQ CSoundFile::m_EQ;
#endif
#ifndef NO_AGC
CAGC CSoundFile::m_AGC;
#endif
UINT CSoundFile::gnVolumeRampUpSamplesActual = 42;		//default value
LPSNDMIXHOOKPROC CSoundFile::gpSndMixHook = NULL;
PMIXPLUGINCREATEPROC CSoundFile::gpMixPluginCreateProc = NULL;
LONG gnDryROfsVol = 0;
LONG gnDryLOfsVol = 0;
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

extern int MixSoundBuffer[MIXBUFFERSIZE * 4];
extern int MixRearBuffer[MIXBUFFERSIZE * 2];

#ifndef NO_REVERB
extern int MixReverbBuffer[MIXBUFFERSIZE * 2];
#endif

// Log tables for pre-amp
// Pre-amp (or more precisely: Pre-attenuation) depends on the number of channels,
// Which this table takes care of.
static const UINT PreAmpTable[16] =
{
	0x60, 0x60, 0x60, 0x70,	// 0-7
	0x80, 0x88, 0x90, 0x98,	// 8-15
	0xA0, 0xA4, 0xA8, 0xAC,	// 16-23
	0xB0, 0xB4, 0xB8, 0xBC,	// 24-31
};

#ifndef NO_AGC
static const UINT PreAmpAGCTable[16] =
{
	0x60, 0x60, 0x60, 0x64,
	0x68, 0x70, 0x78, 0x80,
	0x84, 0x88, 0x8C, 0x90,
	0x92, 0x94, 0x96, 0x98,
};
#endif

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



void CSoundFile::SetMixerSettings(const MixerSettings &mixersettings)
//-------------------------------------------------------------------
{

	// Start with ramping disabled to avoid clicks on first read.
	// Ramping is now set after the first read in CSoundFile::Read();
	gnVolumeRampUpSamplesActual = 0;
	m_MixerSettings = mixersettings;
}


void CSoundFile::SetResamplerSettings(const CResamplerSettings &resamplersettings)
//--------------------------------------------------------------------------------
{
	m_Resampler.m_Settings = resamplersettings;
}


BOOL CSoundFile::InitPlayer(BOOL bReset)
//--------------------------------------
{
	if (!gbInitTables)
	{
		m_Resampler.InitializeTables();
		gbInitTables = true;
	}

	gnDryROfsVol = gnDryLOfsVol = 0;
#ifndef NO_REVERB
	m_Reverb.gnRvbROfsVol = m_Reverb.gnRvbLOfsVol = 0;
#endif
#ifndef NO_REVERB
	m_Reverb.Initialize(bReset, m_MixerSettings.gdwMixingFreq);
#endif
#ifndef NO_DSP
	m_DSP.Initialize(bReset, m_MixerSettings.gdwMixingFreq, m_MixerSettings.gdwSoundSetup);
#endif
#ifndef NO_EQ
	m_EQ.Initialize(bReset, m_MixerSettings.gdwMixingFreq);
#endif
	return TRUE;
}


BOOL CSoundFile::FadeSong(UINT msec)
//----------------------------------
{
	samplecount_t nsamples = Util::muldiv(msec, m_MixerSettings.gdwMixingFreq, 1000);
	if (nsamples <= 0) return FALSE;
	if (nsamples > 0x100000) nsamples = 0x100000;
	m_nBufferCount = nsamples;
	int nRampLength = static_cast<int>(m_nBufferCount);
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
		pramp->dwFlags.set(CHN_VOLUMERAMP);
	}
	m_SongFlags.set(SONG_FADINGSONG);
	return TRUE;
}


BOOL CSoundFile::GlobalFadeSong(UINT msec)
//----------------------------------------
{
	if(m_SongFlags[SONG_GLOBALFADE]) return FALSE;
	m_nGlobalFadeMaxSamples = Util::muldiv(msec, m_MixerSettings.gdwMixingFreq, 1000);
	m_nGlobalFadeSamples = m_nGlobalFadeMaxSamples;
	m_SongFlags.set(SONG_GLOBALFADE);
	return TRUE;
}


UINT CSoundFile::Read(LPVOID lpDestBuffer, UINT count)
//-------------------------------------------------------
{
	LPBYTE lpBuffer = (LPBYTE)lpDestBuffer;
	LPCONVERTPROC pCvt = X86_Convert32To8;
	samplecount_t lMax, lCount, lSampleCount;
	size_t lSampleSize;
	UINT nStat = 0;
	UINT nMaxPlugins;

	nMaxPlugins = MAX_MIXPLUGINS;
	while ((nMaxPlugins > 0) && (!m_MixPlugins[nMaxPlugins-1].pMixPlugin)) nMaxPlugins--;
	m_nMixStat = 0;

	lSampleSize = m_MixerSettings.gnChannels;
	if(m_MixerSettings.gnBitsPerSample == 16) { lSampleSize *= 2; pCvt = X86_Convert32To16; }
	else if(m_MixerSettings.gnBitsPerSample == 24) { lSampleSize *= 3; pCvt = X86_Convert32To24; } 
	else if(m_MixerSettings.gnBitsPerSample == 32) { lSampleSize *= 4; pCvt = X86_Convert32To32; } 

	lMax = count;
	if ((!lMax) || (!lpBuffer) || (!m_nChannels)) return 0;
	samplecount_t lRead = lMax;
	if(m_SongFlags[SONG_ENDREACHED])
		goto MixDone;

	while (lRead > 0)
	{
		// Update Channel Data
		if (!m_nBufferCount)
		{

			if(m_SongFlags[SONG_FADINGSONG])
			{
				m_SongFlags.set(SONG_ENDREACHED);
				m_nBufferCount = lRead;
			} else

			if (ReadNote())
			{
				// Save pattern cue points for WAV rendering here (if we reached a new pattern, that is.)
				if(IsRenderingToDisc() && (m_PatternCuePoints.empty() || m_nCurrentOrder != m_PatternCuePoints.back().order))
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
					m_SongFlags.set(SONG_ENDREACHED);
					break;
				}
#endif // MODPLUG_TRACKER

				if (!FadeSong(FADESONGDELAY) || IsRenderingToDisc())	//rewbs: disable song fade when rendering.
				{
					m_SongFlags.set(SONG_ENDREACHED);
					if (lRead == lMax || IsRenderingToDisc())		//rewbs: don't complete buffer when rendering
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
		m_Reverb.gnReverbSend = 0;
#endif // NO_REVERB

		// Resetting sound buffer
		X86_StereoFill(MixSoundBuffer, lSampleCount, &gnDryROfsVol, &gnDryLOfsVol);
		
		ASSERT(lCount<=MIXBUFFERSIZE);		// ensure MIXBUFFERSIZE really is our max buffer size
		if (m_MixerSettings.gnChannels >= 2)
		{
			lSampleCount *= 2;
			m_nMixStat += CreateStereoMix(lCount);

#ifndef NO_REVERB
			m_Reverb.Process(MixSoundBuffer, MixReverbBuffer, lCount, gdwSysInfo);
#endif // NO_REVERB

			if (nMaxPlugins) ProcessPlugins(lCount);

			// Apply global volume
			if (m_pConfig->getGlobalVolumeAppliesToMaster())
			{
				ApplyGlobalVolume(MixSoundBuffer, MixRearBuffer, lSampleCount);
			}
		} else
		{
			m_nMixStat += CreateStereoMix(lCount);

#ifndef NO_REVERB
			m_Reverb.Process(MixSoundBuffer, MixReverbBuffer, lCount, gdwSysInfo);
#endif // NO_REVERB

			if (nMaxPlugins) ProcessPlugins(lCount);
			X86_MonoFromStereo(MixSoundBuffer, lCount);

			// Apply global volume
			if (m_pConfig->getGlobalVolumeAppliesToMaster())
			{
				ApplyGlobalVolume(MixSoundBuffer, nullptr, lSampleCount);
			}
		}

#ifndef NO_DSP
		m_DSP.Process(MixSoundBuffer, MixRearBuffer, lCount, m_MixerSettings.gdwSoundSetup, m_MixerSettings.gnChannels);
#endif

#ifndef NO_EQ
		// Graphic Equalizer
		if (m_MixerSettings.gdwSoundSetup & SNDMIX_EQ)
		{
			if (m_MixerSettings.gnChannels >= 2)
				m_EQ.ProcessStereo(MixSoundBuffer, lCount, m_pConfig, m_MixerSettings.gdwSoundSetup, gdwSysInfo);
			else
				m_EQ.ProcessMono(MixSoundBuffer, lCount, m_pConfig);
		}
#endif // NO_EQ

		nStat++;

#ifndef NO_AGC
		// Automatic Gain Control
		if (m_MixerSettings.gdwSoundSetup & SNDMIX_AGC) m_AGC.Process(MixSoundBuffer, lSampleCount, m_MixerSettings.gdwMixingFreq, m_MixerSettings.gnChannels);
#endif // NO_AGC

		UINT lTotalSampleCount = lSampleCount;	// Including rear channels

		// Multichannel
		if (m_MixerSettings.gnChannels > 2)
		{
			X86_InterleaveFrontRear(MixSoundBuffer, MixRearBuffer, lSampleCount);
			lTotalSampleCount *= 2;
		}

		// Noise Shaping
		if (m_MixerSettings.gnBitsPerSample <= 16)
		{
			if(m_Resampler.IsHQ())
				X86_Dither(MixSoundBuffer, lTotalSampleCount, m_MixerSettings.gnBitsPerSample);
		}

		// Hook Function
		if (gpSndMixHook)
		{
			//Currently only used for VU Meter, so it's OK to do it after global Vol.
			gpSndMixHook(MixSoundBuffer, lTotalSampleCount, m_MixerSettings.gnChannels);
		}

		// Perform clipping
		lpBuffer += pCvt(lpBuffer, MixSoundBuffer, lTotalSampleCount);

		// Buffer ready
		lRead -= lCount;
		m_nBufferCount -= lCount;
		m_lTotalSampleCount += lCount;		// increase sample count for VSTTimeInfo.
		// Turn on ramping after first read (fix http://forum.openmpt.org/index.php?topic=523.0 )
		gnVolumeRampUpSamplesActual = m_MixerSettings.glVolumeRampUpSamples;
	}
MixDone:
	if (lRead) memset(lpBuffer, (m_MixerSettings.gnBitsPerSample == 8) ? 0x80 : 0, lRead * lSampleSize);
	if (nStat) { m_nMixStat += nStat-1; m_nMixStat /= nStat; }
	return lMax - lRead;
}


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
		if(m_nCurrentOrder != m_nNextOrder) 
			m_nCurrentOrder = m_nNextOrder;

#ifdef MODPLUG_TRACKER
		// "Lock order" editing feature
		if(Order.IsPositionLocked(m_nCurrentOrder) && !IsRenderingToDisc())
		{
			m_nCurrentOrder = m_lockOrderStart;
		}
#endif // MODPLUG_TRACKER

		// Check if pattern is valid
		if(!m_SongFlags[SONG_PATTERNLOOP])
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
					if(!(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_RESETCHANNELS))
#endif // MODPLUG_TRACKER
					{
						m_SongFlags.set(SONG_BREAKTOROW);
					}

					if (!nRestartPosOverride && !m_SongFlags[SONG_BREAKTOROW])
					{
						//rewbs.instroVSTi: stop all VSTi at end of song, if looping.
						StopAllVsti();
						m_nMusicSpeed = m_nDefaultSpeed;
						m_nMusicTempo = m_nDefaultTempo;
						m_nGlobalVolume = m_nDefaultGlobalVolume;
						for (UINT i=0; i<MAX_CHANNELS; i++)
						{
							Chn[i].dwFlags.set(CHN_NOTEFADE | CHN_KEYOFF);
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
					//if (m_nRepeatCount > 0) m_nRepeatCount--;
					m_nCurrentOrder = nRestartPosOverride;
					m_SongFlags.reset(SONG_BREAKTOROW);
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
						visitedSongRows.Initialize(true);
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
		const bool overrideLoopCheck = (m_nRepeatCount != -1) && m_SongFlags[SONG_PATTERNLOOP];
		if(!overrideLoopCheck && visitedSongRows.IsVisited(m_nCurrentOrder, m_nRow, true))
		{
			if(m_nRepeatCount)
			{
				// repeat count == -1 means repeat infinitely.
				if(m_nRepeatCount > 0)
				{
					m_nRepeatCount--;
				}
				// Forget all but the current row.
				visitedSongRows.Initialize(true);
				visitedSongRows.Visit(m_nCurrentOrder, m_nRow);
			} else
			{
#ifdef MODPLUG_TRACKER
				// Let's check again if this really is the end of the song.
				// The visited rows vector might have been screwed up while editing...
				// This is of course not possible during rendering to WAV, so we ignore that case.
				GetLengthType t = GetLength(eNoAdjust);
				if(IsRenderingToDisc() || (t.lastOrder == m_nCurrentOrder && t.lastRow == m_nRow))
#else
				if(1)
#endif // MODPLUG_TRACKER
				{
					// This is really the song's end!
					visitedSongRows.Initialize(true);
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
			if (!m_SongFlags[SONG_PATTERNLOOP]) m_nNextOrder = m_nCurrentOrder + 1;
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
			pChn->dwFlags.reset(CHN_PORTAMENTO | CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO);
			pChn->nCommand = CMD_NONE;
			pChn->m_plugParamValueStep = 0;
		}

		// Now that we know which pattern we're on, we can update time signatures (global or pattern-specific)
		UpdateTimeSignature();

	}
	// Should we process tick0 effects?
	if (!m_nMusicSpeed) m_nMusicSpeed = 1;
	m_SongFlags.set(SONG_FIRSTTICK);

	//End of row? stop pattern step (aka "play row").
#ifdef MODPLUG_TRACKER
	if (m_nTickCount >= GetNumTicksOnCurrentRow() - 1)
	{
		if(m_SongFlags[SONG_STEP])
		{
			m_SongFlags.reset(SONG_STEP);
			m_SongFlags.set(SONG_PAUSED);
		}
	}
#endif // MODPLUG_TRACKER

	if (m_nTickCount)
	{
		m_SongFlags.reset(SONG_FIRSTTICK);
		if(!(GetType() & MOD_TYPE_XM) && m_nTickCount < GetNumTicksOnCurrentRow())
		{
			// Emulate first tick behaviour if Row Delay is set.
			// Test cases: PatternDelaysRetrig.it, PatternDelaysRetrig.s3m, PatternDelaysRetrig.xm
			if(!(m_nTickCount % (m_nMusicSpeed + m_nFrameDelay)))
			{
				m_SongFlags.set(SONG_FIRSTTICK);
			}
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
	if (pChn->dwFlags[CHN_TREMOLO])
	{
		if(m_SongFlags.test_all(SONG_FIRSTTICK | SONG_PT1XMODE))
		{
			// ProTracker doesn't apply tremolo nor advance on the first tick.
			// Test case: VibratoReset.mod
			return;
		}

		UINT trempos = pChn->nTremoloPos;
		// IT compatibility: Why would you not want to execute tremolo at volume 0?
		if(vol > 0 || IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			// IT compatibility: We don't need a different attenuation here because of the different tables we're going to use
			const int tremattn = ((GetType() & MOD_TYPE_XM) || IsCompatibleMode(TRK_IMPULSETRACKER)) ? 5 : 6;

			vol += (GetVibratoDelta(pChn->nTremoloType, trempos) * (int)pChn->nTremoloDepth) >> tremattn;
		}
		if(!m_SongFlags[SONG_FIRSTTICK] || ((GetType() & (MOD_TYPE_STM|MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)) && !m_SongFlags[SONG_ITOLDEFFECTS]))
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
	if(IsCompatibleMode(TRK_FASTTRACKER2))
	{
		// FT2 Compatibility: Weird XM tremor.
		// Test case: Tremor.xm
		if(pChn->nTremorCount & 0x80)
		{
			if(!m_SongFlags[SONG_FIRSTTICK] && pChn->nCommand == CMD_TREMOR)
			{
				pChn->nTremorCount &= ~0x20;
				if(pChn->nTremorCount == 0x80)
				{
					// Reached end of off-time
					pChn->nTremorCount = (pChn->nTremorParam >> 4) | 0xC0;
				} else if(pChn->nTremorCount == 0xC0)
				{
					// Reached end of on-time
					pChn->nTremorCount = (pChn->nTremorParam & 0x0F) | 0x80;
				} else
				{
					pChn->nTremorCount--;
				}
				
				pChn->dwFlags.set(CHN_FASTVOLRAMP);
			}

			if((pChn->nTremorCount & 0xE0) == 0x80)
			{
				vol = 0;
			}
		}
	} else if(pChn->nCommand == CMD_TREMOR)
	{
		// IT compatibility 12. / 13.: Tremor
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			if((pChn->nTremorCount & 0x80) && pChn->nLength)
			{
				if (pChn->nTremorCount == 0x80)
					pChn->nTremorCount = (pChn->nTremorParam >> 4) | 0xC0;
				else if (pChn->nTremorCount == 0xC0)
					pChn->nTremorCount = (pChn->nTremorParam & 0x0F) | 0x80;
				else
					pChn->nTremorCount--;
			}

			if((pChn->nTremorCount & 0xC0) == 0x80)
				vol = 0;
		} else
		{
			UINT ontime = pChn->nTremorParam >> 4;
			UINT n = ontime + (pChn->nTremorParam & 0x0F);	// Total tremor cycle time (On + Off)
			if ((!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) || m_SongFlags[SONG_ITOLDEFFECTS])
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
				if(m_SongFlags[SONG_FIRSTTICK])
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
		pChn->dwFlags.set(CHN_FASTVOLRAMP);
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
	// Test cases: s77.it, EnvLoops.xm
	return ((pChn->GetEnvelope(env).flags[ENV_ENABLED] || (insEnv.dwFlags[ENV_ENABLED] && IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2)))
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
		// Get values in [0, 256]
		int envval = Util::Round<int>(pIns->VolEnv.GetValueFromPosition(envpos) * 256.0f);

		// if we are in the release portion of the envelope,
		// rescale envelope factor so that it is proportional to the release point
		// and release envelope beginning.
		if(pIns->VolEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET
			&& envpos >= pIns->VolEnv.Ticks[pIns->VolEnv.nReleaseNode]
			&& pChn->VolEnv.nEnvValueAtReleaseJump != NOT_YET_RELEASED)
		{
			int envValueAtReleaseJump = pChn->VolEnv.nEnvValueAtReleaseJump;
			int envValueAtReleaseNode = pIns->VolEnv.Values[pIns->VolEnv.nReleaseNode] * 4;

			//If we have just hit the release node, force the current env value
			//to be that of the release node. This works around the case where 
			// we have another node at the same position as the release node.
			if(envpos == pIns->VolEnv.Ticks[pIns->VolEnv.nReleaseNode])
				envval = envValueAtReleaseNode;

			int relativeVolumeChange = (envval - envValueAtReleaseNode) * 2;
			envval = envValueAtReleaseJump + relativeVolumeChange;
		}
		vol = (vol * Clamp(envval, 0, 512)) >> 8;
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
		// Get values in [-32, 32]
		const int envval = Util::Round<int>((pIns->PanEnv.GetValueFromPosition(envpos) - 0.5f) * 64.0f);

		int pan = pChn->nPan;
		if(pan >= 128)
		{
			pan += (envval * (256 - pan)) / 32;
		} else
		{
			pan += (envval * (pan)) / 32;
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

		const int envpos = pChn->PitchEnv.nEnvPosition - (IsCompatibleMode(TRK_IMPULSETRACKER) ? 1 : 0);
		// Get values in [-256, 256]
		const int envval = Util::Round<int>((pIns->PitchEnv.GetValueFromPosition(envpos) - 0.5f) * 512.0f);

		if(pChn->PitchEnv.flags[ENV_FILTER])
		{
			// Filter Envelope: controls cutoff frequency
			SetupChannelFilter(pChn, !pChn->dwFlags[CHN_FILTER], envval);
		} else
		{
			// Pitch Envelope
			if(GetType() == MOD_TYPE_MPT && pChn->pModInstrument && pChn->pModInstrument->pTuning)
			{
				if(pChn->nFineTune != envval)
				{
					pChn->nFineTune = envval;
					pChn->m_CalculateFreq = true;
					//Preliminary tests indicated that this behavior
					//is very close to original(with 12TET) when finestep count
					//is 15.
				}
			} else //Original behavior
			{
				int l = envval;
				if(l < 0)
				{
					l = -l;
					LimitMax(l, 255);
					period = Util::muldiv(period, LinearSlideUpTable[l], 0x10000);
				} else
				{
					LimitMax(l, 255);
					period = Util::muldiv(period, LinearSlideDownTable[l], 0x10000);
				}
			} //End: Original behavior.
		}
	}
}


void CSoundFile::IncrementEnvelopePosition(ModChannel *pChn, enmEnvelopeTypes envType)
//------------------------------------------------------------------------------------
{
	ModChannel::EnvInfo &chnEnv = pChn->GetEnvelope(envType);

	if(pChn->pModInstrument == nullptr || !chnEnv.flags[ENV_ENABLED])
	{
		return;
	}

	// Increase position
	uint32 position = chnEnv.nEnvPosition + (IsCompatibleMode(TRK_IMPULSETRACKER) ? 0 : 1);

	const InstrumentEnvelope &insEnv = pChn->pModInstrument->GetEnvelope(envType);

	bool endReached = false;

	if(!IsCompatibleMode(TRK_IMPULSETRACKER))
	{
		// FT2-style envelope processing.
		if(insEnv.dwFlags[ENV_LOOP])
		{
			// Normal loop active
			uint32 end = insEnv.Ticks[insEnv.nLoopEnd];
			if(GetType() != MOD_TYPE_XM) end++;

			// FT2 compatibility: If the sustain point is at the loop end and the sustain loop has been released, don't loop anymore.
			// Test case: EnvLoops.xm
			const bool escapeLoop = (insEnv.nLoopEnd == insEnv.nSustainEnd && insEnv.dwFlags[ENV_SUSTAIN] && pChn->dwFlags[CHN_KEYOFF] && IsCompatibleMode(TRK_FASTTRACKER2));

			if(position == end && !escapeLoop)
			{
				position = insEnv.Ticks[insEnv.nLoopStart];

				if(envType == ENV_VOLUME && insEnv.nLoopStart == insEnv.nLoopEnd && insEnv.Values[insEnv.nLoopEnd] == 0
					&& (!(GetType() & MOD_TYPE_XM) || (insEnv.nLoopEnd + 1u == insEnv.nNodes)))
				{
					// Stop channel if the envelope loop only covers the last silent envelope point.
					// Can't see a point in this, and it breaks doommix3.xm if you allow loading one-point loops, so disabling it for now.
					//pChn->dwFlags |= CHN_NOTEFADE;
					//pChn->nFadeOutVol = 0;
				}
			}
		}

		if(insEnv.dwFlags[ENV_SUSTAIN] && !pChn->dwFlags[CHN_KEYOFF])
		{
			// Envelope sustained
			if(position == insEnv.Ticks[insEnv.nSustainEnd] + 1u)
			{
				position = insEnv.Ticks[insEnv.nSustainStart];
			}
		} else
		{
			// Limit to last envelope point
			if(position > insEnv.Ticks[insEnv.nNodes - 1])
			{
				// Env of envelope
				position = insEnv.Ticks[insEnv.nNodes - 1];
				endReached = true;
			}
		}
	} else
	{
		// IT envelope processing.
		// Test case: EnvLoops.it
		uint32 start, end;

		if(insEnv.dwFlags[ENV_SUSTAIN] && !pChn->dwFlags[CHN_KEYOFF])
		{
			// Envelope sustained
			start = insEnv.Ticks[insEnv.nSustainStart];
			end = insEnv.Ticks[insEnv.nSustainEnd] + 1;
		} else if(insEnv.dwFlags[ENV_LOOP])
		{
			// Normal loop active
			start = insEnv.Ticks[insEnv.nLoopStart];
			end = insEnv.Ticks[insEnv.nLoopEnd] + 1;
		} else
		{
			// Limit to last envelope point
			start = end = insEnv.Ticks[insEnv.nNodes - 1];
			if(position > end)
			{
				// Env of envelope
				endReached = true;
			}
		}

		if(position >= end)
		{
			position = start;
		}
	}

	if(envType == ENV_VOLUME && endReached)
	{
		// Special handling for volume envelopes at end of envelope
		if((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || pChn->dwFlags[CHN_KEYOFF])
		{
			pChn->dwFlags.set(CHN_NOTEFADE);
		}

		if(insEnv.Values[insEnv.nNodes - 1] == 0 && (pChn->nMasterChn > 0 || (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))))
		{
			// Stop channel if the last envelope node is silent anyway.
			pChn->dwFlags.set(CHN_NOTEFADE);
			pChn->nFadeOutVol = 0;
			pChn->nRealVolume = 0;
			pChn->nCalcVolume = 0;
		}
	}

	chnEnv.nEnvPosition = position + (IsCompatibleMode(TRK_IMPULSETRACKER) ? 1 : 0);

}


void CSoundFile::IncrementEnvelopePositions(ModChannel *pChn)
//-----------------------------------------------------------
{
	IncrementEnvelopePosition(pChn, ENV_VOLUME);
	IncrementEnvelopePosition(pChn, ENV_PANNING);
	IncrementEnvelopePosition(pChn, ENV_PITCH);
}


void CSoundFile::ProcessInstrumentFade(ModChannel *pChn, int &vol)
//----------------------------------------------------------------
{
	// FadeOut volume
	if(pChn->dwFlags[CHN_NOTEFADE])
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
	if(pChn->dwFlags[CHN_PANBRELLO])
	{
		UINT panpos;
		// IT compatibility: IT has its own, more precise tables
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
			panpos = pChn->nPanbrelloPos & 0xFF;
		else
			panpos = ((pChn->nPanbrelloPos + 0x10) >> 2) & 0x3F;

		int pdelta = GetVibratoDelta(pChn->nPanbrelloType, panpos);

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
		// Note: We could use pChn->nLastNote here to simplify things.
		if(pChn->pModInstrument && pChn->pModInstrument->nMixPlug && !m_SongFlags[SONG_FIRSTTICK])
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

		if((GetType() & MOD_TYPE_MPT) && pChn->pModInstrument && pChn->pModInstrument->pTuning)
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
				// Pattern delay restarts tick counting. Not quite correct yet!
				const UINT tick = m_nTickCount % (m_nMusicSpeed + m_nFrameDelay);
				if(pChn->nArpeggio >> 4 != 0 || (pChn->nArpeggio & 0x0F) != 0)
				{
					switch(tick % 3)
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

				if(!m_SongFlags[SONG_FIRSTTICK])
				{
					arpPos = ((int)m_nMusicSpeed - (int)m_nTickCount) % 3;
					if((m_nMusicSpeed > 18) && (m_nMusicSpeed - m_nTickCount > 16)) arpPos = 2; // swedish tracker logic, I love it
					switch(arpPos)
					{
					case 1:	note += (pChn->nArpeggio >> 4); break;
					case 2:	note += (pChn->nArpeggio & 0x0F); break;
					}
				}

				if (note > 108 + NOTE_MIN && arpPos != 0)
					note = 108 + NOTE_MIN; // FT2's note limit

				period = GetPeriodFromNote(note, pChn->nFineTune, pChn->nC5Speed);

			}
			// Other trackers
			else
			{
				int note = pChn->nNote;
				switch(m_nTickCount % 3)
				{
				case 1: note += (pChn->nArpeggio >> 4); break;
				case 2: note += (pChn->nArpeggio & 0x0F); break;
				}
				if(note != pChn->nNote)
				{
					if(m_SongFlags[SONG_PT1XMODE] && note >= NOTE_MIDDLEC + 24)
					{
						// Weird arpeggio wrap-around in ProTracker.
						// Test case: ArpWraparound.mod, and the snare sound in "Jim is dead" by doh.
						note -= 37;
					}
					period = GetPeriodFromNote(note, pChn->nFineTune, pChn->nC5Speed);
				}
			}
		}
	}
}


void CSoundFile::ProcessVibrato(CHANNELINDEX nChn, int &period, CTuning::RATIOTYPE &vibratoFactor)
//-----------------------------------------------------------------------------------------------
{
	ModChannel &chn = Chn[nChn];

	if(chn.dwFlags[CHN_VIBRATO])
	{
		UINT vibpos = chn.nVibratoPos;

		int vdelta = GetVibratoDelta(chn.nVibratoType, vibpos);

		if(GetType() == MOD_TYPE_MPT && chn.pModInstrument && chn.pModInstrument->pTuning)
		{
			//Hack implementation: Scaling vibratofactor to [0.95; 1.05]
			//using figure from above tables and vibratodepth parameter
			vibratoFactor += 0.05F * vdelta * chn.m_VibratoDepth / 128.0F;
			chn.m_CalculateFreq = true;
			chn.m_ReCalculateFreqOnFirstTick = false;

			if(m_nTickCount + 1 == m_nMusicSpeed)
				chn.m_ReCalculateFreqOnFirstTick = true;
		} else
		{
			// Original behaviour

			if(m_SongFlags.test_all(SONG_FIRSTTICK | SONG_PT1XMODE))
			{
				// ProTracker doesn't apply vibrato nor advance on the first tick.
				// Test case: VibratoReset.mod
				return;
			} else if((GetType() & MOD_TYPE_XM) && (chn.nVibratoType & 0x03) == 1)
			{
				// FT2 compatibility: Vibrato ramp down table is upside down.
				// Test case: VibratoWaveforms.xm
				vdelta = -vdelta;
			}

			UINT vdepth;
			// IT compatibility: correct vibrato depth
			if(IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				// Yes, vibrato goes backwards with old effects enabled!
				if(m_SongFlags[SONG_ITOLDEFFECTS])
				{
					// Test case: vibrato-oldfx.it
					vdepth = 5;
				} else
				{
					// Test case: vibrato.it
					vdepth = 6;
					vdelta = -vdelta;
				}
			} else
			{
				vdepth = ((!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) || m_SongFlags[SONG_ITOLDEFFECTS]) ? 6 : 7;
			}

			vdelta = (vdelta * (int)chn.nVibratoDepth) >> vdepth;
			int16 midiDelta = static_cast<int16>(-vdelta);	// Periods are upside down

			if (m_SongFlags[SONG_LINEARSLIDES] && (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)))
			{
				int l = vdelta;
				if (l < 0)
				{
					l = -l;
					vdelta = Util::muldiv(period, LinearSlideDownTable[l >> 2], 0x10000) - period;
					if (l & 0x03) vdelta += Util::muldiv(period, FineLinearSlideDownTable[l & 0x03], 0x10000) - period;
				} else
				{
					vdelta = Util::muldiv(period, LinearSlideUpTable[l >> 2], 0x10000) - period;
					if (l & 0x03) vdelta += Util::muldiv(period, FineLinearSlideUpTable[l & 0x03], 0x10000) - period;
				}
			}
			period += vdelta;

			// Process MIDI vibrato for plugins:
			IMixPlugin *plugin = GetChannelInstrumentPlugin(nChn);
			if(plugin != nullptr)
			{
				// If the Pitch Wheel Depth is configured correctly (so it's the same as the plugin's PWD),
				// MIDI vibrato will sound identical to vibrato with linear slides enabled.
				int8 pwd = 2;
				if(chn.pModInstrument != nullptr)
				{
					pwd = chn.pModInstrument->midiPWD;
				}
				plugin->MidiVibrato(GetBestMidiChannel(nChn), midiDelta, pwd);
			}
		}

		// Advance vibrato position - IT updates on every tick, unless "old effects" are enabled (in this case it only updates on non-first ticks like other trackers)
		if(!m_SongFlags[SONG_FIRSTTICK] || ((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && !(m_SongFlags[SONG_ITOLDEFFECTS])))
		{
			// IT compatibility: IT has its own, more precise tables
			if(IsCompatibleMode(TRK_IMPULSETRACKER))
				chn.nVibratoPos = (vibpos + 4 * chn.nVibratoSpeed) & 0xFF;
			else
				chn.nVibratoPos = (vibpos + chn.nVibratoSpeed) & 0x3F;
		}
	} else if(chn.dwOldFlags & CHN_VIBRATO)
	{
		// Stop MIDI vibrato for plugins:
		IMixPlugin *plugin = GetChannelInstrumentPlugin(nChn);
		if(plugin != nullptr)
		{
			plugin->MidiVibrato(GetBestMidiChannel(nChn), 0, 0);
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
				vdelta = Util::muldiv(period, LinearSlideDownTable[l >> 2], 0x10000) - period;
				if (l & 0x03)
				{
					vdelta += Util::muldiv(period, FineLinearSlideDownTable[l & 0x03], 0x10000) - period;
				}
			} else
			{
				vdelta = Util::muldiv(period, LinearSlideUpTable[l >> 2], 0x10000) - period;
				if (l & 0x03)
				{
					vdelta += Util::muldiv(period, FineLinearSlideUpTable[l & 0x03], 0x10000) - period;
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
					if(!pChn->dwFlags[CHN_KEYOFF])
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
					period = Util::muldiv(period, df1 + ((df2 - df1) * (n & 0x3F) >> 6), 256);
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
	if(pChn->dwFlags[CHN_VOLUMERAMP] && (pChn->nRightVol != pChn->nNewRightVol || pChn->nLeftVol != pChn->nNewLeftVol))
	{
		const bool rampUp = (pChn->nNewRightVol > pChn->nRightVol) || (pChn->nNewLeftVol > pChn->nLeftVol);
		LONG rampLength, globalRampLength, instrRampLength = 0;
		rampLength = globalRampLength = (rampUp ? gnVolumeRampUpSamplesActual : m_MixerSettings.glVolumeRampDownSamples);
		//XXXih: add real support for bidi ramping here	
		
		if(pChn->pModInstrument != nullptr && rampUp)
		{
			instrRampLength = pChn->pModInstrument->nVolRampUp;
			rampLength = instrRampLength ? (m_MixerSettings.gdwMixingFreq * instrRampLength / 100000) : globalRampLength;
		}
		const bool enableCustomRamp = (instrRampLength > 0);

		if(!rampLength)
		{
			rampLength = 1;
		}

		LONG nRightDelta = ((pChn->nNewRightVol - pChn->nRightVol) << VOLUMERAMPPRECISION);
		LONG nLeftDelta = ((pChn->nNewLeftVol - pChn->nLeftVol) << VOLUMERAMPPRECISION);
//		if (IsRenderingToDisc()
//			|| m_Resampler.IsHQ())
		if(IsRenderingToDisc() || (m_Resampler.IsHQ() && !enableCustomRamp))
		{
			if((pChn->nRightVol | pChn->nLeftVol) && (pChn->nNewRightVol | pChn->nNewLeftVol) && !pChn->dwFlags[CHN_FASTVOLRAMP])
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

		pChn->nRightRamp = nRightDelta / rampLength;
		pChn->nLeftRamp = nLeftDelta / rampLength;
		pChn->nRightVol = pChn->nNewRightVol - ((pChn->nRightRamp * rampLength) >> VOLUMERAMPPRECISION);
		pChn->nLeftVol = pChn->nNewLeftVol - ((pChn->nLeftRamp * rampLength) >> VOLUMERAMPPRECISION);

		if (pChn->nRightRamp|pChn->nLeftRamp)
		{
			pChn->nRampLength = rampLength;
		} else
		{
			pChn->dwFlags.reset(CHN_VOLUMERAMP);
			pChn->nRightVol = pChn->nNewRightVol;
			pChn->nLeftVol = pChn->nNewLeftVol;
		}
	} else
	{
		pChn->dwFlags.reset(CHN_VOLUMERAMP);
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
	if(m_SongFlags[SONG_PAUSED])
	{
		m_nTickCount = 0;
		if (!m_nMusicSpeed) m_nMusicSpeed = 6;
		if (!m_nMusicTempo) m_nMusicTempo = 125;
	} else
#endif // MODPLUG_TRACKER
	{
		if(!ProcessRow())
			return FALSE;
	}
	////////////////////////////////////////////////////////////////////////////////////
	if (!m_nMusicTempo) return FALSE;

	m_nSamplesPerTick = m_nBufferCount = GetTickDuration(m_nMusicTempo, m_nMusicSpeed, m_nCurrentRowsPerBeat);

	// Master Volume + Pre-Amplification / Attenuation setup
	DWORD nMasterVol;
	{
		CHANNELINDEX nchn32 = Clamp(m_nChannels, CHANNELINDEX(1), CHANNELINDEX(31));
		
		DWORD mastervol;

		if (m_pConfig->getUseGlobalPreAmp())
		{
			int realmastervol = m_nMasterVolume;
			if (realmastervol > 0x80)
			{
				//Attenuate global pre-amp depending on num channels
				realmastervol = 0x80 + ((realmastervol - 0x80) * (nchn32 + 4)) / 16;
			}
			mastervol = (realmastervol * (m_nSamplePreAmp)) >> 6;
		} else
		{
			//Preferred option: don't use global pre-amp at all.
			mastervol = m_nSamplePreAmp;
		}

		if(m_SongFlags[SONG_GLOBALFADE] && m_nGlobalFadeMaxSamples != 0)
		{
			mastervol = Util::muldiv(mastervol, m_nGlobalFadeSamples, m_nGlobalFadeMaxSamples);
		}

		if (m_pConfig->getUseGlobalPreAmp())
		{
			UINT attenuation =
#ifndef NO_AGC
				(m_MixerSettings.gdwSoundSetup & SNDMIX_AGC) ? PreAmpAGCTable[nchn32 >> 1] :
#endif
				PreAmpTable[nchn32 >> 1];
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
		// FT2 Compatibility: Prevent notes to be stopped after a fadeout. This way, a portamento effect can pick up a faded instrument which is long enough.
		// This occours for example in the bassline (channel 11) of jt_burn.xm. I hope this won't break anything else...
		// I also suppose this could decrease mixing performance a bit, but hey, which CPU can't handle 32 muted channels these days... :-)
		if(pChn->dwFlags[CHN_NOTEFADE] && (!(pChn->nFadeOutVol|pChn->nRightVol|pChn->nLeftVol)) && (!IsCompatibleMode(TRK_FASTTRACKER2)))
		{
			pChn->nLength = 0;
			pChn->nROfs = pChn->nLOfs = 0;
		}
		// Check for unused channel
		if(pChn->dwFlags[CHN_MUTE] || (nChn >= m_nChannels && !pChn->nLength))
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
			int insVol = pChn->nInsVol;		// This is the "SV * IV" value in ITTECH.TXT

			ProcessVolumeSwing(pChn, IsCompatibleMode(TRK_IMPULSETRACKER) ? insVol : vol);
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
					// In IT and FT2 compatible mode, envelope position indices are shifted by one for proper envelope pausing,
					// so we have to update the position before we actually process the envelopes.
					// When using MPT behaviour, we get the envelope position for the next tick while we are still calculating the current tick,
					// which then results in wrong position information when the envelope is paused on the next row.
					// Test cases: s77.it, EnvLoops.xm
					IncrementEnvelopePositions(pChn);
				}
				ProcessVolumeEnvelope(pChn, vol);
				ProcessInstrumentFade(pChn, vol);
				ProcessPanningEnvelope(pChn);
				ProcessPitchPanSeparation(pChn);
			} else
			{
				// No Envelope: key off => note cut
				if(pChn->dwFlags[CHN_NOTEFADE]) // 1.41-: CHN_KEYOFF|CHN_NOTEFADE
				{
					pChn->nFadeOutVol = 0;
					vol = 0;
				}
			}
			// vol is 14-bits
			if (vol)
			{
				// IMPORTANT: pChn->nRealVolume is 14 bits !!!
				// -> Util::muldiv( 14+8, 6+6, 18); => RealVolume: 14-bit result (22+12-20)
				
				if(pChn->dwFlags[CHN_SYNCMUTE])
				{
					pChn->nRealVolume = 0;
				} else if (m_pConfig->getGlobalVolumeAppliesToMaster())
				{
					// Don't let global volume affect level of sample if
					// Global volume is going to be applied to master output anyway.
					pChn->nRealVolume = Util::muldiv(vol * MAX_GLOBAL_VOLUME, pChn->nGlobalVol * insVol, 1 << 20);
				} else
				{
					pChn->nRealVolume = Util::muldiv(vol * m_nGlobalVolume, pChn->nGlobalVol * insVol, 1 << 20);
				}
			}

			pChn->nCalcVolume = vol;	// Update calculated volume for MIDI macros

			if (pChn->nPeriod < m_nMinPeriod) pChn->nPeriod = m_nMinPeriod;
			period = pChn->nPeriod;
			// TODO Glissando effect is reset after portamento! What would this sound like without the CHN_PORTAMENTO flag?
			if((pChn->dwFlags & (CHN_GLISSANDO | CHN_PORTAMENTO)) == (CHN_GLISSANDO | CHN_PORTAMENTO))
			{
				period = GetPeriodFromNote(GetNoteFromPeriod(period), pChn->nFineTune, pChn->nC5Speed);
			}

			ProcessArpeggio(pChn, period, arpeggioSteps);

			// Preserve Amiga freq limits.
			// In ST3, the frequency is always clamped to periods 113 to 856, while in ProTracker,
			// the limit is variable, depending on the finetune of the sample.
			// Test case: AmigaLimits.s3m, AmigaLimitsFinetune.mod
			if(m_SongFlags[SONG_AMIGALIMITS | SONG_PT1XMODE])
			{
				ASSERT(pChn->nFineTune == 0 || GetType() != MOD_TYPE_S3M);
				Limit(period, 113 * 4 - pChn->nFineTune, 856 * 4 - pChn->nFineTune);
				Limit(pChn->nPeriod, 113 * 4 - pChn->nFineTune, 856 * 4 - pChn->nFineTune);
			}

			ProcessPanbrello(pChn);

		}

		// IT Compatibility: Ensure that there is no pan swing, panbrello, panning envelopes, etc. applied on surround channels.
		// Test case: surround-pan.it
		if(pChn->dwFlags[CHN_SURROUND] && !m_SongFlags[SONG_SURROUNDPAN] && IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			pChn->nRealPan = 128;
		}

		// Now that all relevant envelopes etc. have been processed, we can parse the MIDI macro data.
		ProcessMacroOnChannel(nChn);

		// After MIDI macros have been processed, we can also process the pitch / filter envelope and other pitch-related things.
		if(samplePlaying)
		{
			ProcessPitchFilterEnvelope(pChn, period);
		}

		// Plugins may also receive vibrato
		ProcessVibrato(nChn, period, vibratoFactor);
		
		if(samplePlaying)
		{
			int nPeriodFrac = 0;
			ProcessSampleAutoVibrato(pChn, period, vibratoFactor, nPeriodFrac);

			// Final Period
			if (period <= m_nMinPeriod)
			{
				// ST3 simply stops playback if frequency is too high.
				// Test case: FreqLimits.s3m
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
				freq = Util::muldivr(freq, m_nMusicTempo, pIns->wPitchToTempoLock);
			}

			if ((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (freq < 256))
			{
				pChn->nFadeOutVol = 0;
				pChn->dwFlags.set(CHN_NOTEFADE);
				pChn->nRealVolume = 0;
				pChn->nCalcVolume = 0;
			}

			UINT ninc = Util::muldiv(freq, 0x10000, m_MixerSettings.gdwMixingFreq);
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
		if(pIns != nullptr && !IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			// In IT and FT2 compatible mode, envelope positions are updated above.
			// Test cases: s77.it, EnvLoops.xm
			IncrementEnvelopePositions(pChn);
		}

		// Volume ramping
		pChn->dwFlags.set(CHN_VOLUMERAMP, (pChn->nRealVolume | pChn->nLeftVol | pChn->nRightVol) != 0);

#ifdef ENABLE_STEREOVU
		if (pChn->nLeftVU > VUMETER_DECAY) pChn->nLeftVU -= VUMETER_DECAY; else pChn->nLeftVU = 0;
		if (pChn->nRightVU > VUMETER_DECAY) pChn->nRightVU -= VUMETER_DECAY; else pChn->nRightVU = 0;
#endif

		// Check for too big nInc
		if (((pChn->nInc >> 16) + 1) >= (LONG)(pChn->nLoopEnd - pChn->nLoopStart)) pChn->dwFlags.reset(CHN_LOOP);
		pChn->nNewRightVol = pChn->nNewLeftVol = 0;
		pChn->pCurrentSample = ((pChn->pSample) && (pChn->nLength) && (pChn->nInc)) ? pChn->pSample : NULL;
		if (pChn->pCurrentSample)
		{
			// Update VU-Meter (nRealVolume is 14-bit)
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
			const UINT kChnMasterVol = pChn->dwFlags[CHN_EXTRALOUD] ? 0x100 : nMasterVol;
#else
#define		kChnMasterVol	nMasterVol
#endif // MODPLUG_TRACKER

			// Adjusting volumes
			if (m_MixerSettings.gnChannels >= 2)
			{
				int pan = ((int)pChn->nRealPan) - 128;
				pan *= (int)m_MixerSettings.m_nStereoSeparation;
				pan /= 128;
				pan += 128;
				Limit(pan, 0, 256);

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
				if (panningMode == forceSoftPanning || (panningMode == dontForcePanningMode && (m_MixerSettings.gdwSoundSetup & SNDMIX_SOFTPANNING)))
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
			if (m_Resampler.Is(SRCMODE_NEAREST))
			{
				pChn->dwFlags.set(CHN_NOIDO);
			} else
			{
				pChn->dwFlags.reset(CHN_NOIDO | CHN_HQSRC);

				if (pChn->nInc == 0x10000)
				{
					pChn->dwFlags.set(CHN_NOIDO);
				} else
				if (m_Resampler.IsHQ())
				{
					if (!IsRenderingToDisc() && !m_Resampler.IsUltraHQ())
					{
						int fmax = 0x20000;
						if ((pChn->nNewLeftVol < 0x80) && (pChn->nNewRightVol < 0x80)
						 && (pChn->nLeftVol < 0x80) && (pChn->nRightVol < 0x80))
						{
							if (pChn->nInc >= 0xFF00) pChn->dwFlags.set(CHN_NOIDO);
						} else
						{
							pChn->dwFlags.set((pChn->nInc >= fmax) ? CHN_NOIDO : CHN_HQSRC);
						}
					} else
					{
						pChn->dwFlags.set(CHN_HQSRC);
					}
					
				} else
				{
					if ((pChn->nInc >= 0x14000) || ((pChn->nInc >= 0xFF00) && (pChn->nInc < 0x10100)))
						pChn->dwFlags.set(CHN_NOIDO);
				}
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
			if(pChn->dwFlags[CHN_SURROUND] && m_MixerSettings.gnChannels == 2) pChn->nNewLeftVol = - pChn->nNewLeftVol;

			// Checking Ping-Pong Loops
			if(pChn->dwFlags[CHN_PINGPONGFLAG]) pChn->nInc = -pChn->nInc;

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

		pChn->dwOldFlags = pChn->dwFlags;
	}

	// Checking Max Mix Channels reached: ordering by volume
	if ((m_nMixChannels >= m_MixerSettings.m_nMaxMixChannels) && !IsRenderingToDisc())
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
	if(m_SongFlags[SONG_GLOBALFADE])
	{
		if (!m_nGlobalFadeSamples)
		{
			m_SongFlags.set(SONG_ENDREACHED);
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
	if(nChn < GetNumChannels())
	{
		// TODO evaluate per-plugin macros here
		//ProcessMIDIMacro(nChn, false, m_MidiCfg.szMidiGlb[MIDIOUT_PAN]);
		//ProcessMIDIMacro(nChn, false, m_MidiCfg.szMidiGlb[MIDIOUT_VOLUME]);

		if((pChn->rowCommand.command == CMD_MIDI && m_SongFlags[SONG_FIRSTTICK]) || pChn->rowCommand.command == CMD_SMOOTHMIDI)
		{
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
	ModChannel &chn = Chn[nChn];

	// Do we need to process MIDI?
	// For now there is no difference between mute and sync mute with VSTis.
	if(chn.dwFlags[CHN_MUTE | CHN_SYNCMUTE] || !chn.HasMIDIOutput()) return;

	// Get instrument info and plugin reference
	const ModInstrument *pIns = chn.pModInstrument;	// Can't be nullptr at this point, as we have valid MIDI output.

	// No instrument or muted instrument?
	if(pIns->dwFlags[INS_MUTE])
	{
		return;
	}

	// Check instrument plugins
	const PLUGINDEX nPlugin = GetBestPlugin(nChn, PrioritiseInstrument, RespectMutes);
	IMixPlugin *pPlugin = nullptr;
	if(nPlugin > 0 && nPlugin <= MAX_MIXPLUGINS)
	{
		pPlugin = m_MixPlugins[nPlugin - 1].pMixPlugin;
	}

	// Couldn't find a valid plugin
	if(pPlugin == nullptr) return;

	const ModCommand::NOTE note = chn.rowCommand.note;
	// Check for volume commands
	uint8 vol = 0xFF;
	if(chn.rowCommand.volcmd == VOLCMD_VOLUME)
	{
		vol = Util::Min(chn.rowCommand.vol, uint8(64));
	} else if(chn.rowCommand.command == CMD_VOLUME)
	{
		vol = Util::Min(chn.rowCommand.param, uint8(64));
	}
	const bool hasVolCommand = (vol != 0xFF);

	if(GetModFlag(MSF_MIDICC_BUGEMULATION))
	{
		if(note != NOTE_NONE)
		{
			ModCommand::NOTE realNote = note;
			if(ModCommand::IsNote(note))
				realNote = pIns->NoteMap[note - 1];
			pPlugin->MidiCommand(GetBestMidiChannel(nChn), pIns->nMidiProgram, pIns->wMidiBank, realNote, static_cast<uint16>(chn.nVolume), nChn);
		} else if(hasVolCommand)
		{
			pPlugin->MidiCC(GetBestMidiChannel(nChn), MIDIEvents::MIDICC_Volume_Fine, vol, nChn);
		}
		return;
	}

	const uint32 defaultVolume = pIns->nGlobalVol;
	
	//If new note, determine notevelocity to use.
	if(note != NOTE_NONE)
	{
		uint16 velocity = static_cast<uint16>(4 * defaultVolume);
		switch(pIns->nPluginVelocityHandling)
		{
			case PLUGIN_VELOCITYHANDLING_CHANNEL:
				velocity = static_cast<uint16>(chn.nVolume);
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
				if(hasVolCommand) pPlugin->SetDryRatio(2 * vol);
				else pPlugin->SetDryRatio(2 * defaultVolume);
				break;

			case PLUGIN_VOLUMEHANDLING_MIDI:
				if(hasVolCommand) pPlugin->MidiCC(GetBestMidiChannel(nChn), MIDIEvents::MIDICC_Volume_Coarse, min(127, 2 * vol), nChn);
				else pPlugin->MidiCC(GetBestMidiChannel(nChn), MIDIEvents::MIDICC_Volume_Coarse, static_cast<uint8>(min(127, 2 * defaultVolume)), nChn);
				break;

		}
	}
}

#endif // MODPLUG_TRACKER


void CSoundFile::ApplyGlobalVolume(int SoundBuffer[], int RearBuffer[], long lTotalSampleCount)
//---------------------------------------------------------------------------------------------
{
	long step = 0;

	if (m_nGlobalVolumeDestination != m_nGlobalVolume)
	{
		// User has provided new global volume
		const bool rampUp = m_nGlobalVolumeDestination > m_nGlobalVolume;
		m_nGlobalVolumeDestination = m_nGlobalVolume;
		m_nSamplesToGlobalVolRampDest = m_nGlobalVolumeRampAmount = rampUp ? gnVolumeRampUpSamplesActual : m_MixerSettings.glVolumeRampDownSamples;
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
	for (int pairs = max(m_MixerSettings.gnChannels / 2, 1); pairs > 0; pairs--)
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
				*sample = Util::muldiv(*sample, m_lHighResRampingGlobalVolume, MAX_GLOBAL_VOLUME << VOLUMERAMPPRECISION);
				m_nSamplesToGlobalVolRampDest--;
			} else
			{
				*sample = Util::muldiv(*sample, m_nGlobalVolume, MAX_GLOBAL_VOLUME);
				m_lHighResRampingGlobalVolume = m_nGlobalVolume << VOLUMERAMPPRECISION;
			}
		}
	}
}
