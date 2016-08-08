/*
 * DLSBank.h
 * ---------
 * Purpose: Sound bank loading.
 * Notes  : Supported sound bank types: DLS (including embedded DLS in MSS & RMI), SF2
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN
class CSoundFile;
OPENMPT_NAMESPACE_END
#include "Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

#define DLSMAXREGIONS		128
#define DLSMAXENVELOPES		2048

// Region Flags
#define DLSREGION_KEYGROUPMASK		0x0F
#define DLSREGION_OVERRIDEWSMP		0x10
#define DLSREGION_PINGPONGLOOP		0x20
#define DLSREGION_SAMPLELOOP		0x40
#define DLSREGION_SELFNONEXCLUSIVE	0x80
#define DLSREGION_SUSTAINLOOP		0x100

typedef struct DLSREGION
{
	uint32le ulLoopStart;
	uint32le ulLoopEnd;
	uint16le nWaveLink;
	uint16le uPercEnv;
	uint16le usVolume;		// 0..256
	uint16le fuOptions;	// flags + key group
	int16le  sFineTune;	// 1..100
	uint8le  uKeyMin;
	uint8le  uKeyMax;
	uint8le  uUnityNote;
} DLSREGION;

MPT_BINARY_STRUCT(DLSREGION, 21)

typedef struct DLSENVELOPE
{
	// Volume Envelope
	uint16le wVolAttack;		// Attack Time: 0-1000, 1 = 20ms (1/50s) -> [0-20s]
	uint16le wVolDecay;		// Decay Time: 0-1000, 1 = 20ms (1/50s) -> [0-20s]
	uint16le wVolRelease;		// Release Time: 0-1000, 1 = 20ms (1/50s) -> [0-20s]
	uint8le nVolSustainLevel;	// Sustain Level: 0-128, 128=100%	
	uint8le nDefPan;			// Default Pan
} DLSENVELOPE;

MPT_BINARY_STRUCT(DLSENVELOPE, 8)

// Special Bank bits
#define F_INSTRUMENT_DRUMS		0x80000000

typedef struct DLSINSTRUMENT
{
	uint32le ulBank, ulInstrument;
	uint32le nRegions, nMelodicEnv;
	DLSREGION Regions[DLSMAXREGIONS];
	char szName[32];
	// SF2 stuff (DO NOT USE! -> used internally by the SF2 loader)
	uint16le wPresetBagNdx, wPresetBagNum;
} DLSINSTRUMENT;

MPT_BINARY_STRUCT(DLSINSTRUMENT, 2740)

typedef struct DLSSAMPLEEX
{
	char      szName[20];
	uint32le dwLen;
	uint32le dwStartloop;
	uint32le dwEndloop;
	uint32le dwSampleRate;
	uint8le  byOriginalPitch;
	int8le   chPitchCorrection;
} DLSSAMPLEEX;

MPT_BINARY_STRUCT(DLSSAMPLEEX, 38)


#ifdef MODPLUG_TRACKER


#define SOUNDBANK_TYPE_INVALID	0
#define SOUNDBANK_TYPE_DLS		0x01
#define SOUNDBANK_TYPE_SF2		0x02

typedef struct SOUNDBANKINFO
{
	char szBankName[256];
	char szCopyRight[256];
	char szComments[512];
	char szEngineer[256];
	char szSoftware[256];		// ISFT: Software
	char szDescription[256];	// ISBJ: Subject
} SOUNDBANKINFO;


//============
class CDLSBank
//============
{
protected:
	SOUNDBANKINFO m_BankInfo;
	mpt::PathString m_szFileName;
	uint32 m_nType;
	uint32 m_dwWavePoolOffset;
	// DLS Information
	uint32 m_nInstruments, m_nWaveForms, m_nEnvelopes, m_nSamplesEx, m_nMaxWaveLink;
	uint32 *m_pWaveForms;
	DLSINSTRUMENT *m_pInstruments;
	DLSSAMPLEEX *m_pSamplesEx;
	DLSENVELOPE m_Envelopes[DLSMAXENVELOPES];

public:
	CDLSBank();
	virtual ~CDLSBank();
	void Destroy();
	static bool IsDLSBank(const mpt::PathString &filename);
	static uint32 MakeMelodicCode(uint32 bank, uint32 instr) { return ((bank << 16) | (instr));}
	static uint32 MakeDrumCode(uint32 rgn, uint32 instr) { return (0x80000000 | (rgn << 16) | (instr));}

public:
	bool Open(const mpt::PathString &filename);
	bool Open(FileReader file);
	mpt::PathString GetFileName() const { return m_szFileName; }
	uint32 GetBankType() const { return m_nType; }
	uint32 GetBankInfo(SOUNDBANKINFO *pBankInfo=NULL) const { if (pBankInfo) *pBankInfo = m_BankInfo; return m_nType; }

public:
	uint32 GetNumInstruments() const { return m_nInstruments; }
	uint32 GetNumSamples() const { return m_nWaveForms; }
	DLSINSTRUMENT *GetInstrument(uint32 iIns) { return (m_pInstruments) ? &m_pInstruments[iIns] : NULL; }
	DLSINSTRUMENT *FindInstrument(bool bDrum, uint32 nBank=0xFF, uint32 dwProgram=0xFF, uint32 dwKey=0xFF, uint32 *pInsNo=NULL);
	uint32 GetRegionFromKey(uint32 nIns, uint32 nKey);
	bool ExtractWaveForm(uint32 nIns, uint32 nRgn, std::vector<uint8> &waveData, uint32 &length);
	bool ExtractSample(CSoundFile &sndFile, SAMPLEINDEX nSample, uint32 nIns, uint32 nRgn, int transpose=0);
	bool ExtractInstrument(CSoundFile &sndFile, INSTRUMENTINDEX nInstr, uint32 nIns, uint32 nDrumRgn);
	const char *GetRegionName(uint32 nIns, uint32 nRgn) const;
	uint8 GetPanning(uint32 ins, uint32 region) const;

// Internal Loader Functions
protected:
	bool UpdateInstrumentDefinition(DLSINSTRUMENT *pDlsIns, void *pchunk, uint32 dwMaxLen);
	bool UpdateSF2PresetData(void *psf2info, void *pchunk, uint32 dwMaxLen);
	bool ConvertSF2ToDLS(void *psf2info);

public:
	// DLS Unit conversion
	static int32 DLS32BitTimeCentsToMilliseconds(int32 lTimeCents);
	static int32 DLS32BitRelativeGainToLinear(int32 lCentibels);	// 0dB = 0x10000
	static int32 DLS32BitRelativeLinearToGain(int32 lGain);		// 0dB = 0x10000
	static int32 DLSMidiVolumeToLinear(uint32 nMidiVolume);		// [0-127] -> [0-0x10000]
};


#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_END
