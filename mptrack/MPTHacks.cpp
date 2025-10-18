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


// Find and fix envelopes where two nodes are on the same tick.
static bool FindIncompatibleEnvelopes(InstrumentEnvelope &env, bool autofix)
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
{
	const CModSpecifications *originalSpecs = &m_SndFile.GetModSpecifications();
	// retrieve original (not hacked) specs.
	MODTYPE modType = m_SndFile.GetBestSaveFormat();
	switch(modType)
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
	case MOD_TYPE_MPT:
		break;
	default:
		MPT_ASSERT_NOTREACHED();
		break;
	}

	bool foundHacks = false, foundHere = false;
	ClearLog();

	// Check for plugins
	foundHere = false;
	for(const auto &plug : m_SndFile.m_MixPlugins)
	{
		if(plug.IsValidPlugin())
		{
			foundHere = foundHacks = true;
			break;
		}
		// REQUIRES AUTOFIX
	}
	if(foundHere)
		AddToLog("Found plugins");

	// Check for invalid order items
	if(!originalSpecs->hasIgnoreIndex && mpt::contains(m_SndFile.Order(), PATTERNINDEX_SKIP))
	{
		foundHacks = true;
		AddToLog("This format does not support separator (+++) patterns");

		if(autofix)
		{
			m_SndFile.Order().RemovePattern(PATTERNINDEX_SKIP);
		}
	}

	if(!originalSpecs->hasStopIndex && m_SndFile.Order().GetLengthFirstEmpty() != m_SndFile.Order().GetLengthTailTrimmed())
	{
		foundHacks = true;
		AddToLog("The pattern sequence should end after the first stop (---) index in this format.");

		if(autofix)
		{
			m_SndFile.Order().RemovePattern(PATTERNINDEX_INVALID);
		}
	}

	// Global volume
	if(modType == MOD_TYPE_XM && m_SndFile.m_nDefaultGlobalVolume != MAX_GLOBAL_VOLUME)
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
		AddToLog(MPT_AFORMAT("Found too many patterns ({} allowed)")(originalSpecs->patternsMax));
		foundHacks = true;
		// REQUIRES (INTELLIGENT) AUTOFIX
	}

	// Check for too big/small patterns
	foundHere = false;
	for(auto &pat : m_SndFile.Patterns)
	{
		if(pat.IsValid())
		{
			const ROWINDEX patSize = pat.GetNumRows();
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
					pat.Resize(originalSpecs->patternRowsMin);
					pat.WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(patSize - 1).RetryNextRow());
				} else
				{
					break;
				}
			}
		}
	}
	if(foundHere)
	{
		AddToLog(MPT_AFORMAT("Found incompatible pattern lengths (must be between {} and {} rows)")(originalSpecs->patternRowsMin, originalSpecs->patternRowsMax));
	}

	// Check for invalid pattern commands
	foundHere = false;
	m_SndFile.Patterns.ForEachModCommand([originalSpecs, &foundHere, autofix, modType] (ModCommand &m)
	{
		// definitely not perfect yet. :)
		// Probably missing: Some extended effect parameters
		if(!originalSpecs->HasNote(m.note))
		{
			foundHere = true;
			if(autofix)
				m.note = NOTE_NONE;
		}

		if(!originalSpecs->HasCommand(m.command))
		{
			foundHere = true;
			if(autofix)
				m.command = CMD_NONE;
		}

		if(!originalSpecs->HasVolCommand(m.volcmd))
		{
			foundHere = true;
			if(autofix)
				m.volcmd = VOLCMD_NONE;
		}

		if(modType == MOD_TYPE_XM)		// ModPlug XM extensions
		{
			if(m.command == CMD_XFINEPORTAUPDOWN && m.param >= 0x30)
			{
				foundHere = true;
				if(autofix)
					m.command = CMD_NONE;
			}
		} else if(modType == MOD_TYPE_IT)		// ModPlug IT extensions
		{
			if((m.command == CMD_S3MCMDEX) && ((m.param & 0xF0) == 0x90) && (m.param != 0x91))
			{
				foundHere = true;
				if(autofix)
					m.command = CMD_NONE;
			}
		}
	});
	if(foundHere)
	{
		AddToLog("Found invalid pattern commands");
		foundHacks = true;
	}

	// Check for pattern names
	const PATTERNINDEX numNamedPatterns = m_SndFile.Patterns.GetNumNamedPatterns();
	if(numNamedPatterns > 0 && !originalSpecs->hasPatternNames)
	{
		AddToLog("Found pattern names");
		foundHacks = true;
		if(autofix)
		{
			for(PATTERNINDEX i = 0; i < numNamedPatterns; i++)
			{
				m_SndFile.Patterns[i].SetName("");
			}
		}
	}

	// Check for too many channels
	if(m_SndFile.GetNumChannels() > originalSpecs->channelsMax || m_SndFile.GetNumChannels() < originalSpecs->channelsMin)
	{
		AddToLog(MPT_AFORMAT("Found incompatible channel count (must be between {} and {} channels)")(originalSpecs->channelsMin, originalSpecs->channelsMax));
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
		if(!m_SndFile.ChnSettings[i].szName.empty())
		{
			foundHere = foundHacks = true;
			if(autofix)
				m_SndFile.ChnSettings[i].szName = "";
			else
				break;
		}
	}
	if(foundHere)
		AddToLog("Found channel names");

	// Check for too many samples
	if(m_SndFile.GetNumSamples() > originalSpecs->samplesMax)
	{
		AddToLog(MPT_AFORMAT("Found too many samples ({} allowed)")(originalSpecs->samplesMax));
		foundHacks = true;
		// REQUIRES (INTELLIGENT) AUTOFIX
	}

	// Check for sample extensions
	foundHere = false;
	bool opl3inS3M = false;
	for(SAMPLEINDEX i = 1; i <= m_SndFile.GetNumSamples(); i++)
	{
		ModSample &smp = m_SndFile.GetSample(i);
		if(modType == MOD_TYPE_XM && smp.GetNumChannels() > 1)
		{
			foundHere = foundHacks = true;
			if(autofix)
				ctrlSmp::ConvertToMono(smp, m_SndFile, ctrlSmp::mixChannels);
		} else if(modType == MOD_TYPE_S3M && smp.uFlags[CHN_ADLIB])
		{
			if(smp.adlib[8] >= 4)
			{
				opl3inS3M = foundHacks = true;
				if(autofix)
					smp.adlib[8] = 0;
			}
			if(smp.adlib[9] >= 4)
			{
				opl3inS3M = foundHacks = true;
				if(autofix)
					smp.adlib[9] = 0;
			}
		}
	}
	if(foundHere)
		AddToLog("Stereo samples are not supported in the original XM format");
	if(opl3inS3M)
		AddToLog("Extended OPL3 waveforms should not be used in the S3M format");

	// Check for too many instruments
	if(m_SndFile.GetNumInstruments() > originalSpecs->instrumentsMax)
	{
		AddToLog(MPT_AFORMAT("Found too many instruments ({} allowed)")(originalSpecs->instrumentsMax));
		foundHacks = true;
		// REQUIRES (INTELLIGENT) AUTOFIX
	}

	// Check for instrument extensions
	foundHere = false;
	bool foundEnvelopes = false;
	INSTRUMENTINDEX instrWithTooManySmps = 0;
	for(INSTRUMENTINDEX i = 1; i <= m_SndFile.GetNumInstruments(); i++)
	{
		ModInstrument *instr = m_SndFile.Instruments[i];
		if(instr == nullptr) continue;

		// Extended instrument attributes
		if(instr->filterMode != FilterMode::Unchanged || instr->nVolRampUp != 0 || instr->resampling != SRCMODE_DEFAULT ||
			instr->nCutSwing != 0 || instr->nResSwing != 0 || instr->nMixPlug != 0 || instr->pitchToTempoLock.GetRaw() != 0 ||
			instr->nDCT == DuplicateCheckType::Plugin ||
			instr->VolEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET ||
			instr->PanEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET ||
			instr->PitchEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET
		)
		{
			foundHere = foundHacks = true;
			if(autofix)
			{
				instr->filterMode = FilterMode::Unchanged;
				instr->nVolRampUp = 0;
				instr->resampling = SRCMODE_DEFAULT;
				instr->nCutSwing = 0;
				instr->nResSwing = 0;
				instr->nMixPlug = 0;
				instr->pitchToTempoLock.Set(0);
				if(instr->nDCT == DuplicateCheckType::Plugin) instr->nDCT = DuplicateCheckType::None;
				instr->VolEnv.nReleaseNode = instr->PanEnv.nReleaseNode = instr->PitchEnv.nReleaseNode = ENV_RELEASE_NODE_UNSET;
			}
		}
		if((modType & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (instr->nFadeOut % 32u) != 0)
		{
			foundHere = foundHacks = true;
			if(autofix)
				instr->nFadeOut = ((instr->nFadeOut + 16) / 32) * 32;
		}

		if(modType == MOD_TYPE_XM && instr->GetSamples().size() > 16)
			instrWithTooManySmps++;

		// Incompatible envelope shape
		foundEnvelopes |= FindIncompatibleEnvelopes(instr->VolEnv, autofix);
		foundEnvelopes |= FindIncompatibleEnvelopes(instr->PanEnv, autofix);
		foundEnvelopes |= FindIncompatibleEnvelopes(instr->PitchEnv, autofix);
		foundHacks |= foundEnvelopes;
	}
	if(foundHere)
		AddToLog("Found MPT instrument extensions");
	if(foundEnvelopes)
		AddToLog("Two envelope points may not share the same tick");
	if(instrWithTooManySmps)
		AddToLog(MPT_AFORMAT("{} instruments use too many samples (16 allowed)")(instrWithTooManySmps));

	for(auto &order : m_SndFile.Order)
	{
		// Check for too many orders
		if(order.GetLengthTailTrimmed() > originalSpecs->ordersMax)
		{
			AddToLog(MPT_AFORMAT("Found too many orders ({} allowed)")(originalSpecs->ordersMax));
			foundHacks = true;
			if(autofix)
			{
				// Can we be more intelligent here and maybe remove stop patterns and such?
				order.resize(originalSpecs->ordersMax);
			}
		}

		// Check for invalid default tempo
		if(order.GetDefaultTempo() > originalSpecs->GetTempoMax() || order.GetDefaultTempo() < originalSpecs->GetTempoMin())
		{
			AddToLog(MPT_AFORMAT("Found incompatible default tempo (must be between {} and {})")(originalSpecs->GetTempoMin().GetInt(), originalSpecs->GetTempoMax().GetInt()));
			foundHacks = true;
			if(autofix)
				order.SetDefaultTempo(Clamp(order.GetDefaultTempo(), originalSpecs->GetTempoMin(), originalSpecs->GetTempoMax()));
		}

		// Check for invalid default speed
		if(order.GetDefaultSpeed() > originalSpecs->speedMax || order.GetDefaultSpeed() < originalSpecs->speedMin)
		{
			AddToLog(MPT_AFORMAT("Found incompatible default speed (must be between {} and {})")(originalSpecs->speedMin, originalSpecs->speedMax));
			foundHacks = true;
			if(autofix)
				order.SetDefaultSpeed(Clamp(order.GetDefaultSpeed(), originalSpecs->speedMin, originalSpecs->speedMax));
		}
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
		for(auto &pat : m_SndFile.Patterns)
		{
			if(pat.GetOverrideSignature())
			{
				if(!foundHere)
					AddToLog("Found pattern-specific time signatures");

				if(autofix)
					pat.RemoveSignature();

				foundHacks = foundHere = true;
				if(!autofix)
					break;
			}
		}
	}

	// Check for new tempo modes
	if(m_SndFile.m_nTempoMode != TempoMode::Classic)
	{
		AddToLog("Found incompatible tempo mode (only classic tempo mode allowed)");
		foundHacks = true;
		if(autofix)
			m_SndFile.m_nTempoMode = TempoMode::Classic;
	}

	// Check for extended filter range flag
	if(m_SndFile.m_SongFlags[SONG_EXFILTERRANGE])
	{
		AddToLog("Found extended filter range");
		foundHacks = true;
		if(autofix)
			m_SndFile.m_SongFlags.reset(SONG_EXFILTERRANGE);
	}

	// Player flags
	if((modType & (MOD_TYPE_XM|MOD_TYPE_IT)) && m_SndFile.m_playBehaviour != m_SndFile.GetDefaultPlaybackBehaviour(modType))
	{
		AddToLog("Some playback compatibility settings are not at their defaults");
		foundHacks = true;
		if(autofix)
			m_SndFile.SetDefaultPlaybackBehaviour(modType);
	}

	// Check for restart position where it should not be
	for(SEQUENCEINDEX seq = 0; seq < m_SndFile.Order.GetNumSequences(); seq++)
	{
		if(m_SndFile.Order(seq).GetRestartPos() > 0 && !originalSpecs->hasRestartPos)
		{
			AddToLog("Found restart position");
			foundHacks = true;
			if(autofix)
			{
				m_SndFile.Order.WriteGlobalsToPattern(seq, true, false);
			}
		}
	}

	if(!originalSpecs->hasArtistName && !m_SndFile.m_songArtist.empty() && !(modType & (MOD_TYPE_MOD | MOD_TYPE_S3M)))
	{
		AddToLog("Found artist name");
		foundHacks = true;
		if(autofix)
		{
			m_SndFile.m_songArtist.clear();
		}
	}

	if(m_SndFile.GetMixLevels() != MixLevels::Compatible && m_SndFile.GetMixLevels() != MixLevels::CompatibleFT2)
	{
		AddToLog("Found incorrect mix levels (only compatible mix levels allowed)");
		foundHacks = true;
		if(autofix)
			m_SndFile.SetMixLevels(modType == MOD_TYPE_XM ? MixLevels::CompatibleFT2 : MixLevels::Compatible);
	}

	// Check for extended MIDI macros
	if(modType == MOD_TYPE_IT)
	{
		for(const auto &macro : m_SndFile.m_MidiCfg)
		{
			for(const auto c : std::string_view{macro})
			{
				if(c == 's')
				{
					foundHacks = true;
					AddToLog("Found SysEx checksum variable in MIDI macro");
					break;
				}
			}
		}
	}

	if(autofix && foundHacks)
		SetModified();

	return foundHacks;
}


OPENMPT_NAMESPACE_END
