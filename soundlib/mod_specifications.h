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
#include "modcommand.h"						//
#include "../soundlib/SoundFilePlayConfig.h" // mixlevel constants.



struct CModSpecifications
//=======================
{
	/// Returns modtype corresponding to given file extension. The extension string
	/// may begin with or without dot, e.g. both ".it" and "it" will be handled correctly.
	static MODTYPE ExtensionToType(LPCTSTR pszExt);

	// Return true if format supports given note.
	bool HasNote(ModCommand::NOTE note) const;
	bool HasVolCommand(ModCommand::VOLCMD volcmd) const;
	bool HasCommand(ModCommand::COMMAND cmd) const;
	// Return corresponding effect letter for this format
	char GetEffectLetter(ModCommand::COMMAND cmd) const;
	char GetVolEffectLetter(ModCommand::VOLCMD cmd) const;

	// NOTE: If changing order, update all initializations below.
	char fileExtension[6];	  // File extension without dot.
	MODTYPE internalType;	  // Internal MODTYPE value
	ModCommand::NOTE noteMin; // Minimum note index (index starts from 1)
	ModCommand::NOTE noteMax; // Maximum note index (index starts from 1)
	bool hasNoteCut;		  // True if format has notecut.
	bool hasNoteOff;		  // True if format has noteoff.
	bool hasNoteFade;		  // True if format has notefade.
	PATTERNINDEX patternsMax;
	ORDERINDEX ordersMax;
	CHANNELINDEX channelsMin; // Minimum number of editable channels in pattern.
	CHANNELINDEX channelsMax; // Maximum number of editable channels in pattern.
	TEMPO tempoMin;
	TEMPO tempoMax;
	ROWINDEX patternRowsMin;
	ROWINDEX patternRowsMax;
	uint16 modNameLengthMax;		// Meaning 'usable letters', possible null character is not included.
	uint16 sampleNameLengthMax;		// Dito
	uint16 sampleFilenameLengthMax;	// Dito
	uint16 instrNameLengthMax;		// Dito
	uint16 instrFilenameLengthMax;	// Dito
	SAMPLEINDEX samplesMax;			// Max number of samples == Highest possible sample index
	INSTRUMENTINDEX instrumentsMax;	// Max number of instruments == Highest possible instrument index
	BYTE defaultMixLevels;
	BYTE MIDIMappingDirectivesMax;
	UINT speedMin;					// Minimum ticks per frame
	UINT speedMax;					// Maximum ticks per frame
	bool hasComments;				// True if format has a comments field
	uint8 envelopePointsMax;		// Maximum number of points of each envelope
	bool hasReleaseNode;			// Envelope release node
	char commands[MAX_EFFECTS + 1]; // An array holding all commands this format supports; commands that are not supported are marked with "?"
	char volcommands[MAX_VOLCMDS + 1]; // dito, but for volume column
	bool hasIgnoreIndex;			// Does "+++" pattern exist?
	bool hasRestartPos;
	bool supportsPlugins;
	bool hasPatternSignatures;		// Can patterns have a custom time signature?
	bool hasPatternNames;			// Cat patterns have a name?
	SongFlags songFlags;			// Supported song flags
};


namespace ModSpecs
{

const CModSpecifications mptm =
{
	/*
	TODO: Proper, less arbitrarily chosen values here.
	NOTE: If changing limits, see whether:
			-savefile format and GUI methods can handle new values(might not be a small task :).
	 */
	"mptm",								// File extension
	MOD_TYPE_MPT,						// Internal MODTYPE value
	NOTE_MIN,							// Minimum note index
	NOTE_MAX,							// Maximum note index
	true,								// Has notecut.
	true,								// Has noteoff.
	true,								// Has notefade.
	4000,								// Pattern max.
	4000,								// Order max.
	1,									// Channel min
	127,								// Channel max
	32,									// Min tempo
	512,								// Max tempo
	1,									// Min pattern rows
	1024,								// Max pattern rows
	25,									// Max mod name length
	25,									// Max sample name length
	12,									// Max sample filename length
	25,									// Max instrument name length
	12,									// Max instrument filename length
	3999,								// SamplesMax
	255,								// instrumentMax
	mixLevels_117RC3,					// defaultMixLevels
	200,								// Max MIDI mapping directives
	1,									// Min Speed
	255,								// Max Speed
	true,								// Has song comments
	MAX_ENVPOINTS,						// Envelope point count
	true,								// Has envelope release node
	" JFEGHLKRXODB?CQATI?SMNVW?UY?P?Z\\:#",	// Supported Effects
	" vpcdabuhlrgfe?o",					// Supported Volume Column commands
	true,								// Has "+++" pattern
	true,								// Has restart position (order)
	true,								// Supports plugins
	true,								// Custom pattern time signatures
	true,								// Pattern names
	SONG_LINEARSLIDES | SONG_EXFILTERRANGE | SONG_ITOLDEFFECTS | SONG_ITCOMPATGXX | SONG_EMBEDMIDICFG,	// Supported song flags
};




const CModSpecifications mod =
{
	// TODO: Set correct values.
	"mod",								// File extension
	MOD_TYPE_MOD,						// Internal MODTYPE value
	37,									// Minimum note index
	108,								// Maximum note index
	false,								// No notecut.
	false,								// No noteoff.
	false,								// No notefade.
	128,								// Pattern max.
	128,								// Order max.
	4,									// Channel min
	32,									// Channel max
	32,									// Min tempo
	255,								// Max tempo
	64,									// Min pattern rows
	64,									// Max pattern rows
	20,									// Max mod name length
	22,									// Max sample name length
	0,									// Max sample filename length
	0,									// Max instrument name length
	0,									// Max instrument filename length
	31,									// SamplesMax
	0,									// instrumentMax
	mixLevels_compatible,				// defaultMixLevels
	0,									// Max MIDI mapping directives
	1,									// Min Speed
	32,									// Max Speed
	false,								// No song comments
	0,									// No instrument envelopes
	false,								// No envelope release node
	" 0123456789ABCD?FF?E???????????????",	// Supported Effects
	" ???????????????",					// Supported Volume Column commands
	false,								// Doesn't have "+++" pattern
	true,								// Has restart position (order)
	false,								// Doesn't support plugins
	false,								// No custom pattern time signatures
	false,								// No pattern names
	SONG_PT1XMODE,						// Supported song flags
};


const CModSpecifications xm =
{
	// TODO: Set correct values.
	"xm",								// File extension
	MOD_TYPE_XM,						// Internal MODTYPE value
	13,									// Minimum note index
	108,								// Maximum note index
	false,								// No notecut.
	true,								// Has noteoff.
	false,								// No notefade.
	256,								// Pattern max.
	255,								// Order max.
	1,									// Channel min
	32,									// Channel max
	32,									// Min tempo
	10000,								// Max tempo
	1,									// Min pattern rows
	256,								// Max pattern rows
	20,									// Max mod name length
	22,									// Max sample name length
	0,									// Max sample filename length
	22,									// Max instrument name length
	0,									// Max instrument filename length
	128 * 16,							// SamplesMax (actually 16 per instrument)
	128,								// instrumentMax
	mixLevels_compatible,				// defaultMixLevels
	0,									// Max MIDI mapping directives
	1,									// Min Speed
	31,									// Max Speed
	false,								// No song comments
	12,									// Envelope point count
	false,								// No envelope release node
	" 0123456789ABCDRFFTE???GHK??XPL????",	// Supported Effects
	" vpcdabuhlrg????",					// Supported Volume Column commands
	false,								// Doesn't have "+++" pattern
	true,								// Has restart position (order)
	false,								// Doesn't support plugins
	false,								// No custom pattern time signatures
	false,								// No pattern names
	SONG_LINEARSLIDES,					// Supported song flags
};

// XM with MPT extensions
const CModSpecifications xmEx =
{
	// TODO: Set correct values.
	"xm",								// File extension
	MOD_TYPE_XM,						// Internal MODTYPE value
	13,									// Minimum note index
	108,								// Maximum note index
	false,								// No notecut.
	true,								// Has noteoff.
	false,								// No notefade.
	256,								// Pattern max.
	255,								// Order max.
	1,									// Channel min
	127,								// Channel max
	32,									// Min tempo
	10000,								// Max tempo
	1,									// Min pattern rows
	1024,								// Max pattern rows
	20,									// Max mod name length
	22,									// Max sample name length
	0,									// Max sample filename length
	22,									// Max instrument name length
	0,									// Max instrument filename length
	MAX_SAMPLES - 1,					// SamplesMax (actually 32 per instrument(256 * 32 = 8192), but limited to MAX_SAMPLES = 4000)
	255,								// instrumentMax
	mixLevels_compatible,				// defaultMixLevels
	200,								// Max MIDI mapping directives
	1,									// Min Speed
	31,									// Max Speed
	true,								// Has song comments
	12,									// Envelope point count
	false,								// No envelope release node
	" 0123456789ABCDRFFTE???GHK?YXPLZ\\?#",	// Supported Effects
	" vpcdabuhlrgfe?o",					// Supported Volume Column commands
	false,								// Doesn't have "+++" pattern
	true,								// Has restart position (order)
	true,								// Supports plugins
	false,								// No custom pattern time signatures
	true,								// Pattern names
	SONG_LINEARSLIDES | SONG_EXFILTERRANGE | SONG_EMBEDMIDICFG,	// Supported song flags
};

const CModSpecifications s3m =
{
	// TODO: Set correct values.
	"s3m",								// File extension
	MOD_TYPE_S3M,						// Internal MODTYPE value
	13,									// Minimum note index
	108,								// Maximum note index
	true,								// Has notecut.
	false,								// No noteoff.
	false,								// No notefade.
	100,								// Pattern max.
	255,								// Order max.
	1,									// Channel min
	32,									// Channel max
	33,									// Min tempo
	255,								// Max tempo
	64,									// Min pattern rows
	64,									// Max pattern rows
	27,									// Max mod name length
	27,									// Max sample name length
	12,									// Max sample filename length
	0,									// Max instrument name length
	0,									// Max instrument filename length
	99,									// SamplesMax
	0,									// instrumentMax
	mixLevels_compatible,				// defaultMixLevels
	0,									// Max MIDI mapping directives
	1,									// Min Speed
	255,								// Max Speed
	false,								// No song comments
	0,									// No instrument envelopes
	false,								// No envelope release node
	" JFEGHLKRXODB?CQATI?SMNVW?U????????",	// Supported Effects
	" vp?????????????",					// Supported Volume Column commands
	true,								// Has "+++" pattern
	false,								// Doesn't have restart position (order)
	false,								// Doesn't support plugins
	false,								// No custom pattern time signatures
	false,								// No pattern names
	SONG_FASTVOLSLIDES | SONG_AMIGALIMITS,	// Supported song flags
};

// S3M with MPT extensions
const CModSpecifications s3mEx =
{
	// TODO: Set correct values.
	"s3m",								// File extension
	MOD_TYPE_S3M,						// Internal MODTYPE value
	13,									// Minimum note index
	108,								// Maximum note index
	true,								// Has notecut.
	false,								// No noteoff.
	false,								// No notefade.
	100,								// Pattern max.
	255,								// Order max.
	1,									// Channel min
	32,									// Channel max
	33,									// Min tempo
	255,								// Max tempo
	64,									// Min pattern rows
	64,									// Max pattern rows
	27,									// Max mod name length
	27,									// Max sample name length
	12,									// Max sample filename length
	0,									// Max instrument name length
	0,									// Max instrument filename length
	99,									// SamplesMax
	0,									// instrumentMax
	mixLevels_compatible,				// defaultMixLevels
	0,									// Max MIDI mapping directives
	1,									// Min Speed
	255,								// Max Speed
	false,								// No song comments
	0,									// No instrument envelopes
	false,								// No envelope release node
	" JFEGHLKRXODB?CQATI?SMNVW?UY?P?Z???",	// Supported Effects
	" vp?????????????",					// Supported Volume Column commands
	true,								// Has "+++" pattern
	false,								// Doesn't have restart position (order)
	false,								// Doesn't support plugins
	false,								// No custom pattern time signatures
	false,								// No pattern names
	SONG_FASTVOLSLIDES | SONG_AMIGALIMITS,	// Supported song flags
};

const CModSpecifications it =
{
	// TODO: Set correct values.
	"it",								// File extension
	MOD_TYPE_IT,						// Internal MODTYPE value
	1,									// Minimum note index
	120,								// Maximum note index
	true,								// Has notecut.
	true,								// Has noteoff.
	true,								// Has notefade.
	200,								// Pattern max.
	256,								// Order max.
	1,									// Channel min
	64,									// Channel max
	32,									// Min tempo
	255,								// Max tempo
	1,									// Min pattern rows
	200,								// Max pattern rows
	25,									// Max mod name length
	25,									// Max sample name length
	12,									// Max sample filename length
	25,									// Max instrument name length
	12,									// Max instrument filename length
	99,									// SamplesMax
	99,									// instrumentMax
	mixLevels_compatible,				// defaultMixLevels
	0,									// Max MIDI mapping directives
	1,									// Min Speed
	255,								// Max Speed
	true,								// Has song comments
	25,									// Envelope point count
	false,								// No envelope release node
	" JFEGHLKRXODB?CQATI?SMNVW?UY?P?Z???",	// Supported Effects
	" vpcdab?h??gfe??",					// Supported Volume Column commands
	true,								// Has "+++" pattern
	false,								// Doesn't have restart position (order)
	false,								// Doesn't support plugins
	false,								// No custom pattern time signatures
	false,								// No pattern names
	SONG_LINEARSLIDES | SONG_ITOLDEFFECTS | SONG_ITCOMPATGXX | SONG_EMBEDMIDICFG,	// Supported song flags
};

const CModSpecifications itEx =
{
	// TODO: Set correct values.
	"it",								// File extension
	MOD_TYPE_IT,						// Internal MODTYPE value
	1,									// Minimum note index
	120,								// Maximum note index
	true,								// Has notecut.
	true,								// Has noteoff.
	true,								// Has notefade.
	240,								// Pattern max.
	256,								// Order max.
	1,									// Channel min
	127,								// Channel max
	32,									// Min tempo
	512,								// Max tempo
	1,									// Min pattern rows
	1024,								// Max pattern rows
	25,									// Max mod name length
	25,									// Max sample name length
	12,									// Max sample filename length
	25,									// Max instrument name length
	12,									// Max instrument filename length
	3999,								// SamplesMax
	255,								// instrumentMax
	mixLevels_compatible,				// defaultMixLevels
	200,								// Max MIDI mapping directives
	1,									// Min Speed
	255,								// Max Speed
	true,								// Has song comments
	25,									// Envelope point count
	false,								// No envelope release node
	" JFEGHLKRXODB?CQATI?SMNVW?UY?P?Z\\?#",	// Supported Effects
	" vpcdab?h??gfe?o",					// Supported Volume Column commands
	true,								// Has "+++" pattern
	false,								// Doesn't have restart position (order)
	true,								// Supports plugins
	false,								// No custom pattern time signatures
	true,								// Pattern names
	SONG_LINEARSLIDES | SONG_EXFILTERRANGE | SONG_ITOLDEFFECTS | SONG_ITCOMPATGXX | SONG_EMBEDMIDICFG | SONG_ITPROJECT | SONG_ITPEMBEDIH,	// Supported song flags
};

static const CModSpecifications *Collection[] = { &mptm, &mod, &s3m, &s3mEx, &xm, &xmEx, &it, &itEx };

} // namespace ModSpecs