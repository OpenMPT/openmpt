#ifndef MODINS_H
#define MODINS_H

#define MAX_ENVPOINTS		32


#define ENV_VOLUME			0x0001
#define ENV_VOLSUSTAIN		0x0002
#define ENV_VOLLOOP			0x0004
#define ENV_PANNING			0x0008
#define ENV_PANSUSTAIN		0x0010
#define ENV_PANLOOP			0x0020
#define ENV_PITCH			0x0040
#define ENV_PITCHSUSTAIN	0x0080
#define ENV_PITCHLOOP		0x0100
#define ENV_SETPANNING		0x0200
#define ENV_FILTER			0x0400
#define ENV_VOLCARRY		0x0800
#define ENV_PANCARRY		0x1000
#define ENV_PITCHCARRY		0x2000
#define ENV_MUTE			0x4000

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

// Filter Modes
#define FLTMODE_UNCHANGED		0xFF
#define FLTMODE_LOWPASS			0
#define FLTMODE_HIGHPASS		1
#define FLTMODE_BANDPASS		2

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


// Instrument Struct
struct INSTRUMENTHEADER
//=====================
{
	UINT nFadeOut;
	DWORD dwFlags;
	UINT nGlobalVol;
	UINT nPan;
	UINT nVolEnv; //Number of points in the volume envelope
	UINT nPanEnv;
	UINT nPitchEnv;
	BYTE nVolLoopStart;
	BYTE nVolLoopEnd;
	BYTE nVolSustainBegin;
	BYTE nVolSustainEnd;
	BYTE nPanLoopStart;
	BYTE nPanLoopEnd;
	BYTE nPanSustainBegin;
	BYTE nPanSustainEnd;
	BYTE nPitchLoopStart;
	BYTE nPitchLoopEnd;
	BYTE nPitchSustainBegin;
	BYTE nPitchSustainEnd;
	BYTE nNNA;
	BYTE nDCT;
	BYTE nDNA;
	BYTE nPanSwing;
	BYTE nVolSwing;
	BYTE nIFC;
	BYTE nIFR;
	WORD wMidiBank;
	BYTE nMidiProgram;
	BYTE nMidiChannel;
	BYTE nMidiDrumKey;
	signed char nPPS; //Pitch to Pan Separator?
	unsigned char nPPC; //Pitch Centre?
	WORD VolPoints[MAX_ENVPOINTS];
	WORD PanPoints[MAX_ENVPOINTS];
	WORD PitchPoints[MAX_ENVPOINTS];
	BYTE VolEnv[MAX_ENVPOINTS];
	BYTE PanEnv[MAX_ENVPOINTS];
	BYTE PitchEnv[MAX_ENVPOINTS];
	BYTE NoteMap[128];
	WORD Keyboard[128];
	CHAR name[32];
	CHAR filename[12];
	BYTE nMixPlug;							//rewbs.instroVSTi
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
	USHORT nVolRamp;
// -! NEW_FEATURE#0027
	UINT nResampling;
	BYTE nCutSwing;
	BYTE nResSwing;
	BYTE nFilterMode;
	BYTE nPitchEnvReleaseNode;
	BYTE nPanEnvReleaseNode;
	BYTE nVolEnvReleaseNode;
	WORD wPitchToTempoLock;
	BYTE nPluginVelocityHandling;
	BYTE nPluginVolumeHandling;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// WHEN adding new members here, ALSO update Sndfile.cpp (instructions near the top of that file)!
// Important: Objects of this class are memset to zero in some places in the code. Before those
	//		  are replaced with same other style of resetting the object, only types that can safely
	//		  be memset can be used in this struct.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	CTuning* pTuning;
	static CTuning* s_DefaultTuning;

	void SetTuning(CTuning* pT) {pTuning = pT;}
};

#endif
