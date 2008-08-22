#ifndef MODCHANNEL_H
#define MODCHANNEL_H

#define MAX_GLOBAL_VOLUME	256
#define MIN_PERIOD			0x0020
#define MAX_PERIOD			0xFFFF


// Channel flags:
// Bits 0-7:	Sample Flags
#define CHN_16BIT			0x01
#define CHN_LOOP			0x02
#define CHN_PINGPONGLOOP	0x04
#define CHN_SUSTAINLOOP		0x08
#define CHN_PINGPONGSUSTAIN	0x10
#define CHN_PANNING			0x20
#define CHN_STEREO			0x40
#define CHN_PINGPONGFLAG	0x80	//When flag is on, bidiloop is processed backwards?
// Bits 8-31:	Channel Flags
#define CHN_MUTE			0x100
#define CHN_KEYOFF			0x200
#define CHN_NOTEFADE		0x400
#define CHN_SURROUND		0x800
#define CHN_NOIDO			0x1000
#define CHN_HQSRC			0x2000
#define CHN_FILTER			0x4000
#define CHN_VOLUMERAMP		0x8000
#define CHN_VIBRATO			0x10000
#define CHN_TREMOLO			0x20000
#define CHN_PANBRELLO		0x40000
#define CHN_PORTAMENTO		0x80000
#define CHN_GLISSANDO		0x100000
#define CHN_VOLENV			0x200000
#define CHN_PANENV			0x400000
#define CHN_PITCHENV		0x800000
#define CHN_FASTVOLRAMP		0x1000000
#define CHN_EXTRALOUD		0x2000000
#define CHN_REVERB			0x4000000
#define CHN_NOREVERB		0x8000000
#define CHN_SOLO			0x10000000 //Midi keyboard split
#define CHN_NOFX			0x20000000 //Channels management dlg
#define CHN_SYNCMUTE		0x40000000

enum {
	ENV_RESET_ALL,
	ENV_RESET_VOL,
	ENV_RESET_PAN,
	ENV_RESET_PITCH,
	ENV_RELEASE_NODE_UNSET=0xFF,
	NOT_YET_RELEASED=-1
};

#define CHNRESET_CHNSETTINGS	1 //  1 b 
#define CHNRESET_SETPOS_BASIC	2 // 10 b
#define	CHNRESET_SETPOS_FULL	7 //111 b
#define CHNRESET_TOTAL			255 //11111111b


// Channel Struct
typedef struct _MODCHANNEL
//========================
{
	// First 32-bytes: Most used mixing information: don't change it
	LPSTR pCurrentSample;		
	DWORD nPos;
	DWORD nPosLo;	// actually 16-bit
	LONG nInc;		// 16.16
	LONG nRightVol;
	LONG nLeftVol;
	LONG nRightRamp;
	LONG nLeftRamp;
	// 2nd cache line
	DWORD nLength;
	DWORD dwFlags;
	DWORD nLoopStart;
	DWORD nLoopEnd;
	LONG nRampRightVol;
	LONG nRampLeftVol;
	LONG nFilter_Y1, nFilter_Y2, nFilter_Y3, nFilter_Y4;
	LONG nFilter_A0, nFilter_B0, nFilter_B1, nFilter_HP;
	LONG nROfs, nLOfs;
	LONG nRampLength;
	// Information not used in the mixer
	LPSTR pSample;
	LONG nNewRightVol, nNewLeftVol;
	LONG nRealVolume, nRealPan;
	LONG nVolume, nPan, nFadeOutVol;
	LONG nPeriod, nC4Speed, nPortamentoDest;
	INSTRUMENTHEADER *pHeader;
	MODINSTRUMENT *pInstrument;
	DWORD nVolEnvPosition, nPanEnvPosition, nPitchEnvPosition;
	LONG nVolEnvValueAtReleaseJump, nPanEnvValueAtReleaseJump, nPitchEnvValueAtReleaseJump;
	DWORD nMasterChn, nVUMeter;
	LONG nGlobalVol, nInsVol;
	LONG nFineTune, nTranspose;
	LONG nPortamentoSlide, nAutoVibDepth;
	UINT nAutoVibPos, nVibratoPos, nTremoloPos, nPanbrelloPos;
	LONG nVolSwing, nPanSwing;
	LONG nCutSwing, nResSwing;
	LONG nRestorePanOnNewNote; //If > 0, nPan should be set to nRestorePanOnNewNote - 1 on new note. Used to recover from panswing.
	LONG nRestoreResonanceOnNewNote; //Like above
	LONG nRestoreCutoffOnNewNote; //Like above
	// 8-bit members
	BYTE nNote, nNNA;
	BYTE nNewNote, nNewIns, nCommand, nArpeggio;
	BYTE nOldVolumeSlide, nOldFineVolUpDown;
	BYTE nOldPortaUpDown, nOldFinePortaUpDown;
	BYTE nOldPanSlide, nOldChnVolSlide;
	BYTE nVibratoType, nVibratoSpeed, nVibratoDepth;
	BYTE nTremoloType, nTremoloSpeed, nTremoloDepth;
	BYTE nPanbrelloType, nPanbrelloSpeed, nPanbrelloDepth;
	BYTE nOldCmdEx, nOldVolParam, nOldTempo;
	BYTE nOldOffset, nOldHiOffset;
	BYTE nCutOff, nResonance;
	BYTE nRetrigCount, nRetrigParam;
	BYTE nTremorCount, nTremorParam;
	BYTE nPatternLoop, nPatternLoopCount;
	BYTE nRowNote, nRowInstr;
	BYTE nRowVolCmd, nRowVolume;
	BYTE nRowCommand, nRowParam;
	BYTE nLeftVU, nRightVU;
	BYTE nActiveMacro, nFilterMode;

	float m_nPlugParamValueStep;	//rewbs.smoothVST 
	float m_nPlugInitialParamValue; //rewbs.smoothVST
	long m_RowPlugParam;			//NOTE_PCs memory.
	PLUGINDEX m_RowPlug;			//NOTE_PCs memory.


	void ClearRowCmd() {nRowNote = 0; nRowInstr = 0; nRowVolCmd = 0; nRowVolume = 0; nRowCommand = 0; nRowParam = 0;}

	typedef UINT VOLUME;
	VOLUME GetVSTVolume() {return (pHeader) ? pHeader->nGlobalVol*4 : nVolume;}

	//-->Variables used to make user-definable tuningmodes work with pattern effects.
		bool m_ReCalculateFreqOnFirstTick;
		//If true, freq should be recalculated in ReadNote() on first tick.
		//Currently used only for vibrato things - using in other context might be 
		//problematic.

		bool m_CalculateFreq;
		//To tell whether to calculate frequency.

		CTuning::STEPINDEXTYPE m_PortamentoFineSteps;
		long m_PortamentoTickSlide;

		UINT m_Freq;
		float m_VibratoDepth;
	//<----

	// Important: Objects of this class are memset to zero in some places in the code. Before those
	//			  are replaced with same other style of resetting the object, only types that can safely
	//			  be memset can be used in this struct.
	char pad[22];
} MODCHANNEL;

#define MAX_CHANNELNAME		20

struct MODCHANNELSETTINGS
//=======================
{
	UINT nPan;
	UINT nVolume;
	DWORD dwFlags;
	UINT nMixPlugin;
	CHAR szName[MAX_CHANNELNAME];
};

#endif
