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

	FlagSet<Flags> findFlags, replaceFlags;	// See Flags

	// Data to replace with
	ReplaceMode replaceNoteAction, replaceInstrAction, replaceVolumeAction, replaceParamAction;
	int replaceNote, replaceInstr, replaceVolume, replaceParam;
	ModCommand::VOLCMD  replaceVolCmd;
	ModCommand::COMMAND replaceCommand;

	// Data to find
	ModCommand::NOTE    findNoteMin, findNoteMax;
	ModCommand::INSTR   findInstrMin, findInstrMax;
	ModCommand::VOLCMD  findVolCmd;
	int                 findVolumeMin, findVolumeMax;
	ModCommand::COMMAND findCommand;
	int                 findParamMin, findParamMax;

	PatternRect selection;					// Find in this selection (if FindReplace::InPatSelection is set)
	CHANNELINDEX findChnMin, findChnMax;	// Find in these channels (if FindReplace::InChannels is set)

	FindReplace();
};

DECLARE_FLAGSET(FindReplace::Flags);
