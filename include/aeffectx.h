#ifndef __aeffectx__
#define __aeffectx__

#ifndef __AEffect__
#include "AEffect.h"
#endif

//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// version 2.0 extension
// (c)1999 Steinberg Soft+Hardware GmbH
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// VstEvent
//-------------------------------------------------------------------------------------------------------

typedef struct VstEvent VstEvent;
typedef struct VstMidiEvent	VstMidiEvent;
typedef struct VstEvents VstEvents;

struct VstEvent			// a generic timestamped event
{
	long type;			// see enum below
	long byteSize;		// of this event, excl. type and byteSize
	long deltaFrames;	// sample frames related to the current block start sample position
	long flags;			// generic flags, none defined yet (0)

	char data[16];		// size may vary but is usually 16
};

enum					// VstEvent types
{
	kVstMidiType = 1,	// midi event, can be cast as VstMidiEvent (see below)
	kVstAudioType,		// audio
	kVstVideoType,		// video
	kVstParameterType,	// parameter
	kVstTriggerType		// trigger
	// ...etc
};

struct VstMidiEvent		// to be casted from a VstEvent
{
	long type;			// kVstMidiType
	long byteSize;		// 24
	long deltaFrames;	// sample frames related to the current block start sample position
	long flags;			// none defined yet

	long noteLength;	// (in sample frames) of entire note, if available, else 0
	long noteOffset;	// offset into note from note start if available, else 0

	char midiData[4];	// 1 thru 3 midi bytes; midiData[3] is reserved (zero)
	char detune;		// -64 to +63 cents; for scales other than 'well-tempered' ('microtuning')
	char noteOffVelocity;
	char reserved1;		// zero
	char reserved2;		// zero
};

struct VstEvents			// a block of events for the current audio block
{
	long numEvents;
	long reserved;			// zero
	VstEvent* events[2];	// variable
};

//-------------------------------------------------------------------------------------------------------
// VstTimeInfo
//-------------------------------------------------------------------------------------------------------

typedef struct VstTimeInfo VstTimeInfo;

// VstTimeInfo as requested via audioMasterGetTime (getTimeInfo())
// refers to the current time slice. note the new slice is
// already started when processEvents() is called

struct VstTimeInfo
{
	double samplePos;			// current location
	double sampleRate;
	double nanoSeconds;			// system time
	double ppqPos;				// 1 ppq
	double tempo;				// in bpm
	double barStartPos;			// last bar start, in 1 ppq
	double cycleStartPos;		// 1 ppq
	double cycleEndPos;			// 1 ppq
	long timeSigNumerator;		// time signature
	long timeSigDenominator;
	long smpteOffset;
	long smpteFrameRate;		// 0:24, 1:25, 2:29.97, 3:30, 4:29.97 df, 5:30 df
	long samplesToNextClock;	// midi clock resolution (24 ppq), can be negative
	long flags;					// see below
};

enum
{
	kVstTransportChanged 		= 1,
	kVstTransportPlaying 		= 1 << 1,
	kVstTransportCycleActive	= 1 << 2,

	kVstAutomationWriting		= 1 << 6,
	kVstAutomationReading		= 1 << 7,

	// flags which indicate which of the fields in this VstTimeInfo
	//  are valid; samplePos and sampleRate are always valid
	kVstNanosValid  			= 1 << 8,
	kVstPpqPosValid 			= 1 << 9,
	kVstTempoValid				= 1 << 10,
	kVstBarsValid				= 1 << 11,
	kVstCyclePosValid			= 1 << 12,	// start and end
	kVstTimeSigValid 			= 1 << 13,
	kVstSmpteValid				= 1 << 14,
	kVstClockValid 				= 1 << 15
};

//-------------------------------------------------------------------------------------------------------
// VarIo
//-------------------------------------------------------------------------------------------------------

typedef struct VstVariableIo VstVariableIo;

struct VstVariableIo
{
	float **inputs;
	float **outputs;
	long numSamplesInput;
	long numSamplesOutput;
	long *numSamplesInputProcessed;
	long *numSamplesOutputProcessed;
};

//---------------------------------------------------------------------------------------------
// new audioMaster opCodes
//---------------------------------------------------------------------------------------------

enum
{
	// VstEvents + VstTimeInfo
	audioMasterWantMidi = audioMasterPinConnected + 2,	// <value> is a filter which is currently ignored
	audioMasterGetTime,				// returns const VstTimeInfo* (or 0 if not supported)
									// <value> should contain a mask indicating which fields are required
									// (see valid masks above), as some items may require extensive
									// conversions
	audioMasterProcessEvents,		// VstEvents* in <ptr>
	audioMasterSetTime,				// VstTimenfo* in <ptr>, filter in <value>, not supported
	audioMasterTempoAt,				// returns tempo (in bpm * 10000) at sample frame location passed in <value>

	// parameters
	audioMasterGetNumAutomatableParameters,
	audioMasterGetParameterQuantization,	// returns the integer value for +1.0 representation,
											// or 1 if full single float precision is maintained
											// in automation. parameter index in <value> (-1: all, any)
	// connections, configuration
	audioMasterIOChanged,				// numInputs and/or numOutputs has changed
	audioMasterNeedIdle,				// plug needs idle calls (outside its editor window)
	audioMasterSizeWindow,				// index: width, value: height
	audioMasterGetSampleRate,
	audioMasterGetBlockSize,
	audioMasterGetInputLatency,
	audioMasterGetOutputLatency,
	audioMasterGetPreviousPlug,			// input pin in <value> (-1: first to come), returns cEffect*
	audioMasterGetNextPlug,				// output pin in <value> (-1: first to come), returns cEffect*

	// realtime info
	audioMasterWillReplaceOrAccumulate,	// returns: 0: not supported, 1: replace, 2: accumulate
	audioMasterGetCurrentProcessLevel,	// returns: 0: not supported,
										// 1: currently in user thread (gui)
										// 2: currently in audio thread (where process is called)
										// 3: currently in 'sequencer' thread (midi, timer etc)
										// 4: currently offline processing and thus in user thread
										// other: not defined, but probably pre-empting user thread.
	audioMasterGetAutomationState,		// returns 0: not supported, 1: off, 2:read, 3:write, 4:read/write

	// offline
	audioMasterOfflineStart,
	audioMasterOfflineRead,				// ptr points to offline structure, see below. return 0: error, 1 ok
	audioMasterOfflineWrite,			// same as read
	audioMasterOfflineGetCurrentPass,
	audioMasterOfflineGetCurrentMetaPass,

	// other
	audioMasterSetOutputSampleRate,		// for variable i/o, sample rate in <opt>
	audioMasterGetSpeakerArrangement,	// (long)input in <value>, output in <ptr>
	audioMasterGetVendorString,			// fills <ptr> with a string identifying the vendor (max 64 char)
	audioMasterGetProductString,		// fills <ptr> with a string with product name (max 64 char)
	audioMasterGetVendorVersion,		// returns vendor-specific version
	audioMasterVendorSpecific,			// no definition, vendor specific handling
	audioMasterSetIcon,					// void* in <ptr>, format not defined yet
	audioMasterCanDo,					// string in ptr, see below
	audioMasterGetLanguage,				// see enum
	audioMasterOpenWindow,				// returns platform specific ptr
	audioMasterCloseWindow,				// close window, platform specific handle in <ptr>
	audioMasterGetDirectory,			// get plug directory, FSSpec on MAC, else char*
	audioMasterUpdateDisplay			// something has changed, update 'multi-fx' display
};

enum VstHostLanguage
{
	kVstLangEnglish = 1,
	kVstLangGerman,
	kVstLangFrench,
	kVstLangItalian,
	kVstLangSpanish,
	kVstLangJapanese
};

//---------------------------------------------------------------------------------------------
// dispatcher opCodes
//---------------------------------------------------------------------------------------------

enum
{
	// VstEvents
	effProcessEvents = effSetChunk + 1,		// VstEvents* in <ptr>

	// parameters and programs
	effCanBeAutomated,						// parameter index in <index>
	effString2Parameter,					// parameter index in <index>, string in <ptr>
	effGetNumProgramCategories,				// no arguments. this is for dividing programs into groups (like GM)
	effGetProgramNameIndexed,				// get program name of category <value>, program <index> into <ptr>.
											// category (that is, <value>) may be -1, in which case program indices
											// are enumerated linearily (as usual); otherwise, each category starts
											// over with index 0.
	effCopyProgram,							// copy current program to destination <index>
											// note: implies setParameter	
	// connections, configuration
	effConnectInput,						// input at <index> has been (dis-)connected;
											// <value> == 0: disconnected, else connected
	effConnectOutput,						// same as input
	effGetInputProperties,					// <index>, VstPinProperties* in ptr, return != 0 => true
	effGetOutputProperties,					// dto
	effGetPlugCategory,						// no parameter, return value is category

	// realtime
	effGetCurrentPosition,					// for external dsp, see flag bits below
	effGetDestinationBuffer,				// for external dsp, see flag bits below. returns float*

	// offline
	effOfflineNotify,						// ptr = VstAudioFile array, value = count, index = start flag
	effOfflinePrepare,						// ptr = VstOfflineTask array, value = count
	effOfflineRun,							// dto

	// other
	effProcessVarIo,						// VstVariableIo* in <ptr>
	effSetSpeakerArrangement,				// VstSpeakerArrangement* pluginInput in <value>
											// VstSpeakerArrangement* pluginOutput in <ptr>
	effSetBlockSizeAndSampleRate,			// block size in <value>, sampleRate in <opt>
	effSetBypass,							// onOff in <value> (0 = off)
	effGetEffectName,						// char* name (max 32 bytes) in <ptr> 
	effGetErrorText,						// char* text (max 256 bytes) in <ptr> 
	effGetVendorString,						// fills <ptr> with a string identifying the vendor (max 64 char)
	effGetProductString,					// fills <ptr> with a string with product name (max 64 char)
	effGetVendorVersion,					// returns vendor-specific version
	effVendorSpecific,						// no definition, vendor specific handling
	effCanDo,								// <ptr>
	effGetTailSize,							// returns tail size; 0 is default (return 1 for 'no tail')
	effIdle,								// idle call in response to audioMasterneedIdle. must
											// return 1 to keep idle calls beeing issued

	// gui
	effGetIcon,								// void* in <ptr>, not yet defined
	effSetViewPosition,						// set view position (in window) to x <index> y <value>

	// and...
	effGetParameterProperties,				// of param <index>, VstParameterProperties* in <ptr>
	effKeysRequired,						// returns 0: needs keys (default for 1.0 plugs), 1: don't need
	effGetVstVersion,						// returns 2; older versions return 0

	effNumV2Opcodes
	// note that effNumOpcodes doesn't apply anymore
};

typedef struct VstParameterProperties VstParameterProperties;
typedef struct VstPinProperties VstPinProperties;

struct VstParameterProperties
{
	float stepFloat;
	float smallStepFloat;
	float largeStepFloat;
	char label[64];
	long flags;
	long minInteger;
	long maxInteger;
	long stepInteger;
	long largeStepInteger;
	char shortLabel[8];	// recommended: 6 + delimiter
	char future[48];
};

// parameter properties flags
enum
{
	kVstParameterIsSwitch			= 1 << 0,
	kVstParameterUsesIntegerMinMax	= 1 << 1,
	kVstParameterUsesFloatStep		= 1 << 2,
	kVstParameterUsesIntStep		= 1 << 3
};

struct VstPinProperties
{
	char label[64];
	long flags;
	long reserved;
	char shortLabel[8];	// recommended: 6 + delimiter
	char future[48];
};

// pin properties flags
enum
{
	kVstPinIsActive = 1 << 0,
	kVstPinIsStereo = 1 << 1
};

// category
enum VstPlugCategory
{
    kPlugCategUnknown = 0,
    kPlugCategEffect,
    kPlugCategSynth,
    kPlugCategAnalysis,
    kPlugCategMastering,
	kPlugCategSpacializer,	// 'panners'
	kPlugCategRoomFx,		// delays and reverbs
	kPlugSurroundFx			// dedicated surround processor
};

//---------------------------------------------------------------------------------------------
// flags bits
//---------------------------------------------------------------------------------------------

enum
{
	effFlagsIsSynth =		1 << 8,		// host may assign mixer channels for its outputs
	effFlagsNoSoundInStop = 1 << 9,		// does not produce sound when input is all silence
	effFlagsExtIsAsync =	1 << 10,	// for external dsp; plug returns immedeately from process()
										// host polls plug position (current block) via effGetCurrentPosition
	effFlagsExtHasBuffer =	1 << 11		// external dsp, may have their own output buffe (32 bit float)
										// host then requests this via effGetDestinationBuffer
};

//---------------------------------------------------------------------------------------------
// surround setup
//---------------------------------------------------------------------------------------------

typedef struct VstSpeakerProperties VstSpeakerProperties;
typedef struct VstSpeakerArrangement VstSpeakerArrangement;

struct VstSpeakerProperties
{ 						// units:	range:			except:
	float azimuth;		// rad		-PI...PI		10.f for LFE channel
	float elevation;	// rad		-PI/2...PI/2	10.f for LFE channel
	float radius;		// meter					0.f for LFE channel
	float reserved;		// 0.
	char  name[64];		// for new setups, new names should be given (L/R/C... won't do)
	char future[32];
};

// note: the origin for azimuth is right (as by math conventions dealing with radians);
// the elevation origin is also right, visualizing a rotation of a circle across the
// -pi/pi axis of the horizontal circle. thus, an elevation of -pi/2 corresponds
// to bottom, and a speaker standing on the left, and 'beaming' upwards would have
// an azimuth of -pi, and an elevation of pi/2.
// for user interface representation, grads are more likely to be used, and the
// origins will obviously 'shift' accordingly.

struct VstSpeakerArrangement
{
	float lfeGain;			// LFE channel gain is adjusted [dB] higher than other channels
	long numChannels;		// number of channels in this speaker arrangement
	VstSpeakerProperties speakers[8];	// variable
};

//---------------------------------------------------------------------------------------------
// offline
//---------------------------------------------------------------------------------------------

typedef struct VstOfflineTask VstOfflineTask;
typedef struct VstAudioFile VstAudioFile;
typedef struct VstAudioFileMarker VstAudioFileMarker;

struct VstOfflineTask
{
	char	processName[96];	// set by plug

	// audio access
	double	readPosition;		// set by plug/host
	double	writePosition;		// set by plug/host
	long	readCount;			// set by plug/host
	long	writeCount;			// set by plug
	long	sizeInputBuffer;	// set by host
	long	sizeOutputBuffer;	// set by host
	void*	inputBuffer;		// set by host
	void*	outputBuffer;		// set by host
	double	positionToProcessFrom;	// set by host
	double	numFramesToProcess;	// set by host
	double	maxFramesToWrite;	// set by plug

	// other data access
	void*	extraBuffer;		// set by plug
	long	value;				// set by host or plug
	long	index;				// set by host or plug

	// file attributes
	double	numFramesInSourceFile;	// set by host
	double	sourceSampleRate;		// set by host or plug
	double	destinationSampleRate;	// set by host or plug
	long	numSourceChannels;		// set by host or plug
	long	numDestinationChannels;	// set by host or plug
	long	sourceFormat;			// set by host
	long	destinationFormat;		// set by plug
	char	outputText[512];		// set by plug or host

	// progress notification
	double	progress;				// set by plug
	long	progressMode;			// reserved for future
	char	progressText[100];		// set by plug

	long	flags;					// set by host and plug; see VstOfflineTaskFlags
	long	returnValue;			// reserved for future
	void*	hostOwned;				// set by host
	void*	plugOwned;				// set by plug

	char	future[1024];
};

enum VstOfflineTaskFlags
{
	// set by host
	kVstOfflineUnvalidParameter	= 1 << 0,
	kVstOfflineNewFile			= 1 << 1,

	// set by plug
	kVstOfflinePlugError		= 1 << 10,
	kVstOfflineInterleavedAudio	= 1 << 11,
	kVstOfflineTempOutputFile	= 1 << 12,
	kVstOfflineFloatOutputFile	= 1 << 13,
	kVstOfflineRandomWrite		= 1 << 14,
	kVstOfflineStretch			= 1 << 15,
	kVstOfflineNoThread			= 1 << 16
};

// option passed to offlineRead/offlineWrite

enum VstOfflineOption
{
   kVstOfflineAudio,		// reading/writing audio samples
   kVstOfflinePeaks,		// reading graphic representation
   kVstOfflineParameter,	// reading/writing parameters
   kVstOfflineMarker,		// reading/writing marker
   kVstOfflineCursor,		// reading/moving edit cursor
   kVstOfflineSelection,	// reading/changing selection
   kVstOfflineQueryFiles	// to request the host to call asynchronously offlineNotify
};

// structure passed to offlineNotify and offlineStart

struct VstAudioFile
{
	long	flags;				// see enum VstAudioFileFlags
	void*	hostOwned;			// any data private to host
	void*	plugOwned;			// any data private to plugin
	char	name[100];			// file title
	long	uniqueId;			// uniquely identify a file during a session
	double	sampleRate;			// file sample rate
	long	numChannels;		// number of channels (1 for mono, 2 for stereo...)
	double	numFrames;			// number of frames in the audio file
	long	format;				// reserved for future
	double	editCursorPosition;	// -1 if no such cursor
	double	selectionStart;		// frame index of first selected frame, or -1
	double	selectionSize;		// number of frames in selection, or 0
	long	selectedChannelsMask;	// 1 bit per channel
	long	numMarkers;			// number of markers in the file
	long	timeRulerUnit;		// see doc for possible values
	double	timeRulerOffset;	// offset in time ruler (positive or negative)
	double	tempo;				// as bpm
	long	timeSigNumerator;	// time signature numerator
	long	timeSigDenominator;	// time signature denominator
	long	ticksPerBlackNote;	// resolution
	long	smpteFrameRate;		// smpte rate (set as in VstTimeInfo)

	char	future[64];
};

enum VstAudioFileFlags
{
	// set by host (in call offlineNotify)
	kVstOfflineReadOnly				= 1 << 0,
	kVstOfflineNoRateConversion		= 1 << 1,
	kVstOfflineNoChannelChange		= 1 << 2,

	// Set by plug (in function offlineStart)
	kVstOfflineCanProcessSelection	= 1 << 10,
	kVstOfflineNoCrossfade			= 1 << 11,
	kVstOfflineWantRead				= 1 << 12,
	kVstOfflineWantWrite			= 1 << 13,
	kVstOfflineWantWriteMarker		= 1 << 14,
	kVstOfflineWantMoveCursor		= 1 << 15,
	kVstOfflineWantSelect			= 1 << 16
};

struct VstAudioFileMarker
{
	double	position;
	char	name[32];
	long	type;
	long	id;
	long	reserved;
};

//---------------------------------------------------------------------------------------------
// others
//---------------------------------------------------------------------------------------------

// structure passed to openWindow and closeWindow

struct VstWindow
{
	char  title[128];    // title
	short xPos;          // position and size
	short yPos;
	short width;
	short height;
	long  style;         // 0: with title, 1: without title

	void *parent;        // parent of this window
	void *userHandle;    // reserved
	void *winHandle;     // reserved
 
	char future[104];
};

#endif

