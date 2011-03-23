/*
 * OpenMPT
 *
 * Snd_fx.cpp
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *          OpenMPT devs
*/

#include "stdafx.h"
#include "sndfile.h"
// -> CODE#0022
// -> DESC="alternative BPM/Speed interpretation method"
#include "../mptrack/mptrack.h"
#include "../mptrack/moddoc.h"
#include "../mptrack/misc_util.h"
// -! NEW_FEATURE#0022
#include "tuning.h"

#pragma warning(disable:4244)

// Tables defined in tables.cpp
extern BYTE ImpulseTrackerPortaVolCmd[16];
extern WORD S3MFineTuneTable[16];
extern WORD ProTrackerPeriodTable[6*12];
extern WORD ProTrackerTunedPeriods[15*12];
extern WORD FreqS3MTable[];
extern WORD XMPeriodTable[96+8];
extern UINT XMLinearTable[768];
extern DWORD FineLinearSlideUpTable[16];
extern DWORD FineLinearSlideDownTable[16];
extern DWORD LinearSlideUpTable[256];
extern DWORD LinearSlideDownTable[256];
extern signed char retrigTable1[16];
extern signed char retrigTable2[16];
extern short int ModRandomTable[64];
extern BYTE ModEFxTable[16];


////////////////////////////////////////////////////////////
// Length


/*
void CSoundFile::GenerateSamplePosMap() {





	double accurateBufferCount = 

	long lSample = 0;
	nPattern

	//for each order
		//get pattern for this order
		//if pattern if duff: break, we're done.
		//for each row in this pattern
			//get ticks for this row
			//get ticks per row and tempo for this row
			//for each tick

			//Recalc if ticks per row or tempo has changed

		samplesPerTick = (double)gdwMixingFreq * (60.0/(double)m_nMusicTempo / ((double)m_nMusicSpeed * (double)m_nRowsPerBeat));
		lSample += samplesPerTick;


		nPattern = ++nOrder;
	}

}
*/


// Get mod length in various cases. Parameters:
// [in]  adjustMode: See enmGetLengthResetMode for possible adjust modes.
// [in]  endOrder: Order which should be reached (ORDERINDEX_INVALID means whole song)
// [in]  endRow: Row in that order that should be reached
// [out] duration: total time in seconds
// [out] targetReached: true if the specified order/row combination has been reached while going through the module.
// [out] lastOrder: last parsed order (if no target is specified, this is the first order that is parsed twice, i.e. not the *last* played order)
// [out] lastRow: last parsed row (dito)
// [out] endOrder: last order before module loops (UNDEFINED if a target is specified)
// [out] endRow: last row before module loops (dito)
GetLengthType CSoundFile::GetLength(enmGetLengthResetMode adjustMode, ORDERINDEX endOrder, ROWINDEX endRow)
//---------------------------------------------------------------------------------------------------------
{
	GetLengthType retval;
	retval.duration = 0.0;
	retval.targetReached = false;
	retval.lastOrder = retval.endOrder = ORDERINDEX_INVALID;
	retval.lastRow = retval.endRow = ROWINDEX_INVALID;

// -> CODE#0022
// -> DESC="alternative BPM/Speed interpretation method"
//	UINT dwElapsedTime=0, nRow=0, nCurrentPattern=0, nNextPattern=0, nPattern=Order[0];
	ROWINDEX nRow = 0;
	ROWINDEX nNextPatStartRow = 0; // FT2 E60 bug
	ORDERINDEX nCurrentPattern = 0;
	ORDERINDEX nNextPattern = 0;
	PATTERNINDEX nPattern = Order[0];
	double dElapsedTime=0.0;
// -! NEW_FEATURE#0022
	UINT nMusicSpeed = m_nDefaultSpeed, nMusicTempo = m_nDefaultTempo, nNextRow = 0;
	LONG nGlbVol = m_nDefaultGlobalVolume, nOldGlbVolSlide = 0;
	vector<BYTE> instr(m_nChannels, 0);
	vector<UINT> notes(m_nChannels, 0);
	vector<BYTE> vols(m_nChannels, 0xFF);
	vector<BYTE> oldparam(m_nChannels, 0);
	vector<UINT> chnvols(m_nChannels, 64);
	vector<double> patloop(m_nChannels, 0);
	vector<ROWINDEX> patloopstart(m_nChannels, 0);
	VisitedRowsType visitedRows;	// temporary visited rows vector (so that GetLength() won't interfere with the player code if the module is playing at the same time)

	InitializeVisitedRows(true, &visitedRows);

	for(CHANNELINDEX icv = 0; icv < m_nChannels; icv++) chnvols[icv] = ChnSettings[icv].nVolume;
	nCurrentPattern = nNextPattern = 0;
	nPattern = Order[0];
	nRow = nNextRow = 0;

	for (;;)
	{
		UINT nSpeedCount = 0;
		nRow = nNextRow;
		nCurrentPattern = nNextPattern;

		if(nCurrentPattern >= Order.size())
			break;
		
		// Check if pattern is valid
		nPattern = Order[nCurrentPattern];
		bool positionJumpOnThisRow = false;
		bool patternBreakOnThisRow = false;

		while(nPattern >= Patterns.Size())
		{
			// End of song?
			if((nPattern == Order.GetInvalidPatIndex()) || (nCurrentPattern >= Order.size()))
			{
				if(nCurrentPattern == m_nRestartPos)
					break;
				else
					nCurrentPattern = m_nRestartPos;
			} else
			{
				nCurrentPattern++;
			}
			nPattern = (nCurrentPattern < Order.size()) ? Order[nCurrentPattern] : Order.GetInvalidPatIndex();
			nNextPattern = nCurrentPattern;
			if((!Patterns.IsValidPat(nPattern)) && IsRowVisited(nCurrentPattern, 0, true, &visitedRows))
				break;
		}
		// Skip non-existing patterns
		if ((nPattern >= Patterns.Size()) || (!Patterns[nPattern]))
		{
			// If there isn't even a tune, we should probably stop here.
			if(nCurrentPattern == m_nRestartPos)
				break;
			nNextPattern = nCurrentPattern + 1;
			continue;
		}
		// Should never happen
		if (nRow >= Patterns[nPattern].GetNumRows())
			nRow = 0;

		//Check whether target reached.
		if(nCurrentPattern == endOrder && nRow == endRow)
		{
			retval.targetReached = true;
			break;
		}

		if(IsRowVisited(nCurrentPattern, nRow, true, &visitedRows))
			break;

		retval.endOrder = nCurrentPattern;
		retval.endRow = nRow;

		// Update next position
		nNextRow = nRow + 1;

		if (nNextRow >= Patterns[nPattern].GetNumRows())
		{
			nNextPattern = nCurrentPattern + 1;
			nNextRow = 0;
			if(IsCompatibleMode(TRK_FASTTRACKER2)) nNextRow = nNextPatStartRow;  // FT2 E60 bug
			nNextPatStartRow = 0;
		}
		if (!nRow)
		{
			for(UINT ipck = 0; ipck < m_nChannels; ipck++)
				patloop[ipck] = dElapsedTime;
		}

		MODCHANNEL *pChn = Chn;
		MODCOMMAND *p = Patterns[nPattern] + nRow * m_nChannels;
		MODCOMMAND *nextRow = NULL;
		for (CHANNELINDEX nChn = 0; nChn < m_nChannels; p++, pChn++, nChn++) if (*((DWORD *)p))
		{
			if((GetType() == MOD_TYPE_S3M) && (ChnSettings[nChn].dwFlags & CHN_MUTE) != 0)	// not even effects are processed on muted S3M channels
				continue;
			UINT command = p->command;
			UINT param = p->param;
			UINT note = p->note;
			if (p->instr) { instr[nChn] = p->instr; notes[nChn] = 0; vols[nChn] = 0xFF; }
			if ((note) && (note <= NOTE_MAX)) notes[nChn] = note;
			if (p->volcmd == VOLCMD_VOLUME)	{ vols[nChn] = p->vol; }
			if (command) switch (command)
			{
			// Position Jump
			case CMD_POSITIONJUMP:
				positionJumpOnThisRow = true;
				nNextPattern = (ORDERINDEX)param;
				nNextPatStartRow = 0;  // FT2 E60 bug
				// see http://forum.openmpt.org/index.php?topic=2769.0 - FastTracker resets Dxx if Bxx is called _after_ Dxx
				if(!patternBreakOnThisRow || (GetType() == MOD_TYPE_XM))
					nNextRow = 0;

				if ((adjustMode & eAdjust))
				{
					pChn->nPatternLoopCount = 0;
					pChn->nPatternLoop = 0;
				}
				break;
			// Pattern Break
			case CMD_PATTERNBREAK:
				patternBreakOnThisRow = true;
				//Try to check next row for XPARAM
				nextRow = nullptr;
				nNextPatStartRow = 0;  // FT2 E60 bug
				if (nRow < Patterns[nPattern].GetNumRows() - 1)
				{
					nextRow = Patterns[nPattern] + (nRow + 1) * m_nChannels + nChn;
				}
				if (nextRow && nextRow->command == CMD_XPARAM)
				{
					nNextRow = (param << 8) + nextRow->param;
				} else
				{
					nNextRow = param;
				}
						
				if (!positionJumpOnThisRow)
				{
					nNextPattern = nCurrentPattern + 1;
				}
				if ((adjustMode & eAdjust))
				{
					pChn->nPatternLoopCount = 0;
					pChn->nPatternLoop = 0;
				}
				break;
			// Set Speed
			case CMD_SPEED:
				if (!param) break;
				// Allow high speed values here for VBlank MODs. (Maybe it would be better to have a "VBlank MOD" flag somewhere? Is it worth the effort?)
				if ((param <= GetModSpecifications().speedMax) || (m_nType & MOD_TYPE_MOD))
				{
					nMusicSpeed = param;
				}
				break;
			// Set Tempo
			case CMD_TEMPO:
				if ((adjustMode & eAdjust) && (m_nType & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)))
				{
					if (param) pChn->nOldTempo = (BYTE)param; else param = pChn->nOldTempo;
				}
				if (param >= 0x20) nMusicTempo = param; else
				// Tempo Slide
				if ((param & 0xF0) == 0x10)
				{
					nMusicTempo += (param & 0x0F)  * (nMusicSpeed - 1);  //rewbs.tempoSlideFix
				} else
				{
					nMusicTempo -= (param & 0x0F) * (nMusicSpeed - 1); //rewbs.tempoSlideFix
				}
// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
				if(IsCompatibleMode(TRK_ALLTRACKERS))	// clamp tempo correctly in compatible mode
					nMusicTempo = CLAMP(nMusicTempo, 32, 255);
				else
					nMusicTempo = CLAMP(nMusicTempo, GetModSpecifications().tempoMin, GetModSpecifications().tempoMax);
// -! NEW_FEATURE#0010
				break;
			// Pattern Delay
			case CMD_S3MCMDEX:	
				if ((param & 0xF0) == 0x60) { nSpeedCount = param & 0x0F; break; } else
				if ((param & 0xF0) == 0xA0) { pChn->nOldHiOffset = param & 0x0F; break; } else
				if ((param & 0xF0) == 0xB0) { param &= 0x0F; param |= 0x60; }
			case CMD_MODCMDEX:
				if ((param & 0xF0) == 0xE0) nSpeedCount = (param & 0x0F) * nMusicSpeed; else
				if ((param & 0xF0) == 0x60)
				{
					if (param & 0x0F)
					{
						dElapsedTime += (dElapsedTime - patloop[nChn]) * (double)(param & 0x0F);
						nNextPatStartRow = patloopstart[nChn]; // FT2 E60 bug
					} else
					{
						patloop[nChn] = dElapsedTime;
						patloopstart[nChn] = nRow;
					}
				}
				break;
			case CMD_XFINEPORTAUPDOWN:
				// ignore high offset in compatible mode
				if (((param & 0xF0) == 0xA0) && !IsCompatibleMode(TRK_FASTTRACKER2)) pChn->nOldHiOffset = param & 0x0F;
				break;
			}
			if ((adjustMode & eAdjust) == 0) continue;
			switch(command)
			{
			// Portamento Up/Down
			case CMD_PORTAMENTOUP:
			case CMD_PORTAMENTODOWN:
				if (param) pChn->nOldPortaUpDown = param;
				break;
			// Tone-Portamento
			case CMD_TONEPORTAMENTO:
				if (param) pChn->nPortamentoSlide = param << 2;
				break;
			// Offset
			case CMD_OFFSET:
				if (param) pChn->nOldOffset = param;
				break;
			// Volume Slide
			case CMD_VOLUMESLIDE:
			case CMD_TONEPORTAVOL:
			case CMD_VIBRATOVOL:
				if (param) pChn->nOldVolumeSlide = param;
				break;
			// Set Volume
			case CMD_VOLUME:
				vols[nChn] = param;
				break;
			// Global Volume
			case CMD_GLOBALVOLUME:
				if (!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) param <<= 1;
				if(IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2))
				{
					//IT compatibility 16. Both FT2 and IT ignore out-of-range values
					if (param <= 128)
						nGlbVol = param << 1;
				}
				else
				{
					if (param > 128) param = 128;
					nGlbVol = param << 1;
				}
				break;
			// Global Volume Slide
			case CMD_GLOBALVOLSLIDE:
				if(IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2))
				{
					//IT compatibility 16. Global volume slide params are stored per channel (FT2/IT)
					if (param) pChn->nOldGlobalVolSlide = param; else param = pChn->nOldGlobalVolSlide;
				}
				else
				{
					if (param) nOldGlbVolSlide = param; else param = nOldGlbVolSlide;

				}
				if (((param & 0x0F) == 0x0F) && (param & 0xF0))
				{
					param >>= 4;
					if (!(GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))) param <<= 1;
					nGlbVol += param << 1;
				} else
				if (((param & 0xF0) == 0xF0) && (param & 0x0F))
				{
					param = (param & 0x0F) << 1;
					if (!(GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))) param <<= 1;
					nGlbVol -= param;
				} else
				if (param & 0xF0)
				{
					param >>= 4;
					param <<= 1;
					if (!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) param <<= 1;
					nGlbVol += param * nMusicSpeed;
				} else
				{
					param = (param & 0x0F) << 1;
					if (!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) param <<= 1;
					nGlbVol -= param * nMusicSpeed;
				}
				nGlbVol = CLAMP(nGlbVol, 0, 256);
				break;
			case CMD_CHANNELVOLUME:
				if (param <= 64) chnvols[nChn] = param;
				break;
			case CMD_CHANNELVOLSLIDE:
				if (param) oldparam[nChn] = param; else param = oldparam[nChn];
				pChn->nOldChnVolSlide = param;
				if (((param & 0x0F) == 0x0F) && (param & 0xF0))
				{	
					param = (param >> 4) + chnvols[nChn];
				} else
				if (((param & 0xF0) == 0xF0) && (param & 0x0F))
				{
					if (chnvols[nChn] > (int)(param & 0x0F)) param = chnvols[nChn] - (param & 0x0F);
					else param = 0;
				} else
				if (param & 0x0F)
				{
					param = (param & 0x0F) * nMusicSpeed;
					param = (chnvols[nChn] > param) ? chnvols[nChn] - param : 0;
				} else param = ((param & 0xF0) >> 4) * nMusicSpeed + chnvols[nChn];
				param = min(param, 64);
				chnvols[nChn] = param;
				break;
			}
		}
		nSpeedCount += nMusicSpeed;
		switch(m_nTempoMode)
		{
			case tempo_mode_alternative: 
				dElapsedTime +=  60000.0 / (1.65625 * (double)(nMusicSpeed * nMusicTempo)); break;
			case tempo_mode_modern: 
				dElapsedTime += 60000.0 / (double)nMusicTempo / (double)m_nCurrentRowsPerBeat; break;
			case tempo_mode_classic: default:
				dElapsedTime += (2500.0 * (double)nSpeedCount) / (double)nMusicTempo;
		}
	}

	if(retval.targetReached || endOrder == ORDERINDEX_INVALID || endRow == ROWINDEX_INVALID)
	{
		retval.lastOrder = nCurrentPattern;
		retval.lastRow = nRow;
	}
	retval.duration = dElapsedTime / 1000.0;

	// Store final variables
	if ((adjustMode & eAdjust))
	{
		if (retval.targetReached || endOrder == ORDERINDEX_INVALID || endRow == ROWINDEX_INVALID)
		{
			// Target found, or there is no target (i.e. play whole song)...
			m_nGlobalVolume = nGlbVol;
			m_nOldGlbVolSlide = nOldGlbVolSlide;
			m_nMusicSpeed = nMusicSpeed;
			m_nMusicTempo = nMusicTempo;
			for (CHANNELINDEX n = 0; n < m_nChannels; n++)
			{
				Chn[n].nGlobalVol = chnvols[n];
				if (notes[n]) Chn[n].nNewNote = notes[n];
				if (instr[n]) Chn[n].nNewIns = instr[n];
				if (vols[n] != 0xFF)
				{
					if (vols[n] > 64) vols[n] = 64;
					Chn[n].nVolume = vols[n] * 4;
				}
			}
		} else if(adjustMode != eAdjustOnSuccess)
		{
			// Target not found (f.e. when jumping to a hidden sub song), reset global variables...
			m_nMusicSpeed = m_nDefaultSpeed;
			m_nMusicTempo = m_nDefaultTempo;
			m_nGlobalVolume = m_nDefaultGlobalVolume;
		}
		// When adjusting the playback status, we will also want to update the visited rows vector according to the current position.
		m_VisitedRows = visitedRows;
	}

	return retval;

}


//////////////////////////////////////////////////////////////////////////////////////////////////
// Effects

void CSoundFile::InstrumentChange(MODCHANNEL *pChn, UINT instr, bool bPorta, bool bUpdVol, bool bResetEnv)
//--------------------------------------------------------------------------------------------------------
{
	bool bInstrumentChanged = false;

	if (instr >= MAX_INSTRUMENTS) return;
	MODINSTRUMENT *pIns = Instruments[instr];
	MODSAMPLE *pSmp = &Samples[instr];
	UINT note = pChn->nNewNote;

	if(note == NOTE_NONE && IsCompatibleMode(TRK_IMPULSETRACKER)) return;

	if ((pIns) && (note) && (note <= 128))
	{
		if(bPorta && pIns == pChn->pModInstrument && (pChn->pModSample != nullptr && pChn->pModSample->pSample != nullptr) && IsCompatibleMode(TRK_IMPULSETRACKER))
		{
#ifdef DEBUG
			{
				// Check if original behaviour would have been used here
				if (pIns->NoteMap[note-1] >= NOTE_MIN_SPECIAL)
				{
					ASSERT(false);
					return;
				}
				UINT n = pIns->Keyboard[note-1];
				pSmp = ((n) && (n < MAX_SAMPLES)) ? &Samples[n] : nullptr;
				if(pSmp != pChn->pModSample)
				{
					ASSERT(false);
				}
			}
#endif // DEBUG
			// Impulse Tracker doesn't seem to look up the sample for new notes when in instrument mode and it encounters a situation like this:
			// G-6 01 ... ... <-- G-6 is bound to sample 01
			// F-6 01 ... GFF <-- F-6 is bound to sample 02, but sample 01 will be played
			// This behaviour is not used if sample 01 has no actual sample data (hence the "pChn->pModSample->pSample != nullptr")
			// and it is also ignored when the instrument number changes. This fixes f.e. the guitars in "Ultima Ratio" by Nebularia
			// and some slides in spx-shuttledeparture.it.
			pSmp = pChn->pModSample;
		} else
		{
			// Original behaviour
			if(pIns->NoteMap[note-1] > NOTE_MAX) return;
			UINT n = pIns->Keyboard[note-1];
			pSmp = ((n) && (n < MAX_SAMPLES)) ? &Samples[n] : nullptr;
		}
	} else
	if (m_nInstruments)
	{
		if (note >= 0xFE) return;
		pSmp = NULL;
	}

	const bool bNewTuning = (m_nType == MOD_TYPE_MPT && pIns && pIns->pTuning);
	//Playback behavior change for MPT: With portamento don't change sample if it is in
	//the same instrument as previous sample.
	if(bPorta && bNewTuning && pIns == pChn->pModInstrument)
		return;

	bool returnAfterVolumeAdjust = false;
	// bInstrumentChanged is used for IT carry-on env option
	if (pIns != pChn->pModInstrument)
	{
		bInstrumentChanged = true;
		// we will set the new instrument later.
	} 
	else 
	{	
		// Special XM hack
		if ((bPorta) && (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2)) && (pIns)
		&& (pChn->pModSample) && (pSmp != pChn->pModSample))
		{
			// FT2 doesn't change the sample in this case,
			// but still uses the sample info from the old one (bug?)
			returnAfterVolumeAdjust = true;
		}
	}

	// XM compatibility: new instrument + portamento = ignore new instrument number, but reload old instrument settings (the world of XM is upside down...)
	if(bInstrumentChanged && bPorta && IsCompatibleMode(TRK_FASTTRACKER2))
	{
		pIns = pChn->pModInstrument;
		pSmp = pChn->pModSample;
		bInstrumentChanged = false;
	} else
	{
		pChn->pModInstrument = pIns;
	}
	
	// Update Volume
	if (bUpdVol)
	{
		pChn->nVolume = 0;
		if(pSmp)
			pChn->nVolume = pSmp->nVolume;
		else
		{
			if(pIns && pIns->nMixPlug)
				pChn->nVolume = pChn->GetVSTVolume();
		}
	}

	if(returnAfterVolumeAdjust) return;

	
	// Instrument adjust
	pChn->nNewIns = 0;
	
	if (pIns && (pIns->nMixPlug || pSmp))		//rewbs.VSTiNNA
		pChn->nNNA = pIns->nNNA;

	if (pSmp)
	{
		if (pIns)
		{
			pChn->nInsVol = (pSmp->nGlobalVol * pIns->nGlobalVol) >> 6;
			if (pIns->dwFlags & INS_SETPANNING) pChn->nPan = pIns->nPan;
		} else
		{
			pChn->nInsVol = pSmp->nGlobalVol;
		}
		if (pSmp->uFlags & CHN_PANNING) pChn->nPan = pSmp->nPan;
	}


	// Reset envelopes
	if (bResetEnv)
	{
		if ((!bPorta) || (!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) || (m_dwSongFlags & SONG_ITCOMPATGXX)
		 || (!pChn->nLength) || ((pChn->dwFlags & CHN_NOTEFADE) && (!pChn->nFadeOutVol))
		 //IT compatibility tentative fix: Reset envelopes when instrument changes.
		 || (IsCompatibleMode(TRK_IMPULSETRACKER) && bInstrumentChanged))
		{
			pChn->dwFlags |= CHN_FASTVOLRAMP;
			if ((m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)) && (!bInstrumentChanged) && (pIns) && (!(pChn->dwFlags & (CHN_KEYOFF|CHN_NOTEFADE))))
			{
				if (!(pIns->VolEnv.dwFlags & ENV_CARRY)) resetEnvelopes(pChn, ENV_RESET_VOL);
				if (!(pIns->PanEnv.dwFlags & ENV_CARRY)) resetEnvelopes(pChn, ENV_RESET_PAN);
				if (!(pIns->PitchEnv.dwFlags & ENV_CARRY)) resetEnvelopes(pChn, ENV_RESET_PITCH);
			} else
			{
				resetEnvelopes(pChn);
			}
			// IT Compatibility: Autovibrato reset
			if(!IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				pChn->nAutoVibDepth = 0;
				pChn->nAutoVibPos = 0;
			}
		} else if ((pIns) && (!(pIns->VolEnv.dwFlags & ENV_ENABLED)))
		{
			resetEnvelopes(pChn, IsCompatibleMode(TRK_IMPULSETRACKER) ? ENV_RESET_VOL : ENV_RESET_ALL);
		}
	}
	// Invalid sample ?
	if (!pSmp)
	{
		pChn->pModSample = nullptr;
		pChn->nInsVol = 0;
		return;
	}

	// Tone-Portamento doesn't reset the pingpong direction flag
	if ((bPorta) && (pSmp == pChn->pModSample))
	{
		if(m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)) return;
		pChn->dwFlags &= ~(CHN_KEYOFF|CHN_NOTEFADE);
		pChn->dwFlags = (pChn->dwFlags & (CHN_CHANNELFLAGS | CHN_PINGPONGFLAG)) | (pSmp->uFlags & CHN_SAMPLEFLAGS);
	} else
	{
		pChn->dwFlags &= ~(CHN_KEYOFF|CHN_NOTEFADE|CHN_VOLENV|CHN_PANENV|CHN_PITCHENV);

		//IT compatibility tentative fix: Don't change bidi loop direction when 
		//no sample nor instrument is changed.
		if(IsCompatibleMode(TRK_IMPULSETRACKER) && pSmp == pChn->pModSample && !bInstrumentChanged)
			pChn->dwFlags = (pChn->dwFlags & (CHN_CHANNELFLAGS | CHN_PINGPONGFLAG)) | (pSmp->uFlags & CHN_SAMPLEFLAGS);
		else
			pChn->dwFlags = (pChn->dwFlags & CHN_CHANNELFLAGS) | (pSmp->uFlags & CHN_SAMPLEFLAGS);


		if (pIns)
		{
			if (pIns->VolEnv.dwFlags & ENV_ENABLED) pChn->dwFlags |= CHN_VOLENV;
			if (pIns->PanEnv.dwFlags & ENV_ENABLED) pChn->dwFlags |= CHN_PANENV;
			if (pIns->PitchEnv.dwFlags & ENV_ENABLED) pChn->dwFlags |= CHN_PITCHENV;
			if ((pIns->PitchEnv.dwFlags & ENV_ENABLED) && (pIns->PitchEnv.dwFlags & ENV_FILTER))
			{
				pChn->dwFlags |= CHN_FILTERENV;
				if (!pChn->nCutOff) pChn->nCutOff = 0x7F;
			} else
			{
				// Special case: Reset filter envelope flag manually (because of S7D/S7E effects).
				// This way, the S7x effects can be applied to several notes, as long as they don't come with an instrument number.
				pChn->dwFlags &= ~CHN_FILTERENV;
			}
			if (pIns->nIFC & 0x80) pChn->nCutOff = pIns->nIFC & 0x7F;
			if (pIns->nIFR & 0x80) pChn->nResonance = pIns->nIFR & 0x7F;
		}
		pChn->nVolSwing = pChn->nPanSwing = 0;
		pChn->nResSwing = pChn->nCutSwing = 0;
	}
	pChn->pModSample = pSmp;
	pChn->nLength = pSmp->nLength;
	pChn->nLoopStart = pSmp->nLoopStart;
	pChn->nLoopEnd = pSmp->nLoopEnd;

	// IT Compatibility: Autovibrato reset
	if(IsCompatibleMode(TRK_IMPULSETRACKER))
	{
		pChn->nAutoVibDepth = 0;
		pChn->nAutoVibPos = 0;
	}

	if(bNewTuning)
	{
		pChn->nC5Speed = pSmp->nC5Speed;
		pChn->m_CalculateFreq = true;
		pChn->nFineTune = 0;
	}
	else
	{
		pChn->nC5Speed = pSmp->nC5Speed;
		pChn->nFineTune = pSmp->nFineTune;
	}

	
	pChn->pSample = pSmp->pSample;
	pChn->nTranspose = pSmp->RelativeTone;

	pChn->m_PortamentoFineSteps = 0;
	pChn->nPortamentoDest = 0;

	if (pChn->dwFlags & CHN_SUSTAINLOOP)
	{
		pChn->nLoopStart = pSmp->nSustainStart;
		pChn->nLoopEnd = pSmp->nSustainEnd;
		pChn->dwFlags |= CHN_LOOP;
		if (pChn->dwFlags & CHN_PINGPONGSUSTAIN) pChn->dwFlags |= CHN_PINGPONGLOOP;
	}
	if ((pChn->dwFlags & CHN_LOOP) && (pChn->nLoopEnd < pChn->nLength)) pChn->nLength = pChn->nLoopEnd;
}


void CSoundFile::NoteChange(UINT nChn, int note, bool bPorta, bool bResetEnv, bool bManual)
//-----------------------------------------------------------------------------------------
{
	if (note < NOTE_MIN) return;
	MODCHANNEL * const pChn = &Chn[nChn];
	MODSAMPLE *pSmp = pChn->pModSample;
	MODINSTRUMENT *pIns = pChn->pModInstrument;

	const bool bNewTuning = (m_nType == MOD_TYPE_MPT && pIns && pIns->pTuning);

	if ((pIns) && (note <= 0x80))
	{
		UINT n = pIns->Keyboard[note - 1];
		if ((n) && (n < MAX_SAMPLES)) pSmp = &Samples[n];
		note = pIns->NoteMap[note-1];
	}
	// Key Off
	if (note > NOTE_MAX)
	{
		// Key Off (+ Invalid Note for XM - TODO is this correct?)
		if (note == NOTE_KEYOFF || !(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)))
			KeyOff(nChn);
		else // Invalid Note -> Note Fade
		//if (note == NOTE_FADE)
			if(m_nInstruments)	
				pChn->dwFlags |= CHN_NOTEFADE;

		// Note Cut
		if (note == NOTE_NOTECUT)
		{
			pChn->dwFlags |= (CHN_NOTEFADE|CHN_FASTVOLRAMP);
			if ((!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) || (m_nInstruments)) pChn->nVolume = 0;
			pChn->nFadeOutVol = 0;
		}

		//IT compatibility tentative fix: Clear channel note memory.
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			pChn->nNote = NOTE_NONE;
			pChn->nNewNote = NOTE_NONE;
		}
		return;
	}

	if(bNewTuning)
	{
		if(!bPorta || pChn->nNote == NOTE_NONE)
			pChn->nPortamentoDest = 0;
		else
		{
			pChn->nPortamentoDest = pIns->pTuning->GetStepDistance(pChn->nNote, pChn->m_PortamentoFineSteps, note, 0);
			//Here pCnh->nPortamentoDest means 'steps to slide'.
			pChn->m_PortamentoFineSteps = -pChn->nPortamentoDest;
		}
	}

	if ((!bPorta) && (m_nType & (MOD_TYPE_XM|MOD_TYPE_MED|MOD_TYPE_MT2)))
	{
		if (pSmp)
		{
			pChn->nTranspose = pSmp->RelativeTone;
			pChn->nFineTune = pSmp->nFineTune;
		}
	}
	// IT Compatibility: Update multisample instruments frequency even if instrument is not specified (fixes the guitars in spx-shuttledeparture.it)
	if(!bPorta && pSmp && IsCompatibleMode(TRK_IMPULSETRACKER)) pChn->nC5Speed = pSmp->nC5Speed;

	// XM Compatibility: Ignore notes with portamento if there was no note playing.
	if(bPorta && (pChn->nInc == 0) && IsCompatibleMode(TRK_FASTTRACKER2))
	{
		pChn->nPeriod = 0;
		return;
	}

	if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2|MOD_TYPE_MED))
	{
		note += pChn->nTranspose;
		note = CLAMP(note, NOTE_MIN, 131);	// why 131? 120+11, how does this make sense?
	} else
	{
		note = CLAMP(note, NOTE_MIN, NOTE_MAX);
	}
	pChn->nNote = note;
	pChn->m_CalculateFreq = true;

	if ((!bPorta) || (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)))
		pChn->nNewIns = 0;

	UINT period = GetPeriodFromNote(note, pChn->nFineTune, pChn->nC5Speed);

	if (!pSmp) return;
	if (period)
	{
		if ((!bPorta) || (!pChn->nPeriod)) pChn->nPeriod = period;
		if(!bNewTuning) pChn->nPortamentoDest = period;
		if ((!bPorta) || ((!pChn->nLength) && (!(m_nType & MOD_TYPE_S3M))))
		{
			pChn->pModSample = pSmp;
			pChn->pSample = pSmp->pSample;
			pChn->nLength = pSmp->nLength;
			pChn->nLoopEnd = pSmp->nLength;
			pChn->nLoopStart = 0;
			pChn->dwFlags = (pChn->dwFlags & CHN_CHANNELFLAGS) | (pSmp->uFlags & CHN_SAMPLEFLAGS);
			if (pChn->dwFlags & CHN_SUSTAINLOOP)
			{
				pChn->nLoopStart = pSmp->nSustainStart;
				pChn->nLoopEnd = pSmp->nSustainEnd;
				pChn->dwFlags &= ~CHN_PINGPONGLOOP;
				pChn->dwFlags |= CHN_LOOP;
				if (pChn->dwFlags & CHN_PINGPONGSUSTAIN) pChn->dwFlags |= CHN_PINGPONGLOOP;
				if (pChn->nLength > pChn->nLoopEnd) pChn->nLength = pChn->nLoopEnd;
			} else
			if (pChn->dwFlags & CHN_LOOP)
			{
				pChn->nLoopStart = pSmp->nLoopStart;
				pChn->nLoopEnd = pSmp->nLoopEnd;
				if (pChn->nLength > pChn->nLoopEnd) pChn->nLength = pChn->nLoopEnd;
			}
			pChn->nPos = 0;
			pChn->nPosLo = 0;
			// Handle "retrigger" waveform type
			if (pChn->nVibratoType < 4)
			{
				// IT Compatibilty: Slightly different waveform offsets (why does MPT have two different offsets here with IT old effects enabled and disabled?)
				if(!IsCompatibleMode(TRK_IMPULSETRACKER) && (GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)) && (!(m_dwSongFlags & SONG_ITOLDEFFECTS)))
					pChn->nVibratoPos = 0x10;
				else
					pChn->nVibratoPos = 0;
			}
			// IT Compatibility: No "retrigger" waveform here
			if(!IsCompatibleMode(TRK_IMPULSETRACKER) && pChn->nTremoloType < 4)
			{
				pChn->nTremoloPos = 0;
			}
		}
		if (pChn->nPos >= pChn->nLength) pChn->nPos = pChn->nLoopStart;
	} 
	else 
		bPorta = false;

	if ((!bPorta) || (!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)))
	 || ((pChn->dwFlags & CHN_NOTEFADE) && (!pChn->nFadeOutVol))
	 || ((m_dwSongFlags & SONG_ITCOMPATGXX) && (pChn->nRowInstr)))
	{
		if ((m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)) && (pChn->dwFlags & CHN_NOTEFADE) && (!pChn->nFadeOutVol))
		{
			resetEnvelopes(pChn);
			// IT Compatibility: Autovibrato reset
			if(!IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				pChn->nAutoVibDepth = 0;
				pChn->nAutoVibPos = 0;
			}
			pChn->dwFlags &= ~CHN_NOTEFADE;
			pChn->nFadeOutVol = 65536;
		}
		if ((!bPorta) || (!(m_dwSongFlags & SONG_ITCOMPATGXX)) || (pChn->nRowInstr))
		{
			if ((!(m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))) || (pChn->nRowInstr))
			{
				pChn->dwFlags &= ~CHN_NOTEFADE;
				pChn->nFadeOutVol = 65536;
			}
		}
	}
	pChn->dwFlags &= ~(CHN_EXTRALOUD|CHN_KEYOFF);
	// Enable Ramping
	if (!bPorta)
	{
		pChn->nVUMeter = 0x100;
		pChn->nLeftVU = pChn->nRightVU = 0xFF;
		pChn->dwFlags &= ~CHN_FILTER;
		pChn->dwFlags |= CHN_FASTVOLRAMP;
		// IT Compatibility: Autovibrato reset
		if(bResetEnv && IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			pChn->nAutoVibDepth = 0;
			pChn->nAutoVibPos = 0;
			pChn->nVibratoPos = 0;
		}
		//IT compatibility 15. Retrigger will not be reset (Tremor doesn't store anything here, so we just don't reset this as well)
		if(!IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			// XM compatibility: FT2 also doesn't reset retrigger
			if(!IsCompatibleMode(TRK_FASTTRACKER2)) pChn->nRetrigCount = 0;
			pChn->nTremorCount = 0;
		}
		if (bResetEnv)
		{
			pChn->nVolSwing = pChn->nPanSwing = 0;
			pChn->nResSwing = pChn->nCutSwing = 0;
			if (pIns)
			{
				// IT compatibility tentative fix: Reset NNA action on every new note, even without instrument number next to note (fixes spx-farspacedance.it, but is this actually 100% correct?)
				if(IsCompatibleMode(TRK_IMPULSETRACKER)) pChn->nNNA = pIns->nNNA;
				if (!(pIns->VolEnv.dwFlags & ENV_CARRY)) pChn->VolEnv.nEnvPosition = 0;
				if (!(pIns->PanEnv.dwFlags & ENV_CARRY)) pChn->PanEnv.nEnvPosition = 0;
				if (!(pIns->PitchEnv.dwFlags & ENV_CARRY)) pChn->PitchEnv.nEnvPosition = 0;
				if (m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))
				{
					// Volume Swing
					if (pIns->nVolSwing)
					{
						// IT compatibility: MPT has a weird vol swing algorithm....
						if(IsCompatibleMode(TRK_IMPULSETRACKER))
						{
							double d = 2 * (((double) rand()) / RAND_MAX) - 1;
							pChn->nVolSwing = d * pIns->nVolSwing / 100.0 * pChn->nVolume;
						} else
						{
							int d = ((LONG)pIns->nVolSwing * (LONG)((rand() & 0xFF) - 0x7F)) / 128;
							pChn->nVolSwing = (signed short)((d * pChn->nVolume + 1)/128);
						}
					}
					// Pan Swing
					if (pIns->nPanSwing)
					{
						// IT compatibility: MPT has a weird pan swing algorithm....
						if(IsCompatibleMode(TRK_IMPULSETRACKER))
						{
							double d = 2 * (((double) rand()) / RAND_MAX) - 1;
							pChn->nPanSwing = d * pIns->nPanSwing * 4;
						} else
						{
							int d = ((LONG)pIns->nPanSwing * (LONG)((rand() & 0xFF) - 0x7F)) / 128;
							pChn->nPanSwing = (signed short)d;
							pChn->nRestorePanOnNewNote = pChn->nPan + 1;
						}
					}
					// Cutoff Swing
					if (pIns->nCutSwing)
					{
						int d = ((LONG)pIns->nCutSwing * (LONG)((rand() & 0xFF) - 0x7F)) / 128;
						pChn->nCutSwing = (signed short)((d * pChn->nCutOff + 1)/128);
						pChn->nRestoreCutoffOnNewNote = pChn->nCutOff + 1;
					}
					// Resonance Swing
					if (pIns->nResSwing)
					{
						int d = ((LONG)pIns->nResSwing * (LONG)((rand() & 0xFF) - 0x7F)) / 128;
						pChn->nResSwing = (signed short)((d * pChn->nResonance + 1)/128);
						pChn->nRestoreResonanceOnNewNote = pChn->nResonance + 1;
					}
				}
			}
			// IT Compatibility: Autovibrato reset
			if(!IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				pChn->nAutoVibDepth = 0;
				pChn->nAutoVibPos = 0;
			}
		}
		pChn->nLeftVol = pChn->nRightVol = 0;
		bool bFlt = (m_dwSongFlags & SONG_MPTFILTERMODE) ? false : true;
		// Setup Initial Filter for this note
		if (pIns)
		{
			if (pIns->nIFR & 0x80) { pChn->nResonance = pIns->nIFR & 0x7F; bFlt = true; }
			if (pIns->nIFC & 0x80) { pChn->nCutOff = pIns->nIFC & 0x7F; bFlt = true; }
			if (bFlt && (pIns->nFilterMode != FLTMODE_UNCHANGED))
			{
				pChn->nFilterMode = pIns->nFilterMode;
			}
		} else
		{
			pChn->nVolSwing = pChn->nPanSwing = 0;
			pChn->nCutSwing = pChn->nResSwing = 0;
		}
#ifndef NO_FILTER
		if ((pChn->nCutOff < 0x7F) && (bFlt)) SetupChannelFilter(pChn, true);
#endif // NO_FILTER
	}
	// Special case for MPT
	if (bManual) pChn->dwFlags &= ~CHN_MUTE;
	if (((pChn->dwFlags & CHN_MUTE) && (gdwSoundSetup & SNDMIX_MUTECHNMODE))
	 || ((pChn->pModSample) && (pChn->pModSample->uFlags & CHN_MUTE) && (!bManual))
	 || ((pChn->pModInstrument) && (pChn->pModInstrument->dwFlags & INS_MUTE) && (!bManual)))
	{
		if (!bManual) pChn->nPeriod = 0;
	}

}


UINT CSoundFile::GetNNAChannel(UINT nChn) const
//---------------------------------------------
{
	const MODCHANNEL *pChn = &Chn[nChn];
	// Check for empty channel
	const MODCHANNEL *pi = &Chn[m_nChannels];
	for (UINT i=m_nChannels; i<MAX_CHANNELS; i++, pi++) if (!pi->nLength) return i;
	if (!pChn->nFadeOutVol) return 0;
	// All channels are used: check for lowest volume
	UINT result = 0;
	DWORD vol = 64*65536;	// 25%
	DWORD envpos = 0xFFFFFF;
	const MODCHANNEL *pj = &Chn[m_nChannels];
	for (UINT j=m_nChannels; j<MAX_CHANNELS; j++, pj++)
	{
		if (!pj->nFadeOutVol) return j;
		DWORD v = pj->nVolume;
		if (pj->dwFlags & CHN_NOTEFADE)
			v = v * pj->nFadeOutVol;
		else
			v <<= 16;
		if (pj->dwFlags & CHN_LOOP) v >>= 1;
		if ((v < vol) || ((v == vol) && (pj->VolEnv.nEnvPosition > envpos)))
		{
			envpos = pj->VolEnv.nEnvPosition;
			vol = v;
			result = j;
		}
	}
	return result;
}


void CSoundFile::CheckNNA(UINT nChn, UINT instr, int note, BOOL bForceCut)
//------------------------------------------------------------------------
{
	MODCHANNEL *pChn = &Chn[nChn];
	MODINSTRUMENT* pHeader = 0;
	LPSTR pSample;
	if (note > 0x80) note = NOTE_NONE;
	if (note < 1) return;
	// Always NNA cut - using
	if ((!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_MT2))) || (!m_nInstruments) || (bForceCut))
	{
		if ((m_dwSongFlags & SONG_CPUVERYHIGH)
		 || (!pChn->nLength) || (pChn->dwFlags & CHN_MUTE)
		 || ((!pChn->nLeftVol) && (!pChn->nRightVol))) return;
		UINT n = GetNNAChannel(nChn);
		if (!n) return;
		MODCHANNEL *p = &Chn[n];
		// Copy Channel
		*p = *pChn;
		p->dwFlags &= ~(CHN_VIBRATO|CHN_TREMOLO|CHN_PANBRELLO|CHN_MUTE|CHN_PORTAMENTO);
		p->nMasterChn = nChn+1;
		p->nCommand = 0;
		// Cut the note
		p->nFadeOutVol = 0;
		p->dwFlags |= (CHN_NOTEFADE|CHN_FASTVOLRAMP);
		// Stop this channel
		pChn->nLength = pChn->nPos = pChn->nPosLo = 0;
		pChn->nROfs = pChn->nLOfs = 0;
		pChn->nLeftVol = pChn->nRightVol = 0;
		return;
	}
	if (instr >= MAX_INSTRUMENTS) instr = 0;
	pSample = pChn->pSample;
	pHeader = pChn->pModInstrument;
	if ((instr) && (note))
	{
		pHeader = Instruments[instr];
		if (pHeader)
		{
			UINT n = 0;
			if (note <= 0x80)
			{
				n = pHeader->Keyboard[note-1];
				note = pHeader->NoteMap[note-1];
				if ((n) && (n < MAX_SAMPLES)) pSample = Samples[n].pSample;
			}
		} else pSample = nullptr;
	}
	MODCHANNEL *p = pChn;
	//if (!pIns) return;
	if (pChn->dwFlags & CHN_MUTE) return;

	bool applyDNAtoPlug;	//rewbs.VSTiNNA
	for (UINT i=nChn; i<MAX_CHANNELS; p++, i++)
	if ((i >= m_nChannels) || (p == pChn))
	{
		applyDNAtoPlug = false; //rewbs.VSTiNNA
		if (((p->nMasterChn == nChn+1) || (p == pChn)) && (p->pModInstrument))
		{
			bool bOk = false;
			// Duplicate Check Type
			switch(p->pModInstrument->nDCT)
			{
			// Note
			case DCT_NOTE:
				if ((note) && (p->nNote == note) && (pHeader == p->pModInstrument)) bOk = true;
				if (pHeader && pHeader->nMixPlug) applyDNAtoPlug = true; //rewbs.VSTiNNA
				break;
			// Sample
			case DCT_SAMPLE:
				if ((pSample) && (pSample == p->pSample)) bOk = true;
				break;
			// Instrument
			case DCT_INSTRUMENT:
				if (pHeader == p->pModInstrument) bOk = true;
				//rewbs.VSTiNNA
				if (pHeader && pHeader->nMixPlug) applyDNAtoPlug = true;
				break;
			// Plugin
			case DCT_PLUGIN:
				if (pHeader && (pHeader->nMixPlug) && (pHeader->nMixPlug == p->pModInstrument->nMixPlug))
				{
					applyDNAtoPlug = true;
					bOk = true;
				}
				//end rewbs.VSTiNNA
				break;

			}
			// Duplicate Note Action
			if (bOk)
			{
				//rewbs.VSTiNNA
				if (applyDNAtoPlug)
				{
					switch(p->pModInstrument->nDNA)
					{
					case DNA_NOTECUT:
					case DNA_NOTEOFF:
					case DNA_NOTEFADE:	
						//switch off duplicated note played on this plugin 
						IMixPlugin *pPlugin =  m_MixPlugins[pHeader->nMixPlug-1].pMixPlugin;
						if (pPlugin && p->nNote)
							pPlugin->MidiCommand(p->pModInstrument->nMidiChannel, p->pModInstrument->nMidiProgram, p->pModInstrument->wMidiBank, p->nNote + NOTE_KEYOFF, 0, i);
						break;
					}
				}
				//end rewbs.VSTiNNA

				switch(p->pModInstrument->nDNA)
				{
				// Cut
				case DNA_NOTECUT:
					KeyOff(i);
					p->nVolume = 0;
					break;
				// Note Off
				case DNA_NOTEOFF:
					KeyOff(i);
					break;
				// Note Fade
				case DNA_NOTEFADE:
					p->dwFlags |= CHN_NOTEFADE;
					break;
				}
				if (!p->nVolume)
				{
					p->nFadeOutVol = 0;
					p->dwFlags |= (CHN_NOTEFADE|CHN_FASTVOLRAMP);
				}
			}
		}
	}
	
	//rewbs.VSTiNNA
	// Do we need to apply New/Duplicate Note Action to a VSTi?
	bool applyNNAtoPlug = false;
	IMixPlugin *pPlugin = NULL;
	if (pChn->pModInstrument && pChn->pModInstrument->nMidiChannel > 0 && pChn->pModInstrument->nMidiChannel < 17 && pChn->nNote>0 && pChn->nNote<128) // instro sends to a midi chan
	{
		UINT nPlugin = GetBestPlugin(nChn, PRIORITISE_INSTRUMENT, RESPECT_MUTES);
		/*
		UINT nPlugin = 0;
		nPlugin = pChn->pModInstrument->nMixPlug;  		   // first try intrument VST
		if ((!nPlugin) || (nPlugin > MAX_MIXPLUGINS))  // Then try Channel VST
			nPlugin = ChnSettings[nChn].nMixPlugin;			
		*/
		if ((nPlugin) && (nPlugin <= MAX_MIXPLUGINS))
		{ 
			pPlugin =  m_MixPlugins[nPlugin-1].pMixPlugin;
			if (pPlugin) 
			{
				// apply NNA to this Plug iff this plug is currently playing a note on this tracking chan
				// (and if it is playing a note, we know that would be the last note played on this chan).
				applyNNAtoPlug = pPlugin->isPlaying(pChn->nNote, pChn->pModInstrument->nMidiChannel, nChn);
			}
		}
	}
	//end rewbs.VSTiNNA	

	// New Note Action
	//if ((pChn->nVolume) && (pChn->nLength))
	if (((pChn->nVolume) && (pChn->nLength)) || applyNNAtoPlug) //rewbs.VSTiNNA
	{
		UINT n = GetNNAChannel(nChn);
		if (n)
		{
			MODCHANNEL *p = &Chn[n];
			// Copy Channel
			*p = *pChn;
			p->dwFlags &= ~(CHN_VIBRATO|CHN_TREMOLO|CHN_PANBRELLO|CHN_MUTE|CHN_PORTAMENTO);
			
			//rewbs: Copy mute and FX status from master chan.
			//I'd like to copy other flags too, but this would change playback behaviour.
			p->dwFlags |= (pChn->dwFlags & (CHN_MUTE|CHN_NOFX));

			p->nMasterChn = nChn+1;
			p->nCommand = 0;
			//rewbs.VSTiNNA	
			if (applyNNAtoPlug && pPlugin)
			{
				//Move note to the NNA channel (odd, but makes sense with DNA stuff).
				//Actually a bad idea since it then become very hard to kill some notes.
				//pPlugin->MoveNote(pChn->nNote, pChn->pModInstrument->nMidiChannel, nChn, n);
				switch(pChn->nNNA)
				{
				case NNA_NOTEOFF:
				case NNA_NOTECUT:
				case NNA_NOTEFADE:	
					//switch off note played on this plugin, on this tracker channel and midi channel 
					//pPlugin->MidiCommand(pChn->pModInstrument->nMidiChannel, pChn->pModInstrument->nMidiProgram, pChn->nNote+0xFF, 0, n);
					pPlugin->MidiCommand(pChn->pModInstrument->nMidiChannel, pChn->pModInstrument->nMidiProgram, pChn->pModInstrument->wMidiBank, /*pChn->nNote+*/NOTE_KEYOFF, 0, nChn);
					break;
				}
			}
			//end rewbs.VSTiNNA	
			// Key Off the note
			switch(pChn->nNNA)
			{
			case NNA_NOTEOFF:	KeyOff(n); break;
			case NNA_NOTECUT:
				p->nFadeOutVol = 0;
			case NNA_NOTEFADE:	p->dwFlags |= CHN_NOTEFADE; break;
			}
			if (!p->nVolume)
			{
				p->nFadeOutVol = 0;
				p->dwFlags |= (CHN_NOTEFADE|CHN_FASTVOLRAMP);
			}
			// Stop this channel
			pChn->nLength = pChn->nPos = pChn->nPosLo = 0;
			pChn->nROfs = pChn->nLOfs = 0;
		}
	}
}


BOOL CSoundFile::ProcessEffects()
//-------------------------------
{
	MODCHANNEL *pChn = Chn;
	ROWINDEX nBreakRow = ROWINDEX_INVALID, nPatLoopRow = ROWINDEX_INVALID;
	ORDERINDEX nPosJump = ORDERINDEX_INVALID;

// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
	MODCOMMAND* m = nullptr;
// -! NEW_FEATURE#0010
	for (CHANNELINDEX nChn = 0; nChn < m_nChannels; nChn++, pChn++)
	{
		UINT instr = pChn->nRowInstr;
		UINT volcmd = pChn->nRowVolCmd;
		UINT vol = pChn->nRowVolume;
		UINT cmd = pChn->nRowCommand;
		UINT param = pChn->nRowParam;
		bool bPorta = ((cmd != CMD_TONEPORTAMENTO) && (cmd != CMD_TONEPORTAVOL) && (volcmd != VOLCMD_TONEPORTAMENTO)) ? false : true;

		UINT nStartTick = 0;

		pChn->dwFlags &= ~CHN_FASTVOLRAMP;

		// Process parameter control note.
		if(pChn->nRowNote == NOTE_PC)
		{
			const PLUGINDEX plug = pChn->nRowInstr;
			const PlugParamIndex plugparam = MODCOMMAND::GetValueVolCol(pChn->nRowVolCmd, pChn->nRowVolume);
			const PlugParamValue value = MODCOMMAND::GetValueEffectCol(pChn->nRowCommand, pChn->nRowParam) / PlugParamValue(MODCOMMAND::maxColumnValue);

			if(plug > 0 && plug <= MAX_MIXPLUGINS && m_MixPlugins[plug-1].pMixPlugin)
				m_MixPlugins[plug-1].pMixPlugin->SetParameter(plugparam, value);
		}

		// Process continuous parameter control note.
		// Row data is cleared after first tick so on following
		// ticks using channels m_nPlugParamValueStep to identify
		// the need for parameter control. The condition cmd == 0
		// is to make sure that m_nPlugParamValueStep != 0 because 
		// of NOTE_PCS, not because of macro.
		if(pChn->nRowNote == NOTE_PCS || (cmd == 0 && pChn->m_nPlugParamValueStep != 0))
		{
			const bool isFirstTick = (m_dwSongFlags & SONG_FIRSTTICK) != 0;
			if(isFirstTick)
				pChn->m_RowPlug = pChn->nRowInstr;
			const PLUGINDEX nPlug = pChn->m_RowPlug;
			const bool hasValidPlug = (nPlug > 0 && nPlug <= MAX_MIXPLUGINS && m_MixPlugins[nPlug-1].pMixPlugin);
			if(hasValidPlug)
			{
				if(isFirstTick)
					pChn->m_RowPlugParam = MODCOMMAND::GetValueVolCol(pChn->nRowVolCmd, pChn->nRowVolume);
				const PlugParamIndex plugparam = pChn->m_RowPlugParam;
				if(isFirstTick)
				{
					PlugParamValue targetvalue = MODCOMMAND::GetValueEffectCol(pChn->nRowCommand, pChn->nRowParam) / PlugParamValue(MODCOMMAND::maxColumnValue);
					// Hack: Use m_nPlugInitialParamValue to store the target value, not initial.
					pChn->m_nPlugInitialParamValue = targetvalue;
					pChn->m_nPlugParamValueStep = (targetvalue - m_MixPlugins[nPlug-1].pMixPlugin->GetParameter(plugparam)) / float(m_nMusicSpeed);
				}
				if(m_nTickCount + 1 == m_nMusicSpeed)
				{	// On last tick, set parameter exactly to target value.
					// Note: m_nPlugInitialParamValue is used to store the target value,
					//		 not the initial value as the name suggests.
					m_MixPlugins[nPlug-1].pMixPlugin->SetParameter(plugparam, pChn->m_nPlugInitialParamValue);
				}
				else
					m_MixPlugins[nPlug-1].pMixPlugin->ModifyParameter(plugparam, pChn->m_nPlugParamValueStep);
			}
		}

		// Apart from changing parameters, parameter control notes are intended to be 'invisible'.
		// To achieve this, clearing the note data so that rest of the process sees the row as empty row.
		if(MODCOMMAND::IsPcNote(pChn->nRowNote))
		{
			pChn->ClearRowCmd();
			instr = 0;
			volcmd = 0;
			vol = 0;
			cmd = 0;
			param = 0;
			bPorta = false;
		}

		// Process Invert Loop (MOD Effect, called every row)
		if((m_dwSongFlags & SONG_FIRSTTICK) == 0) InvertLoop(&Chn[nChn]);

		// Process special effects (note delay, pattern delay, pattern loop)
		if (cmd == CMD_DELAYCUT)
		{
			//:xy --> note delay until tick x, note cut at tick x+y
			nStartTick = (param & 0xF0) >> 4;
			int cutAtTick = nStartTick + (param & 0x0F);
			NoteCut(nChn, cutAtTick);
		} else
		if ((cmd == CMD_MODCMDEX) || (cmd == CMD_S3MCMDEX))
		{
			if ((!param) && (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))) param = pChn->nOldCmdEx; else pChn->nOldCmdEx = param;
			// Note Delay ?
			if ((param & 0xF0) == 0xD0)
			{
				nStartTick = param & 0x0F;
				if(nStartTick == 0)
				{
					//IT compatibility 22. SD0 == SD1
					if(IsCompatibleMode(TRK_IMPULSETRACKER))
						nStartTick = 1;
					//ST3 ignores notes with SD0 completely
					else if(GetType() == MOD_TYPE_S3M)
						nStartTick = UINT_MAX;
				}

				//IT compatibility 08. Handling of out-of-range delay command.
				if(nStartTick >= m_nMusicSpeed && IsCompatibleMode(TRK_IMPULSETRACKER))
				{
					if(instr)
					{
						if(GetNumInstruments() < 1 && instr < MAX_SAMPLES)
						{
							pChn->pModSample = &Samples[instr];
						}
						else
						{	
							if(instr < MAX_INSTRUMENTS)
								pChn->pModInstrument = Instruments[instr];
						}
					}
					continue;
				}
			} else
			if(m_dwSongFlags & SONG_FIRSTTICK)
			{
				// Pattern Loop ?
				if ((((param & 0xF0) == 0x60) && (cmd == CMD_MODCMDEX))
				 || (((param & 0xF0) == 0xB0) && (cmd == CMD_S3MCMDEX)))
				{
					int nloop = PatternLoop(pChn, param & 0x0F);
					if (nloop >= 0) nPatLoopRow = nloop;
				} else
				// Pattern Delay
				if ((param & 0xF0) == 0xE0)
				{
					m_nPatternDelay = param & 0x0F;
				}
			}
		}
		
		// Handles note/instrument/volume changes
		if (m_nTickCount == nStartTick) // can be delayed by a note delay effect
		{
			UINT note = pChn->nRowNote;
			if (instr) pChn->nNewIns = instr;
			bool retrigEnv = (!note) && (instr);
			
			// Now it's time for some FT2 crap...
			if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))
			{
				// XM: FT2 ignores a note next to a K00 effect, and a fade-out seems to be done when no volume envelope is present (not exactly the Kxx behaviour)
				if(cmd == CMD_KEYOFF && param == 0 && IsCompatibleMode(TRK_FASTTRACKER2))
				{
					note = instr = 0;
					retrigEnv = false;
				}

				// XM: Key-Off + Sample == Note Cut (BUT: Only if no instr number or volume effect is present!)
				if ((note == NOTE_KEYOFF) && ((!instr && !vol && cmd != CMD_VOLUME) || !IsCompatibleMode(TRK_FASTTRACKER2)) && ((!pChn->pModInstrument) || (!(pChn->pModInstrument->VolEnv.dwFlags & ENV_ENABLED))))
				{
					pChn->dwFlags |= CHN_FASTVOLRAMP;
					pChn->nVolume = 0;
					note = instr = 0;
					retrigEnv = false;
				} else if(IsCompatibleMode(TRK_FASTTRACKER2) && !(m_dwSongFlags & SONG_FIRSTTICK))
				{
					// XM Compatibility: Some special hacks for rogue note delays... (EDx with x > 0)
					// Apparently anything that is next to a note delay behaves totally unpredictable in FT2. Swedish tracker logic. :)

					// If there's a note delay but no note, retrig the last note.
					if(note == NOTE_NONE)
					{
						note = pChn->nNote - pChn->nTranspose;
					} else if(note >= NOTE_MIN_SPECIAL)
					{
						// Gah! Note Off + Note Delay will cause envelopes to *retrigger*! How stupid is that?
						retrigEnv = true;
					}
					// Stupid HACK to retrieve the last used instrument *number* for rouge note delays in XM
					// This is only applied if the instrument column is empty and if there is either no note or a "normal" note (e.g. no note off)
					if(instr == 0 && note <= NOTE_MAX)
					{
						for(INSTRUMENTINDEX nIns = 1; nIns <= m_nInstruments; nIns++)
						{
							if(Instruments[nIns] == pChn->pModInstrument)
							{
								instr = nIns;
								break;
							}
						}
					}
				}
			}
			if (retrigEnv) //Case: instrument with no note data. 
			{
				//IT compatibility: Instrument with no note.
				if(IsCompatibleMode(TRK_IMPULSETRACKER) || (m_dwSongFlags & SONG_PT1XMODE))
				{
					if(m_nInstruments)
					{
						if(instr < MAX_INSTRUMENTS && pChn->pModInstrument != Instruments[instr])
							note = pChn->nNote;
					}
					else //Case: Only samples used
					{
						if(instr < MAX_SAMPLES && pChn->pSample != Samples[instr].pSample)
							note = pChn->nNote;
					}
				}

				if (m_nInstruments)
				{
					if (pChn->pModSample) pChn->nVolume = pChn->pModSample->nVolume;
					if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))
					{
						pChn->dwFlags |= CHN_FASTVOLRAMP;
						resetEnvelopes(pChn);	
						// IT Compatibility: Autovibrato reset
						if(!IsCompatibleMode(TRK_IMPULSETRACKER))
						{
							pChn->nAutoVibDepth = 0;
							pChn->nAutoVibPos = 0;
						}
						pChn->dwFlags &= ~CHN_NOTEFADE;
						pChn->nFadeOutVol = 65536;
					}
				} else //Case: Only samples are used; no instruments.
				{
					if (instr < MAX_SAMPLES) pChn->nVolume = Samples[instr].nVolume;
				}
				if (!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) instr = 0;
			}
			// Invalid Instrument ?
			if (instr >= MAX_INSTRUMENTS) instr = 0;

			// Note Cut/Off/Fade => ignore instrument
			if (note >= NOTE_MIN_SPECIAL) instr = 0;

			if ((note) && (note <= NOTE_MAX)) pChn->nNewNote = note;

			// New Note Action ?
			if ((note) && (note <= NOTE_MAX) && (!bPorta))
			{
				CheckNNA(nChn, instr, note, FALSE);
			}

		//rewbs.VSTnoteDelay
		#ifdef MODPLUG_TRACKER
//			if (m_nInstruments) ProcessMidiOut(nChn, pChn); 
		#endif // MODPLUG_TRACKER
		//end rewbs.VSTnoteDelay

			if(note)
			{
				if(pChn->nRestorePanOnNewNote > 0)
				{
					pChn->nPan = pChn->nRestorePanOnNewNote - 1;
					pChn->nRestorePanOnNewNote = 0;
				}
				if(pChn->nRestoreResonanceOnNewNote > 0)
				{
					pChn->nResonance = pChn->nRestoreResonanceOnNewNote - 1;
					pChn->nRestoreResonanceOnNewNote = 0;
				}
				if(pChn->nRestoreCutoffOnNewNote > 0)
				{
					pChn->nCutOff = pChn->nRestoreCutoffOnNewNote - 1;
					pChn->nRestoreCutoffOnNewNote = 0;
				}
				
			}

			// Instrument Change ?
			if (instr)
			{
				MODSAMPLE *psmp = pChn->pModSample;
				InstrumentChange(pChn, instr, bPorta, true);
				pChn->nNewIns = 0;
				// Special IT case: portamento+note causes sample change -> ignore portamento
				if ((m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
				 && (psmp != pChn->pModSample) && (note) && (note < 0x80))
				{
					bPorta = false;
				}
			}
			// New Note ?
			if (note)
			{
				if ((!instr) && (pChn->nNewIns) && (note < 0x80))
				{
					InstrumentChange(pChn, pChn->nNewIns, bPorta, false, (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2)) ? false : true);
					pChn->nNewIns = 0;
				}
				NoteChange(nChn, note, bPorta, (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2)) ? false : true);
				if ((bPorta) && (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2)) && (instr))
				{
					pChn->dwFlags |= CHN_FASTVOLRAMP;
					resetEnvelopes(pChn);
					// IT Compatibility: Autovibrato reset
					if(!IsCompatibleMode(TRK_IMPULSETRACKER))
					{
						pChn->nAutoVibDepth = 0;
						pChn->nAutoVibPos = 0;
					}
				}
			}
			// Tick-0 only volume commands
			if (volcmd == VOLCMD_VOLUME)
			{
				if (vol > 64) vol = 64;
				pChn->nVolume = vol << 2;
				pChn->dwFlags |= CHN_FASTVOLRAMP;
			} else
			if (volcmd == VOLCMD_PANNING)
			{
				if (vol > 64) vol = 64;
				pChn->nPan = vol << 2;
				pChn->dwFlags |= CHN_FASTVOLRAMP;
				pChn->nRestorePanOnNewNote = 0;
				//IT compatibility 20. Set pan overrides random pan
				if(IsCompatibleMode(TRK_IMPULSETRACKER))
					pChn->nPanSwing = 0;
			}

		//rewbs.VSTnoteDelay
		#ifdef MODPLUG_TRACKER
			if (m_nInstruments) ProcessMidiOut(nChn, pChn); 
		#endif // MODPLUG_TRACKER
		}

		if((GetType() == MOD_TYPE_S3M) && (ChnSettings[nChn].dwFlags & CHN_MUTE) != 0)	// not even effects are processed on muted S3M channels
			continue;

		// Volume Column Effect (except volume & panning)
		/*	A few notes, paraphrased from ITTECH.TXT by Storlek (creator of schismtracker):
			Ex/Fx/Gx are shared with Exx/Fxx/Gxx; Ex/Fx are 4x the 'normal' slide value
			Gx is linked with Ex/Fx if Compat Gxx is off, just like Gxx is with Exx/Fxx
			Gx values: 1, 4, 8, 16, 32, 64, 96, 128, 255
			Ax/Bx/Cx/Dx values are used directly (i.e. D9 == D09), and are NOT shared with Dxx
			(value is stored into nOldVolParam and used by A0/B0/C0/D0)
			Hx uses the same value as Hxx and Uxx, and affects the *depth*
			so... hxx = (hx | (oldhxx & 0xf0))  ???
			TODO is this done correctly?
		*/
      	if ((volcmd > VOLCMD_PANNING) && (m_nTickCount >= nStartTick))
		{
			if (volcmd == VOLCMD_TONEPORTAMENTO)
			{
				if (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT))
					TonePortamento(pChn, ImpulseTrackerPortaVolCmd[vol & 0x0F]);
				else
					TonePortamento(pChn, vol * 16);
			} else
			{
				// XM Compatibility: FT2 ignores some voluem commands with parameter = 0.
				if(IsCompatibleMode(TRK_FASTTRACKER2) && vol == 0)
				{
					switch(volcmd)
					{
					case VOLCMD_VOLUME:
					case VOLCMD_PANNING:
					case VOLCMD_VIBRATODEPTH:
					case VOLCMD_TONEPORTAMENTO:
						break;
					default:
						// no memory here.
						volcmd = VOLCMD_NONE;
					}
					
				} else
				{
					if(vol) pChn->nOldVolParam = vol; else vol = pChn->nOldVolParam;
				}

				switch(volcmd)
				{
				case VOLCMD_VOLSLIDEUP:
					VolumeSlide(pChn, vol << 4);
					break;

				case VOLCMD_VOLSLIDEDOWN:
					VolumeSlide(pChn, vol);
					break;

				case VOLCMD_FINEVOLUP:
					if (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT))
					{
						if (m_nTickCount == nStartTick) VolumeSlide(pChn, (vol << 4) | 0x0F);
					} else
						FineVolumeUp(pChn, vol);
					break;

				case VOLCMD_FINEVOLDOWN:
					if (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT))
					{
						if (m_nTickCount == nStartTick) VolumeSlide(pChn, 0xF0 | vol);
					} else
						FineVolumeDown(pChn, vol);
					break;

				case VOLCMD_VIBRATOSPEED:
					// FT2 does not automatically enable vibrato with the "set vibrato speed" command
					if(IsCompatibleMode(TRK_FASTTRACKER2))
						pChn->nVibratoSpeed = vol & 0x0F;
					else
						Vibrato(pChn, vol << 4);
					break;

				case VOLCMD_VIBRATODEPTH:
					Vibrato(pChn, vol);
					break;

				case VOLCMD_PANSLIDELEFT:
					PanningSlide(pChn, vol);
					break;

				case VOLCMD_PANSLIDERIGHT:
					PanningSlide(pChn, vol << 4);
					break;

				case VOLCMD_PORTAUP:
					//IT compatibility (one of the first testcases - link effect memory)
					if(IsCompatibleMode(TRK_IMPULSETRACKER))
						PortamentoUp(pChn, vol << 2, true);
					else
						PortamentoUp(pChn, vol << 2, false);
					break;

				case VOLCMD_PORTADOWN:
					//IT compatibility (one of the first testcases - link effect memory)
					if(IsCompatibleMode(TRK_IMPULSETRACKER))
						PortamentoDown(pChn, vol << 2, true);
					else
						PortamentoDown(pChn, vol << 2, false);
					break;
							
				case VOLCMD_OFFSET:					//rewbs.volOff
					if (m_nTickCount == nStartTick) 
						SampleOffset(nChn, vol<<3, bPorta);
					break;
				}
			}
		}

		// Effects
		if (cmd) switch (cmd)
		{
// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
		case CMD_XPARAM:
			break;
// -> NEW_FEATURE#0010
		// Set Volume
		case CMD_VOLUME:
			if(m_dwSongFlags & SONG_FIRSTTICK)
			{
				pChn->nVolume = (param < 64) ? param*4 : 256;
				pChn->dwFlags |= CHN_FASTVOLRAMP;
			}
			break;

		// Portamento Up
		case CMD_PORTAMENTOUP:
			if ((!param) && (m_nType & MOD_TYPE_MOD)) break;
			PortamentoUp(pChn, param);
			break;

		// Portamento Down
		case CMD_PORTAMENTODOWN:
			if ((!param) && (m_nType & MOD_TYPE_MOD)) break;
			PortamentoDown(pChn, param);
			break;

		// Volume Slide
		case CMD_VOLUMESLIDE:
			if ((param) || (m_nType != MOD_TYPE_MOD)) VolumeSlide(pChn, param);
			break;

		// Tone-Portamento
		case CMD_TONEPORTAMENTO:
			TonePortamento(pChn, param);
			break;

		// Tone-Portamento + Volume Slide
		case CMD_TONEPORTAVOL:
			if ((param) || (m_nType != MOD_TYPE_MOD)) VolumeSlide(pChn, param);
			TonePortamento(pChn, 0);
			break;

		// Vibrato
		case CMD_VIBRATO:
			Vibrato(pChn, param);
			break;

		// Vibrato + Volume Slide
		case CMD_VIBRATOVOL:
			if ((param) || (m_nType != MOD_TYPE_MOD)) VolumeSlide(pChn, param);
			Vibrato(pChn, 0);
			break;

		// Set Speed
		case CMD_SPEED:
			if(m_dwSongFlags & SONG_FIRSTTICK)
				SetSpeed(param);
			break;

		// Set Tempo
		case CMD_TEMPO:
// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
				m = NULL;
				if (m_nRow < Patterns[m_nPattern].GetNumRows()-1) {
					m = Patterns[m_nPattern] + (m_nRow+1) * m_nChannels + nChn;
				}
				if (m && m->command == CMD_XPARAM) { 
					if (m_nType & MOD_TYPE_XM) {
                        param -= 0x20; //with XM, 0x20 is the lowest tempo. Anything below changes ticks per row.
					}
					param = (param<<8) + m->param;
				}
// -! NEW_FEATURE#0010
				if (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
				{
					if (param) pChn->nOldTempo = param; else param = pChn->nOldTempo;
				}
				if (param > GetModSpecifications().tempoMax) param = GetModSpecifications().tempoMax; // rewbs.merge: added check to avoid hyperspaced tempo!
				SetTempo(param);
			break;

		// Set Offset
		case CMD_OFFSET:
			if (m_nTickCount) break;
			//rewbs.volOffset: moved sample offset code to own method
			SampleOffset(nChn, param, bPorta);
			break;

		// Arpeggio
		case CMD_ARPEGGIO:
			// IT compatibility 01. Don't ignore Arpeggio if no note is playing (also valid for ST3)
			if ((m_nTickCount) || (((!pChn->nPeriod) || !pChn->nNote) && !IsCompatibleMode(TRK_IMPULSETRACKER | TRK_SCREAMTRACKER))) break;
			if ((!param) && (!(m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)))) break;
			pChn->nCommand = CMD_ARPEGGIO;
			if (param) pChn->nArpeggio = param;
			break;

		// Retrig
		case CMD_RETRIG:
			if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))
			{
				if (!(param & 0xF0)) param |= pChn->nRetrigParam & 0xF0;
				if (!(param & 0x0F)) param |= pChn->nRetrigParam & 0x0F;
				param |= 0x100; // increment retrig count on first row
			}
			// IT compatibility 15. Retrigger
			if(IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				if (param)
					pChn->nRetrigParam = (BYTE)(param & 0xFF);

				if (volcmd == VOLCMD_OFFSET)
					RetrigNote(nChn, pChn->nRetrigParam, vol << 3);
				else
					RetrigNote(nChn, pChn->nRetrigParam);
			}
			else
			{
				// XM Retrig
				if (param) pChn->nRetrigParam = (BYTE)(param & 0xFF); else param = pChn->nRetrigParam;
				//RetrigNote(nChn, param);
				if (volcmd == VOLCMD_OFFSET)
					RetrigNote(nChn, param, vol << 3);
				else
					RetrigNote(nChn, param);
			}
			break;

		// Tremor
		case CMD_TREMOR:
			if (!(m_dwSongFlags & SONG_FIRSTTICK)) break;

			// IT compatibility 12. / 13. Tremor (using modified DUMB's Tremor logic here because of old effects - http://dumb.sf.net/)
			if(IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				if (param && !(m_dwSongFlags & SONG_ITOLDEFFECTS))
				{
					// Old effects have different length interpretation (+1 for both on and off)
					if (param & 0xf0) param -= 0x10;
					if (param & 0x0f) param -= 0x01;
				}
				pChn->nTremorCount |= 128; // set on/off flag
			}
			else
			{
				// XM Tremor. Logic is being processed in sndmix.cpp
			}
				
			pChn->nCommand = CMD_TREMOR;
			if (param) pChn->nTremorParam = param;

			break;

		// Set Global Volume
		case CMD_GLOBALVOLUME:
			if (!(m_dwSongFlags & SONG_FIRSTTICK)) break;
			
			if (!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) param <<= 1;
			//IT compatibility 16. Both FT2 and IT ignore out-of-range values
			if(IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2))
			{
				if (param <= 128)
					m_nGlobalVolume = param << 1;
			}
			else
			{
				if (param > 128) param = 128;
				m_nGlobalVolume = param << 1;
			}
			break;

		// Global Volume Slide
		case CMD_GLOBALVOLSLIDE:
			//IT compatibility 16. Saving last global volume slide param per channel (FT2/IT)
			if(IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2))
				GlobalVolSlide(param, &pChn->nOldGlobalVolSlide);
			else
				GlobalVolSlide(param, &m_nOldGlbVolSlide);
			break;

		// Set 8-bit Panning
		case CMD_PANNING8:
			if (!(m_dwSongFlags & SONG_FIRSTTICK)) break;
			if ((m_dwSongFlags & SONG_PT1XMODE)) break;
			if (!(m_dwSongFlags & SONG_SURROUNDPAN)) pChn->dwFlags &= ~CHN_SURROUND;
			if (m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_XM|MOD_TYPE_MOD|MOD_TYPE_MT2))
			{
				pChn->nPan = param;
			} else
			if (param <= 0x80)
			{
				pChn->nPan = param << 1;
			} else
			if (param == 0xA4)
			{
				pChn->dwFlags |= CHN_SURROUND;
				pChn->nPan = 0x80;
			}
			pChn->dwFlags |= CHN_FASTVOLRAMP;
			pChn->nRestorePanOnNewNote = 0;
			//IT compatibility 20. Set pan overrides random pan
			if(IsCompatibleMode(TRK_IMPULSETRACKER))
				pChn->nPanSwing = 0;
			break;
			
		// Panning Slide
		case CMD_PANNINGSLIDE:
			PanningSlide(pChn, param);
			break;

		// Tremolo
		case CMD_TREMOLO:
			Tremolo(pChn, param);
			break;

		// Fine Vibrato
		case CMD_FINEVIBRATO:
			FineVibrato(pChn, param);
			break;

		// MOD/XM Exx Extended Commands
		case CMD_MODCMDEX:
			ExtendedMODCommands(nChn, param);
			break;

		// S3M/IT Sxx Extended Commands
		case CMD_S3MCMDEX:
			ExtendedS3MCommands(nChn, param);
			break;

		// Key Off
		case CMD_KEYOFF:
			// This is how Key Off is supposed to sound... (in FT2 at least)
			if(IsCompatibleMode(TRK_FASTTRACKER2))
			{
				if (m_nTickCount == param)
				{
					// XM: Key-Off + Sample == Note Cut
					if ((!pChn->pModInstrument) || (!(pChn->pModInstrument->VolEnv.dwFlags & ENV_ENABLED)))
					{
						if(param == 0) // FT2 is weird....
						{
							pChn->dwFlags |= CHN_NOTEFADE;
						}
						else
						{
							pChn->dwFlags |= CHN_FASTVOLRAMP;
							pChn->nVolume = 0;
						}
					}
					KeyOff(nChn);
				}
			}
			// This is how it's NOT supposed to sound...
			else
			{
				if(m_dwSongFlags & SONG_FIRSTTICK)
					KeyOff(nChn);
			}
			break;

		// Extra-fine porta up/down
		case CMD_XFINEPORTAUPDOWN:
			switch(param & 0xF0)
			{
			case 0x10: ExtraFinePortamentoUp(pChn, param & 0x0F); break;
			case 0x20: ExtraFinePortamentoDown(pChn, param & 0x0F); break;
			// ModPlug XM Extensions (ignore in compatible mode)
			case 0x50:
			case 0x60: 
			case 0x70:
			case 0x90:
			case 0xA0:
				if(!IsCompatibleMode(TRK_FASTTRACKER2)) ExtendedS3MCommands(nChn, param);
				break;
			}
			break;

		// Set Channel Global Volume
		case CMD_CHANNELVOLUME:
			if ((m_dwSongFlags & SONG_FIRSTTICK) == 0) break;
			if (param <= 64)
			{
				pChn->nGlobalVol = param;
				pChn->dwFlags |= CHN_FASTVOLRAMP;
			}
			break;

		// Channel volume slide
		case CMD_CHANNELVOLSLIDE:
			ChannelVolSlide(pChn, param);
			break;

		// Panbrello (IT)
		case CMD_PANBRELLO:
			Panbrello(pChn, param);
			break;

		// Set Envelope Position
		case CMD_SETENVPOSITION:
			if(m_dwSongFlags & SONG_FIRSTTICK)
			{
				pChn->VolEnv.nEnvPosition = param;

				// XM compatibility: FT2 only sets the position of the Volume envelope
				if(!IsCompatibleMode(TRK_FASTTRACKER2))
				{
					pChn->PanEnv.nEnvPosition = param;
					pChn->PitchEnv.nEnvPosition = param;
					if (pChn->pModInstrument)
					{
						MODINSTRUMENT *pIns = pChn->pModInstrument;
						if ((pChn->dwFlags & CHN_PANENV) && (pIns->PanEnv.nNodes) && (param > pIns->PanEnv.Ticks[pIns->PanEnv.nNodes-1]))
						{
							pChn->dwFlags &= ~CHN_PANENV;
						}
					}
				}

			}
			break;

		// Position Jump
		case CMD_POSITIONJUMP:
			m_nNextPatStartRow = 0; // FT2 E60 bug
			nPosJump = param;
			if((m_dwSongFlags & SONG_PATTERNLOOP) && m_nSeqOverride == 0)
			{
				 m_nSeqOverride = param + 1;
				 //Releasing pattern loop after position jump could cause 
				 //instant jumps - modifying behavior so that now position jumps
				 //occurs also when pattern loop is enabled.
			}
			// see http://forum.openmpt.org/index.php?topic=2769.0 - FastTracker resets Dxx if Bxx is called _after_ Dxx
			if(GetType() == MOD_TYPE_XM)
			{
				nBreakRow = 0;
			}
			break;

		// Pattern Break
		case CMD_PATTERNBREAK:
			m_nNextPatStartRow = 0; // FT2 E60 bug
			m = NULL;
			if (m_nRow < Patterns[m_nPattern].GetNumRows()-1)
			{
			  m = Patterns[m_nPattern] + (m_nRow+1) * m_nChannels + nChn;
			}
			if (m && m->command == CMD_XPARAM)
			{
				nBreakRow = (param << 8) + m->param;
			} else
			{
				nBreakRow = param;
			}
			
			if((m_dwSongFlags & SONG_PATTERNLOOP))
			{
				//If song is set to loop and a pattern break occurs we should stay on the same pattern.
				//Use nPosJump to force playback to "jump to this pattern" rather than move to next, as by default.
				//rewbs.to
				nPosJump = (int)m_nCurrentPattern;
			}
			break;

		// Midi Controller (on first tick only)
		case CMD_MIDI:
			if(!(m_dwSongFlags & SONG_FIRSTTICK)) break;
			if (param < 0x80)
				ProcessMidiMacro(nChn, &m_MidiCfg.szMidiSFXExt[pChn->nActiveMacro << 5], param);
			else
				ProcessMidiMacro(nChn, &m_MidiCfg.szMidiZXXExt[(param & 0x7F) << 5], 0);
			break;

		// Midi Controller (smooth, i.e. on every tick)
		case CMD_SMOOTHMIDI:
			if (param < 0x80)
				ProcessSmoothMidiMacro(nChn, &m_MidiCfg.szMidiSFXExt[pChn->nActiveMacro << 5], param);
			else
				ProcessSmoothMidiMacro(nChn, &m_MidiCfg.szMidiZXXExt[(param & 0x7F) << 5], 0);
			break;

		// IMF Commands
		case CMD_NOTESLIDEUP:
			NoteSlide(pChn, param, 1);
			break;
		case CMD_NOTESLIDEDOWN:
			NoteSlide(pChn, param, -1);
			break;
		}

	} // for(...) end

	// Navigation Effects
	if(m_dwSongFlags & SONG_FIRSTTICK)
	{
		// Pattern Loop
		if (nPatLoopRow != ROWINDEX_INVALID)
		{
			m_nNextPattern = m_nCurrentPattern;
			m_nNextRow = nPatLoopRow;
			if (m_nPatternDelay) m_nNextRow++;
			// As long as the pattern loop is running, mark the looped rows as not visited yet
			for(ROWINDEX nRow = nPatLoopRow; nRow <= m_nRow; nRow++)
			{
				SetRowVisited(m_nCurrentPattern, nRow, false);
			}
		} else
		// Pattern Break / Position Jump only if no loop running
		if ((nBreakRow != ROWINDEX_INVALID) || (nPosJump != ORDERINDEX_INVALID))
		{
			if (nPosJump == ORDERINDEX_INVALID) nPosJump = m_nCurrentPattern + 1;
			if (nBreakRow == ROWINDEX_INVALID) nBreakRow = 0;
			m_dwSongFlags |= SONG_BREAKTOROW;

			if (nPosJump >= Order.size()) 
			{
				nPosJump = 0;
			}

			// This checks whether we're jumping to the same row we're already on.
			// Sounds pretty stupid and pointless to me. And noone else does this, either.
			//if((nPosJump != (int)m_nCurrentPattern) || (nBreakRow != (int)m_nRow))
			{
				// IT compatibility: don't reset loop count on pattern break
				if (nPosJump != (int)m_nCurrentPattern && !IsCompatibleMode(TRK_IMPULSETRACKER))
				{
					for (CHANNELINDEX i = 0; i < m_nChannels; i++)
					{
						Chn[i].nPatternLoopCount = 0;
					}
				}
				m_nNextPattern = nPosJump;
				m_nNextRow = nBreakRow;
				m_bPatternTransitionOccurred = true;
			}
		} //Ends condition (nBreakRow >= 0) || (nPosJump >= 0)
		//SetRowVisited(m_nCurrentPattern, m_nRow);
	}
	return TRUE;
}


void CSoundFile::resetEnvelopes(MODCHANNEL* pChn, enmResetEnv envToReset)
//-----------------------------------------------------------------------
{
	switch (envToReset)
	{
		case ENV_RESET_ALL:
			pChn->VolEnv.nEnvPosition = 0;
			pChn->PanEnv.nEnvPosition = 0;
			pChn->PitchEnv.nEnvPosition = 0;
			pChn->VolEnv.nEnvValueAtReleaseJump = NOT_YET_RELEASED;
			pChn->PitchEnv.nEnvValueAtReleaseJump = NOT_YET_RELEASED;
			pChn->PanEnv.nEnvValueAtReleaseJump = NOT_YET_RELEASED;
			break;
		case ENV_RESET_VOL:
			pChn->VolEnv.nEnvPosition = 0;
			pChn->VolEnv.nEnvValueAtReleaseJump = NOT_YET_RELEASED;
			break;
		case ENV_RESET_PAN:
			pChn->PanEnv.nEnvPosition = 0;
			pChn->PanEnv.nEnvValueAtReleaseJump = NOT_YET_RELEASED;
			break;
		case ENV_RESET_PITCH:
			pChn->PitchEnv.nEnvPosition = 0;
			pChn->PitchEnv.nEnvValueAtReleaseJump = NOT_YET_RELEASED;
			break;				
	}
}


////////////////////////////////////////////////////////////
// Channels effects

void CSoundFile::PortamentoUp(MODCHANNEL *pChn, UINT param, const bool doFinePortamentoAsRegular)
//---------------------------------------------------------
{
	MidiPortamento(pChn, param); //Send midi pitch bend event if there's a plugin

	if(GetType() == MOD_TYPE_MPT && pChn->pModInstrument && pChn->pModInstrument->pTuning)
	{
		if(param)
			pChn->nOldPortaUpDown = param;
		else
			param = pChn->nOldPortaUpDown;

		if(param >= 0xF0 && !doFinePortamentoAsRegular)
			PortamentoFineMPT(pChn, param - 0xF0);
		else
			PortamentoMPT(pChn, param);
		return;
	}

	if (param) pChn->nOldPortaUpDown = param; else param = pChn->nOldPortaUpDown;
	if (!doFinePortamentoAsRegular && (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_STM)) && ((param & 0xF0) >= 0xE0))
	{
		if (param & 0x0F)
		{
			if ((param & 0xF0) == 0xF0)
			{
				FinePortamentoUp(pChn, param & 0x0F);
			} else
			if ((param & 0xF0) == 0xE0)
			{
				ExtraFinePortamentoUp(pChn, param & 0x0F);
			}
		}
		return;
	}
	// Regular Slide
	if (!(m_dwSongFlags & SONG_FIRSTTICK) || (m_nMusicSpeed == 1))  //rewbs.PortaA01fix
	{
		DoFreqSlide(pChn, -(int)(param * 4));
	}
}


void CSoundFile::PortamentoDown(MODCHANNEL *pChn, UINT param, const bool doFinePortamentoAsRegular)
//-----------------------------------------------------------
{
	MidiPortamento(pChn, -1*param); //Send midi pitch bend event if there's a plugin

	if(m_nType == MOD_TYPE_MPT && pChn->pModInstrument && pChn->pModInstrument->pTuning)
	{
		if(param)
			pChn->nOldPortaUpDown = param;
		else
			param = pChn->nOldPortaUpDown;

		if(param >= 0xF0 && !doFinePortamentoAsRegular)
			PortamentoFineMPT(pChn, -1*(param - 0xF0));
		else
			PortamentoMPT(pChn, -1*param);
		return;
	}

	if (param) pChn->nOldPortaUpDown = param; else param = pChn->nOldPortaUpDown;
	if (!doFinePortamentoAsRegular && (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_STM)) && ((param & 0xF0) >= 0xE0))
	{
		if (param & 0x0F)
		{
			if ((param & 0xF0) == 0xF0)
			{
				FinePortamentoDown(pChn, param & 0x0F);
			} else
			if ((param & 0xF0) == 0xE0)
			{
				ExtraFinePortamentoDown(pChn, param & 0x0F);
			}
		}
		return;
	}
	if (!(m_dwSongFlags & SONG_FIRSTTICK)  || (m_nMusicSpeed == 1)) {  //rewbs.PortaA01fix
		DoFreqSlide(pChn, (int)(param << 2));
	}
}

void CSoundFile::MidiPortamento(MODCHANNEL *pChn, int param)
//----------------------------------------------------------
{
	//Send midi pitch bend event if there's a plugin:
	MODINSTRUMENT *pHeader = pChn->pModInstrument;
	if (pHeader && pHeader->nMidiChannel>0 && pHeader->nMidiChannel<17) { // instro sends to a midi chan
		UINT nPlug = pHeader->nMixPlug;
		if ((nPlug) && (nPlug <= MAX_MIXPLUGINS)) {
			IMixPlugin *pPlug = (IMixPlugin*)m_MixPlugins[nPlug-1].pMixPlugin;
			if (pPlug) {
				pPlug->MidiPitchBend(pHeader->nMidiChannel, param, 0);
			}
		}
	}
}

void CSoundFile::FinePortamentoUp(MODCHANNEL *pChn, UINT param)
//-------------------------------------------------------------
{
	if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))
	{
		if (param) pChn->nOldFinePortaUpDown = param; else param = pChn->nOldFinePortaUpDown;
	}
	if (m_dwSongFlags & SONG_FIRSTTICK)
	{
		if ((pChn->nPeriod) && (param))
		{
			if ((m_dwSongFlags & SONG_LINEARSLIDES) && (!(m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))))
			{
				pChn->nPeriod = _muldivr(pChn->nPeriod, LinearSlideDownTable[param & 0x0F], 65536);
			} else
			{
				pChn->nPeriod -= (int)(param * 4);
			}
			if (pChn->nPeriod < 1) pChn->nPeriod = 1;
		}
	}
}


void CSoundFile::FinePortamentoDown(MODCHANNEL *pChn, UINT param)
//---------------------------------------------------------------
{
	if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))
	{
		if (param) pChn->nOldFinePortaUpDown = param; else param = pChn->nOldFinePortaUpDown;
	}
	if (m_dwSongFlags & SONG_FIRSTTICK)
	{
		if ((pChn->nPeriod) && (param))
		{
			if ((m_dwSongFlags & SONG_LINEARSLIDES) && (!(m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))))
			{
				pChn->nPeriod = _muldivr(pChn->nPeriod, LinearSlideUpTable[param & 0x0F], 65536);
			} else
			{
				pChn->nPeriod += (int)(param * 4);
			}
			if (pChn->nPeriod > 0xFFFF) pChn->nPeriod = 0xFFFF;
		}
	}
}


void CSoundFile::ExtraFinePortamentoUp(MODCHANNEL *pChn, UINT param)
//------------------------------------------------------------------
{
	if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))
	{
		if (param) pChn->nOldFinePortaUpDown = param; else param = pChn->nOldFinePortaUpDown;
	}
	if (m_dwSongFlags & SONG_FIRSTTICK)
	{
		if ((pChn->nPeriod) && (param))
		{
			if ((m_dwSongFlags & SONG_LINEARSLIDES) && (!(m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))))
			{
				pChn->nPeriod = _muldivr(pChn->nPeriod, FineLinearSlideDownTable[param & 0x0F], 65536);
			} else
			{
				pChn->nPeriod -= (int)(param);
			}
			if (pChn->nPeriod < 1) pChn->nPeriod = 1;
		}
	}
}


void CSoundFile::ExtraFinePortamentoDown(MODCHANNEL *pChn, UINT param)
//--------------------------------------------------------------------
{
	if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))
	{
		if (param) pChn->nOldFinePortaUpDown = param; else param = pChn->nOldFinePortaUpDown;
	}
	if (m_dwSongFlags & SONG_FIRSTTICK)
	{
		if ((pChn->nPeriod) && (param))
		{
			if ((m_dwSongFlags & SONG_LINEARSLIDES) && (!(m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))))
			{
				pChn->nPeriod = _muldivr(pChn->nPeriod, FineLinearSlideUpTable[param & 0x0F], 65536);
			} else
			{
				pChn->nPeriod += (int)(param);
			}
			if (pChn->nPeriod > 0xFFFF) pChn->nPeriod = 0xFFFF;
		}
	}
}

// Implemented for IMF compatibility, can't actually save this in any formats
// sign should be 1 (up) or -1 (down)
void CSoundFile::NoteSlide(MODCHANNEL *pChn, UINT param, int sign)
//----------------------------------------------------------------
{
	BYTE x, y;
	if (m_dwSongFlags & SONG_FIRSTTICK)
	{
		x = param & 0xf0;
		if (x)
			pChn->nNoteSlideSpeed = (x >> 4);
		y = param & 0xf;
		if (y)
			pChn->nNoteSlideStep = y;
		pChn->nNoteSlideCounter = pChn->nNoteSlideSpeed;
	} else
	{
		if (--pChn->nNoteSlideCounter == 0)
		{
			pChn->nNoteSlideCounter = pChn->nNoteSlideSpeed;
			// update it
			pChn->nPeriod = GetPeriodFromNote
				(sign * pChn->nNoteSlideStep + GetNoteFromPeriod(pChn->nPeriod), 8363, 0);
		}
	}
}

// Portamento Slide
void CSoundFile::TonePortamento(MODCHANNEL *pChn, UINT param)
//-----------------------------------------------------------
{
	pChn->dwFlags |= CHN_PORTAMENTO;

	//IT compatibility 03
	if(!(m_dwSongFlags & SONG_ITCOMPATGXX) && IsCompatibleMode(TRK_IMPULSETRACKER))
	{
		if(param == 0) param = pChn->nOldPortaUpDown;
		pChn->nOldPortaUpDown = param;
	}

	if(m_nType == MOD_TYPE_MPT && pChn->pModInstrument && pChn->pModInstrument->pTuning)
	{
		//Behavior: Param tells number of finesteps(or 'fullsteps'(notes) with glissando)
		//to slide per row(not per tick).	
		const long old_PortamentoTickSlide = (m_nTickCount != 0) ? pChn->m_PortamentoTickSlide : 0;

		if(param)
			pChn->nPortamentoSlide = param;
		else
			if(pChn->nPortamentoSlide == 0)
				return;


		if((pChn->nPortamentoDest > 0 && pChn->nPortamentoSlide < 0) ||
			(pChn->nPortamentoDest < 0 && pChn->nPortamentoSlide > 0))
			pChn->nPortamentoSlide = -pChn->nPortamentoSlide;

		pChn->m_PortamentoTickSlide = (m_nTickCount + 1.0) * pChn->nPortamentoSlide / m_nMusicSpeed;

		if(pChn->dwFlags & CHN_GLISSANDO)
		{
			pChn->m_PortamentoTickSlide *= pChn->pModInstrument->pTuning->GetFineStepCount() + 1;
			//With glissando interpreting param as notes instead of finesteps.
		}

		const long slide = pChn->m_PortamentoTickSlide - old_PortamentoTickSlide;

		if(abs(pChn->nPortamentoDest) <= abs(slide))
		{
			if(pChn->nPortamentoDest != 0)
			{
				pChn->m_PortamentoFineSteps += pChn->nPortamentoDest;
				pChn->nPortamentoDest = 0;
				pChn->m_CalculateFreq = true;
			}
		}
		else
		{
			pChn->m_PortamentoFineSteps += slide;
			pChn->nPortamentoDest -= slide;
			pChn->m_CalculateFreq = true;
		}

		return;
	} //End candidate MPT behavior.

	if (param) pChn->nPortamentoSlide = param * 4;
	if ((pChn->nPeriod) && (pChn->nPortamentoDest) && ((m_nMusicSpeed == 1) || !(m_dwSongFlags & SONG_FIRSTTICK)))  //rewbs.PortaA01fix
	{
		if (pChn->nPeriod < pChn->nPortamentoDest)
		{
			LONG delta = (int)pChn->nPortamentoSlide;
			if ((m_dwSongFlags & SONG_LINEARSLIDES) && (!(m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))))
			{
				UINT n = pChn->nPortamentoSlide >> 2;
				if (n > 255) n = 255;
				// Return (a*b+c/2)/c - no divide error
				// Table is 65536*2(n/192)
				delta = _muldivr(pChn->nPeriod, LinearSlideUpTable[n], 65536) - pChn->nPeriod;
				if (delta < 1) delta = 1;
			}
			pChn->nPeriod += delta;
			if (pChn->nPeriod > pChn->nPortamentoDest) pChn->nPeriod = pChn->nPortamentoDest;
		} else
		if (pChn->nPeriod > pChn->nPortamentoDest)
		{
			LONG delta = - (int)pChn->nPortamentoSlide;
			if ((m_dwSongFlags & SONG_LINEARSLIDES) && (!(m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))))
			{
				UINT n = pChn->nPortamentoSlide >> 2;
				if (n > 255) n = 255;
				delta = _muldivr(pChn->nPeriod, LinearSlideDownTable[n], 65536) - pChn->nPeriod;
				if (delta > -1) delta = -1;
			}
			pChn->nPeriod += delta;
			if (pChn->nPeriod < pChn->nPortamentoDest) pChn->nPeriod = pChn->nPortamentoDest;
		}
	}

	//IT compatibility 23. Portamento with no note
	if(pChn->nPeriod == pChn->nPortamentoDest && IsCompatibleMode(TRK_IMPULSETRACKER))
		pChn->nPortamentoDest = 0;

}


void CSoundFile::Vibrato(MODCHANNEL *p, UINT param)
//-------------------------------------------------
{
	p->m_VibratoDepth = param % 16 / 15.0F;
	//'New tuning'-thing: 0 - 1 <-> No depth - Full depth.


	if (param & 0x0F) p->nVibratoDepth = (param & 0x0F) * 4;
	if (param & 0xF0) p->nVibratoSpeed = (param >> 4) & 0x0F;
	p->dwFlags |= CHN_VIBRATO;
}


void CSoundFile::FineVibrato(MODCHANNEL *p, UINT param)
//-----------------------------------------------------
{
	if (param & 0x0F) p->nVibratoDepth = param & 0x0F;
	if (param & 0xF0) p->nVibratoSpeed = (param >> 4) & 0x0F;
	p->dwFlags |= CHN_VIBRATO;
}


void CSoundFile::Panbrello(MODCHANNEL *p, UINT param)
//---------------------------------------------------
{
	if (param & 0x0F) p->nPanbrelloDepth = param & 0x0F;
	if (param & 0xF0) p->nPanbrelloSpeed = (param >> 4) & 0x0F;
	p->dwFlags |= CHN_PANBRELLO;
}


void CSoundFile::VolumeSlide(MODCHANNEL *pChn, UINT param)
//--------------------------------------------------------
{
	if (param)
		pChn->nOldVolumeSlide = param;
	else
		param = pChn->nOldVolumeSlide;
	LONG newvolume = pChn->nVolume;
	if (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_STM|MOD_TYPE_AMF))
	{
		if ((param & 0x0F) == 0x0F) //Fine upslide or slide -15
		{
			if (param & 0xF0) //Fine upslide
			{
				FineVolumeUp(pChn, (param >> 4));
				return;
			} else //Slide -15
			{
				if ((m_dwSongFlags & SONG_FIRSTTICK) && (!(m_dwSongFlags & SONG_FASTVOLSLIDES)))
				{
					newvolume -= 0x0F * 4;
				}
			}
		} else
		if ((param & 0xF0) == 0xF0) //Fine downslide or slide +15
		{
			if (param & 0x0F) //Fine downslide
			{
				FineVolumeDown(pChn, (param & 0x0F));
				return;
			} else //Slide +15
			{
				if ((m_dwSongFlags & SONG_FIRSTTICK) && (!(m_dwSongFlags & SONG_FASTVOLSLIDES)))
				{
					newvolume += 0x0F * 4;
				}
			}
		}
	}
	if ((!(m_dwSongFlags & SONG_FIRSTTICK)) || (m_dwSongFlags & SONG_FASTVOLSLIDES))
	{
		// IT compatibility: Ignore slide commands with both nibbles set.
		if (param & 0x0F)
		{
			if(!IsCompatibleMode(TRK_IMPULSETRACKER) || (param & 0xF0) == 0)
				newvolume -= (int)((param & 0x0F) * 4);
		}
		else
		{
			if(!IsCompatibleMode(TRK_IMPULSETRACKER) || (param & 0x0F) == 0)
				newvolume += (int)((param & 0xF0) >> 2);
		}
		if (m_nType == MOD_TYPE_MOD) pChn->dwFlags |= CHN_FASTVOLRAMP;
	}
	newvolume = CLAMP(newvolume, 0, 256);

	pChn->nVolume = newvolume;
}


void CSoundFile::PanningSlide(MODCHANNEL *pChn, UINT param)
//---------------------------------------------------------
{
	LONG nPanSlide = 0;
	if (param)
		pChn->nOldPanSlide = param;
	else
		param = pChn->nOldPanSlide;
	if (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_STM))
	{
		if (((param & 0x0F) == 0x0F) && (param & 0xF0))
		{
			if (m_dwSongFlags & SONG_FIRSTTICK)
			{
				param = (param & 0xF0) >> 2;
				nPanSlide = - (int)param;
			}
		} else
		if (((param & 0xF0) == 0xF0) && (param & 0x0F))
		{
			if (m_dwSongFlags & SONG_FIRSTTICK)
			{
				nPanSlide = (param & 0x0F) << 2;
			}
		} else
		{
			if (!(m_dwSongFlags & SONG_FIRSTTICK))
			{
				if (param & 0x0F) nPanSlide = (int)((param & 0x0F) << 2);
				else nPanSlide = -(int)((param & 0xF0) >> 2);
			}
		}
	} else
	{
		if (!(m_dwSongFlags & SONG_FIRSTTICK))
		{
			// IT compatibility: Ignore slide commands with both nibbles set.
			if (param & 0x0F)
			{
				if(!IsCompatibleMode(TRK_IMPULSETRACKER) || (param & 0xF0) == 0)
					nPanSlide = -(int)((param & 0x0F) << 2);
			}
			else
			{
				if(!IsCompatibleMode(TRK_IMPULSETRACKER) || (param & 0x0F) == 0)
					nPanSlide = (int)((param & 0xF0) >> 2);
			}
			// XM compatibility: FT2's panning slide is not as deep
			if(IsCompatibleMode(TRK_FASTTRACKER2))
				nPanSlide >>= 2;
		}
	}
	if (nPanSlide)
	{
		nPanSlide += pChn->nPan;
		nPanSlide = CLAMP(nPanSlide, 0, 256);
		pChn->nPan = nPanSlide;
		pChn->nRestorePanOnNewNote = 0;
	}
}


void CSoundFile::FineVolumeUp(MODCHANNEL *pChn, UINT param)
//---------------------------------------------------------
{
	if (param) pChn->nOldFineVolUpDown = param; else param = pChn->nOldFineVolUpDown;
	if (m_dwSongFlags & SONG_FIRSTTICK)
	{
		pChn->nVolume += param * 4;
		if (pChn->nVolume > 256) pChn->nVolume = 256;
		if (m_nType & MOD_TYPE_MOD) pChn->dwFlags |= CHN_FASTVOLRAMP;
	}
}


void CSoundFile::FineVolumeDown(MODCHANNEL *pChn, UINT param)
//-----------------------------------------------------------
{
	if (param) pChn->nOldFineVolUpDown = param; else param = pChn->nOldFineVolUpDown;
	if (m_dwSongFlags & SONG_FIRSTTICK)
	{
		pChn->nVolume -= param * 4;
		if (pChn->nVolume < 0) pChn->nVolume = 0;
		if (m_nType & MOD_TYPE_MOD) pChn->dwFlags |= CHN_FASTVOLRAMP;
	}
}


void CSoundFile::Tremolo(MODCHANNEL *p, UINT param)
//-------------------------------------------------
{
	if (param & 0x0F) p->nTremoloDepth = (param & 0x0F) << 2;
	if (param & 0xF0) p->nTremoloSpeed = (param >> 4) & 0x0F;
	p->dwFlags |= CHN_TREMOLO;
}


void CSoundFile::ChannelVolSlide(MODCHANNEL *pChn, UINT param)
//------------------------------------------------------------
{
	LONG nChnSlide = 0;
	if (param) pChn->nOldChnVolSlide = param; else param = pChn->nOldChnVolSlide;
	if (((param & 0x0F) == 0x0F) && (param & 0xF0))
	{
		if (m_dwSongFlags & SONG_FIRSTTICK) nChnSlide = param >> 4;
	} else
	if (((param & 0xF0) == 0xF0) && (param & 0x0F))
	{
		if (m_dwSongFlags & SONG_FIRSTTICK) nChnSlide = - (int)(param & 0x0F);
	} else
	{
		if (!(m_dwSongFlags & SONG_FIRSTTICK))
		{
			if (param & 0x0F) nChnSlide = -(int)(param & 0x0F);
			else nChnSlide = (int)((param & 0xF0) >> 4);
		}
	}
	if (nChnSlide)
	{
		nChnSlide += pChn->nGlobalVol;
		nChnSlide = CLAMP(nChnSlide, 0, 64);
		pChn->nGlobalVol = nChnSlide;
	}
}


void CSoundFile::ExtendedMODCommands(UINT nChn, UINT param)
//---------------------------------------------------------
{
	MODCHANNEL *pChn = &Chn[nChn];
	UINT command = param & 0xF0;
	param &= 0x0F;
	switch(command)
	{
	// E0x: Set Filter
	// E1x: Fine Portamento Up
	case 0x10:	if ((param) || (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))) FinePortamentoUp(pChn, param); break;
	// E2x: Fine Portamento Down
	case 0x20:	if ((param) || (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))) FinePortamentoDown(pChn, param); break;
	// E3x: Set Glissando Control
	case 0x30:	pChn->dwFlags &= ~CHN_GLISSANDO; if (param) pChn->dwFlags |= CHN_GLISSANDO; break;
	// E4x: Set Vibrato WaveForm
	case 0x40:	pChn->nVibratoType = param & 0x07; break;
	// E5x: Set FineTune
	case 0x50:	if(!(m_dwSongFlags & SONG_FIRSTTICK)) break;
				pChn->nC5Speed = S3MFineTuneTable[param];
				if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))
					pChn->nFineTune = param*2;
				else
					pChn->nFineTune = MOD2XMFineTune(param);
				if (pChn->nPeriod) pChn->nPeriod = GetPeriodFromNote(pChn->nNote, pChn->nFineTune, pChn->nC5Speed);
				break;
	// E6x: Pattern Loop
	// E7x: Set Tremolo WaveForm
	case 0x70:	pChn->nTremoloType = param & 0x07; break;
	// E8x: Set 4-bit Panning
	case 0x80:	if((m_dwSongFlags & SONG_FIRSTTICK) && !(m_dwSongFlags & SONG_PT1XMODE))
				{ 
					//IT compatibility 20. (Panning always resets surround state)
					if(IsCompatibleMode(TRK_ALLTRACKERS))
					{
						if (!(m_dwSongFlags & SONG_SURROUNDPAN)) pChn->dwFlags &= ~CHN_SURROUND;
					}
					pChn->nPan = (param << 4) + 8; pChn->dwFlags |= CHN_FASTVOLRAMP;
				}
				break;
	// E9x: Retrig
	case 0x90:	RetrigNote(nChn, param); break;
	// EAx: Fine Volume Up
	case 0xA0:	if ((param) || (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))) FineVolumeUp(pChn, param); break;
	// EBx: Fine Volume Down
	case 0xB0:	if ((param) || (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))) FineVolumeDown(pChn, param); break;
	// ECx: Note Cut
	case 0xC0:	NoteCut(nChn, param); break;
	// EDx: Note Delay
	// EEx: Pattern Delay
	case 0xF0:	
		if(GetType() == MOD_TYPE_MOD) // MOD: Invert Loop
		{
			pChn->nEFxSpeed = param;
			if(m_dwSongFlags & SONG_FIRSTTICK) InvertLoop(pChn);
		}	
		else // XM: Set Active Midi Macro
		{
			pChn->nActiveMacro = param;
		}
		break;
	}
}


void CSoundFile::ExtendedS3MCommands(UINT nChn, UINT param)
//---------------------------------------------------------
{
	MODCHANNEL *pChn = &Chn[nChn];
	UINT command = param & 0xF0;
	param &= 0x0F;
	switch(command)
	{
	// S0x: Set Filter
	// S1x: Set Glissando Control
	case 0x10:	pChn->dwFlags &= ~CHN_GLISSANDO; if (param) pChn->dwFlags |= CHN_GLISSANDO; break;
	// S2x: Set FineTune
	case 0x20:	if(!(m_dwSongFlags & SONG_FIRSTTICK)) break;
				pChn->nC5Speed = S3MFineTuneTable[param];
				pChn->nFineTune = MOD2XMFineTune(param);
				if (pChn->nPeriod) pChn->nPeriod = GetPeriodFromNote(pChn->nNote, pChn->nFineTune, pChn->nC5Speed);
				break;
	// S3x: Set Vibrato Waveform
	case 0x30:	if(GetType() == MOD_TYPE_S3M)
				{
					pChn->nVibratoType = param & 0x03;
				} else
				{
					// IT compatibility: Ignore waveform types > 3
					if(IsCompatibleMode(TRK_IMPULSETRACKER))
						pChn->nVibratoType = (param < 0x04) ? param : 0;
					else
						pChn->nVibratoType = param & 0x07;
				}
				break;
	// S4x: Set Tremolo Waveform
	case 0x40:	if(GetType() == MOD_TYPE_S3M)
				{
					pChn->nTremoloType = param & 0x03;
				} else
				{
					// IT compatibility: Ignore waveform types > 3
					if(IsCompatibleMode(TRK_IMPULSETRACKER))
						pChn->nTremoloType = (param < 0x04) ? param : 0;
					else
						pChn->nTremoloType = param & 0x07;
				}
				break;
	// S5x: Set Panbrello Waveform
	case 0x50:
		// IT compatibility: Ignore waveform types > 3
				if(IsCompatibleMode(TRK_IMPULSETRACKER))
					pChn->nPanbrelloType = (param < 0x04) ? param : 0;
				else
					pChn->nPanbrelloType = param & 0x07;
				break;
	// S6x: Pattern Delay for x frames
	case 0x60:	m_nFrameDelay = param; break;
	// S7x: Envelope Control / Instrument Control
	case 0x70:	if(!(m_dwSongFlags & SONG_FIRSTTICK)) break;
				switch(param)
				{
				case 0:
				case 1:
				case 2:
					{
						MODCHANNEL *bkp = &Chn[m_nChannels];
						for (UINT i=m_nChannels; i<MAX_CHANNELS; i++, bkp++)
						{
							if (bkp->nMasterChn == nChn+1)
							{
								if (param == 1)
								{
									KeyOff(i);
								} else if (param == 2)
								{
									bkp->dwFlags |= CHN_NOTEFADE;
								} else
								{
									bkp->dwFlags |= CHN_NOTEFADE;
									bkp->nFadeOutVol = 0;
								}
							}
						}
					}
					break;
				case 3:		pChn->nNNA = NNA_NOTECUT; break;
				case 4:		pChn->nNNA = NNA_CONTINUE; break;
				case 5:		pChn->nNNA = NNA_NOTEOFF; break;
				case 6:		pChn->nNNA = NNA_NOTEFADE; break;
				case 7:		pChn->dwFlags &= ~CHN_VOLENV; break;
				case 8:		pChn->dwFlags |= CHN_VOLENV; break;
				case 9:		pChn->dwFlags &= ~CHN_PANENV; break;
				case 10:	pChn->dwFlags |= CHN_PANENV; break;
				case 11:	pChn->dwFlags &= ~CHN_PITCHENV; break;
				case 12:	pChn->dwFlags |= CHN_PITCHENV; break;
				case 13:	
				case 14:
					if(GetType() == MOD_TYPE_MPT)
					{
						pChn->dwFlags |= CHN_PITCHENV;
						if(param == 13)	// pitch env on, filter env off
						{
							pChn->dwFlags &= ~CHN_FILTERENV;
						} else	// filter env on
						{
							pChn->dwFlags |= CHN_FILTERENV;
						}
					}
					break;
				}
				break;
	// S8x: Set 4-bit Panning
	case 0x80:	if(m_dwSongFlags & SONG_FIRSTTICK)
				{ 
					// IT Compatibility (and other trackers as well): panning disables surround (unless panning in rear channels is enabled, which is not supported by the original trackers anyway)
					if(IsCompatibleMode(TRK_ALLTRACKERS))
					{
						if (!(m_dwSongFlags & SONG_SURROUNDPAN)) pChn->dwFlags &= ~CHN_SURROUND;
					}
					pChn->nPan = (param << 4) + 8; pChn->dwFlags |= CHN_FASTVOLRAMP;

					//IT compatibility 20. Set pan overrides random pan
					if(IsCompatibleMode(TRK_IMPULSETRACKER))
						pChn->nPanSwing = 0;
				}
				break;
	// S9x: Sound Control
	case 0x90:	ExtendedChannelEffect(pChn, param); break;
	// SAx: Set 64k Offset
	case 0xA0:	if(m_dwSongFlags & SONG_FIRSTTICK)
				{
					pChn->nOldHiOffset = param;
					if ((pChn->nRowNote) && (pChn->nRowNote < 0x80))
					{
						DWORD pos = param << 16;
						if (pos < pChn->nLength) pChn->nPos = pos;
					}
				}
				break;
	// SBx: Pattern Loop
	// SCx: Note Cut
	case 0xC0:	NoteCut(nChn, param); break;
	// SDx: Note Delay
	// SEx: Pattern Delay for x rows
	// SFx: S3M: Not used, IT: Set Active Midi Macro
	case 0xF0:	pChn->nActiveMacro = param; break;
	}
}


void CSoundFile::ExtendedChannelEffect(MODCHANNEL *pChn, UINT param)
//------------------------------------------------------------------
{
	// S9x and X9x commands (S3M/XM/IT only)
	if(!(m_dwSongFlags & SONG_FIRSTTICK)) return;
	switch(param & 0x0F)
	{
	// S90: Surround Off
	case 0x00:	pChn->dwFlags &= ~CHN_SURROUND;	break;
	// S91: Surround On
	case 0x01:	pChn->dwFlags |= CHN_SURROUND; pChn->nPan = 128; break;
	////////////////////////////////////////////////////////////
	// ModPlug Extensions
	// S98: Reverb Off
	case 0x08:
		pChn->dwFlags &= ~CHN_REVERB;
		pChn->dwFlags |= CHN_NOREVERB;
		break;
	// S99: Reverb On
	case 0x09:
		pChn->dwFlags &= ~CHN_NOREVERB;
		pChn->dwFlags |= CHN_REVERB;
		break;
	// S9A: 2-Channels surround mode
	case 0x0A:
		m_dwSongFlags &= ~SONG_SURROUNDPAN;
		break;
	// S9B: 4-Channels surround mode
	case 0x0B:
		m_dwSongFlags |= SONG_SURROUNDPAN;
		break;
	// S9C: IT Filter Mode
	case 0x0C:
		m_dwSongFlags &= ~SONG_MPTFILTERMODE;
		break;
	// S9D: MPT Filter Mode
	case 0x0D:
		m_dwSongFlags |= SONG_MPTFILTERMODE;
		break;
	// S9E: Go forward
	case 0x0E:
		pChn->dwFlags &= ~(CHN_PINGPONGFLAG);
		break;
	// S9F: Go backward (set position at the end for non-looping samples)
	case 0x0F:
		if ((!(pChn->dwFlags & CHN_LOOP)) && (!pChn->nPos) && (pChn->nLength))
		{
			pChn->nPos = pChn->nLength - 1;
			pChn->nPosLo = 0xFFFF;
		}
		pChn->dwFlags |= CHN_PINGPONGFLAG;
		break;
	}
}


inline void CSoundFile::InvertLoop(MODCHANNEL *pChn)
//--------------------------------------------------
{
	// EFx implementation for MOD files (PT 1.1A and up: Invert Loop)
	// This effect trashes samples. Thanks to 8bitbubsy for making this work. :)
	if(GetType() != MOD_TYPE_MOD || pChn->nEFxSpeed == 0) return;

	// we obviously also need a sample for this
	MODSAMPLE *pModSample = pChn->pModSample;
	if(pModSample == nullptr || pModSample->pSample == nullptr || !(pModSample->uFlags & CHN_LOOP) || (pModSample->uFlags & CHN_16BIT)) return;

	pChn->nEFxDelay += ModEFxTable[pChn->nEFxSpeed & 0x0F];
	if((pChn->nEFxDelay & 0x80) == 0) return; // only applied if the "delay" reaches 128
	pChn->nEFxDelay = 0;

	if (++pChn->nEFxOffset >= pModSample->nLoopEnd - pModSample->nLoopStart)
		pChn->nEFxOffset = 0;

	// TRASH IT!!! (Yes, the sample!)
	pModSample->pSample[pModSample->nLoopStart + pChn->nEFxOffset] = ~pModSample->pSample[pModSample->nLoopStart + pChn->nEFxOffset];
	//AdjustSampleLoop(pModSample);
}


void CSoundFile::ProcessMidiMacro(UINT nChn, LPCSTR pszMidiMacro, UINT param)
//---------------------------------------------------------------------------
{
	MODCHANNEL *pChn = &Chn[nChn];
	DWORD dwMacro = (*((LPDWORD)pszMidiMacro)) & MACRO_MASK;
	int nInternalCode;

	// Not Internal Device ?
	if (dwMacro != MACRO_INTERNAL && dwMacro != MACRO_INTERNALEX)
	{
		UINT pos = 0, nNib = 0, nBytes = 0;
		DWORD dwMidiCode = 0, dwByteCode = 0;

		while (pos+6 <= 32)
		{
			CHAR cData = pszMidiMacro[pos++];
			if (!cData) break;
			if ((cData >= '0') && (cData <= '9')) { dwByteCode = (dwByteCode<<4) | (cData-'0'); nNib++; } else
			if ((cData >= 'A') && (cData <= 'F')) { dwByteCode = (dwByteCode<<4) | (cData-'A'+10); nNib++; } else
			if ((cData >= 'a') && (cData <= 'f')) { dwByteCode = (dwByteCode<<4) | (cData-'a'+10); nNib++; } else
			if ((cData == 'z') || (cData == 'Z')) { dwByteCode = param & 0x7f; nNib = 2; } else
			if ((cData == 'x') || (cData == 'X')) { dwByteCode = param & 0x70; nNib = 2; } else
			if ((cData == 'y') || (cData == 'Y')) { dwByteCode = (param & 0x0f)<<3; nNib = 2; }
			if ((cData == 'k') || (cData == 'K')) { dwByteCode = (dwByteCode<<4) | GetBestMidiChan(pChn); nNib++; }

			if (nNib >= 2)
			{
				nNib = 0;
				dwMidiCode |= dwByteCode << (nBytes*8);
				dwByteCode = 0;
				nBytes++;

				if (nBytes >= 3)
				{
					UINT nMasterCh = (nChn < m_nChannels) ? nChn+1 : pChn->nMasterChn;
					if ((nMasterCh) && (nMasterCh <= m_nChannels))
					{
// -> CODE#0015
// -> DESC="channels management dlg"
						UINT nPlug = GetBestPlugin(nChn, PRIORITISE_CHANNEL, EVEN_IF_MUTED);
						if(pChn->dwFlags & CHN_NOFX) nPlug = 0;
// -! NEW_FEATURE#0015
						if ((nPlug) && (nPlug <= MAX_MIXPLUGINS)) {
							IMixPlugin *pPlugin = m_MixPlugins[nPlug-1].pMixPlugin;
							if ((pPlugin) && (m_MixPlugins[nPlug-1].pMixState))
							{
								pPlugin->MidiSend(dwMidiCode);
							}
						}
					}
					nBytes = 0;
					dwMidiCode = 0;
				}
			}

		}
		
		return;
	}

	// Internal device
	//HACK:
	const bool extendedParam = (dwMacro == MACRO_INTERNALEX);

	pszMidiMacro += 4;
	nInternalCode = -256;
	if ((pszMidiMacro[0] >= '0') && (pszMidiMacro[0] <= '9')) nInternalCode = (pszMidiMacro[0] - '0') << 4; else
	if ((pszMidiMacro[0] >= 'A') && (pszMidiMacro[0] <= 'F')) nInternalCode = (pszMidiMacro[0] - 'A' + 0x0A) << 4;
	if ((pszMidiMacro[1] >= '0') && (pszMidiMacro[1] <= '9')) nInternalCode += (pszMidiMacro[1] - '0'); else
	if ((pszMidiMacro[1] >= 'A') && (pszMidiMacro[1] <= 'F')) nInternalCode += (pszMidiMacro[1] - 'A' + 0x0A);
	// Filter ?
	if (nInternalCode >= 0)
	{
		CHAR cData1 = pszMidiMacro[2];
		DWORD dwParam = 0;
		if ((cData1 == 'z') || (cData1 == 'Z'))
		{
			dwParam = param;
		} else
		{
			CHAR cData2 = pszMidiMacro[3];
			if ((cData1 >= '0') && (cData1 <= '9')) dwParam += (cData1 - '0') << 4; else
			if ((cData1 >= 'A') && (cData1 <= 'F')) dwParam += (cData1 - 'A' + 0x0A) << 4;
			if ((cData2 >= '0') && (cData2 <= '9')) dwParam += (cData2 - '0'); else
			if ((cData2 >= 'A') && (cData2 <= 'F')) dwParam += (cData2 - 'A' + 0x0A);
		}
		switch(nInternalCode)
		{
		// F0.F0.00.xx: Set CutOff
		case 0x00:
			{
				int oldcutoff = pChn->nCutOff;
				if (dwParam < 0x80)
				{
					pChn->nCutOff = dwParam;
					pChn->nRestoreCutoffOnNewNote = 0;
				}
			#ifndef NO_FILTER
				oldcutoff -= pChn->nCutOff;
				if (oldcutoff < 0) oldcutoff = -oldcutoff;
				if ((pChn->nVolume > 0) || (oldcutoff < 0x10)
				|| (!(pChn->dwFlags & CHN_FILTER)) || (!(pChn->nLeftVol|pChn->nRightVol)))
					SetupChannelFilter(pChn, (pChn->dwFlags & CHN_FILTER) ? false : true);
			#endif // NO_FILTER
			}
			break;
		
		// F0.F0.01.xx: Set Resonance
		case 0x01:
			if (dwParam < 0x80) 
			{
				pChn->nRestoreResonanceOnNewNote = 0;
				pChn->nResonance = dwParam;
			}
				
		#ifndef NO_FILTER
			SetupChannelFilter(pChn, (pChn->dwFlags & CHN_FILTER) ? false : true);
		#endif // NO_FILTER
			break;

		// F0.F0.02.xx: Set filter mode
		case 0x02:
			if (dwParam < 0x20)
			{
				pChn->nFilterMode = (dwParam>>4);
			#ifndef NO_FILTER
				SetupChannelFilter(pChn, (pChn->dwFlags & CHN_FILTER) ? false : true);
			#endif // NO_FILTER
			}
			break;

	// F0.F0.03.xx: Set plug dry/wet
		case 0x03:
			{
				UINT nPlug = GetBestPlugin(nChn, PRIORITISE_CHANNEL, EVEN_IF_MUTED);
				if ((nPlug) && (nPlug <= MAX_MIXPLUGINS))	{
					if (dwParam < 0x80)
						m_MixPlugins[nPlug-1].fDryRatio = 1.0-(static_cast<float>(dwParam)/127.0f);
				}
			}
			break;


		// F0.F0.{80|n}.xx: Set VST effect parameter n to xx
		default:
			if (nInternalCode & 0x80  || extendedParam)
			{
				UINT nPlug = GetBestPlugin(nChn, PRIORITISE_CHANNEL, EVEN_IF_MUTED);
				if ((nPlug) && (nPlug <= MAX_MIXPLUGINS))
				{
					IMixPlugin *pPlugin = m_MixPlugins[nPlug-1].pMixPlugin;
					if ((pPlugin) && (m_MixPlugins[nPlug-1].pMixState))
					{
						pPlugin->SetZxxParameter(extendedParam?(0x80+nInternalCode):(nInternalCode&0x7F), dwParam & 0x7F);
					}
				}
			}

		} // end switch
	} // end internal device

}

//rewbs.smoothVST: begin tick resolution handling.
void CSoundFile::ProcessSmoothMidiMacro(UINT nChn, LPCSTR pszMidiMacro, UINT param)
//---------------------------------------------------------------------------
{
	MODCHANNEL *pChn = &Chn[nChn];
	DWORD dwMacro = (*((LPDWORD)pszMidiMacro)) & MACRO_MASK;
	int nInternalCode;
	CHAR cData1;		// rewbs.smoothVST: 
	DWORD dwParam;		// increased scope to fuction.

	if (dwMacro != MACRO_INTERNAL && dwMacro != MACRO_INTERNALEX)
	{
		// we don't cater for external devices at tick resolution.
		if(m_dwSongFlags & SONG_FIRSTTICK) {
			ProcessMidiMacro(nChn, pszMidiMacro, param);
		}
		return;
	}
	
	//HACK:
	const bool extendedParam = (dwMacro == MACRO_INTERNALEX);

	// not sure what we're doing here; some sort of info gathering from the macros
	pszMidiMacro += 4;
	nInternalCode = -256;
	if ((pszMidiMacro[0] >= '0') && (pszMidiMacro[0] <= '9')) nInternalCode = (pszMidiMacro[0] - '0') << 4; else
	if ((pszMidiMacro[0] >= 'A') && (pszMidiMacro[0] <= 'F')) nInternalCode = (pszMidiMacro[0] - 'A' + 0x0A) << 4;
	if ((pszMidiMacro[1] >= '0') && (pszMidiMacro[1] <= '9')) nInternalCode += (pszMidiMacro[1] - '0'); else
	if ((pszMidiMacro[1] >= 'A') && (pszMidiMacro[1] <= 'F')) nInternalCode += (pszMidiMacro[1] - 'A' + 0x0A);

	if  (nInternalCode < 0) // not good plugin param macro. (?)
		return;

	cData1 = pszMidiMacro[2];
	dwParam = 0;

	if ((cData1 == 'z') || (cData1 == 'Z'))	//parametric macro
	{
		dwParam = param;
	} else 	//fixed macro
	{
		CHAR cData2 = pszMidiMacro[3];
		if ((cData1 >= '0') && (cData1 <= '9')) dwParam += (cData1 - '0') << 4; else
		if ((cData1 >= 'A') && (cData1 <= 'F')) dwParam += (cData1 - 'A' + 0x0A) << 4;
		if ((cData2 >= '0') && (cData2 <= '9')) dwParam += (cData2 - '0'); else
		if ((cData2 >= 'A') && (cData2 <= 'F')) dwParam += (cData2 - 'A' + 0x0A);
	}

	switch(nInternalCode)
	{
	// F0.F0.00.xx: Set CutOff
		case 0x00:
		{
			int oldcutoff = pChn->nCutOff;
			if (dwParam < 0x80)
			{
				// on the fist tick only, calculate step 
				if (m_dwSongFlags & SONG_FIRSTTICK)
				{
					pChn->m_nPlugInitialParamValue = pChn->nCutOff;
					// (dwParam & 0x7F) extracts the actual value that we're going to pass
					pChn->m_nPlugParamValueStep = (float)((int)dwParam-pChn->m_nPlugInitialParamValue)/(float)m_nMusicSpeed;
				}
				//update param on all ticks
				pChn->nCutOff = (BYTE) (pChn->m_nPlugInitialParamValue + (m_nTickCount+1)*pChn->m_nPlugParamValueStep + 0.5);
				pChn->nRestoreCutoffOnNewNote = 0;
			}
		#ifndef NO_FILTER
			oldcutoff -= pChn->nCutOff;
			if (oldcutoff < 0) oldcutoff = -oldcutoff;
			if ((pChn->nVolume > 0) || (oldcutoff < 0x10)
			|| (!(pChn->dwFlags & CHN_FILTER)) || (!(pChn->nLeftVol|pChn->nRightVol)))
				SetupChannelFilter(pChn, (pChn->dwFlags & CHN_FILTER) ? false : true);
		#endif // NO_FILTER
		} break;
	
	// F0.F0.01.xx: Set Resonance
		case 0x01:
			if (dwParam < 0x80)
			{
				// on the fist tick only, calculate step 
				if (m_dwSongFlags & SONG_FIRSTTICK)
				{
					pChn->m_nPlugInitialParamValue = pChn->nResonance;
					// (dwParam & 0x7F) extracts the actual value that we're going to pass
					pChn->m_nPlugParamValueStep = (float)((int)dwParam-pChn->m_nPlugInitialParamValue)/(float)m_nMusicSpeed;
				}
				//update param on all ticks
				pChn->nResonance = (BYTE) (pChn->m_nPlugInitialParamValue + (m_nTickCount+1)*pChn->m_nPlugParamValueStep + 0.5);
				pChn->nRestoreResonanceOnNewNote = 0;
			#ifndef NO_FILTER
				SetupChannelFilter(pChn, (pChn->dwFlags & CHN_FILTER) ? false : true);
			#endif // NO_FILTER	
			}

			break;

	// F0.F0.02.xx: Set filter mode
		case 0x02:
			if (dwParam < 0x20)
			{
				pChn->nFilterMode = (dwParam>>4);
			#ifndef NO_FILTER
				SetupChannelFilter(pChn, (pChn->dwFlags & CHN_FILTER) ? false : true);
			#endif // NO_FILTER
			}
			break;

	// F0.F0.03.xx: Set plug dry/wet
		case 0x03:
			{
				UINT nPlug = GetBestPlugin(nChn, PRIORITISE_CHANNEL, EVEN_IF_MUTED);
				if ((nPlug) && (nPlug <= MAX_MIXPLUGINS))	{
					// on the fist tick only, calculate step 
					if (m_dwSongFlags & SONG_FIRSTTICK)
					{
						pChn->m_nPlugInitialParamValue = m_MixPlugins[nPlug-1].fDryRatio;
						// (dwParam & 0x7F) extracts the actual value that we're going to pass
						pChn->m_nPlugParamValueStep =  ((1-((float)(dwParam)/127.0f))-pChn->m_nPlugInitialParamValue)/(float)m_nMusicSpeed;
					}
					//update param on all ticks
					IMixPlugin *pPlugin = m_MixPlugins[nPlug-1].pMixPlugin;
					if ((pPlugin) && (m_MixPlugins[nPlug-1].pMixState)) 	{
						m_MixPlugins[nPlug-1].fDryRatio = pChn->m_nPlugInitialParamValue+(float)(m_nTickCount+1)*pChn->m_nPlugParamValueStep;
					}
					
				}
			}
			break;
		

		// F0.F0.{80|n}.xx: Set VST effect parameter n to xx
		default:
			if (nInternalCode & 0x80 || extendedParam)
			{
				UINT nPlug = GetBestPlugin(nChn, PRIORITISE_CHANNEL, EVEN_IF_MUTED);
				if ((nPlug) && (nPlug <= MAX_MIXPLUGINS))
				{
					IMixPlugin *pPlugin = m_MixPlugins[nPlug-1].pMixPlugin;
					if ((pPlugin) && (m_MixPlugins[nPlug-1].pMixState))
					{
						// on the fist tick only, calculate step 
						if (m_dwSongFlags & SONG_FIRSTTICK)
						{
							pChn->m_nPlugInitialParamValue = pPlugin->GetZxxParameter(extendedParam?(0x80+nInternalCode):(nInternalCode&0x7F));
							// (dwParam & 0x7F) extracts the actual value that we're going to pass
							pChn->m_nPlugParamValueStep = ((int)(dwParam & 0x7F)-pChn->m_nPlugInitialParamValue)/(float)m_nMusicSpeed;
						}
						//update param on all ticks
						pPlugin->SetZxxParameter(extendedParam?(0x80+nInternalCode):(nInternalCode&0x7F), (UINT) (pChn->m_nPlugInitialParamValue + (m_nTickCount+1)*pChn->m_nPlugParamValueStep + 0.5));
					}
				}

			} 
	} // end switch

}
//end rewbs.smoothVST

//rewbs.volOffset: moved offset code to own method as it will be used in several places now
void CSoundFile::SampleOffset(UINT nChn, UINT param, bool bPorta)
//---------------------------------------------------------------
{

	MODCHANNEL *pChn = &Chn[nChn];
// -! NEW_FEATURE#0010
// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
// rewbs.NOTE: maybe move param calculation outside of method to cope with [ effect.
			//if (param) pChn->nOldOffset = param; else param = pChn->nOldOffset;
			//param <<= 8;
			//param |= (UINT)(pChn->nOldHiOffset) << 16;
			MODCOMMAND *m;
			m = NULL;

			if(m_nRow < Patterns[m_nPattern].GetNumRows()-1) m = Patterns[m_nPattern] + (m_nRow+1) * m_nChannels + nChn;

			if(m && m->command == CMD_XPARAM){
				UINT tmp = m->param;
				m = NULL;
				if(m_nRow < Patterns[m_nPattern].GetNumRows()-2) m = Patterns[m_nPattern] + (m_nRow+2) * m_nChannels  + nChn;

				if(m && m->command == CMD_XPARAM) param = (param<<16) + (tmp<<8) + m->param;
				else param = (param<<8) + tmp;
			}
			else{
				if (param) pChn->nOldOffset = param; else param = pChn->nOldOffset;
				param <<= 8;
				param |= (UINT)(pChn->nOldHiOffset) << 16;
			}
// -! NEW_FEATURE#0010

	if ((pChn->nRowNote >= NOTE_MIN) && (pChn->nRowNote <= NOTE_MAX))
	{
		/* if (bPorta)
			pChn->nPos = param;
		else
			pChn->nPos += param; */
		// The code above doesn't make sense at all. If there's a note and no porta, how on earth could nPos be something else than 0?
		// Anyway, keeping a debug assert here, just in case...
		ASSERT(bPorta || pChn->nPos == 0);

		// XM compatibility: Portamento + Offset = Ignore offset
		if(bPorta && IsCompatibleMode(TRK_FASTTRACKER2))
		{
			return;
		}
		pChn->nPos = param;

		if (pChn->nPos >= pChn->nLength)
		{
			// Offset beyond sample size
			if (!(m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2)))
			{
				// IT Compatibility: Offset
				if(IsCompatibleMode(TRK_IMPULSETRACKER))
				{
					if(m_dwSongFlags & SONG_ITOLDEFFECTS)
						pChn->nPos = pChn->nLength; // Old FX: Clip to end of sample
					else
						pChn->nPos = 0; // Reset to beginning of sample
				}
				else
				{
					pChn->nPos = pChn->nLoopStart;
					if ((m_dwSongFlags & SONG_ITOLDEFFECTS) && (pChn->nLength > 4))
					{
						pChn->nPos = pChn->nLength - 2;
					}
				}
			} else if(IsCompatibleMode(TRK_FASTTRACKER2))
			{
				// XM Compatibility: Don't play note
				pChn->dwFlags |= CHN_FASTVOLRAMP;
				pChn->nVolume = pChn->nPeriod = 0;
			}
		}
	} else
	if ((param < pChn->nLength) && (m_nType & (MOD_TYPE_MTM|MOD_TYPE_DMF)))
	{
		pChn->nPos = param;
	}

	return;
}
//end rewbs.volOffset:

void CSoundFile::RetrigNote(UINT nChn, int param, UINT offset)	//rewbs.VolOffset: added offset param.
//------------------------------------------------------------
{
	// Retrig: bit 8 is set if it's the new XM retrig
	MODCHANNEL *pChn = &Chn[nChn];
	int nRetrigSpeed = param & 0x0F;
	int nRetrigCount = pChn->nRetrigCount;
	bool bDoRetrig = false;

	//IT compatibility 15. Retrigger
	if(IsCompatibleMode(TRK_IMPULSETRACKER))
	{
		if ((m_dwSongFlags & SONG_FIRSTTICK) && pChn->nRowNote)
		{
			pChn->nRetrigCount = param & 0xf;
		}
		else if (!pChn->nRetrigCount || !--pChn->nRetrigCount)
		{
			pChn->nRetrigCount = param & 0xf;
			bDoRetrig = true;
		}
	}
	// buggy-like-hell FT2 Rxy retrig!
	else if(IsCompatibleMode(TRK_FASTTRACKER2) && (param & 0x100))
	{
		if(m_dwSongFlags & SONG_FIRSTTICK)
		{
			// here are some really stupid things FT2 does
			if(pChn->nRowVolCmd == VOLCMD_VOLUME) return;
			if(pChn->nRowInstr > 0 && pChn->nRowNote == NOTE_NONE) nRetrigCount = 1;
			if(pChn->nRowNote != NOTE_NONE && pChn->nRowNote <= GetModSpecifications().noteMax) nRetrigCount++;
		}
		if (nRetrigCount >= nRetrigSpeed)
		{
			if (!(m_dwSongFlags & SONG_FIRSTTICK) || (pChn->nRowNote == NOTE_NONE)) 
			{
				bDoRetrig = true;
				nRetrigCount = 0;
			}
		}
	} else
	{
		// old routines

		if (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			if (!nRetrigSpeed) nRetrigSpeed = 1;
			if ((nRetrigCount) && (!(nRetrigCount % nRetrigSpeed))) bDoRetrig = true;
			nRetrigCount++;
		} else
		{
			int realspeed = nRetrigSpeed;
			// FT2 bug: if a retrig (Rxy) occours together with a volume command, the first retrig interval is increased by one tick
			if ((param & 0x100) && (pChn->nRowVolCmd == VOLCMD_VOLUME) && (pChn->nRowParam & 0xF0)) realspeed++;
			if (!(m_dwSongFlags & SONG_FIRSTTICK) || (param & 0x100))
			{
				if (!realspeed) realspeed = 1;
				if ((!(param & 0x100)) && (m_nMusicSpeed) && (!(m_nTickCount % realspeed))) bDoRetrig = true;
				nRetrigCount++;
			} else if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2)) nRetrigCount = 0;
			if (nRetrigCount >= realspeed)
			{
				if ((m_nTickCount) || ((param & 0x100) && (!pChn->nRowNote))) bDoRetrig = true;
			}
		}
	}

	if (bDoRetrig)
	{
		UINT dv = (param >> 4) & 0x0F;
		if (dv)
		{
			int vol = pChn->nVolume;

			// XM compatibility: Retrig + volume will not change volume of retrigged notes
			if(!IsCompatibleMode(TRK_FASTTRACKER2) || !(pChn->nRowVolCmd == VOLCMD_VOLUME))
			{
				if (retrigTable1[dv])
					vol = (vol * retrigTable1[dv]) >> 4;
				else
					vol += ((int)retrigTable2[dv]) << 2;
			}

			vol = CLAMP(vol, 0, 256);

			pChn->nVolume = vol;
			pChn->dwFlags |= CHN_FASTVOLRAMP;
		}
		UINT nNote = pChn->nNewNote;
		LONG nOldPeriod = pChn->nPeriod;
		if ((nNote) && (nNote <= NOTE_MAX) && (pChn->nLength)) CheckNNA(nChn, 0, nNote, TRUE);
		bool bResetEnv = false;
		if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))
		{
			if ((pChn->nRowInstr) && (param < 0x100))
			{
				InstrumentChange(pChn, pChn->nRowInstr, false, false);
				bResetEnv = true;
			}
			if (param < 0x100) bResetEnv = true;
		}
		// IT compatibility: Really weird combination of envelopes and retrigger (see Storlek's q.it testcase)
		NoteChange(nChn, nNote, IsCompatibleMode(TRK_IMPULSETRACKER) ? true : false, bResetEnv);
		if (m_nInstruments) {
			ProcessMidiOut(nChn, pChn);	//Send retrig to Midi
		}
		if ((m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)) && (!pChn->nRowNote) && (nOldPeriod)) pChn->nPeriod = nOldPeriod;
		if (!(m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))) nRetrigCount = 0;
		// IT compatibility: see previous IT compatibility comment =)
		if(IsCompatibleMode(TRK_IMPULSETRACKER)) pChn->nPos = pChn->nPosLo = 0;

		if (offset)									//rewbs.volOffset: apply offset on retrig
		{
			if (pChn->pModSample)
				pChn->nLength = pChn->pModSample->nLength;
			SampleOffset(nChn, offset, false);
		}
	}

	// buggy-like-hell FT2 Rxy retrig!
	if(IsCompatibleMode(TRK_FASTTRACKER2) && (param & 0x100)) nRetrigCount++;

	// Now we can also store the retrig value for IT...
	if(!IsCompatibleMode(TRK_IMPULSETRACKER))
		pChn->nRetrigCount = (BYTE)nRetrigCount;
}


void CSoundFile::DoFreqSlide(MODCHANNEL *pChn, LONG nFreqSlide)
//-------------------------------------------------------------
{
	// IT Linear slides
	if (!pChn->nPeriod) return;
	if ((m_dwSongFlags & SONG_LINEARSLIDES) && (!(m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))))
	{
		LONG nOldPeriod = pChn->nPeriod;
		if (nFreqSlide < 0)
		{
			UINT n = (- nFreqSlide) >> 2;
			if (n)
			{
				if (n > 255) n = 255;
				pChn->nPeriod = _muldivr(pChn->nPeriod, LinearSlideDownTable[n], 65536);
				if (pChn->nPeriod == nOldPeriod) pChn->nPeriod = nOldPeriod-1;
			}
		} else
		{
			UINT n = (nFreqSlide) >> 2;
			if (n)
			{
				if (n > 255) n = 255;
				pChn->nPeriod = _muldivr(pChn->nPeriod, LinearSlideUpTable[n], 65536);
				if (pChn->nPeriod == nOldPeriod) pChn->nPeriod = nOldPeriod+1;
			}
		}
	} else
	{
		pChn->nPeriod += nFreqSlide;
	}
	if (pChn->nPeriod < 1)
	{
		pChn->nPeriod = 1;
		if (m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			pChn->dwFlags |= CHN_NOTEFADE;
			pChn->nFadeOutVol = 0;
		}
	}
}


void CSoundFile::NoteCut(UINT nChn, UINT nTick)
//---------------------------------------------
{
	if(nTick == 0)
	{
		//IT compatibility 22. SC0 == SC1
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
			nTick = 1;
		// ST3 doesn't cut notes with SC0
		else if(GetType() == MOD_TYPE_S3M)
			return;
	}

	if (m_nTickCount == nTick)
	{
		MODCHANNEL *pChn = &Chn[nChn];
		// if (m_nInstruments) KeyOff(pChn); ?
		pChn->nVolume = 0;
		// S3M/IT compatibility: Note Cut really cuts notes and does not just mute them (so that following volume commands could restore the sample)
		if(IsCompatibleMode(TRK_IMPULSETRACKER|TRK_SCREAMTRACKER))
		{
			pChn->nPeriod = 0;
		}
		pChn->dwFlags |= CHN_FASTVOLRAMP;

		MODINSTRUMENT *pHeader = pChn->pModInstrument;
		// instro sends to a midi chan
		if (pHeader && pHeader->nMidiChannel>0 && pHeader->nMidiChannel<17)
		{
			UINT nPlug = pHeader->nMixPlug;
			if ((nPlug) && (nPlug <= MAX_MIXPLUGINS))
			{
				IMixPlugin *pPlug = (IMixPlugin*)m_MixPlugins[nPlug-1].pMixPlugin;
				if (pPlug)
				{
					pPlug->MidiCommand(pHeader->nMidiChannel, pHeader->nMidiProgram, pHeader->wMidiBank, /*pChn->nNote+*/NOTE_KEYOFF, 0, nChn);
				}
			}
		}

	}
}


void CSoundFile::KeyOff(UINT nChn)
//--------------------------------
{
	MODCHANNEL *pChn = &Chn[nChn];
	const bool bKeyOn = (pChn->dwFlags & CHN_KEYOFF) ? false : true;
	pChn->dwFlags |= CHN_KEYOFF;
	//if ((!pChn->pModInstrument) || (!(pChn->dwFlags & CHN_VOLENV)))
	if ((pChn->pModInstrument) && (!(pChn->dwFlags & CHN_VOLENV)))
	{
		pChn->dwFlags |= CHN_NOTEFADE;
	}
	if (!pChn->nLength) return;
	if ((pChn->dwFlags & CHN_SUSTAINLOOP) && (pChn->pModSample) && (bKeyOn))
	{
		const MODSAMPLE *pSmp = pChn->pModSample;
		if (pSmp->uFlags & CHN_LOOP)
		{
			if (pSmp->uFlags & CHN_PINGPONGLOOP)
				pChn->dwFlags |= CHN_PINGPONGLOOP;
			else
				pChn->dwFlags &= ~(CHN_PINGPONGLOOP|CHN_PINGPONGFLAG);
			pChn->dwFlags |= CHN_LOOP;
			pChn->nLength = pSmp->nLength;
			pChn->nLoopStart = pSmp->nLoopStart;
			pChn->nLoopEnd = pSmp->nLoopEnd;
			if (pChn->nLength > pChn->nLoopEnd) pChn->nLength = pChn->nLoopEnd;
		} else
		{
			pChn->dwFlags &= ~(CHN_LOOP|CHN_PINGPONGLOOP|CHN_PINGPONGFLAG);
			pChn->nLength = pSmp->nLength;
		}
	}
	if (pChn->pModInstrument)
	{
		MODINSTRUMENT *pIns = pChn->pModInstrument;
		if (((pIns->VolEnv.dwFlags & ENV_LOOP) || (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))) && (pIns->nFadeOut))
		{
			pChn->dwFlags |= CHN_NOTEFADE;
		}
	
		if (pIns->VolEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET)
		{
			pChn->VolEnv.nEnvValueAtReleaseJump = getVolEnvValueFromPosition(pChn->VolEnv.nEnvPosition, pIns);
			pChn->VolEnv.nEnvPosition= pIns->VolEnv.Ticks[pIns->VolEnv.nReleaseNode];
		}

	}
}


//////////////////////////////////////////////////////////
// CSoundFile: Global Effects


void CSoundFile::SetSpeed(UINT param)
//-----------------------------------
{
	// ModPlug Tracker and Mod-Plugin don't do this check
#ifndef MODPLUG_TRACKER
#ifndef FASTSOUNDLIB
	// Big Hack!!!
	if ((!param) || (param >= 0x80) || ((m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM|MOD_TYPE_MT2)) && (param >= 0x1E)))
	{
		if ((!m_nRepeatCount) && (IsSongFinished(m_nCurrentPattern, m_nRow+1)))
		{
			GlobalFadeSong(1000);
		}
	}
#endif // FASTSOUNDLIB
#endif // MODPLUG_TRACKER
	//if ((m_nType & MOD_TYPE_S3M) && (param > 0x80)) param -= 0x80;
	// Allow high speed values here for VBlank MODs. (Maybe it would be better to have a "VBlank MOD" flag somewhere? Is it worth the effort?)
	if ((param) && (param <= GetModSpecifications().speedMax || (m_nType & MOD_TYPE_MOD))) m_nMusicSpeed = param;
}


void CSoundFile::SetTempo(UINT param, bool setAsNonModcommand)
//------------------------------------------------------------
{
	const CModSpecifications& specs = GetModSpecifications();
	if(setAsNonModcommand)
	{
		if(param < specs.tempoMin) m_nMusicTempo = specs.tempoMin;
		else
		{
			if(param > specs.tempoMax) m_nMusicTempo = specs.tempoMax;
			else m_nMusicTempo = param;
		}
	}
	else
	{
		if (param >= 0x20 && (m_dwSongFlags & SONG_FIRSTTICK)) //rewbs.tempoSlideFix: only set if not (T0x or T1x) and tick is 0
		{
			m_nMusicTempo = param;
		}
		// Tempo Slide
		else if (param < 0x20 && !(m_dwSongFlags & SONG_FIRSTTICK)) //rewbs.tempoSlideFix: only slide if (T0x or T1x) and tick is not 0
		{
			if ((param & 0xF0) == 0x10)
				m_nMusicTempo += (param & 0x0F); //rewbs.tempoSlideFix: no *2
			else
				m_nMusicTempo -= (param & 0x0F); //rewbs.tempoSlideFix: no *2

		// -> CODE#0016
		// -> DESC="default tempo update"
			if(IsCompatibleMode(TRK_ALLTRACKERS))	// clamp tempo correctly in compatible mode
				m_nMusicTempo = CLAMP(m_nMusicTempo, 32, 255);
			else
				m_nMusicTempo = CLAMP(m_nMusicTempo, specs.tempoMin, specs.tempoMax);
		// -! BEHAVIOUR_CHANGE#0016
		}
	}
}


int CSoundFile::PatternLoop(MODCHANNEL *pChn, UINT param)
//-------------------------------------------------------
{
	if (param)
	{
		if (pChn->nPatternLoopCount)
		{
			pChn->nPatternLoopCount--;
			if(!pChn->nPatternLoopCount)
			{
				//IT compatibility 10. Pattern loops (+ same fix for MOD files)
				if(IsCompatibleMode(TRK_IMPULSETRACKER | TRK_PROTRACKER))
					pChn->nPatternLoop = m_nRow + 1;

				return -1;	
			}
		} else
		{
			MODCHANNEL *p = Chn;

			//IT compatibility 10. Pattern loops (+ same fix for XM and MOD files)
			if(!IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2 | TRK_PROTRACKER))
			{
				for (UINT i=0; i<m_nChannels; i++, p++) if (p != pChn)
				{
					// Loop already done
					if (p->nPatternLoopCount) return -1;
				}
			}
			pChn->nPatternLoopCount = param;
		}
		m_nNextPatStartRow = pChn->nPatternLoop; // Nasty FT2 E60 bug emulation!
		return pChn->nPatternLoop;
	} else
	{
		pChn->nPatternLoop = m_nRow;
	}
	return -1;
}


void CSoundFile::GlobalVolSlide(UINT param, UINT * nOldGlobalVolSlide)
//-----------------------------------------
{
	LONG nGlbSlide = 0;
	if (param) *nOldGlobalVolSlide = param; else param = *nOldGlobalVolSlide;
	if (((param & 0x0F) == 0x0F) && (param & 0xF0))
	{
		if (m_dwSongFlags & SONG_FIRSTTICK) nGlbSlide = (param >> 4) * 2;
	} else
	if (((param & 0xF0) == 0xF0) && (param & 0x0F))
	{
		if (m_dwSongFlags & SONG_FIRSTTICK) nGlbSlide = - (int)((param & 0x0F) * 2);
	} else
	{
		if (!(m_dwSongFlags & SONG_FIRSTTICK))
		{
			if (param & 0xF0) nGlbSlide = (int)((param & 0xF0) >> 4) * 2;
			else nGlbSlide = -(int)((param & 0x0F) * 2);
		}
	}
	if (nGlbSlide)
	{
		if (!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))) nGlbSlide *= 2;
		nGlbSlide += m_nGlobalVolume;
		nGlbSlide = CLAMP(nGlbSlide, 0, 256);
		m_nGlobalVolume = nGlbSlide;
	}
}


DWORD CSoundFile::IsSongFinished(UINT nStartOrder, UINT nStartRow) const
//----------------------------------------------------------------------
{
	UINT nOrd;

	for (nOrd=nStartOrder; nOrd<Order.size(); nOrd++)
	{
		UINT nPat = Order[nOrd];
		if (nPat != Order.GetIgnoreIndex())
		{
			if (nPat >= Patterns.Size()) break;
			const MODPATTERN& p = Patterns[nPat];
			if (p)
			{
				UINT len = Patterns[nPat].GetNumRows() * m_nChannels;
				UINT pos = (nOrd == nStartOrder) ? nStartRow : 0;
				pos *= m_nChannels;
				while (pos < len)
				{
					UINT cmd;
					if ((p[pos].note) || (p[pos].volcmd)) return 0;
					cmd = p[pos].command;
					if (cmd == CMD_MODCMDEX)
					{
						UINT cmdex = p[pos].param & 0xF0;
						if ((!cmdex) || (cmdex == 0x60) || (cmdex == 0xE0) || (cmdex == 0xF0)) cmd = 0;
					}
					if ((cmd) && (cmd != CMD_SPEED) && (cmd != CMD_TEMPO)) return 0;
					pos++;
				}
			}
		}
	}
	return (nOrd < Order.size()) ? nOrd : Order.size()-1;
}


// This is how backward jumps should not be tested. :) (now unused)
BOOL CSoundFile::IsValidBackwardJump(UINT nStartOrder, UINT nStartRow, UINT nJumpOrder, UINT nJumpRow) const
//----------------------------------------------------------------------------------------------------------
{
	while ((nJumpOrder < Patterns.Size()) && (Order[nJumpOrder] == Order.GetIgnoreIndex())) nJumpOrder++;
	if ((nStartOrder >= Patterns.Size()) || (nJumpOrder >= Patterns.Size())) return FALSE;
	// Treat only case with jumps in the same pattern
	if (nJumpOrder > nStartOrder) return TRUE;

	if ((nJumpOrder < nStartOrder) || (nJumpRow >= Patterns[nStartOrder].GetNumRows())
	 || (!(Patterns[nStartOrder])) || (nStartRow >= MAX_PATTERN_ROWS) || (nJumpRow >= MAX_PATTERN_ROWS)) return FALSE;
	
	// See if the pattern is being played backward
	BYTE row_hist[MAX_PATTERN_ROWS];

	memset(row_hist, 0, sizeof(row_hist));
	UINT nRows = Patterns[nStartOrder].GetNumRows(), row = nJumpRow;

	if (nRows > MAX_PATTERN_ROWS) nRows = MAX_PATTERN_ROWS;
	row_hist[nStartRow] = TRUE;
	while ((row < MAX_PATTERN_ROWS) && (!row_hist[row]))
	{
		if (row >= nRows) return TRUE;
		row_hist[row] = TRUE;
		const MODCOMMAND* p = Patterns[nStartOrder].GetpModCommand(row, m_nChannels);
		row++;
		int breakrow = -1, posjump = 0;
		for (UINT i=0; i<m_nChannels; i++, p++)
		{
			if (p->command == CMD_POSITIONJUMP)
			{
				if (p->param < nStartOrder) return FALSE;
				if (p->param > nStartOrder) return TRUE;
				posjump = TRUE;
			} else
			if (p->command == CMD_PATTERNBREAK)
			{
				breakrow = p->param;
			}
		}
		if (breakrow >= 0)
		{
			if (!posjump) return TRUE;
			row = breakrow;
		}
		if (row >= nRows) return TRUE;
	}
	return FALSE;
}


//////////////////////////////////////////////////////
// Note/Period/Frequency functions

UINT CSoundFile::GetNoteFromPeriod(UINT period) const
//---------------------------------------------------
{
	if (!period) return 0;
	if (m_nType & (MOD_TYPE_MED|MOD_TYPE_MOD|MOD_TYPE_MTM|MOD_TYPE_669|MOD_TYPE_OKT|MOD_TYPE_AMF0))
	{
		period >>= 2;
		for (UINT i=0; i<6*12; i++)
		{
			if (period >= ProTrackerPeriodTable[i])
			{
				if ((period != ProTrackerPeriodTable[i]) && (i))
				{
					UINT p1 = ProTrackerPeriodTable[i-1];
					UINT p2 = ProTrackerPeriodTable[i];
					if (p1 - period < (period - p2)) return i+36;
				}
				return i+1+36;
			}
		}
		return 6*12+36;
	} else
	{
		for (UINT i=1; i<NOTE_MAX; i++)
		{
			LONG n = GetPeriodFromNote(i, 0, 0);
			if ((n > 0) && (n <= (LONG)period)) return i;
		}
		return NOTE_MAX;
	}
}



UINT CSoundFile::GetPeriodFromNote(UINT note, int nFineTune, UINT nC5Speed) const
//-------------------------------------------------------------------------------
{
	if ((!note) || (note >= NOTE_MIN_SPECIAL)) return 0;
	if (m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_S3M|MOD_TYPE_STM|MOD_TYPE_MDL|MOD_TYPE_ULT|MOD_TYPE_WAV
				|MOD_TYPE_FAR|MOD_TYPE_DMF|MOD_TYPE_PTM|MOD_TYPE_AMS|MOD_TYPE_DBM|MOD_TYPE_AMF|MOD_TYPE_PSM|MOD_TYPE_J2B|MOD_TYPE_IMF))
	{
		note--;
		if (m_dwSongFlags & SONG_LINEARSLIDES)
		{
			return (FreqS3MTable[note % 12] << 5) >> (note / 12);
		} else
		{
			if (!nC5Speed) nC5Speed = 8363;
			//(a*b)/c
			return _muldiv(8363, (FreqS3MTable[note % 12] << 5), nC5Speed << (note / 12));
			//8363 * freq[note%12] / nC5Speed * 2^(5-note/12)
		}
	} else
	if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))
	{
		if (note < 13) note = 13;
		note -= 13;
		if (m_dwSongFlags & SONG_LINEARSLIDES)
		{
			LONG l = ((NOTE_MAX - note) << 6) - (nFineTune / 2);
			if (l < 1) l = 1;
			return (UINT)l;
		} else
		{
			int finetune = nFineTune;
			UINT rnote = (note % 12) << 3;
			UINT roct = note / 12;
			int rfine = finetune / 16;
			int i = rnote + rfine + 8;
			if (i < 0) i = 0;
			if (i >= 104) i = 103;
			UINT per1 = XMPeriodTable[i];
			if ( finetune < 0 )
			{
				rfine--;
				finetune = -finetune;
			} else rfine++;
			i = rnote+rfine+8;
			if (i < 0) i = 0;
			if (i >= 104) i = 103;
			UINT per2 = XMPeriodTable[i];
			rfine = finetune & 0x0F;
			per1 *= 16-rfine;
			per2 *= rfine;
			return ((per1 + per2) << 1) >> roct;
		}
	} else
	{
		note--;
		nFineTune = XM2MODFineTune(nFineTune);
		if ((nFineTune) || (note < 36) || (note >= 36+6*12))
			return (ProTrackerTunedPeriods[nFineTune*12 + note % 12] << 5) >> (note / 12);
		else
			return (ProTrackerPeriodTable[note-36] << 2);
	}
}


UINT CSoundFile::GetFreqFromPeriod(UINT period, UINT nC5Speed, int nPeriodFrac) const
//-----------------------------------------------------------------------------------
{
	if (!period) return 0;
	if (m_nType & (MOD_TYPE_MED|MOD_TYPE_MOD|MOD_TYPE_MTM|MOD_TYPE_669|MOD_TYPE_OKT|MOD_TYPE_AMF0))
	{
		return (3546895L*4) / period;
	} else
	if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2))
	{
		if (m_dwSongFlags & SONG_LINEARSLIDES)
			return XMLinearTable[period % 768] >> (period / 768);
		else
			return 8363 * 1712L / period;
	} else
	{
		if (m_dwSongFlags & SONG_LINEARSLIDES)
		{
			if (!nC5Speed) nC5Speed = 8363;
			return _muldiv(nC5Speed, 1712L << 8, (period << 8)+nPeriodFrac);
		} else
		{
			return _muldiv(8363, 1712L << 8, (period << 8)+nPeriodFrac);
		}
	}
}


UINT  CSoundFile::GetBestPlugin(UINT nChn, UINT priority, bool respectMutes)
//-------------------------------------------------------------------------
{
	if (nChn > MAX_CHANNELS)		//Check valid channel number
	{
		return 0;
	}
	
	//Define search source order
	UINT nPlugin=0;
	switch (priority) {
		case CHANNEL_ONLY:						
			nPlugin = GetChannelPlugin(nChn, respectMutes);
			break;
		case INSTRUMENT_ONLY:						
			nPlugin  = GetActiveInstrumentPlugin(nChn, respectMutes);
			break;
		case PRIORITISE_INSTRUMENT:						
			nPlugin  = GetActiveInstrumentPlugin(nChn, respectMutes);
			if ((!nPlugin) || (nPlugin>MAX_MIXPLUGINS)) {
				nPlugin = GetChannelPlugin(nChn, respectMutes);
			}
			break;
		case PRIORITISE_CHANNEL:										
			nPlugin  = GetChannelPlugin(nChn, respectMutes);
			if ((!nPlugin) || (nPlugin>MAX_MIXPLUGINS)) {
				nPlugin = GetActiveInstrumentPlugin(nChn, respectMutes);
			}
			break;
	}

	return nPlugin; // 0 Means no plugin found.
}


UINT __cdecl CSoundFile::GetChannelPlugin(UINT nChn, bool respectMutes)
//--------------------------------------------------------------
{
	MODCHANNEL *pChn= &Chn[nChn];

	// If it looks like this is an NNA channel, we need to find the master channel.
	// This ensures we pick up the right ChnSettings. 
	// NB: nMasterChn==0 means no master channel, so we need to -1 to get correct index.
	if (nChn>=m_nChannels && pChn && pChn->nMasterChn>0) { 
		nChn = pChn->nMasterChn-1;				  
	}

	UINT nPlugin;
	if ( (respectMutes && (pChn->dwFlags & CHN_MUTE)) || 
	 (pChn->dwFlags&CHN_NOFX) ) {
		nPlugin = 0;
	} else {
		nPlugin = ChnSettings[nChn].nMixPlugin;
	}
	return nPlugin;
}


UINT CSoundFile::GetActiveInstrumentPlugin(UINT nChn, bool respectMutes)
//-----------------------------------------------------------------------
{
	MODCHANNEL *pChn = &Chn[nChn];
	// Unlike channel settings, pModInstrument is copied from the original chan to the NNA chan,
	// so we don't nee to worry about finding the master chan.

	UINT nPlugin=0;
	if (pChn && pChn->pModInstrument) {
		if (respectMutes && pChn->pModSample && (pChn->pModSample->uFlags & CHN_MUTE)) { 
			nPlugin = 0;
		} else {
			nPlugin = pChn->pModInstrument->nMixPlug;
		}
	}
	return nPlugin;
}


UINT CSoundFile::GetBestMidiChan(MODCHANNEL *pChn)
//------------------------------------------------
{
	if (pChn && pChn->pModInstrument && pChn->pModInstrument->nMidiChannel)
	{
		return (pChn->pModInstrument->nMidiChannel - 1) & 0x0F;
	}
	return 0;
}


void CSoundFile::HandlePatternTransitionEvents()
//----------------------------------------------
{
	if (!m_bPatternTransitionOccurred)
		return;

	// MPT sequence override
	if ((m_nSeqOverride > 0) && (m_nSeqOverride <= Order.size()))
	{
		if (m_dwSongFlags & SONG_PATTERNLOOP)
		{
			m_nPattern = Order[m_nSeqOverride - 1];
		}
		m_nNextPattern = m_nSeqOverride - 1;
		m_nSeqOverride = 0;
	}

	// Channel mutes
	for (CHANNELINDEX chan=0; chan<m_nChannels; chan++)
	{
		if (m_bChannelMuteTogglePending[chan])
		{
			if(m_pModDoc)
				m_pModDoc->MuteChannel(chan, !m_pModDoc->IsChannelMuted(chan));
			m_bChannelMuteTogglePending[chan] = false;
		}
	}

	m_bPatternTransitionOccurred = false;
}


// Update time signatures (global or pattern-specific). Don't forget to call this when changing the RPB/RPM settings anywhere!
void CSoundFile::UpdateTimeSignature()
//------------------------------------
{
	if(!Patterns.IsValidIndex(m_nPattern) || !Patterns[m_nPattern].GetOverrideSignature())
	{
		m_nCurrentRowsPerBeat = m_nDefaultRowsPerBeat;
		m_nCurrentRowsPerMeasure = m_nDefaultRowsPerMeasure;
	} else
	{
		m_nCurrentRowsPerBeat = Patterns[m_nPattern].GetRowsPerBeat();
		m_nCurrentRowsPerMeasure = Patterns[m_nPattern].GetRowsPerMeasure();
	}
}


void CSoundFile::PortamentoMPT(MODCHANNEL* pChn, int param)
//---------------------------------------------------------
{
	//Behavior: Modifies portamento by param-steps on every tick.
	//Note that step meaning depends on tuning.

	pChn->m_PortamentoFineSteps += param;
	pChn->m_CalculateFreq = true;
}


void CSoundFile::PortamentoFineMPT(MODCHANNEL* pChn, int param)
//-------------------------------------------------------------
{
	//Behavior: Divides portamento change between ticks/row. For example
	//if Ticks/row == 6, and param == +-6, portamento goes up/down by one tuning-dependent
	//fine step every tick.

	if(m_nTickCount == 0)
		pChn->nOldFinePortaUpDown = 0;

	const int tickParam = (m_nTickCount + 1.0) * param / m_nMusicSpeed;
	pChn->m_PortamentoFineSteps += (param >= 0) ? tickParam - pChn->nOldFinePortaUpDown : tickParam + pChn->nOldFinePortaUpDown;
	if(m_nTickCount + 1 == m_nMusicSpeed)
		pChn->nOldFinePortaUpDown = abs(param);
	else
		pChn->nOldFinePortaUpDown = abs(tickParam);

	pChn->m_CalculateFreq = true;
}


/* Now, some fun code begins: This will determine if a specific row in a pattern (orderlist item)
   has been visited before. This way, we can tell when the module starts to loop, i.e. when we have determined
   the song length (or found out that a given point of the module cannot be reached).
   The concept is actually very simple: Store a boolean value for every row for every possible orderlist item.

   Specific implementations:

   Length detection code:
   As the ModPlug engine already deals with pattern loops sufficiently (though not always correctly),
   there's no problem with (infinite) pattern loops in this code.
   
   Normal player code:
   Bare in mind that rows inside pattern loops should only be evaluated once, or else the algorithm will cancel too early!
   So in that case, the pattern loop rows have to be reset when looping back.
*/


// Resize / Clear the row vector.
// If bReset is true, the vector is not only resized to the required dimensions, but also completely cleared (i.e. all visited rows are unset).
// If pRowVector is specified, an alternative row vector instead of the module's global one will be used (f.e. when using GetLength()).
void CSoundFile::InitializeVisitedRows(const bool bReset, VisitedRowsType *pRowVector)
//------------------------------------------------------------------------------------
{
	if(pRowVector == nullptr)
	{
		pRowVector = &m_VisitedRows;
	}
	pRowVector->resize(Order.GetLengthTailTrimmed());

	for(ORDERINDEX nOrd = 0; nOrd < Order.GetLengthTailTrimmed(); nOrd++)
	{
		VisitedRowsBaseType *pRow = &(pRowVector->at(nOrd));
		// If we want to reset the vectors completely, we overwrite existing items with false.
		if(bReset)
		{
			pRow->assign(pRow->size(), false);
		}
		pRow->resize(GetVisitedRowsVectorSize(Order[nOrd]), false);
	}
}


// (Un)sets a given row as visited.
// nOrd, nRow - which row should be (un)set
// If bVisited is true, the row will be set as visited.
// If pRowVector is specified, an alternative row vector instead of the module's global one will be used (f.e. when using GetLength()).
void CSoundFile::SetRowVisited(const ORDERINDEX nOrd, const ROWINDEX nRow, const bool bVisited, VisitedRowsType *pRowVector)
//--------------------------------------------------------------------------------------------------------------------------
{
	const ORDERINDEX nMaxOrd = Order.GetLengthTailTrimmed();
	if(nOrd >= nMaxOrd || nRow > Patterns[Order[nOrd]].GetNumRows())
	{
		return;
	}

	if(pRowVector == nullptr)
	{
		pRowVector = &m_VisitedRows;
	}

	// The module might have been edited in the meantime - so we have to extend this a bit.
	if(nOrd >= pRowVector->size() || nRow >= pRowVector->at(nOrd).size())
	{
		InitializeVisitedRows(false, pRowVector);
	}

	pRowVector->at(nOrd)[nRow] = bVisited;
}


// Returns if a given row has been visited yet.
// If bAutoSet is true, the queried row will automatically be marked as visited.
// Use this parameter instead of consecutive IsRowVisited/SetRowVisited calls.
// If pRowVector is specified, an alternative row vector instead of the module's global one will be used (f.e. when using GetLength()).
bool CSoundFile::IsRowVisited(const ORDERINDEX nOrd, const ROWINDEX nRow, const bool bAutoSet, VisitedRowsType *pRowVector)
//-------------------------------------------------------------------------------------------------------------------------
{
	const ORDERINDEX nMaxOrd = Order.GetLengthTailTrimmed();
	if(nOrd >= nMaxOrd)
	{
		return false;
	}

	if(pRowVector == nullptr)
	{
		pRowVector = &m_VisitedRows;
	}

	// The row slot for this row has not been assigned yet - Just return false, as this means that the program has not played the row yet.
	if(nOrd >= pRowVector->size() || nRow >= pRowVector->at(nOrd).size())
	{
		if(bAutoSet)
		{
			SetRowVisited(nOrd, nRow, true, pRowVector);
		}
		return false;
	}

	if(pRowVector->at(nOrd)[nRow])
	{
		// we visited this row already - this module must be looping.
		return true;
	}

	if(bAutoSet)
	{
		pRowVector->at(nOrd)[nRow] = true;
	}

	return false;
}


// Get the needed vector size for pattern nPat.
size_t CSoundFile::GetVisitedRowsVectorSize(const PATTERNINDEX nPat)
//------------------------------------------------------------------
{
	if(Patterns.IsValidPat(nPat))
	{
		return (size_t)(Patterns[nPat].GetNumRows());
	}
	else
	{
		// invalid patterns consist of a "fake" row.
		return 1;
	}
}