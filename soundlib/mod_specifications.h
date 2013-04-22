/*
 * mod_specifications.h
 * --------------------
 * Purpose: Mod specifications characterise the features of every editable module format in OpenMPT, such as the number of supported channels, samples, effects, etc...
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "Snd_defs.h"
#include "modcommand.h"							// ModCommand::
#include "../soundlib/SoundFilePlayConfig.h"	// mixlevel constants.


//=======================
struct CModSpecifications
//=======================
{
	/// Returns modtype corresponding to given file extension. The extension string
	/// may begin with or without dot, e.g. both ".it" and "it" will be handled correctly.
	static MODTYPE ExtensionToType(const std::string &ext);

	// Return true if format supports given note.
	bool HasNote(ModCommand::NOTE note) const;
	bool HasVolCommand(ModCommand::VOLCMD volcmd) const;
	bool HasCommand(ModCommand::COMMAND cmd) const;
	// Return corresponding effect letter for this format
	char GetEffectLetter(ModCommand::COMMAND cmd) const;
	char GetVolEffectLetter(ModCommand::VOLCMD cmd) const;

	// NOTE: If changing order, update all initializations below.
	char fileExtension[6];				// File extension without dot.
	MODTYPE internalType;				// Internal MODTYPE value
	ModCommand::NOTE noteMin;			// Minimum note index (index starts from 1)
	ModCommand::NOTE noteMax;			// Maximum note index (index starts from 1)
	bool hasNoteCut;					// True if format has notecut.
	bool hasNoteOff;					// True if format has noteoff.
	bool hasNoteFade;					// True if format has notefade.
	PATTERNINDEX patternsMax;
	ORDERINDEX ordersMax;
	CHANNELINDEX channelsMin;			// Minimum number of editable channels in pattern.
	CHANNELINDEX channelsMax;			// Maximum number of editable channels in pattern.
	TEMPO tempoMin;
	TEMPO tempoMax;
	ROWINDEX patternRowsMin;
	ROWINDEX patternRowsMax;
	uint16 modNameLengthMax;			// Meaning 'usable letters', possible null character is not included.
	uint16 sampleNameLengthMax;			// Dito
	uint16 sampleFilenameLengthMax;		// Dito
	uint16 instrNameLengthMax;			// Dito
	uint16 instrFilenameLengthMax;		// Dito
	SAMPLEINDEX samplesMax;				// Max number of samples == Highest possible sample index
	INSTRUMENTINDEX instrumentsMax;		// Max number of instruments == Highest possible instrument index
	mixLevels defaultMixLevels;			// Default mix levels that are used when creating a new file in this format
	uint8 MIDIMappingDirectivesMax;		// Number of MIDI Mapping directives that the format can store (0 = none)
	UINT speedMin;						// Minimum ticks per frame
	UINT speedMax;						// Maximum ticks per frame
	bool hasComments;					// True if format has a comments field
	uint8 envelopePointsMax;			// Maximum number of points of each envelope
	bool hasReleaseNode;				// Envelope release node
	char commands[MAX_EFFECTS + 1];		// An array holding all commands this format supports; commands that are not supported are marked with "?"
	char volcommands[MAX_VOLCMDS + 1];	// dito, but for volume column
	bool hasIgnoreIndex;				// Does "+++" pattern exist?
	bool hasStopIndex;					// Does "---" pattern exist?
	bool hasRestartPos;					// Format has an automatic restart order position
	bool supportsPlugins;				// Format can store plugins
	bool hasPatternSignatures;			// Can patterns have a custom time signature?
	bool hasPatternNames;				// Cat patterns have a name?
	SongFlags songFlags;				// Supported song flags
};


namespace ModSpecs
{
	extern const CModSpecifications mptm;
	extern const CModSpecifications mod;
	extern const CModSpecifications s3m;
	extern const CModSpecifications s3mEx;
	extern const CModSpecifications xm;
	extern const CModSpecifications xmEx;
	extern const CModSpecifications it;
	extern const CModSpecifications itEx;
	extern const CModSpecifications *Collection[8];
}