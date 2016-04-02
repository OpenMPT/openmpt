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

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

#define DLSMAXREGIONS		128
#define DLSMAXENVELOPES		2048

// Region Flags
#define DLSREGION_KEYGROUPMASK		0x0F
#define DLSREGION_OVERRIDEWSMP		0x10
#define DLSREGION_PINGPONGLOOP		0x20
#define DLSREGION_SAMPLELOOP		0x40
#define DLSREGION_SELFNONEXCLUSIVE	0x80
#define DLSREGION_SUSTAINLOOP		0x100

typedef struct PACKED DLSREGION
{
	uint32 ulLoopStart;
	uint32 ulLoopEnd;
	uint16 nWaveLink;
	uint16 uPercEnv;
	uint16 usVolume;		// 0..256
	uint16 fuOptions;	// flags + key group
	int16 sFineTune;	// 1..100
	uint8 uKeyMin;
	uint8 uKeyMax;
	uint8 uUnityNote;
} DLSREGION;

STATIC_ASSERT(sizeof(DLSREGION) == 21);

typedef struct PACKED DLSENVELOPE
{
	// Volume Envelope
	uint16 wVolAttack;		// Attack Time: 0-1000, 1 = 20ms (1/50s) -> [0-20s]
	uint16 wVolDecay;			// Decay Time: 0-1000, 1 = 20ms (1/50s) -> [0-20s]
	uint16 wVolRelease;		// Release Time: 0-1000, 1 = 20ms (1/50s) -> [0-20s]
	uint8 nVolSustainLevel;	// Sustain Level: 0-128, 128=100%
	// Default Pan
	uint8 nDefPan;
} DLSENVELOPE;

STATIC_ASSERT(sizeof(DLSENVELOPE) == 8);

// Special Bank bits
#define F_INSTRUMENT_DRUMS		0x80000000

typedef struct PACKED DLSINSTRUMENT
{
	uint32 ulBank, ulInstrument;
	uint32 nRegions, nMelodicEnv;
	DLSREGION Regions[DLSMAXREGIONS];
	char szName[32];
	// SF2 stuff (DO NOT USE! -> used internally by the SF2 loader)
	uint16 wPresetBagNdx, wPresetBagNum;
} DLSINSTRUMENT;

STATIC_ASSERT(sizeof(DLSINSTRUMENT) == 2740);

typedef struct PACKED DLSSAMPLEEX
{
	char szName[20];
	uint32 dwLen;
	uint32 dwStartloop;
	uint32 dwEndloop;
	uint32 dwSampleRate;
	uint8 byOriginalPitch;
	char chPitchCorrection;
} DLSSAMPLEEX;

STATIC_ASSERT(sizeof(DLSSAMPLEEX) == 38);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


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
	bool FreeWaveForm(uint8 *p);
	bool ExtractWaveForm(uint32 nIns, uint32 nRgn, uint8 **ppWave, uint32 *pLen);
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
	static LONG DLS32BitTimeCentsToMilliseconds(LONG lTimeCents);
	static LONG DLS32BitRelativeGainToLinear(LONG lCentibels);	// 0dB = 0x10000
	static LONG DLS32BitRelativeLinearToGain(LONG lGain);		// 0dB = 0x10000
	static LONG DLSMidiVolumeToLinear(uint32 nMidiVolume);		// [0-127] -> [0-0x10000]
};


#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_END
