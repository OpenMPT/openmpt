/*
 * This source code is public domain.
 *
 * Copied to OpenMPT from libmodplug.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>,
 *          Adam Goode       <adam@evdebs.org> (endian and char fixes for PPC)
 *
*/

//////////////////////////////////////////////
// Oktalyzer (OKT) module loader            //
//////////////////////////////////////////////
#include "stdafx.h"
#include "sndfile.h"

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

typedef struct OKTFILEHEADER
{
	DWORD okta;		// "OKTA"
	DWORD song;		// "SONG"
	DWORD cmod;		// "CMOD"
	DWORD fixed8;
	BYTE chnsetup[8];
	DWORD samp;		// "SAMP"
	DWORD samplen;
} OKTFILEHEADER;


typedef struct OKTSAMPLE
{
	CHAR name[20];
	DWORD length;
	WORD loopstart;
	WORD looplen;
	BYTE pad1;
	BYTE volume;
	BYTE pad2;
	BYTE pad3;
} OKTSAMPLE;


bool CSoundFile::ReadOKT(const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
	OKTFILEHEADER *pfh = (OKTFILEHEADER *)lpStream;
	DWORD dwMemPos = sizeof(OKTFILEHEADER);
	UINT nsamples = 0, npatterns = 0, norders = 0;

	if ((!lpStream) || (dwMemLength < 1024)) return false;
	if ((pfh->okta != 0x41544B4F) || (pfh->song != 0x474E4F53)
	 || (pfh->cmod != 0x444F4D43) || (pfh->chnsetup[0]) || (pfh->chnsetup[2])
	 || (pfh->chnsetup[4]) || (pfh->chnsetup[6]) || (pfh->fixed8 != 0x08000000)
	 || (pfh->samp != 0x504D4153)) return false;
	m_nType = MOD_TYPE_OKT;
	m_nChannels = 4 + pfh->chnsetup[1] + pfh->chnsetup[3] + pfh->chnsetup[5] + pfh->chnsetup[7];
	if (m_nChannels > MAX_CHANNELS) m_nChannels = MAX_CHANNELS;
	nsamples = BigEndian(pfh->samplen) >> 5;
	m_nSamples = nsamples;
	if (m_nSamples >= MAX_SAMPLES) m_nSamples = MAX_SAMPLES-1;
	// Reading samples
	for (UINT smp=1; smp <= nsamples; smp++)
	{
		if (dwMemPos >= dwMemLength) return true;
		if (smp < MAX_SAMPLES)
		{
			OKTSAMPLE *psmp = (OKTSAMPLE *)(lpStream + dwMemPos);
			MODSAMPLE *pSmp = &Samples[smp];

			memcpy(m_szNames[smp], psmp->name, 20);
			pSmp->uFlags = 0;
			pSmp->nLength = BigEndian(psmp->length) & ~1;
			pSmp->nLoopStart = BigEndianW(psmp->loopstart);
			pSmp->nLoopEnd = pSmp->nLoopStart + BigEndianW(psmp->looplen);
			if (pSmp->nLoopStart + 2 < pSmp->nLoopEnd) pSmp->uFlags |= CHN_LOOP;
			pSmp->nGlobalVol = 64;
			pSmp->nVolume = psmp->volume << 2;
			pSmp->nC5Speed = 8363;
		}
		dwMemPos += sizeof(OKTSAMPLE);
	}
	// SPEE
	if (dwMemPos >= dwMemLength) return true;
	if (*((DWORD *)(lpStream + dwMemPos)) == 0x45455053)
	{
		m_nDefaultSpeed = lpStream[dwMemPos+9];
		dwMemPos += BigEndian(*((DWORD *)(lpStream + dwMemPos + 4))) + 8;
	}
	// SLEN
	if (dwMemPos >= dwMemLength) return true;
	if (*((DWORD *)(lpStream + dwMemPos)) == 0x4E454C53)
	{
		npatterns = lpStream[dwMemPos+9];
		dwMemPos += BigEndian(*((DWORD *)(lpStream + dwMemPos + 4))) + 8;
	}
	// PLEN
	if (dwMemPos >= dwMemLength) return true;
	if (*((DWORD *)(lpStream + dwMemPos)) == 0x4E454C50)
	{
		norders = lpStream[dwMemPos+9];
		dwMemPos += BigEndian(*((DWORD *)(lpStream + dwMemPos + 4))) + 8;
	}
	// PATT
	if (dwMemPos >= dwMemLength) return true;
	if (*((DWORD *)(lpStream + dwMemPos)) == 0x54544150)
	{
		UINT orderlen = norders;
		if (orderlen >= MAX_ORDERS) orderlen = MAX_ORDERS-1;
		for (UINT i=0; i<orderlen; i++) Order[i] = lpStream[dwMemPos+10+i];
		for (UINT j=orderlen; j>1; j--) { if (Order[j-1]) break; Order[j-1] = 0xFF; }
		dwMemPos += BigEndian(*((DWORD *)(lpStream + dwMemPos + 4))) + 8;
	}
	// PBOD
	UINT npat = 0;
	while ((dwMemPos < dwMemLength-10) && (*((DWORD *)(lpStream + dwMemPos)) == 0x444F4250))
	{
		DWORD dwPos = dwMemPos + 10;
		UINT rows = lpStream[dwMemPos+9];
		if (!rows) rows = 64;
		if (npat < MAX_PATTERNS)
		{
			if(Patterns.Insert(npat, rows))
				return true;
			MODCOMMAND *m = Patterns[npat];
			UINT imax = m_nChannels*rows;
			for (UINT i=0; i<imax; i++, m++, dwPos+=4)
			{
				if (dwPos+4 > dwMemLength) break;
				const BYTE *p = lpStream+dwPos;
				UINT note = p[0];
				if (note)
				{
					m->note = note + 48;
					m->instr = p[1] + 1;
				}
				UINT command = p[2];
				UINT param = p[3];
				m->param = param;
				switch(command)
				{
				// 0: no effect
				case 0:
					break;
				// 1: Portamento Up
				case 1:
				case 17:
				case 30:
					if (param) m->command = CMD_PORTAMENTOUP;
					break;
				// 2: Portamento Down
				case 2:
				case 13:
				case 21:
					if (param) m->command = CMD_PORTAMENTODOWN;
					break;
				// 10: Arpeggio
				case 10:
				case 11:
				case 12:
					m->command = CMD_ARPEGGIO;
					break;
				// 15: Filter
				case 15:
					m->command = CMD_MODCMDEX;
					m->param = param & 0x0F;
					break;
				// 25: Position Jump
				case 25:
					m->command = CMD_POSITIONJUMP;
					break;
				// 28: Set Speed
				case 28:
					m->command = CMD_SPEED;
					break;
				// 31: Volume Control
				case 31:
					if (param <= 0x40) m->command = CMD_VOLUME; else
					if (param <= 0x50) { m->command = CMD_VOLUMESLIDE; m->param &= 0x0F; if (!m->param) m->param = 0x0F; } else
					if (param <= 0x60) { m->command = CMD_VOLUMESLIDE; m->param = (param & 0x0F) << 4; if (!m->param) m->param = 0xF0; } else
					if (param <= 0x70) { m->command = CMD_MODCMDEX; m->param = 0xB0 | (param & 0x0F); if (!(param & 0x0F)) m->param = 0xBF; } else
					if (param <= 0x80) { m->command = CMD_MODCMDEX; m->param = 0xA0 | (param & 0x0F); if (!(param & 0x0F)) m->param = 0xAF; }
					break;
				}
			}
		}
		npat++;
		dwMemPos += BigEndian(*((DWORD *)(lpStream + dwMemPos + 4))) + 8;
	}
	// SBOD
	UINT nsmp = 1;
	while ((dwMemPos < dwMemLength - 10) && (*((DWORD *)(lpStream + dwMemPos)) == 0x444F4253))
	{
		if (nsmp < MAX_SAMPLES) ReadSample(&Samples[nsmp], RS_PCM8S, (LPSTR)(lpStream+dwMemPos+8), dwMemLength-dwMemPos-8);
		dwMemPos += BigEndian(*((DWORD *)(lpStream + dwMemPos + 4))) + 8;
		nsmp++;
	}
	return true;
}

