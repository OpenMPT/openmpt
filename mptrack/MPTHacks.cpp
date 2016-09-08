/*
 * MPTHacks.cpp
 * ------------
 * Purpose: Find out if MOD/XM/S3M/IT modules have MPT-specific hacks and fix them.
 * Notes  : This is not finished yet. Still need to handle:
 *          - Out-of-range sample pre-amp settings
 *          - Comments in XM files
 *          - Many auto-fix actions (so that the auto-fix mode can actually be used at some point!)
 *          Maybe there should be two options if hacks are found: Convert the song to MPTM or remove hacks.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Moddoc.h"
#include "../soundlib/modsmp_ctrl.h"
#include "../soundlib/mod_specifications.h"


OPENMPT_NAMESPACE_BEGIN


// Functor for fixing hacked patterns
struct FixHackedPatterns
//======================
{
	FixHackedPatterns(const CModSpecifications *originalSpecs, MODTYPE type, bool autofix, bool *foundHacks)
	{
		this->originalSpecs = originalSpecs;
		this->type = type;
		this->autofix = autofix;
		this->foundHacks = foundHacks;
		*foundHacks = false;
	}

	void operator()(ModCommand& m)
	{
		// definitely not perfect yet. :)
		// Probably missing: Some extended effect parameters
		if(!originalSpecs->HasNote(m.note))
		{
			*foundHacks = true;
			if(autofix)
				m.note = NOTE_NONE;
		}

		if(!originalSpecs->HasCommand(m.command))
		{
			*foundHacks = true;
			if(autofix)
				m.command = CMD_NONE;
		}

		if(!originalSpecs->HasVolCommand(m.volcmd))
		{
			*foundHacks = true;
			if(autofix)
				m.volcmd = VOLCMD_NONE;
		}

		if(type == MOD_TYPE_XM)		// ModPlug XM extensions
		{
			if(m.command == CMD_XFINEPORTAUPDOWN && m.param >= 0x30)
			{
				*foundHacks = true;
				if(autofix)
					m.command = CMD_NONE;
			}
		} else if(type == MOD_TYPE_IT)		// ModPlug IT extensions
		{
			if((m.command == CMD_S3MCMDEX) && ((m.param & 0xF0) == 0x90) && (m.param != 0x91))
			{
				*foundHacks = true;
				if(autofix)
					m.command = CMD_NONE;
			}

		}
	}
	
	const CModSpecifications *originalSpecs;
	MODTYPE type;
	bool autofix;
	bool *foundHacks;
};


// Find and fix envelopes where two nodes are on the same tick.
bool FindIncompatibleEnvelopes(InstrumentEnvelope &env, bool autofix)
//-------------------------------------------------------------------
{
	bool found = false;
	for(uint32 i = 1; i < env.size(); i++)
	{
		if(env[i].tick <= env[i - 1].tick)	// "<=" so we can fix envelopes "on the fly"
		{
			found = true;
			if(autofix)
			{
				env[i].tick = env[i - 1].tick + 1;
			}
		}
	}
	return found;
}


// Go through the module to find out if it contains any hacks introduced by (Open)MPT
bool CModDoc::HasMPTHacks(const bool autofix)
//-------------------------------------------
{
	const CModSpecifications *originalSpecs = &m_SndFile.GetModSpecifications();
	// retrieve original (not hacked) specs.
	switch(m_SndFile.GetBestSaveFormat())
	{
	case MOD_TYPE_MOD:
		originalSpecs = &ModSpecs::mod;
		break;
	case MOD_TYPE_XM:
		originalSpecs = &ModSpecs::xm;
		break;
	case MOD_TYPE_S3M:
		originalSpecs = &ModSpecs::s3m;
		break;
	case MOD_TYPE_IT:
		originalSpecs = &ModSpecs::it;
		break;
	}

	bool foundHacks = false, foundHere = false;
	ClearLog();

	// Check for plugins
#ifndef NO_PLUGINS
	foundHere = false;
	for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
	{
		if(m_SndFile.m_MixPlugins[i].IsValidPlugin())
		{
			foundHere = foundHacks = true;
			break;
		}
		// REQUIRES AUTOFIX
	}
	if(foundHere)
		AddToLog("Found plugins");
#endif // NO_PLUGINS

	// Check for invalid order items
	for(ORDERINDEX i = m_SndFile.Order.GetLengthTailTrimmed(); i > 0; i--)
	{
		if(m_SndFile.Order[i - 1] == m_SndFile.Order.GetIgnoreIndex() && !originalSpecs->hasIgnoreIndex)
		{
			foundHacks = true;
			AddToLog("This format does not support separator (+++) patterns");

			if(autofix)
			{
				m_SndFile.Order.RemovePattern(m_SndFile.Order.GetIgnoreIndex());
			}

			break;
		}
	}

	if(!originalSpecs->hasStopIndex && m_SndFile.Order.GetLengthFirstEmpty() != m_SndFile.Order.GetLengthTailTrimmed())
	{
		foundHacks = true;
		AddToLog("The pattern sequence should end after the first stop (---) index in this format.");

		if(autofix)
		{
			m_SndFile.Order.RemovePattern(m_SndFile.Order.GetInvalidPatIndex());
		}
	}

	// Global volume
	if(m_SndFile.GetType() == MOD_TYPE_XM && m_SndFile.m_nDefaultGlobalVolume != MAX_GLOBAL_VOLUME)
	{
		foundHacks = true;
		AddToLog("XM format does not support default global volume");
		if(autofix)
		{
			GlobalVolumeToPattern();
		}
	}

	// Pattern count
	if(m_SndFile.Patterns.GetNumPatterns() > originalSpecs->patternsMax)
	{
		AddToLog(mpt::String::Print("Found too many patterns (%1 allowed)", originalSpecs->patternsMax));
		foundHacks = true;
		// REQUIRES (INTELLIGENT) AUTOFIX
	}

	// Check for too big/small patterns
	foundHere = false;
	for(PATTERNINDEX i = 0; i < m_SndFile.Patterns.Size(); i++)
	{
		if(m_SndFile.Patterns.IsValidPat(i))
		{
			const ROWINDEX patSize = m_SndFile.Patterns[i].GetNumRows();
			if(patSize > originalSpecs->patternRowsMax)
			{
				foundHacks = foundHere = true;
				if(autofix)
				{
					// REQUIRES (INTELLIGENT) AUTOFIX
				} else
				{
					break;
				}
			} else if(patSize < originalSpecs->patternRowsMin)
			{
				foundHacks = foundHere = true;
				if(autofix)
				{
					m_SndFile.Patterns[i].Resize(originalSpecs->patternRowsMin);
					m_SndFile.Patterns[i].WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(patSize - 1).Retry(EffectWriter::rmTryNextRow));
				} else
				{
					break;
				}
			}
		}
	}
	if(foundHere)
	{
		AddToLog(mpt::String::Print("Found incompatible pattern lengths (must be between %1 and %2 rows)", originalSpecs->patternRowsMin, originalSpecs->patternRowsMax));
	}

	// Check for invalid pattern commands
	foundHere = false;
	m_SndFile.Patterns.ForEachModCommand(FixHackedPatterns(originalSpecs, m_SndFile.GetType(), autofix, &foundHere));
	if(foundHere)
	{
		AddToLog("Found invalid pattern commands");
		foundHacks = true;
	}

	// Check for pattern names
	if(m_SndFile.Patterns.GetNumNamedPatterns() > 0 && !originalSpecs->hasPatternNames)
	{
		AddToLog("Found pattern names");
		foundHacks = true;
		if(autofix)
		{
			const PATTERNINDEX numPats = m_SndFile.Patterns.GetNumPatterns();
			for(PATTERNINDEX i = 0; i < numPats; i++)
			{
				m_SndFile.Patterns[i].SetName("");
			}
		}
	}

	// Check for too many channels
	if(m_SndFile.GetNumChannels() > originalSpecs->channelsMax || m_SndFile.GetNumChannels() < originalSpecs->channelsMin)
	{
		AddToLog(mpt::String::Print("Found incompatible channel count (must be between %1 and %2 channels)", originalSpecs->channelsMin, originalSpecs->channelsMax));
		foundHacks = true;
		if(autofix)
		{
			std::vector<bool> usedChannels;
			CheckUsedChannels(usedChannels);
			RemoveChannels(usedChannels);
			// REQUIRES (INTELLIGENT) AUTOFIX
		}
	}

	// Check for channel names
	foundHere = false;
	for(CHANNELINDEX i = 0; i < m_SndFile.GetNumChannels(); i++)
	{
		if(strcmp(m_SndFile.ChnSettings[i].szName, "") != 0)
		{
			foundHere = foundHacks = true;
			if(autofix)
				MemsetZero(m_SndFile.ChnSettings[i].szName);
			else
				break;
		}
	}
	if(foundHere)
		AddToLog("Found channel names");

	// Check for too many samples
	if(m_SndFile.GetNumSamples() > originalSpecs->samplesMax)
	{
		AddToLog(mpt::String::Print("Found too many samples (%1 allowed)", originalSpecs->samplesMax));
		foundHacks = true;
		// REQUIRES (INTELLIGENT) AUTOFIX
	}

	// Check for sample extensions
	foundHere = false;
	for(SAMPLEINDEX i = 1; i <= m_SndFile.GetNumSamples(); i++)
	{
		ModSample &smp = m_SndFile.GetSample(i);
		if(m_SndFile.GetType() == MOD_TYPE_XM && smp.GetNumChannels() > 1)
		{
			foundHere = foundHacks = true;
			if(autofix)
			{
				ctrlSmp::ConvertToMono(smp, m_SndFile, ctrlSmp::mixChannels);
			} else
			{
				break;
			}
		}
	}
	if(foundHere)
		AddToLog("Stereo samples are not supported in the original XM format");

	// Check for too many instruments
	if(m_SndFile.GetNumInstruments() > originalSpecs->instrumentsMax)
	{
		AddToLog(mpt::String::Print("Found too many instruments (%1 allowed)", originalSpecs->instrumentsMax));
		foundHacks = true;
		// REQUIRES (INTELLIGENT) AUTOFIX
	}

	// Check for instrument extensions
	foundHere = false;
	bool foundEnvelopes = false;
	for(INSTRUMENTINDEX i = 1; i <= m_SndFile.GetNumInstruments(); i++)
	{
		ModInstrument *instr = m_SndFile.Instruments[i];
		if(instr == nullptr) continue;

		// Extended instrument attributes
		if(instr->nFilterMode != FLTMODE_UNCHANGED || instr->nVolRampUp != 0 || instr->nResampling != SRCMODE_DEFAULT ||
			instr->nCutSwing != 0 || instr->nResSwing != 0 || instr->nMixPlug != 0 || instr->pitchToTempoLock.GetRaw() != 0 ||
			instr->nDCT == DCT_PLUGIN ||
			instr->VolEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET ||
			instr->PanEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET ||
			instr->PitchEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET
		)
		{
			foundHere = foundHacks = true;
			if(autofix)
			{
				instr->nFilterMode = FLTMODE_UNCHANGED;
				instr->nVolRampUp = 0;
				instr->nResampling = SRCMODE_DEFAULT;
				instr->nCutSwing = 0;
				instr->nResSwing = 0;
				instr->nMixPlug = 0;
				instr->pitchToTempoLock.Set(0);
				if(instr->nDCT == DCT_PLUGIN) instr->nDCT = DCT_NONE;
				instr->VolEnv.nReleaseNode = instr->PanEnv.nReleaseNode = instr->PitchEnv.nReleaseNode = ENV_RELEASE_NODE_UNSET;
			}
		}
		// Incompatible envelope shape
		foundEnvelopes |= FindIncompatibleEnvelopes(instr->VolEnv, autofix);
		foundEnvelopes |= FindIncompatibleEnvelopes(instr->PanEnv, autofix);
		foundEnvelopes |= FindIncompatibleEnvelopes(instr->PitchEnv, autofix);
		foundHacks |= foundEnvelopes;
	}
	if(foundHere)
		AddToLog("Found MPT instrument extensions");
	if(foundEnvelopes)
		AddToLog("Two envelope points may not share the same tick.");

	// Check for too many orders
	if(m_SndFile.Order.GetLengthTailTrimmed() > originalSpecs->ordersMax)
	{
		AddToLog(mpt::String::Print("Found too many orders (%1 allowed)", originalSpecs->ordersMax));
		foundHacks = true;
		// REQUIRES (INTELLIGENT) AUTOFIX
	}

	// Check for invalid default tempo
	if(m_SndFile.m_nDefaultTempo > originalSpecs->GetTempoMax() || m_SndFile.m_nDefaultTempo < originalSpecs->GetTempoMin())
	{
		AddToLog(mpt::String::Print("Found incompatible default tempo (must be between %1 and %2)", originalSpecs->GetTempoMin().GetInt(), originalSpecs->GetTempoMax().GetInt()));
		foundHacks = true;
		if(autofix)
			m_SndFile.m_nDefaultTempo = Clamp(m_SndFile.m_nDefaultTempo, originalSpecs->GetTempoMin(), originalSpecs->GetTempoMax());
	}

	// Check for invalid default speed
	if(m_SndFile.m_nDefaultSpeed > originalSpecs->speedMax || m_SndFile.m_nDefaultSpeed < originalSpecs->speedMin)
	{
		AddToLog(mpt::String::Print("Found incompatible default speed (must be between %1 and %2)", originalSpecs->speedMin, originalSpecs->speedMax));
		foundHacks = true;
		if(autofix)
			m_SndFile.m_nDefaultSpeed = Clamp(m_SndFile.m_nDefaultSpeed, originalSpecs->speedMin, originalSpecs->speedMax);
	}

	// Check for invalid rows per beat / measure values
	if(m_SndFile.m_nDefaultRowsPerBeat >= originalSpecs->patternRowsMax || m_SndFile.m_nDefaultRowsPerMeasure >= originalSpecs->patternRowsMax)
	{
		AddToLog("Found incompatible rows per beat / measure");
		foundHacks = true;
		if(autofix)
		{
			m_SndFile.m_nDefaultRowsPerBeat = Clamp(m_SndFile.m_nDefaultRowsPerBeat, 1u, (originalSpecs->patternRowsMax - 1));
			m_SndFile.m_nDefaultRowsPerMeasure = Clamp(m_SndFile.m_nDefaultRowsPerMeasure, m_SndFile.m_nDefaultRowsPerBeat, (originalSpecs->patternRowsMax - 1));
		}
	}

	// Find pattern-specific time signatures
	if(!originalSpecs->hasPatternSignatures)
	{
		foundHere = false;
		const PATTERNINDEX numPats = m_SndFile.Patterns.GetNumPatterns();
		for(PATTERNINDEX i = 0; i < numPats; i++)
		{
			if(m_SndFile.Patterns[i].GetOverrideSignature())
			{
				if(!foundHere)
					AddToLog("Found pattern-specific time signatures");

				if(autofix)
					m_SndFile.Patterns[i].RemoveSignature();

				foundHacks = foundHere = true;
				if(!autofix)
					break;
			}
		}
	}

	// Check for new tempo modes
	if(m_SndFile.m_nTempoMode != tempoModeClassic)
	{
		AddToLog("Found incompatible tempo mode (only classic tempo mode allowed)");
		foundHacks = true;
		if(autofix)
			m_SndFile.m_nTempoMode = tempoModeClassic;
	}

	// Check for extended filter range flag
	if(m_SndFile.m_SongFlags[SONG_EXFILTERRANGE])
	{
		AddToLog("Found extended filter range");
		foundHacks = true;
		if(autofix)
			m_SndFile.m_SongFlags.reset(SONG_EXFILTERRANGE);
	}

	// Embedded MIDI configuration in XM files
	if(m_SndFile.m_SongFlags[SONG_EMBEDMIDICFG] && !(originalSpecs->songFlags & SONG_EMBEDMIDICFG))
	{
		AddToLog("Found embedded MIDI macros");
		foundHacks = true;
		if(autofix)
		{
			m_SndFile.m_MidiCfg.Reset();
			m_SndFile.m_SongFlags.reset(SONG_EMBEDMIDICFG);
		}
	}

	// Player flags
	if((m_SndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_IT)) && !m_SndFile.m_playBehaviour[MSF_COMPATIBLE_PLAY])
	{
		AddToLog("Compatible play is deactivated");
		foundHacks = true;
		if(autofix)
			m_SndFile.SetDefaultPlaybackBehaviour(m_SndFile.GetType());
	}

	// Check for restart position where it should not be
	for(SEQUENCEINDEX seq = 0; seq < m_SndFile.Order.GetNumSequences(); seq++)
	{
		if(m_SndFile.Order.GetSequence(0).GetRestartPos() > 0 && !originalSpecs->hasRestartPos)
		{
			AddToLog("Found restart position");
			foundHacks = true;
			if(autofix)
			{
				m_SndFile.Order.RestartPosToPattern(seq);
			}
		}
	}

	if(!originalSpecs->hasArtistName && !m_SndFile.m_songArtist.empty())
	{
		AddToLog("Found artist name");
		foundHacks = true;
		if(autofix)
		{
			m_SndFile.m_songArtist.clear();
		}
	}

	if(m_SndFile.GetMixLevels() != mixLevelsCompatible && m_SndFile.GetMixLevels() != mixLevelsCompatibleFT2)
	{
		AddToLog("Found incorrect mix levels (only compatible mix levels allowed)");
		foundHacks = true;
		if(autofix)
			m_SndFile.SetMixLevels(m_SndFile.GetType() == MOD_TYPE_XM ? mixLevelsCompatibleFT2 : mixLevelsCompatible);
	}

	if(autofix && foundHacks)
		SetModified();

	return foundHacks;
}


OPENMPT_NAMESPACE_END
