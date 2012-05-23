/*
 * Load_far.cpp
 * ------------
 * Purpose: Farandole (FAR) module loader
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

#define FARFILEMAGIC	0xFE524146	// "FAR"

#pragma pack(push, 1)

typedef struct FARHEADER1
{
	DWORD id;				// file magic FAR=
	char songname[32];		// songname
	uint8 reserved[8];
	char eofMagic[3];		// 13,10,26
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

#pragma pack(pop)


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

	if ((!lpStream) || (dwMemLength < 1024) || (pmh1->id != LittleEndian(FARFILEMAGIC))
	 || (pmh1->eofMagic[0] != 13) || (pmh1->eofMagic[1] != 10) || (pmh1->eofMagic[2] != 26))
	{
		return false;
	}

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
	m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;

	StringFixer::ReadString<StringFixer::maybeNullTerminated>(m_szNames[0], pmh1->songname);

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
		ReadFixedLineLengthMessage(lpStream + dwMemPos, szLen, 132, 0);	// 132 characters per line... wow. :)
		dwMemPos += pmh1->stlen;
	}
	// Reading orders
	if (sizeof(FARHEADER2) > dwMemLength - dwMemPos) return true;
	FARHEADER2 farHeader2;
	memcpy(&farHeader2, lpStream + dwMemPos, sizeof(FARHEADER2));
	pmh2 = &farHeader2;
	dwMemPos += sizeof(FARHEADER2);
	if (dwMemPos >= dwMemLength) return true;

	Order.ReadFromArray(pmh2->orders, pmh2->snglen);
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
		ModCommand *m = Patterns[ipat];
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
	for(UINT ismp=0; ismp<64; ismp++) if (samplemap[ismp >> 3] & (1 << (ismp & 7)))
	{
		if (dwMemPos + sizeof(FARSAMPLE) > dwMemLength) return true;
		const FARSAMPLE *pfs = reinterpret_cast<const FARSAMPLE*>(lpStream + dwMemPos);
		dwMemPos += sizeof(FARSAMPLE);
		m_nSamples = ismp + 1;

		StringFixer::ReadString<StringFixer::nullTerminated>(m_szNames[ismp + 1], pfs->samplename);

		ModSample &sample = Samples[ismp + 1];
		sample.Initialize();

		const DWORD length = LittleEndian(pfs->length);
		sample.nLength = length;
		sample.nLoopStart = LittleEndian(pfs->reppos);
		sample.nLoopEnd = LittleEndian(pfs->repend);
		sample.nC5Speed = 8363 * 2;
		sample.nVolume = pfs->volume << 4;

		if((sample.nLength > 3) && (dwMemPos + 4 < dwMemLength))
		{
			SampleIO sampleIO(
				SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::littleEndian,
				SampleIO::signedPCM);

			if(pfs->type & 1)
			{
				sampleIO |= SampleIO::_16bit;
				sample.nLength >>= 1;
				sample.nLoopStart >>= 1;
				sample.nLoopEnd >>= 1;
			}
			if ((pfs->loop & 8) && (sample.nLoopEnd > 4)) sample.uFlags |= CHN_LOOP;

			sampleIO.ReadSample(sample, (LPSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
		}
		dwMemPos += length;
	}
	return true;
}
