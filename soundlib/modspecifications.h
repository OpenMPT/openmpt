#ifndef MOD_SPECIFICATIONS_H
#define MOD_SPECIFICATIONS_H


//Module IDs
#define MOD_TYPE_NONE		0x00
#define MOD_TYPE_MOD		0x01
#define MOD_TYPE_S3M		0x02
#define MOD_TYPE_XM			0x04
#define MOD_TYPE_MED		0x08
#define MOD_TYPE_MTM		0x10
#define MOD_TYPE_IT			0x20
#define MOD_TYPE_669		0x40
#define MOD_TYPE_ULT		0x80
#define MOD_TYPE_STM		0x100
#define MOD_TYPE_FAR		0x200
#define MOD_TYPE_WAV		0x400
#define MOD_TYPE_AMF		0x800
#define MOD_TYPE_AMS		0x1000
#define MOD_TYPE_DSM		0x2000
#define MOD_TYPE_MDL		0x4000
#define MOD_TYPE_OKT		0x8000
#define MOD_TYPE_MID		0x10000
#define MOD_TYPE_DMF		0x20000
#define MOD_TYPE_PTM		0x40000
#define MOD_TYPE_DBM		0x80000
#define MOD_TYPE_MT2		0x100000
#define MOD_TYPE_AMF0		0x200000
#define MOD_TYPE_PSM		0x400000
#define MOD_TYPE_J2B		0x800000
#define MOD_TYPE_MPT		0x1000000
#define MOD_TYPE_UMX		0x80000000 // Fake type
#define MAX_MODTYPE			24

//Note: These are bit indeces. MSF <-> Mod(Specific)Flag.
//If changing these, ChangeModTypeTo() might need modification.
const BYTE MSF_IT_COMPATIBLE_PLAY	= 0;		//IT/MPT
const BYTE MSF_OLDVOLSWING			= 1;		//IT/MPT
const BYTE MSF_MIDICC_BUGEMULATION	= 2;		//IT/MPT/XM

//Simple struct to gather various modspecifications in one place.

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
