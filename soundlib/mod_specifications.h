#ifndef MOD_SPECIFICATIONS_H
#define MOD_SPECIFICATIONS_H

//Simple struct to gather various modspecifications in one place.
//Feel free to improve.


struct CModSpecifications
//=======================
{
	//NOTE: If changing order, update all initializations below.
	char fileExtension[5];
	PATTERNINDEX patternsMax;
	ORDERINDEX ordersMax;
	CHANNELINDEX channelsMin; 
	CHANNELINDEX channelsMax;
	//Note: The two above refer to the user editable pattern channels,
	//not to the internal sound channels. Maybe there should be a separate
	//setting for those(January 2007).
	TEMPO tempoMin;
	TEMPO tempoMax;
	ROWINDEX patternRowsMin;
	ROWINDEX patternRowsMax;
	uint16 modNameLengthMax;	//Meaning 'usable letters', possible null character is not included.
	SAMPLEINDEX samplesMax;
	INSTRUMENTINDEX instrumentsMax;
	BYTE defaultMixLevels;
	BYTE MIDIMappingDirectivesMax;
};


const CModSpecifications MPTM_SPECS =
{
	/*
	TODO: Proper, less arbitrarily chosen, values here.
	NOTE: If changing limits, see whether:
			-savefile format and GUI methods can handle new values(might not be a small task :).
	 */
	"MPTm",								//File extension
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
	200									//Max MIDI mapping directives
};




const CModSpecifications MOD_STD_SPECS =
{
	//TODO: Set correct values.
	"mod",								//File extension
	64,									//Pattern max.
	128,								//Order max.
	4,									//Channel min
	4,									//Channel max
	32,									//Min tempo
	256,								//Max tempo
	64,									//Min pattern rows
	64,									//Max pattern rows
    20,									//Max mod name length
	31,									//SamplesMax
	0,									//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0,									//Max MIDI mapping directives
};

const CModSpecifications MOD_MPTEXT_SPECS =
{
	//TODO: Set correct values.
	"mod",								//File extension
	64,									//Pattern max.
	128,								//Order max.
	4,									//Channel min
	32,									//Channel max
	32,									//Min tempo
	256,								//Max tempo
	64,									//Min pattern rows
	64,									//Max pattern rows
    20,									//Max mod name length
	31,									//SamplesMax
	0,									//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0									//Max MIDI mapping directives
};

const CModSpecifications XM_STD_SPECS =
{
	//TODO: Set correct values.
	"xm",								//File extension
	64,									//Pattern max.
	128,								//Order max.
	4,									//Channel min
	32,									//Channel max
	32,									//Min tempo
	256,								//Max tempo
	4,									//Min pattern rows
	256,								//Max pattern rows
    20,									//Max mod name length
	31,									//SamplesMax
	200,								//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0									//Max MIDI mapping directives
};


const CModSpecifications XM_MPTEXT_SPECS =
{
	//TODO: Set correct values.
	"xm",								//File extension
	240,								//Pattern max.
	256,								//Order max.
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
	200									//Max MIDI mapping directives
};

const CModSpecifications S3M_STD_SPECS =
{
	//TODO: Set correct values.
	"s3m",								//File extension
	240,								//Pattern max.
	256,								//Order max.
	4,									//Channel min
	32,									//Channel max
	32,									//Min tempo
	256,								//Max tempo
	64,									//Min pattern rows
	64,									//Max pattern rows
    27,									//Max mod name length
	99,									//SamplesMax
	0,									//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0									//Max MIDI mapping directives
};

const CModSpecifications S3M_MPTEXT_SPECS =
{
	//TODO: Set correct values.
	"s3m",								//File extension
	240,								//Pattern max.
	256,								//Order max.
	4,									//Channel min
	32,									//Channel max
	32,									//Min tempo
	256,								//Max tempo
	64,									//Min pattern rows
	64,									//Max pattern rows
    27,									//Max mod name length
	99,									//SamplesMax
	0,									//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0									//Max MIDI mapping directives
};

const CModSpecifications IT_STD_SPECS =
{
	//TODO: Set correct values.
	"it",								//File extension
	240,								//Pattern max.
	200,								//Order max.
	4,									//Channel min
	64,									//Channel max
	32,									//Min tempo
	256,								//Max tempo
	4,									//Min pattern rows
	256,								//Max pattern rows
    25,									//Max mod name length
	256,								//SamplesMax
	200,								//instrumentMax
	mixLevels_original,					//defaultMixLevels
	0									//Max MIDI mapping directives
};

const CModSpecifications IT_MPTEXT_SPECS =
{
	//TODO: Set correct values.
	"it",								//File extension
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
	200									//Max MIDI mapping directives
};




#endif
