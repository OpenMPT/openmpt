/*
 * Snd_Defs.h
 * ----------
 * Purpose: Basic definitions of data types, enums, etc. for the playback engine core.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/typedefs.h"

#ifndef LPCBYTE
typedef const BYTE * LPCBYTE;
#endif

typedef uint32 ROWINDEX;
	const ROWINDEX ROWINDEX_MAX = uint32_max;
	const ROWINDEX ROWINDEX_INVALID = ROWINDEX_MAX;
typedef uint16 CHANNELINDEX;
	const CHANNELINDEX CHANNELINDEX_MAX	= uint16_max;
	const CHANNELINDEX CHANNELINDEX_INVALID	= CHANNELINDEX_MAX;
typedef uint16 ORDERINDEX;
	const ORDERINDEX ORDERINDEX_MAX	= uint16_max;
	const ORDERINDEX ORDERINDEX_INVALID	= ORDERINDEX_MAX;
typedef uint16 PATTERNINDEX;
	const PATTERNINDEX PATTERNINDEX_MAX	= uint16_max;
	const PATTERNINDEX PATTERNINDEX_INVALID	= PATTERNINDEX_MAX;
typedef uint8  PLUGINDEX;
	const PLUGINDEX PLUGINDEX_INVALID = uint8_max;
typedef uint16 TEMPO;
typedef uint16 SAMPLEINDEX;
	const SAMPLEINDEX SAMPLEINDEX_MAX	= uint16_max;
	const SAMPLEINDEX SAMPLEINDEX_INVALID	= SAMPLEINDEX_MAX;
typedef uint16 INSTRUMENTINDEX;
	const INSTRUMENTINDEX INSTRUMENTINDEX_MAX	= uint16_max;
	const INSTRUMENTINDEX INSTRUMENTINDEX_INVALID	= INSTRUMENTINDEX_MAX;
typedef uint8 SEQUENCEINDEX;
	const SEQUENCEINDEX SEQUENCEINDEX_MAX	= uint8_max;
	const SEQUENCEINDEX SEQUENCEINDEX_INVALID	= SEQUENCEINDEX_MAX;

typedef uintptr_t SmpLength;



#define MOD_AMIGAC2			0x1AB					// Period of Amiga middle-c
const SmpLength MAX_SAMPLE_LENGTH	= 0x10000000;	// Sample length in *samples*
													// Note: Sample size in bytes can be more than this (= 256 MB).
#define MAX_SAMPLE_RATE		192000					// Max playback / render rate in Hz

const ROWINDEX MAX_PATTERN_ROWS			= 1024;	// -> CODE#0008 -> DESC="#define to set pattern size" -! BEHAVIOUR_CHANGE#0008
const ORDERINDEX MAX_ORDERS				= 256;
const PATTERNINDEX MAX_PATTERNS			= 240;
const SAMPLEINDEX MAX_SAMPLES			= 4000;
const INSTRUMENTINDEX MAX_INSTRUMENTS	= 256;	//200
const PLUGINDEX MAX_MIXPLUGINS			= 100;	//50

const SEQUENCEINDEX MAX_SEQUENCES		= 50;

const CHANNELINDEX MAX_BASECHANNELS		= 127;	// Max pattern channels.
const CHANNELINDEX MAX_CHANNELS			= 256;	//200 // Maximum number of mixing channels.

#define MIN_PERIOD			0x0020	// Note: Period is an Amiga metric that is inverse to frequency.
#define MAX_PERIOD			0xFFFF	// Periods in MPT are 4 times as fine as Amiga periods because of extra fine frequency slides.

// String lengths (including trailing null char)
#define MAX_SAMPLENAME			32	// also affects module name!
#define MAX_SAMPLEFILENAME		22
#define MAX_INSTRUMENTNAME		32
#define MAX_INSTRUMENTFILENAME	32
#define MAX_PATTERNNAME			32
#define MAX_CHANNELNAME			20

#define MAX_EQ_BANDS		6

#define MAX_PLUGPRESETS		1000 //rewbs.plugPresets

enum MODTYPE
{
	MOD_TYPE_NONE	= 0x00,
	MOD_TYPE_MOD	= 0x01,
	MOD_TYPE_S3M	= 0x02,
	MOD_TYPE_XM		= 0x04,
	MOD_TYPE_MED	= 0x08,
	MOD_TYPE_MTM	= 0x10,
	MOD_TYPE_IT		= 0x20,
	MOD_TYPE_669	= 0x40,
	MOD_TYPE_ULT	= 0x80,
	MOD_TYPE_STM	= 0x100,
	MOD_TYPE_FAR	= 0x200,
	MOD_TYPE_WAV	= 0x400,
	MOD_TYPE_AMF	= 0x800,
	MOD_TYPE_AMS	= 0x1000,
	MOD_TYPE_DSM	= 0x2000,
	MOD_TYPE_MDL	= 0x4000,
	MOD_TYPE_OKT	= 0x8000,
	MOD_TYPE_MID	= 0x10000,
	MOD_TYPE_DMF	= 0x20000,
	MOD_TYPE_PTM	= 0x40000,
	MOD_TYPE_DBM	= 0x80000,
	MOD_TYPE_MT2	= 0x100000,
	MOD_TYPE_AMF0	= 0x200000,
	MOD_TYPE_PSM	= 0x400000,
	MOD_TYPE_J2B	= 0x800000,
	MOD_TYPE_MPT	= 0x1000000,
	MOD_TYPE_IMF	= 0x2000000,
	MOD_TYPE_UMX	= 0x80000000, // Fake type
};

// Allow for type safe combinations of MODTYPEs.
inline MODTYPE operator | (MODTYPE a, MODTYPE b)
{
	return static_cast<MODTYPE>(+a | +b);
};

inline MODTYPE operator & (MODTYPE a, MODTYPE b)
{
	return static_cast<MODTYPE>(+a & +b);
};

// For compatibility mode
#define TRK_IMPULSETRACKER	(MOD_TYPE_IT | MOD_TYPE_MPT)
#define TRK_FASTTRACKER2	(MOD_TYPE_XM)
#define TRK_SCREAMTRACKER	(MOD_TYPE_S3M)
#define TRK_PROTRACKER		(MOD_TYPE_MOD)
#define TRK_ALLTRACKERS		(TRK_IMPULSETRACKER | TRK_FASTTRACKER2 | TRK_SCREAMTRACKER | TRK_PROTRACKER)



// Channel flags:
// Bits 0-7:	Sample Flags
#define CHN_16BIT			0x01		// 16-bit sample
#define CHN_LOOP			0x02		// looped sample
#define CHN_PINGPONGLOOP	0x04		// bidi-looped sample
#define CHN_SUSTAINLOOP		0x08		// sample with sustain loop
#define CHN_PINGPONGSUSTAIN	0x10		// sample with bidi sustain loop
#define CHN_PANNING			0x20		// sample with forced panning
#define CHN_STEREO			0x40		// stereo sample
#define CHN_PINGPONGFLAG	0x80		// when flag is on, sample is processed backwards
// Bits 8-31:	Channel Flags
#define CHN_MUTE			0x100		// muted channel
#define CHN_KEYOFF			0x200		// exit sustain
#define CHN_NOTEFADE		0x400		// fade note (instrument mode)
#define CHN_SURROUND		0x800		// use surround channel
#define CHN_NOIDO			0x1000		// Indicates if the channel is near enough to an exact multiple of the base frequency that any interpolation won't be noticeable - or if interpolation was switched off completely. --Storlek
#define CHN_HQSRC			0x2000		// High quality sample rate conversion (i.e. apply interpolation)
#define CHN_FILTER			0x4000		// filtered output
#define CHN_VOLUMERAMP		0x8000		// ramp volume
#define CHN_VIBRATO			0x10000		// apply vibrato
#define CHN_TREMOLO			0x20000		// apply tremolo
#define CHN_PANBRELLO		0x40000		// apply panbrello
#define CHN_PORTAMENTO		0x80000		// apply portamento
#define CHN_GLISSANDO		0x100000	// glissando mode
#define CHN_FASTVOLRAMP		0x200000	// ramp volume very fast
#define CHN_EXTRALOUD		0x400000	// force master volume to 0x100
#define CHN_REVERB			0x800000	// apply reverb
#define CHN_NOREVERB		0x1000000	// forbid reverb
#define CHN_SOLO			0x2000000	// solo channel -> CODE#0012 -> DESC="midi keyboard split" -! NEW_FEATURE#0012
#define CHN_NOFX			0x4000000	// dry channel -> CODE#0015 -> DESC="channels management dlg" -! NEW_FEATURE#0015
#define CHN_SYNCMUTE		0x8000000	// keep sample sync on mute

#define CHN_SAMPLEFLAGS		(CHN_16BIT|CHN_LOOP|CHN_PINGPONGLOOP|CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN|CHN_PANNING|CHN_STEREO|CHN_PINGPONGFLAG)
#define CHN_CHANNELFLAGS	(~CHN_SAMPLEFLAGS)

// instrument envelope-specific flags
#define ENV_ENABLED			0x01	// env is enabled
#define ENV_LOOP			0x02	// env loop
#define ENV_SUSTAIN			0x04	// env sustain
#define ENV_CARRY			0x08	// env carry
#define ENV_FILTER			0x10	// filter env enabled (this has to be combined with ENV_ENABLED in the pitch envelope's flags)

// Envelope value boundaries
#define ENVELOPE_MIN		0		// vertical min value of a point
#define ENVELOPE_MID		32		// vertical middle line
#define ENVELOPE_MAX		64		// vertical max value of a point
#define MAX_ENVPOINTS		240		// Maximum length of each instrument envelope
#define ENVELOPE_MAX_LENGTH 0x3FFF	// max envelope length in ticks. note: this value seems to be conservatively low...


// Flags of 'dF..' datafield in extended instrument properties.
#define dFdd_VOLUME 		0x0001
#define dFdd_VOLSUSTAIN 	0x0002
#define dFdd_VOLLOOP 		0x0004
#define dFdd_PANNING 		0x0008
#define dFdd_PANSUSTAIN 	0x0010
#define dFdd_PANLOOP 		0x0020
#define dFdd_PITCH 			0x0040
#define dFdd_PITCHSUSTAIN 	0x0080
#define dFdd_PITCHLOOP 		0x0100
#define dFdd_SETPANNING 	0x0200
#define dFdd_FILTER 		0x0400
#define dFdd_VOLCARRY 		0x0800
#define dFdd_PANCARRY 		0x1000
#define dFdd_PITCHCARRY 	0x2000
#define dFdd_MUTE 			0x4000


// instrument-specific flags
#define INS_SETPANNING		0x01	// panning enabled
#define INS_MUTE			0x02	// instrument is muted


// envelope types in instrument editor
enum enmEnvelopeTypes
{
	ENV_VOLUME = 1,
	ENV_PANNING = 2,
	ENV_PITCH = 3,
};

// Filter Modes
#define FLTMODE_UNCHANGED		0xFF
#define FLTMODE_LOWPASS			0
#define FLTMODE_HIGHPASS		1


// NNA types (New Note Action)
#define NNA_NOTECUT		0
#define NNA_CONTINUE	1
#define NNA_NOTEOFF		2
#define NNA_NOTEFADE	3

// DCT types (Duplicate Check Types)
#define DCT_NONE		0
#define DCT_NOTE		1
#define DCT_SAMPLE		2
#define DCT_INSTRUMENT	3
#define DCT_PLUGIN		4 //rewbs.VSTiNNA

// DNA types (Duplicate Note Action)
#define DNA_NOTECUT		0
#define DNA_NOTEOFF		1
#define DNA_NOTEFADE	2

// Mixer Hardware-Dependent features
#define SYSMIX_ENABLEMMX	0x01		// Hardware acceleration features (MMX/3DNow!/SSE) are supported by this processor
#define SYSMIX_SLOWCPU		0x02		// *Really* old processor (in this context, it doesn't know CPUID instructions => Must be older than Pentium)
#define SYSMIX_FASTCPU		0x04		// "Fast" processor (in this context, anything that is at least a Pentium MMX)
#define SYSMIX_MMXEX		0x08		// Processor supports AMD MMX extensions
#define SYSMIX_3DNOW		0x10		// Processor supports AMD 3DNow! instructions
#define SYSMIX_SSE			0x20		// Processor supports SSE instructions

// Module flags
enum SongFlags
{
	SONG_EMBEDMIDICFG	= 0x0001,		// Embed macros in file
	SONG_FASTVOLSLIDES	= 0x0002,		// Old Scream Tracker 3.0 volume slides
	SONG_ITOLDEFFECTS	= 0x0004,		// Old Impulse Tracker effect implementations
	SONG_ITCOMPATGXX	= 0x0008,		// IT "Compatible Gxx" (IT's flag to behave more like other trackers w/r/t portamento effects)
	SONG_LINEARSLIDES	= 0x0010,		// Linear slides vs. Amiga slides
	SONG_PATTERNLOOP	= 0x0020,		// Loop current pattern (pattern editor)
	SONG_STEP			= 0x0040,		// Song is in "step" mode (pattern editor)
	SONG_PAUSED			= 0x0080,		// Song is paused
	SONG_FADINGSONG		= 0x0100,		// Song is fading out
	SONG_ENDREACHED		= 0x0200,		// Song is finished
	SONG_GLOBALFADE		= 0x0400,		// Song is fading out
	SONG_CPUVERYHIGH	= 0x0800,		// High CPU usage
	SONG_FIRSTTICK		= 0x1000,		// Is set when the current tick is the first tick of the row
	SONG_MPTFILTERMODE	= 0x2000,		// Local filter mode (reset filter on each note)
	SONG_SURROUNDPAN	= 0x4000,		// Pan in the rear channels
	SONG_EXFILTERRANGE	= 0x8000,		// Cutoff Filter has double frequency range (up to ~10Khz)
	SONG_AMIGALIMITS	= 0x10000,		// Enforce amiga frequency limits
	// -> CODE#0023
	// -> DESC="IT project files (.itp)"
	SONG_ITPROJECT		= 0x20000,		// Is a project file
	SONG_ITPEMBEDIH		= 0x40000,		// Embed instrument headers in project file
	// -! NEW_FEATURE#0023
	SONG_BREAKTOROW		= 0x80000,		// Break to row command encountered (internal flag, do not touch)
	SONG_POSJUMP		= 0x100000,	// Position jump encountered (internal flag, do not touch)
	SONG_PT1XMODE		= 0x200000,	// ProTracker 1.x playback mode
};

#define SONG_FILE_FLAGS	(SONG_EMBEDMIDICFG|SONG_FASTVOLSLIDES|SONG_ITOLDEFFECTS|SONG_ITCOMPATGXX|SONG_LINEARSLIDES|SONG_EXFILTERRANGE|SONG_AMIGALIMITS|SONG_ITPROJECT|SONG_ITPEMBEDIH|SONG_PT1XMODE)
#define SONG_PLAY_FLAGS (~SONG_FILE_FLAGS)

// Global Options (Renderer)
#define SNDMIX_REVERSESTEREO	0x0001	// swap L/R audio channels
#define SNDMIX_NOISEREDUCTION	0x0002	// reduce hiss (do not use, it's just a simple low-pass filter)
#define SNDMIX_AGC				0x0004	// automatic gain control
#define SNDMIX_NORESAMPLING		0x0008	// force no resampling
//      SNDMIX_NOLINEARSRCMODE is the default
//#define SNDMIX_HQRESAMPLER		0x0010	 //rewbs.resamplerConf: renamed SNDMIX_HQRESAMPLER to SNDMIX_SPLINESRCMODE
#define SNDMIX_SPLINESRCMODE	0x0010	// cubic resampling (?)
#define SNDMIX_MEGABASS			0x0020	// bass expansion
#define SNDMIX_SURROUND			0x0040	// surround mix
#define SNDMIX_REVERB			0x0080	// apply reverb
#define SNDMIX_EQ				0x0100	// apply EQ
#define SNDMIX_SOFTPANNING		0x0200	// soft panning mode (this is forced with mixmode RC3 and later)
//#define SNDMIX_ULTRAHQSRCMODE	0x0400 	//rewbs.resamplerConf: renamed SNDMIX_ULTRAHQSRCMODE to SNDMIX_POLYPHASESRCMODE
#define SNDMIX_POLYPHASESRCMODE	0x0400	// polyphase resampling
#define SNDMIX_FIRFILTERSRCMODE	0x0800  //rewbs: added SNDMIX_FIRFILTERSRCMODE

//rewbs.resamplerConf: for stuff that applies to cubic spline, polyphase and FIR
#define SNDMIX_HQRESAMPLER (SNDMIX_SPLINESRCMODE|SNDMIX_POLYPHASESRCMODE|SNDMIX_FIRFILTERSRCMODE)

////rewbs.resamplerConf: for stuff that applies to polyphase and FIR
#define SNDMIX_ULTRAHQSRCMODE (SNDMIX_POLYPHASESRCMODE|SNDMIX_FIRFILTERSRCMODE)

// Misc Flags (can safely be turned on or off)
#define SNDMIX_DIRECTTODISK		0x10000		// WAV writer mode
#define SNDMIX_ENABLEMMX		0x20000		// use MMX-accelerated code
//#define SNDMIX_NOBACKWARDJUMPS	0x40000		// stop when jumping back in the order (currently unused as it seems)
#define SNDMIX_MAXDEFAULTPAN	0x80000		// Used by the MOD loader (currently unused)
#define SNDMIX_MUTECHNMODE		0x100000	// Notes are not played on muted channels


#define MAX_GLOBAL_VOLUME 256

// Resampling modes
enum {
	SRCMODE_NEAREST,
	SRCMODE_LINEAR,
	SRCMODE_SPLINE,
	SRCMODE_POLYPHASE,
	SRCMODE_FIRFILTER, //rewbs.resamplerConf
	SRCMODE_DEFAULT,
	NUM_SRC_MODES
};

// Release node defines
#define ENV_RELEASE_NODE_UNSET	0xFF
#define NOT_YET_RELEASED		(-1)
STATIC_ASSERT(ENV_RELEASE_NODE_UNSET > MAX_ENVPOINTS);


enum PluginPriority
{
	ChannelOnly,
	InstrumentOnly,
	PrioritiseInstrument,
	PrioritiseChannel,
};

enum PluginMutePriority
{
	EvenIfMuted,
	RespectMutes,
};

//Plugin velocity handling options
enum PLUGVELOCITYHANDLING
{
	PLUGIN_VELOCITYHANDLING_CHANNEL = 0,
	PLUGIN_VELOCITYHANDLING_VOLUME
};

//Plugin volumecommand handling options
enum PLUGVOLUMEHANDLING
{
	PLUGIN_VOLUMEHANDLING_MIDI = 0,
	PLUGIN_VOLUMEHANDLING_DRYWET,
	PLUGIN_VOLUMEHANDLING_IGNORE,
	PLUGIN_VOLUMEHANDLING_CUSTOM,
};

// filtermodes
/*enum {
	INST_FILTERMODE_DEFAULT=0,
	INST_FILTERMODE_HIGHPASS,
	INST_FILTERMODE_LOWPASS,
	INST_NUMFILTERMODES
};*/

enum MidiChannel
{
	MidiNoChannel		= 0,
	MidiFirstChannel	= 1,
	MidiLastChannel		= 16,
	MidiMappedChannel	= 17,
};


// Vibrato Types
enum VibratoType
{
	VIB_SINE = 0,
	VIB_SQUARE,
	VIB_RAMP_UP,
	VIB_RAMP_DOWN,
	VIB_RANDOM
};