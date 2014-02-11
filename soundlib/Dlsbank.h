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

class CSoundFile;
#include "Snd_defs.h"

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
	DWORD ulLoopStart;
	DWORD ulLoopEnd;
	WORD nWaveLink;
	WORD uPercEnv;
	WORD usVolume;		// 0..256
	WORD fuOptions;	// flags + key group
	SHORT sFineTune;	// 1..100
	BYTE uKeyMin;
	BYTE uKeyMax;
	BYTE uUnityNote;
} DLSREGION;

STATIC_ASSERT(sizeof(DLSREGION) == 21);

typedef struct PACKED DLSENVELOPE
{
	// Volume Envelope
	WORD wVolAttack;		// Attack Time: 0-1000, 1 = 20ms (1/50s) -> [0-20s]
	WORD wVolDecay;			// Decay Time: 0-1000, 1 = 20ms (1/50s) -> [0-20s]
	WORD wVolRelease;		// Release Time: 0-1000, 1 = 20ms (1/50s) -> [0-20s]
	BYTE nVolSustainLevel;	// Sustain Level: 0-128, 128=100%
	// Default Pan
	BYTE nDefPan;
} DLSENVELOPE;

STATIC_ASSERT(sizeof(DLSENVELOPE) == 8);

// Special Bank bits
#define F_INSTRUMENT_DRUMS		0x80000000

typedef struct PACKED DLSINSTRUMENT
{
	DWORD ulBank, ulInstrument;
	UINT nRegions, nMelodicEnv;
	DLSREGION Regions[DLSMAXREGIONS];
	CHAR szName[32];
	// SF2 stuff (DO NOT USE! -> used internally by the SF2 loader)
	WORD wPresetBagNdx, wPresetBagNum;
} DLSINSTRUMENT;

STATIC_ASSERT(sizeof(DLSINSTRUMENT) == 2740);

typedef struct PACKED DLSSAMPLEEX
{
	CHAR szName[20];
	DWORD dwLen;
	DWORD dwStartloop;
	DWORD dwEndloop;
	DWORD dwSampleRate;
	BYTE byOriginalPitch;
	CHAR chPitchCorrection;
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
	CHAR szBankName[256];
	CHAR szCopyRight[256];
	CHAR szComments[512];
	CHAR szEngineer[256];
	CHAR szSoftware[256];		// ISFT: Software
	CHAR szDescription[256];	// ISBJ: Subject
} SOUNDBANKINFO;


//============
class CDLSBank
//============
{
protected:
	SOUNDBANKINFO m_BankInfo;
	mpt::PathString m_szFileName;
	UINT m_nType;
	DWORD m_dwWavePoolOffset;
	// DLS Information
	UINT m_nInstruments, m_nWaveForms, m_nEnvelopes, m_nSamplesEx, m_nMaxWaveLink;
	DWORD *m_pWaveForms;
	DLSINSTRUMENT *m_pInstruments;
	DLSSAMPLEEX *m_pSamplesEx;
	DLSENVELOPE m_Envelopes[DLSMAXENVELOPES];

public:
	CDLSBank();
	virtual ~CDLSBank();
	void Destroy();
	static BOOL IsDLSBank(const mpt::PathString &filename);
	static DWORD MakeMelodicCode(UINT bank, UINT instr) { return ((bank << 16) | (instr));}
	static DWORD MakeDrumCode(UINT rgn, UINT instr) { return (0x80000000 | (rgn << 16) | (instr));}

public:
	BOOL Open(const mpt::PathString &filename);
	mpt::PathString GetFileName() const { return m_szFileName; }
	UINT GetBankType() const { return m_nType; }
	UINT GetBankInfo(SOUNDBANKINFO *pBankInfo=NULL) const { if (pBankInfo) *pBankInfo = m_BankInfo; return m_nType; }

public:
	UINT GetNumInstruments() const { return m_nInstruments; }
	UINT GetNumSamples() const { return m_nWaveForms; }
	DLSINSTRUMENT *GetInstrument(UINT iIns) { return (m_pInstruments) ? &m_pInstruments[iIns] : NULL; }
	DLSINSTRUMENT *FindInstrument(BOOL bDrum, UINT nBank=0xFF, DWORD dwProgram=0xFF, DWORD dwKey=0xFF, UINT *pInsNo=NULL);
	UINT GetRegionFromKey(UINT nIns, UINT nKey);
	BOOL FreeWaveForm(LPBYTE p);
	BOOL ExtractWaveForm(UINT nIns, UINT nRgn, LPBYTE *ppWave, DWORD *pLen);
	BOOL ExtractSample(CSoundFile &sndFile, SAMPLEINDEX nSample, UINT nIns, UINT nRgn, int transpose=0);
	BOOL ExtractInstrument(CSoundFile &sndFile, INSTRUMENTINDEX nInstr, UINT nIns, UINT nDrumRgn);
	const CHAR *GetRegionName(UINT nIns, UINT nRgn) const;

// Internal Loader Functions
protected:
	BOOL UpdateInstrumentDefinition(DLSINSTRUMENT *pDlsIns, void *pchunk, DWORD dwMaxLen);
	BOOL UpdateSF2PresetData(void *psf2info, void *pchunk, DWORD dwMaxLen);
	BOOL ConvertSF2ToDLS(void *psf2info);

public:
	// DLS Unit conversion
	static LONG DLS32BitTimeCentsToMilliseconds(LONG lTimeCents);
	static LONG DLS32BitRelativeGainToLinear(LONG lCentibels);	// 0dB = 0x10000
	static LONG DLS32BitRelativeLinearToGain(LONG lGain);		// 0dB = 0x10000
	static LONG DLSMidiVolumeToLinear(UINT nMidiVolume);		// [0-127] -> [0-0x10000]
};


#endif // MODPLUG_TRACKER

