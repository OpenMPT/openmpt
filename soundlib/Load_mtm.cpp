/*
 * This source code is public domain.
 *
 * Copied to OpenMPT from libmodplug.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 * 
*/

#include "stdafx.h"
#include "sndfile.h"

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

//////////////////////////////////////////////////////////
// MTM file support (import only)

#pragma pack(1)

typedef struct tagMTMSAMPLE
{
    char samplename[22];
	DWORD length;
	DWORD reppos;
	DWORD repend;
	CHAR finetune;
	BYTE volume;
	BYTE attribute;
} MTMSAMPLE;


typedef struct tagMTMHEADER
{
	char id[4];			// MTM file marker + version
	char songname[20];  // ASCIIZ songname
	WORD numtracks;		// number of tracks saved
	BYTE lastpattern;	// last pattern number saved
	BYTE lastorder;		// last order number to play (songlength-1)
	WORD commentsize;	// length of comment field
	BYTE numsamples;	// number of samples saved
	BYTE attribute;		// attribute byte (unused)
	BYTE beatspertrack;
	BYTE numchannels;	// number of channels used
	BYTE panpos[32];	// voice pan positions
} MTMHEADER;


#pragma pack()


bool CSoundFile::ReadMTM(LPCBYTE lpStream, DWORD dwMemLength)
//-----------------------------------------------------------
{
	MTMHEADER *pmh = (MTMHEADER *)lpStream;
	DWORD dwMemPos = 66;

	if ((!lpStream) || (dwMemLength < 0x100)) return false;
	if ((strncmp(pmh->id, "MTM", 3)) || (pmh->numchannels > 32)
	 || (pmh->numsamples >= MAX_SAMPLES) || (!pmh->numsamples)
	 || (!pmh->numtracks) || (!pmh->numchannels)
	 || (!pmh->lastpattern) || (pmh->lastpattern > MAX_PATTERNS)) return false;
	strncpy(m_szNames[0], pmh->songname, 20);
	m_szNames[0][20] = 0;
	if (dwMemPos + 37*pmh->numsamples + 128 + 192*pmh->numtracks
	 + 64 * (pmh->lastpattern+1) + pmh->commentsize >= dwMemLength) return false;
	m_nType = MOD_TYPE_MTM;
	m_nSamples = pmh->numsamples;
	m_nChannels = pmh->numchannels;
	// Reading instruments
	for	(UINT i=1; i<=m_nSamples; i++)
	{
		MTMSAMPLE *pms = (MTMSAMPLE *)(lpStream + dwMemPos);
		strncpy(m_szNames[i], pms->samplename, 22);
		m_szNames[i][22] = 0;
		Samples[i].nVolume = pms->volume << 2;
		Samples[i].nGlobalVol = 64;
		DWORD len = pms->length;
		if ((len > 4) && (len <= MAX_SAMPLE_LENGTH))
		{
			Samples[i].nLength = len;
			Samples[i].nLoopStart = pms->reppos;
			Samples[i].nLoopEnd = pms->repend;
			if (Samples[i].nLoopEnd > Samples[i].nLength) Samples[i].nLoopEnd = Samples[i].nLength;
			if (Samples[i].nLoopStart + 4 >= Samples[i].nLoopEnd) Samples[i].nLoopStart = Samples[i].nLoopEnd = 0;
			if (Samples[i].nLoopEnd) Samples[i].uFlags |= CHN_LOOP;
			Samples[i].nFineTune = MOD2XMFineTune(pms->finetune);
			if (pms->attribute & 0x01)
			{
				Samples[i].uFlags |= CHN_16BIT;
				Samples[i].nLength >>= 1;
				Samples[i].nLoopStart >>= 1;
				Samples[i].nLoopEnd >>= 1;
			}
			Samples[i].nPan = 128;
		}
		dwMemPos += 37;
	}
	// Setting Channel Pan Position
	for (UINT ich=0; ich<m_nChannels; ich++)
	{
		ChnSettings[ich].nPan = ((pmh->panpos[ich] & 0x0F) << 4) + 8;
		ChnSettings[ich].nVolume = 64;
	}
	// Reading pattern order
	Order.ReadAsByte(lpStream+dwMemPos, pmh->lastorder+1, dwMemLength-dwMemPos);
	dwMemPos += 128;
	// Reading Patterns
	LPCBYTE pTracks = lpStream + dwMemPos;
	dwMemPos += 192 * pmh->numtracks;
	LPWORD pSeq = (LPWORD)(lpStream + dwMemPos);
	for (UINT pat=0; pat<=pmh->lastpattern; pat++)
	{
		if(Patterns.Insert(pat, 64)) break;
		for (UINT n=0; n<32; n++) if ((pSeq[n]) && (pSeq[n] <= pmh->numtracks) && (n < m_nChannels))
		{
			LPCBYTE p = pTracks + 192 * (pSeq[n]-1);
			MODCOMMAND *m = Patterns[pat] + n;
			for (UINT i=0; i<64; i++, m+=m_nChannels, p+=3)
			{
				if (p[0] & 0xFC) m->note = (p[0] >> 2) + 37;
				m->instr = ((p[0] & 0x03) << 4) | (p[1] >> 4);
				UINT cmd = p[1] & 0x0F;
				UINT param = p[2];
				if (cmd == 0x0A)
				{
					if (param & 0xF0) param &= 0xF0; else param &= 0x0F;
				}
				m->command = cmd;
				m->param = param;
				if ((cmd) || (param)) ConvertModCommand(m);
			}
		}
		pSeq += 32;
	}
	dwMemPos += 64*(pmh->lastpattern+1);
	if ((pmh->commentsize) && (dwMemPos + pmh->commentsize < dwMemLength))
	{
		UINT n = pmh->commentsize;
		m_lpszSongComments = new char[n+1];
		if (m_lpszSongComments)
		{
			memcpy(m_lpszSongComments, lpStream+dwMemPos, n);
			m_lpszSongComments[n] = 0;
			for (UINT i=0; i<n; i++)
			{
				if (!m_lpszSongComments[i])
				{
					m_lpszSongComments[i] = ((i+1) % 40) ? 0x20 : 0x0D;
				}
			}
		}
	}
	dwMemPos += pmh->commentsize;
	// Reading Samples
	for (UINT ismp=1; ismp<=m_nSamples; ismp++)
	{
		if (dwMemPos >= dwMemLength) break;
		dwMemPos += ReadSample(&Samples[ismp], (Samples[ismp].uFlags & CHN_16BIT) ? RS_PCM16U : RS_PCM8U,
								(LPSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
	}
	m_nMinPeriod = 64;
	m_nMaxPeriod = 32767;
	return true;
}

