#ifndef MIXER_H
#define MIXER_H

#define MAX_EQ_BANDS		6

// Mixer Hardware-Dependent features
#define SYSMIX_ENABLEMMX	0x01
#define SYSMIX_SLOWCPU		0x02
#define SYSMIX_FASTCPU		0x04
#define SYSMIX_MMXEX		0x08
#define SYSMIX_3DNOW		0x10
#define SYSMIX_SSE			0x20

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
#define SNDMIX_MAXDEFAULTPAN	0x80000	 // Used by the MOD loader
#define SNDMIX_MUTECHNMODE		0x100000 // Notes are not played on muted channels
#define SNDMIX_EMULATE_MIX_BUGS 0x200000 // rewbs.emulateMixBugs

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

struct SNDMIXSONGEQ
{
	ULONG nEQBands;
	ULONG EQFreq_Gains[MAX_EQ_BANDS];
};
typedef SNDMIXSONGEQ* PSNDMIXSONGEQ;

////////////////////////////////////////////////////////////////////////
// Reverberation

struct SNDMIX_REVERB_PROPERTIES
{
	LONG  lRoom;                   // [-10000, 0]      default: -10000 mB
    LONG  lRoomHF;                 // [-10000, 0]      default: 0 mB
    FLOAT flDecayTime;             // [0.1, 20.0]      default: 1.0 s
    FLOAT flDecayHFRatio;          // [0.1, 2.0]       default: 0.5
    LONG  lReflections;            // [-10000, 1000]   default: -10000 mB
    FLOAT flReflectionsDelay;      // [0.0, 0.3]       default: 0.02 s
    LONG  lReverb;                 // [-10000, 2000]   default: -10000 mB
    FLOAT flReverbDelay;           // [0.0, 0.1]       default: 0.04 s
    FLOAT flDiffusion;             // [0.0, 100.0]     default: 100.0 %
    FLOAT flDensity;               // [0.0, 100.0]     default: 100.0 %
};
typedef SNDMIX_REVERB_PROPERTIES* PSNDMIX_REVERB_PROPERTIES;

#ifndef NO_REVERB
	#define NUM_REVERBTYPES			29
	LPCSTR GetReverbPresetName(UINT nPreset);
#endif

typedef VOID (__cdecl * LPSNDMIXHOOKPROC)(int *, unsigned long, unsigned long); // buffer, samples, channels

///////////////////////////////////////////////////////////
// Low-level Mixing functions ->

#define MIXBUFFERSIZE		512
#define SCRATCH_BUFFER_SIZE 64 //Used for plug's final processing (cleanup)
#define MIXING_ATTENUATION	4
#define VOLUMERAMPPRECISION	12	
#define FADESONGDELAY		100
#define EQ_BUFFERSIZE		(MIXBUFFERSIZE)
#define AGC_PRECISION		10
#define AGC_UNITY			(1 << AGC_PRECISION)

// Calling conventions
#ifdef WIN32
#define MPPASMCALL	__cdecl
#define MPPFASTCALL	__fastcall
#else
#define MPPASMCALL
#define MPPFASTCALL
#endif

#define MOD2XMFineTune(k)	((int)( (signed char)((k)<<4) ))
#define XM2MODFineTune(k)	((int)( (k>>4)&0x0f ))

int _muldiv(long a, long b, long c);
int _muldivr(long a, long b, long c);

// <- Low-level Mixing functions
///////////////////////////////////////////////////////////


#endif
