/*
 * OpenMPT
 *
 * Snd_defs.h
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *          OpenMPT devs
*/

#ifndef SND_DEF_H
#define SND_DEF_H

#ifndef LPCBYTE
typedef const BYTE * LPCBYTE;
#endif

typedef uint32 ROWINDEX;
	const ROWINDEX ROWINDEX_MAX = uint32_max;
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
typedef uint32 MODTYPE;

#define MAX_PATTERN_ROWS	1024	// -> CODE#0008 -> DESC="#define to set pattern size" -! BEHAVIOUR_CHANGE#0008


#define MOD_AMIGAC2			0x1AB
// -> CODE#0006 
// -> DESC="misc quantity changes"
#define MAX_SAMPLE_LENGTH	0x10000000	// 0x04000000 (64MB -> now 256MB).
                                        // Note: Sample size in bytes can be more than 256 MB.
// -! BEHAVIOUR_CHANGE#0006
#define MAX_SAMPLE_RATE		192000
#define MAX_ORDERS			256
#define MAX_PATTERNS		240
#define MAX_SAMPLES			4000

const SEQUENCEINDEX MAX_SEQUENCES = 50;

#define MAX_INSTRUMENTS		256	//200 // -> CODE#0006 -> DESC="misc quantity changes" // -! BEHAVIOUR_CHANGE#0006
//#ifdef FASTSOUNDLIB
//#define MAX_CHANNELS		80
//#else
// -> CODE#0006
// -> DESC="misc quantity changes"
#define MAX_CHANNELS		256	//200 // Note: This is the maximum number of sound channels,
                                //            see MAX_BASECHANNELS for max pattern channels.
// -! BEHAVIOUR_CHANGE#0006
//#endif
// -> CODE#0006
// -> DESC="misc quantity changes"
//#ifdef FASTSOUNDLIB
//#define MAX_BASECHANNELS	64
//#else
#define MAX_BASECHANNELS	127	// Max pattern channels.
//#endif
// -! BEHAVIOUR_CHANGE#0006
#define MAX_ENVPOINTS		240
#define MIN_PERIOD			0x0020
#define MAX_PERIOD			0xFFFF
#define MAX_PATTERNNAME		32
#define MAX_CHANNELNAME		20
#define MAX_INFONAME		80
#define MAX_EQ_BANDS		6

#define MAX_MIXPLUGINS		100	//50 // -> CODE#0006 -> DESC="misc quantity changes" -! BEHAVIOUR_CHANGE#0006
#define MAX_PLUGPRESETS		1000 //rewbs.plugPresets

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

// For compatibility mode
#define TRK_IMPULSETRACKER	MOD_TYPE_IT | MOD_TYPE_MPT
#define TRK_FASTTRACKER2	MOD_TYPE_XM
#define TRK_SCREAMTRACKER	MOD_TYPE_S3M
#define TRK_PROTRACKER		MOD_TYPE_MOD
#define TRK_ALLTRACKERS		TRK_IMPULSETRACKER | TRK_FASTTRACKER2 | TRK_SCREAMTRACKER | TRK_PROTRACKER



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
#define CHN_NOIDO			0x1000		// ???
#define CHN_HQSRC			0x2000		// ???
#define CHN_FILTER			0x4000		// filtered output
#define CHN_VOLUMERAMP		0x8000		// ramp volume
#define CHN_VIBRATO			0x10000		// apply vibrato
#define CHN_TREMOLO			0x20000		// apply tremolo
#define CHN_PANBRELLO		0x40000		// apply panbrello
#define CHN_PORTAMENTO		0x80000		// apply portamento
#define CHN_GLISSANDO		0x100000	// glissando mode
#define CHN_VOLENV			0x200000	// volume envelope is active
#define CHN_PANENV			0x400000	// pan envelope is active
#define CHN_PITCHENV		0x800000	// pitch envelope is active
#define CHN_FASTVOLRAMP		0x1000000	// ramp volume very fast
#define CHN_EXTRALOUD		0x2000000	// force master volume to 0x100
#define CHN_REVERB			0x4000000	// apply reverb
#define CHN_NOREVERB		0x8000000	// forbid reverb
#define CHN_SOLO			0x10000000	// solo channel -> CODE#0012 -> DESC="midi keyboard split" -! NEW_FEATURE#0012
#define CHN_NOFX			0x20000000	// dry channel -> CODE#0015 -> DESC="channels management dlg" -! NEW_FEATURE#0015
#define CHN_SYNCMUTE		0x40000000	// keep sample sync on mute
#define CHN_FILTERENV		0x80000000	// force pitch envelope to act as filter envelope

#define CHN_SAMPLEFLAGS		(CHN_16BIT|CHN_LOOP|CHN_PINGPONGLOOP|CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN|CHN_PANNING|CHN_STEREO|CHN_PINGPONGFLAG)
#define CHN_CHANNELFLAGS	(~CHN_SAMPLEFLAGS)

// instrument envelope-specific flags
#define ENV_ENABLED			0x01	// env is enabled
#define ENV_LOOP			0x02	// env loop
#define ENV_SUSTAIN			0x04	// env sustain
#define ENV_CARRY			0x08	// env carry
#define ENV_FILTER			0x10	// filter env enabled (this has to be combined with ENV_ENABLED in the pitch envelope's flags)


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


#define RSF_16BIT		0x04
#define RSF_STEREO		0x08

#define RS_PCM8S		0	// 8-bit signed
#define RS_PCM8U		1	// 8-bit unsigned
#define RS_PCM8D		2	// 8-bit delta values
#define RS_ADPCM4		3	// 4-bit ADPCM-packed
#define RS_PCM16D		4	// 16-bit delta values
#define RS_PCM16S		5	// 16-bit signed
#define RS_PCM16U		6	// 16-bit unsigned
#define RS_PCM16M		7	// 16-bit motorola order
#define RS_STPCM8S		(RS_PCM8S|RSF_STEREO)	// stereo 8-bit signed
#define RS_STPCM8U		(RS_PCM8U|RSF_STEREO)	// stereo 8-bit unsigned
#define RS_STPCM8D		(RS_PCM8D|RSF_STEREO)	// stereo 8-bit delta values
#define RS_STPCM16S		(RS_PCM16S|RSF_STEREO)	// stereo 16-bit signed
#define RS_STPCM16U		(RS_PCM16U|RSF_STEREO)	// stereo 16-bit unsigned
#define RS_STPCM16D		(RS_PCM16D|RSF_STEREO)	// stereo 16-bit delta values
#define RS_STPCM16M		(RS_PCM16M|RSF_STEREO)	// stereo 16-bit signed big endian
// IT 2.14 compressed samples
#define RS_IT2148		0x10
#define RS_IT21416		0x14
#define RS_IT2158		0x12
#define RS_IT21516		0x16
// AMS Packed Samples
#define RS_AMS8			0x11
#define RS_AMS16		0x15
// DMF Huffman compression
#define RS_DMF8			0x13
#define RS_DMF16		0x17
// MDL Huffman compression
#define RS_MDL8			0x20
#define RS_MDL16		0x24
#define RS_PTM8DTO16	0x25
// Stereo Interleaved Samples
#define RS_STIPCM8S		(RS_PCM8S|0x40|RSF_STEREO)	// stereo 8-bit signed
#define RS_STIPCM8U		(RS_PCM8U|0x40|RSF_STEREO)	// stereo 8-bit unsigned
#define RS_STIPCM16S	(RS_PCM16S|0x40|RSF_STEREO)	// stereo 16-bit signed
#define RS_STIPCM16U	(RS_PCM16U|0x40|RSF_STEREO)	// stereo 16-bit unsigned
#define RS_STIPCM16M	(RS_PCM16M|0x40|RSF_STEREO)	// stereo 16-bit signed big endian
// 24-bit signed
#define RS_PCM24S		(RS_PCM16S|0x80)			// mono 24-bit signed
#define RS_STIPCM24S	(RS_PCM16S|0x80|RSF_STEREO)	// stereo 24-bit signed
// 32-bit
#define RS_PCM32S		(RS_PCM16S|0xC0)			// mono 32-bit signed
#define RS_STIPCM32S	(RS_PCM16S|0xC0|RSF_STEREO)	// stereo 32-bit signed



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
#define SYSMIX_ENABLEMMX	0x01
#define SYSMIX_SLOWCPU		0x02
#define SYSMIX_FASTCPU		0x04
#define SYSMIX_MMXEX		0x08
#define SYSMIX_3DNOW		0x10
#define SYSMIX_SSE			0x20

// Module flags
#define SONG_EMBEDMIDICFG	0x0001		// Embed macros in file
#define SONG_FASTVOLSLIDES	0x0002		// Old Scream Tracker 3.0 volume slides
#define SONG_ITOLDEFFECTS	0x0004		// Old Impulse Tracker effect implementations
#define SONG_ITCOMPATMODE	0x0008		// IT "Compatible Gxx"
#define SONG_LINEARSLIDES	0x0010		// Linear slides vs. Amiga slides
#define SONG_PATTERNLOOP	0x0020		// Loop current pattern (pattern editor)
#define SONG_STEP			0x0040		// Song is in "step" mode (pattern editor)
#define SONG_PAUSED			0x0080		// Song is paused
#define SONG_FADINGSONG		0x0100		// Song is fading out
#define SONG_ENDREACHED		0x0200		// Song is finished
#define SONG_GLOBALFADE		0x0400		// Song is fading out
#define SONG_CPUVERYHIGH	0x0800		// High CPU usage
#define SONG_FIRSTTICK		0x1000		// Is set when the current tick is the first tick of the row
#define SONG_MPTFILTERMODE	0x2000		// Local filter mode (reset filter on each note)
#define SONG_SURROUNDPAN	0x4000		// Pan in the rear channels
#define SONG_EXFILTERRANGE	0x8000		// Cutoff Filter has double frequency range (up to ~10Khz)
#define SONG_AMIGALIMITS	0x10000		// Enforce amiga frequency limits
// -> CODE#0023
// -> DESC="IT project files (.itp)"
#define SONG_ITPROJECT		0x20000		// Is a project file
#define SONG_ITPEMBEDIH		0x40000		// Embed instrument headers in project file
// -! NEW_FEATURE#0023
#define SONG_BREAKTOROW		0x80000		// Break to row command encountered (internal flag, do not touch)
#define SONG_POSJUMP		0x100000	// Position jump encountered (internal flag, do not touch)
#define SONG_PT1XMODE		0x200000	// ProTracker 1.x playback mode

// Global Options (Renderer)
#define SNDMIX_REVERSESTEREO	0x0001
#define SNDMIX_NOISEREDUCTION	0x0002
#define SNDMIX_AGC				0x0004
#define SNDMIX_NORESAMPLING		0x0008
//      SNDMIX_NOLINEARSRCMODE is the default
//#define SNDMIX_HQRESAMPLER		0x0010	 //rewbs.resamplerConf: renamed SNDMIX_HQRESAMPLER to SNDMIX_SPLINESRCMODE
#define SNDMIX_SPLINESRCMODE	0x0010
#define SNDMIX_MEGABASS			0x0020
#define SNDMIX_SURROUND			0x0040
#define SNDMIX_REVERB			0x0080
#define SNDMIX_EQ				0x0100
#define SNDMIX_SOFTPANNING		0x0200
//#define SNDMIX_ULTRAHQSRCMODE	0x0400 	 //rewbs.resamplerConf: renamed SNDMIX_ULTRAHQSRCMODE to SNDMIX_POLYPHASESRCMODE
#define SNDMIX_POLYPHASESRCMODE	0x0400
#define SNDMIX_FIRFILTERSRCMODE	0x0800   //rewbs: added SNDMIX_FIRFILTERSRCMODE

//rewbs.resamplerConf: for stuff that applies to cubic spline, polyphase and FIR
#define SNDMIX_HQRESAMPLER (SNDMIX_SPLINESRCMODE|SNDMIX_POLYPHASESRCMODE|SNDMIX_FIRFILTERSRCMODE)

////rewbs.resamplerConf: for stuff that applies to polyphase and FIR
#define SNDMIX_ULTRAHQSRCMODE (SNDMIX_POLYPHASESRCMODE|SNDMIX_FIRFILTERSRCMODE)

// Misc Flags (can safely be turned on or off)
#define SNDMIX_DIRECTTODISK		0x10000
#define SNDMIX_ENABLEMMX		0x20000
#define SNDMIX_NOBACKWARDJUMPS	0x40000
#define SNDMIX_MAXDEFAULTPAN	0x80000	 // Used by the MOD loader (currently unused)
#define SNDMIX_MUTECHNMODE		0x100000 // Notes are not played on muted channels
#define SNDMIX_EMULATE_MIX_BUGS 0x200000 // rewbs.emulateMixBugs

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

enum {
	ENV_RESET_ALL,
	ENV_RESET_VOL,
	ENV_RESET_PAN,
	ENV_RESET_PITCH,
	ENV_RELEASE_NODE_UNSET=0xFF,
	NOT_YET_RELEASED=-1
};

enum {
	CHANNEL_ONLY		  = 0,
	INSTRUMENT_ONLY       = 1,
	PRIORITISE_INSTRUMENT = 2,
	PRIORITISE_CHANNEL    = 3,
	EVEN_IF_MUTED         = false,
	RESPECT_MUTES         = true,
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
};

// filtermodes
/*enum {
	INST_FILTERMODE_DEFAULT=0,
	INST_FILTERMODE_HIGHPASS,
	INST_FILTERMODE_LOWPASS,
	INST_NUMFILTERMODES
};*/

#endif
