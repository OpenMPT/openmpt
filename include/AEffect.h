#ifndef __AEffect__
#define __AEffect__

/*
	to create an Audio Effect for power pc's, create a
	code resource
	file type: 'aPcs'
	resource type: 'aEff'
	ppc header: none (raw pef)

	for windows, it's a .dll

	the only symbol searched for is:
	AEffect *main(float (*audioMaster)(AEffect *effect, long opcode, long index,
		long value, void *ptr, float opt));
*/

#if PRAGMA_ALIGN_SUPPORTED || __MWERKS__
	#pragma options align=mac68k
#elif defined CBUILDER
	#pragma -a8
#elif defined(WIN32) || defined(__FLAT__)
	#pragma pack(push)
	#pragma pack(8)
#endif

#if defined(WIN32) || defined(__FLAT__) || defined CBUILDER
	#define VSTCALLBACK __cdecl
#else
	#define VSTCALLBACK
#endif

//---------------------------------------------------------------------------------------------
// misc def's
//---------------------------------------------------------------------------------------------

typedef struct AEffect AEffect;
typedef	long (VSTCALLBACK *audioMasterCallback)(AEffect *effect, long opcode, long index,
		long value, void *ptr, float opt);

// prototype for plug-in main
// AEffect *main(audioMasterCallback audioMaster);

#ifdef CBUILDER
	#define kEffectMagic 'PtsV'
#else
	#define kEffectMagic 'VstP'
#endif

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

struct AEffect
{
	long magic;			// must be kEffectMagic ('VstP')
	long (VSTCALLBACK *dispatcher)(AEffect *effect, long opCode, long index, long value,
		void *ptr, float opt);
	void (VSTCALLBACK *process)(AEffect *effect, float **inputs, float **outputs, long sampleframes);
	void (VSTCALLBACK *setParameter)(AEffect *effect, long index, float parameter);
	float (VSTCALLBACK *getParameter)(AEffect *effect, long index);

	long numPrograms;
	long numParams;		// all programs are assumed to have numParams parameters
	long numInputs;		//
	long numOutputs;	//
	long flags;			// see constants
	long resvd1;		// reserved, must be 0
	long resvd2;		// reserved, must be 0
	long initialDelay;	// for algorithms which need input in the first place
	long realQualities;	// number of realtime qualities (0: realtime)
	long offQualities;	// number of offline qualities (0: realtime only)
	float ioRatio;		// input samplerate to output samplerate ratio, not used yet
	void *object;		// for class access (see AudioEffect.hpp), MUST be 0 else!
	void *user;			// user access
	long uniqueID;		// pls choose 4 character as unique as possible.
						// this is used to identify an effect for save+load
	long version;		//
	void (VSTCALLBACK *processReplacing)(AEffect *effect, float **inputs, float **outputs, long sampleframes);
	char future[60];	// pls zero
};


//---------------------------------------------------------------------------------------------
// flags bits
//---------------------------------------------------------------------------------------------

#define effFlagsHasEditor		1	// if set, is expected to react to editor messages
#define effFlagsHasClip			2	// return > 1. in getVu() if clipped
#define effFlagsHasVu			4	// return vu value in getVu(); > 1. means clipped
#define effFlagsCanMono			8	// if numInputs == 2, makes sense to be used for mono in
#define effFlagsCanReplacing	16	// supports in place output (processReplacing() exsists)
#define effFlagsProgramChunks	32	// program data are handled in formatless chunks

//---------------------------------------------------------------------------------------------
// dispatcher opCodes
//---------------------------------------------------------------------------------------------

enum
{
	effOpen = 0,		// initialise
	effClose,			// exit, release all memory and other resources!

	effSetProgram,		// program no in <value>
	effGetProgram,		// return current program no.
	effSetProgramName,	// user changed program name (max 24 char + 0) to as passed in string 
	effGetProgramName,	// stuff program name (max 24 char + 0) into string 

	effGetParamLabel,	// stuff parameter <index> label (max 8 char + 0) into string
						// (examples: sec, dB, type)
	effGetParamDisplay,	// stuff parameter <index> textual representation into string
						// (examples: 0.5, -3, PLATE)
	effGetParamName,	// stuff parameter <index> label (max 8 char + 0) into string
						// (examples: Time, Gain, RoomType) 
	effGetVu,			// called if (flags & (effFlagsHasClip | effFlagsHasVu))

	// system

	effSetSampleRate,	// in opt (float)
	effSetBlockSize,	// in value
	effMainsChanged,	// the user has switched the 'power on' button to
						// value (0 off, else on). This only switches audio
						// processing; you should flush delay buffers etc.
	// editor
	
	effEditGetRect,		// stuff rect (top, left, bottom, right) into ptr
	effEditOpen,		// system dependant Window pointer in ptr
	effEditClose,		// no arguments
	effEditDraw,		// draw method, ptr points to rect
	effEditMouse,		// index: x, value: y
	effEditKey,			// system keycode in value
	effEditIdle,		// no arguments. Be gentle!
	effEditTop,			// window has topped, no arguments
	effEditSleep,		// window goes to background

	// new
	
	effIdentify,		// returns 'NvEf'
	effGetChunk,		// host requests pointer to chunk into (void**)ptr, byteSize returned
	effSetChunk,		// plug-in receives saved chunk, byteSize passed
 
	effNumOpcodes		
};

//---------------------------------------------------------------------------------------------
// audioMaster opCodes
//---------------------------------------------------------------------------------------------

enum
{
	audioMasterAutomate = 0,		// index, value, returns 0
	audioMasterVersion,				// vst version, currently 2 (0 for older)
	audioMasterCurrentId,			// returns the unique id of a plug that's currently
									// loading
	audioMasterIdle,				// call application idle routine (this will
									// call effEditIdle for all open editors too) 
	audioMasterPinConnected			// inquire if an input or output is beeing connected;
									// index enumerates input or output counting from zero,
									// value is 0 for input and != 0 otherwise. note: the
									// return value is 0 for <true> such that older versions
									// will always return true.
	
};

#if PRAGMA_ALIGN_SUPPORTED || __MWERKS__
	#pragma options align=reset
#elif defined(WIN32) || defined(__FLAT__)
	#pragma pack(pop)
#elif defined CBUILDER
	#pragma -a-
#endif

#endif	// __AEffect__
