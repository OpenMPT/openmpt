/*
 * MPTHacks.cpp
 * ------------
 * Purpose: Find out if MOD/XM/S3M/IT modules have MPT-specific hacks and fix them.
 * Notes  : This is not finished yet.
 * Authors: OpenMPT Devs
 */

#include "stdafx.h"
#include "Moddoc.h"

/* TODO:
stereo/16bit samples (only XM? maybe S3M? many new IT trackers support stereo samples)
out-of range sample pre-amp
song flags and properties (just look at the song properties window)
+++/--- orders in XM/MOD sequence
comments in XM files
AUTOFIX actions!

maybe two options in program: convert to mptm / remove hacks from file!
*/

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

	void operator()(MODCOMMAND& m)
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

		if(type == MOD_TYPE_XM)		// modplug XM extensions
		{
			if(m.command == CMD_XFINEPORTAUPDOWN && m.param >= 0x30)
			{
				*foundHacks = true;
				if(autofix)
					m.command = CMD_NONE;
			}
		} else if(type == MOD_TYPE_IT)		// modplug IT extensions
		{
			if((m.command == CMD_S3MCMDEX) && ((m.param >> 4) == 0x09) && (m.param != 0x91))
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

// Go through the module to find out if it contains any hacks introduced by (Open)MPT
bool CModDoc::HasMPTHacks(bool autofix)
//-------------------------------------
{
	const CModSpecifications *originalSpecs = &m_SndFile.GetModSpecifications();
	// retrieve original (not hacked) specs.
	switch(m_SndFile.GetType())
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
	foundHere = false;
	for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
	{
		if(m_SndFile.m_MixPlugins[i].pMixPlugin != nullptr || m_SndFile.m_MixPlugins[i].Info.dwPluginId1 || m_SndFile.m_MixPlugins[i].Info.dwPluginId2)
		{
			foundHere = foundHacks = true;
			break;
		}
		// REQUIRES AUTOFIX
	}
	if(foundHere)
		AddToLog("Found VST plugins\n");

	// Pattern count
	if(m_SndFile.GetNumPatterns() > originalSpecs->patternsMax)
	{
		AddToLog("Found too many patterns\n");
		foundHacks = true;
		// REQUIRES AUTOFIX
	}

	// Check for too big/small patterns
	foundHere = false;
	for(PATTERNINDEX i = 0; i < m_SndFile.Patterns.Size(); i++)
	{
		if(m_SndFile.Patterns.IsValidPat(i))
		{
			if(m_SndFile.Patterns[i].GetNumRows() > originalSpecs->patternRowsMax || m_SndFile.Patterns[i].GetNumRows() < originalSpecs->patternRowsMin)
			{
				foundHacks = foundHere = true;
				break;
			}
			// REQUIRES AUTOFIX
		}
	}
	if(foundHere)
		AddToLog("Found incompatible pattern lengths\n");

	// Check for invalid pattern commands
	foundHere = false;
	m_SndFile.Patterns.ForEachModCommand(FixHackedPatterns(originalSpecs, m_SndFile.GetType(), autofix, &foundHere));
	if(foundHere)
		AddToLog("Found invalid pattern commands\n");

	// Check for pattern names
	foundHere = false;
	for(PATTERNINDEX i = 0; i < m_SndFile.Patterns.Size(); i++)
	{
		if(m_SndFile.Patterns.IsValidPat(i))
		{
			TCHAR tempname[MAX_PATTERNNAME];
			MemsetZero(tempname);
			m_SndFile.GetPatternName(i, tempname, sizeof(tempname));
			if(strcmp(tempname, "") != 0)
			{
				foundHere = foundHacks = true;
				if(autofix)
					m_SndFile.SetPatternName(i, "");
				else
					break;
			}
		}
	}
	if(foundHere)
		AddToLog("Found pattern names\n");

	// Check for too many channels
	if(m_SndFile.GetNumChannels() > originalSpecs->channelsMax || m_SndFile.GetNumChannels() < originalSpecs->channelsMin)
	{
		AddToLog("Found incompatible channel count\n");
		foundHacks = true;
		// REQUIRES AUTOFIX
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
		AddToLog("Found channel names\n");

	// Check for too many samples
	if(m_SndFile.GetNumSamples() > originalSpecs->samplesMax)
	{
		AddToLog("Found too many samples\n");
		foundHacks = true;
		// REQUIRES AUTOFIX
	}

	// Check for too many instruments
	if(m_SndFile.GetNumInstruments() > originalSpecs->instrumentsMax)
	{
		AddToLog("Found too many instruments\n");
		foundHacks = true;
		// REQUIRES AUTOFIX
	}

	// Check for instrument extensions
	foundHere = false;
	for(INSTRUMENTINDEX i = 1; i <= m_SndFile.GetNumInstruments(); i++)
	{
		MODINSTRUMENT *instr = m_SndFile.Instruments[i];
		if(instr == nullptr) continue;

		if(instr->nFilterMode != FLTMODE_UNCHANGED || instr->nVolRamp != 0 || instr->nResampling != SRCMODE_DEFAULT ||
			instr->nCutSwing != 0 || instr->nResSwing != 0 || instr->nMixPlug != 0 || instr->wPitchToTempoLock != 0 ||
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
				instr->nVolRamp = 0;
				instr->nResampling = SRCMODE_DEFAULT;
				instr->nCutSwing = 0;
				instr->nResSwing = 0;
				instr->nMixPlug = 0;
				instr->wPitchToTempoLock = 0;
				if(instr->nDCT == DCT_PLUGIN) instr->nDCT = DCT_NONE;
				instr->VolEnv.nReleaseNode = instr->PanEnv.nReleaseNode = instr->PitchEnv.nReleaseNode = ENV_RELEASE_NODE_UNSET;
			}
		}

	}
	if(foundHere)
		AddToLog("Found MPT instrument extensions\n");

	// Check for too many orders
	if(m_SndFile.Order.GetLengthTailTrimmed() > originalSpecs->ordersMax)
	{
		AddToLog("Found too many orders\n");
		foundHacks = true;
		// REQUIRES AUTOFIX
	}

	// Check for invalid default tempo
	if(m_SndFile.m_nDefaultTempo > originalSpecs->tempoMax || m_SndFile.m_nDefaultTempo < originalSpecs->tempoMin)
	{
		AddToLog("Found incompatible default tempo\n");
		foundHacks = true;
		if(autofix)
			m_SndFile.m_nDefaultTempo = CLAMP(m_SndFile.m_nDefaultTempo, originalSpecs->tempoMin, originalSpecs->tempoMax);
	}

	// Check for invalid default speed
	if(m_SndFile.m_nDefaultSpeed > originalSpecs->speedMax || m_SndFile.m_nDefaultSpeed < originalSpecs->speedMin)
	{
		AddToLog("Found incompatible default speed\n");
		foundHacks = true;
		if(autofix)
			m_SndFile.m_nDefaultSpeed = CLAMP(m_SndFile.m_nDefaultSpeed, originalSpecs->speedMin, originalSpecs->speedMax);
	}

	// Check for invalid rows per beat / measure values
	if(m_SndFile.m_nRowsPerBeat >= originalSpecs->patternRowsMax || m_SndFile.m_nRowsPerMeasure >= originalSpecs->patternRowsMax)
	{
		AddToLog("Found incompatible rows per beat / measure\n");
		foundHacks = true;
		if(autofix)
		{
			m_SndFile.m_nRowsPerBeat = CLAMP(m_SndFile.m_nRowsPerBeat, 1, (originalSpecs->patternRowsMax - 1));
			m_SndFile.m_nRowsPerMeasure = CLAMP(m_SndFile.m_nRowsPerMeasure, m_SndFile.m_nRowsPerBeat, (originalSpecs->patternRowsMax - 1));
		}
	}

	// Check for new tempo modes
	if(m_SndFile.m_nTempoMode != tempo_mode_classic)
	{
		AddToLog("Found incompatible tempo mode\n");
		foundHacks = true;
		if(autofix)
			m_SndFile.m_nTempoMode = tempo_mode_classic;
	}

	// Check for extended filter range flag
	if(m_SndFile.m_dwSongFlags & SONG_EXFILTERRANGE)
	{
		AddToLog("Found extended filter range\n");
		foundHacks = true;
		if(autofix)
			m_SndFile.m_dwSongFlags &= ~SONG_EXFILTERRANGE;
	}

	// Embedded MIDI configuration in XM files
	if((m_SndFile.m_dwSongFlags & SONG_EMBEDMIDICFG) != 0 && m_SndFile.GetType() == MOD_TYPE_XM)
	{
		AddToLog("Found embedded MIDI macros\n");
		foundHacks = true;
		if(autofix)
			m_SndFile.m_dwSongFlags &= ~SONG_EMBEDMIDICFG;
	}

	// Player flags
	if(!m_SndFile.GetModFlag(MSF_COMPATIBLE_PLAY))
	{
		AddToLog("Compatible play is deactivated\n");
		foundHacks = true;
		if(autofix)
			m_SndFile.SetModFlag(MSF_COMPATIBLE_PLAY, true);
	}

	// Check for restart position where it should not be
	if(m_SndFile.m_nRestartPos > 0 && !originalSpecs->hasRestartPos)
	{
		AddToLog("Found restart position\n");
		foundHacks = true;
		if(autofix)
			m_SndFile.m_nRestartPos = 0;
	}

	if(autofix && foundHacks)
		SetModified();
	
	return foundHacks;
}
