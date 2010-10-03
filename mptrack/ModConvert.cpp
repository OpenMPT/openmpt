/*
 * ModConvert.cpp
 * --------------
 * Purpose: Code for converting between various module formats.
 * Notes  : Incomplete list of MPTm-only features and extensions in the old formats:
 *          Features only available for MPTm:
 *           - User definable tunings.
 *           - Extended pattern range
 *           - Extended sequence
 *           - Multiple sequences ("songs")
 *           - Pattern-specific time signatures
 *           - Pattern effects :xy, S7D, S7E
 *           - Long instrument envelopes
 *           - Envelope release node (this was previously also usable in the IT format, but is now deprecated in that format)
 *
 *          Extended features in IT/XM/S3M/MOD (not all listed below are available in all of those formats):
 *           - Plugins
 *           - Extended ranges for
 *              - Sample count
 *              - Instrument count
 *              - Pattern count
 *              - Sequence size
 *              - Row count
 *              - Channel count
 *              - Tempo limits
 *           - Extended sample/instrument properties.
 *           - MIDI mapping directives
 *           - Version info
 *           - Channel names
 *           - Pattern names
 *           - Alternative tempomodes
 *           - For more info, see e.g. SaveExtendedSongProperties(), SaveExtendedInstrumentProperties()
 *
 * Authors: OpenMPT Devs
 *
 */

#include "Stdafx.h"
#include "Moddoc.h"
#include "Mainfrm.h"
#include "modsmp_ctrl.h"
#include "ModConvert.h"


#define CHANGEMODTYPE_WARNING(x)	warnings.set(x);
#define CHANGEMODTYPE_CHECK(x, s)	if(warnings[x]) AddToLog(_T(s));


// Trim envelopes and remove release nodes.
void UpdateEnvelopes(INSTRUMENTENVELOPE *mptEnv, CSoundFile *pSndFile, std::bitset<wNumWarnings> &warnings)
//---------------------------------------------------------------------------------------------------------
{
	// shorten instrument envelope if necessary (for mod conversion)
	const UINT iEnvMax = pSndFile->GetModSpecifications().envelopePointsMax;

	#define TRIMENV(nPat) if(nPat > iEnvMax) { nPat = iEnvMax; CHANGEMODTYPE_WARNING(wTrimmedEnvelopes); }

	TRIMENV(mptEnv->nNodes);
	TRIMENV(mptEnv->nLoopStart);
	TRIMENV(mptEnv->nLoopEnd);
	TRIMENV(mptEnv->nSustainStart);
	TRIMENV(mptEnv->nSustainEnd);
	if(mptEnv->nReleaseNode != ENV_RELEASE_NODE_UNSET)
	{
		TRIMENV(mptEnv->nReleaseNode);
		if(!pSndFile->GetModSpecifications().hasReleaseNode)
		{
			mptEnv->nReleaseNode = ENV_RELEASE_NODE_UNSET;
			CHANGEMODTYPE_WARNING(wReleaseNode);
		}
	}

	#undef TRIMENV
}


bool CModDoc::ChangeModType(MODTYPE nNewType)
//-------------------------------------------
{
	std::bitset<wNumWarnings> warnings;
	warnings.reset();
	PATTERNINDEX nResizedPatterns = 0;

	const MODTYPE nOldType = m_SndFile.GetType();
	
	if (nNewType == nOldType && nNewType == MOD_TYPE_IT)
	{
		// Even if m_nType doesn't change, we might need to change extension in itp<->it case.
		// This is because ITP is a HACK and doesn't genuinely change m_nType,
		// but uses flags instead.
		ChangeFileExtension(nNewType);
		return true;
	}

	if(nNewType == nOldType)
		return true;

	const bool oldTypeIsMOD = (nOldType == MOD_TYPE_MOD), oldTypeIsXM = (nOldType == MOD_TYPE_XM),
				oldTypeIsS3M = (nOldType == MOD_TYPE_S3M), oldTypeIsIT = (nOldType == MOD_TYPE_IT),
				oldTypeIsMPT = (nOldType == MOD_TYPE_MPT), oldTypeIsMOD_XM = (oldTypeIsMOD || oldTypeIsXM),
                oldTypeIsS3M_IT_MPT = (oldTypeIsS3M || oldTypeIsIT || oldTypeIsMPT),
                oldTypeIsIT_MPT = (oldTypeIsIT || oldTypeIsMPT);

	const bool newTypeIsMOD = (nNewType == MOD_TYPE_MOD), newTypeIsXM =  (nNewType == MOD_TYPE_XM), 
				newTypeIsS3M = (nNewType == MOD_TYPE_S3M), newTypeIsIT = (nNewType == MOD_TYPE_IT),
				newTypeIsMPT = (nNewType == MOD_TYPE_MPT), newTypeIsMOD_XM = (newTypeIsMOD || newTypeIsXM), 
				newTypeIsS3M_IT_MPT = (newTypeIsS3M || newTypeIsIT || newTypeIsMPT), 
				newTypeIsXM_IT_MPT = (newTypeIsXM || newTypeIsIT || newTypeIsMPT),
				newTypeIsIT_MPT = (newTypeIsIT || newTypeIsMPT);

	const CModSpecifications& specs = m_SndFile.GetModSpecifications(nNewType);

	// Check if conversion to 64 rows is necessary
	for(PATTERNINDEX nPat = 0; nPat < m_SndFile.Patterns.Size(); nPat++)
	{
		if ((m_SndFile.Patterns[nPat]) && (m_SndFile.Patterns[nPat].GetNumRows() != 64))
			nResizedPatterns++;
	}

	if(((m_SndFile.m_nInstruments) || (nResizedPatterns)) && (nNewType & (MOD_TYPE_MOD|MOD_TYPE_S3M)))
	{
		if(::MessageBox(NULL,
				"This operation will convert all instruments to samples,\n"
				"and resize all patterns to 64 rows.\n"
				"Do you want to continue?", "Warning", MB_YESNO | MB_ICONQUESTION) != IDYES) return false;
		BeginWaitCursor();
		BEGIN_CRITICAL();

		// Converting instruments to samples
		if(m_SndFile.m_nInstruments)
		{
			ConvertInstrumentsToSamples();
			CHANGEMODTYPE_WARNING(wInstrumentsToSamples);
		}

		// Resizing all patterns to 64 rows
		for(PATTERNINDEX nPat = 0; nPat < m_SndFile.Patterns.Size(); nPat++) if ((m_SndFile.Patterns[nPat]) && (m_SndFile.Patterns[nPat].GetNumRows() != 64))
		{
			// try to save short patterns by inserting a pattern break.
			if(m_SndFile.Patterns[nPat].GetNumRows() < 64)
			{
				m_SndFile.TryWriteEffect(nPat, m_SndFile.Patterns[nPat].GetNumRows() - 1, CMD_PATTERNBREAK, 0, false, CHANNELINDEX_INVALID, false, true);
			}
			m_SndFile.Patterns[nPat].Resize(64, false);
			CHANGEMODTYPE_WARNING(wResizedPatterns);
		}

		// Removing all instrument headers
		for(CHANNELINDEX nChn = 0; nChn < MAX_CHANNELS; nChn++)
		{
			m_SndFile.Chn[nChn].pModInstrument = nullptr;
		}

		for(INSTRUMENTINDEX nIns = 0; nIns < m_SndFile.m_nInstruments; nIns++) if (m_SndFile.Instruments[nIns])
		{
			delete m_SndFile.Instruments[nIns];
			m_SndFile.Instruments[nIns] = nullptr;
		}
		m_SndFile.m_nInstruments = 0;
		END_CRITICAL();
		EndWaitCursor();
	} //End if (((m_SndFile.m_nInstruments) || (b64)) && (nNewType & (MOD_TYPE_MOD|MOD_TYPE_S3M)))
	BeginWaitCursor();


	/////////////////////////////
	// Converting pattern data

	for(PATTERNINDEX nPat = 0; nPat < m_SndFile.Patterns.Size(); nPat++) if (m_SndFile.Patterns[nPat])
	{
		MODCOMMAND *m = m_SndFile.Patterns[nPat];

		// This is used for -> MOD/XM conversion
		vector<vector<MODCOMMAND::PARAM> > cEffectMemory;
		cEffectMemory.resize(m_SndFile.GetNumChannels());
		for(size_t i = 0; i < m_SndFile.GetNumChannels(); i++)
		{
			cEffectMemory[i].resize(MAX_EFFECTS, 0);
		}

		CHANNELINDEX nChannel = m_SndFile.m_nChannels - 1;

		for (UINT len = m_SndFile.Patterns[nPat].GetNumRows() * m_SndFile.m_nChannels; len; m++, len--)
		{
			nChannel = (nChannel + 1) % m_SndFile.m_nChannels; // 0...Channels - 1

			m_SndFile.ConvertCommand(m, nOldType, nNewType);

			// Deal with effect memory for MOD/XM arpeggio
			if (oldTypeIsS3M_IT_MPT && newTypeIsMOD_XM)
			{
				switch(m->command)
				{
				case CMD_ARPEGGIO:
				case CMD_S3MCMDEX:
				case CMD_MODCMDEX:
					// No effect memory in XM / MOD
					if(m->param == 0)
						m->param = cEffectMemory[nChannel][m->command];
					else
						cEffectMemory[nChannel][m->command] = m->param;
					break;
				}
			}

			// Adjust effect memory for MOD files
			if(newTypeIsMOD)
			{
				switch(m->command)
				{
				case CMD_PORTAMENTOUP:
				case CMD_PORTAMENTODOWN:
				case CMD_TONEPORTAVOL:
				case CMD_VIBRATOVOL:
				case CMD_VOLUMESLIDE:
					// ProTracker doesn't have effect memory for these commands, so let's try to fix them
					if(m->param == 0)
						m->param = cEffectMemory[nChannel][m->command];
					else
						cEffectMemory[nChannel][m->command] = m->param;
					break;				

				}
			}
		}
	}

	////////////////////////////////////////////////
	// Converting instrument / sample / etc. data


	// Do some sample conversion
	for(SAMPLEINDEX nSmp = 1; nSmp <= m_SndFile.m_nSamples; nSmp++)
	{
		// No Bidi / Sustain loops / Autovibrato for MOD/S3M
		if(newTypeIsMOD || newTypeIsS3M)
		{
			// Bidi loops
			if((m_SndFile.Samples[nSmp].uFlags & CHN_PINGPONGLOOP) != 0)
			{
				m_SndFile.Samples[nSmp].uFlags &= ~CHN_PINGPONGLOOP;
				CHANGEMODTYPE_WARNING(wSampleBidiLoops);
			}

			// Sustain loops
			if(m_SndFile.Samples[nSmp].nSustainStart || m_SndFile.Samples[nSmp].nSustainEnd)
			{
				// We can at least try to convert sustain loops to normal loops
				if(m_SndFile.Samples[nSmp].nLoopEnd == 0)
				{
					m_SndFile.Samples[nSmp].nSustainStart = m_SndFile.Samples[nSmp].nLoopStart;
					m_SndFile.Samples[nSmp].nSustainEnd = m_SndFile.Samples[nSmp].nLoopEnd;
					m_SndFile.Samples[nSmp].uFlags |= CHN_LOOP;
				}
				m_SndFile.Samples[nSmp].nSustainStart = m_SndFile.Samples[nSmp].nSustainEnd = 0;
				m_SndFile.Samples[nSmp].uFlags &= ~(CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN);
				CHANGEMODTYPE_WARNING(wSampleSustainLoops);
			}

			// Autovibrato
			if(m_SndFile.Samples[nSmp].nVibDepth || m_SndFile.Samples[nSmp].nVibRate || m_SndFile.Samples[nSmp].nVibSweep)
			{
				m_SndFile.Samples[nSmp].nVibDepth = m_SndFile.Samples[nSmp].nVibRate = m_SndFile.Samples[nSmp].nVibSweep = m_SndFile.Samples[nSmp].nVibType = 0;
				CHANGEMODTYPE_WARNING(wSampleAutoVibrato);
			}
		}

		// Transpose to Frequency (MOD/XM to S3M/IT/MPT)
		if(oldTypeIsMOD_XM && newTypeIsS3M_IT_MPT)
		{
			m_SndFile.Samples[nSmp].nC5Speed = CSoundFile::TransposeToFrequency(m_SndFile.Samples[nSmp].RelativeTone, m_SndFile.Samples[nSmp].nFineTune);
			m_SndFile.Samples[nSmp].RelativeTone = 0;
			m_SndFile.Samples[nSmp].nFineTune = 0;
		}

		// Frequency to Transpose, panning (S3M/IT/MPT to MOD/XM)
		if(oldTypeIsS3M_IT_MPT && newTypeIsMOD_XM)
		{
			CSoundFile::FrequencyToTranspose(&m_SndFile.Samples[nSmp]);
			if (!(m_SndFile.Samples[nSmp].uFlags & CHN_PANNING)) m_SndFile.Samples[nSmp].nPan = 128;
			// No relative note for MOD files
			// TODO: Pattern notes could be transposed based on the previous relative tone?
			if(newTypeIsMOD && m_SndFile.Samples[nSmp].RelativeTone != 0)
			{
				m_SndFile.Samples[nSmp].RelativeTone = 0;
				CHANGEMODTYPE_WARNING(wMODSampleFrequency);
			}
		}

		if(oldTypeIsXM && newTypeIsIT_MPT)
		{
			// Autovibrato settings (XM to IT, where sweep 0 means "no vibrato")
			if(m_SndFile.Samples[nSmp].nVibSweep == 0 && m_SndFile.Samples[nSmp].nVibRate != 0 && m_SndFile.Samples[nSmp].nVibDepth != 0)
				m_SndFile.Samples[nSmp].nVibSweep = 255;
		} else if(oldTypeIsIT_MPT && newTypeIsXM)
		{
			// Autovibrato settings (IT to XM, where sweep 0 means "no sweep")
			if(m_SndFile.Samples[nSmp].nVibSweep == 0)
				m_SndFile.Samples[nSmp].nVibRate = m_SndFile.Samples[nSmp].nVibDepth = 0;
		}
	}

	// Convert IT/MPT to XM (instruments)
	if(oldTypeIsIT_MPT && newTypeIsXM)
	{
		for(INSTRUMENTINDEX nIns = 1; nIns <= m_SndFile.GetNumInstruments(); nIns++)
		{
			MODINSTRUMENT *pIns = m_SndFile.Instruments[nIns];
			if (pIns)
			{
				for (UINT k = 0; k < NOTE_MAX; k++)
				{
					if ((pIns->NoteMap[k]) && (pIns->NoteMap[k] != (BYTE)(k+1)))
					{
						CHANGEMODTYPE_WARNING(wBrokenNoteMap);
						break;
					}
				}
				// Convert sustain loops to sustain "points"
				if(pIns->VolEnv.nSustainStart != pIns->VolEnv.nSustainEnd)
				{
					CHANGEMODTYPE_WARNING(wInstrumentSustainLoops);
					pIns->VolEnv.nSustainEnd = pIns->VolEnv.nSustainStart;
				}
				if(pIns->PanEnv.nSustainStart != pIns->PanEnv.nSustainEnd)
				{
					CHANGEMODTYPE_WARNING(wInstrumentSustainLoops);
					pIns->PanEnv.nSustainEnd = pIns->PanEnv.nSustainStart;
				}
				pIns->VolEnv.dwFlags &= ~ENV_CARRY;
				pIns->PanEnv.dwFlags &= ~ENV_CARRY;
				pIns->PitchEnv.dwFlags &= ~(ENV_CARRY|ENV_ENABLED|ENV_FILTER);
				pIns->dwFlags &= ~INS_SETPANNING;
				pIns->nIFC &= 0x7F;
				pIns->nIFR &= 0x7F;
			}
		}
	}

	// Instrument tunings
	if(oldTypeIsMPT)
	{
		for(INSTRUMENTINDEX nIns = 1; nIns <= m_SndFile.GetNumInstruments(); nIns++)
		{
			if(m_SndFile.Instruments[nIns] != nullptr && m_SndFile.Instruments[nIns]->pTuning != nullptr)
			{
				m_SndFile.Instruments[nIns]->SetTuning(nullptr);
				CHANGEMODTYPE_WARNING(wInstrumentTuning);
			}
		}
	}

	if(newTypeIsMOD)
	{
		// Not supported in MOD format
		m_SndFile.m_nDefaultSpeed = 6;
		m_SndFile.m_nDefaultTempo = 125;
		m_SndFile.m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
		m_SndFile.m_nSamplePreAmp = 48;
		m_SndFile.m_nVSTiVolume = 48;
		CHANGEMODTYPE_WARNING(wMODGlobalVars);

		// Too many samples?
		if(m_SndFile.m_nSamples > 31)
		{
			CHANGEMODTYPE_WARNING(wMOD31Samples);
		}
	}

	// Is the "restart position" value allowed in this format?
	if(m_SndFile.m_nRestartPos > 0 && !CSoundFile::GetModSpecifications(nNewType).hasRestartPos)
	{
		m_SndFile.m_nRestartPos = 0;
		CHANGEMODTYPE_WARNING(wRestartPos);
	}

	// Fix channel settings (pan/vol)
	for(CHANNELINDEX nChn = 0; nChn < m_SndFile.m_nChannels; nChn++)
	{
		if(newTypeIsMOD_XM || newTypeIsS3M)
		{
			if(m_SndFile.ChnSettings[nChn].nVolume != 64 || (m_SndFile.ChnSettings[nChn].dwFlags & CHN_SURROUND))
			{
				m_SndFile.ChnSettings[nChn].nVolume = 64;
				m_SndFile.ChnSettings[nChn].dwFlags &= ~CHN_SURROUND;
				CHANGEMODTYPE_WARNING(wChannelVolSurround);
			}
		}
		if(newTypeIsXM)
		{
			if(m_SndFile.ChnSettings[nChn].nPan != 128)
			{
				m_SndFile.ChnSettings[nChn].nPan = 128;
				CHANGEMODTYPE_WARNING(wChannelPanning);
			}
		}
	}

	// Check for patterns with custom time signatures (fixing will be applied in the pattern container)
	if(!CSoundFile::GetModSpecifications(nNewType).hasPatternSignatures)
	{
		for(PATTERNINDEX nPat = 0; nPat < m_SndFile.GetNumPatterns(); nPat++)
		{
			if(m_SndFile.Patterns[nPat].GetOverrideSignature())
			{
				CHANGEMODTYPE_WARNING(wPatternSignatures);
				break;
			}
		}
	}

	// Check whether the new format supports embedding the edit history in the file.
	if(oldTypeIsIT_MPT && !newTypeIsIT_MPT && GetFileHistory()->size() > 0)
	{
		CHANGEMODTYPE_WARNING(wEditHistory);
	}

	BEGIN_CRITICAL();
	m_SndFile.ChangeModTypeTo(nNewType);
	if(!newTypeIsXM_IT_MPT && (m_SndFile.m_dwSongFlags & SONG_LINEARSLIDES))
	{
		CHANGEMODTYPE_WARNING(wLinearSlides);
		m_SndFile.m_dwSongFlags &= ~SONG_LINEARSLIDES;
	}
	if(!newTypeIsIT_MPT) m_SndFile.m_dwSongFlags &= ~(SONG_ITOLDEFFECTS|SONG_ITCOMPATGXX);
	if(!newTypeIsS3M) m_SndFile.m_dwSongFlags &= ~SONG_FASTVOLSLIDES;
	if(!newTypeIsMOD) m_SndFile.m_dwSongFlags &= ~SONG_PT1XMODE;
	if(newTypeIsS3M || newTypeIsMOD) m_SndFile.m_dwSongFlags &= ~SONG_EXFILTERRANGE;
	if(oldTypeIsXM && newTypeIsIT_MPT) m_SndFile.m_dwSongFlags |= SONG_ITCOMPATGXX;

	END_CRITICAL();
	ChangeFileExtension(nNewType);

	// Check mod specifications
	m_SndFile.m_nDefaultTempo = CLAMP(m_SndFile.m_nDefaultTempo, specs.tempoMin, specs.tempoMax);
	m_SndFile.m_nDefaultSpeed = CLAMP(m_SndFile.m_nDefaultSpeed, specs.speedMin, specs.speedMax);

	for(INSTRUMENTINDEX i = 1; i <= m_SndFile.m_nInstruments; i++) if(m_SndFile.Instruments[i] != nullptr)
	{
		UpdateEnvelopes(&(m_SndFile.Instruments[i]->VolEnv), &m_SndFile, warnings);
		UpdateEnvelopes(&(m_SndFile.Instruments[i]->PanEnv), &m_SndFile, warnings);
		UpdateEnvelopes(&(m_SndFile.Instruments[i]->PitchEnv), &m_SndFile, warnings);
	}
		
	CHAR s[64];
	CHANGEMODTYPE_CHECK(wInstrumentsToSamples, "All instruments have been converted to samples.\n");
	wsprintf(s, "%d patterns have been resized to 64 rows\n", nResizedPatterns);
	CHANGEMODTYPE_CHECK(wResizedPatterns, s);
	CHANGEMODTYPE_CHECK(wSampleBidiLoops, "Sample bidi loops are not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wSampleSustainLoops, "New format doesn't support sample sustain loops.\n");
	CHANGEMODTYPE_CHECK(wSampleAutoVibrato, "New format doesn't support sample autovibrato.\n");
	CHANGEMODTYPE_CHECK(wMODSampleFrequency, "Sample C-5 frequencies will be lost.\n");
	CHANGEMODTYPE_CHECK(wBrokenNoteMap, "Note Mapping will be lost when saving as XM.\n");
	CHANGEMODTYPE_CHECK(wInstrumentSustainLoops, "Sustain loops were converted to sustain points.\n");
	CHANGEMODTYPE_CHECK(wInstrumentTuning, "Instrument tunings will be lost.\n");
	CHANGEMODTYPE_CHECK(wMODGlobalVars, "Default speed, tempo and global volume will be lost.\n");
	CHANGEMODTYPE_CHECK(wMOD31Samples, "Samples above 31 will be lost when saving as MOD. Consider rearranging samples if there are unused slots available.\n");
	CHANGEMODTYPE_CHECK(wRestartPos, "Restart position is not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wChannelVolSurround, "Channel volume and surround are not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wChannelPanning, "Channel panning is not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wPatternSignatures, "Pattern-specific time signatures are not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wLinearSlides, "Linear Frequency Slides not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wTrimmedEnvelopes, "Instrument envelopes have been shortened.\n");
	CHANGEMODTYPE_CHECK(wReleaseNode, "Instrument envelope release nodes are not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wEditHistory, "Edit history will not be saved in the new format.\n");

	SetModified();
	GetPatternUndo()->ClearUndo();
	GetSampleUndo()->ClearUndo();
	UpdateAllViews(NULL, HINT_MODTYPE | HINT_MODGENERAL);
	EndWaitCursor();

	//rewbs.customKeys: update effect key commands
	CInputHandler *ih = CMainFrame::GetMainFrame()->GetInputHandler();
	if	(newTypeIsMOD_XM)
		ih->SetXMEffects();
	else
		ih->SetITEffects();
	//end rewbs.customKeys

	return true;
}


#undef CHANGEMODTYPE_WARNING
#undef CHANGEMODTYPE_CHECK
