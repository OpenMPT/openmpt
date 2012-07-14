/*
 * ModConvert.cpp
 * --------------
 * Purpose: Converting between various module formats.
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
 *          Extended features in IT/XM/S3M (not all listed below are available in all of those formats):
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
 *           - Alternative tempo modes
 *           - For more info, see e.g. SaveExtendedSongProperties(), SaveExtendedInstrumentProperties()
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "Stdafx.h"
#include "Moddoc.h"
#include "Mainfrm.h"
#include "modsmp_ctrl.h"
#include "ModConvert.h"


#define CHANGEMODTYPE_WARNING(x)	warnings.set(x);
#define CHANGEMODTYPE_CHECK(x, s)	if(warnings[x]) AddToLog(_T(s));


// Trim envelopes and remove release nodes.
void UpdateEnvelopes(InstrumentEnvelope *mptEnv, CSoundFile *pSndFile, std::bitset<wNumWarnings> &warnings)
//---------------------------------------------------------------------------------------------------------
{
	// shorten instrument envelope if necessary (for mod conversion)
	const uint8 iEnvMax = pSndFile->GetModSpecifications().envelopePointsMax;

	#define TRIMENV(iEnvLen) if(iEnvLen > iEnvMax) { iEnvLen = iEnvMax; CHANGEMODTYPE_WARNING(wTrimmedEnvelopes); }

	TRIMENV(mptEnv->nNodes);
	TRIMENV(mptEnv->nLoopStart);
	TRIMENV(mptEnv->nLoopEnd);
	TRIMENV(mptEnv->nSustainStart);
	TRIMENV(mptEnv->nSustainEnd);
	if(mptEnv->nReleaseNode != ENV_RELEASE_NODE_UNSET)
	{
		if(pSndFile->GetModSpecifications().hasReleaseNode)
		{
			TRIMENV(mptEnv->nReleaseNode);
		} else
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
	
	if(nNewType == nOldType && nNewType == MOD_TYPE_IT)
	{
		// Even if m_nType doesn't change, we might need to change extension in itp<->it case.
		// This is because ITP is a HACK and doesn't genuinely change m_nType,
		// but uses flags instead.
		ChangeFileExtension(nNewType);
		return true;
	}

	if(nNewType == nOldType)
		return true;

	const bool oldTypeIsXM = (nOldType == MOD_TYPE_XM),
		oldTypeIsS3M = (nOldType == MOD_TYPE_S3M), oldTypeIsIT = (nOldType == MOD_TYPE_IT),
		oldTypeIsMPT = (nOldType == MOD_TYPE_MPT),
		oldTypeIsS3M_IT_MPT = (oldTypeIsS3M || oldTypeIsIT || oldTypeIsMPT),
		oldTypeIsIT_MPT = (oldTypeIsIT || oldTypeIsMPT);

	const bool newTypeIsMOD = (nNewType == MOD_TYPE_MOD), newTypeIsXM =  (nNewType == MOD_TYPE_XM), 
		newTypeIsS3M = (nNewType == MOD_TYPE_S3M), newTypeIsIT = (nNewType == MOD_TYPE_IT),
		newTypeIsMPT = (nNewType == MOD_TYPE_MPT), newTypeIsMOD_XM = (newTypeIsMOD || newTypeIsXM), 
		newTypeIsIT_MPT = (newTypeIsIT || newTypeIsMPT);

	const CModSpecifications& specs = m_SndFile.GetModSpecifications(nNewType);

	// Check if conversion to 64 rows is necessary
	for(PATTERNINDEX nPat = 0; nPat < m_SndFile.Patterns.Size(); nPat++)
	{
		if ((m_SndFile.Patterns[nPat]) && (m_SndFile.Patterns[nPat].GetNumRows() != 64))
			nResizedPatterns++;
	}

	if((m_SndFile.GetNumInstruments() || nResizedPatterns) && (nNewType & (MOD_TYPE_MOD|MOD_TYPE_S3M)))
	{
		if(Reporting::Confirm(
			"This operation will convert all instruments to samples,\n"
			"and resize all patterns to 64 rows.\n"
			"Do you want to continue?", "Warning") != cnfYes) return false;
		BeginWaitCursor();
		CriticalSection cs;

		// Converting instruments to samples
		if(m_SndFile.GetNumInstruments())
		{
			ConvertInstrumentsToSamples();
			CHANGEMODTYPE_WARNING(wInstrumentsToSamples);
		}

		// Resizing all patterns to 64 rows
		for(PATTERNINDEX nPat = 0; nPat < m_SndFile.Patterns.Size(); nPat++) if ((m_SndFile.Patterns[nPat]) && (m_SndFile.Patterns[nPat].GetNumRows() != 64))
		{
			ROWINDEX origRows = m_SndFile.Patterns[nPat].GetNumRows();
			m_SndFile.Patterns[nPat].Resize(64);

			if(origRows < 64)
			{
				// Try to save short patterns by inserting a pattern break.
				m_SndFile.Patterns[nPat].WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(m_SndFile.Patterns[nPat].GetNumRows() - 1).Retry(EffectWriter::rmTryNextRow));
			}

			CHANGEMODTYPE_WARNING(wResizedPatterns);
		}

		// Removing all instrument headers from channels
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

		cs.Leave();
		EndWaitCursor();
	} //End if (((m_SndFile.m_nInstruments) || (b64)) && (nNewType & (MOD_TYPE_MOD|MOD_TYPE_S3M)))
	BeginWaitCursor();


	/////////////////////////////
	// Converting pattern data

	for(PATTERNINDEX nPat = 0; nPat < m_SndFile.Patterns.Size(); nPat++) if (m_SndFile.Patterns[nPat])
	{
		ModCommand *m = m_SndFile.Patterns[nPat];

		// This is used for -> MOD/XM conversion
		vector<vector<ModCommand::PARAM> > cEffectMemory(GetNumChannels());
		for(size_t i = 0; i < GetNumChannels(); i++)
		{
			cEffectMemory[i].resize(MAX_EFFECTS, 0);
		}

		bool addBreak = false;	// When converting to XM, avoid the E60 bug.
		CHANNELINDEX channel = 0;
		ROWINDEX row = 0;

		for (UINT len = m_SndFile.Patterns[nPat].GetNumRows() * m_SndFile.GetNumChannels(); len; m++, len--, channel++)
		{
			if(channel >= GetNumChannels())
			{
				channel = 0;
				row++;
			}

			m->Convert(nOldType, nNewType);

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
						m->param = cEffectMemory[channel][m->command];
					else
						cEffectMemory[channel][m->command] = m->param;
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
						m->param = cEffectMemory[channel][m->command];
					else
						cEffectMemory[channel][m->command] = m->param;
					break;

				}
			}

			// When converting to XM, avoid the E60 bug.
			if(newTypeIsXM)
			{
				switch(m->command)
				{
				case CMD_MODCMDEX:
					if(m->param == 0x60 && row > 0)
					{
						addBreak = true;
					}
					break;
				case CMD_POSITIONJUMP:
				case CMD_PATTERNBREAK:
					addBreak = false;
					break;
				}
			}

			// Fix Row Delay commands when converting between MOD/XM and S3M/IT.
			// FT2 only considers the rightmost command, ST3/IT only the leftmost...
			if((nOldType & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)) && (nNewType & (MOD_TYPE_MOD | MOD_TYPE_XM))
				&& m->command == CMD_MODCMDEX && (m->param & 0xF0) == 0xE0)
			{
				if(oldTypeIsIT_MPT || m->param != 0xE0)
				{
					// If the leftmost row delay command is SE0, ST3 ignores it, IT doesn't.

					// Delete all commands right of the first command
					ModCommand *p = m + 1;
					for(CHANNELINDEX c = channel + 1; c < m_SndFile.GetNumChannels(); c++, p++)
					{
						if(p->command == CMD_S3MCMDEX && (p->param & 0xF0) == 0xE0)
						{
							p->command = CMD_NONE;
						}
					}
				}
			} else if((nOldType & (MOD_TYPE_MOD | MOD_TYPE_XM)) && (nNewType & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT))
				&& m->command == CMD_S3MCMDEX && (m->param & 0xF0) == 0xE0)
			{
				// Delete all commands left of the last command
				ModCommand *p = m - 1;
				for(CHANNELINDEX c = 0; c < channel; c++, p--)
				{
					if(p->command == CMD_S3MCMDEX && (p->param & 0xF0) == 0xE0)
					{
						p->command = CMD_NONE;
					}
				}
			}

		}
		if(addBreak)
		{
			m_SndFile.Patterns[nPat].WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(m_SndFile.Patterns[nPat].GetNumRows() - 1));
		}
	}

	////////////////////////////////////////////////
	// Converting instrument / sample / etc. data


	// Do some sample conversion
	for(SAMPLEINDEX nSmp = 1; nSmp <= m_SndFile.GetNumSamples(); nSmp++)
	{
		ModSample &sample = m_SndFile.GetSample(nSmp);
		GetSampleUndo().PrepareUndo(nSmp, sundo_none);

		// Too many samples? Only 31 samples allowed in MOD format...
		if(newTypeIsMOD && nSmp > 31 && sample.nLength > 0)
		{
			CHANGEMODTYPE_WARNING(wMOD31Samples);
		}

		// No Bidi and Autovibrato for MOD/S3M
		if(newTypeIsMOD || newTypeIsS3M)
		{
			// Bidi loops
			if((sample.uFlags & CHN_PINGPONGLOOP) != 0)
			{
				CHANGEMODTYPE_WARNING(wSampleBidiLoops);
			}

			// Autovibrato
			if(sample.nVibDepth || sample.nVibRate || sample.nVibSweep)
			{
				CHANGEMODTYPE_WARNING(wSampleAutoVibrato);
			}
		}

		// No sustain loops for MOD/S3M/XM
		if(newTypeIsMOD_XM || newTypeIsS3M)
		{
			// Sustain loops - convert to normal loops
			if((sample.uFlags & CHN_SUSTAINLOOP) != 0)
			{
				CHANGEMODTYPE_WARNING(wSampleSustainLoops);
			}
		}

		// TODO: Pattern notes could be transposed based on the previous relative tone?
		if(newTypeIsMOD && sample.RelativeTone != 0)
		{
			CHANGEMODTYPE_WARNING(wMODSampleFrequency);
		}

		sample.Convert(nOldType, nNewType);
	}

	for(INSTRUMENTINDEX nIns = 1; nIns <= m_SndFile.GetNumInstruments(); nIns++)
	{
		ModInstrument *pIns = m_SndFile.Instruments[nIns];
		if(pIns == nullptr)
		{
			continue;
		}

		// Convert IT/MPT to XM (fix instruments)
		if(oldTypeIsIT_MPT && newTypeIsXM)
		{
			for (size_t i = 0; i < CountOf(pIns->NoteMap); i++)
			{
				if (pIns->NoteMap[i] && pIns->NoteMap[i] != static_cast<BYTE>(i + 1))
				{
					CHANGEMODTYPE_WARNING(wBrokenNoteMap);
					break;
				}
			}
			// Convert sustain loops to sustain "points"
			if(pIns->VolEnv.nSustainStart != pIns->VolEnv.nSustainEnd)
			{
				CHANGEMODTYPE_WARNING(wInstrumentSustainLoops);
			}
			if(pIns->PanEnv.nSustainStart != pIns->PanEnv.nSustainEnd)
			{
				CHANGEMODTYPE_WARNING(wInstrumentSustainLoops);
			}
		}


		// Convert MPT to anything - remove instrument tunings, Pitch/Tempo Lock, filter variation
		if(oldTypeIsMPT)
		{
			if(pIns->pTuning != nullptr)
			{
				CHANGEMODTYPE_WARNING(wInstrumentTuning);
			}

			if(pIns->wPitchToTempoLock != 0)
			{
				CHANGEMODTYPE_WARNING(wPitchToTempoLock);
			}

			if((pIns->nCutSwing | pIns->nResSwing) != 0)
			{
				CHANGEMODTYPE_WARNING(wFilterVariation);
			}
		}

		pIns->Convert(nOldType, nNewType);
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
	}

	// Is the "restart position" value allowed in this format?
	if(m_SndFile.m_nRestartPos > 0 && !CSoundFile::GetModSpecifications(nNewType).hasRestartPos)
	{
		// Try to fix it by placing a pattern jump command in the pattern.
		if(!RestartPosToPattern())
		{
			// Couldn't fix it! :(
			CHANGEMODTYPE_WARNING(wRestartPos);
		}
	}

	// Fix channel settings (pan/vol)
	for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
	{
		if(newTypeIsMOD_XM || newTypeIsS3M)
		{
			if(m_SndFile.ChnSettings[nChn].nVolume != 64 || m_SndFile.ChnSettings[nChn].dwFlags[CHN_SURROUND])
			{
				m_SndFile.ChnSettings[nChn].nVolume = 64;
				m_SndFile.ChnSettings[nChn].dwFlags.reset(CHN_SURROUND);
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
		for(PATTERNINDEX nPat = 0; nPat < m_SndFile.Patterns.GetNumPatterns(); nPat++)
		{
			if(m_SndFile.Patterns[nPat].GetOverrideSignature())
			{
				CHANGEMODTYPE_WARNING(wPatternSignatures);
				break;
			}
		}
	}

	// Check whether the new format supports embedding the edit history in the file.
	if(oldTypeIsIT_MPT && !newTypeIsIT_MPT && GetFileHistory().size() > 0)
	{
		CHANGEMODTYPE_WARNING(wEditHistory);
	}

	CriticalSection cs;
	m_SndFile.ChangeModTypeTo(nNewType);

	// Song flags
	if(!(CSoundFile::GetModSpecifications(nNewType).songFlags & SONG_LINEARSLIDES) && m_SndFile.m_SongFlags[SONG_LINEARSLIDES])
	{
		CHANGEMODTYPE_WARNING(wLinearSlides);
	}
	if(oldTypeIsXM && newTypeIsIT_MPT) m_SndFile.m_SongFlags.set(SONG_ITCOMPATGXX);
	m_SndFile.m_SongFlags &= (CSoundFile::GetModSpecifications(nNewType).songFlags | SONG_PLAY_FLAGS);

	// Adjust mix levels
	if(newTypeIsMOD || newTypeIsS3M)
	{
		m_SndFile.m_nMixLevels = mixLevels_compatible;
		m_SndFile.m_pConfig->SetMixLevels(mixLevels_compatible);
	}
	if(oldTypeIsMPT && m_SndFile.m_nMixLevels != mixLevels_compatible)
	{
		CHANGEMODTYPE_WARNING(wMixmode);
	}

	// Automatically enable compatible mode when converting from MOD and S3M, since it's automatically enabled in those formats.
	if((nOldType & (MOD_TYPE_MOD | MOD_TYPE_S3M)) && (nNewType & (MOD_TYPE_XM | MOD_TYPE_IT)))
	{
		m_SndFile.SetModFlag(MSF_COMPATIBLE_PLAY, true);
	}
	if((nNewType & (MOD_TYPE_XM | MOD_TYPE_IT)) && !m_SndFile.GetModFlag(MSF_COMPATIBLE_PLAY))
	{
		CHANGEMODTYPE_WARNING(wCompatibilityMode);
	}

	cs.Leave();
	ChangeFileExtension(nNewType);

	// Check mod specifications
	Limit(m_SndFile.m_nDefaultTempo, specs.tempoMin, specs.tempoMax);
	Limit(m_SndFile.m_nDefaultSpeed, specs.speedMin, specs.speedMax);

	for(INSTRUMENTINDEX i = 1; i <= m_SndFile.GetNumInstruments(); i++) if(m_SndFile.Instruments[i] != nullptr)
	{
		UpdateEnvelopes(&(m_SndFile.Instruments[i]->VolEnv), &m_SndFile, warnings);
		UpdateEnvelopes(&(m_SndFile.Instruments[i]->PanEnv), &m_SndFile, warnings);
		UpdateEnvelopes(&(m_SndFile.Instruments[i]->PitchEnv), &m_SndFile, warnings);
	}

	// XM requires instruments, so we create them right away.
	if(newTypeIsXM && GetNumInstruments() == 0)
	{
		ConvertSamplesToInstruments();
	}

	// XM has no global volume
	if(newTypeIsXM && m_SndFile.m_nDefaultGlobalVolume != MAX_GLOBAL_VOLUME)
	{
		if(!GlobalVolumeToPattern())
		{
			CHANGEMODTYPE_WARNING(wGlobalVolumeNotSupported);
		}
	}
		
	// Pattern warnings
	CHAR s[64];
	wsprintf(s, "%d patterns have been resized to 64 rows\n", nResizedPatterns);
	CHANGEMODTYPE_CHECK(wResizedPatterns, s);
	CHANGEMODTYPE_CHECK(wRestartPos, "Restart position is not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wPatternSignatures, "Pattern-specific time signatures are not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wChannelVolSurround, "Channel volume and surround are not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wChannelPanning, "Channel panning is not supported by the new format.\n");

	// Sample warnings
	CHANGEMODTYPE_CHECK(wSampleBidiLoops, "Sample bidi loops are not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wSampleSustainLoops, "New format doesn't support sample sustain loops.\n");
	CHANGEMODTYPE_CHECK(wSampleAutoVibrato, "New format doesn't support sample autovibrato.\n");
	CHANGEMODTYPE_CHECK(wMODSampleFrequency, "Sample C-5 frequencies will be lost.\n");
	CHANGEMODTYPE_CHECK(wMOD31Samples, "Samples above 31 will be lost when saving as MOD. Consider rearranging samples if there are unused slots available.\n");

	// Instrument warnings
	CHANGEMODTYPE_CHECK(wInstrumentsToSamples, "All instruments have been converted to samples.\n");
	CHANGEMODTYPE_CHECK(wTrimmedEnvelopes, "Instrument envelopes have been shortened.\n");
	CHANGEMODTYPE_CHECK(wInstrumentSustainLoops, "Sustain loops were converted to sustain points.\n");
	CHANGEMODTYPE_CHECK(wInstrumentTuning, "Instrument tunings will be lost.\n");
	CHANGEMODTYPE_CHECK(wPitchToTempoLock, "Pitch / Tempo Lock instrument property is not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wBrokenNoteMap, "Instrument Note Mapping is not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wReleaseNode, "Instrument envelope release nodes are not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wFilterVariation, "Random filter variation is not supported by the new format.\n");

	// General warnings
	CHANGEMODTYPE_CHECK(wMODGlobalVars, "Default speed, tempo and global volume will be lost.\n");
	CHANGEMODTYPE_CHECK(wLinearSlides, "Linear Frequency Slides not supported by the new format.\n");
	CHANGEMODTYPE_CHECK(wEditHistory, "Edit history will not be saved in the new format.\n");
	CHANGEMODTYPE_CHECK(wMixmode, "Consider setting the mix levels to \"Compatible\" in the song properties when working with legacy formats.\n");
	CHANGEMODTYPE_CHECK(wCompatibilityMode, "Consider enabling the \"compatible playback\" option in the song properties to increase compatiblity with other players.\n");
	CHANGEMODTYPE_CHECK(wGlobalVolumeNotSupported, "Default global volume is not supported by the new format.\n");

	SetModified();
	GetPatternUndo().ClearUndo();
	UpdateAllViews(NULL, HINT_MODTYPE | HINT_MODGENERAL);
	EndWaitCursor();

	//rewbs.customKeys: update effect key commands
	CInputHandler *ih = CMainFrame::GetMainFrame()->GetInputHandler();
	ih->SetEffectLetters(m_SndFile.GetModSpecifications());
	//end rewbs.customKeys

	return true;
}


#undef CHANGEMODTYPE_WARNING
#undef CHANGEMODTYPE_CHECK
