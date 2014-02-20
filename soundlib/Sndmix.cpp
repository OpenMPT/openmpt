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

#include "Sndfile.h"
#include "MixerLoops.h"
#include "MIDIEvents.h"
#include "tuning.h"
#include "Tables.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/TrackerSettings.h"
#endif

// VU-Meter
#define VUMETER_DECAY		4

#ifndef NO_VST
PMIXPLUGINCREATEPROC CSoundFile::gpMixPluginCreateProc = NULL;
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
	SetPreAmp(mixersettings.m_nPreAmp); // adjust agc
	bool reset = false;
	if(
		(mixersettings.gdwMixingFreq != m_MixerSettings.gdwMixingFreq)
		||
		(mixersettings.gnChannels != m_MixerSettings.gnChannels)
		||
		(mixersettings.MixerFlags != m_MixerSettings.MixerFlags))
		reset = true;
	m_MixerSettings = mixersettings;
	InitPlayer(reset?TRUE:FALSE);
}


void CSoundFile::SetResamplerSettings(const CResamplerSettings &resamplersettings)
//--------------------------------------------------------------------------------
{
	m_Resampler.m_Settings = resamplersettings;
	m_Resampler.InitializeTables();
}


void CSoundFile::InitPlayer(BOOL bReset)
//--------------------------------------
{
	if(bReset)
	{
		ResetMixStat();
		gnDryLOfsVol = 0;
		gnDryROfsVol = 0;
	}
	m_Resampler.InitializeTables();
#ifndef NO_REVERB
	m_Reverb.Initialize(bReset, m_MixerSettings.gdwMixingFreq);
#endif
#ifndef NO_DSP
	m_DSP.Initialize(bReset, m_MixerSettings.gdwMixingFreq, m_MixerSettings.DSPMask);
#endif
#ifndef NO_EQ
	m_EQ.Initialize(bReset, m_MixerSettings.gdwMixingFreq);
#endif
#ifndef NO_AGC
	m_AGC.Initialize(bReset, m_MixerSettings.gdwMixingFreq);
#endif
}


BOOL CSoundFile::FadeSong(UINT msec)
//----------------------------------
{
	samplecount_t nsamples = Util::muldiv(msec, m_MixerSettings.gdwMixingFreq, 1000);
	if (nsamples <= 0) return FALSE;
	if (nsamples > 0x100000) nsamples = 0x100000;
	m_nBufferCount = nsamples;
	int32 nRampLength = static_cast<int32>(m_nBufferCount);
	// Ramp everything down
	for (UINT noff=0; noff < m_nMixChannels; noff++)
	{
		ModChannel *pramp = &Chn[ChnMix[noff]];
		if (!pramp) continue;
		pramp->newRightVol = pramp->newLeftVol = 0;
		pramp->leftRamp = (-pramp->leftVol << VOLUMERAMPPRECISION) / nRampLength;
		pramp->rightRamp = (-pramp->rightVol << VOLUMERAMPPRECISION) / nRampLength;
		pramp->rampLeftVol = pramp->leftVol << VOLUMERAMPPRECISION;
		pramp->rampRightVol = pramp->rightVol << VOLUMERAMPPRECISION;
		pramp->nRampLength = nRampLength;
		pramp->dwFlags.set(CHN_VOLUMERAMP);
	}
	return TRUE;
}


CSoundFile::samplecount_t CSoundFile::Read(samplecount_t count, IAudioReadTarget &target)
//---------------------------------------------------------------------------------------
{
	ALWAYS_ASSERT(m_MixerSettings.IsValid());

	bool mixPlugins = false;
	for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; ++i)
	{
		if(m_MixPlugins[i].pMixPlugin)
		{
			mixPlugins = true;
			break;
		}
	}

	const samplecount_t countGoal = count;
	samplecount_t countRendered = 0;
	samplecount_t countToRender = countGoal;

	while(!m_SongFlags[SONG_ENDREACHED] && countToRender > 0)
	{

		// Update Channel Data
		if(!m_nBufferCount)
		{ // last tick or fade completely processed, find out what to do next

			if(m_SongFlags[SONG_FADINGSONG])
			{ // song was faded out
				m_SongFlags.set(SONG_ENDREACHED);
			} else if(ReadNote())
			{ // render next tick (normal progress)
				ASSERT(m_nBufferCount > 0);
				#ifdef MODPLUG_TRACKER
					// Save pattern cue points for WAV rendering here (if we reached a new pattern, that is.)
					if(IsRenderingToDisc() && (m_PatternCuePoints.empty() || m_nCurrentOrder != m_PatternCuePoints.back().order))
					{
						PatternCuePoint cue;
						cue.offset = countRendered;
						cue.order = m_nCurrentOrder;
						cue.processed = false;	// We don't know the base offset in the file here. It has to be added in the main conversion loop.
						m_PatternCuePoints.push_back(cue);
					}
				#endif
			} else
			{ // no new pattern data
				#ifdef MODPLUG_TRACKER
					if((m_nMaxOrderPosition) && (m_nCurrentOrder >= m_nMaxOrderPosition))
					{
						m_SongFlags.set(SONG_ENDREACHED);
					}
				#endif // MODPLUG_TRACKER
				if(IsRenderingToDisc())
				{ // rewbs: disable song fade when rendering.
					m_SongFlags.set(SONG_ENDREACHED);
				} else
				{ // end of song reached, fade it out
					if(FadeSong(FADESONGDELAY)) // sets m_nBufferCount xor returns false
					{ // FadeSong sets m_nBufferCount here
						ASSERT(m_nBufferCount > 0);
						m_SongFlags.set(SONG_FADINGSONG);
					} else
					{
						m_SongFlags.set(SONG_ENDREACHED);
					}
				}
			}

		}

		if(m_SongFlags[SONG_ENDREACHED])
		{
			break; // mix done
		}

		ASSERT(m_nBufferCount > 0); // assert that we have actually something to do

		const samplecount_t countChunk = std::min<samplecount_t>(MIXBUFFERSIZE, std::min<samplecount_t>(m_nBufferCount, countToRender));

		CreateStereoMix(countChunk);

		#ifndef NO_REVERB
			m_Reverb.Process(MixSoundBuffer, countChunk);
		#endif // NO_REVERB

		if(mixPlugins)
		{
			ProcessPlugins(countChunk);
		}

		if(m_MixerSettings.gnChannels == 1)
		{
			MonoFromStereo(MixSoundBuffer, countChunk);
		}

		if(m_PlayConfig.getGlobalVolumeAppliesToMaster())
		{
			ApplyGlobalVolume(MixSoundBuffer, MixRearBuffer, countChunk);
		}

		if(m_MixerSettings.DSPMask)
		{
			ProcessDSP(countChunk);
		}

		if(m_MixerSettings.gnChannels == 4)
		{
			InterleaveFrontRear(MixSoundBuffer, MixRearBuffer, countChunk);
		}

		target.DataCallback(MixSoundBuffer, m_MixerSettings.gnChannels, countChunk);

		// Buffer ready
		countRendered += countChunk;
		countToRender -= countChunk;
		m_nBufferCount -= countChunk;
		m_lTotalSampleCount += countChunk;		// increase sample count for VSTTimeInfo.

#ifdef MODPLUG_TRACKER
		if(IsRenderingToDisc())
		{
			// Stop playback on F00 if no more voices are active.
			// F00 sets the tick count to 65536 in FT2, so it just generates a reaaaally long row.
			// Usually this command can be found at the end of a song to effectively stop playback.
			// Since we don't want to render hours of silence, we are going to check if there are
			// still any channels playing, and if that is no longer the case, we stop playback at
			// the end of the next tick.
			if(m_nMusicSpeed == uint16_max && m_nMixStat == 0 && GetType() == MOD_TYPE_XM && !m_nBufferCount)
			{
				m_SongFlags.set(SONG_ENDREACHED);
			}
		}
#endif // MODPLUG_TRACKER
	}

	// mix done

	return countRendered;

}


void CSoundFile::ProcessDSP(std::size_t countChunk)
//-------------------------------------------------
{
	#ifndef NO_DSP
		if(m_MixerSettings.DSPMask & (SNDDSP_SURROUND|SNDDSP_MEGABASS|SNDDSP_NOISEREDUCTION))
		{
			m_DSP.Process(MixSoundBuffer, MixRearBuffer, countChunk, m_MixerSettings.gnChannels, m_MixerSettings.DSPMask);
		}
	#endif // NO_DSP

	#ifndef NO_EQ
		if(m_MixerSettings.DSPMask & SNDDSP_EQ)
		{
			m_EQ.Process(MixSoundBuffer, MixRearBuffer, countChunk, m_MixerSettings.gnChannels);
		}
	#endif // NO_EQ

	#ifndef NO_AGC
		if(m_MixerSettings.DSPMask & SNDDSP_AGC)
		{
			m_AGC.Process(MixSoundBuffer, MixRearBuffer, countChunk, m_MixerSettings.gnChannels);
		}
	#endif // NO_AGC
	#if defined(NO_DSP) && defined(NO_EQ) && defined(NO_AGC)
		MPT_UNREFERENCED_PARAMETER(countChunk);
	#endif
}


/////////////////////////////////////////////////////////////////////////////
// Handles navigation/effects

BOOL CSoundFile::ProcessRow()
//---------------------------
{
	while(++m_nTickCount >= GetNumTicksOnCurrentRow())
	{
		// When having an EEx effect on the same row as a Dxx jump, the target row is not played in ProTracker.
		// Test case: DelayBreak.mod (based on condom_corruption by Travolta)
		const bool ignoreRow = m_nPatternDelay != 0 && m_SongFlags[SONG_BREAKTOROW] && GetType() == MOD_TYPE_MOD;

		HandlePatternTransitionEvents();
		m_nPatternDelay = 0;
		m_nFrameDelay = 0;
		m_nTickCount = 0;
		m_nRow = m_nNextRow;
		// Reset Pattern Loop Effect
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
						for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
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
					while(m_nCurrentOrder < Order.size() && Order[m_nCurrentOrder] == Order.GetIgnoreIndex())
					{
						m_nCurrentOrder++;
					}
					//Check for end of song or bad pattern
					if (m_nCurrentOrder >= Order.size()
						|| !Patterns.IsValidPat(Order[m_nCurrentOrder]))
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
					GetLength(eAdjustOnSuccess, GetLengthTarget(m_nCurrentOrder, m_nRow));
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

			pChn->rightVol = pChn->newRightVol;
			pChn->leftVol = pChn->newLeftVol;
			pChn->dwFlags.reset(CHN_PORTAMENTO | CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO);
			pChn->nCommand = CMD_NONE;
			pChn->m_plugParamValueStep = 0;
		}

		// Now that we know which pattern we're on, we can update time signatures (global or pattern-specific)
		UpdateTimeSignature();

		if(ignoreRow)
		{
			m_nTickCount = m_nMusicSpeed;
			continue;
		}
		break;
	}
	// Should we process tick0 effects?
	if (!m_nMusicSpeed) m_nMusicSpeed = 1;

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
		if(!(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2 | MOD_TYPE_MOD)) && m_nTickCount < GetNumTicksOnCurrentRow())
		{
			// Emulate first tick behaviour if Row Delay is set.
			// Test cases: PatternDelaysRetrig.it, PatternDelaysRetrig.s3m, PatternDelaysRetrig.xm
			if(!(m_nTickCount % (m_nMusicSpeed + m_nFrameDelay)))
			{
				m_SongFlags.set(SONG_FIRSTTICK);
			}
		}
	} else
	{
		m_SongFlags.set(SONG_FIRSTTICK);
		m_SongFlags.reset(SONG_BREAKTOROW);
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
	// IT compatibility: IT has its own, more precise tables
	if(IsCompatibleMode(TRK_IMPULSETRACKER))
	{
		switch(type & 0x03)
		{
		case 0:
		default:
			return ITSinusTable[position];
		case 1:
			return ITRampDownTable[position];
		case 2:
			return position < 128 ? 64 : 0;
		case 3:
			return (rand() & 0x7F) - 0x40;
		}
	} else
	{
		switch(type & 0x03)
		{
		case 0:
		default:
			return ModSinusTable[position];
		case 1:
			return ModRampDownTable[position];
		case 2:
			return position < 32 ? 127 : -127;
		case 3:
			return ModRandomTable[position];
		}
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
			uint8 ontime = pChn->nTremorParam >> 4;
			uint8 n = ontime + (pChn->nTremorParam & 0x0F);	// Total tremor cycle time (On + Off)
			if ((!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) || m_SongFlags[SONG_ITOLDEFFECTS])
			{
				n += 2;
				ontime++;
			}
			uint8 tremcount = pChn->nTremorCount;
			if(!(GetType() & MOD_TYPE_XM))
			{
				if (tremcount >= n) tremcount = 0;
				if (tremcount >= ontime) vol = 0;
				pChn->nTremorCount = tremcount + 1;
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
					pChn->nTremorCount = tremcount + 1;
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

		int pan = pChn->nRealPan;
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
#ifdef MODPLUG_TRACKER
		const int range = ENVELOPE_MAX;
		const float amp = 512.0f;
#else
		// TODO: AMS2 envelopes behave differently when linear slides are off - emulate with 15 * (-128...127) >> 6
		// Copy over vibrato behaviour for that?
		const int range = GetType() == MOD_TYPE_AMS2 ? uint8_max : ENVELOPE_MAX;
		const float amp = GetType() == MOD_TYPE_AMS2 ? 64.0f : 512.0f;
#endif
		const int envval = Util::Round<int>((pIns->PitchEnv.GetValueFromPosition(envpos, range) - 0.5f) * amp);

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
			if(!(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2))) end++;

			// FT2 compatibility: If the sustain point is at the loop end and the sustain loop has been released, don't loop anymore.
			// Test case: EnvLoops.xm
			const bool escapeLoop = (insEnv.nLoopEnd == insEnv.nSustainEnd && insEnv.dwFlags[ENV_SUSTAIN] && pChn->dwFlags[CHN_KEYOFF] && IsCompatibleMode(TRK_FASTTRACKER2));

			if(position == end && !escapeLoop)
			{
				position = insEnv.Ticks[insEnv.nLoopStart];

				if(envType == ENV_VOLUME && insEnv.nLoopStart == insEnv.nLoopEnd && insEnv.Values[insEnv.nLoopEnd] == 0
					&& (!(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2)) || (insEnv.nLoopEnd + 1u == insEnv.nNodes)))
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

	if ((pIns->nPPS) && (pChn->nNote != NOTE_NONE))
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
		uint32 panpos;
		// IT compatibility: IT has its own, more precise tables
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
			panpos = pChn->nPanbrelloPos & 0xFF;
		else
			panpos = ((pChn->nPanbrelloPos + 0x10) >> 2) & 0x3F;

		int pdelta = GetVibratoDelta(pChn->nPanbrelloType, panpos);

		// IT compatibility: Sample-and-hold style random panbrello (tremolo and vibrato don't use this mechanism in IT)
		// Test case: RandomWaveform.it
		if(IsCompatibleMode(TRK_IMPULSETRACKER) && pChn->nPanbrelloType == 3)
		{
			if(pChn->nPanbrelloPos == 0 || pChn->nPanbrelloPos >= pChn->nPanbrelloSpeed)
			{
				pChn->nPanbrelloPos = 0;
				pChn->nPanbrelloRandomMemory = static_cast<int8>(pdelta);
			}
			pChn->nPanbrelloPos++;
			pdelta = pChn->nPanbrelloRandomMemory;
		} else
		{
			pChn->nPanbrelloPos += pChn->nPanbrelloSpeed;
		}

		pdelta = ((pdelta * (int)pChn->nPanbrelloDepth) + 2) >> 3;
		pdelta += pChn->nRealPan;

		pChn->nRealPan = Clamp(pdelta, 0, 256);
		//if(IsCompatibleMode(TRK_IMPULSETRACKER)) pChn->nPan = pChn->nRealPan; // TODO
	}
}


void CSoundFile::ProcessArpeggio(CHANNELINDEX nChn, int &period, CTuning::NOTEINDEXTYPE &arpeggioSteps)
//-----------------------------------------------------------------------------------------------------
{
	ModChannel *pChn = &Chn[nChn];

#ifndef NO_VST
	// Plugin arpeggio
	if(pChn->pModInstrument && pChn->pModInstrument->nMixPlug)
	{
		const ModInstrument *pIns = pChn->pModInstrument;
		IMixPlugin *pPlugin =  m_MixPlugins[pIns->nMixPlug - 1].pMixPlugin;
		if(pPlugin)
		{
			uint8 step = 0;
			const bool arpOnRow = (pChn->rowCommand.command == CMD_ARPEGGIO);
			if(arpOnRow)
			{
				switch(m_nTickCount % 3)
				{
				case 1: step = pChn->nArpeggio >> 4; break;
				case 2: step = pChn->nArpeggio & 0x0F; break;
				}
				pChn->nArpeggioBaseNote = pChn->nLastNote;
			}

			// Trigger new note:
			// - If there's an arpeggio on this row and
			//   - the note to trigger is not the same as the previous arpeggio note or
			//   - a pattern note has just been triggered on this tick
			// - If there's no arpeggio
			//   - but an arpeggio note is still active and
			//   - there's no note stop or new note that would stop it anyway
			if((arpOnRow && pChn->nArpeggioLastNote != pChn->nArpeggioBaseNote + step && (!m_SongFlags[SONG_FIRSTTICK] || !pChn->rowCommand.IsNote()))
				|| (!arpOnRow && pChn->rowCommand.note == NOTE_NONE && pChn->nArpeggioLastNote != NOTE_NONE))
				pPlugin->MidiCommand(GetBestMidiChannel(nChn), pIns->nMidiProgram, pIns->wMidiBank, pChn->nArpeggioBaseNote + step, static_cast<uint16>(pChn->nVolume), nChn);
			// Stop note:
			// - If some arpeggio note is still registered or
			// - When starting an arpeggio on a row with no other note on it, stop some possibly still playing note.
			if(pChn->nArpeggioLastNote != NOTE_NONE)
				pPlugin->MidiCommand(GetBestMidiChannel(nChn), pIns->nMidiProgram, pIns->wMidiBank, pChn->nArpeggioLastNote + NOTE_MAX_SPECIAL, 0, nChn);
			else if(arpOnRow && m_SongFlags[SONG_FIRSTTICK] && !pChn->rowCommand.IsNote() && ModCommand::IsNote(pChn->nLastNote))
				pPlugin->MidiCommand(GetBestMidiChannel(nChn), pIns->nMidiProgram, pIns->wMidiBank, pChn->nLastNote + NOTE_MAX_SPECIAL, 0, nChn);

			if(pChn->rowCommand.command == CMD_ARPEGGIO)
				pChn->nArpeggioLastNote = pChn->nArpeggioBaseNote + step;
			else
				pChn->nArpeggioLastNote = NOTE_NONE;
		}
	}
#endif // NO_VST

	if(pChn->nCommand == CMD_ARPEGGIO)
	{
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
					case 1: period = Util::Round<int>(period / TwoToPowerXOver12(pChn->nArpeggio >> 4)); break;
					case 2: period = Util::Round<int>(period / TwoToPowerXOver12(pChn->nArpeggio & 0x0F)); break;
					}
				}
			}
			// FastTracker 2: Swedish tracker logic (TM) arpeggio
			else if(IsCompatibleMode(TRK_FASTTRACKER2))
			{
				uint8 note = pChn->nNote;
				int arpPos = 0;

				if(!m_SongFlags[SONG_FIRSTTICK])
				{
					arpPos = m_nMusicSpeed - (m_nTickCount % m_nMusicSpeed);
					// The fact that arpeggio behaves in a totally fucked up way at 16 ticks/row or more is that the arpeggio offset LUT only has 16 entries in FT2.
					// At more than 16 ticks/row, FT2 reads into the vibrato table, which is placed right after the arpeggio table.
					// Test case: Arpeggio.xm
					if(arpPos > 16) arpPos = 2;
					else if(arpPos == 16) arpPos = 0;
					else arpPos %= 3;
					switch(arpPos)
					{
					case 1: note += (pChn->nArpeggio >> 4); break;
					case 2: note += (pChn->nArpeggio & 0x0F); break;
					}
				}

				// Test case: ArpeggioClamp.xm
				if(note > 108 + NOTE_MIN && arpPos != 0)
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
				if(note != pChn->nNote || GetType() == MOD_TYPE_STM)
				{
					if(m_SongFlags[SONG_PT1XMODE] && note >= NOTE_MIDDLEC + 24)
					{
						// Weird arpeggio wrap-around in ProTracker.
						// Test case: ArpWraparound.mod, and the snare sound in "Jim is dead" by doh.
						note -= 37;
					}
					period = GetPeriodFromNote(note, pChn->nFineTune, pChn->nC5Speed);

					// The arpeggio note offset remains effective after the end of the current row in ScreamTracker 2.
					// This fixes the flute lead in MORPH.STM by Skaven, pattern 27.
					if(GetType() == MOD_TYPE_STM)
					{
						pChn->nPeriod = period;
					}
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
			if(m_SongFlags.test_all(SONG_FIRSTTICK | SONG_PT1XMODE) || ((GetType() & (MOD_TYPE_DIGI | MOD_TYPE_DBM)) && m_SongFlags[SONG_FIRSTTICK]))
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
				if(GetType() == MOD_TYPE_DBM) vdepth = 7;	// Closer than 6, but not quite.
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
	} else if(chn.dwOldFlags[CHN_VIBRATO])
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
			adepth = MIN(adepth, (int)(pSmp->nVibDepth << 8));
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
				vdelta = vibpos < 128 ? 64 : 0;
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
	pChn->leftRamp = pChn->rightRamp = 0;
	if(pChn->dwFlags[CHN_VOLUMERAMP] && (pChn->leftVol != pChn->newLeftVol || pChn->rightVol != pChn->newRightVol))
	{
		const bool rampUp = (pChn->newLeftVol > pChn->leftVol) || (pChn->newRightVol > pChn->rightVol);
		int32 rampLength, globalRampLength, instrRampLength = 0;
		rampLength = globalRampLength = (rampUp ? m_MixerSettings.GetVolumeRampUpSamples() : m_MixerSettings.GetVolumeRampDownSamples());
		//XXXih: add real support for bidi ramping here

		if(GetModFlag(MSF_VOLRAMP) && (GetType() & MOD_TYPE_XM))
		{
			// apply FT2-style super-soft volume ramping (5ms), overriding openmpt settings
			rampLength = globalRampLength = Util::muldivr(5, m_MixerSettings.gdwMixingFreq, 1000);
		}
		
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

		int32 leftDelta = ((pChn->newLeftVol - pChn->leftVol) << VOLUMERAMPPRECISION);
		int32 rightDelta = ((pChn->newRightVol - pChn->rightVol) << VOLUMERAMPPRECISION);
		if(!enableCustomRamp)
		{
			// Extra-smooth ramping, unless we're forced to use the default values
			if((pChn->leftVol | pChn->rightVol) && (pChn->newLeftVol | pChn->newRightVol) && !pChn->dwFlags[CHN_FASTVOLRAMP])
			{
				rampLength = m_nBufferCount;
				Limit(rampLength, globalRampLength, 1 << (VOLUMERAMPPRECISION - 1));
			}
		}

		pChn->leftRamp = leftDelta / rampLength;
		pChn->rightRamp = rightDelta / rampLength;
		pChn->leftVol = pChn->newLeftVol - ((pChn->leftRamp * rampLength) >> VOLUMERAMPPRECISION);
		pChn->rightVol = pChn->newRightVol - ((pChn->rightRamp * rampLength) >> VOLUMERAMPPRECISION);

		if (pChn->leftRamp|pChn->rightRamp)
		{
			pChn->nRampLength = rampLength;
		} else
		{
			pChn->dwFlags.reset(CHN_VOLUMERAMP);
			pChn->leftVol = pChn->newLeftVol;
			pChn->rightVol = pChn->newRightVol;
		}
	} else
	{
		pChn->dwFlags.reset(CHN_VOLUMERAMP);
		pChn->leftVol = pChn->newLeftVol;
		pChn->rightVol = pChn->newRightVol;
	}
	pChn->rampLeftVol = pChn->leftVol << VOLUMERAMPPRECISION;
	pChn->rampRightVol = pChn->rightVol << VOLUMERAMPPRECISION;
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

	m_nSamplesPerTick = GetTickDuration(m_nMusicTempo, m_nMusicSpeed, m_nCurrentRowsPerBeat);
	m_nBufferCount = m_nSamplesPerTick;

	// Master Volume + Pre-Amplification / Attenuation setup
	DWORD nMasterVol;
	{
		CHANNELINDEX nchn32 = Clamp(m_nChannels, CHANNELINDEX(1), CHANNELINDEX(31));
		
		DWORD mastervol;

		if (m_PlayConfig.getUseGlobalPreAmp())
		{
			int realmastervol = m_MixerSettings.m_nPreAmp;
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

		if (m_PlayConfig.getUseGlobalPreAmp())
		{
			UINT attenuation =
#ifndef NO_AGC
				(m_MixerSettings.DSPMask & SNDDSP_AGC) ? PreAmpAGCTable[nchn32 >> 1] :
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
		if(pChn->dwFlags[CHN_NOTEFADE] && (!(pChn->nFadeOutVol|pChn->leftVol|pChn->rightVol)) && (!IsCompatibleMode(TRK_FASTTRACKER2)))
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
			pChn->nLeftVU = pChn->nRightVU = 0;
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
					// In IT compatible mode, envelope position indices are shifted by one for proper envelope pausing,
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
				} else if (m_PlayConfig.getGlobalVolumeAppliesToMaster())
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

			ProcessArpeggio(nChn, period, arpeggioSteps);

			// Preserve Amiga freq limits.
			// In ST3, the frequency is always clamped to periods 113 to 856, while in ProTracker,
			// the limit is variable, depending on the finetune of the sample.
			// Test case: AmigaLimits.s3m, AmigaLimitsFinetune.mod
			if(m_SongFlags[SONG_AMIGALIMITS | SONG_PT1XMODE])
			{
				int limitLow = 113 * 4, limitHigh = 856 * 4;
				if(m_SongFlags[SONG_PT1XMODE])
				{
					const int tableOffset = XM2MODFineTune(pChn->nFineTune) * 12;
					limitLow = ProTrackerTunedPeriods[tableOffset +  11] / 2;
					limitHigh = ProTrackerTunedPeriods[tableOffset] * 2;
				}
				Limit(period, limitLow, limitHigh);
				Limit(pChn->nPeriod, limitLow, limitHigh);
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
					pChn->m_Freq = Util::Round<uint32>((pChn->nC5Speed << FREQ_FRACBITS) * vibratoFactor * pIns->pTuning->GetRatio(pChn->nNote - NOTE_MIDDLEC + arpeggioSteps, pChn->nFineTune+pChn->m_PortamentoFineSteps));
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

			uint32 ninc = Util::muldivr(freq, 0x10000, m_MixerSettings.gdwMixingFreq << FREQ_FRACBITS);
#ifndef MODPLUG_TRACKER
			ninc = Util::muldivr(ninc, m_nFreqFactor, 128);
#endif // !MODPLUG_TRACKER
			if(ninc == 0)
			{
				ninc = 1;
			}
			pChn->nInc = ninc;
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
		pChn->dwFlags.set(CHN_VOLUMERAMP, (pChn->nRealVolume | pChn->rightVol | pChn->leftVol) != 0);

		if (pChn->nLeftVU > VUMETER_DECAY) pChn->nLeftVU -= VUMETER_DECAY; else pChn->nLeftVU = 0;
		if (pChn->nRightVU > VUMETER_DECAY) pChn->nRightVU -= VUMETER_DECAY; else pChn->nRightVU = 0;

		// Check for too big nInc
		//if (((pChn->nInc >> 16) + 1) >= (int32)(pChn->nLoopEnd - pChn->nLoopStart)) pChn->dwFlags.reset(CHN_LOOP);
		pChn->newLeftVol = pChn->newRightVol = 0;
		pChn->pCurrentSample = (pChn->pModSample && pChn->pModSample->pSample && pChn->nLength && pChn->nInc) ? pChn->pModSample->pSample : nullptr;
		if (pChn->pCurrentSample)
		{
			// Update VU-Meter (nRealVolume is 14-bit)
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

#ifdef MODPLUG_TRACKER
			const UINT kChnMasterVol = pChn->dwFlags[CHN_EXTRALOUD] ? (UINT)m_PlayConfig.getNormalSamplePreAmp() : nMasterVol;
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
				if (m_PlayConfig.getUseGlobalPreAmp())
				{
					realvol = (pChn->nRealVolume * kChnMasterVol) >> 7;
				} else
				{
					// Extra attenuation required here if we're bypassing pre-amp.
					realvol = (pChn->nRealVolume * kChnMasterVol) >> 8;
				}
				
				const forcePanningMode panningMode = m_PlayConfig.getForcePanningMode();
				if (panningMode == forceSoftPanning || (panningMode == dontForcePanningMode && (m_MixerSettings.MixerFlags & SNDMIX_SOFTPANNING)))
				{
					if (pan < 128)
					{
						pChn->newLeftVol = (realvol * 128) >> 8;
						pChn->newRightVol = (realvol * pan) >> 8;
					} else
					{
						pChn->newLeftVol = (realvol * (256 - pan)) >> 8;
						pChn->newRightVol = (realvol * 128) >> 8;
					}
				} else if(GetType() == MOD_TYPE_XM && m_PlayConfig.getEmulateQuirks())
				{
					// FT2 uses square root panning. There is a 257-entry LUT for this,
					// but FT2's internal panning ranges from 0 to 255 only, meaning that
					// you can never truly achieve 100% right panning in FT2, only 100% left.
					// Test case: FT2PanLaw.xm
					LimitMax(pan, 255);
					const int panL = pan > 0 ? XMPanningTable[256 - pan] : 65536;
					const int panR = XMPanningTable[pan];
					pChn->newLeftVol = (realvol * panL) >> 16;
					pChn->newRightVol = (realvol * panR) >> 16;
				} else
				{
					pChn->newLeftVol = (realvol * (256 - pan)) >> 8;
					pChn->newRightVol = (realvol * pan) >> 8;
				}

			} else
			{
				pChn->newLeftVol = (pChn->nRealVolume * kChnMasterVol) >> 8;
				pChn->newRightVol = pChn->newLeftVol;
			}
			// Clipping volumes
			//if (pChn->nNewRightVol > 0xFFFF) pChn->nNewRightVol = 0xFFFF;
			//if (pChn->nNewLeftVol > 0xFFFF) pChn->nNewLeftVol = 0xFFFF;

			if(pChn->nInc == 0x10000)
			{
				// exact samplerate match, do not resample at all, regardless of selected resampler
				pChn->resamplingMode = SRCMODE_NEAREST;
			} else if(pChn->pModInstrument && IsKnownResamplingMode(pChn->pModInstrument->nResampling))
			{
				// for defined resampling modes, use per-instrument resampling mode if set
				pChn->resamplingMode = static_cast<uint8>(pChn->pModInstrument->nResampling);
			} else
			{
				// default to global mixer settings
				pChn->resamplingMode = static_cast<uint8>(m_Resampler.m_Settings.SrcMode);
			}

			/*if (m_pConfig->getUseGlobalPreAmp())
			{
				pChn->nNewRightVol >>= MIXING_ATTENUATION;
				pChn->nNewLeftVol >>= MIXING_ATTENUATION;
			}*/
			const int extraAttenuation = m_PlayConfig.getExtraSampleAttenuation();
			pChn->newLeftVol >>= extraAttenuation;
			pChn->newRightVol >>= extraAttenuation;

			// Dolby Pro-Logic Surround
			if(pChn->dwFlags[CHN_SURROUND] && m_MixerSettings.gnChannels == 2) pChn->newRightVol = - pChn->newRightVol;

			// Checking Ping-Pong Loops
			if(pChn->dwFlags[CHN_PINGPONGFLAG]) pChn->nInc = -pChn->nInc;

			// Setting up volume ramp
			ProcessRamping(pChn);

			// Adding the channel in the channel list
			ChnMix[m_nMixChannels++] = nChn;
		} else
		{
			// Note change but no sample
			if (pChn->nLeftVU > 128) pChn->nLeftVU = 0;
			if (pChn->nRightVU > 128) pChn->nRightVU = 0;

			pChn->rightVol = pChn->leftVol = 0;
			pChn->nLength = 0;
		}

		pChn->dwOldFlags = pChn->dwFlags;
	}

	// Checking Max Mix Channels reached: ordering by volume
	if(m_nMixChannels >= m_MixerSettings.m_nMaxMixChannels)
	{
		for(CHANNELINDEX i=0; i<m_nMixChannels; i++)
		{
			CHANNELINDEX j=i;
			while ((j+1<m_nMixChannels) && (Chn[ChnMix[j]].nRealVolume < Chn[ChnMix[j+1]].nRealVolume))
			{
				CHANNELINDEX n = ChnMix[j];
				ChnMix[j] = ChnMix[j+1];
				ChnMix[j+1] = n;
				j++;
			}
		}
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
		vol = std::min(chn.rowCommand.vol, uint8(64));
	} else if(chn.rowCommand.command == CMD_VOLUME)
	{
		vol = std::min(chn.rowCommand.param, uint8(64));
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
				if(hasVolCommand) pPlugin->MidiCC(GetBestMidiChannel(nChn), MIDIEvents::MIDICC_Volume_Coarse, MIN(127, 2 * vol), nChn);
				else pPlugin->MidiCC(GetBestMidiChannel(nChn), MIDIEvents::MIDICC_Volume_Coarse, static_cast<uint8>(MIN(127, 2 * defaultVolume)), nChn);
				break;

		}
	}
}

#endif // MODPLUG_TRACKER


template<int channels>
forceinline void ApplyGlobalVolumeWithRamping(int *SoundBuffer, int *RearBuffer, long lCount, UINT m_nGlobalVolume, long step, UINT &m_nSamplesToGlobalVolRampDest, long &m_lHighResRampingGlobalVolume)
{
	const bool isStereo = (channels >= 2);
	const bool hasRear = (channels >= 4);
	for(int pos = 0; pos < lCount; ++pos)
	{
		if(m_nSamplesToGlobalVolRampDest > 0)
		{
			// Ramping required
			m_lHighResRampingGlobalVolume += step;
			             SoundBuffer[0] = Util::muldiv(SoundBuffer[0], m_lHighResRampingGlobalVolume, MAX_GLOBAL_VOLUME << VOLUMERAMPPRECISION);
			if(isStereo) SoundBuffer[1] = Util::muldiv(SoundBuffer[1], m_lHighResRampingGlobalVolume, MAX_GLOBAL_VOLUME << VOLUMERAMPPRECISION);
			if(hasRear)  RearBuffer[0]  = Util::muldiv(RearBuffer[0] , m_lHighResRampingGlobalVolume, MAX_GLOBAL_VOLUME << VOLUMERAMPPRECISION);
			if(hasRear)  RearBuffer[1]  = Util::muldiv(RearBuffer[1] , m_lHighResRampingGlobalVolume, MAX_GLOBAL_VOLUME << VOLUMERAMPPRECISION);
			m_nSamplesToGlobalVolRampDest--;
		} else
		{
			             SoundBuffer[0] = Util::muldiv(SoundBuffer[0], m_nGlobalVolume, MAX_GLOBAL_VOLUME);
			if(isStereo) SoundBuffer[1] = Util::muldiv(SoundBuffer[1], m_nGlobalVolume, MAX_GLOBAL_VOLUME);
			if(hasRear)  RearBuffer[0]  = Util::muldiv(RearBuffer[0] , m_nGlobalVolume, MAX_GLOBAL_VOLUME);
			if(hasRear)  RearBuffer[1]  = Util::muldiv(RearBuffer[1] , m_nGlobalVolume, MAX_GLOBAL_VOLUME);
			m_lHighResRampingGlobalVolume = m_nGlobalVolume << VOLUMERAMPPRECISION;
		}
		SoundBuffer += isStereo ? 2 : 1;
		if(hasRear) RearBuffer += 2;
	}
}


void CSoundFile::ApplyGlobalVolume(int *SoundBuffer, int *RearBuffer, long lCount)
//--------------------------------------------------------------------------------
{

	// should we ramp?
	if(IsGlobalVolumeUnset())
	{
		// do not ramp if no global volume was set before (which is the case at song start), to prevent audible glitches when default volume is > 0 and it is set to 0 in the first row
		m_nGlobalVolumeDestination = m_nGlobalVolume;
		m_nSamplesToGlobalVolRampDest = 0;
		m_nGlobalVolumeRampAmount = 0;
	} else if(m_nGlobalVolumeDestination != m_nGlobalVolume)
	{
		// User has provided new global volume

		// m_nGlobalVolume: the last global volume which got set e.g. by a pattern command
		// m_nGlobalVolumeDestination: the current target of the ramping algorithm
		const bool rampUp = m_nGlobalVolume > m_nGlobalVolumeDestination;

		m_nGlobalVolumeDestination = m_nGlobalVolume;
		m_nSamplesToGlobalVolRampDest = m_nGlobalVolumeRampAmount = rampUp ? m_MixerSettings.GetVolumeRampUpSamples() : m_MixerSettings.GetVolumeRampDownSamples();
	} 

	// calculate ramping step
	long step = 0;
	if (m_nSamplesToGlobalVolRampDest > 0)
	{

		// Still some ramping left to do.
		long highResGlobalVolumeDestination = static_cast<long>(m_nGlobalVolumeDestination) << VOLUMERAMPPRECISION;

		const long delta = highResGlobalVolumeDestination - m_lHighResRampingGlobalVolume;
		step = delta / static_cast<long>(m_nSamplesToGlobalVolRampDest);

		// Define max step size as some factor of user defined ramping value: the lower the value, the more likely the click.
		// If step is too big (might cause click), extend ramp length.
		UINT maxStep = MAX(50, (10000 / (m_nGlobalVolumeRampAmount + 1)));
		while(static_cast<UINT>(abs(step)) > maxStep)
		{
			m_nSamplesToGlobalVolRampDest += m_nGlobalVolumeRampAmount;
			step = delta / static_cast<long>(m_nSamplesToGlobalVolRampDest);
		}
	}

	// apply volume and ramping
	if(m_MixerSettings.gnChannels == 1)
	{
		ApplyGlobalVolumeWithRamping<1>(SoundBuffer, RearBuffer, lCount, m_nGlobalVolume, step, m_nSamplesToGlobalVolRampDest, m_lHighResRampingGlobalVolume);
	} else if(m_MixerSettings.gnChannels == 2)
	{
		ApplyGlobalVolumeWithRamping<2>(SoundBuffer, RearBuffer, lCount, m_nGlobalVolume, step, m_nSamplesToGlobalVolRampDest, m_lHighResRampingGlobalVolume);
	} else if(m_MixerSettings.gnChannels == 4)
	{
		ApplyGlobalVolumeWithRamping<4>(SoundBuffer, RearBuffer, lCount, m_nGlobalVolume, step, m_nSamplesToGlobalVolRampDest, m_lHighResRampingGlobalVolume);
	}

}
