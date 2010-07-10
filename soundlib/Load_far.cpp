/*
 * This source code is public domain. 
 *
 * Copied to OpenMPT from libmodplug.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *			OpenMPT dev(s)	(miscellaneous modifications)
*/

////////////////////////////////////////
// Farandole (FAR) module loader	  //
////////////////////////////////////////
#include "stdafx.h"
#include "sndfile.h"

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

#define FARFILEMAGIC	0xFE524146	// "FAR"

#pragma pack(1)

typedef struct FARHEADER1
{
	DWORD id;				// file magic FAR=
	CHAR songname[40];		// songname
	CHAR magic2[3];			// 13,10,26
	WORD headerlen;			// remaining length of header in bytes
	BYTE version;			// 0xD1
	BYTE onoff[16];
	BYTE edit1[9];
	BYTE speed;
	BYTE panning[16];
	BYTE edit2[4];
	WORD stlen;
} FARHEADER1;

typedef struct FARHEADER2
{
	BYTE orders[256];
	BYTE numpat;
	BYTE snglen;
	BYTE loopto;
	WORD patsiz[256];
} FARHEADER2;

typedef struct FARSAMPLE
{
	CHAR samplename[32];
	DWORD length;
	BYTE finetune;
	BYTE volume;
	DWORD reppos;
	DWORD repend;
	BYTE type;
	BYTE loop;
} FARSAMPLE;

#pragma pack()


bool CSoundFile::ReadFAR(const BYTE *lpStream, const DWORD dwMemLength)
//---------------------------------------------------------------------
{
	if(dwMemLength < sizeof(FARHEADER1))
		return false;

	FARHEADER1 farHeader;
	memcpy(&farHeader, lpStream, sizeof(FARHEADER1));
	FARHEADER1 *pmh1 = &farHeader;
	FARHEADER2 *pmh2 = 0;
	DWORD dwMemPos = sizeof(FARHEADER1);
	UINT headerlen;
	BYTE samplemap[8];

	if ((!lpStream) || (dwMemLength < 1024) || (LittleEndian(pmh1->id) != FARFILEMAGIC)
	 || (pmh1->magic2[0] != 13) || (pmh1->magic2[1] != 10) || (pmh1->magic2[2] != 26)) return false;
	headerlen = LittleEndianW(pmh1->headerlen);
	pmh1->stlen = LittleEndianW( pmh1->stlen ); /* inplace byteswap -- Toad */
	if ((headerlen >= dwMemLength) || (dwMemPos + pmh1->stlen + sizeof(FARHEADER2) >= dwMemLength)) return false;
	// Globals
	m_nType = MOD_TYPE_FAR;
	m_nChannels = 16;
	m_nInstruments = 0;
	m_nSamples = 0;
	m_nSamplePreAmp = 0x20;
	m_nDefaultSpeed = pmh1->speed;
	m_nDefaultTempo = 80;
	m_nDefaultGlobalVolume = 256;

	memcpy(m_szNames[0], pmh1->songname, 31);
	SpaceToNullStringFixed(m_szNames[0], 31);

	// Channel Setting
	for (UINT nchpan=0; nchpan<16; nchpan++)
	{
		ChnSettings[nchpan].dwFlags = 0;
		ChnSettings[nchpan].nPan = ((pmh1->panning[nchpan] & 0x0F) << 4) + 8;
		ChnSettings[nchpan].nVolume = 64;
	}
	// Reading comment
	if (pmh1->stlen)
	{
		UINT szLen = pmh1->stlen;
		if (szLen > dwMemLength - dwMemPos) szLen = dwMemLength - dwMemPos;
		if ((m_lpszSongComments = new char[szLen + 1]) != NULL)
		{
			memcpy(m_lpszSongComments, lpStream+dwMemPos, szLen);
			m_lpszSongComments[szLen] = 0;
		}
		dwMemPos += pmh1->stlen;
	}
	// Reading orders
	if (sizeof(FARHEADER2) > dwMemLength - dwMemPos) return true;
	FARHEADER2 farHeader2;
	memcpy(&farHeader2, lpStream + dwMemPos, sizeof(FARHEADER2));
	pmh2 = &farHeader2;
	dwMemPos += sizeof(FARHEADER2);
	if (dwMemPos >= dwMemLength) return true;

	Order.ReadAsByte(pmh2->orders, pmh2->snglen, sizeof(pmh2->orders));
	m_nRestartPos = pmh2->loopto;
	// Reading Patterns	
	dwMemPos += headerlen - (869 + pmh1->stlen);
	if (dwMemPos >= dwMemLength) return true;

	// byteswap pattern data.
	for(uint16 psfix = 0; psfix < 256; psfix++)
	{
		pmh2->patsiz[psfix] = LittleEndianW( pmh2->patsiz[psfix] ) ;
	}
	// end byteswap of pattern data

	WORD *patsiz = (WORD *)pmh2->patsiz;
	for (UINT ipat=0; ipat<256; ipat++) if (patsiz[ipat])
	{
		UINT patlen = patsiz[ipat];
		if ((ipat >= MAX_PATTERNS) || (patsiz[ipat] < 2))
		{
			dwMemPos += patlen;
			continue;
		}
		if (dwMemPos + patlen >= dwMemLength) return true;
		UINT rows = (patlen - 2) >> 6;
		if (!rows)
		{
			dwMemPos += patlen;
			continue;
		}
		if (rows > 256) rows = 256;
		if (rows < 16) rows = 16;
		if(Patterns.Insert(ipat, rows)) return true;
		MODCOMMAND *m = Patterns[ipat];
		UINT patbrk = lpStream[dwMemPos];
		const BYTE *p = lpStream + dwMemPos + 2;
		UINT max = rows*16*4;
		if (max > patlen-2) max = patlen-2;
		for (UINT len=0; len<max; len += 4, m++)
		{
			BYTE note = p[len];
			BYTE ins = p[len+1];
			BYTE vol = p[len+2];
			BYTE eff = p[len+3];
			if (note)
			{
				m->instr = ins + 1;
				m->note = note + 36;
			}
			if (vol & 0x0F)
			{
				m->volcmd = VOLCMD_VOLUME;
				m->vol = (vol & 0x0F) << 2;
				if (m->vol <= 4) m->vol = 0;
			}
			switch(eff & 0xF0)
			{
			// 1.x: Portamento Up
			case 0x10:
				m->command = CMD_PORTAMENTOUP;
				m->param = eff & 0x0F;
				break;
			// 2.x: Portamento Down
			case 0x20:
				m->command = CMD_PORTAMENTODOWN;
				m->param = eff & 0x0F;
				break;
			// 3.x: Tone-Portamento
			case 0x30:
				m->command = CMD_TONEPORTAMENTO;
				m->param = (eff & 0x0F) << 2;
				break;
			// 4.x: Retrigger
			case 0x40:
				m->command = CMD_RETRIG;
				m->param = 6 / (1+(eff&0x0F)) + 1;
				break;
			// 5.x: Set Vibrato Depth
			case 0x50:
				m->command = CMD_VIBRATO;
				m->param = (eff & 0x0F);
				break;
			// 6.x: Set Vibrato Speed
			case 0x60:
				m->command = CMD_VIBRATO;
				m->param = (eff & 0x0F) << 4;
				break;
			// 7.x: Vol Slide Up
			case 0x70:
				m->command = CMD_VOLUMESLIDE;
				m->param = (eff & 0x0F) << 4;
				break;
			// 8.x: Vol Slide Down
			case 0x80:
				m->command = CMD_VOLUMESLIDE;
				m->param = (eff & 0x0F);
				break;
			// A.x: Port to vol
			case 0xA0:
				m->volcmd = VOLCMD_VOLUME;
				m->vol = ((eff & 0x0F) << 2) + 4;
				break;
			// B.x: Set Balance
			case 0xB0:
				m->command = CMD_PANNING8;
				m->param = (eff & 0x0F) << 4;
				break;
			// F.x: Set Speed
			case 0xF0:
				m->command = CMD_SPEED;
				m->param = eff & 0x0F;
				break;
			default:
				if ((patbrk) &&	(patbrk+1 == (len >> 6)) && (patbrk+1 != rows-1))
				{
					m->command = CMD_PATTERNBREAK;
					patbrk = 0;
				}
			}
		}
		dwMemPos += patlen;
	}
	// Reading samples
	if (dwMemPos + 8 >= dwMemLength) return true;
	memcpy(samplemap, lpStream+dwMemPos, 8);
	dwMemPos += 8;
	MODSAMPLE *pSmp = &Samples[1];
	for (UINT ismp=0; ismp<64; ismp++, pSmp++) if (samplemap[ismp >> 3] & (1 << (ismp & 7)))
	{
		if (dwMemPos + sizeof(FARSAMPLE) > dwMemLength) return true;
		const FARSAMPLE *pfs = reinterpret_cast<const FARSAMPLE*>(lpStream + dwMemPos);
		dwMemPos += sizeof(FARSAMPLE);
		m_nSamples = ismp + 1;
		memcpy(m_szNames[ismp+1], pfs->samplename, 31);
		SpaceToNullStringFixed(m_szNames[ismp + 1], 31);
		const DWORD length = LittleEndian( pfs->length );
		pSmp->nLength = length;
		pSmp->nLoopStart = LittleEndian(pfs->reppos) ;
		pSmp->nLoopEnd = LittleEndian(pfs->repend) ;
		pSmp->nFineTune = 0;
		pSmp->nC5Speed = 8363*2;
		pSmp->nGlobalVol = 64;
		pSmp->nVolume = pfs->volume << 4;
		pSmp->uFlags = 0;
		if ((pSmp->nLength > 3) && (dwMemPos + 4 < dwMemLength))
		{
			if (pfs->type & 1)
			{
				pSmp->uFlags |= CHN_16BIT;
				pSmp->nLength >>= 1;
				pSmp->nLoopStart >>= 1;
				pSmp->nLoopEnd >>= 1;
			}
			if ((pfs->loop & 8) && (pSmp->nLoopEnd > 4)) pSmp->uFlags |= CHN_LOOP;
			ReadSample(pSmp, (pSmp->uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S,
						(LPSTR)(lpStream+dwMemPos), dwMemLength - dwMemPos);
		}
		dwMemPos += length;
	}
	return true;
}

