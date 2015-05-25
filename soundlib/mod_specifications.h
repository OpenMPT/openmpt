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


OPENMPT_NAMESPACE_BEGIN


//=======================
struct CModSpecifications
//=======================
{
	/// Returns modtype corresponding to given file extension. The extension string
	/// may begin with or without dot, e.g. both ".it" and "it" will be handled correctly.
	static MODTYPE ExtensionToType(std::string ext);

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
	bool hasNoteCut : 1;				// True if format has note cut (^^).
	bool hasNoteOff : 1;				// True if format has note off (==).
	bool hasNoteFade : 1;				// True if format has note fade (~~).
	PATTERNINDEX patternsMax;
	ORDERINDEX ordersMax;
	SEQUENCEINDEX sequencesMax;
	CHANNELINDEX channelsMin;			// Minimum number of editable channels in pattern.
	CHANNELINDEX channelsMax;			// Maximum number of editable channels in pattern.
	TEMPO tempoMin;
	TEMPO tempoMax;
	ROWINDEX patternRowsMin;
	ROWINDEX patternRowsMax;
	uint16 modNameLengthMax;			// Meaning 'usable letters', possible null character is not included.
	uint16 sampleNameLengthMax;			// Ditto
	uint16 sampleFilenameLengthMax;		// Ditto
	uint16 instrNameLengthMax;			// Ditto
	uint16 instrFilenameLengthMax;		// Ditto
	SAMPLEINDEX samplesMax;				// Max number of samples == Highest possible sample index
	INSTRUMENTINDEX instrumentsMax;		// Max number of instruments == Highest possible instrument index
	mixLevels defaultMixLevels;			// Default mix levels that are used when creating a new file in this format
	uint8 MIDIMappingDirectivesMax;		// Number of MIDI Mapping directives that the format can store (0 = none)
	UINT speedMin;						// Minimum ticks per frame
	UINT speedMax;						// Maximum ticks per frame
	uint8 envelopePointsMax;			// Maximum number of points of each envelope
	bool hasReleaseNode : 1;			// Envelope release node
	bool hasComments : 1;				// True if format has a comments field
	bool hasIgnoreIndex : 1;			// Does "+++" pattern exist?
	bool hasStopIndex : 1;				// Does "---" pattern exist?
	bool hasRestartPos : 1;				// Format has an automatic restart order position
	bool supportsPlugins : 1;			// Format can store plugins
	bool hasPatternSignatures : 1;		// Can patterns have a custom time signature?
	bool hasPatternNames : 1;			// Cat patterns have a name?
	bool hasArtistName : 1;				// Can artist name be stored in file?
	bool hasDefaultResampling : 1;		// Can default resampling be saved? (if not, it can still be modified in the GUI but won't set the module as modified)
	FlagSet<SongFlags>::store_type songFlags;	// Supported song flags   NOTE: Do not use the overloaded operator | to set these flags because this results in dynamic initialization
	char commands[MAX_EFFECTS + 1];		// An array holding all commands this format supports; commands that are not supported are marked with "?"
	char volcommands[MAX_VOLCMDS + 1];	// Ditto, but for volume column
	FlagSet<SongFlags> GetSongFlags() const { return FlagSet<SongFlags>(songFlags); }
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


OPENMPT_NAMESPACE_END
