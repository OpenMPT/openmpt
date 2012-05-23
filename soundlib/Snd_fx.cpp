/*
 * Snd_fx.cpp
 * -----------
 * Purpose: Processing of pattern commands, song length calculation...
 * Notes  : This needs some heavy refactoring.
 *          I thought of actually adding an effect interface class. Every pattern effect
 *          could then be moved into its own class that inherits from the effect interface.
 *          If effect handling differs severly between module formats, every format would have
 *          its own class for that effect. Then, a call chain of effect classes could be set up
 *          for each format, since effects cannot be processed in the same order in all formats.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "sndfile.h"
// -> CODE#0022
// -> DESC="alternative BPM/Speed interpretation method"
#include "../mptrack/mptrack.h"
#include "../mptrack/moddoc.h"
#include "../common/misc_util.h"
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


// Memory class for GetLength() code
class GetLengthMemory
{
public:
	double elapsedTime;
	UINT musicSpeed, musicTempo;
	LONG glbVol;
	vector<ModCommand::PARAM> oldGlbVolSlide;
	vector<ModCommand::NOTE> notes;
	vector<ModCommand::INSTR> instr;
	vector<ModCommand::PARAM> oldParam;
	vector<BYTE> vols;
	vector<UINT> chnVols;
	vector<double> patLoop;
	vector<ROWINDEX> patLoopStart;

protected:
	const CSoundFile &sndFile;

public:

	GetLengthMemory(const CSoundFile &sf) : sndFile(sf)
	{
		Reset();
	};

	void Reset()
	{
		elapsedTime = 0.0;
		musicSpeed = sndFile.m_nDefaultSpeed;
		musicTempo = sndFile.m_nDefaultTempo;
		glbVol = sndFile.m_nDefaultGlobalVolume;
		oldGlbVolSlide.assign(sndFile.GetNumChannels(), 0);
		instr.assign(sndFile.GetNumChannels(), 0);
		notes.assign(sndFile.GetNumChannels(), NOTE_NONE);
		vols.assign(sndFile.GetNumChannels(), 0xFF);
		oldParam.assign(sndFile.GetNumChannels(), 0);
		patLoop.assign(sndFile.GetNumChannels(), 0);
		patLoopStart.assign(sndFile.GetNumChannels(), 0);

		chnVols.resize(sndFile.GetNumChannels());
		for(CHANNELINDEX icv = 0; icv < sndFile.GetNumChannels(); icv++)
		{
			chnVols[icv] = sndFile.ChnSettings[icv].nVolume;
		}
	}
};


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

	// Are we trying to reach a certain pattern position?
	const bool hasSearchTarget = (endOrder != ORDERINDEX_INVALID && endRow != ROWINDEX_INVALID);

	ROWINDEX nRow = 0, nNextRow = 0;
	ROWINDEX nNextPatStartRow = 0; // FT2 E60 bug
	ORDERINDEX nCurrentOrder = 0, nNextOrder = 0;
	PATTERNINDEX nPattern = Order[0];

	GetLengthMemory memory(*this);
	// Temporary visited rows vector (so that GetLength() won't interfere with the player code if the module is playing at the same time)
	RowVisitor visitedRows(*this);

	for (;;)
	{
		UINT rowDelay = 0, tickDelay = 0;
		nRow = nNextRow;
		nCurrentOrder = nNextOrder;

		if(nCurrentOrder >= Order.size())
			break;

		// Check if pattern is valid
		nPattern = Order[nCurrentOrder];
		bool positionJumpOnThisRow = false;
		bool patternBreakOnThisRow = false;

		while(nPattern >= Patterns.Size())
		{
			// End of song?
			if((nPattern == Order.GetInvalidPatIndex()) || (nCurrentOrder >= Order.size()))
			{
				if(nCurrentOrder == m_nRestartPos)
					break;
				else
					nCurrentOrder = m_nRestartPos;
			} else
			{
				nCurrentOrder++;
			}
			nPattern = (nCurrentOrder < Order.size()) ? Order[nCurrentOrder] : Order.GetInvalidPatIndex();
			nNextOrder = nCurrentOrder;
			if((!Patterns.IsValidPat(nPattern)) && visitedRows.IsVisited(nCurrentOrder, 0, true))
			{
				if(!hasSearchTarget || !visitedRows.GetFirstUnvisitedRow(nNextOrder, nNextRow, true))
				{
					// We aren't searching for a specific row, or we couldn't find any more unvisited rows.
					break;
				} else
				{
					// We haven't found the target row yet, but we found some other unplayed row... continue searching from here.
					memory.Reset();
					continue;
				}
			}
		}
		// Skip non-existing patterns
		if ((nPattern >= Patterns.Size()) || (!Patterns[nPattern]))
		{
			// If there isn't even a tune, we should probably stop here.
			if(nCurrentOrder == m_nRestartPos)
			{
				if(!hasSearchTarget || !visitedRows.GetFirstUnvisitedRow(nNextOrder, nNextRow, true))
				{
					// We aren't searching for a specific row, or we couldn't find any more unvisited rows.
					break;
				} else
				{
					// We haven't found the target row yet, but we found some other unplayed row... continue searching from here.
					memory.Reset();
					continue;
				}
			}
			nNextOrder = nCurrentOrder + 1;
			continue;
		}
		// Should never happen
		if (nRow >= Patterns[nPattern].GetNumRows())
			nRow = 0;

		//Check whether target reached.
		if(nCurrentOrder == endOrder && nRow == endRow)
		{
			retval.targetReached = true;
			break;
		}

		if(visitedRows.IsVisited(nCurrentOrder, nRow, true))
		{
			if(!hasSearchTarget || !visitedRows.GetFirstUnvisitedRow(nNextOrder, nNextRow, true))
			{
				// We aren't searching for a specific row, or we couldn't find any more unvisited rows.
				break;
			} else
			{
				// We haven't found the target row yet, but we found some other unplayed row... continue searching from here.
				memory.Reset();
				continue;
			}
		}

		retval.endOrder = nCurrentOrder;
		retval.endRow = nRow;

		// Update next position
		nNextRow = nRow + 1;

		if (!nRow)
		{
			for(UINT ipck = 0; ipck < m_nChannels; ipck++)
				memory.patLoop[ipck] = memory.elapsedTime;
		}

		ModChannel *pChn = Chn;
		ModCommand *p = Patterns[nPattern].GetRow(nRow);
		ModCommand *nextRow = nullptr;
		for (CHANNELINDEX nChn = 0; nChn < m_nChannels; p++, pChn++, nChn++) if (*((DWORD *)p))
		{
			if((GetType() == MOD_TYPE_S3M) && (ChnSettings[nChn].dwFlags & CHN_MUTE) != 0)	// not even effects are processed on muted S3M channels
				continue;
			ModCommand::COMMAND command = p->command;
			ModCommand::PARAM param = p->param;
			ModCommand::NOTE note = p->note;
			if (p->instr) { memory.instr[nChn] = p->instr; memory.notes[nChn] = NOTE_NONE; memory.vols[nChn] = 0xFF; }
			if ((note >= NOTE_MIN) && (note <= NOTE_MAX)) memory.notes[nChn] = note;
			if (p->volcmd == VOLCMD_VOLUME)	{ memory.vols[nChn] = p->vol; }
			switch(command)
			{
			// Position Jump
			case CMD_POSITIONJUMP:
				positionJumpOnThisRow = true;
				nNextOrder = (ORDERINDEX)param;
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
				if(param >= 64 && (GetType() & MOD_TYPE_S3M))
				{
					// ST3 ignores invalid pattern breaks.
					break;
				}
				patternBreakOnThisRow = true;
				//Try to check next row for XPARAM
				nextRow = nullptr;
				nNextPatStartRow = 0;  // FT2 E60 bug
				if (nRow < Patterns[nPattern].GetNumRows() - 1)
				{
					nextRow = Patterns[nPattern].GetpModCommand(nRow +1, nChn);
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
					nNextOrder = nCurrentOrder + 1;
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
					memory.musicSpeed = param;
				}
				break;
			// Set Tempo
			case CMD_TEMPO:
				if ((adjustMode & eAdjust) && (m_nType & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)))
				{
					if (param) pChn->nOldTempo = (BYTE)param; else param = pChn->nOldTempo;
				}
				if (param >= 0x20) memory.musicTempo = param; else
				// Tempo Slide
				if ((param & 0xF0) == 0x10)
				{
					memory.musicTempo += (param & 0x0F)  * (memory.musicSpeed - 1);  //rewbs.tempoSlideFix
				} else
				{
					memory.musicTempo -= (param & 0x0F) * (memory.musicSpeed - 1); //rewbs.tempoSlideFix
				}
// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
				if(IsCompatibleMode(TRK_ALLTRACKERS))	// clamp tempo correctly in compatible mode
					memory.musicTempo = CLAMP(memory.musicTempo, 32, 255);
				else
					memory.musicTempo = CLAMP(memory.musicTempo, GetModSpecifications().tempoMin, GetModSpecifications().tempoMax);
// -! NEW_FEATURE#0010
				break;

			case CMD_S3MCMDEX:
				if((param & 0xF0) == 0x60)
				{
					// Fine Pattern Delay
					tickDelay += (param & 0x0F);
				} else if((param & 0xF0) == 0xE0 && !rowDelay)
				{
					// Pattern Delay
					if(!(GetType() & MOD_TYPE_S3M) || (param & 0x0F) != 0)
					{
						// While Impulse Tracker *does* count S60 as a valid row delay (and thus ignores any other row delay commands on the right),
						// Scream Tracker 3 simply ignores such commands.
						rowDelay = 1 + (param & 0x0F);
					}
				} else if((param & 0xF0) == 0xA0)
				{
					// High sample offset
					pChn->nOldHiOffset = param & 0x0F;
				} else if((param & 0xF0) == 0xB0)
				{
					// Pattern Loop
					if (param & 0x0F)
					{
						memory.elapsedTime += (memory.elapsedTime - memory.patLoop[nChn]) * (double)(param & 0x0F);
					} else
					{
						memory.patLoop[nChn] = memory.elapsedTime;
						memory.patLoopStart[nChn] = nRow;
					}
				}
				break;

			case CMD_MODCMDEX:
				if((param & 0xF0) == 0xE0)
				{
					// Pattern Delay
					rowDelay = 1 + (param & 0x0F);
				} else if ((param & 0xF0) == 0x60)
				{
					// Pattern Loop
					if (param & 0x0F)
					{
						memory.elapsedTime += (memory.elapsedTime - memory.patLoop[nChn]) * (double)(param & 0x0F);
						nNextPatStartRow = memory.patLoopStart[nChn]; // FT2 E60 bug
					} else
					{
						memory.patLoop[nChn] = memory.elapsedTime;
						memory.patLoopStart[nChn] = nRow;
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
				memory.vols[nChn] = param;
				break;
			// Global Volume
			case CMD_GLOBALVOLUME:
				// ST3 applies global volume on tick 1 and does other weird things, but we won't emulate this for now.
// 				if((GetType() & MOD_TYPE_S3M) && memory.musicSpeed <= 1)
// 				{
// 					break;
// 				}

				if (!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) param <<= 1;
				// IT compatibility 16. FT2, ST3 and IT ignore out-of-range values
				if (param <= 128)
				{
					memory.glbVol = param << 1;
				}
				break;
			// Global Volume Slide
			case CMD_GLOBALVOLSLIDE:
				if(IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2))
				{
					//IT compatibility 16. Global volume slide params are stored per channel (FT2/IT)
					if (param) memory.oldGlbVolSlide[nChn] = param; else param = memory.oldGlbVolSlide[nChn];
				} else
				{
					if (param) memory.oldGlbVolSlide[0] = param; else param = memory.oldGlbVolSlide[0];
				}
				if (((param & 0x0F) == 0x0F) && (param & 0xF0))
				{
					param >>= 4;
					if (!(GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))) param <<= 1;
					memory.glbVol += param << 1;
				} else
				if (((param & 0xF0) == 0xF0) && (param & 0x0F))
				{
					param = (param & 0x0F) << 1;
					if (!(GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))) param <<= 1;
					memory.glbVol -= param;
				} else
				if (param & 0xF0)
				{
					param >>= 4;
					param <<= 1;
					if (!(GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))) param <<= 1;
					memory.glbVol += param * memory.musicSpeed;
				} else
				{
					param = (param & 0x0F) << 1;
					if (!(GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))) param <<= 1;
					memory.glbVol -= param * memory.musicSpeed;
				}
				memory.glbVol = CLAMP(memory.glbVol, 0, 256);
				break;
			case CMD_CHANNELVOLUME:
				if (param <= 64) memory.chnVols[nChn] = param;
				break;
			case CMD_CHANNELVOLSLIDE:
				if (param) memory.oldParam[nChn] = param; else param = memory.oldParam[nChn];
				pChn->nOldChnVolSlide = param;
				if (((param & 0x0F) == 0x0F) && (param & 0xF0))
				{
					param = (param >> 4) + memory.chnVols[nChn];
				} else
				if (((param & 0xF0) == 0xF0) && (param & 0x0F))
				{
					if (memory.chnVols[nChn] > (param & 0x0F)) param = memory.chnVols[nChn] - (param & 0x0F);
					else param = 0;
				} else
				if (param & 0x0F)
				{
					param = (param & 0x0F) * memory.musicSpeed;
					param = (memory.chnVols[nChn] > param) ? memory.chnVols[nChn] - param : 0;
				} else param = ((param & 0xF0) >> 4) * memory.musicSpeed + memory.chnVols[nChn];
				param = min(param, 64);
				memory.chnVols[nChn] = param;
				break;
			}
		}

		if (nNextRow >= Patterns[nPattern].GetNumRows())
		{
			nNextOrder = nCurrentOrder + 1;
			nNextRow = 0;
			if(IsCompatibleMode(TRK_FASTTRACKER2)) nNextRow = nNextPatStartRow;  // FT2 E60 bug
			nNextPatStartRow = 0;
		}

		// XXX this does not take per-pattern time signatures into consideration!
		memory.elapsedTime += GetRowDuration(memory.musicTempo, memory.musicSpeed, (memory.musicSpeed + tickDelay) * max(rowDelay, 1));
	}

	if(retval.targetReached || endOrder == ORDERINDEX_INVALID || endRow == ROWINDEX_INVALID)
	{
		retval.lastOrder = nCurrentOrder;
		retval.lastRow = nRow;
	}
	retval.duration = memory.elapsedTime / 1000.0;

	// Store final variables
	if ((adjustMode & eAdjust))
	{
		if (retval.targetReached || endOrder == ORDERINDEX_INVALID || endRow == ROWINDEX_INVALID)
		{
			// Target found, or there is no target (i.e. play whole song)...
			m_nGlobalVolume = memory.glbVol;
			if(IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2))
			{
				//IT compatibility 16. Global volume slide params are stored per channel (FT2/IT)
				for(CHANNELINDEX n = 0; n < GetNumChannels(); n++)
				{
					Chn[n].nOldGlobalVolSlide = memory.oldGlbVolSlide[n];
				}
			} else
			{
				m_nOldGlbVolSlide = memory.oldGlbVolSlide[0];
			}
			m_nMusicSpeed = memory.musicSpeed;
			m_nMusicTempo = memory.musicTempo;
			for (CHANNELINDEX n = 0; n < GetNumChannels(); n++)
			{
				Chn[n].nGlobalVol = memory.chnVols[n];
				if (memory.notes[n] != NOTE_NONE)
				{
					Chn[n].nNewNote = memory.notes[n];
					if(ModCommand::IsNote(memory.notes[n]))
					{
						Chn[n].nLastNote = memory.notes[n];
					}
				}
				if (memory.instr[n]) Chn[n].nNewIns = memory.instr[n];
				if (memory.vols[n] != 0xFF)
				{
					if (memory.vols[n] > 64) memory.vols[n] = 64;
					Chn[n].nVolume = memory.vols[n] * 4;
				}
			}
		} else if(adjustMode != eAdjustOnSuccess)
		{
			// Target not found (e.g. when jumping to a hidden sub song), reset global variables...
			m_nMusicSpeed = m_nDefaultSpeed;
			m_nMusicTempo = m_nDefaultTempo;
			m_nGlobalVolume = m_nDefaultGlobalVolume;
		}
		// When adjusting the playback status, we will also want to update the visited rows vector according to the current position.
		visitedSongRows.Set(visitedRows);
	}

	return retval;

}


//////////////////////////////////////////////////////////////////////////////////////////////////
// Effects

// Change sample or instrument number.
void CSoundFile::InstrumentChange(ModChannel *pChn, UINT instr, bool bPorta, bool bUpdVol, bool bResetEnv)
//--------------------------------------------------------------------------------------------------------
{
	if (instr >= MAX_INSTRUMENTS) return;
	ModInstrument *pIns = (instr < MAX_INSTRUMENTS) ? Instruments[instr] : nullptr;
	ModSample *pSmp = &Samples[instr];
	UINT note = pChn->nNewNote;

	if(note == NOTE_NONE && IsCompatibleMode(TRK_IMPULSETRACKER)) return;

	if (pIns != nullptr && ModCommand::IsNote(note))
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
			// and it is also ignored when the instrument number changes. This fixes e.g. the guitars in "Ultima Ratio" by Nebularia
			// and some slides in spx-shuttledeparture.it.
			pSmp = pChn->pModSample;
		} else
		{
			// Original behaviour
			if(pIns->NoteMap[note - 1] > NOTE_MAX) return;
			UINT n = pIns->Keyboard[note - 1];
			pSmp = ((n) && (n < MAX_SAMPLES)) ? &Samples[n] : nullptr;
		}
	} else if (GetNumInstruments())
	{
		// No valid instrument, or not a valid note.
		if (note >= NOTE_MIN_SPECIAL) return;
		pSmp = nullptr;
	}

	const bool bNewTuning = (GetType() == MOD_TYPE_MPT && pIns && pIns->pTuning);
	// Playback behavior change for MPT: With portamento don't change sample if it is in
	// the same instrument as previous sample.
	if(bPorta && bNewTuning && pIns == pChn->pModInstrument)
		return;

	bool returnAfterVolumeAdjust = false;

	// instrumentChanged is used for IT carry-on env option
	bool instrumentChanged = (pIns != pChn->pModInstrument);

	if(!instrumentChanged)
	{
		// Special XM hack
		if ((bPorta) && (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2)) && (pIns)
		&& (pChn->pModSample) && (pSmp != pChn->pModSample))
		{
			// FT2 doesn't change the sample in this case,
			// but still uses the sample info from the old one (bug?)
			returnAfterVolumeAdjust = true;
		}
	}

	// IT Compatibility: Envelope pickup after SCx cut
	// Test case: cut-carry.it
	if(pChn->nInc == 0 && (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)))
	{
		instrumentChanged = true;
	}

	// XM compatibility: new instrument + portamento = ignore new instrument number, but reload old instrument settings (the world of XM is upside down...)
	// And this does *not* happen if volume column portamento is used together with note delay... (handled in ProcessEffects(), where all the other note delay stuff is.)
	// Test case: porta-delay.xm
	if(instrumentChanged && bPorta && IsCompatibleMode(TRK_FASTTRACKER2))
	{
		pIns = pChn->pModInstrument;
		pSmp = pChn->pModSample;
		instrumentChanged = false;
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

	// IT Compatiblity: NNA is reset on every note change, not every instrument change (fixes s7xinsnum.it).
	if (pIns && !IsCompatibleMode(TRK_IMPULSETRACKER)  && (pIns->nMixPlug || pSmp))		//rewbs.VSTiNNA
		pChn->nNNA = pIns->nNNA;

	if (pSmp)
	{
		if (pIns)
		{
			pChn->nInsVol = (pSmp->nGlobalVol * pIns->nGlobalVol) >> 6;
			// Default instrument panning
			if (pIns->dwFlags & INS_SETPANNING)
			{
				pChn->nPan = pIns->nPan;
				// IT compatibility: Sample and instrument panning overrides channel surround status.
				// Test case: SmpInsPanSurround.it
				if(IsCompatibleMode(TRK_IMPULSETRACKER) && !(m_dwSongFlags & SONG_SURROUNDPAN))
				{
					pChn->dwFlags &= ~CHN_SURROUND;
				}
			}
		} else
		{
			pChn->nInsVol = pSmp->nGlobalVol;
		}

		// Default sample panning
		if((pSmp->uFlags & CHN_PANNING) || (GetType() & MOD_TYPE_XM))
		{
			pChn->nPan = pSmp->nPan;
			// IT compatibility: Sample and instrument panning overrides channel surround status.
			// Test case: SmpInsPanSurround.it
			if(IsCompatibleMode(TRK_IMPULSETRACKER) && !(m_dwSongFlags & SONG_SURROUNDPAN))
			{
				pChn->dwFlags &= ~CHN_SURROUND;
			}
		}
	}


	// Reset envelopes
	if (bResetEnv)
	{
		if ((!bPorta) || (!(GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))) || (m_dwSongFlags & SONG_ITCOMPATGXX)
		 || (!pChn->nLength) || ((pChn->dwFlags & CHN_NOTEFADE) && (!pChn->nFadeOutVol))
		 //IT compatibility tentative fix: Reset envelopes when instrument changes.
		 || (IsCompatibleMode(TRK_IMPULSETRACKER) && instrumentChanged))
		{
			pChn->dwFlags |= CHN_FASTVOLRAMP;
			if ((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (!instrumentChanged) && (pIns) && (!(pChn->dwFlags & (CHN_KEYOFF|CHN_NOTEFADE))))
			{
				if (!(pIns->VolEnv.dwFlags & ENV_CARRY)) pChn->VolEnv.Reset();
				if (!(pIns->PanEnv.dwFlags & ENV_CARRY)) pChn->PanEnv.Reset();
				if (!(pIns->PitchEnv.dwFlags & ENV_CARRY)) pChn->PitchEnv.Reset();
			} else
			{
				pChn->ResetEnvelopes();
			}
			// IT Compatibility: Autovibrato reset
			if(!IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				pChn->nAutoVibDepth = 0;
				pChn->nAutoVibPos = 0;
			}
		} else if ((pIns) && (!(pIns->VolEnv.dwFlags & ENV_ENABLED)))
		{
			if(IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				pChn->VolEnv.Reset();
			} else
			{
				pChn->ResetEnvelopes();
			}
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
	if (bPorta && pSmp == pChn->pModSample)
	{
		// If channel length is 0, we cut a previous sample using SCx. In that case, we have to update sample length, loop points, etc...
		if(GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT) && pChn->nLength != 0) return;
		pChn->dwFlags &= ~(CHN_KEYOFF|CHN_NOTEFADE);
		pChn->dwFlags = (pChn->dwFlags & (CHN_CHANNELFLAGS | CHN_PINGPONGFLAG)) | (pSmp->uFlags & CHN_SAMPLEFLAGS);
	} else
	{
		pChn->dwFlags &= ~(CHN_KEYOFF|CHN_NOTEFADE);

		// IT compatibility tentative fix: Don't change bidi loop direction when
		// no sample nor instrument is changed.
		if(IsCompatibleMode(TRK_ALLTRACKERS) && pSmp == pChn->pModSample && !instrumentChanged)
			pChn->dwFlags = (pChn->dwFlags & (CHN_CHANNELFLAGS | CHN_PINGPONGFLAG)) | (pSmp->uFlags & CHN_SAMPLEFLAGS);
		else
			pChn->dwFlags = (pChn->dwFlags & CHN_CHANNELFLAGS) | (pSmp->uFlags & CHN_SAMPLEFLAGS);


		if (pIns)
		{
			// Copy envelope flags (we actually only need the "enabled" and "pitch" flag)
			pChn->VolEnv.flags = pIns->VolEnv.dwFlags;
			pChn->PanEnv.flags = pIns->PanEnv.dwFlags;
			pChn->PitchEnv.flags = pIns->PitchEnv.dwFlags;

			// A cutoff frequency of 0 should not be reset just because the filter envelope is enabled.
			// Test case: FilterEnvReset.it
			if ((pIns->PitchEnv.dwFlags & (ENV_ENABLED | ENV_FILTER)) == (ENV_ENABLED | ENV_FILTER) && !IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				if (!pChn->nCutOff) pChn->nCutOff = 0x7F;
			}

			if (pIns->IsCutoffEnabled()) pChn->nCutOff = pIns->GetCutoff();
			if (pIns->IsResonanceEnabled()) pChn->nResonance = pIns->GetResonance();
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
	} else
	{
		pChn->nC5Speed = pSmp->nC5Speed;
		pChn->nFineTune = pSmp->nFineTune;
	}


	pChn->pSample = pSmp->pSample;
	pChn->nTranspose = pSmp->RelativeTone;

	// FT2 compatibility: Don't reset portamento target with new instrument numbers.
	// Test case: Porta-Pickup.xm
	if(!IsCompatibleMode(TRK_FASTTRACKER2))
	{
		pChn->nPortamentoDest = 0;
	}
	pChn->m_PortamentoFineSteps = 0;

	if (pChn->dwFlags & CHN_SUSTAINLOOP)
	{
		pChn->nLoopStart = pSmp->nSustainStart;
		pChn->nLoopEnd = pSmp->nSustainEnd;
		pChn->dwFlags |= CHN_LOOP;
		if (pChn->dwFlags & CHN_PINGPONGSUSTAIN) pChn->dwFlags |= CHN_PINGPONGLOOP;
	}
	if ((pChn->dwFlags & CHN_LOOP) && (pChn->nLoopEnd < pChn->nLength)) pChn->nLength = pChn->nLoopEnd;

	// Fix sample position on instrument change. This is needed for PT1x MOD and IT "on the fly" sample change.
	// XXX is this actually called? In ProcessEffects(), a note-on effect is emulated if there's an on the fly sample change!
	if(pChn->nPos >= pChn->nLength)
	{
		if((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)))
		{
			pChn->nPos = pChn->nPosLo = 0;
		} else if((GetType() & MOD_TYPE_MOD))	// TODO does not always seem to work, especially with short chip samples?
		{
			pChn->nPos = pChn->nLoopStart;
			pChn->nPosLo = 0;
		}
	}
}


void CSoundFile::NoteChange(CHANNELINDEX nChn, int note, bool bPorta, bool bResetEnv, bool bManual)
//-------------------------------------------------------------------------------------------------
{
	if (note < NOTE_MIN) return;
	ModChannel * const pChn = &Chn[nChn];
	ModSample *pSmp = pChn->pModSample;
	ModInstrument *pIns = pChn->pModInstrument;

	const bool newTuning = (GetType() == MOD_TYPE_MPT && pIns != nullptr && pIns->pTuning);
	// save the note that's actually used, as it's necessary to properly calculate PPS and stuff
	const int realnote = note;

	if ((pIns) && (note <= 0x80))
	{
		UINT n = pIns->Keyboard[note - NOTE_MIN];
		if ((n) && (n < MAX_SAMPLES)) pSmp = &Samples[n];
		note = pIns->NoteMap[note-1];
	}
	// Key Off
	if (note > NOTE_MAX)
	{
		// Key Off (+ Invalid Note for XM - TODO is this correct?)
		if (note == NOTE_KEYOFF || !(GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)))
		{
			KeyOff(nChn);
		}
		else // Invalid Note -> Note Fade
		{
			if(/*note == NOTE_FADE && */ GetNumInstruments())
				pChn->dwFlags |= CHN_NOTEFADE;
		}

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
			pChn->nNote = pChn->nNewNote = NOTE_NONE;
		}
		return;
	}

	if(newTuning)
	{
		if(!bPorta || pChn->nNote == NOTE_NONE)
			pChn->nPortamentoDest = 0;
		else
		{
			pChn->nPortamentoDest = pIns->pTuning->GetStepDistance(pChn->nNote, pChn->m_PortamentoFineSteps, note, 0);
			//Here pChn->nPortamentoDest means 'steps to slide'.
			pChn->m_PortamentoFineSteps = -pChn->nPortamentoDest;
		}
	}

	if ((!bPorta) && (GetType() & (MOD_TYPE_XM|MOD_TYPE_MED|MOD_TYPE_MT2)))
	{
		if (pSmp)
		{
			pChn->nTranspose = pSmp->RelativeTone;
			pChn->nFineTune = pSmp->nFineTune;
		}
	}
	// IT Compatibility: Update multisample instruments frequency even if instrument is not specified (fixes the guitars in spx-shuttledeparture.it)
	if(!bPorta && pSmp && IsCompatibleMode(TRK_IMPULSETRACKER)) pChn->nC5Speed = pSmp->nC5Speed;

	if(bPorta && pChn->nInc == 0)
	{
		if(IsCompatibleMode(TRK_FASTTRACKER2))
		{
			// XM Compatibility: Ignore notes with portamento if there was no note playing.
			// Test case: 3xx-no-old-samp.xm
			pChn->nPeriod = 0;
			return;
		} else if(IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			// IT Compatibility: Ignore portamento command if no note was playing (e.g. if a previous note has faded out).
			// Test case: Fade-Porta.it
			bPorta = false;
		}
	}

	if (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2|MOD_TYPE_MED))
	{
		note += pChn->nTranspose;
		// RealNote = PatternNote + RelativeTone; (0..118, 0 = C-0, 118 = A#9)
		Limit(note, NOTE_MIN + 11, NOTE_MIN + 130);	// 119 possible notes
	} else
	{
		Limit(note, NOTE_MIN, NOTE_MAX);
	}
	if(IsCompatibleMode(TRK_IMPULSETRACKER))
	{
		// need to memorize the original note for various effects (e.g. PPS)
		pChn->nNote = CLAMP(realnote, NOTE_MIN, NOTE_MAX);
	} else
	{
		pChn->nNote = note;
	}
	pChn->m_CalculateFreq = true;

	if ((!bPorta) || (GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)))
		pChn->nNewIns = 0;

	UINT period = GetPeriodFromNote(note, pChn->nFineTune, pChn->nC5Speed);

	if (!pSmp) return;
	if (period)
	{
		if ((!bPorta) || (!pChn->nPeriod)) pChn->nPeriod = period;
		if(!newTuning)
		{
			// FT2 compatibility: Don't reset portamento target with new notes.
			// Test case: Porta-Pickup.xm
			if(bPorta || !IsCompatibleMode(TRK_FASTTRACKER2))
			{
				pChn->nPortamentoDest = period;
			}
		}

		if (!bPorta || (!pChn->nLength && !(GetType() & MOD_TYPE_S3M)))
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
	{
		bPorta = false;
	}

	if ((!bPorta) || (!(GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)))
	 || ((pChn->dwFlags & CHN_NOTEFADE) && (!pChn->nFadeOutVol))
	 || ((m_dwSongFlags & SONG_ITCOMPATGXX) && (pChn->rowCommand.instr)))
	{
		if ((GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)) && (pChn->dwFlags & CHN_NOTEFADE) && (!pChn->nFadeOutVol))
		{
			pChn->ResetEnvelopes();
			// IT Compatibility: Autovibrato reset
			if(!IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				pChn->nAutoVibDepth = 0;
				pChn->nAutoVibPos = 0;
			}
			pChn->dwFlags &= ~CHN_NOTEFADE;
			pChn->nFadeOutVol = 65536;
		}
		if ((!bPorta) || (!(m_dwSongFlags & SONG_ITCOMPATGXX)) || (pChn->rowCommand.instr))
		{
			if ((!(GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))) || (pChn->rowCommand.instr))
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
				// IT Compatiblity: NNA is reset on every note change, not every instrument change (fixes spx-farspacedance.it).
				if(IsCompatibleMode(TRK_IMPULSETRACKER)) pChn->nNNA = pIns->nNNA;

				if (!(pIns->VolEnv.dwFlags & ENV_CARRY)) pChn->VolEnv.Reset();
				if (!(pIns->PanEnv.dwFlags & ENV_CARRY)) pChn->PanEnv.Reset();
				if (!(pIns->PitchEnv.dwFlags & ENV_CARRY)) pChn->PitchEnv.Reset();

				if (GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))
				{
					// Volume Swing
					if (pIns->nVolSwing)
					{
						const double delta = 2 * (((double) rand()) / RAND_MAX) - 1;
						pChn->nVolSwing = (LONG)std::floor(delta * (IsCompatibleMode(TRK_IMPULSETRACKER) ? pChn->nInsVol : ((pChn->nVolume + 1) / 2)) * pIns->nVolSwing / 100.0);
					}
					// Pan Swing
					if (pIns->nPanSwing)
					{
						const double delta = 2 * (((double) rand()) / RAND_MAX) - 1;
						pChn->nPanSwing = (LONG)std::floor(delta * (IsCompatibleMode(TRK_IMPULSETRACKER) ? 4 : 1) * pIns->nPanSwing);
						if(!IsCompatibleMode(TRK_IMPULSETRACKER))
						{
							pChn->nRestorePanOnNewNote = pChn->nPan + 1;
						}
					}
					// Cutoff Swing
					if (pIns->nCutSwing)
					{
						int d = ((LONG)pIns->nCutSwing * (LONG)((rand() & 0xFF) - 0x7F)) / 128;
						pChn->nCutSwing = (signed short)((d * pChn->nCutOff + 1) / 128);
						pChn->nRestoreCutoffOnNewNote = pChn->nCutOff + 1;
					}
					// Resonance Swing
					if (pIns->nResSwing)
					{
						int d = ((LONG)pIns->nResSwing * (LONG)((rand() & 0xFF) - 0x7F)) / 128;
						pChn->nResSwing = (signed short)((d * pChn->nResonance + 1) / 128);
						pChn->nRestoreResonanceOnNewNote = pChn->nResonance + 1;
					}
				}
			}
			pChn->nAutoVibDepth = 0;
			pChn->nAutoVibPos = 0;
			// IT Compatibility: Vibrato reset
			if(IsCompatibleMode(TRK_IMPULSETRACKER))
			{
				// I think this is not necessary, so let's check if it is actually called.
				ASSERT(pChn->nVibratoPos == 0);
				pChn->nVibratoPos = 0;
			}
		}
		pChn->nLeftVol = pChn->nRightVol = 0;
		bool bFlt = (m_dwSongFlags & SONG_MPTFILTERMODE) == 0;
		// Setup Initial Filter for this note
		if (pIns)
		{
			if (pIns->IsResonanceEnabled())
			{
				pChn->nResonance = pIns->GetResonance();
				bFlt = true;
			}
			if (pIns->IsCutoffEnabled())
			{
				pChn->nCutOff = pIns->GetCutoff();
				bFlt = true;
			}
			if (bFlt && (pIns->nFilterMode != FLTMODE_UNCHANGED))
			{
				pChn->nFilterMode = pIns->nFilterMode;
			}
		} else
		{
			pChn->nVolSwing = pChn->nPanSwing = 0;
			pChn->nCutSwing = pChn->nResSwing = 0;
		}
		if ((pChn->nCutOff < 0x7F || IsCompatibleMode(TRK_IMPULSETRACKER)) && (bFlt))
		{
			SetupChannelFilter(pChn, true);
		}
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


CHANNELINDEX CSoundFile::GetNNAChannel(CHANNELINDEX nChn) const
//-------------------------------------------------------------
{
	const ModChannel *pChn = &Chn[nChn];
	// Check for empty channel
	const ModChannel *pi = &Chn[m_nChannels];
	for (CHANNELINDEX i=m_nChannels; i<MAX_CHANNELS; i++, pi++) if (!pi->nLength) return i;
	if (!pChn->nFadeOutVol) return 0;
	// All channels are used: check for lowest volume
	CHANNELINDEX result = 0;
	DWORD vol = 64*65536;	// 25%
	DWORD envpos = 0xFFFFFF;
	const ModChannel *pj = &Chn[m_nChannels];
	for (CHANNELINDEX j=m_nChannels; j<MAX_CHANNELS; j++, pj++)
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


void CSoundFile::CheckNNA(CHANNELINDEX nChn, UINT instr, int note, BOOL bForceCut)
//--------------------------------------------------------------------------------
{
	ModChannel *pChn = &Chn[nChn];
	ModInstrument *pIns = nullptr;
	LPSTR pSample;
	if(!ModCommand::IsNote(note))
	{
		return;
	}
	// Always NNA cut - using
	if ((!(m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_MT2))) || (!m_nInstruments) || (bForceCut))
	{
		if ((m_dwSongFlags & SONG_CPUVERYHIGH)
		 || (!pChn->nLength) || (pChn->dwFlags & CHN_MUTE)
		 || ((!pChn->nLeftVol) && (!pChn->nRightVol))) return;
		UINT n = GetNNAChannel(nChn);
		if (!n) return;
		ModChannel *p = &Chn[n];
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
	if(instr >= MAX_INSTRUMENTS) instr = 0;
	pSample = pChn->pSample;
	pIns = pChn->pModInstrument;
	if(instr && note)
	{
		pIns = Instruments[instr];
		if (pIns)
		{
			UINT n = 0;
			if (note <= NOTE_MAX)
			{
				n = pIns->Keyboard[note - 1];
				note = pIns->NoteMap[note - 1];
				if(n > 0  && n < MAX_SAMPLES) pSample = Samples[n].pSample;
			}
		} else pSample = nullptr;
	}
	ModChannel *p = pChn;
	//if (!pIns) return;
	if (pChn->dwFlags & CHN_MUTE) return;

	bool applyDNAtoPlug;	//rewbs.VSTiNNA

	for(CHANNELINDEX i = nChn; i < MAX_CHANNELS; p++, i++)
	if(i >= m_nChannels || p == pChn)
	{
		applyDNAtoPlug = false; //rewbs.VSTiNNA
		if((p->nMasterChn == nChn + 1 || p == pChn) && p->pModInstrument != nullptr)
		{
			bool bOk = false;
			// Duplicate Check Type
			switch(p->pModInstrument->nDCT)
			{
			// Note
			case DCT_NOTE:
				if(note && p->nNote == note && pIns == p->pModInstrument) bOk = true;
				if(pIns && pIns->nMixPlug) applyDNAtoPlug = true; //rewbs.VSTiNNA
				break;
			// Sample
			case DCT_SAMPLE:
				if(pSample != nullptr && pSample == p->pSample) bOk = true;
				break;
			// Instrument
			case DCT_INSTRUMENT:
				if(pIns == p->pModInstrument) bOk = true;
				//rewbs.VSTiNNA
				if(pIns && pIns->nMixPlug) applyDNAtoPlug = true;
				break;
			// Plugin
			case DCT_PLUGIN:
				if(pIns && (pIns->nMixPlug) && (pIns->nMixPlug == p->pModInstrument->nMixPlug))
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
						IMixPlugin *pPlugin =  m_MixPlugins[pIns->nMixPlug-1].pMixPlugin;
						if(pPlugin && p->nNote != NOTE_NONE)
							pPlugin->MidiCommand(GetBestMidiChannel(i), p->pModInstrument->nMidiProgram, p->pModInstrument->wMidiBank, p->nNote + NOTE_KEYOFF, 0, i);
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
				if(!p->nVolume)
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
	if (pChn->pModInstrument && pChn->pModInstrument->HasValidMIDIChannel() && ModCommand::IsNote(pChn->nNote)) // instro sends to a midi chan
	{
		PLUGINDEX nPlugin = GetBestPlugin(nChn, PrioritiseInstrument, RespectMutes);

		if(nPlugin > 0 && nPlugin <= MAX_MIXPLUGINS)
		{
			pPlugin =  m_MixPlugins[nPlugin-1].pMixPlugin;
			if(pPlugin)
			{
				// apply NNA to this Plug iff this plug is currently playing a note on this tracking chan
				// (and if it is playing a note, we know that would be the last note played on this chan).
				ModCommand::NOTE note = pChn->nNote;
				// Caution: When in compatible mode, ModChannel::nNote stores the "real" note, not the mapped note!
				if(IsCompatibleMode(TRK_IMPULSETRACKER) && note < CountOf(pChn->pModInstrument->NoteMap))
				{
					note = pChn->pModInstrument->NoteMap[note - 1];
				}
				applyNNAtoPlug = pPlugin->isPlaying(note, GetBestMidiChannel(nChn), nChn);
			}
		}
	}
	//end rewbs.VSTiNNA

	// New Note Action
	//if ((pChn->nVolume) && (pChn->nLength))
	if((pChn->nVolume != 0 && pChn->nLength != 0) || applyNNAtoPlug) //rewbs.VSTiNNA
	{
		CHANNELINDEX n = GetNNAChannel(nChn);
		if(n != 0)
		{
			ModChannel *p = &Chn[n];
			// Copy Channel
			*p = *pChn;
			p->dwFlags &= ~(CHN_VIBRATO|CHN_TREMOLO|CHN_PANBRELLO|CHN_MUTE|CHN_PORTAMENTO);

			//rewbs: Copy mute and FX status from master chan.
			//I'd like to copy other flags too, but this would change playback behaviour.
			p->dwFlags |= (pChn->dwFlags & (CHN_MUTE|CHN_NOFX));

			p->nMasterChn = nChn + 1;
			p->nCommand = 0;
			//rewbs.VSTiNNA
			if(applyNNAtoPlug && pPlugin)
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
					pPlugin->MidiCommand(GetBestMidiChannel(nChn), pChn->pModInstrument->nMidiProgram, pChn->pModInstrument->wMidiBank, /*pChn->nNote+*/NOTE_KEYOFF, 0, nChn);
					break;
				}
			}
			//end rewbs.VSTiNNA
			// Key Off the note
			switch(pChn->nNNA)
			{
			case NNA_NOTEOFF:
				KeyOff(n);
				break;
			case NNA_NOTECUT:
				p->nFadeOutVol = 0;
			case NNA_NOTEFADE:
				p->dwFlags |= CHN_NOTEFADE;
				break;
			}
			if(!p->nVolume)
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
	ModChannel *pChn = Chn;
	ROWINDEX nBreakRow = ROWINDEX_INVALID;		// Is changed if a break to row command is encountere.d
	ROWINDEX nPatLoopRow = ROWINDEX_INVALID;	// Is changed if a pattern loop jump-back is executed
	ORDERINDEX nPosJump = ORDERINDEX_INVALID;

// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
	ModCommand *m = nullptr;
// -! NEW_FEATURE#0010
	for (CHANNELINDEX nChn = 0; nChn < m_nChannels; nChn++, pChn++)
	{
		UINT instr = pChn->rowCommand.instr;
		UINT volcmd = pChn->rowCommand.volcmd;
		UINT vol = pChn->rowCommand.vol;
		UINT cmd = pChn->rowCommand.command;
		UINT param = pChn->rowCommand.param;
		bool bPorta = (cmd == CMD_TONEPORTAMENTO) || (cmd == CMD_TONEPORTAVOL) || (volcmd == VOLCMD_TONEPORTAMENTO);

		UINT nStartTick = 0;

		pChn->dwFlags &= ~CHN_FASTVOLRAMP;

		// Process parameter control note.
		if(pChn->rowCommand.note == NOTE_PC)
		{
			const PLUGINDEX plug = pChn->rowCommand.instr;
			const PlugParamIndex plugparam = ModCommand::GetValueVolCol(pChn->rowCommand.volcmd, pChn->rowCommand.vol);
			const PlugParamValue value = ModCommand::GetValueEffectCol(pChn->rowCommand.command, pChn->rowCommand.param) / PlugParamValue(ModCommand::maxColumnValue);

			if(plug > 0 && plug <= MAX_MIXPLUGINS && m_MixPlugins[plug-1].pMixPlugin)
				m_MixPlugins[plug-1].pMixPlugin->SetParameter(plugparam, value);
		}

		// Process continuous parameter control note.
		// Row data is cleared after first tick so on following
		// ticks using channels m_nPlugParamValueStep to identify
		// the need for parameter control. The condition cmd == 0
		// is to make sure that m_nPlugParamValueStep != 0 because
		// of NOTE_PCS, not because of macro.
		if(pChn->rowCommand.note == NOTE_PCS || (cmd == CMD_NONE && pChn->m_plugParamValueStep != 0))
		{
			const bool isFirstTick = (m_dwSongFlags & SONG_FIRSTTICK) != 0;
			if(isFirstTick)
				pChn->m_RowPlug = pChn->rowCommand.instr;
			const PLUGINDEX nPlug = pChn->m_RowPlug;
			const bool hasValidPlug = (nPlug > 0 && nPlug <= MAX_MIXPLUGINS && m_MixPlugins[nPlug-1].pMixPlugin);
			if(hasValidPlug)
			{
				if(isFirstTick)
					pChn->m_RowPlugParam = ModCommand::GetValueVolCol(pChn->rowCommand.volcmd, pChn->rowCommand.vol);
				const PlugParamIndex plugparam = pChn->m_RowPlugParam;
				if(isFirstTick)
				{
					PlugParamValue targetvalue = ModCommand::GetValueEffectCol(pChn->rowCommand.command, pChn->rowCommand.param) / PlugParamValue(ModCommand::maxColumnValue);
					pChn->m_plugParamTargetValue = targetvalue;
					pChn->m_plugParamValueStep = (targetvalue - m_MixPlugins[nPlug-1].pMixPlugin->GetParameter(plugparam)) / float(GetNumTicksOnCurrentRow());
				}
				if(m_nTickCount + 1 == GetNumTicksOnCurrentRow())
				{	// On last tick, set parameter exactly to target value.
					m_MixPlugins[nPlug-1].pMixPlugin->SetParameter(plugparam, pChn->m_plugParamTargetValue);
				}
				else
					m_MixPlugins[nPlug-1].pMixPlugin->ModifyParameter(plugparam, pChn->m_plugParamValueStep);
			}
		}

		// Apart from changing parameters, parameter control notes are intended to be 'invisible'.
		// To achieve this, clearing the note data so that rest of the process sees the row as empty row.
		if(ModCommand::IsPcNote(pChn->rowCommand.note))
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
			const UINT cutAtTick = nStartTick + (param & 0x0F);
			NoteCut(nChn, cutAtTick);
		} else if ((cmd == CMD_MODCMDEX) || (cmd == CMD_S3MCMDEX))
		{
			if ((!param) && (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))) param = pChn->nOldCmdEx; else pChn->nOldCmdEx = param;
			// Note Delay ?
			if ((param & 0xF0) == 0xD0)
			{
				nStartTick = param & 0x0F;
				if(nStartTick == 0)
				{
					//IT compatibility 22. SD0 == SD1
					if(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
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
						} else
						{
							if(instr < MAX_INSTRUMENTS)
								pChn->pModInstrument = Instruments[instr];
						}
					}
					continue;
				}
			} else if(m_dwSongFlags & SONG_FIRSTTICK)
			{
				// Pattern Loop ?
				if ((((param & 0xF0) == 0x60) && (cmd == CMD_MODCMDEX))
				 || (((param & 0xF0) == 0xB0) && (cmd == CMD_S3MCMDEX)))
				{

					ROWINDEX nloop = PatternLoop(pChn, param & 0x0F);
					if (nloop != ROWINDEX_INVALID)
					{
						// FT2 compatibility: E6x overwrites jump targets of Dxx effects that are located left of the E6x effect.
						// Test cases: PatLoop-Jumps.xm, PatLoop-Various.xm
						if(nBreakRow != ROWINDEX_INVALID && IsCompatibleMode(TRK_FASTTRACKER2))
						{
							nBreakRow = nloop;
						}

						nPatLoopRow = nloop;
					}

					if(GetType() == MOD_TYPE_S3M)
					{
						// ST3 doesn't have per-channel pattern loop memory, so spam all changes to other channels as well.
						for (CHANNELINDEX i = 0; i < GetNumChannels(); i++)
						{
							Chn[i].nPatternLoop = pChn->nPatternLoop;
							Chn[i].nPatternLoopCount = pChn->nPatternLoopCount;
						}
					}
				} else if ((param & 0xF0) == 0xE0)
				{
					// Pattern Delay
					// In Scream Tracker 3 / Impulse Tracker, only the first delay command on this row is considered.
					// Test cases: PatternDelays.it, PatternDelays.s3m, PatternDelays.xm
					// XXX In Scream Tracker 3, the "left" channels are evaluated before the "right" channels, which is not emulated here!
					if(!(GetType() & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)) || !m_nPatternDelay)
					{
						if(!(GetType() & (MOD_TYPE_S3M)) || (param & 0x0F) != 0)
						{
							// While Impulse Tracker *does* count S60 as a valid row delay (and thus ignores any other row delay commands on the right),
							// Scream Tracker 3 simply ignores such commands.
							m_nPatternDelay = 1 + (param & 0x0F);
						}
					}
				}
			}
		}

		bool triggerNote = (m_nTickCount == nStartTick);	// Can be delayed by a note delay effect
		// IT Compatibility: Delayed notes (using SDx) that are on the same row as a Row Delay effect are retriggered. Scream Tracker 3 does the same.
		// Test case: PatternDelay-NoteDelay.it
		if((GetType() & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)) && nStartTick > 0 && (m_nTickCount % (m_nMusicSpeed + m_nFrameDelay)) == nStartTick)
		{
			triggerNote = true;
		}

		// Handles note/instrument/volume changes
		if(triggerNote)
		{
			UINT note = pChn->rowCommand.note;
			if (instr) pChn->nNewIns = instr;

			if(ModCommand::IsNote(note) && IsCompatibleMode(TRK_FASTTRACKER2))
			{
				// Notes that exceed FT2's limit are completely ignored.
				// Test case: note-limit.xm
				int transpose = pChn->nTranspose;
				if(instr && !bPorta)
				{
					// Refresh transpose
					// Test case: note-limit 2.xm
					SAMPLEINDEX sample = SAMPLEINDEX_INVALID;
					if(GetNumInstruments())
					{
						// Instrument mode
						if(instr <= GetNumInstruments() && Instruments[instr] != nullptr)
						{
							sample = Instruments[instr]->Keyboard[note - NOTE_MIN];
						}
					} else
					{
						// Sample mode
						sample = instr;
					}
					if(sample <= GetNumSamples())
					{
						transpose = GetSample(sample).RelativeTone;
					}
				}

				const int computedNote = note + transpose;
				if((computedNote < NOTE_MIN + 11 || computedNote > NOTE_MIN + 130))
				{
					note = NOTE_NONE;
				}
			}

			// XM: FT2 ignores a note next to a K00 effect, and a fade-out seems to be done when no volume envelope is present (not exactly the Kxx behaviour)
			if(cmd == CMD_KEYOFF && param == 0 && IsCompatibleMode(TRK_FASTTRACKER2))
			{
				note = NOTE_NONE;
				instr = 0;
			}

			bool retrigEnv = note == NOTE_NONE && instr != 0;

			// Apparently, any note number in a pattern causes instruments to recall their original volume settings - no matter if there's a Note Off next to it or whatever.
			// Test cases: keyoff+instr.xm, delay.xm
			bool reloadSampleSettings = (IsCompatibleMode(TRK_FASTTRACKER2) && instr != 0);
			bool keepInstr = (GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)) != 0;

			// Now it's time for some FT2 crap...
			if (GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2))
			{

				// XM: Key-Off + Sample == Note Cut (BUT: Only if no instr number or volume effect is present!)
				if ((note == NOTE_KEYOFF) && ((!instr && volcmd == VOLCMD_NONE && cmd != CMD_VOLUME) || !IsCompatibleMode(TRK_FASTTRACKER2)) && ((!pChn->pModInstrument) || (!(pChn->pModInstrument->VolEnv.dwFlags & ENV_ENABLED))))
				{
					pChn->dwFlags |= CHN_FASTVOLRAMP;
					pChn->nVolume = 0;
					note = NOTE_NONE;
					instr = 0;
					retrigEnv = false;
				} else if(IsCompatibleMode(TRK_FASTTRACKER2) && !(m_dwSongFlags & SONG_FIRSTTICK))
				{
					// XM Compatibility: Some special hacks for rogue note delays... (EDx with x > 0)
					// Apparently anything that is next to a note delay behaves totally unpredictable in FT2. Swedish tracker logic. :)

					retrigEnv = true;

					// Portamento + Note Delay = No Portamento
					// Test case: porta-delay.xm
					bPorta = false;

					if(note == NOTE_NONE)
					{
						// If there's a note delay but no real note, retrig the last note.
						// Test case: delay2.xm, delay3.xm
						note = pChn->nNote - pChn->nTranspose;
					} else if(note >= NOTE_MIN_SPECIAL)
					{
						// Gah! Even Note Off + Note Delay will cause envelopes to *retrigger*! How stupid is that?
						// ... Well, and that is actually all it does if there's an envelope. No fade out, no nothing. *sigh*
						// Test case: OffDelay.xm
						note = NOTE_NONE;
						keepInstr = false;
						reloadSampleSettings = true;
					} else
					{
						// Normal note
						keepInstr = true;
						reloadSampleSettings = true;
					}
				}

			}

			if((retrigEnv && !IsCompatibleMode(TRK_FASTTRACKER2)) || reloadSampleSettings)
			{
				const ModSample *oldSample = nullptr;
				// Reset default volume when retriggering envelopes

				if(GetNumInstruments())
				{
					oldSample = pChn->pModSample;
				} else if (instr <= GetNumSamples())
				{
					// Case: Only samples are used; no instruments.
					oldSample = &Samples[instr];
				}

				if(oldSample != nullptr)
				{
					pChn->nVolume = oldSample->nVolume;
					if(reloadSampleSettings)
					{
						// Also reload panning
						pChn->nPan = oldSample->nPan;
					}
				}
			}

			if (retrigEnv) //Case: instrument with no note data.
			{
				//IT compatibility: Instrument with no note.
				if(IsCompatibleMode(TRK_IMPULSETRACKER) || (m_dwSongFlags & SONG_PT1XMODE))
				{
					if(GetNumInstruments())
					{
						// Instrument mode
						if(instr < MAX_INSTRUMENTS && pChn->pModInstrument != Instruments[instr])
							note = pChn->nNote;
					} else
					{
						// Sample mode
						if(instr < MAX_SAMPLES && pChn->pSample != Samples[instr].pSample)
							note = pChn->nNote;
					}
				}

				if (GetNumInstruments() && (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2)))
				{
					pChn->dwFlags |= CHN_FASTVOLRAMP;
					pChn->ResetEnvelopes();
					pChn->nAutoVibDepth = 0;
					pChn->nAutoVibPos = 0;
					pChn->dwFlags &= ~CHN_NOTEFADE;
					pChn->nFadeOutVol = 65536;
				}
				if (!keepInstr) instr = 0;
			}
			// Invalid Instrument ?
			if (instr >= MAX_INSTRUMENTS) instr = 0;

			// Note Cut/Off/Fade => ignore instrument
			if (note >= NOTE_MIN_SPECIAL) instr = 0;

			if (ModCommand::IsNote(note))
			{
				pChn->nNewNote = pChn->nLastNote = note;

				// New Note Action ?
				if (!bPorta)
				{
					CheckNNA(nChn, instr, note, FALSE);
				}
			}

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
				ModSample *psmp = pChn->pModSample;
				InstrumentChange(pChn, instr, bPorta, true);
				pChn->nNewIns = 0;
				// Special IT case: portamento+note causes sample change -> ignore portamento
				if ((GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
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
					InstrumentChange(pChn, pChn->nNewIns, bPorta, false, (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2)) == 0);
					pChn->nNewIns = 0;
				}
				NoteChange(nChn, note, bPorta, (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2)) == 0);
				if ((bPorta) && (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2)) && (instr))
				{
					pChn->dwFlags |= CHN_FASTVOLRAMP;
					pChn->ResetEnvelopes();
					pChn->nAutoVibDepth = 0;
					pChn->nAutoVibPos = 0;
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

#ifdef MODPLUG_TRACKER
			if (m_nInstruments) ProcessMidiOut(nChn);
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
				else if(vol != 0 || !IsCompatibleMode(TRK_FASTTRACKER2))
					TonePortamento(pChn, vol * 16);
			} else
			{
				// XM Compatibility: FT2 ignores some volume commands with parameter = 0.
				if(IsCompatibleMode(TRK_FASTTRACKER2) && vol == 0)
				{
					switch(volcmd)
					{
					case VOLCMD_VOLUME:
					case VOLCMD_PANNING:
					case VOLCMD_VIBRATODEPTH:
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
					if (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
					{
						if (m_nTickCount == nStartTick) VolumeSlide(pChn, (vol << 4) | 0x0F);
					} else
						FineVolumeUp(pChn, vol);
					break;

				case VOLCMD_FINEVOLDOWN:
					if (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
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
					// IT compatibility (one of the first testcases - link effect memory)
					PortamentoUp(nChn, vol << 2, IsCompatibleMode(TRK_IMPULSETRACKER));
					break;

				case VOLCMD_PORTADOWN:
					// IT compatibility (one of the first testcases - link effect memory)
					PortamentoDown(nChn, vol << 2, IsCompatibleMode(TRK_IMPULSETRACKER));
					break;

				case VOLCMD_OFFSET:					//rewbs.volOff
					if (m_nTickCount == nStartTick)
						SampleOffset(nChn, vol << 3);
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
			if ((!param) && (GetType() & MOD_TYPE_MOD)) break;
			PortamentoUp(nChn, param);
			break;

		// Portamento Down
		case CMD_PORTAMENTODOWN:
			if ((!param) && (GetType() & MOD_TYPE_MOD)) break;
			PortamentoDown(nChn, param);
			break;

		// Volume Slide
		case CMD_VOLUMESLIDE:
			if ((param) || (GetType() != MOD_TYPE_MOD)) VolumeSlide(pChn, param);
			break;

		// Tone-Portamento
		case CMD_TONEPORTAMENTO:
			TonePortamento(pChn, param);
			break;

		// Tone-Portamento + Volume Slide
		case CMD_TONEPORTAVOL:
			if ((param) || (GetType() != MOD_TYPE_MOD)) VolumeSlide(pChn, param);
			TonePortamento(pChn, 0);
			break;

		// Vibrato
		case CMD_VIBRATO:
			Vibrato(pChn, param);
			break;

		// Vibrato + Volume Slide
		case CMD_VIBRATOVOL:
			if ((param) || (GetType() != MOD_TYPE_MOD)) VolumeSlide(pChn, param);
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
				m = nullptr;
				if (m_nRow < Patterns[m_nPattern].GetNumRows()-1)
				{
					m = Patterns[m_nPattern].GetpModCommand(m_nRow + 1, nChn);
				}
				if (m && m->command == CMD_XPARAM)
				{
					if ((GetType() & MOD_TYPE_XM))
					{
						param -= 0x20; //with XM, 0x20 is the lowest tempo. Anything below changes ticks per row.
					}
					param = (param << 8) + m->param;
				}
// -! NEW_FEATURE#0010
				if (GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
				{
					if (param) pChn->nOldTempo = param; else param = pChn->nOldTempo;
				}
				if (param > GetModSpecifications().tempoMax) param = GetModSpecifications().tempoMax; // rewbs.merge: added check to avoid hyperspaced tempo!
				SetTempo(param);
			break;

		// Set Offset
		case CMD_OFFSET:
			if (m_nTickCount) break;
			// XM compatibility: Portamento + Offset = Ignore offset
			if(bPorta && GetType() == MOD_TYPE_XM)
			{
				break;
			}
			SampleOffset(nChn, param);
			break;

		// Arpeggio
		case CMD_ARPEGGIO:
			// IT compatibility 01. Don't ignore Arpeggio if no note is playing (also valid for ST3)
			if ((m_nTickCount) || (((!pChn->nPeriod) || !pChn->nNote) && !IsCompatibleMode(TRK_IMPULSETRACKER | TRK_SCREAMTRACKER))) break;
			if ((!param) && (!(GetType() & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)))) break;
			pChn->nCommand = CMD_ARPEGGIO;
			if (param) pChn->nArpeggio = param;
			break;

		// Retrig
		case CMD_RETRIG:
			if (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))
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
			// ST3 applies global volume on tick 1 and does other weird things, but we won't emulate this for now.
// 			if(((GetType() & MOD_TYPE_S3M) && m_nTickCount != 1)
// 				|| (!(GetType() & MOD_TYPE_S3M) && !(m_dwSongFlags & SONG_FIRSTTICK)))
// 			{
// 				break;
// 			}

			// FT2 compatibility: Global volume is applied *after* the first tick.
			// This is not emulated quite correctly for speed 1 (because there is no second tick), but it should be close enough.
			// Test case: GlobalVolume.xm
			if(IsCompatibleMode(TRK_FASTTRACKER2) && (m_dwSongFlags & SONG_FIRSTTICK) && m_nMusicSpeed > 1)
			{
				break;
			}

			if (!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) param *= 2;

			// IT compatibility 16. FT2, ST3 and IT ignore out-of-range values.
			// Test case: globalvol-invalid.it
			if (param <= 128)
			{
				m_nGlobalVolume = param * 2;
			}
			break;

		// Global Volume Slide
		case CMD_GLOBALVOLSLIDE:
			//IT compatibility 16. Saving last global volume slide param per channel (FT2/IT)
			if(IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2))
				GlobalVolSlide(param, pChn->nOldGlobalVolSlide);
			else
				GlobalVolSlide(param, m_nOldGlbVolSlide);
			break;

		// Set 8-bit Panning
		case CMD_PANNING8:
			if (!(m_dwSongFlags & SONG_FIRSTTICK)) break;
			if ((m_dwSongFlags & SONG_PT1XMODE)) break;
			if (!(m_dwSongFlags & SONG_SURROUNDPAN)) pChn->dwFlags &= ~CHN_SURROUND;
			if (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM | MOD_TYPE_MOD | MOD_TYPE_MT2))
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
			if(GetType() == MOD_TYPE_S3M && param == 0)
			{
				param = pChn->nArpeggio;	// S00 uses the last non-zero effect parameter as memory, like other effects including Arpeggio, so we "borrow" our memory there.
			}
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
						if(param == 0 && (pChn->rowCommand.instr || pChn->rowCommand.volcmd != VOLCMD_NONE)) // FT2 is weird....
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
						ModInstrument *pIns = pChn->pModInstrument;
						if ((pChn->PanEnv.flags & ENV_ENABLED) && (pIns->PanEnv.nNodes) && (param > pIns->PanEnv.Ticks[pIns->PanEnv.nNodes - 1]))
						{
							pChn->PanEnv.flags &= ~ENV_ENABLED;
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
			if(GetType() == MOD_TYPE_XM && nBreakRow != ROWINDEX_INVALID)
			{
				nBreakRow = 0;
			}
			break;

		// Pattern Break
		case CMD_PATTERNBREAK:
			if(param >= 64 && (GetType() & MOD_TYPE_S3M))
			{
				// ST3 ignores invalid pattern breaks.
				break;
			}

			m_nNextPatStartRow = 0; // FT2 E60 bug

			m = NULL;
			if (m_nRow < Patterns[m_nPattern].GetNumRows() - 1)
			{
			  m = Patterns[m_nPattern].GetpModCommand(m_nRow + 1, nChn);
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
				nPosJump = (int)m_nCurrentOrder;
			}
			break;

		// IMF Commands
		case CMD_NOTESLIDEUP:
			NoteSlide(pChn, param, true);
			break;

		case CMD_NOTESLIDEDOWN:
			NoteSlide(pChn, param, false);
			break;
		}

		if(GetType() == MOD_TYPE_S3M && param != 0)
		{
			UpdateS3MEffectMemory(pChn, param);
		}

	} // for(...) end

	// Navigation Effects
	if(m_dwSongFlags & SONG_FIRSTTICK)
	{
		const bool doPatternLoop = (nPatLoopRow != ROWINDEX_INVALID);

		// Pattern Loop
		if(doPatternLoop)
		{
			m_nNextOrder = m_nCurrentOrder;
			m_nNextRow = nPatLoopRow;
			if(m_nPatternDelay)
			{
				m_nNextRow++;
			}

			// As long as the pattern loop is running, mark the looped rows as not visited yet
			for(ROWINDEX nRow = nPatLoopRow; nRow <= m_nRow; nRow++)
			{
				visitedSongRows.Unvisit(m_nCurrentOrder, nRow);
			}
		}

		// Pattern Break / Position Jump only if no loop running
		// Test case for FT2 exception: PatLoop-Jumps.xm, PatLoop-Various.xm
		if((nBreakRow != ROWINDEX_INVALID || nPosJump != ORDERINDEX_INVALID)
			&& (!doPatternLoop || IsCompatibleMode(TRK_FASTTRACKER2)))
		{
			if(nPosJump == ORDERINDEX_INVALID) nPosJump = m_nCurrentOrder + 1;
			if(nBreakRow == ROWINDEX_INVALID) nBreakRow = 0;
			m_dwSongFlags |= SONG_BREAKTOROW;

			if(nPosJump >= Order.size())
			{
				nPosJump = 0;
			}

			// IT / FT2 compatibility: don't reset loop count on pattern break.
			// Test case: gm-trippy01.it, PatLoop-Break.xm, PatLoop-Weird.xm
			if(nPosJump != m_nCurrentOrder && !IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2))
			{
				for(CHANNELINDEX i = 0; i < GetNumChannels(); i++)
				{
					Chn[i].nPatternLoopCount = 0;
				}
			}
			m_nNextOrder = nPosJump;
			m_nNextRow = nBreakRow;
			m_bPatternTransitionOccurred = true;

		}

	}
	return TRUE;
}


////////////////////////////////////////////////////////////
// Channels effects


// Update the effect memory of all S3M effects that use the last non-zero effect parameter ot show up (Dxy, Exx, Fxx, Ixy, Jxy, Kxy, Lxy, Qxy, Rxy, Sxy)
void CSoundFile::UpdateS3MEffectMemory(ModChannel *pChn, UINT param) const
//------------------------------------------------------------------------
{
	pChn->nOldVolumeSlide = param;	// Dxy / Kxy / Lxy
	pChn->nOldPortaUpDown = param;	// Exx / Fxx
	pChn->nTremorParam = param;		// Ixy
	pChn->nArpeggio = param;		// Jxy
	pChn->nRetrigParam = param;		// Qxy
	pChn->nTremoloDepth = (param & 0x0F) << 2;	// Rxy
	pChn->nTremoloSpeed = (param >> 4) & 0x0F;	// Rxy
	// Sxy is not handled here.
}


void CSoundFile::PortamentoUp(CHANNELINDEX nChn, UINT param, const bool doFinePortamentoAsRegular)
//------------------------------------------------------------------------------------------------
{
	ModChannel *pChn = &Chn[nChn];
	MidiPortamento(nChn, param); //Send midi pitch bend event if there's a plugin

	if(param)
		pChn->nOldPortaUpDown = param;
	else
		param = pChn->nOldPortaUpDown;

	if(GetType() == MOD_TYPE_MPT && pChn->pModInstrument && pChn->pModInstrument->pTuning)
	{
		if(param >= 0xF0 && !doFinePortamentoAsRegular)
			PortamentoFineMPT(pChn, param - 0xF0);
		else
			PortamentoMPT(pChn, param);
		return;
	}

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
		DoFreqSlide(pChn, -int(param) * 4);
	}
}


void CSoundFile::PortamentoDown(CHANNELINDEX nChn, UINT param, const bool doFinePortamentoAsRegular)
//--------------------------------------------------------------------------------------------------
{
	ModChannel *pChn = &Chn[nChn];
	MidiPortamento(nChn, -(int)param); //Send midi pitch bend event if there's a plugin

	if(param)
		pChn->nOldPortaUpDown = param;
	else
		param = pChn->nOldPortaUpDown;

	if(m_nType == MOD_TYPE_MPT && pChn->pModInstrument && pChn->pModInstrument->pTuning)
	{
		if(param >= 0xF0 && !doFinePortamentoAsRegular)
			PortamentoFineMPT(pChn, -1*(param - 0xF0));
		else
			PortamentoMPT(pChn, -1*param);
		return;
	}

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
	if (!(m_dwSongFlags & SONG_FIRSTTICK)  || (m_nMusicSpeed == 1))	//rewbs.PortaA01fix
	{
		DoFreqSlide(pChn, int(param) * 4);
	}
}

void CSoundFile::MidiPortamento(CHANNELINDEX nChn, int param)
//-----------------------------------------------------------
{
	if((Chn[nChn].dwFlags & (CHN_MUTE | CHN_SYNCMUTE)) != 0)
	{
		// Don't process portamento on muted channels. Note that this might have a side-effect
		// on other channels which trigger notes on the same MIDI channel of the same plugin,
		// as those won't be pitch-bent anymore.
		return;
	}

	//Send midi pitch bend event if there's a plugin:
	const ModInstrument *pIns = Chn[nChn].pModInstrument;
	if (pIns && pIns->HasValidMIDIChannel())
	{
		// instro sends to a midi chan
		UINT nPlug = pIns->nMixPlug;
		if ((nPlug) && (nPlug <= MAX_MIXPLUGINS))
		{
			IMixPlugin *pPlug = (IMixPlugin*)m_MixPlugins[nPlug-1].pMixPlugin;
			if (pPlug)
			{
				pPlug->MidiPitchBend(GetBestMidiChannel(nChn), param, 0);
			}
		}
	}
}

void CSoundFile::FinePortamentoUp(ModChannel *pChn, UINT param)
//-------------------------------------------------------------
{
	if (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))
	{
		if (param) pChn->nOldFinePortaUpDown = param; else param = pChn->nOldFinePortaUpDown;
	}
	if (m_dwSongFlags & SONG_FIRSTTICK)
	{
		if ((pChn->nPeriod) && (param))
		{
			if ((m_dwSongFlags & SONG_LINEARSLIDES) && (!(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2))))
			{
				int oldPeriod = pChn->nPeriod;
				pChn->nPeriod = _muldivr(pChn->nPeriod, LinearSlideDownTable[param & 0x0F], 65536);
				if(oldPeriod == pChn->nPeriod)
				{
					pChn->nPeriod--;
				}
			} else
			{
				pChn->nPeriod -= (int)(param * 4);
			}
			if (pChn->nPeriod < 1) pChn->nPeriod = 1;
		}
	}
}


void CSoundFile::FinePortamentoDown(ModChannel *pChn, UINT param)
//---------------------------------------------------------------
{
	if (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))
	{
		if (param) pChn->nOldFinePortaUpDown = param; else param = pChn->nOldFinePortaUpDown;
	}
	if (m_dwSongFlags & SONG_FIRSTTICK)
	{
		if ((pChn->nPeriod) && (param))
		{
			if ((m_dwSongFlags & SONG_LINEARSLIDES) && (!(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2))))
			{
				int oldPeriod = pChn->nPeriod;
				pChn->nPeriod = _muldivr(pChn->nPeriod, LinearSlideUpTable[param & 0x0F], 65536);
				if(oldPeriod == pChn->nPeriod)
				{
					pChn->nPeriod++;
				}
			} else
			{
				pChn->nPeriod += (int)(param * 4);
			}
			if (pChn->nPeriod > 0xFFFF) pChn->nPeriod = 0xFFFF;
		}
	}
}


void CSoundFile::ExtraFinePortamentoUp(ModChannel *pChn, UINT param)
//------------------------------------------------------------------
{
	if (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))
	{
		if (param) pChn->nOldFinePortaUpDown = param; else param = pChn->nOldFinePortaUpDown;
	}
	if (m_dwSongFlags & SONG_FIRSTTICK)
	{
		if ((pChn->nPeriod) && (param))
		{
			if ((m_dwSongFlags & SONG_LINEARSLIDES) && (!(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2))))
			{
				int oldPeriod = pChn->nPeriod;
				pChn->nPeriod = _muldivr(pChn->nPeriod, FineLinearSlideDownTable[param & 0x0F], 65536);
				if(oldPeriod == pChn->nPeriod)
				{
					pChn->nPeriod--;
				}
			} else
			{
				pChn->nPeriod -= (int)(param);
			}
			if (pChn->nPeriod < 1) pChn->nPeriod = 1;
		}
	}
}


void CSoundFile::ExtraFinePortamentoDown(ModChannel *pChn, UINT param)
//--------------------------------------------------------------------
{
	if (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))
	{
		if (param) pChn->nOldFinePortaUpDown = param; else param = pChn->nOldFinePortaUpDown;
	}
	if (m_dwSongFlags & SONG_FIRSTTICK)
	{
		if ((pChn->nPeriod) && (param))
		{
			if ((m_dwSongFlags & SONG_LINEARSLIDES) && (!(GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2))))
			{
				int oldPeriod = pChn->nPeriod;
				pChn->nPeriod = _muldivr(pChn->nPeriod, FineLinearSlideUpTable[param & 0x0F], 65536);
				if(oldPeriod == pChn->nPeriod)
				{
					pChn->nPeriod++;
				}
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
void CSoundFile::NoteSlide(ModChannel *pChn, UINT param, bool slideUp)
//--------------------------------------------------------------------
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
				((slideUp ? 1 : -1)  * pChn->nNoteSlideStep + GetNoteFromPeriod(pChn->nPeriod), 8363, 0);
		}
	}
}

// Portamento Slide
void CSoundFile::TonePortamento(ModChannel *pChn, UINT param)
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


void CSoundFile::Vibrato(ModChannel *p, UINT param)
//-------------------------------------------------
{
	p->m_VibratoDepth = param % 16 / 15.0F;
	//'New tuning'-thing: 0 - 1 <-> No depth - Full depth.


	if (param & 0x0F) p->nVibratoDepth = (param & 0x0F) * 4;
	if (param & 0xF0) p->nVibratoSpeed = (param >> 4) & 0x0F;
	p->dwFlags |= CHN_VIBRATO;
}


void CSoundFile::FineVibrato(ModChannel *p, UINT param)
//-----------------------------------------------------
{
	if (param & 0x0F) p->nVibratoDepth = param & 0x0F;
	if (param & 0xF0) p->nVibratoSpeed = (param >> 4) & 0x0F;
	p->dwFlags |= CHN_VIBRATO;
}


void CSoundFile::Panbrello(ModChannel *p, UINT param)
//---------------------------------------------------
{
	if (param & 0x0F) p->nPanbrelloDepth = param & 0x0F;
	if (param & 0xF0) p->nPanbrelloSpeed = (param >> 4) & 0x0F;
	p->dwFlags |= CHN_PANBRELLO;
}


void CSoundFile::VolumeSlide(ModChannel *pChn, UINT param)
//--------------------------------------------------------
{
	if (param)
		pChn->nOldVolumeSlide = param;
	else
		param = pChn->nOldVolumeSlide;

	if((GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM | MOD_TYPE_MT2)))
	{
		// MOD / XM nibble priority
		if((param & 0xF0) != 0)
		{
			param &= 0xF0;
		} else
		{
			param &= 0x0F;
		}
	}

	LONG newvolume = pChn->nVolume;
	if (GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_STM|MOD_TYPE_AMF))
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
			if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || (param & 0xF0) == 0)
				newvolume -= (int)((param & 0x0F) * 4);
		}
		else
		{
			newvolume += (int)((param & 0xF0) >> 2);
		}
		if (m_nType == MOD_TYPE_MOD) pChn->dwFlags |= CHN_FASTVOLRAMP;
	}
	newvolume = CLAMP(newvolume, 0, 256);

	pChn->nVolume = newvolume;
}


void CSoundFile::PanningSlide(ModChannel *pChn, UINT param)
//---------------------------------------------------------
{
	LONG nPanSlide = 0;
	if (param)
		pChn->nOldPanSlide = param;
	else
		param = pChn->nOldPanSlide;

	if((GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2)))
	{
		// XM nibble priority
		if((param & 0xF0) != 0)
		{
			param &= 0xF0;
		} else
		{
			param &= 0x0F;
		}
	}

	if (GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_STM))
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
				if (param & 0x0F)
				{
					// IT compatibility: Ignore slide commands with both nibbles set.
					if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || (param & 0xF0) == 0)
						nPanSlide = (int)((param & 0x0F) << 2);
				} else
				{
					nPanSlide = -(int)((param & 0xF0) >> 2);
				}
			}
		}
	} else
	{
		if (!(m_dwSongFlags & SONG_FIRSTTICK))
		{
			if (param & 0xF0)
			{
				nPanSlide = (int)((param & 0xF0) >> 2);
			} else
			{
				nPanSlide = -(int)((param & 0x0F) << 2);
			}
			// XM compatibility: FT2's panning slide is like IT's fine panning slide (not as deep)
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


void CSoundFile::FineVolumeUp(ModChannel *pChn, UINT param)
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


void CSoundFile::FineVolumeDown(ModChannel *pChn, UINT param)
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


void CSoundFile::Tremolo(ModChannel *pChn, UINT param)
//----------------------------------------------------
{
	if (param & 0x0F) pChn->nTremoloDepth = (param & 0x0F) << 2;
	if (param & 0xF0) pChn->nTremoloSpeed = (param >> 4) & 0x0F;
	pChn->dwFlags |= CHN_TREMOLO;
}


void CSoundFile::ChannelVolSlide(ModChannel *pChn, UINT param)
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
			if (param & 0x0F)
			{
				if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || (param & 0xF0) == 0)
					nChnSlide = -(int)(param & 0x0F);
			} else
			{
				nChnSlide = (int)((param & 0xF0) >> 4);
			}
		}
	}
	if (nChnSlide)
	{
		nChnSlide += pChn->nGlobalVol;
		nChnSlide = CLAMP(nChnSlide, 0, 64);
		pChn->nGlobalVol = nChnSlide;
	}
}


void CSoundFile::ExtendedMODCommands(CHANNELINDEX nChn, UINT param)
//-----------------------------------------------------------------
{
	ModChannel *pChn = &Chn[nChn];
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
					//pChn->nPan = (param << 4) + 8;
					pChn->nPan = (param * 256 + 8) / 15;
					pChn->dwFlags |= CHN_FASTVOLRAMP;
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
		} else // XM: Set Active Midi Macro
		{
			pChn->nActiveMacro = param;
		}
		break;
	}
}


void CSoundFile::ExtendedS3MCommands(CHANNELINDEX nChn, UINT param)
//-----------------------------------------------------------------
{
	ModChannel *pChn = &Chn[nChn];
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
	case 0x60:
				if((m_dwSongFlags & SONG_FIRSTTICK) && m_nTickCount == 0)
				{
					// Tick delays are added up.
					// Scream Tracker 3 does actually not support this command.
					// We'll use the same behaviour as for Impulse Tracker, as we can assume that
					// most S3Ms that make use of this command were made with Impulse Tracker.
					// MPT added this command to the XM format through the X6x effect, so we will use
					// the same behaviour here as well.
					// Test cases: PatternDelays.it, PatternDelays.s3m, PatternDelays.xm
					m_nFrameDelay += param;
				}
				break;
	// S7x: Envelope Control / Instrument Control
	case 0x70:	if(!(m_dwSongFlags & SONG_FIRSTTICK)) break;
				switch(param)
				{
				case 0:
				case 1:
				case 2:
					{
						ModChannel *bkp = &Chn[m_nChannels];
						for (CHANNELINDEX i=m_nChannels; i<MAX_CHANNELS; i++, bkp++)
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
				case 7:		pChn->VolEnv.flags &= ~ENV_ENABLED; break;
				case 8:		pChn->VolEnv.flags |= ENV_ENABLED; break;
				case 9:		pChn->PanEnv.flags &= ~ENV_ENABLED; break;
				case 10:	pChn->PanEnv.flags |= ENV_ENABLED; break;
				case 11:	pChn->PitchEnv.flags &= ~ENV_ENABLED; break;
				case 12:	pChn->PitchEnv.flags |= ENV_ENABLED; break;
				case 13:
				case 14:
					if(GetType() == MOD_TYPE_MPT)
					{
						pChn->PitchEnv.flags |= ENV_ENABLED;
						if(param == 13)	// pitch env on, filter env off
						{
							pChn->PitchEnv.flags &= ~ENV_FILTER;
						} else	// filter env on
						{
							pChn->PitchEnv.flags |= ENV_FILTER;
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
					//pChn->nPan = (param << 4) + 8;
					pChn->nPan = (param * 256 + 8) / 15;
					pChn->dwFlags |= CHN_FASTVOLRAMP;

					//IT compatibility 20. Set pan overrides random pan
					if(IsCompatibleMode(TRK_IMPULSETRACKER))
					{
						pChn->nPanSwing = 0;
					}
				}
				break;
	// S9x: Sound Control
	case 0x90:	ExtendedChannelEffect(pChn, param); break;
	// SAx: Set 64k Offset
	case 0xA0:	if(m_dwSongFlags & SONG_FIRSTTICK)
				{
					pChn->nOldHiOffset = param;
					if (!IsCompatibleMode(TRK_IMPULSETRACKER) && pChn->rowCommand.IsNote())
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
	case 0xF0:
		if(GetType() != MOD_TYPE_S3M)
		{
			pChn->nActiveMacro = param;
		}
		break;
	}
}


void CSoundFile::ExtendedChannelEffect(ModChannel *pChn, UINT param)
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


void CSoundFile::InvertLoop(ModChannel *pChn)
//-------------------------------------------
{
	// EFx implementation for MOD files (PT 1.1A and up: Invert Loop)
	// This effect trashes samples. Thanks to 8bitbubsy for making this work. :)
	if(GetType() != MOD_TYPE_MOD || pChn->nEFxSpeed == 0) return;

	// we obviously also need a sample for this
	ModSample *pModSample = pChn->pModSample;
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


// Process a MIDI Macro.
// Parameters:
// [in] nChn: Mod channel to apply macro on
// [in] isSmooth: If true, internal macros are interpolated between two rows
// [in] macro: Actual MIDI Macro string
// [in] param: Parameter for parametric macros (Z00 - Z7F)
// [in] plugin: Plugin to send MIDI message to (if not specified but needed, it is autodetected)
void CSoundFile::ProcessMIDIMacro(CHANNELINDEX nChn, bool isSmooth, char *macro, uint8 param, PLUGINDEX plugin)
//-------------------------------------------------------------------------------------------------------------
{
	const ModChannel *pChn = &Chn[nChn];
	const ModInstrument *pIns = GetNumInstruments() ? pChn->pModInstrument : nullptr;

	unsigned char out[MACRO_LENGTH];
	size_t outPos = 0;	// output buffer position, which also equals the number of complete bytes
	bool firstNibble = true;

	for(size_t pos = 0; pos < (MACRO_LENGTH - 1) && macro[pos]; pos++)
	{
		bool isNibble = false;		// did we parse a nibble or a byte value?
		unsigned char data = 0;		// data that has just been parsed

		// Parse next macro byte... See Impulse Tracker's MIDI.TXT for detailed information on each possible character.
		if(macro[pos] >= '0' && macro[pos] <= '9')
		{
			isNibble = true;
			data = (unsigned char)macro[pos] - '0';
		}
		else if(macro[pos] >= 'A' && macro[pos] <= 'F')
		{
			isNibble = true;
			data = (unsigned char)macro[pos] - 'A' + 0x0A;
		} else if(macro[pos] == 'c')		// c: MIDI channel
		{
			isNibble = true;
			data = (unsigned char)GetBestMidiChannel(nChn);
		} else if(macro[pos] == 'n')		// n: note value (last triggered note)
		{
			if(ModCommand::IsNote(pChn->nLastNote))
			{
				data = (unsigned char)(pChn->nLastNote - NOTE_MIN);
			}
		} else if(macro[pos] == 'v')		// v: velocity
		{
			data = (unsigned char)min(pChn->nVolume / 2, 127);

		} else if(macro[pos] == 'u')		// u: volume (calculated)
		{
			data = (unsigned char)min(pChn->nCalcVolume >> 7, 127);
		} else if(macro[pos] == 'x')		// x: pan set
		{
			data = (unsigned char)min(pChn->nPan / 2, 127);
		} else if(macro[pos] == 'y')		// y: calculated pan
		{
			data = (unsigned char)min(pChn->nRealPan / 2, 127);
		} else if(macro[pos] == 'a')		// a: high byte of bank select
		{
			if(pIns && pIns->wMidiBank)
			{
				data = (unsigned char)(((pIns->wMidiBank - 1) >> 7) & 0x7F);
			}
		} else if(macro[pos] == 'b')		// b: low byte of bank select
		{
			if(pIns && pIns->wMidiBank)
			{
				data = (unsigned char)((pIns->wMidiBank - 1) & 0x7F);
			}
		} else if(macro[pos] == 'p')		// p: program select
		{
			if(pIns && pIns->nMidiProgram)
			{
				data = (unsigned char)((pIns->nMidiProgram - 1) & 0x7F);
			}
		} else if(macro[pos] == 'z')		// z: macro data
		{
			data = (unsigned char)(param & 0x7F);
		} else								// unrecognized byte (e.g. space char)
		{
			continue;
		}

		// Append parsed data
		if(isNibble)	// parsed a nibble (constant or 'c' variable)
		{
			if(firstNibble)
			{
				out[outPos] = data;
			} else
			{
				out[outPos] = (out[outPos] << 4) | data;
				outPos++;
			}
			firstNibble = !firstNibble;
		} else			// parsed a byte (variable)
		{
			if(!firstNibble)	// From MIDI.TXT: '9n' is exactly the same as '09 n' or '9 n' -- so finish current byte first
			{
				outPos++;
			}
			out[outPos++] = data;
			firstNibble = true;
		}
	}
	if(!firstNibble)
	{
		// Finish current byte
		outPos++;
	}

	if(outPos == 0)
	{
		// Nothing there to send!
		return;
	}

	// Macro string has been parsed and translated, now send the message(s)...
	size_t sendPos = 0;
	while(sendPos < outPos)
	{
		size_t sendLen = 0;
		if(out[sendPos] == 0xF0)
		{
			// SysEx start
			if((sendPos <= outPos - 4) && (out[sendPos + 1] == 0xF0 || out[sendPos + 1] == 0xF1))
			{
				// Internal macro (normal (F0F0) or extended (F0F1)), 4 bytes long
				sendLen = 4;
			} else
			{
				// SysEx message, find end of message
				for(size_t i = sendPos + 1; i < outPos; i++)
				{
					if(out[i] == 0xF7)
					{
						// Found end of SysEx message
						sendLen = i - sendPos + 1;
						break;
					}
				}
				if(sendLen == 0)
				{
					// Didn't find end, so "invent" end of SysEx message
					out[outPos++] = 0xF7;
					sendLen = outPos - sendPos;
				}
			}
		} else
		{
			// Other MIDI messages, find beginning of next message
			while(sendPos + (++sendLen) < outPos)
			{
				if((out[sendPos + sendLen] & 0x80) != 0)
				{
					// Next message begins here.
					break;
				}
			}
		}

		if(sendLen == 0)
		{
			break;
		}

		size_t bytesSent = SendMIDIData(nChn, isSmooth, out + sendPos, sendLen, plugin);
		// Ideally (if there's no error in the macro data), we should have sendLen == bytesSent.
		if(bytesSent > 0)
		{
			sendPos += bytesSent;
		} else
		{
			sendPos += sendLen;
		}

	}

}


// Calculate smooth MIDI macro slide parameter for current tick.
float CSoundFile::CalculateSmoothParamChange(float currentValue, float param) const
//---------------------------------------------------------------------------------
{
	ASSERT(GetNumTicksOnCurrentRow() > m_nTickCount);
	const UINT ticksLeft = GetNumTicksOnCurrentRow() - m_nTickCount;
	if(ticksLeft > 1)
	{
		// Slide param
		const float step = (param - currentValue) / (float)ticksLeft;
		return (currentValue + step);
	} else
	{
		// On last tick, set exact value.
		return param;
	}
}


// Process MIDI macro data parsed by ProcessMIDIMacro... return bytes sent on success, 0 on (parse) failure.
size_t CSoundFile::SendMIDIData(CHANNELINDEX nChn, bool isSmooth, const unsigned char *macro, size_t macroLen, PLUGINDEX plugin)
//------------------------------------------------------------------------------------------------------------------------------
{
	if(macroLen < 1)
	{
		return 0;
	}

	ModChannel *pChn = &Chn[nChn];

	if(macro[0] == 0xF0 && (macro[1] == 0xF0 || macro[1] == 0xF1))
	{
		// Internal device.
		if(macroLen < 4)
		{
			return 0;
		}
		const bool isExtended = (macro[1] == 0xF1);
		const uint8 macroCode = macro[2];
		const uint8 param = macro[3];

		if(macroCode == 0x00 && !isExtended)
		{
			// F0.F0.00.xx: Set CutOff
			int oldcutoff = pChn->nCutOff;
			if(param < 0x80)
			{
				if(!isSmooth)
				{
					pChn->nCutOff = param;
				} else
				{
					pChn->nCutOff = (BYTE)CalculateSmoothParamChange((float)pChn->nCutOff, (float)param);
				}
				pChn->nRestoreCutoffOnNewNote = 0;
			}

			oldcutoff -= pChn->nCutOff;
			if(oldcutoff < 0) oldcutoff = -oldcutoff;
			if((pChn->nVolume > 0) || (oldcutoff < 0x10)
				|| (!(pChn->dwFlags & CHN_FILTER)) || (!(pChn->nLeftVol | pChn->nRightVol)))
				SetupChannelFilter(pChn, !(pChn->dwFlags & CHN_FILTER));

			return 4;

		} else if(macroCode == 0x01 && !isExtended)
		{
			// F0.F0.01.xx: Set Resonance
			if(param < 0x80)
			{
				pChn->nRestoreResonanceOnNewNote = 0;
				if(!isSmooth)
				{
					pChn->nResonance = param;
				} else
				{
					pChn->nResonance = (BYTE)CalculateSmoothParamChange((float)pChn->nResonance, (float)param);
				}
			}

			SetupChannelFilter(pChn, !(pChn->dwFlags & CHN_FILTER));

			return 4;

		} else if(macroCode == 0x02 && !isExtended)
		{
			// F0.F0.02.xx: Set filter mode (high nibble determines filter mode)
			if(param < 0x20)
			{
				pChn->nFilterMode = (param >> 4);
				SetupChannelFilter(pChn, !(pChn->dwFlags & CHN_FILTER));
			}

			return 4;

		} else if(macroCode == 0x03 && !isExtended)
		{
			// F0.F0.03.xx: Set plug dry/wet
			const PLUGINDEX nPlug = (plugin != 0) ? plugin : GetBestPlugin(nChn, PrioritiseChannel, EvenIfMuted);
			if ((nPlug) && (nPlug <= MAX_MIXPLUGINS) && param < 0x80)
			{
				const float newRatio = 1.0 - (static_cast<float>(param & 0x7F) / 127.0f);
				if(!isSmooth)
				{
					m_MixPlugins[nPlug - 1].fDryRatio = newRatio;
				} else
				{
					m_MixPlugins[nPlug - 1].fDryRatio = CalculateSmoothParamChange(m_MixPlugins[nPlug - 1].fDryRatio, newRatio);
				}
			}

			return 4;

		} else if((macroCode & 0x80) || isExtended)
		{
			// F0.F0.{80|n}.xx / F0.F1.n.xx: Set VST effect parameter n to xx
			const PLUGINDEX nPlug = (plugin != 0) ? plugin : GetBestPlugin(nChn, PrioritiseChannel, EvenIfMuted);
			const UINT plugParam = isExtended ? (0x80 + macroCode) : (macroCode & 0x7F);
			if((nPlug) && (nPlug <= MAX_MIXPLUGINS))
			{
				IMixPlugin *pPlugin = m_MixPlugins[nPlug - 1].pMixPlugin;
				if((pPlugin) && (m_MixPlugins[nPlug - 1].pMixState) && (param < 0x80))
				{
					if(!isSmooth)
					{
						pPlugin->SetZxxParameter(plugParam, param & 0x7F);
					} else
					{
						pPlugin->SetZxxParameter(plugParam, (UINT)CalculateSmoothParamChange((float)pPlugin->GetZxxParameter(plugParam), (float)(param & 0x7F)));
					}
				}
			}

			return 4;

		}

		// If we reach this point, the internal macro was invalid.

	} else
	{
		// Not an internal device. Pass on to appropriate plugin.
		const CHANNELINDEX nMasterCh = (nChn < GetNumChannels()) ? nChn + 1 : pChn->nMasterChn;
		if((nMasterCh) && (nMasterCh <= GetNumChannels()))
		{
			const PLUGINDEX nPlug = (pChn->dwFlags & CHN_NOFX) ? 0 : ((plugin != 0) ? plugin : GetBestPlugin(nChn, PrioritiseChannel, EvenIfMuted));
			if((nPlug) && (nPlug <= MAX_MIXPLUGINS))
			{
				IMixPlugin *pPlugin = m_MixPlugins[nPlug - 1].pMixPlugin;
				if ((pPlugin) && (m_MixPlugins[nPlug - 1].pMixState))
				{
					// currently, we don't support sending long MIDI messages in one go... split it up
					for(size_t pos = 0; pos < macroLen; pos += 3)
					{
						DWORD curData = 0;
						memcpy(&curData, macro + pos, Util::Min(3u, macroLen - pos));
						pPlugin->MidiSend(curData);
					}
				}
			}
		}

		return macroLen;

	}

	return 0;

}


//rewbs.volOffset: moved offset code to own method as it will be used in several places now
void CSoundFile::SampleOffset(CHANNELINDEX nChn, UINT param)
//----------------------------------------------------------
{

	ModChannel *pChn = &Chn[nChn];
// -! NEW_FEATURE#0010
// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
// rewbs.NOTE: maybe move param calculation outside of method to cope with [ effect.
			//if (param) pChn->nOldOffset = param; else param = pChn->nOldOffset;
			//param <<= 8;
			//param |= (UINT)(pChn->nOldHiOffset) << 16;
			ModCommand *m;
			m = NULL;

			if(m_nRow < Patterns[m_nPattern].GetNumRows() - 1) m = Patterns[m_nPattern].GetpModCommand(m_nRow + 1, nChn);

			if(m && m->command == CMD_XPARAM)
			{
				UINT tmp = m->param;
				m = NULL;
				if(m_nRow < Patterns[m_nPattern].GetNumRows() - 2) m = Patterns[m_nPattern].GetpModCommand(m_nRow + 2, nChn);

				if(m && m->command == CMD_XPARAM) param = (param << 16) + (tmp << 8) + m->param;
				else param = (param<<8) + tmp;
			}
			else
			{
				if (param) pChn->nOldOffset = param; else param = pChn->nOldOffset;
				param <<= 8;
				param |= (UINT)(pChn->nOldHiOffset) << 16;
			}
// -! NEW_FEATURE#0010

	if ((pChn->rowCommand.note >= NOTE_MIN) && (pChn->rowCommand.note <= NOTE_MAX))
	{
		pChn->nPos = param;
		pChn->nPosLo = 0;

		if (pChn->nPos >= pChn->nLength)
		{
			// Offset beyond sample size
			if (!(GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2)))
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
				// XM Compatibility: Don't play note if offset is beyond sample length
				// Test case: 3xx-no-old-samp.xm
				pChn->dwFlags |= CHN_FASTVOLRAMP;
				pChn->nVolume = pChn->nPeriod = 0;
			}
		}
	} else if ((param < pChn->nLength) && (GetType() & (MOD_TYPE_MTM|MOD_TYPE_DMF)))
	{
		// XXX what's this?
		pChn->nPos = param;
		pChn->nPosLo = 0;
	}

	return;
}
//end rewbs.volOffset:

void CSoundFile::RetrigNote(CHANNELINDEX nChn, int param, UINT offset)	//rewbs.VolOffset: added offset param.
//--------------------------------------------------------------------
{
	// Retrig: bit 8 is set if it's the new XM retrig
	ModChannel *pChn = &Chn[nChn];
	int nRetrigSpeed = param & 0x0F;
	int nRetrigCount = pChn->nRetrigCount;
	bool bDoRetrig = false;

	//IT compatibility 15. Retrigger
	if(IsCompatibleMode(TRK_IMPULSETRACKER))
	{
		if(m_nTickCount == 0 && pChn->rowCommand.note)
		{
			pChn->nRetrigCount = param & 0xf;
		}
		else if(!pChn->nRetrigCount || !--pChn->nRetrigCount)
		{
			pChn->nRetrigCount = param & 0xf;
			bDoRetrig = true;
		}
	} else if(IsCompatibleMode(TRK_FASTTRACKER2) && (param & 0x100))
	{
		// buggy-like-hell FT2 Rxy retrig!
		if(m_dwSongFlags & SONG_FIRSTTICK)
		{
			// here are some really stupid things FT2 does
			if(pChn->rowCommand.volcmd == VOLCMD_VOLUME) return;
			if(pChn->rowCommand.instr > 0 && pChn->rowCommand.note == NOTE_NONE) nRetrigCount = 1;
			if(pChn->rowCommand.note != NOTE_NONE && pChn->rowCommand.note <= GetModSpecifications().noteMax) nRetrigCount++;
		}
		if (nRetrigCount >= nRetrigSpeed)
		{
			if (!(m_dwSongFlags & SONG_FIRSTTICK) || (pChn->rowCommand.note == NOTE_NONE))
			{
				bDoRetrig = true;
				nRetrigCount = 0;
			}
		}
	} else
	{
		// old routines
		if (GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			if (!nRetrigSpeed) nRetrigSpeed = 1;
			if ((nRetrigCount) && (!(nRetrigCount % nRetrigSpeed))) bDoRetrig = true;
			nRetrigCount++;
		} else
		{
			int realspeed = nRetrigSpeed;
			// FT2 bug: if a retrig (Rxy) occours together with a volume command, the first retrig interval is increased by one tick
			if ((param & 0x100) && (pChn->rowCommand.volcmd == VOLCMD_VOLUME) && (pChn->rowCommand.param & 0xF0)) realspeed++;
			if (!(m_dwSongFlags & SONG_FIRSTTICK) || (param & 0x100))
			{
				if (!realspeed) realspeed = 1;
				if ((!(param & 0x100)) && (m_nMusicSpeed) && (!(m_nTickCount % realspeed))) bDoRetrig = true;
				nRetrigCount++;
			} else if (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2)) nRetrigCount = 0;
			if (nRetrigCount >= realspeed)
			{
				if ((m_nTickCount) || ((param & 0x100) && (!pChn->rowCommand.note))) bDoRetrig = true;
			}
		}
	}

	// IT compatibility: If a sample is shorter than the retrig time (i.e. it stops before the retrig counter hits zero), it is not retriggered.
	// Test case: retrig-short.it
	if(pChn->nLength == 0 && IsCompatibleMode(TRK_IMPULSETRACKER))
	{
		return;
	}

	if (bDoRetrig)
	{
		UINT dv = (param >> 4) & 0x0F;
		if (dv)
		{
			int vol = pChn->nVolume;

			// XM compatibility: Retrig + volume will not change volume of retrigged notes
			if(!IsCompatibleMode(TRK_FASTTRACKER2) || !(pChn->rowCommand.volcmd == VOLCMD_VOLUME))
			{
				if (retrigTable1[dv])
					vol = (vol * retrigTable1[dv]) >> 4;
				else
					vol += ((int)retrigTable2[dv]) << 2;
			}

			Limit(vol, 0, 256);

			pChn->nVolume = vol;
			pChn->dwFlags |= CHN_FASTVOLRAMP;
		}
		UINT nNote = pChn->nNewNote;
		LONG nOldPeriod = pChn->nPeriod;
		if ((nNote) && (nNote <= NOTE_MAX) && (pChn->nLength)) CheckNNA(nChn, 0, nNote, TRUE);
		bool bResetEnv = false;
		if (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))
		{
			if ((pChn->rowCommand.instr) && (param < 0x100))
			{
				InstrumentChange(pChn, pChn->rowCommand.instr, false, false);
				bResetEnv = true;
			}
			if (param < 0x100) bResetEnv = true;
		}
		// IT compatibility: Really weird combination of envelopes and retrigger (see Storlek's q.it testcase)
		NoteChange(nChn, nNote, IsCompatibleMode(TRK_IMPULSETRACKER), bResetEnv);
		if (m_nInstruments)
		{
			ProcessMidiOut(nChn);	//Send retrig to Midi
		}
		if ((GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)) && (!pChn->rowCommand.note) && (nOldPeriod)) pChn->nPeriod = nOldPeriod;
		if (!(GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))) nRetrigCount = 0;
		// IT compatibility: see previous IT compatibility comment =)
		if(IsCompatibleMode(TRK_IMPULSETRACKER)) pChn->nPos = pChn->nPosLo = 0;

		if (offset)									//rewbs.volOffset: apply offset on retrig
		{
			if (pChn->pModSample)
				pChn->nLength = pChn->pModSample->nLength;
			SampleOffset(nChn, offset);
		}
	}

	// buggy-like-hell FT2 Rxy retrig!
	if(IsCompatibleMode(TRK_FASTTRACKER2) && (param & 0x100)) nRetrigCount++;

	// Now we can also store the retrig value for IT...
	if(!IsCompatibleMode(TRK_IMPULSETRACKER))
		pChn->nRetrigCount = (BYTE)nRetrigCount;
}


void CSoundFile::DoFreqSlide(ModChannel *pChn, LONG nFreqSlide)
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


void CSoundFile::NoteCut(CHANNELINDEX nChn, UINT nTick)
//-----------------------------------------------------
{
	if(nTick == 0)
	{
		//IT compatibility 22. SC0 == SC1
		if(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
			nTick = 1;
		// ST3 doesn't cut notes with SC0
		else if(GetType() == MOD_TYPE_S3M)
			return;
	}

	if (m_nTickCount == nTick)
	{
		ModChannel *pChn = &Chn[nChn];
		// if (m_nInstruments) KeyOff(pChn); ?
		pChn->nVolume = 0;
		// S3M/IT compatibility: Note Cut really cuts notes and does not just mute them (so that following volume commands could restore the sample)
		// Test case: scx.it
		if(IsCompatibleMode(TRK_IMPULSETRACKER|TRK_SCREAMTRACKER))
		{
			pChn->nLength = 0;
			pChn->nPos = pChn->nPosLo = 0;
		}
		pChn->dwFlags |= CHN_FASTVOLRAMP;

		const ModInstrument *pIns = pChn->pModInstrument;
		// instro sends to a midi chan
		if (pIns && pIns->HasValidMIDIChannel())
		{
			UINT nPlug = pIns->nMixPlug;
			if ((nPlug) && (nPlug <= MAX_MIXPLUGINS))
			{
				IMixPlugin *pPlug = (IMixPlugin*)m_MixPlugins[nPlug-1].pMixPlugin;
				if (pPlug)
				{
					pPlug->MidiCommand(GetBestMidiChannel(nChn), pIns->nMidiProgram, pIns->wMidiBank, /*pChn->nNote+*/NOTE_KEYOFF, 0, nChn);
				}
			}
		}

	}
}


void CSoundFile::KeyOff(CHANNELINDEX nChn)
//----------------------------------------
{
	ModChannel *pChn = &Chn[nChn];
	const bool bKeyOn = !(pChn->dwFlags & CHN_KEYOFF);
	pChn->dwFlags |= CHN_KEYOFF;
	//if ((!pChn->pModInstrument) || (!(pChn->VolEnv.flags & CHN_VOLENV)))
	if ((pChn->pModInstrument) && (!(pChn->VolEnv.flags & ENV_ENABLED)))
	{
		pChn->dwFlags |= CHN_NOTEFADE;
	}
	if (!pChn->nLength) return;
	if ((pChn->dwFlags & CHN_SUSTAINLOOP) && (pChn->pModSample) && (bKeyOn))
	{
		const ModSample *pSmp = pChn->pModSample;
		if (pSmp->uFlags & CHN_LOOP)
		{
			if (pSmp->uFlags & CHN_PINGPONGLOOP)
				pChn->dwFlags |= CHN_PINGPONGLOOP;
			else
				pChn->dwFlags &= ~(CHN_PINGPONGLOOP | CHN_PINGPONGFLAG);
			pChn->dwFlags |= CHN_LOOP;
			pChn->nLength = pSmp->nLength;
			pChn->nLoopStart = pSmp->nLoopStart;
			pChn->nLoopEnd = pSmp->nLoopEnd;
			if (pChn->nLength > pChn->nLoopEnd) pChn->nLength = pChn->nLoopEnd;
			if(pChn->nPos > pChn->nLength)
			{
				pChn->nPos = pChn->nPos - pChn->nLength + pChn->nLoopStart;
				pChn->nPosLo = 0;
			}
		} else
		{
			pChn->dwFlags &= ~(CHN_LOOP | CHN_PINGPONGLOOP | CHN_PINGPONGFLAG);
			pChn->nLength = pSmp->nLength;
		}
	}

	if (pChn->pModInstrument)
	{
		const ModInstrument *pIns = pChn->pModInstrument;
		if (((pIns->VolEnv.dwFlags & ENV_LOOP) || (GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2))) && (pIns->nFadeOut))
		{
			pChn->dwFlags |= CHN_NOTEFADE;
		}

		if (pIns->VolEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET)
		{
			pChn->VolEnv.nEnvValueAtReleaseJump = Util::Round<LONG>(pIns->VolEnv.GetValueFromPosition(pChn->VolEnv.nEnvPosition) * 256.0f);
			pChn->VolEnv.nEnvPosition = pIns->VolEnv.Ticks[pIns->VolEnv.nReleaseNode];
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
	if ((!param) || (param >= 0x80) || ((GetType() & (MOD_TYPE_MOD|MOD_TYPE_XM|MOD_TYPE_MT2)) && (param >= 0x1E)))
	{
		if ((!m_nRepeatCount) && (IsSongFinished(m_nCurrentOrder, m_nRow+1)))
		{
			GlobalFadeSong(1000);
		}
	}
#endif // FASTSOUNDLIB
#endif // MODPLUG_TRACKER
	// Allow high speed values here for VBlank MODs. (Maybe it would be better to have a "VBlank MOD" flag somewhere? Is it worth the effort?)
	if ((param) && (param <= GetModSpecifications().speedMax || (GetType() & MOD_TYPE_MOD))) m_nMusicSpeed = param;
}


void CSoundFile::SetTempo(UINT param, bool setAsNonModcommand)
//------------------------------------------------------------
{
	const CModSpecifications& specs = GetModSpecifications();
	if(setAsNonModcommand)
	{
		// Set tempo from UI - ignore slide commands and such.
		m_nMusicTempo = CLAMP(param, specs.tempoMin, specs.tempoMax);
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


ROWINDEX CSoundFile::PatternLoop(ModChannel *pChn, UINT param)
//------------------------------------------------------------
{
	if (param)
	{
		// Loop Repeat
		if (pChn->nPatternLoopCount)
		{
			// There's a loop left
			pChn->nPatternLoopCount--;
			if(!pChn->nPatternLoopCount)
			{
				// IT compatibility 10. Pattern loops (+ same fix for MOD / S3M files)
				// When finishing a pattern loop, the next loop without a dedicated SB0 starts on the first row after the previous loop.
				if(IsCompatibleMode(TRK_IMPULSETRACKER | TRK_PROTRACKER | TRK_SCREAMTRACKER))
				{
					pChn->nPatternLoop = m_nRow + 1;
				}

				return ROWINDEX_INVALID;
			}
		} else
		{
			// This was the last loop

			// IT compatibility 10. Pattern loops (+ same fix for XM / MOD / S3M files)
			if(!IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2 | TRK_PROTRACKER | TRK_SCREAMTRACKER))
			{
				ModChannel *p = Chn;
				for (CHANNELINDEX i = 0; i < GetNumChannels(); i++, p++) if (p != pChn)
				{
					// Loop already done
					if (p->nPatternLoopCount) return ROWINDEX_INVALID;
				}
			}
			pChn->nPatternLoopCount = param;
		}
		m_nNextPatStartRow = pChn->nPatternLoop; // Nasty FT2 E60 bug emulation!
		return pChn->nPatternLoop;
	} else
	{
		// Loop Start
		pChn->nPatternLoop = m_nRow;
	}
	return ROWINDEX_INVALID;
}


void CSoundFile::GlobalVolSlide(UINT param, UINT &nOldGlobalVolSlide)
//--------------------------------------------------------------------
{
	LONG nGlbSlide = 0;
	if (param) nOldGlobalVolSlide = param; else param = nOldGlobalVolSlide;

	if((GetType() & (MOD_TYPE_XM | MOD_TYPE_MT2)))
	{
		// XM nibble priority
		if((param & 0xF0) != 0)
		{
			param &= 0xF0;
		} else
		{
			param &= 0x0F;
		}
	}

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
			if (param & 0xF0)
			{
				// IT compatibility: Ignore slide commands with both nibbles set.
				if(!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || (param & 0x0F) == 0)
					nGlbSlide = (int)((param & 0xF0) >> 4) * 2;
			} else
			{
				nGlbSlide = -(int)((param & 0x0F) * 2);
			}
		}
	}
	if (nGlbSlide)
	{
		if (!(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) nGlbSlide *= 2;
		nGlbSlide += m_nGlobalVolume;
		Limit(nGlbSlide, 0, 256);
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
	if (GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT|MOD_TYPE_S3M|MOD_TYPE_STM|MOD_TYPE_MDL|MOD_TYPE_ULT|MOD_TYPE_WAV
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
	if (GetType() & (MOD_TYPE_XM|MOD_TYPE_MT2))
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
			Limit(i , 0, 103);
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


PLUGINDEX CSoundFile::GetBestPlugin(CHANNELINDEX nChn, PluginPriority priority, PluginMutePriority respectMutes) const
//--------------------------------------------------------------------------------------------------------------------
{
	if (nChn >= MAX_CHANNELS)		//Check valid channel number
	{
		return 0;
	}

	//Define search source order
	PLUGINDEX nPlugin = 0;
	switch (priority)
	{
		case ChannelOnly:
			nPlugin = GetChannelPlugin(nChn, respectMutes);
			break;
		case InstrumentOnly:
			nPlugin  = GetActiveInstrumentPlugin(nChn, respectMutes);
			break;
		case PrioritiseInstrument:
			nPlugin  = GetActiveInstrumentPlugin(nChn, respectMutes);
			if ((!nPlugin) || (nPlugin > MAX_MIXPLUGINS))
			{
				nPlugin = GetChannelPlugin(nChn, respectMutes);
			}
			break;
		case PrioritiseChannel:
			nPlugin  = GetChannelPlugin(nChn, respectMutes);
			if ((!nPlugin) || (nPlugin > MAX_MIXPLUGINS))
			{
				nPlugin = GetActiveInstrumentPlugin(nChn, respectMutes);
			}
			break;
	}

	return nPlugin; // 0 Means no plugin found.
}


PLUGINDEX __cdecl CSoundFile::GetChannelPlugin(CHANNELINDEX nChn, PluginMutePriority respectMutes) const
//------------------------------------------------------------------------------------------------------
{
	const ModChannel &channel = Chn[nChn];

	PLUGINDEX nPlugin;
	if((respectMutes == RespectMutes && (channel.dwFlags & CHN_MUTE)) || (channel.dwFlags & CHN_NOFX))
	{
		nPlugin = 0;
	} else
	{
		// If it looks like this is an NNA channel, we need to find the master channel.
		// This ensures we pick up the right ChnSettings.
		// NB: nMasterChn == 0 means no master channel, so we need to -1 to get correct index.
		if (nChn >= m_nChannels && channel.nMasterChn > 0)
		{
			nChn = channel.nMasterChn - 1;
		}

		nPlugin = ChnSettings[nChn].nMixPlugin;
	}
	return nPlugin;
}


PLUGINDEX CSoundFile::GetActiveInstrumentPlugin(CHANNELINDEX nChn, PluginMutePriority respectMutes) const
//-------------------------------------------------------------------------------------------------------
{
	// Unlike channel settings, pModInstrument is copied from the original chan to the NNA chan,
	// so we don't need to worry about finding the master chan.

	PLUGINDEX nPlugin = 0;
	if (Chn[nChn].pModInstrument)
	{
		if (respectMutes == RespectMutes && Chn[nChn].pModSample && (Chn[nChn].pModSample->uFlags & CHN_MUTE))
		{
			nPlugin = 0;
		} else
		{
			nPlugin = Chn[nChn].pModInstrument->nMixPlug;
		}
	}
	return nPlugin;
}


uint8 CSoundFile::GetBestMidiChannel(CHANNELINDEX nChn) const
//----------------------------------------------------------
{
	if(nChn == CHANNELINDEX_INVALID)
	{
		return 0;
	}

	const ModInstrument *ins = Chn[nChn].pModInstrument;
	if(ins != nullptr)
	{
		if(ins->nMidiChannel == MidiMappedChannel)
		{
			// For mapped channels, return their pattern channel, modulo 16 (because there are only 16 MIDI channels)
			return (Chn[nChn].nMasterChn ? (Chn[nChn].nMasterChn - 1) : nChn) % 16;
		} else if(ins->HasValidMIDIChannel())
		{
			return (ins->nMidiChannel - 1) & 0x0F;
		}
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
		m_nNextOrder = m_nSeqOverride - 1;
		m_nSeqOverride = 0;
	}

	// Channel mutes
	for (CHANNELINDEX chan = 0; chan < GetNumChannels(); chan++)
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


void CSoundFile::PortamentoMPT(ModChannel* pChn, int param)
//---------------------------------------------------------
{
	//Behavior: Modifies portamento by param-steps on every tick.
	//Note that step meaning depends on tuning.

	pChn->m_PortamentoFineSteps += param;
	pChn->m_CalculateFreq = true;
}


void CSoundFile::PortamentoFineMPT(ModChannel* pChn, int param)
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