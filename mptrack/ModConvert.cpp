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
 *           - Fractional tempo
 *           - Song-specific resampling
 *           - Alternative tempo modes (only usable in legacy XM / IT files)
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
 *           - For more info, see e.g. SaveExtendedSongProperties(), SaveExtendedInstrumentProperties()
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Moddoc.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "../tracklib/SampleEdit.h"
#include "../soundlib/modsmp_ctrl.h"
#include "../soundlib/mod_specifications.h"
#include "ModConvert.h"


OPENMPT_NAMESPACE_BEGIN


// Trim envelopes and remove release nodes.
static void UpdateEnvelopes(InstrumentEnvelope &mptEnv, const CModSpecifications &specs, std::bitset<wNumWarnings> &warnings)
{
	// shorten instrument envelope if necessary (for mod conversion)
	const uint8 envMax = specs.envelopePointsMax;

#define TRIMENV(envLen) if(envLen >= envMax) { envLen = envMax - 1; warnings.set(wTrimmedEnvelopes); }

	if(mptEnv.size() > envMax)
	{
		mptEnv.resize(envMax);
		warnings.set(wTrimmedEnvelopes);
	}
	TRIMENV(mptEnv.nLoopStart);
	TRIMENV(mptEnv.nLoopEnd);
	TRIMENV(mptEnv.nSustainStart);
	TRIMENV(mptEnv.nSustainEnd);
	if(mptEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET)
	{
		if(specs.hasReleaseNode)
		{
			TRIMENV(mptEnv.nReleaseNode);
		} else
		{
			mptEnv.nReleaseNode = ENV_RELEASE_NODE_UNSET;
			warnings.set(wReleaseNode);
		}
	}

#undef TRIMENV
}


bool CModDoc::ChangeModType(MODTYPE nNewType)
{
	std::bitset<wNumWarnings> warnings;
	warnings.reset();
	PATTERNINDEX nResizedPatterns = 0;

	const MODTYPE nOldType = m_SndFile.GetType();
	
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

	const CModSpecifications &specs = m_SndFile.GetModSpecifications(nNewType);

	// Check if conversion to 64 rows is necessary
	for(const auto &pat : m_SndFile.Patterns)
	{
		if(pat.IsValid() && pat.GetNumRows() != 64)
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
			warnings.set(wInstrumentsToSamples);
		}

		// Resizing all patterns to 64 rows
		for(auto &pat : m_SndFile.Patterns) if(pat.IsValid() && pat.GetNumRows() != 64)
		{
			ROWINDEX origRows = pat.GetNumRows();
			pat.Resize(64);

			if(origRows < 64)
			{
				// Try to save short patterns by inserting a pattern break.
				pat.WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(origRows - 1).RetryNextRow());
			}

			warnings.set(wResizedPatterns);
		}

		// Removing all instrument headers from channels
		for(auto &chn : m_SndFile.m_PlayState.Chn)
		{
			chn.pModInstrument = nullptr;
		}

		for(INSTRUMENTINDEX nIns = 0; nIns <= m_SndFile.GetNumInstruments(); nIns++)
		{
			delete m_SndFile.Instruments[nIns];
			m_SndFile.Instruments[nIns] = nullptr;
		}
		m_SndFile.m_nInstruments = 0;

		EndWaitCursor();
	} //End if (((m_SndFile.m_nInstruments) || (b64)) && (nNewType & (MOD_TYPE_MOD|MOD_TYPE_S3M)))
	BeginWaitCursor();


	/////////////////////////////
	// Converting pattern data

	// When converting to MOD, get the new sample transpose setting right here so that we can compensate notes in the pattern.
	if(newTypeIsMOD && !oldTypeIsXM)
	{
		for(SAMPLEINDEX smp = 1; smp <= m_SndFile.GetNumSamples(); smp++)
		{
			m_SndFile.GetSample(smp).FrequencyToTranspose();
		}
	}

	bool onlyAmigaNotes = true;
	for(auto &pat : m_SndFile.Patterns) if(pat.IsValid())
	{
		// This is used for -> MOD/XM conversion
		std::vector<std::array<ModCommand::PARAM, MAX_EFFECTS>> effMemory(GetNumChannels());
		std::vector<ModCommand::VOL> volMemory(GetNumChannels(), 0);
		std::vector<ModCommand::INSTR> instrMemory(GetNumChannels(), 0);

		bool addBreak = false;	// When converting to XM, avoid the E60 bug.
		CHANNELINDEX chn = 0;
		ROWINDEX row = 0;

		for(auto m = pat.begin(); m != pat.end(); m++, chn++)
		{
			if(chn >= GetNumChannels())
			{
				chn = 0;
				row++;
			}

			ModCommand::INSTR instr = m->instr;
			if(m->instr) instrMemory[chn] = instr;
			else instr = instrMemory[chn];

			// Deal with volume column slide memory (it's not shared with the effect column)
			if(oldTypeIsIT_MPT && (newTypeIsMOD_XM || newTypeIsS3M))
			{
				switch(m->volcmd)
				{
				case VOLCMD_VOLSLIDEUP:
				case VOLCMD_VOLSLIDEDOWN:
				case VOLCMD_FINEVOLUP:
				case VOLCMD_FINEVOLDOWN:
					if(m->vol == 0)
						m->vol = volMemory[chn];
					else
						volMemory[chn] = m->vol;
					break;
				}
			}

			// Deal with MOD/XM commands without effect memory
			if(oldTypeIsS3M_IT_MPT && newTypeIsMOD_XM)
			{
				switch(m->command)
				{
					// No effect memory in XM / MOD
				case CMD_ARPEGGIO:
				case CMD_S3MCMDEX:
				case CMD_MODCMDEX:

					// These  have effect memory in XM, but it is spread over several commands (for fine and extra-fine slides), so the easiest way to fix this is to just always use the previous value.
				case CMD_PORTAMENTOUP:
				case CMD_PORTAMENTODOWN:
				case CMD_VOLUMESLIDE:
					if(m->param == 0)
						m->param = effMemory[chn][m->command];
					else
						effMemory[chn][m->command] = m->param;
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
						m->param = effMemory[chn][m->command];
					else
						effMemory[chn][m->command] = m->param;
					break;

				}

				// Compensate for loss of transpose information
				if(m->IsNote() && instr && instr <= GetNumSamples())
				{
					const int newNote = m->note + m_SndFile.GetSample(instr).RelativeTone;
					m->note = static_cast<ModCommand::NOTE>(Clamp(newNote, specs.noteMin, specs.noteMax));
				}
				if(!m->IsAmigaNote())
				{
					onlyAmigaNotes = false;
				}
			}

			m->Convert(nOldType, nNewType, m_SndFile);

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
					auto p = m + 1;
					for(CHANNELINDEX c = chn + 1; c < m_SndFile.GetNumChannels(); c++, p++)
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
				auto p = m - 1;
				for(CHANNELINDEX c = 0; c < chn; c++, p--)
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
			pat.WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(pat.GetNumRows() - 1));
		}
	}

	////////////////////////////////////////////////
	// Converting instrument / sample / etc. data


	// Do some sample conversion
	const bool newTypeHasPingPongLoops = !(newTypeIsMOD || newTypeIsS3M);
	for(SAMPLEINDEX smp = 1; smp <= m_SndFile.GetNumSamples(); smp++)
	{
		ModSample &sample = m_SndFile.GetSample(smp);
		GetSampleUndo().PrepareUndo(smp, sundo_none, "Song Conversion");

		// Too many samples? Only 31 samples allowed in MOD format...
		if(newTypeIsMOD && smp > 31 && sample.nLength > 0)
		{
			warnings.set(wMOD31Samples);
		}

		// No auto-vibrato in MOD/S3M
		if((newTypeIsMOD || newTypeIsS3M) && (sample.nVibDepth | sample.nVibRate | sample.nVibSweep) != 0)
		{
			warnings.set(wSampleAutoVibrato);
		}

		// No sustain loops for MOD/S3M/XM
		bool ignoreLoopConversion = false;
		if(newTypeIsMOD_XM || newTypeIsS3M)
		{
			// Sustain loops - convert to normal loops
			if(sample.uFlags[CHN_SUSTAINLOOP])
			{
				warnings.set(wSampleSustainLoops);
				// Prepare conversion to regular loop
				if(!newTypeHasPingPongLoops)
				{
					ignoreLoopConversion = true;
					if(!SampleEdit::ConvertPingPongLoop(sample, m_SndFile, true))
						warnings.set(wSampleBidiLoops);
				}
			}
		}

		// No ping-pong loops in MOD/S3M
		if(!ignoreLoopConversion && !newTypeHasPingPongLoops && sample.HasPingPongLoop())
		{
			if(!SampleEdit::ConvertPingPongLoop(sample, m_SndFile, false))
				warnings.set(wSampleBidiLoops);
		}

		if(newTypeIsMOD && sample.RelativeTone != 0)
		{
			warnings.set(wMODSampleFrequency);
		}

		if(!CSoundFile::SupportsOPL(nNewType) && sample.uFlags[CHN_ADLIB])
		{
			warnings.set(wAdlibInstruments);
		}

		sample.Convert(nOldType, nNewType);
	}

	for(INSTRUMENTINDEX ins = 1; ins <= m_SndFile.GetNumInstruments(); ins++)
	{
		ModInstrument *pIns = m_SndFile.Instruments[ins];
		if(pIns == nullptr)
		{
			continue;
		}

		// Convert IT/MPT to XM (fix instruments)
		if(newTypeIsXM)
		{
			for(size_t i = 0; i < std::size(pIns->NoteMap); i++)
			{
				if (pIns->NoteMap[i] && pIns->NoteMap[i] != (i + 1))
				{
					warnings.set(wBrokenNoteMap);
					break;
				}
			}
			// Convert sustain loops to sustain "points"
			if(pIns->VolEnv.nSustainStart != pIns->VolEnv.nSustainEnd)
			{
				warnings.set(wInstrumentSustainLoops);
			}
			if(pIns->PanEnv.nSustainStart != pIns->PanEnv.nSustainEnd)
			{
				warnings.set(wInstrumentSustainLoops);
			}
		}


		// Convert MPT to anything - remove instrument tunings, Pitch/Tempo Lock, filter variation
		if(oldTypeIsMPT)
		{
			if(pIns->pTuning != nullptr)
			{
				warnings.set(wInstrumentTuning);
			}
			if(pIns->pitchToTempoLock.GetRaw() != 0)
			{
				warnings.set(wPitchToTempoLock);
			}
			if((pIns->nCutSwing | pIns->nResSwing) != 0)
			{
				warnings.set(wFilterVariation);
			}
		}

		pIns->Convert(nOldType, nNewType);
	}

	if(newTypeIsMOD)
	{
		// Not supported in MOD format
		auto firstPat = std::find_if(m_SndFile.Order().cbegin(), m_SndFile.Order().cend(), [this](PATTERNINDEX pat) { return m_SndFile.Patterns.IsValidPat(pat); });
		bool firstPatValid = firstPat != m_SndFile.Order().cend();
		bool lossy = false;

		if(m_SndFile.m_nDefaultSpeed != 6)
		{
			if(firstPatValid)
			{
				m_SndFile.Patterns[*firstPat].WriteEffect(EffectWriter(CMD_SPEED, ModCommand::PARAM(m_SndFile.m_nDefaultSpeed)).RetryNextRow());
			}
			m_SndFile.m_nDefaultSpeed = 6;
			lossy = true;
		}
		if(m_SndFile.m_nDefaultTempo != TEMPO(125, 0))
		{
			if(firstPatValid)
			{
				m_SndFile.Patterns[*firstPat].WriteEffect(EffectWriter(CMD_TEMPO, ModCommand::PARAM(m_SndFile.m_nDefaultTempo.GetInt())).RetryNextRow());
			}
			m_SndFile.m_nDefaultTempo.Set(125);
			lossy = true;
		}
		if(m_SndFile.m_nDefaultGlobalVolume != MAX_GLOBAL_VOLUME || m_SndFile.m_nSamplePreAmp != 48 || m_SndFile.m_nVSTiVolume != 48)
		{
			m_SndFile.m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
			m_SndFile.m_nSamplePreAmp = 48;
			m_SndFile.m_nVSTiVolume = 48;
			lossy = true;
		}
		if(lossy)
		{
			warnings.set(wMODGlobalVars);
		}
	}

	// Is the "restart position" value allowed in this format?
	for(SEQUENCEINDEX seq = 0; seq < m_SndFile.Order.GetNumSequences(); seq++)
	{
		if(m_SndFile.Order(seq).GetRestartPos() > 0 && !specs.hasRestartPos)
		{
			// Try to fix it by placing a pattern jump command in the pattern.
			if(!m_SndFile.Order.RestartPosToPattern(seq))
			{
				// Couldn't fix it! :(
				warnings.set(wRestartPos);
			}
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
				warnings.set(wChannelVolSurround);
			}
		}
		if(newTypeIsXM)
		{
			if(m_SndFile.ChnSettings[nChn].nPan != 128)
			{
				m_SndFile.ChnSettings[nChn].nPan = 128;
				warnings.set(wChannelPanning);
			}
		}
	}

	// Check for patterns with custom time signatures (fixing will be applied in the pattern container)
	if(!specs.hasPatternSignatures)
	{
		for(const auto &pat: m_SndFile.Patterns)
		{
			if(pat.GetOverrideSignature())
			{
				warnings.set(wPatternSignatures);
				break;
			}
		}
	}

	// Check whether the new format supports embedding the edit history in the file.
	if(oldTypeIsIT_MPT && !newTypeIsIT_MPT && GetSoundFile().GetFileHistory().size() > 0)
	{
		warnings.set(wEditHistory);
	}

	if((nOldType & MOD_TYPE_XM) && m_SndFile.m_playBehaviour[kFT2VolumeRamping])
	{
		warnings.set(wVolRamp);
	}

	{
		CriticalSection cs;
		m_SndFile.ChangeModTypeTo(nNewType);
	}

	if(m_SndFile.Order.CanSplitSubsongs() && Reporting::Confirm("The order list contains separator items.\nThe new format supports multiple sequences. Do you want to split the sequence at the separators into multiple song sequences?",
		"Order list conversion", false, true) == cnfYes)
	{
		m_SndFile.Order.SplitSubsongsToMultipleSequences();
	}

	CriticalSection cs;

	// In case we need to update IT bidi loop handling pre-computation or loops got changed...
	m_SndFile.PrecomputeSampleLoops(false);

	// Song flags
	if(!(specs.songFlags & SONG_LINEARSLIDES) && m_SndFile.m_SongFlags[SONG_LINEARSLIDES])
	{
		warnings.set(wLinearSlides);
	}
	if(oldTypeIsXM && newTypeIsIT_MPT)
	{
		m_SndFile.m_SongFlags.set(SONG_ITCOMPATGXX);
	} else if(newTypeIsMOD && GetNumChannels() == 4 && onlyAmigaNotes)
	{
		m_SndFile.m_SongFlags.set(SONG_ISAMIGA);
		m_SndFile.InitAmigaResampler();
	}
	m_SndFile.m_SongFlags &= (specs.songFlags | SONG_PLAY_FLAGS);

	// Adjust mix levels
	if(newTypeIsMOD || newTypeIsS3M)
	{
		m_SndFile.SetMixLevels(MixLevels::Compatible);
	}
	if(oldTypeIsMPT && m_SndFile.GetMixLevels() != MixLevels::Compatible && m_SndFile.GetMixLevels() != MixLevels::CompatibleFT2)
	{
		warnings.set(wMixmode);
	}

	if(!specs.hasFractionalTempo && m_SndFile.m_nDefaultTempo.GetFract() != 0)
	{
		m_SndFile.m_nDefaultTempo.Set(m_SndFile.m_nDefaultTempo.GetInt(), 0);
		warnings.set(wFractionalTempo);
	}

	ChangeFileExtension(nNewType);

	// Check mod specifications
	Limit(m_SndFile.m_nDefaultTempo, specs.GetTempoMin(), specs.GetTempoMax());
	Limit(m_SndFile.m_nDefaultSpeed, specs.speedMin, specs.speedMax);

	for(INSTRUMENTINDEX i = 1; i <= m_SndFile.GetNumInstruments(); i++) if(m_SndFile.Instruments[i] != nullptr)
	{
		UpdateEnvelopes(m_SndFile.Instruments[i]->VolEnv, specs, warnings);
		UpdateEnvelopes(m_SndFile.Instruments[i]->PanEnv, specs, warnings);
		UpdateEnvelopes(m_SndFile.Instruments[i]->PitchEnv, specs, warnings);
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
			warnings.set(wGlobalVolumeNotSupported);
		}
	}

	// Resampling is only saved in MPTM
	if(!newTypeIsMPT && m_SndFile.m_nResampling != SRCMODE_DEFAULT)
	{
		warnings.set(wResamplingMode);
		m_SndFile.m_nResampling = SRCMODE_DEFAULT;
	}

	cs.Leave();

	if(warnings[wResizedPatterns])
	{
		AddToLog(LogInformation, MPT_UFORMAT("{} patterns have been resized to 64 rows")(nResizedPatterns));
	}
	static constexpr struct
	{
		ConversionWarning warning;
		const char *mesage;
	} messages[] =
	{
	// Pattern warnings
	{ wRestartPos, "Restart position is not supported by the new format." },
	{ wPatternSignatures, "Pattern-specific time signatures are not supported by the new format." },
	{ wChannelVolSurround, "Channel volume and surround are not supported by the new format." },
	{ wChannelPanning, "Channel panning is not supported by the new format." },
	// Sample warnings
	{ wSampleBidiLoops, "Sample bidi loops are not supported by the new format." },
	{ wSampleSustainLoops, "New format doesn't support sample sustain loops." },
	{ wSampleAutoVibrato, "New format doesn't support sample autovibrato." },
	{ wMODSampleFrequency, "Sample C-5 frequencies will be lost." },
	{ wMOD31Samples, "Samples above 31 will be lost when saving as MOD. Consider rearranging samples if there are unused slots available." },
	{ wAdlibInstruments, "OPL instruments are not supported by this format." },
	// Instrument warnings
	{ wInstrumentsToSamples, "All instruments have been converted to samples." },
	{ wTrimmedEnvelopes, "Instrument envelopes have been shortened." },
	{ wInstrumentSustainLoops, "Sustain loops were converted to sustain points." },
	{ wInstrumentTuning, "Instrument tunings will be lost." },
	{ wPitchToTempoLock, "Pitch / Tempo Lock instrument property is not supported by the new format." },
	{ wBrokenNoteMap, "Instrument Note Mapping is not supported by the new format." },
	{ wReleaseNode, "Instrument envelope release nodes are not supported by the new format." },
	{ wFilterVariation, "Random filter variation is not supported by the new format." },
	// General warnings
	{ wMODGlobalVars, "Default speed, tempo and global volume will be lost." },
	{ wLinearSlides, "Linear Frequency Slides not supported by the new format." },
	{ wEditHistory, "Edit history will not be saved in the new format." },
	{ wMixmode, "Consider setting the mix levels to \"Compatible\" in the song properties when working with legacy formats." },
	{ wVolRamp, "Fasttracker 2 compatible super soft volume ramping gets lost when converting XM files to another type." },
	{ wGlobalVolumeNotSupported, "Default global volume is not supported by the new format." },
	{ wResamplingMode, "Song-specific resampling mode is not supported by the new format." },
	{ wFractionalTempo, "Fractional tempo is not supported by the new format." },
	};
	for(const auto &msg : messages)
	{
		if(warnings[msg.warning])
			AddToLog(LogInformation, mpt::ToUnicode(mpt::Charset::UTF8, msg.mesage));
	}

	SetModified();
	GetPatternUndo().ClearUndo();
	UpdateAllViews(nullptr, GeneralHint().General().ModType());
	EndWaitCursor();

	// Update effect key commands
	CMainFrame::GetInputHandler()->SetEffectLetters(m_SndFile.GetModSpecifications());

	return true;
}


OPENMPT_NAMESPACE_END
