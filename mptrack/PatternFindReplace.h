/*
 * PatternFindReplace.h
 * --------------------
 * Purpose: Implementation of the pattern search.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "PatternCursor.h"
#include "../soundlib/modcommand.h"

// Find/Replace data
struct FindReplace
{
	static FindReplace instance;

	enum Flags
	{
		Note			= 0x01,		// Search for note
		Instr			= 0x02,		// Search for instrument
		VolCmd			= 0x04,		// Search for volume effect
		Volume			= 0x08,		// Search for volume
		Command			= 0x10,		// Search for effect
		Param			= 0x20,		// Search for effect parameter
		PCParam			= 0x40,		// Parameter of PC event
		PCValue			= 0x80,		// Value of PC event

		InChannels		= 0x100,	// Limit search to channels
		FullSearch		= 0x200,	// Search whole song
		InPatSelection	= 0x400,	// Search in current pattern selection
		Replace			= 0x800,	// Replace
		ReplaceAll		= 0x1000,	// Replace all
	};

	enum ReplaceMode
	{
		ReplaceValue,
		ReplaceRelative,
		ReplaceMultiply,
	};

	enum
	{
		ReplaceOctaveUp   =  12000,
		ReplaceOctaveDown = -12000,
	};

	FlagSet<Flags> findFlags = FullSearch;
	FlagSet<Flags> replaceFlags = ReplaceAll;

	// Data to replace with
	ReplaceMode replaceNoteAction = ReplaceValue, replaceInstrAction = ReplaceValue, replaceVolumeAction = ReplaceValue, replaceParamAction = ReplaceValue;
	int replaceNote = NOTE_NONE, replaceInstr = 0, replaceVolume = 0, replaceParam = 0;
	ModCommand::VOLCMD  replaceVolCmd = VOLCMD_NONE;
	ModCommand::COMMAND replaceCommand = CMD_NONE;

	// Data to find
	ModCommand::NOTE    findNoteMin = NOTE_NONE, findNoteMax = NOTE_NONE;
	ModCommand::INSTR   findInstrMin = 0, findInstrMax = 0;
	ModCommand::VOLCMD  findVolCmd = VOLCMD_NONE;
	int                 findVolumeMin = 0, findVolumeMax = 0;
	ModCommand::COMMAND findCommand = CMD_NONE;
	int                 findParamMin = 0, findParamMax = 0;

	PatternRect selection;                        // Find in this selection (if FindReplace::InPatSelection is set)
	CHANNELINDEX findChnMin = 0, findChnMax = 0;  // Find in these channels (if FindReplace::InChannels is set)

	FindReplace() = default;
};

DECLARE_FLAGSET(FindReplace::Flags);
