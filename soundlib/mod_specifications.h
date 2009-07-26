#ifndef MOD_SPECIFICATIONS_H
#define MOD_SPECIFICATIONS_H

#include "modcommand.h"						// 
#include "../mptrack/SoundFilePlayConfig.h" // mixlevel constants.



struct CModSpecifications
//=======================
{
	// Return true iff format supports given note.
	bool HasNote(MODCOMMAND::NOTE note) const;

	//NOTE: If changing order, update all initializations below.
	char fileExtension[6];	  // File extension without dot.
	MODCOMMAND::NOTE noteMin; // Minimum note index (index starts from 1)
	MODCOMMAND::NOTE noteMax; // Maximum note index (index starts from 1)
	bool hasNoteCut;		  // True iff format has notecut.
	bool hasNoteOff;		  // True iff format has noteoff.
	PATTERNINDEX patternsMax;
	ORDERINDEX ordersMax;
	CHANNELINDEX channelsMin; // Minimum number of editable channels in pattern.
	CHANNELINDEX channelsMax; // Maximum number of editable channels in pattern.
	TEMPO tempoMin;
	TEMPO tempoMax;
	ROWINDEX patternRowsMin;
	ROWINDEX patternRowsMax;
	uint16 modNameLengthMax;	//Meaning 'usable letters', possible null character is not included.
	SAMPLEINDEX samplesMax;
	INSTRUMENTINDEX instrumentsMax;
	BYTE defaultMixLevels;
	BYTE MIDIMappingDirectivesMax;
	UINT speedMin;
	UINT speedMax;
};


namespace ModSpecs
{

const CModSpecifications mptm =
{
	/*
	TODO: Proper, less arbitrarily chosen, values here.
	NOTE: If changing limits, see whether:
			-savefile format and GUI methods can handle new values(might not be a small task :).
	 */
	"MPTm",								//File extension
	1,									//Minimum note index
	NOTE_MAX,							//Maximum note index
	true,								//Has notecut.
	true,								//Has noteoff.
	4000,								//Pattern max.
	4000,								//Order max.
	4,									//Channel min
	127,								//Channel max
	32,									//Min tempo
	512,								//Max tempo
	4,									//Min pattern rows
	1024,								//Max pattern rows
    25,									//Max mod name length
	4000,								//SamplesMax
	256,								//instrumentMax
	mixLevels_117RC3,					//defaultMixLevels
	200,								//Max MIDI mapping directives
	1,									//Min Speed
	255,								//Max Speed
};




const CModSpecifications mod =
{
	//TODO: Set correct values.
	"mod",								//File extension
	37,									//Minimum note index
	108,								//Maximum note index
	false,								//No notecut.
	false,								//No noteoff.
	64,									//Pattern max.
	128,								//Order max.
	4,									//Channel min
	4,									//Channel max
	32,									//Min tempo
	255,								//Max tempo
	64,									//Min pattern rows
	64,									//Max pattern rows
    20,									//Max mod name length
	31,									//SamplesMax
	0,									//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0,									//Max MIDI mapping directives
	1,									//Min Speed
	31,									//Max Speed
};

// MOD with MPT extensions.
const CModSpecifications modEx =
{
	//TODO: Set correct values.
	"mod",								//File extension
	37,									//Minimum note index
	108,								//Maximum note index
	false,								//No notecut.
	false,								//No noteoff.
	64,									//Pattern max.
	128,								//Order max.
	4,									//Channel min
	32,									//Channel max
	32,									//Min tempo
	255,								//Max tempo
	64,									//Min pattern rows
	64,									//Max pattern rows
    20,									//Max mod name length
	31,									//SamplesMax
	0,									//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0,									//Max MIDI mapping directives
	1,									//Min Speed
	31,									//Max Speed
};

const CModSpecifications xm =
{
	//TODO: Set correct values.
	"xm",								//File extension
	13,									//Minimum note index
	108,								//Maximum note index
	false,								//No notecut.
	true,								//Has noteoff.
	64,									//Pattern max.
	128,								//Order max.
	4,									//Channel min
	32,									//Channel max
	32,									//Min tempo
	255,								//Max tempo
	4,									//Min pattern rows
	256,								//Max pattern rows
    20,									//Max mod name length
	31,									//SamplesMax
	200,								//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0,									//Max MIDI mapping directives
	1,									//Min Speed
	31,									//Max Speed
};

// XM with MPT extensions
const CModSpecifications xmEx =
{
	//TODO: Set correct values.
	"xm",								//File extension
	13,									//Minimum note index
	108,								//Maximum note index
	false,								//No notecut.
	true,								//Has noteoff.
	240,								//Pattern max.
	255,								//Order max.
	4,									//Channel min
	127,								//Channel max
	32,									//Min tempo
	512,								//Max tempo
	4,									//Min pattern rows
	1024,								//Max pattern rows
    20,									//Max mod name length
	4000,								//SamplesMax
	256,								//instrumentMax
	mixLevels_117RC3,					//defaultMixLevels
	200,								//Max MIDI mapping directives
	1,									//Min Speed
	31,									//Max Speed
};

const CModSpecifications s3m =
{
	//TODO: Set correct values.
	"s3m",								//File extension
	13,									//Minimum note index
	NOTE_MAX,							//Maximum note index
	true,								//Has notecut.
	false,								//No noteoff.
	240,								//Pattern max.
	256,								//Order max.
	4,									//Channel min
	32,									//Channel max
	32,									//Min tempo
	255,								//Max tempo
	64,									//Min pattern rows
	64,									//Max pattern rows
    27,									//Max mod name length
	99,									//SamplesMax
	0,									//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0,									//Max MIDI mapping directives
	1,									//Min Speed
	255,								//Max Speed
};

// S3M with MPT extensions
const CModSpecifications s3mEx =
{
	//TODO: Set correct values.
	"s3m",								//File extension
	13,									//Minimum note index
	NOTE_MAX,							//Maximum note index
	true,								//Has notecut.
	false,								//No noteoff.
	240,								//Pattern max.
	256,								//Order max.
	4,									//Channel min
	32,									//Channel max
	32,									//Min tempo
	255,								//Max tempo
	64,									//Min pattern rows
	64,									//Max pattern rows
    27,									//Max mod name length
	99,									//SamplesMax
	0,									//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0,									//Max MIDI mapping directives
	1,									//Min Speed
	255,								//Max Speed
};

const CModSpecifications it =
{
	//TODO: Set correct values.
	"it",								//File extension
	1,									//Minimum note index
	NOTE_MAX,							//Maximum note index
	true,								//Has notecut.
	true,								//Has noteoff.
	240,								//Pattern max.
	200,								//Order max.
	4,									//Channel min
	64,									//Channel max
	32,									//Min tempo
	255,								//Max tempo
	4,									//Min pattern rows
	256,								//Max pattern rows
    25,									//Max mod name length
	256,								//SamplesMax
	200,								//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0,									//Max MIDI mapping directives
	1,									//Min Speed
	255,								//Max Speed
};

const CModSpecifications itEx =
{
	//TODO: Set correct values.
	"it",								//File extension
	1,									//Minimum note index
	NOTE_MAX,							//Maximum note index
	true,								//Has notecut.
	true,								//Has noteoff.
	240,								//Pattern max.
	256,								//Order max.
	4,									//Channel min
	127,								//Channel max
	32,									//Min tempo
	512,								//Max tempo
	4,									//Min pattern rows
	1024,								//Max pattern rows
    25,									//Max mod name length
	4000,								//SamplesMax
	256,								//instrumentMax
	mixLevels_117RC3,					//defaultMixLevels
	200,								//Max MIDI mapping directives
	1,									//Min Speed
	255,								//Max Speed
};

} //namespace ModSpecs



#endif
