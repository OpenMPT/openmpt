/*
 * This program is  free software; you can redistribute it  and modify it
 * under the terms of the GNU  General Public License as published by the
 * Free Software Foundation; either version 2  of the license or (at your
 * option) any later version.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

///////////////////////////////////////////////////////////////////////////
// Sound Bank support
// DLS (including embedded DLS in MSS & RMI)
// SF2

#include "stdafx.h"
#include "sndfile.h"
#include "../mptrack/mptrack.h"
#include "dlsbank.h"

//#define DLSBANK_LOG
//#define DLSINSTR_LOG

//#define ASM_DLSUNITCONVERSION
#ifndef ASM_DLSUNITCONVERSION
#include <math.h>
#endif

#define F_RGN_OPTION_SELFNONEXCLUSIVE	0x0001

///////////////////////////////////////////////////////////////////////////
// Articulation connection graph definitions 

// Generic Sources
#define CONN_SRC_NONE              0x0000
#define CONN_SRC_LFO               0x0001
#define CONN_SRC_KEYONVELOCITY     0x0002
#define CONN_SRC_KEYNUMBER         0x0003
#define CONN_SRC_EG1               0x0004
#define CONN_SRC_EG2               0x0005
#define CONN_SRC_PITCHWHEEL        0x0006

// Midi Controllers 0-127
#define CONN_SRC_CC1               0x0081
#define CONN_SRC_CC7               0x0087
#define CONN_SRC_CC10              0x008a
#define CONN_SRC_CC11              0x008b

// Generic Destinations
#define CONN_DST_NONE              0x0000
#define CONN_DST_ATTENUATION       0x0001
#define CONN_DST_RESERVED          0x0002
#define CONN_DST_PITCH             0x0003
#define CONN_DST_PAN               0x0004

// LFO Destinations
#define CONN_DST_LFO_FREQUENCY     0x0104
#define CONN_DST_LFO_STARTDELAY    0x0105

// EG1 Destinations
#define CONN_DST_EG1_ATTACKTIME    0x0206
#define CONN_DST_EG1_DECAYTIME     0x0207
#define CONN_DST_EG1_RESERVED      0x0208
#define CONN_DST_EG1_RELEASETIME   0x0209
#define CONN_DST_EG1_SUSTAINLEVEL  0x020a

// EG2 Destinations
#define CONN_DST_EG2_ATTACKTIME    0x030a
#define CONN_DST_EG2_DECAYTIME     0x030b
#define CONN_DST_EG2_RESERVED      0x030c
#define CONN_DST_EG2_RELEASETIME   0x030d
#define CONN_DST_EG2_SUSTAINLEVEL  0x030e

#define CONN_TRN_NONE              0x0000
#define CONN_TRN_CONCAVE           0x0001


//////////////////////////////////////////////////////////
// Supported DLS1 Articulations

#define MAKE_ART(src, ctl, dst)	( ((dst)<<16) | ((ctl)<<8) | (src) )

// Vibrato / Tremolo 
#define ART_LFO_FREQUENCY	MAKE_ART	(CONN_SRC_NONE,	CONN_SRC_NONE,	CONN_DST_LFO_FREQUENCY)
#define ART_LFO_STARTDELAY	MAKE_ART	(CONN_SRC_NONE,	CONN_SRC_NONE,	CONN_DST_LFO_STARTDELAY)
#define ART_LFO_ATTENUATION	MAKE_ART	(CONN_SRC_LFO,	CONN_SRC_NONE,	CONN_DST_ATTENUATION)
#define ART_LFO_PITCH		MAKE_ART	(CONN_SRC_LFO,	CONN_SRC_NONE,	CONN_DST_PITCH)
#define ART_LFO_MODWTOATTN	MAKE_ART	(CONN_SRC_LFO,	CONN_SRC_CC1,	CONN_DST_ATTENUATION)
#define ART_LFO_MODWTOPITCH	MAKE_ART	(CONN_SRC_LFO,	CONN_SRC_CC1,	CONN_DST_PITCH)

// Volume Envelope
#define ART_VOL_EG_ATTACKTIME	MAKE_ART(CONN_SRC_NONE,	CONN_SRC_NONE,	CONN_DST_EG1_ATTACKTIME)
#define ART_VOL_EG_DECAYTIME	MAKE_ART(CONN_SRC_NONE,	CONN_SRC_NONE,	CONN_DST_EG1_DECAYTIME)
#define ART_VOL_EG_SUSTAINLEVEL	MAKE_ART(CONN_SRC_NONE,	CONN_SRC_NONE,	CONN_DST_EG1_SUSTAINLEVEL)
#define ART_VOL_EG_RELEASETIME	MAKE_ART(CONN_SRC_NONE,	CONN_SRC_NONE,	CONN_DST_EG1_RELEASETIME)
#define ART_VOL_EG_VELTOATTACK	MAKE_ART(CONN_SRC_KEYONVELOCITY,	CONN_SRC_NONE,	CONN_DST_EG1_ATTACKTIME)
#define ART_VOL_EG_KEYTODECAY	MAKE_ART(CONN_SRC_KEYNUMBER,		CONN_SRC_NONE,	CONN_DST_EG1_DECAYTIME)

// Pitch Envelope
#define ART_PITCH_EG_ATTACKTIME		MAKE_ART(CONN_SRC_NONE,	CONN_SRC_NONE,	CONN_DST_EG2_ATTACKTIME)
#define ART_PITCH_EG_DECAYTIME		MAKE_ART(CONN_SRC_NONE,	CONN_SRC_NONE,	CONN_DST_EG2_DECAYTIME)
#define ART_PITCH_EG_SUSTAINLEVEL	MAKE_ART(CONN_SRC_NONE,	CONN_SRC_NONE,	CONN_DST_EG2_SUSTAINLEVEL)
#define ART_PITCH_EG_RELEASETIME	MAKE_ART(CONN_SRC_NONE,	CONN_SRC_NONE,	CONN_DST_EG2_RELEASETIME)
#define ART_PITCH_EG_VELTOATTACK	MAKE_ART(CONN_SRC_KEYONVELOCITY,	CONN_SRC_NONE,	CONN_DST_EG2_ATTACKTIME)
#define ART_PITCH_EG_KEYTODECAY		MAKE_ART(CONN_SRC_KEYNUMBER,		CONN_SRC_NONE,	CONN_DST_EG2_DECAYTIME)

// Default Pan
#define ART_DEFAULTPAN		MAKE_ART	(CONN_SRC_NONE,	CONN_SRC_NONE,	CONN_DST_PAN)


#pragma pack(1)

//////////////////////////////////////////////////////////
// DLS IFF Chunk IDs

#define IFFID_XDLS		0x534c4458
#define IFFID_DLS		0x20534C44
#define IFFID_MLS		0x20534C4D
#define IFFID_RMID		0x44494D52
#define IFFID_colh		0x686C6F63
#define IFFID_vers		0x73726576
#define IFFID_msyn		0x6E79736D
#define IFFID_lins		0x736E696C
#define IFFID_ins		0x20736E69
#define IFFID_insh		0x68736E69
#define IFFID_ptbl		0x6C627470
#define IFFID_wvpl		0x6C707677
#define IFFID_rgn		0x206E6772
#define IFFID_rgnh		0x686E6772
#define IFFID_wlnk		0x6B6E6C77
#define IFFID_art1		0x31747261

//////////////////////////////////////////////////////////
// DLS Structures definitions

typedef struct IFFCHUNK
{
	DWORD id;
	DWORD len;
} IFFCHUNK, *LPIFFCHUNK;

typedef struct RIFFCHUNKID
{
	DWORD id_RIFF;
	DWORD riff_len;
	DWORD id_DLS;
} RIFFCHUNKID;

typedef struct LISTCHUNK
{
	DWORD id;
	DWORD len;
	DWORD listid;
} LISTCHUNK;

typedef struct DLSRGNRANGE
{
	WORD usLow;
	WORD usHigh;
} DLSRGNRANGE;

typedef struct COLHCHUNK
{
	DWORD id;
	DWORD len;
	DWORD ulInstruments;
} COLHCHUNK;

typedef struct VERSCHUNK
{
	DWORD id;
	DWORD len;
	WORD version[4];
} VERSCHUNK;

typedef struct PTBLCHUNK
{
	DWORD id;
	DWORD len;
	DWORD cbSize;
	DWORD cCues;
	DWORD ulOffsets[1];
} PTBLCHUNK;

typedef struct INSHCHUNK
{
	DWORD id;
	DWORD len;
	DWORD cRegions;
	DWORD ulBank;
	DWORD ulInstrument;
} INSHCHUNK;

typedef struct RGNHCHUNK
{
	DWORD id;
	DWORD len;
	DLSRGNRANGE RangeKey;
	DLSRGNRANGE RangeVelocity;
	WORD fusOptions;
	WORD usKeyGroup;
} RGNHCHUNK;

typedef struct WLNKCHUNK
{
	DWORD id;
	DWORD len;
	WORD fusOptions;
	WORD usPhaseGroup;
	DWORD ulChannel;
	DWORD ulTableIndex;
} WLNKCHUNK;

typedef struct ART1CHUNK
{
	DWORD id;
	DWORD len;
	DWORD cbSize;
	DWORD cConnectionBlocks;
} ART1CHUNK;

typedef struct CONNECTIONBLOCK
{
	WORD usSource;
	WORD usControl;
	WORD usDestination;
	WORD usTransform;
	LONG lScale;
} CONNECTIONBLOCK;

typedef struct WSMPCHUNK
{
	DWORD id;
	DWORD len;
	DWORD cbSize;
	WORD usUnityNote;
	signed short sFineTune;
	LONG lAttenuation;
	DWORD fulOptions;
	DWORD cSampleLoops;
} WSMPCHUNK;

typedef struct WSMPSAMPLELOOP
{
	DWORD cbSize;
	DWORD ulLoopType;
	DWORD ulLoopStart;
	DWORD ulLoopLength;
} WSMPSAMPLELOOP;


/////////////////////////////////////////////////////////////////////
// SF2 IFF Chunk IDs

#define IFFID_sfbk		0x6b626673
#define IFFID_sdta		0x61746473
#define IFFID_pdta		0x61746470
#define IFFID_phdr		0x72646870
#define IFFID_pbag		0x67616270
#define IFFID_pgen		0x6E656770
#define IFFID_inst		0x74736E69
#define IFFID_ibag		0x67616269
#define IFFID_igen		0x6E656769
#define IFFID_shdr		0x72646873

///////////////////////////////////////////
// SF2 Generators IDs

#define SF2_GEN_MODENVTOFILTERFC		11
#define SF2_GEN_PAN						17
#define SF2_GEN_DECAYMODENV				28
#define SF2_GEN_DECAYVOLENV				36
#define SF2_GEN_RELEASEVOLENV			38
#define SF2_GEN_INSTRUMENT				41
#define SF2_GEN_KEYRANGE				43
#define SF2_GEN_ATTENUATION				48
#define SF2_GEN_COARSETUNE				51
#define SF2_GEN_FINETUNE				52
#define SF2_GEN_SAMPLEID				53
#define SF2_GEN_SAMPLEMODES				54
#define SF2_GEN_KEYGROUP				57
#define SF2_GEN_UNITYNOTE				58

/////////////////////////////////////////////////////////////////////
// SF2 Structures Definitions

typedef struct SFPRESETHEADER
{
	CHAR achPresetName[20];
	WORD wPreset;
	WORD wBank;
	WORD wPresetBagNdx;
	DWORD dwLibrary;
	DWORD dwGenre;
	DWORD dwMorphology;
} SFPRESETHEADER;

typedef struct SFPRESETBAG
{
	WORD wGenNdx;
	WORD wModNdx;
} SFPRESETBAG;

typedef struct SFGENLIST
{
	WORD sfGenOper;
	WORD genAmount;
} SFGENLIST;

typedef struct SFINST
{
	CHAR achInstName[20];
	WORD wInstBagNdx;
} SFINST;

typedef struct SFINSTBAG
{
	WORD wGenNdx;
	WORD wModNdx;
} SFINSTBAG;

typedef struct SFINSTGENLIST
{
	WORD sfGenOper;
	WORD genAmount;
} SFINSTGENLIST;

typedef struct SFSAMPLE
{
	CHAR achSampleName[20];
	DWORD dwStart;
	DWORD dwEnd;
	DWORD dwStartloop;
	DWORD dwEndloop;
	DWORD dwSampleRate;
	BYTE byOriginalPitch;
	CHAR chPitchCorrection;
	WORD wSampleLink;
	WORD sfSampleType;
} SFSAMPLE;


typedef struct SF2LOADERINFO
{
	UINT nPresetBags;
	SFPRESETBAG *pPresetBags;
	UINT nPresetGens;
	SFGENLIST *pPresetGens;
	UINT nInsts;
	SFINST *pInsts;
	UINT nInstBags;
	SFINSTBAG *pInstBags;
	UINT nInstGens;
	SFINSTGENLIST *pInstGens;
} SF2LOADERINFO;


// End of structures definitions
/////////////////////////////////////////////////////////////////////

#pragma pack()

/////////////////////////////////////////////////////////////////////
// Unit conversion

LONG __cdecl CDLSBank::DLS32BitTimeCentsToMilliseconds(LONG lTimeCents)
//---------------------------------------------------------------------
{
	// tc = log2(time[secs]) * 1200*65536
	// time[secs] = 2^(tc/(1200*65536))
	if ((DWORD)lTimeCents == 0x80000000) return 0;
	double fmsecs = 1000.0 * pow(2.0, ((double)lTimeCents)/(1200.0*65536.0));
	if (fmsecs < -32767) return -32767;
	if (fmsecs > 32767) return 32767;
	return (LONG)fmsecs;
}


// 0dB = 0x10000
LONG __cdecl CDLSBank::DLS32BitRelativeGainToLinear(LONG lCentibels)
//------------------------------------------------------------------
{
	// v = 10^(cb/(200*65536)) * V
	return (LONG)(65536.0 * pow(10.0, ((double)lCentibels)/(200*65536.0)) );
}


LONG __cdecl CDLSBank::DLS32BitRelativeLinearToGain(LONG lGain)
//-------------------------------------------------------------
{
	// cb = log10(v/V) * 200 * 65536
	if (lGain <= 0) return -960 * 65536;
	return (LONG)( 200*65536.0 * log10( ((double)lGain)/65536.0 ) );
}


LONG __cdecl CDLSBank::DLSMidiVolumeToLinear(UINT nMidiVolume)
//------------------------------------------------------------
{
	return (nMidiVolume * nMidiVolume << 16) / (127*127);
}


/////////////////////////////////////////////////////////////////////
// Implementation

CDLSBank::CDLSBank()
//------------------
{
	m_nInstruments = 0;
	m_nWaveForms = 0;
	m_nEnvelopes = 0;
	m_nSamplesEx = 0;
	m_nMaxWaveLink = 0;
	m_pWaveForms = NULL;
	m_pInstruments = NULL;
	m_pSamplesEx = NULL;
	m_nType = SOUNDBANK_TYPE_INVALID;
	memset(&m_BankInfo, 0, sizeof(m_BankInfo));
}


CDLSBank::~CDLSBank()
//-------------------
{
	Destroy();
}


void CDLSBank::Destroy()
//----------------------
{
	if (m_pWaveForms)
	{
		delete m_pWaveForms;
		m_pWaveForms = NULL;
		m_nWaveForms = 0;
	}
	if (m_pSamplesEx)
	{
		delete m_pSamplesEx;
		m_pSamplesEx = NULL;
		m_nSamplesEx = 0;
	}
	if (m_pInstruments)
	{
		delete m_pInstruments;
		m_pInstruments = NULL;
		m_nInstruments = 0;
	}
}


BOOL CDLSBank::IsDLSBank(LPCSTR lpszFileName)
//-------------------------------------------
{
	RIFFCHUNKID riff;
	FILE *f;
	if ((!lpszFileName) || (!lpszFileName[0])) return FALSE;
	if ((f = fopen(lpszFileName, "rb")) == NULL) return FALSE;
	memset(&riff, 0, sizeof(riff));
	fread(&riff, sizeof(RIFFCHUNKID), 1, f);
	// Check for embedded DLS sections
	if (riff.id_RIFF == IFFID_FORM)
	{
		do {
			int len = BigEndian(riff.riff_len);
			if (len <= 4) break;
			if (riff.id_DLS == IFFID_XDLS)
			{
				fread(&riff, sizeof(RIFFCHUNKID), 1, f);
				break;
			}
			if (fseek(f, len-4, SEEK_CUR) != 0) break;
		} while (fread(&riff, sizeof(RIFFCHUNKID), 1, f) != 0);
	} else
	if ((riff.id_RIFF == IFFID_RIFF) && (riff.id_DLS == IFFID_RMID))
	{
		for (;;)
		{
			fread(&riff, sizeof(RIFFCHUNKID), 1, f);
			if (riff.id_DLS == IFFID_DLS) break; // found it
			int len = riff.riff_len;
			if ((len <= 4) || (fseek(f, len-4, SEEK_CUR) != 0)) break;
		}
	}
	fclose(f);
	return ((riff.id_RIFF == IFFID_RIFF)
		 && ((riff.id_DLS == IFFID_DLS) || (riff.id_DLS == IFFID_MLS) || (riff.id_DLS == IFFID_sfbk))
		 && (riff.riff_len >= 256));
}


///////////////////////////////////////////////////////////////
// Find an instrument based on the given parameters

DLSINSTRUMENT *CDLSBank::FindInstrument(BOOL bDrum, UINT nBank, DWORD dwProgram, DWORD dwKey, UINT *pInsNo)
//---------------------------------------------------------------------------------------------------------
{
	if ((!m_pInstruments) || (!m_nInstruments)) return NULL;
	for (UINT iIns=0; iIns<m_nInstruments; iIns++)
	{
		DLSINSTRUMENT *pins = &m_pInstruments[iIns];
		UINT insbank = ((pins->ulBank & 0x7F00) >> 1) | (pins->ulBank & 0x7F);
		if ((nBank >= 0x4000) || (insbank == nBank))
		{
			if (bDrum)
			{
				if (pins->ulBank & F_INSTRUMENT_DRUMS)
				{
					if ((dwProgram >= 0x80) || (dwProgram == (pins->ulInstrument & 0x7F)))
					{
						for (UINT iRgn=0; iRgn<pins->nRegions; iRgn++)
						{
							if ((!dwKey) || (dwKey >= 0x80)
							 || ((dwKey >= pins->Regions[iRgn].uKeyMin)
							  && (dwKey <= pins->Regions[iRgn].uKeyMax)))
							{
								if (pInsNo) *pInsNo = iIns;
								return pins;
							}
						}
					}
				}
			} else
			{
				if (!(pins->ulBank & F_INSTRUMENT_DRUMS))
				{
					if ((dwProgram >= 0x80) || (dwProgram == (pins->ulInstrument & 0x7F)))
					{
						if (pInsNo) *pInsNo = iIns;
						return pins;
					}
				}
			}
		}
	}
	return NULL;
}


///////////////////////////////////////////////////////////////
// Update DLS instrument definition from an IFF chunk

BOOL CDLSBank::UpdateInstrumentDefinition(DLSINSTRUMENT *pins, LPVOID pvchunk, DWORD dwMaxLen)
//--------------------------------------------------------------------------------------------
{
	IFFCHUNK *pchunk = (IFFCHUNK *)pvchunk;
	if ((!pchunk->len) || (pchunk->len+8 > dwMaxLen)) return FALSE;
	if (pchunk->id == IFFID_LIST)
	{
		LISTCHUNK *plist = (LISTCHUNK *)pchunk;
		DWORD dwPos = 12;
		while (dwPos < plist->len)
		{
			LPIFFCHUNK p = (LPIFFCHUNK)(((LPBYTE)plist) + dwPos);
			if (!(p->id & 0xFF))
			{
				p = (LPIFFCHUNK)( ((LPBYTE)p)+1  );
				dwPos++;
			}
			if (dwPos + p->len + 8 <= plist->len + 12)
			{
				UpdateInstrumentDefinition(pins, p, p->len+8);
			}
			dwPos += p->len + 8;
		}
		switch(plist->listid)
		{
		case IFFID_rgn:
			if (pins->nRegions < DLSMAXREGIONS) pins->nRegions++;
			break;
		}
	} else
	{
		switch(pchunk->id)
		{
		case IFFID_insh:
			pins->ulBank = ((INSHCHUNK *)pchunk)->ulBank;
			pins->ulInstrument = ((INSHCHUNK *)pchunk)->ulInstrument;
			//Log("%3d regions, bank 0x%04X instrument %3d\n", ((INSHCHUNK *)pchunk)->cRegions, pins->ulBank, pins->ulInstrument);
			break;

		case IFFID_rgnh:
			if (pins->nRegions < DLSMAXREGIONS)
			{
				RGNHCHUNK *p = (RGNHCHUNK *)pchunk;
				DLSREGION *pregion = &pins->Regions[pins->nRegions];
				pregion->uKeyMin = (BYTE)p->RangeKey.usLow;
				pregion->uKeyMax = (BYTE)p->RangeKey.usHigh;
				pregion->fuOptions = (BYTE)(p->usKeyGroup & DLSREGION_KEYGROUPMASK);
				if (p->fusOptions & F_RGN_OPTION_SELFNONEXCLUSIVE) pregion->fuOptions |= DLSREGION_SELFNONEXCLUSIVE;
				//Log("  Region %d: fusOptions=0x%02X usKeyGroup=0x%04X ", pins->nRegions, p->fusOptions, p->usKeyGroup);
				//Log("KeyRange[%3d,%3d] ", p->RangeKey.usLow, p->RangeKey.usHigh);
			}
			break;

		case IFFID_wlnk:
			if (pins->nRegions < DLSMAXREGIONS)
			{
				DLSREGION *pregion = &pins->Regions[pins->nRegions];
				WLNKCHUNK *p = (WLNKCHUNK *)pchunk;
				pregion->nWaveLink = (WORD)p->ulTableIndex;
				if ((pregion->nWaveLink < 16384) && (pregion->nWaveLink >= m_nMaxWaveLink)) m_nMaxWaveLink = pregion->nWaveLink + 1;
				//Log("  WaveLink %d: fusOptions=0x%02X usPhaseGroup=0x%04X ", pins->nRegions, p->fusOptions, p->usPhaseGroup);
				//Log("ulChannel=%d ulTableIndex=%4d\n", p->ulChannel, p->ulTableIndex);
			}
			break;

		case IFFID_wsmp:
			if (pins->nRegions < DLSMAXREGIONS)
			{
				DLSREGION *pregion = &pins->Regions[pins->nRegions];
				WSMPCHUNK *p = (WSMPCHUNK *)pchunk;
				pregion->fuOptions |= DLSREGION_OVERRIDEWSMP;
				pregion->uUnityNote = (BYTE)p->usUnityNote;
				pregion->sFineTune = p->sFineTune;
				LONG lVolume = DLS32BitRelativeGainToLinear(p->lAttenuation) / 256;
				if (lVolume > 256) lVolume = 256;
				if (lVolume < 4) lVolume = 4;
				pregion->usVolume = (WORD)lVolume;
				//Log("  WaveSample %d: usUnityNote=%2d sFineTune=%3d ", pins->nRegions, p->usUnityNote, p->sFineTune);
				//Log("fulOptions=0x%04X loops=%d\n", p->fulOptions, p->cSampleLoops);
				if ((p->cSampleLoops) && (p->cbSize + sizeof(WSMPSAMPLELOOP) <= p->len))
				{
					WSMPSAMPLELOOP *ploop = (WSMPSAMPLELOOP *)(((LPBYTE)p)+8+p->cbSize);
					//Log("looptype=%2d loopstart=%5d loopend=%5d\n", ploop->ulLoopType, ploop->ulLoopStart, ploop->ulLoopLength);
					if (ploop->ulLoopLength > 3)
					{
						pregion->fuOptions |= DLSREGION_SAMPLELOOP;
						//if (ploop->ulLoopType) pregion->fuOptions |= DLSREGION_PINGPONGLOOP;
						pregion->ulLoopStart = ploop->ulLoopStart;
						pregion->ulLoopEnd = ploop->ulLoopStart + ploop->ulLoopLength;
					}
				}
			}
			break;

		case IFFID_art1:
			if (m_nEnvelopes < DLSMAXENVELOPES)
			{
				ART1CHUNK *p = (ART1CHUNK *)pchunk;
				if (pins->ulBank & F_INSTRUMENT_DRUMS)
				{
					if (pins->nRegions >= DLSMAXREGIONS) break;
				} else
				{
					pins->nMelodicEnv = m_nEnvelopes + 1;
				}
				if (p->cbSize+p->cConnectionBlocks*sizeof(CONNECTIONBLOCK) > p->len) break;
				DLSENVELOPE *penv = &m_Envelopes[m_nEnvelopes];
				memset(penv, 0, sizeof(DLSENVELOPE));
				penv->nDefPan = 128;
				penv->nVolSustainLevel = 128;
				//Log("  art1 (%3d bytes): cbSize=%d cConnectionBlocks=%d\n", p->len, p->cbSize, p->cConnectionBlocks);
				CONNECTIONBLOCK *pblk = (CONNECTIONBLOCK *)( ((LPBYTE)p)+8+p->cbSize );
				for (UINT iblk=0; iblk<p->cConnectionBlocks; iblk++, pblk++)
				{
					// [4-bit transform][12-bit dest][8-bit control][8-bit source] = 32-bit ID
					DWORD dwArticulation = pblk->usTransform;
					dwArticulation = (dwArticulation << 12) | (pblk->usDestination & 0x0FFF);
					dwArticulation = (dwArticulation << 8) | (pblk->usControl & 0x00FF);
					dwArticulation = (dwArticulation << 8) | (pblk->usSource & 0x00FF);
					switch(dwArticulation)
					{
					case ART_DEFAULTPAN:
						{
							LONG pan = 128 + pblk->lScale / (65536000/128);
							if (pan < 0) pan = 0;
							if (pan > 255) pan = 255;
							penv->nDefPan = (BYTE)pan;
						}
						break;

					case ART_VOL_EG_ATTACKTIME:
						// 32-bit time cents units. range = [0s, 20s]
						penv->wVolAttack = 0;
						if (pblk->lScale > -0x40000000)
						{
							LONG l = pblk->lScale - 78743200; // maximum velocity
							if (l > 0) l = 0;
							LONG attacktime = DLS32BitTimeCentsToMilliseconds(l);
							if (attacktime < 0) attacktime = 0;
							if (attacktime > 20000) attacktime = 20000;
							if (attacktime >= 20) penv->wVolAttack = (WORD)(attacktime / 20);
							//Log("%3d: Envelope Attack Time set to %d (%d time cents)\n", (DWORD)(pins->ulInstrument & 0x7F)|((pins->ulBank >> 16) & 0x8000), attacktime, pblk->lScale);
						}
						break;

					case ART_VOL_EG_DECAYTIME:
						// 32-bit time cents units. range = [0s, 20s]
						penv->wVolDecay = 0;
						if (pblk->lScale > -0x40000000)
						{
							LONG decaytime = DLS32BitTimeCentsToMilliseconds(pblk->lScale);
							if (decaytime > 20000) decaytime = 20000;
							if (decaytime >= 20) penv->wVolDecay = (WORD)(decaytime / 20);
							//Log("%3d: Envelope Decay Time set to %d (%d time cents)\n", (DWORD)(pins->ulInstrument & 0x7F)|((pins->ulBank >> 16) & 0x8000), decaytime, pblk->lScale);
						}
						break;

					case ART_VOL_EG_RELEASETIME:
						// 32-bit time cents units. range = [0s, 20s]
						penv->wVolRelease = 0;
						if (pblk->lScale > -0x40000000)
						{
							LONG releasetime = DLS32BitTimeCentsToMilliseconds(pblk->lScale);
							if (releasetime > 20000) releasetime = 20000;
							if (releasetime >= 20) penv->wVolRelease = (WORD)(releasetime / 20);
							//Log("%3d: Envelope Release Time set to %d (%d time cents)\n", (DWORD)(pins->ulInstrument & 0x7F)|((pins->ulBank >> 16) & 0x8000), penv->wVolRelease, pblk->lScale);
						}
						break;

					case ART_VOL_EG_SUSTAINLEVEL:
						// 0.1% units
						if (pblk->lScale >= 0)
						{
							LONG l = pblk->lScale / (1000*512);
							if ((l >= 0) || (l <= 128)) penv->nVolSustainLevel = (BYTE)l;
							//Log("%3d: Envelope Sustain Level set to %d (%d)\n", (DWORD)(pins->ulInstrument & 0x7F)|((pins->ulBank >> 16) & 0x8000), l, pblk->lScale);
						}
						break;

					//default:
					//	Log("    Articulation = 0x%08X value=%d\n", dwArticulation, pblk->lScale);
					}
				}
				m_nEnvelopes++;
			}
			break;

		case IFFID_INAM:
			{
				UINT len = (pchunk->len < 32) ? pchunk->len : 31;
				if (len) memcpy(pins->szName, ((LPCSTR)pchunk)+8, len);
				pins->szName[31] = 0;
				//Log("%s\n", (DWORD)pins->szName);
			}
			break;
	#if 0
		default:
			{
				CHAR sid[5];
				memcpy(sid, &pchunk->id, 4);
				sid[4] = 0;
				Log("    \"%s\": %d bytes\n", (DWORD)sid, pchunk->len);
			}
	#endif
		}
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////
// Converts SF2 chunks to DLS

BOOL CDLSBank::UpdateSF2PresetData(LPVOID pvsf2, LPVOID pvchunk, DWORD dwMaxLen)
//------------------------------------------------------------------------------
{
	SF2LOADERINFO *psf2 = (SF2LOADERINFO *)pvsf2;
	IFFCHUNK *pchunk = (IFFCHUNK *)pvchunk;
	if ((!pchunk->len) || (pchunk->len+8 > dwMaxLen)) return FALSE;
	switch(pchunk->id)
	{
	case IFFID_phdr:
		if (m_nInstruments) break;
		m_nInstruments = pchunk->len / sizeof(SFPRESETHEADER);
		if (m_nInstruments) m_nInstruments--; // Disgard EOP
		if (!m_nInstruments) break;
		m_pInstruments = new DLSINSTRUMENT[m_nInstruments];
		if (m_pInstruments)
		{
			memset(m_pInstruments, 0, m_nInstruments * sizeof(DLSINSTRUMENT));
		#ifdef DLSBANK_LOG
			Log("phdr: %d instruments\n", m_nInstruments);
		#endif
			SFPRESETHEADER *psfh = (SFPRESETHEADER *)(pchunk+1);
			DLSINSTRUMENT *pins = m_pInstruments;
			for (UINT i=0; i<m_nInstruments; i++, psfh++, pins++)
			{
				memcpy(pins->szName, psfh->achPresetName, 20);
				pins->szName[20] = 0;
				pins->ulInstrument = psfh->wPreset & 0x7F;
				pins->ulBank = (psfh->wBank >= 128) ? F_INSTRUMENT_DRUMS : (psfh->wBank << 8);
				pins->wPresetBagNdx = psfh->wPresetBagNdx;
				pins->wPresetBagNum = 1;
				if (psfh[1].wPresetBagNdx > pins->wPresetBagNdx) pins->wPresetBagNum = (WORD)(psfh[1].wPresetBagNdx - pins->wPresetBagNdx);
			}
		}
		break;

	case IFFID_pbag:
		if (m_pInstruments)
		{
			UINT nBags = pchunk->len / sizeof(SFPRESETBAG);
			if (nBags)
			{
				psf2->nPresetBags = nBags;
				psf2->pPresetBags = (SFPRESETBAG *)(pchunk+1);
			}
		}
	#ifdef DLSINSTR_LOG
		else Log("pbag: no instruments!\n");
	#endif
		break;

	case IFFID_pgen:
		if (m_pInstruments)
		{
			UINT nGens = pchunk->len / sizeof(SFGENLIST);
			if (nGens)
			{
				psf2->nPresetGens = nGens;
				psf2->pPresetGens = (SFGENLIST *)(pchunk+1);
			}
		}
	#ifdef DLSINSTR_LOG
		else Log("pgen: no instruments!\n");
	#endif
		break;

	case IFFID_inst:
		if (m_pInstruments)
		{
			UINT nIns = pchunk->len / sizeof(SFINST);
			psf2->nInsts = nIns;
			psf2->pInsts = (SFINST *)(pchunk+1);
		}
		break;

	case IFFID_ibag:
		if (m_pInstruments)
		{
			UINT nBags = pchunk->len / sizeof(SFINSTBAG);
			if (nBags)
			{
				psf2->nInstBags = nBags;
				psf2->pInstBags = (SFINSTBAG *)(pchunk+1);
			}
		}
		break;

	case IFFID_igen:
		if (m_pInstruments)
		{
			UINT nGens = pchunk->len / sizeof(SFINSTGENLIST);
			if (nGens)
			{
				psf2->nInstGens = nGens;
				psf2->pInstGens = (SFINSTGENLIST *)(pchunk+1);
			}
		}
		break;

	case IFFID_shdr:
		if (m_pSamplesEx) break;
		m_nSamplesEx = pchunk->len / sizeof(SFSAMPLE);
	#ifdef DLSINSTR_LOG
		Log("shdr: %d samples\n", m_nSamplesEx);
	#endif
		if (m_nSamplesEx < 1) break;
		m_nWaveForms = m_nSamplesEx;
		m_pSamplesEx = new DLSSAMPLEEX[m_nSamplesEx];
		m_pWaveForms = new DWORD[m_nWaveForms];
		if ((m_pSamplesEx) && (m_pWaveForms))
		{
			memset(m_pSamplesEx, 0, sizeof(DLSSAMPLEEX)*m_nSamplesEx);
			memset(m_pWaveForms, 0, sizeof(DWORD)*m_nWaveForms);
			DLSSAMPLEEX *psmp = m_pSamplesEx;
			SFSAMPLE *p = (SFSAMPLE *)(pchunk+1);
			for (UINT i=0; i<m_nSamplesEx; i++, psmp++, p++)
			{
				memcpy(psmp->szName, p->achSampleName, 20);
				psmp->dwLen = 0;
				psmp->dwSampleRate = p->dwSampleRate;
				psmp->byOriginalPitch = p->byOriginalPitch;
				psmp->chPitchCorrection = p->chPitchCorrection;
				if (((p->sfSampleType & 0x7FFF) <= 4) && (p->dwStart < 0x08000000) && (p->dwEnd >= p->dwStart+8))
				{
					psmp->dwLen = (p->dwEnd - p->dwStart) * 2;
					if ((p->dwEndloop > p->dwStartloop + 7) && (p->dwStartloop > p->dwStart))
					{
						psmp->dwStartloop = p->dwStartloop - p->dwStart;
						psmp->dwEndloop = p->dwEndloop - p->dwStart;
					}
					m_pWaveForms[i] = p->dwStart * 2;
					//Log("  offset[%d]=%d len=%d\n", i, p->dwStart*2, psmp->dwLen);
				}
			}
		}
		break;
	
	#ifdef DLSINSTR_LOG
	default:
		{
			CHAR sdbg[5];
			memcpy(sdbg, &pchunk->id, 4);
			sdbg[4] = 0;
			Log("Unsupported SF2 chunk: %s (%d bytes)\n", sdbg, pchunk->len);
		}
	#endif
	}
	return TRUE;
}


// Convert all instruments to the DLS format
BOOL CDLSBank::ConvertSF2ToDLS(LPVOID pvsf2info)
//----------------------------------------------
{
	SF2LOADERINFO *psf2;
	DLSINSTRUMENT *pins;

	if ((!m_pInstruments) || (!m_pSamplesEx)) return FALSE;
	psf2 = (SF2LOADERINFO *)pvsf2info;
	pins = m_pInstruments;
	for (UINT nIns=0; nIns<m_nInstruments; nIns++, pins++)
	{
		DLSENVELOPE env;
		UINT nInstrNdx = 0;
		LONG lAttenuation = 0;
		// Default Envelope Values
		env.wVolAttack = 0;
		env.wVolDecay = 0;
		env.wVolRelease = 0;
		env.nVolSustainLevel = 128;
		env.nDefPan = 128;
		// Load Preset Bags
		for (UINT ipbagcnt=0; ipbagcnt<(UINT)pins->wPresetBagNum; ipbagcnt++)
		{
			UINT ipbagndx = pins->wPresetBagNdx + ipbagcnt;
			if ((ipbagndx+1 >= psf2->nPresetBags) || (!psf2->pPresetBags)) break;
			// Load generators for each preset bag
			SFPRESETBAG *pbag = psf2->pPresetBags + ipbagndx;
			for (UINT ipgenndx=pbag[0].wGenNdx; ipgenndx<pbag[1].wGenNdx; ipgenndx++)
			{
				if ((!psf2->pPresetGens) || (ipgenndx+1 >= psf2->nPresetGens)) break;
				SFGENLIST *pgen = psf2->pPresetGens + ipgenndx;
				switch(pgen->sfGenOper)
				{
				case SF2_GEN_DECAYVOLENV:
					{
						LONG decaytime = DLS32BitTimeCentsToMilliseconds(((LONG)(short int)pgen->genAmount)<<16);
						if (decaytime > 20000) decaytime = 20000;
						if (decaytime >= 20) env.wVolDecay = (WORD)(decaytime / 20);
						//Log("  vol decay time set to %d\n", decaytime);
					}
					break;
				case SF2_GEN_RELEASEVOLENV:
					{
						LONG releasetime = DLS32BitTimeCentsToMilliseconds(((LONG)(short int)pgen->genAmount)<<16);
						if (releasetime > 20000) releasetime = 20000;
						if (releasetime >= 20) env.wVolRelease = (WORD)(releasetime / 20);
						//Log("  vol release time set to %d\n", releasetime);
					}
					break;
				case SF2_GEN_INSTRUMENT:
					nInstrNdx = pgen->genAmount + 1;
					break;
				case SF2_GEN_ATTENUATION:
					lAttenuation = - (int)(WORD)(pgen->genAmount);
					break;
#if 0
				default:
					Log("Ins %3d: bag %3d gen %3d: ", nIns, ipbagndx, ipgenndx);
					Log("genoper=%d amount=0x%04X ", pgen->sfGenOper, pgen->genAmount);
					Log((pins->ulBank & F_INSTRUMENT_DRUMS) ? "(drum)\n" : "\n");
#endif
				}
			}
		}
		// Envelope
		if ((m_nEnvelopes < DLSMAXENVELOPES) && (!(pins->ulBank & F_INSTRUMENT_DRUMS)))
		{
			m_Envelopes[m_nEnvelopes] = env;
			m_nEnvelopes++;
			pins->nMelodicEnv = m_nEnvelopes;
		}
		// Load Instrument Bags
		if ((!nInstrNdx) || (nInstrNdx >= psf2->nInsts) || (!psf2->pInsts)) continue;
		nInstrNdx--;
		pins->nRegions = psf2->pInsts[nInstrNdx+1].wInstBagNdx - psf2->pInsts[nInstrNdx].wInstBagNdx;
		//Log("\nIns %3d, %2d regions:\n", nIns, pins->nRegions);
		if (pins->nRegions > DLSMAXREGIONS) pins->nRegions = DLSMAXREGIONS;
		DLSREGION *pRgn = pins->Regions;
		for (UINT nRgn=0; nRgn<pins->nRegions; nRgn++, pRgn++)
		{
			UINT ibagcnt = psf2->pInsts[nInstrNdx].wInstBagNdx + nRgn;
			if ((ibagcnt >= psf2->nInstBags) || (!psf2->pInstBags)) break;
			// Create a new envelope for drums
			DLSENVELOPE *penv = &env;
			if (pins->ulBank & F_INSTRUMENT_DRUMS)
			{
				if ((m_nEnvelopes < DLSMAXENVELOPES) && (!(pins->ulBank & F_INSTRUMENT_DRUMS)))
				{
					m_Envelopes[m_nEnvelopes] = env;
					penv = &m_Envelopes[m_nEnvelopes];
					m_nEnvelopes++;
					pRgn->uPercEnv = (WORD)m_nEnvelopes;
				}
			} else
			if (pins->nMelodicEnv)
			{
				penv = &m_Envelopes[pins->nMelodicEnv-1];
			}
			// Region Default Values
			LONG lAttn = lAttenuation;
			pRgn->uUnityNote = 0xFF;	// 0xFF means undefined -> use sample
			// Load Generators
			SFINSTBAG *pbag = psf2->pInstBags + ibagcnt;
			for (UINT igenndx=pbag[0].wGenNdx; igenndx<pbag[1].wGenNdx; igenndx++)
			{
				if ((igenndx >= psf2->nInstGens) || (!psf2->pInstGens)) break;
				SFINSTGENLIST *pgen = psf2->pInstGens + igenndx;
				WORD value = pgen->genAmount;
				switch(pgen->sfGenOper)
				{
				case SF2_GEN_KEYRANGE:
					pRgn->uKeyMin = (BYTE)(value & 0xFF);
					pRgn->uKeyMax = (BYTE)(value >> 8);
					if (pRgn->uKeyMin > pRgn->uKeyMax)
					{
						BYTE b = pRgn->uKeyMax;
						pRgn->uKeyMax = pRgn->uKeyMin;
						pRgn->uKeyMin = b;
					}
					//if (nIns == 9) Log("  keyrange: %d-%d\n", pRgn->uKeyMin, pRgn->uKeyMax);
					break;
				case SF2_GEN_UNITYNOTE:
					if (value < 128) pRgn->uUnityNote = (BYTE)value;
					break;
				case SF2_GEN_RELEASEVOLENV:
					{
						LONG releasetime = DLS32BitTimeCentsToMilliseconds(((LONG)(short int)pgen->genAmount)<<16);
						if (releasetime > 20000) releasetime = 20000;
						if (releasetime >= 20) penv->wVolRelease = (WORD)(releasetime / 20);
						//Log("  vol release time set to %d\n", releasetime);
					}
					break;
				case SF2_GEN_PAN:
					{
						int pan = (short int)value;
						pan = (((pan + 500) * 127) / 500) + 128;
						if (pan < 0) pan = 0;
						if (pan > 255) pan = 255;
						penv->nDefPan = (BYTE)pan;
					}
					break;
				case SF2_GEN_ATTENUATION:
					lAttn = -(int)value;
					break;
				case SF2_GEN_SAMPLEID:
					//if (nIns == 9) Log("Region %d/%d: SampleID = %d\n", nRgn, pins->nRegions, value);
					if ((m_pSamplesEx) && ((UINT)value < m_nSamplesEx))
					{
						pRgn->nWaveLink = value;
						pRgn->ulLoopStart = m_pSamplesEx[value].dwStartloop;
						pRgn->ulLoopEnd = m_pSamplesEx[value].dwEndloop;
					}
					break;
				case SF2_GEN_SAMPLEMODES:
					value &= 3;
					pRgn->fuOptions &= ~(DLSREGION_SAMPLELOOP|DLSREGION_PINGPONGLOOP|DLSREGION_SUSTAINLOOP);
					if (value == 1) pRgn->fuOptions |= DLSREGION_SAMPLELOOP; else
					if (value == 2) pRgn->fuOptions |= DLSREGION_SAMPLELOOP|DLSREGION_PINGPONGLOOP; else
					if (value == 3) pRgn->fuOptions |= DLSREGION_SAMPLELOOP|DLSREGION_SUSTAINLOOP;
					pRgn->fuOptions |= DLSREGION_OVERRIDEWSMP;
					break;
				case SF2_GEN_KEYGROUP:
					pRgn->fuOptions |= (BYTE)(value & DLSREGION_KEYGROUPMASK);
					break;
				//default:
				//	Log("    gen=%d value=%04X\n", pgen->sfGenOper, pgen->genAmount);
				}
			}
			LONG lVolume = DLS32BitRelativeGainToLinear((lAttn/10) << 16) / 256;
			if (lVolume < 16) lVolume = 16;
			if (lVolume > 256) lVolume = 256;
			pRgn->usVolume = (WORD)lVolume;
			//Log("\n");
		}
	}
	return TRUE;
}


///////////////////////////////////////////////////////////////
// Open: opens a DLS bank

BOOL CDLSBank::Open(LPCSTR lpszFileName)
//--------------------------------------
{
	SF2LOADERINFO sf2info;
	const BYTE *lpMemFile;	// Pointer to memory-mapped file
	RIFFCHUNKID *priff;
	DWORD dwMemPos, dwMemLength;
	UINT nInsDef;

	if ((!lpszFileName) || (!lpszFileName[0])) return FALSE;
	strcpy(m_szFileName, lpszFileName);
	lpMemFile = NULL;
	// Memory-Mapped file
	CMappedFile MapFile;
	if (!MapFile.Open(lpszFileName)) return FALSE;
	dwMemLength = MapFile.GetLength();
	if (dwMemLength >= 256) lpMemFile = MapFile.Lock();
	if (!lpMemFile)
	{
		MapFile.Close();
		return FALSE;
	}

#ifdef DLSBANK_LOG
	Log("\nOpening DLS bank: %s\n", m_szFileName);
#endif

	priff = (RIFFCHUNKID *)lpMemFile;
	dwMemPos = 0;
	
	// Check DLS sections embedded in RMI midi files
	if ((priff->id_RIFF == IFFID_RIFF) && (priff->id_DLS == IFFID_RMID))
	{
		dwMemPos = 12;
		while (dwMemPos + 12 <= dwMemLength)
		{
			priff = (RIFFCHUNKID *)(lpMemFile + dwMemPos);
			if ((priff->id_RIFF == IFFID_RIFF) && (priff->id_DLS == IFFID_DLS)) break;
			dwMemPos += priff->riff_len + 8;
		}
	}

	// Check XDLS sections embedded in big endian IFF files
	if (priff->id_RIFF == IFFID_FORM)
	{
		do {
			priff = (RIFFCHUNKID *)(lpMemFile + dwMemPos);
			int len = BigEndian(priff->riff_len);
			if ((len <= 4) || ((DWORD)len >= dwMemLength - dwMemPos)) break;
			if (priff->id_DLS == IFFID_XDLS)
			{
				dwMemPos += 12;
				priff = (RIFFCHUNKID *)(lpMemFile + dwMemPos);
				break;
			}
			dwMemPos += len + 8;
		} while (dwMemPos + 24 < dwMemLength);
	}
	if ((priff->id_RIFF != IFFID_RIFF)
	 || ((priff->id_DLS != IFFID_DLS) && (priff->id_DLS != IFFID_MLS) && (priff->id_DLS != IFFID_sfbk))
	 || (dwMemPos + priff->riff_len > dwMemLength-8))
	{
		MapFile.Unlock();
		MapFile.Close();
	#ifdef DLSBANK_LOG
		Log("Invalid DLS bank!\n");
	#endif
		return FALSE;
	}
	memset(&sf2info, 0, sizeof(sf2info));
	m_nType = (priff->id_DLS == IFFID_sfbk) ? SOUNDBANK_TYPE_SF2 : SOUNDBANK_TYPE_DLS;
	m_dwWavePoolOffset = 0;
	m_nInstruments = 0;
	m_nWaveForms = 0;
	m_nEnvelopes = 0;
	m_pInstruments = NULL;
	m_pWaveForms = NULL;
	nInsDef = 0;
	if (dwMemLength > 8 + priff->riff_len + dwMemPos) dwMemLength = 8 + priff->riff_len + dwMemPos;
	dwMemPos += sizeof(RIFFCHUNKID);
	while (dwMemPos + sizeof(IFFCHUNK) < dwMemLength)
	{
		IFFCHUNK *pchunk = (IFFCHUNK *)(lpMemFile + dwMemPos);

		if (dwMemPos + 8 + pchunk->len > dwMemLength) break;
		switch(pchunk->id)
		{
		// DLS 1.0: Instruments Collection Header
		case IFFID_colh:
		#ifdef DLSBANK_LOG
			Log("colh (%d bytes)\n", pchunk->len);
		#endif
			if (!m_pInstruments)
			{
				m_nInstruments = ((COLHCHUNK *)pchunk)->ulInstruments;
				if (m_nInstruments)
				{
					m_pInstruments = new DLSINSTRUMENT[m_nInstruments];
					if (m_pInstruments) memset(m_pInstruments, 0, m_nInstruments * sizeof(DLSINSTRUMENT));
				}
			#ifdef DLSBANK_LOG
				Log("  %d instruments\n", m_nInstruments);
			#endif
			}
			break;

		// DLS 1.0: Instruments Pointers Table
		case IFFID_ptbl:
		#ifdef DLSBANK_LOG
			Log("ptbl (%d bytes)\n", pchunk->len);
		#endif
			if (!m_pWaveForms)
			{
				m_nWaveForms = ((PTBLCHUNK *)pchunk)->cCues;
				if (m_nWaveForms)
				{
					m_pWaveForms = new DWORD[m_nWaveForms];
					if (m_pWaveForms)
					{
						memcpy(m_pWaveForms, (lpMemFile + dwMemPos + 8 + ((PTBLCHUNK *)pchunk)->cbSize), m_nWaveForms * sizeof(DWORD));
					}
				}
			#ifdef DLSBANK_LOG
				Log("  %d waveforms\n", m_nWaveForms);
			#endif
			}
			break;

		// DLS 1.0: LIST section
		case IFFID_LIST:
		#ifdef DLSBANK_LOG
			Log("LIST\n");
		#endif
			{
				LISTCHUNK *plist = (LISTCHUNK *)pchunk;
				DWORD dwPos = dwMemPos + sizeof(LISTCHUNK);
				DWORD dwMaxPos = dwMemPos + 8 + plist->len;
				if (dwMaxPos > dwMemLength) dwMaxPos = dwMemLength;
				if (((plist->listid == IFFID_wvpl) && (m_nType & SOUNDBANK_TYPE_DLS))
				 || ((plist->listid == IFFID_sdta) && (m_nType & SOUNDBANK_TYPE_SF2)))
				{
					m_dwWavePoolOffset = dwPos;
				#ifdef DLSBANK_LOG
					Log("Wave Pool offset: %d\n", m_dwWavePoolOffset);
				#endif
					break;
				}
				while (dwPos + 12 < dwMaxPos)
				{
					if (!(lpMemFile[dwPos])) dwPos++;
					LISTCHUNK *psublist = (LISTCHUNK *)(lpMemFile+dwPos);
					if (dwPos + psublist->len + 8 > dwMemLength) break;
					// DLS Instrument Headers
					if ((psublist->id == IFFID_LIST) && (m_nType & SOUNDBANK_TYPE_DLS))
					{
						if ((psublist->listid == IFFID_ins) && (nInsDef < m_nInstruments) && (m_pInstruments))
						{
							DLSINSTRUMENT *pins = &m_pInstruments[nInsDef];
							//Log("Instrument %d:\n", nInsDef);
							UpdateInstrumentDefinition(pins, (IFFCHUNK *)psublist, psublist->len + 8);
							nInsDef++;
						}
					} else
					// DLS/SF2 Bank Information
					if ((plist->listid == IFFID_INFO) && (psublist->len))
					{
						UINT len = (psublist->len < 255) ? psublist->len : 255;
						LPCSTR pszInfo = (LPCSTR)(lpMemFile+dwPos+8);
						switch(psublist->id)
						{
						case IFFID_INAM:
							lstrcpyn(m_BankInfo.szBankName, pszInfo, len);
							break;
						case IFFID_IENG:
							lstrcpyn(m_BankInfo.szEngineer, pszInfo, len);
							break;
						case IFFID_ICOP:
							lstrcpyn(m_BankInfo.szCopyRight, pszInfo, len);
							break;
						case IFFID_ICMT:
							len = psublist->len;
							if (len > sizeof(m_BankInfo.szComments)-1) len = sizeof(m_BankInfo.szComments)-1;
							lstrcpyn(m_BankInfo.szComments, pszInfo, len);
							break;
						case IFFID_ISFT:
							lstrcpyn(m_BankInfo.szSoftware, pszInfo, len);
							break;
						case IFFID_ISBJ:
							lstrcpyn(m_BankInfo.szDescription, pszInfo, len);
							break;
						}
					} else
					if ((plist->listid == IFFID_pdta) && (m_nType & SOUNDBANK_TYPE_SF2))
					{
						UpdateSF2PresetData(&sf2info, (IFFCHUNK *)psublist, psublist->len + 8);
					}
					dwPos += 8 + psublist->len;
				}
			}
			break;

		#ifdef DLSBANK_LOG
		default:
			{
				CHAR sdbg[5];
				memcpy(sdbg, &pchunk->id, 4);
				sdbg[4] = 0;
				Log("Unsupported chunk: %s (%d bytes)\n", sdbg, pchunk->len);
			}
			break;
		#endif
		}
		dwMemPos += 8 + pchunk->len;
	}
	// Build the ptbl is not present in file
	if ((!m_pWaveForms) && (m_dwWavePoolOffset) && (m_nType & SOUNDBANK_TYPE_DLS) && (m_nMaxWaveLink > 0))
	{
	#ifdef DLSBANK_LOG
		Log("ptbl not present: building table (%d wavelinks)...\n", m_nMaxWaveLink);
	#endif
		m_pWaveForms = new DWORD[m_nMaxWaveLink];
		if (m_pWaveForms)
		{
			memset(m_pWaveForms, 0, m_nMaxWaveLink * sizeof(DWORD));
			dwMemPos = m_dwWavePoolOffset;
			while (dwMemPos + sizeof(IFFCHUNK) < dwMemLength)
			{
				IFFCHUNK *pchunk = (IFFCHUNK *)(lpMemFile + dwMemPos);
				if (pchunk->id == IFFID_LIST) m_pWaveForms[m_nWaveForms++] = dwMemPos - m_dwWavePoolOffset;
				dwMemPos += 8 + pchunk->len;
				if (m_nWaveForms >= m_nMaxWaveLink) break;
			}
		#ifdef DLSBANK_LOG
			Log("Found %d waveforms\n", m_nWaveForms);
		#endif
		}
	}
	// Convert the SF2 data to DLS
	if ((m_nType & SOUNDBANK_TYPE_SF2) && (m_pSamplesEx) && (m_pInstruments))
	{
		ConvertSF2ToDLS(&sf2info);
	}
	MapFile.Unlock();
	MapFile.Close();
#ifdef DLSBANK_LOG
	Log("DLS bank closed\n");
#endif
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////
// Extracts the WaveForms from a DLS bank

UINT CDLSBank::GetRegionFromKey(UINT nIns, UINT nKey)
//---------------------------------------------------
{
	DLSINSTRUMENT *pins;
	
	if ((!m_pInstruments) || (nIns >= m_nInstruments)) return 0;
	pins = &m_pInstruments[nIns];
	for (UINT rgn=0; rgn<pins->nRegions; rgn++)
	{
		if ((nKey >= pins->Regions[rgn].uKeyMin) && (nKey <= pins->Regions[rgn].uKeyMax))
		{
			return rgn;
		}
	}
	return 0;
}


BOOL CDLSBank::FreeWaveForm(LPBYTE p)
//-----------------------------------
{
	if (p) GlobalFreePtr(p);
	return TRUE;
}


BOOL CDLSBank::ExtractWaveForm(UINT nIns, UINT nRgn, LPBYTE *ppWave, DWORD *pLen)
//-------------------------------------------------------------------------------
{
	DLSINSTRUMENT *pins;
	DWORD dwOffset;
	UINT nWaveLink;
	FILE *f;
	BOOL bOk = FALSE;

	if ((!ppWave) || (!pLen) || (!m_pInstruments) || (nIns >= m_nInstruments)
	 || (!m_dwWavePoolOffset) || (!m_pWaveForms))
	{
	#ifdef DLSBANK_LOG
		Log("ExtractWaveForm(%d) failed: m_nInstruments=%d m_dwWavePoolOffset=%d m_pWaveForms=0x%08X\n", nIns, m_nInstruments, m_dwWavePoolOffset, m_pWaveForms);
	#endif
		return FALSE;
	}
	*ppWave = NULL;
	*pLen = 0;
	pins = &m_pInstruments[nIns];
	if (nRgn >= pins->nRegions)
	{
	#ifdef DLSBANK_LOG
		Log("invalid waveform region: nIns=%d nRgn=%d pins->nRegions=%d\n", nIns, nRgn, pins->nRegions);
	#endif
		return FALSE;
	}
	nWaveLink = pins->Regions[nRgn].nWaveLink;
	if (nWaveLink >= m_nWaveForms)
	{
	#ifdef DLSBANK_LOG
		Log("Invalid wavelink id: nWaveLink=%d nWaveForms=%d\n", nWaveLink, m_nWaveForms);
	#endif
		return FALSE;
	}
	dwOffset = m_pWaveForms[nWaveLink] + m_dwWavePoolOffset;
	if ((f = fopen(m_szFileName, "rb")) == NULL) return FALSE;
	if (fseek(f, dwOffset, SEEK_SET) == 0)
	{
		if (m_nType & SOUNDBANK_TYPE_SF2)
		{
			if ((m_pSamplesEx) && (m_pSamplesEx[nWaveLink].dwLen))
			{
				if (fseek(f, 8, SEEK_CUR) == 0)
				{
					*pLen = m_pSamplesEx[nWaveLink].dwLen;
					*ppWave = (LPBYTE)GlobalAllocPtr(GHND, *pLen + 8);
					fread((*ppWave), 1, *pLen, f);
					bOk = TRUE;
				}
			}
		} else
		{
			LISTCHUNK chunk;
			if (fread(&chunk, 1, 12, f) == 12)
			{
				if ((chunk.id == IFFID_LIST) && (chunk.listid == IFFID_wave) && (chunk.len > 4))
				{
					*pLen = chunk.len + 8;
					*ppWave = (LPBYTE)GlobalAllocPtr(GHND, chunk.len + 8);
					if (*ppWave)
					{
						memcpy((*ppWave), &chunk, 12);
						fread((*ppWave)+12, 1, *pLen-12, f);
						bOk = TRUE;
					}
				}
			}
		}
	}
	fclose(f);
	return bOk;
}


// returns 12*128*(log2(freq/8363)+midiftune/100)
static int DlsFreqToTranspose(ULONG freq, int nMidiFTune)
//-------------------------------------------------------
{
	const float _f1_8363 = 1.0f / 8363.0f;
	const float _factor = 128 * 12;
	const float _fct_100 = 128.0f / 100.0f;
	int result;
	
	if (!freq) return 0;
	_asm {
	fild nMidiFTune
	fld _fct_100
	fmulp st(1), st(0)
	fld _factor
	fild freq
	fld _f1_8363
	fmulp st(1), st(0)
	fyl2x
	faddp st(1), st(0)
	fistp result
	}
	return result;
}


BOOL CDLSBank::ExtractSample(CSoundFile *pSndFile, UINT nSample, UINT nIns, UINT nRgn, int transpose)
//---------------------------------------------------------------------------------------------------
{
	DLSINSTRUMENT *pins;
	LPBYTE pWaveForm = NULL;
	DWORD dwLen = 0, dwWSMPOffset = 0;
	BOOL bOk, bWaveForm;
	
	if ((!m_pInstruments) || (nIns >= m_nInstruments) || (!pSndFile)) return FALSE;
	pins = &m_pInstruments[nIns];
	if (nRgn >= pins->nRegions) return FALSE;
	if (!ExtractWaveForm(nIns, nRgn, &pWaveForm, &dwLen)) return FALSE;
	if ((!pWaveForm) || (dwLen < 16)) return FALSE;
	bOk = FALSE;
	if (m_nType & SOUNDBANK_TYPE_SF2)
	{
		pSndFile->DestroySample(nSample);
		UINT nWaveLink = pins->Regions[nRgn].nWaveLink;
		MODINSTRUMENT *psmp = &pSndFile->Ins[nSample];
		if (pSndFile->m_nSamples < nSample) pSndFile->m_nSamples = nSample;
		if ((nWaveLink < m_nSamplesEx) && (m_pSamplesEx))
		{
			DLSSAMPLEEX *p = &m_pSamplesEx[nWaveLink];
		#ifdef DLSINSTR_LOG
			Log("  SF2 WaveLink #%3d: %5dHz\n", nWaveLink, p->dwSampleRate);
		#endif
			psmp->nLength = dwLen / 2;
			psmp->uFlags = CHN_16BIT;
			psmp->nLoopStart = pins->Regions[nRgn].ulLoopStart;
			psmp->nLoopEnd = pins->Regions[nRgn].ulLoopEnd;
			psmp->nC4Speed = p->dwSampleRate;
			psmp->nGlobalVol = 64;
			psmp->nVolume = 256;
			psmp->nPan = 128;
			pSndFile->ReadSample(psmp, RS_PCM16S, (LPSTR)pWaveForm, dwLen);
			psmp->RelativeTone = p->byOriginalPitch;
			psmp->nFineTune = p->chPitchCorrection;
		}
		bWaveForm = (psmp->pSample) ? TRUE : FALSE;
	} else
	{
		bWaveForm = pSndFile->ReadWAVSample(nSample, pWaveForm, dwLen, &dwWSMPOffset);
	}
	if (bWaveForm)
	{
		MODINSTRUMENT *psmp = &pSndFile->Ins[nSample];
		DLSREGION *pRgn = &pins->Regions[nRgn];
		psmp->uFlags &= ~(CHN_LOOP|CHN_PINGPONGLOOP|CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN);
		if (pRgn->fuOptions & DLSREGION_SAMPLELOOP) psmp->uFlags |= CHN_LOOP;
		if (pRgn->fuOptions & DLSREGION_SUSTAINLOOP) psmp->uFlags |= CHN_SUSTAINLOOP;
		if (pRgn->fuOptions & DLSREGION_PINGPONGLOOP) psmp->uFlags |= CHN_PINGPONGLOOP;
		if (psmp->uFlags & (CHN_LOOP|CHN_SUSTAINLOOP))
		{
			if (pRgn->ulLoopEnd > pRgn->ulLoopStart)
			{
				if (psmp->uFlags & CHN_SUSTAINLOOP)
				{
					psmp->nSustainStart = pRgn->ulLoopStart;
					psmp->nSustainEnd = pRgn->ulLoopEnd;
				} else
				{
					psmp->nLoopStart = pRgn->ulLoopStart;
					psmp->nLoopEnd = pRgn->ulLoopEnd;
				}
			} else
			{
				psmp->uFlags &= ~(CHN_LOOP|CHN_SUSTAINLOOP);
			}
		}
		// WSMP chunk
		{
			UINT usUnityNote = pRgn->uUnityNote;
			int sFineTune = pRgn->sFineTune;
			int lVolume = pRgn->usVolume;
			if ((dwWSMPOffset) && (!(pRgn->fuOptions & DLSREGION_OVERRIDEWSMP)))
			{
				WSMPCHUNK *p = (WSMPCHUNK *)(pWaveForm + dwWSMPOffset);
				usUnityNote = p->usUnityNote;
				sFineTune = p->sFineTune;
				lVolume = DLS32BitRelativeGainToLinear(p->lAttenuation) / 256;
				if (p->cSampleLoops)
				{
					WSMPSAMPLELOOP *ploop = (WSMPSAMPLELOOP *)(pWaveForm+dwWSMPOffset+8+p->cbSize);
					if (ploop->ulLoopLength > 3)
					{
						psmp->uFlags |= CHN_LOOP;
						//if (ploop->ulLoopType) psmp->uFlags |= CHN_PINGPONGLOOP;
						psmp->nLoopStart = ploop->ulLoopStart;
						psmp->nLoopEnd = ploop->ulLoopStart + ploop->ulLoopLength;
					}
				}
			} else
			if (m_nType & SOUNDBANK_TYPE_SF2)
			{
				usUnityNote += psmp->RelativeTone - 60;
				sFineTune += psmp->nFineTune;
			}
		#ifdef DLSINSTR_LOG
			Log("WSMP: usUnityNote=%d.%d, %dHz (transp=%d)\n", usUnityNote, sFineTune, psmp->nC4Speed, transpose);
		#endif
			if (usUnityNote > 0x7F) usUnityNote = 60;
			int nBaseTune = DlsFreqToTranspose(
								psmp->nC4Speed,
								sFineTune+(60 + transpose - usUnityNote)*100);
			psmp->nFineTune = (CHAR)(nBaseTune & 0x7F);
			psmp->RelativeTone = (CHAR)(nBaseTune >> 7);
			psmp->nC4Speed = CSoundFile::TransposeToFrequency(psmp->RelativeTone, psmp->nFineTune);
			if (lVolume > 256) lVolume = 256;
			if (lVolume < 16) lVolume = 16;
			psmp->nGlobalVol = (BYTE)(lVolume / 4);	// 0-64
		}
		if (pins->ulBank & F_INSTRUMENT_DRUMS)
		{
			if ((pRgn->uPercEnv) && (pRgn->uPercEnv <= m_nEnvelopes))
			{
				psmp->nPan = m_Envelopes[pRgn->uPercEnv-1].nDefPan;
				if (pSndFile->m_nType & MOD_TYPE_XM) psmp->uFlags |= CHN_PANNING;
			}
		} else
		{
			if ((pins->nMelodicEnv) && (pins->nMelodicEnv <= m_nEnvelopes))
			{
				psmp->nPan = m_Envelopes[pins->nMelodicEnv-1].nDefPan;
				if (pSndFile->m_nType & MOD_TYPE_XM) psmp->uFlags |= CHN_PANNING;
			}
		}
		if (pins->szName[0]) memcpy(pSndFile->m_szNames[nSample], pins->szName, 32);
		bOk = TRUE;
	}
	FreeWaveForm(pWaveForm);
	return bOk;
}


BOOL CDLSBank::ExtractInstrument(CSoundFile *pSndFile, UINT nInstr, UINT nIns, UINT nDrumRgn)
//-------------------------------------------------------------------------------------------
{
	BYTE RgnToSmp[DLSMAXREGIONS];
	DLSINSTRUMENT *pins;
	INSTRUMENTHEADER *penv;
	UINT nSample, nRgnMin, nRgnMax, nEnv;
	
	if ((!m_pInstruments) || (nIns >= m_nInstruments) || (!pSndFile)) return FALSE;
	pins = &m_pInstruments[nIns];
	if (pins->ulBank & F_INSTRUMENT_DRUMS)
	{
		if (nDrumRgn >= pins->nRegions) return FALSE;
		nRgnMin = nDrumRgn;
		nRgnMax = nDrumRgn+1;
		nEnv = pins->Regions[nDrumRgn].uPercEnv;
	} else
	{
		if (!pins->nRegions) return FALSE;
		nRgnMin = 0;
		nRgnMax = pins->nRegions;
		nEnv = pins->nMelodicEnv;
	}
#ifdef DLSINSTR_LOG
	Log("DLS Instrument #%d: %s\n", nIns, pins->szName);
	Log("  Bank=0x%04X Instrument=0x%04X\n", pins->ulBank, pins->ulInstrument);
	Log("  %2d regions, nMelodicEnv=%d\n", pins->nRegions, pins->nMelodicEnv);
	for (UINT iDbg=0; iDbg<pins->nRegions; iDbg++)
	{
		DLSREGION *prgn = &pins->Regions[iDbg];
		Log(" Region %d:\n", iDbg);
		Log("  WaveLink = %d (loop [%5d, %5d])\n", prgn->nWaveLink, prgn->ulLoopStart, prgn->ulLoopEnd);
		Log("  Key Range: [%2d, %2d]\n", prgn->uKeyMin, prgn->uKeyMax);
		Log("  fuOptions = 0x%04X\n", prgn->fuOptions);
		Log("  usVolume = %3d, Unity Note = %d\n", prgn->usVolume, prgn->uUnityNote);
	}
#endif
	penv = new INSTRUMENTHEADER;
	if (!penv) return FALSE;
	memset(penv, 0, sizeof(INSTRUMENTHEADER));
	if (pSndFile->Headers[nInstr])
	{
		pSndFile->RemoveInstrumentSamples(nInstr);
		pSndFile->DestroyInstrument(nInstr);
	}
	// Initializes Instrument
	if (pins->ulBank & F_INSTRUMENT_DRUMS)
	{
		CHAR s[64] = "";
		UINT key = pins->Regions[nDrumRgn].uKeyMin;
		if ((key >= 24) && (key <= 84)) lstrcpy(s, szMidiPercussionNames[key-24]);
		if (pins->szName[0])
		{
			wsprintf(&s[strlen(s)], " (%s", pins->szName);
			int n = strlen(s);
			while ((n) && (s[n-1] == ' '))
			{
				n--;
				s[n] = 0;
			}
			lstrcat(s, ")");
		}
		s[31] = 0;
		strcpy(penv->name, s);
	} else
	{
		memcpy(penv->name, pins->szName, 32);
	}
	int nTranspose = 0;
	for (UINT iNoteMap=0; iNoteMap<120; iNoteMap++)
	{
		penv->NoteMap[iNoteMap] = (BYTE)(iNoteMap+1);
		if (pins->ulBank & F_INSTRUMENT_DRUMS)
		{
			if (pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MID))
			{
				if (iNoteMap < pins->Regions[nDrumRgn].uKeyMin) penv->NoteMap[iNoteMap] = (BYTE)(pins->Regions[nDrumRgn].uKeyMin + 1);
				if (iNoteMap > pins->Regions[nDrumRgn].uKeyMax) penv->NoteMap[iNoteMap] = (BYTE)(pins->Regions[nDrumRgn].uKeyMax + 1);
			} else
			if (iNoteMap == pins->Regions[nDrumRgn].uKeyMin)
			{
				nTranspose = pins->Regions[nDrumRgn].uKeyMin - 60;
			}
		}
	}
	penv->nFadeOut = 1024;
	penv->nGlobalVol = 64;	// Maximum
	penv->nPan = 128;		// Center Pan
	penv->nMidiProgram = (BYTE)(pins->ulInstrument & 0x7F);
	penv->nMidiChannel = (BYTE)((pins->ulBank & F_INSTRUMENT_DRUMS) ? 10 : 0);
	penv->wMidiBank = (WORD)(((pins->ulBank & 0x7F00) >> 1) | (pins->ulBank & 0x7F));
	penv->nPPC = 60;		// C-5
	penv->nNNA = NNA_NOTEOFF;
	penv->nDCT = DCT_NOTE;
	penv->nDNA = DNA_NOTEFADE;
	pSndFile->Headers[nInstr] = penv;
	nSample = 1;
	UINT nLoadedSmp = 0;
	// Extract Samples
	for (UINT nRgn=nRgnMin; nRgn<nRgnMax; nRgn++)
	{
		BOOL bDupRgn;
		UINT nSmp;
		DLSREGION *pRgn = &pins->Regions[nRgn];
		// Elimitate Duplicate Regions
		nSmp = 0;
		bDupRgn = FALSE;
		for (UINT iDup=nRgnMin; iDup<nRgn; iDup++)
		{
			DLSREGION *pRgn2 = &pins->Regions[iDup];
			if (((pRgn2->nWaveLink == pRgn->nWaveLink)
			  && (pRgn2->ulLoopEnd == pRgn->ulLoopEnd)
			  && (pRgn2->ulLoopStart == pRgn->ulLoopStart))
			 || ((pRgn2->uKeyMin == pRgn->uKeyMin)
			  && (pRgn2->uKeyMax == pRgn->uKeyMax)))
			{
				bDupRgn = TRUE;
				nSmp = RgnToSmp[iDup];
				break;
			}
		}
		// Create a new sample
		if (pRgn->nWaveLink == 0) nSmp = 0; else
		if (!bDupRgn)
		{
			UINT nmaxsmp = (m_nType & MOD_TYPE_XM) ? 16 : 32;
			if (nLoadedSmp >= nmaxsmp)
			{
				nSmp = RgnToSmp[nRgn-1];
			} else
			{
				while ((nSample < MAX_SAMPLES) && ((pSndFile->Ins[nSample].pSample) || (pSndFile->m_szNames[nSample][0]))) nSample++;
				if (nSample >= MAX_SAMPLES) break;
				if (nSample > pSndFile->m_nSamples) pSndFile->m_nSamples = nSample;
				nSmp = nSample;
				nLoadedSmp++;
			}
		}
		RgnToSmp[nRgn] = (BYTE)nSmp;
		// Map all notes to the right sample
		if (nSmp)
		{
			for (UINT iKey=0; iKey<120; iKey++)
			{
				if ((nRgn == nRgnMin) || ((iKey >= pRgn->uKeyMin) && (iKey <= pRgn->uKeyMax)))
				{
					penv->Keyboard[iKey] = (BYTE)nSmp;
				}
			}
			// Load the sample
			if (!bDupRgn) ExtractSample(pSndFile, nSample, nIns, nRgn, nTranspose);
		}
	}
	// Initializes Envelope
	if ((nEnv) && (nEnv <= m_nEnvelopes))
	{
		DLSENVELOPE *part = &m_Envelopes[nEnv-1];
		UINT nPoint = 0;
		// Volume Envelope
		if ((part->wVolAttack) || (part->wVolDecay < 20*50) || (part->nVolSustainLevel) || (part->wVolRelease < 20*50))
		{
			penv->dwFlags |= ENV_VOLUME;
			// Delay section
			// -> DLS level 2
			// Attack section
			penv->VolPoints[nPoint] = 0;
			if (part->wVolAttack)
			{
				penv->VolEnv[nPoint] = (BYTE)(64/(part->wVolAttack/2+2)+8);//	/-----
				penv->VolPoints[nPoint+1] = part->wVolAttack;				//	|
			} else
			{
				penv->VolEnv[nPoint] = 64;							//	|-----
				penv->VolPoints[nPoint+1] = 1;						//	|
			}
			penv->VolEnv[nPoint+1] = 64;
			nPoint += 2;
			// Hold section
			// -> DLS Level 2
			// Sustain Level
			if (part->nVolSustainLevel > 0)
			{
				if (part->nVolSustainLevel < 128)
				{
					LONG lStartTime = penv->VolPoints[nPoint-1];
					LONG lSusLevel = - DLS32BitRelativeLinearToGain(part->nVolSustainLevel << 9) / 65536;
					LONG lDecayTime = 1;
					if (lSusLevel > 0)
					{
						lDecayTime = (lSusLevel * (LONG)part->wVolDecay) / 960;
						for (UINT i=0; i<7; i++)
						{
							LONG lFactor = 128 - (1 << i);
							if (lFactor <= part->nVolSustainLevel) break;
							LONG lev = - DLS32BitRelativeLinearToGain(lFactor << 9) / 65536;
							if (lev > 0)
							{
								LONG ltime = (lev * (LONG)part->wVolDecay) / 960;
								if ((ltime > 1) && (ltime < lDecayTime))
								{
									ltime += lStartTime;
									if (ltime > penv->VolPoints[nPoint-1])
									{
										penv->VolPoints[nPoint] = (WORD)ltime;
										penv->VolEnv[nPoint] = (BYTE)(lFactor / 2);
										nPoint++;
									}
								}
							}
						}
					}

					if (lStartTime + lDecayTime > (LONG)penv->VolPoints[nPoint-1])
					{
						penv->VolEnv[nPoint] = (BYTE)((part->nVolSustainLevel+1) / 2);
						penv->VolPoints[nPoint] = (WORD)(lStartTime+lDecayTime);
						nPoint++;
					}
				}
				penv->dwFlags |= ENV_VOLSUSTAIN;
			} else
			{
				penv->dwFlags |= ENV_VOLSUSTAIN;
				penv->VolPoints[nPoint] = (WORD)(penv->VolPoints[nPoint-1]+1);
				penv->VolEnv[nPoint] = penv->VolEnv[nPoint-1];
				nPoint++;
			}
			penv->nVolSustainBegin = penv->nVolSustainEnd = (BYTE)(nPoint - 1);
			// Release section
			if ((part->wVolRelease) && (penv->VolEnv[nPoint-1] > 1))
			{
				LONG lReleaseTime = part->wVolRelease;
				LONG lStartTime = penv->VolPoints[nPoint-1];
				LONG lStartFactor = penv->VolEnv[nPoint-1];
				LONG lSusLevel = - DLS32BitRelativeLinearToGain(lStartFactor << 10) / 65536;
				LONG lDecayEndTime = (lReleaseTime * lSusLevel) / 960;
				lReleaseTime -= lDecayEndTime;
				for (UINT i=0; i<5; i++)
				{
					LONG lFactor = 1 + ((lStartFactor * 3) >> (i+2));
					if ((lFactor <= 1) || (lFactor >= lStartFactor)) continue;
					LONG lev = - DLS32BitRelativeLinearToGain(lFactor << 10) / 65536;
					if (lev > 0)
					{
						LONG ltime = (((LONG)part->wVolRelease * lev) / 960) - lDecayEndTime;
						if ((ltime > 1) && (ltime < lReleaseTime))
						{
							ltime += lStartTime;
							if (ltime > penv->VolPoints[nPoint-1])
							{
								penv->VolPoints[nPoint] = (WORD)ltime;
								penv->VolEnv[nPoint] = (BYTE)lFactor;
								nPoint++;
							}
						}
					}
				}
				if (lReleaseTime < 1) lReleaseTime = 1;
				penv->VolPoints[nPoint] = (WORD)(lStartTime + lReleaseTime);
				penv->VolEnv[nPoint] = 0;
				nPoint++;
			} else
			{
				penv->VolPoints[nPoint] = (BYTE)(penv->VolPoints[nPoint-1] + 1);
				penv->VolEnv[nPoint] = 0;
				nPoint++;
			}
			penv->nVolEnv = (BYTE)nPoint;
		}
	}
	if (pins->ulBank & F_INSTRUMENT_DRUMS)
	{
		// Create a default envelope for drums
		penv->dwFlags &= ~ENV_VOLSUSTAIN;
		if (!(penv->dwFlags & ENV_VOLUME))
		{
			penv->dwFlags |= ENV_VOLUME;
			penv->VolPoints[0] = 0;
			penv->VolEnv[0] = 64;
			penv->VolPoints[1] = 5;
			penv->VolEnv[1] = 64;
			penv->VolPoints[2] = 10;
			penv->VolEnv[2] = 32;
			penv->VolPoints[3] = 20;	// 1 second max. for drums
			penv->VolEnv[3] = 0;
			penv->nVolEnv = 4;
		}
	}
	return TRUE;
}



