/*
 * This program is  free software; you can redistribute it  and modify it
 * under the terms of the GNU  General Public License as published by the
 * Free Software Foundation; either version 2  of the license or (at your
 * option) any later version.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

///////////////////////////////////////////////////////
// DMF DELUSION DIGITAL MUSIC FILEFORMAT (X-Tracker) //
///////////////////////////////////////////////////////
#include "stdafx.h"
#include "sndfile.h"

//#define DMFLOG

//#pragma warning(disable:4244)

#pragma pack(1)

typedef struct DMFHEADER
{
	DWORD id;				// "DDMF" = 0x464d4444
	BYTE version;			// 4
	CHAR trackername[8];	// "XTRACKER"
	CHAR songname[30];
	CHAR composer[20];
	BYTE date[3];
} DMFHEADER;

typedef struct DMFINFO
{
	DWORD id;			// "INFO"
	DWORD infosize;
} DMFINFO;

typedef struct DMFSEQU
{
	DWORD id;			// "SEQU"
	DWORD seqsize;
	WORD loopstart;
	WORD loopend;
	WORD sequ[2];
} DMFSEQU;

typedef struct DMFPATT
{
	DWORD id;			// "PATT"
	DWORD patsize;
	WORD numpat;		// 1-1024
	BYTE tracks;
	BYTE firstpatinfo;
} DMFPATT;

typedef struct DMFTRACK
{
	BYTE tracks;
	BYTE beat;		// [hi|lo] -> hi=ticks per beat, lo=beats per measure
	WORD ticks;		// max 512
	DWORD jmpsize;
} DMFTRACK;

typedef struct DMFSMPI
{
	DWORD id;
	DWORD size;
	BYTE samples;
} DMFSMPI;

typedef struct DMFSAMPLE
{
	DWORD len;
	DWORD loopstart;
	DWORD loopend;
	WORD c3speed;
	BYTE volume;
	BYTE flags;
} DMFSAMPLE;

#pragma pack()


//void Log(LPCSTR s, DWORD d1=0, DWORD d2=0, DWORD d3=0);


BOOL CSoundFile::ReadDMF(const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
	DMFHEADER *pfh = (DMFHEADER *)lpStream;
	DMFINFO *psi;
	DMFSEQU *sequ;
	DWORD dwMemPos;
	BYTE infobyte[32];
	BYTE smplflags[MAX_SAMPLES];

	if ((!lpStream) || (dwMemLength < 1024)) return FALSE;
	if ((pfh->id != 0x464d4444) || (!pfh->version) || (pfh->version & 0xF0)) return FALSE;
	dwMemPos = 66;
	memcpy(m_szNames[0], pfh->songname, 30);
	m_szNames[0][30] = 0;
	m_nType = MOD_TYPE_DMF;
	m_nChannels = 0;
#ifdef DMFLOG
	Log("DMF version %d: \"%s\": %d bytes (0x%04X)\n", pfh->version, m_szNames[0], dwMemLength, dwMemLength);
#endif
	while (dwMemPos + 7 < dwMemLength)
	{
		DWORD id = *((LPDWORD)(lpStream+dwMemPos));

		switch(id)
		{
		// "INFO"
		case 0x4f464e49:
		// "CMSG"
		case 0x47534d43:
			psi = (DMFINFO *)(lpStream+dwMemPos);
			if (id == 0x47534d43) dwMemPos++;
			if ((psi->infosize > dwMemLength) || (psi->infosize + dwMemPos + 8 > dwMemLength)) goto dmfexit;
			if ((psi->infosize >= 8) && (!m_lpszSongComments))
			{
				m_lpszSongComments = new CHAR[psi->infosize];
				if (m_lpszSongComments)
				{
					for (UINT i=0; i<psi->infosize-1; i++)
					{
						CHAR c = lpStream[dwMemPos+8+i];
						if ((i % 40) == 39)
							m_lpszSongComments[i] = 0x0d;
						else
							m_lpszSongComments[i] = (c < ' ') ? ' ' : c;
					}
					m_lpszSongComments[psi->infosize-1] = 0;
				}
			}
			dwMemPos += psi->infosize + 8 - 1;
			break;

		// "SEQU"
		case 0x55514553:
			sequ = (DMFSEQU *)(lpStream+dwMemPos);
			if ((sequ->seqsize >= dwMemLength) || (dwMemPos + sequ->seqsize + 12 > dwMemLength)) goto dmfexit;
			{
				UINT nseq = sequ->seqsize >> 1;
				if (nseq >= MAX_ORDERS-1) nseq = MAX_ORDERS-1;
				if (sequ->loopstart < nseq) m_nRestartPos = sequ->loopstart;
				for (UINT i=0; i<nseq; i++) Order[i] = (BYTE)sequ->sequ[i];
			}
			dwMemPos += sequ->seqsize + 8;
			break;

		// "PATT"
		case 0x54544150:
			if (!m_nChannels)
			{
				DMFPATT *patt = (DMFPATT *)(lpStream+dwMemPos);
				UINT numpat;
				DWORD dwPos = dwMemPos + 11;
				if ((patt->patsize >= dwMemLength) || (dwMemPos + patt->patsize + 8 > dwMemLength)) goto dmfexit;
				numpat = patt->numpat;
				if (numpat > MAX_PATTERNS) numpat = MAX_PATTERNS;
				m_nChannels = patt->tracks;
				if (m_nChannels < patt->firstpatinfo) m_nChannels = patt->firstpatinfo;
				if (m_nChannels > 32) m_nChannels = 32;
				if (m_nChannels < 4) m_nChannels = 4;
				for (UINT npat=0; npat<numpat; npat++)
				{
					DMFTRACK *pt = (DMFTRACK *)(lpStream+dwPos);
				#ifdef DMFLOG
					Log("Pattern #%d: %d tracks, %d rows\n", npat, pt->tracks, pt->ticks);
				#endif
					UINT tracks = pt->tracks;
					if (tracks > 32) tracks = 32;
					UINT ticks = pt->ticks;
					if (ticks > 256) ticks = 256;
					if (ticks < 16) ticks = 16;
					dwPos += 8;
					if ((pt->jmpsize >= dwMemLength) || (dwPos + pt->jmpsize + 4 >= dwMemLength)) break;
					PatternSize[npat] = (WORD)ticks;
					MODCOMMAND *m = AllocatePattern(PatternSize[npat], m_nChannels);
					if (!m) goto dmfexit;
					Patterns[npat] = m;
					DWORD d = dwPos;
					dwPos += pt->jmpsize;
					memset(infobyte, 0, sizeof(infobyte));
					for (UINT row=0; row<ticks; row++)
					{
						MODCOMMAND *p = &m[row*m_nChannels];
						//d += 2;
						// Parse track global effects
						{
							BYTE b = lpStream[d++];
							if (b & 0x80) // track counter ?
							{
								UINT trkcnt = lpStream[d++];
							#ifdef DMFLOG
								//Log("trkcnt=0x%02X\n", trkcnt);
							#endif
							}
							if (b & 0x7f)
							{
								UINT tikcnt = lpStream[d++];
							#ifdef DMFLOG
								//Log("b=0x%02X tikcnt=0x%02X\n", b, tikcnt);
							#endif
							}
						}
						for (UINT i=0; i<tracks; i++)
						{
							MODCOMMAND cmd = {0,0,0,0,0,0};

							BYTE b = lpStream[d++];
							if (b & 0x80)
							{
								d++; // Tick counter ?
							}
							// instrument
							if (b & 0x40)
							{
								cmd.instr = lpStream[d++];
							}
							// note
							if (b & 0x20)
							{
								cmd.note = lpStream[d++];
								if ((cmd.note > 128) && (cmd.note < 255)) cmd.note -= 128;
								if ((cmd.note) && (cmd.note < 128)) cmd.note += 24;
							}
							// volume
							if (b & 0x10)
							{
								cmd.volcmd = VOLCMD_VOLUME;
								cmd.vol = (BYTE)((lpStream[d++]+1) >> 2);
							}
							// instrument effect
							if (b & 0x08)
							{
								d += 2;
							}
							// note effect
							if (b & 0x04)
							{
								d += 2;
							}
							// volume effect
							if (b & 0x02)
							{
								d += 2;
							}
							if (i < m_nChannels) p[i] = cmd;
							if (d >= dwPos)
							{
							#ifdef DMFLOG
								Log("Unexpected EOP: row=%d", row);
							#endif
								break;
							}
						}
						if (d >= dwPos) break;
					}
				#ifdef DMFLOG
					Log(" %d/%d bytes remaining\n", dwPos-d, pt->jmpsize);
				#endif
					if (dwPos + 8 >= dwMemLength) break;
				}
				dwMemPos += patt->patsize + 8;
			}
			break;

		// "SMPI": Sample Info
		case 0x49504d53:
			{
				DMFSMPI *pds = (DMFSMPI *)(lpStream+dwMemPos);
				if (pds->size <= dwMemLength - dwMemPos)
				{
					DWORD dwPos = dwMemPos + 9;
					m_nSamples = pds->samples;
					if (m_nSamples >= MAX_SAMPLES) m_nSamples = MAX_SAMPLES-1;
					for (UINT iSmp=1; iSmp<=m_nSamples; iSmp++)
					{
						UINT namelen = lpStream[dwPos];
						smplflags[iSmp] = 0;
						if (dwPos+namelen+1+sizeof(DMFSAMPLE) > dwMemPos+pds->size+8) break;
						if (namelen)
						{
							UINT rlen = (namelen < 32) ? namelen : 31;
							memcpy(m_szNames[iSmp], lpStream+dwPos+1, rlen);
							m_szNames[iSmp][rlen] = 0;
						}
						dwPos += namelen + 1;
						DMFSAMPLE *psh = (DMFSAMPLE *)(lpStream+dwPos);
						MODINSTRUMENT *psmp = &Ins[iSmp];
						psmp->nLength = psh->len;
						psmp->nLoopStart = psh->loopstart;
						psmp->nLoopEnd = psh->loopend;
						psmp->nC4Speed = psh->c3speed;
						psmp->nGlobalVol = 64;
						psmp->nVolume = (psh->volume) ? ((WORD)psh->volume)+1 : (WORD)256;
						psmp->uFlags = (psh->flags & 2) ? CHN_16BIT : 0;
						if (psmp->uFlags & CHN_16BIT) psmp->nLength >>= 1;
						if (psh->flags & 1) psmp->uFlags |= CHN_LOOP;
						smplflags[iSmp] = psh->flags;
						dwPos += (pfh->version < 8) ? 22 : 30;
					#ifdef DMFLOG
						Log("SMPI %d/%d: len=%d flags=%d\n", iSmp, m_nSamples, psmp->nLength, psh->flags);
					#endif
					}
				}
				dwMemPos += pds->size + 8;
			}
			break;

		// "SMPD": Sample Data
		case 0x44504d53:
			{
				DWORD dwPos = dwMemPos + 8;
				UINT ismpd = 0;
				for (UINT iSmp=1; iSmp<=m_nSamples; iSmp++)
				{
					ismpd++;
					DWORD pksize;
					if (dwPos + 4 >= dwMemLength)
					{
					#ifdef DMFLOG
						Log("Unexpected EOF at sample %d/%d! (pos=%d)\n", iSmp, m_nSamples, dwPos);
					#endif
						break;
					}
					pksize = *((LPDWORD)(lpStream+dwPos));
				#ifdef DMFLOG
					Log("sample %d: pos=0x%X pksize=%d ", iSmp, dwPos, pksize);
					Log("len=%d flags=0x%X [%08X]\n", Ins[iSmp].nLength, smplflags[ismpd], *((LPDWORD)(lpStream+dwPos+4)));
				#endif
					dwPos += 4;
					if (pksize > dwMemLength - dwPos)
					{
					#ifdef DMFLOG
						Log("WARNING: pksize=%d, but only %d bytes left\n", pksize, dwMemLength-dwPos);
					#endif
						pksize = dwMemLength - dwPos;
					}
					if ((pksize) && (iSmp <= m_nSamples))
					{
						UINT flags = (Ins[iSmp].uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S;
						if (smplflags[ismpd] & 4) flags = (Ins[iSmp].uFlags & CHN_16BIT) ? RS_DMF16 : RS_DMF8;
						ReadSample(&Ins[iSmp], flags, (LPSTR)(lpStream+dwPos), pksize);
					}
					dwPos += pksize;
				}
				dwMemPos = dwPos;
			}
			break;

		// "ENDE": end of file
		case 0x45444e45:
			goto dmfexit;
		
		// Unrecognized id, or "ENDE" field
		default:
			dwMemPos += 4;
			break;
		}
	}
dmfexit:
	if (!m_nChannels)
	{
		if (!m_nSamples)
		{
			m_nType = MOD_TYPE_NONE;
			return FALSE;
		}
		m_nChannels = 4;
	}
	return TRUE;
}


///////////////////////////////////////////////////////////////////////
// DMF Compression

#pragma pack(1)

typedef struct DMF_HNODE
{
	short int left, right;
	BYTE value;
} DMF_HNODE;

typedef struct DMF_HTREE
{
	LPBYTE ibuf, ibufmax;
	DWORD bitbuf;
	UINT bitnum;
	UINT lastnode, nodecount;
	DMF_HNODE nodes[256];
} DMF_HTREE;

#pragma pack()


// DMF Huffman ReadBits
BYTE DMFReadBits(DMF_HTREE *tree, UINT nbits)
//-------------------------------------------
{
	BYTE x = 0, bitv = 1;
	while (nbits--)
	{
		if (tree->bitnum)
		{
			tree->bitnum--;
		} else
		{
			tree->bitbuf = (tree->ibuf < tree->ibufmax) ? *(tree->ibuf++) : 0;
			tree->bitnum = 7;
		}
		if (tree->bitbuf & 1) x |= bitv;
		bitv <<= 1;
		tree->bitbuf >>= 1;
	}
	return x;
}

//
// tree: [8-bit value][12-bit index][12-bit index] = 32-bit
//

void DMFNewNode(DMF_HTREE *tree)
//------------------------------
{
	BYTE isleft, isright;
	UINT actnode;

	actnode = tree->nodecount;
	if (actnode > 255) return;
	tree->nodes[actnode].value = DMFReadBits(tree, 7);
	isleft = DMFReadBits(tree, 1);
	isright = DMFReadBits(tree, 1);
	actnode = tree->lastnode;
	if (actnode > 255) return;
	tree->nodecount++;
	tree->lastnode = tree->nodecount;
	if (isleft)
	{
		tree->nodes[actnode].left = tree->lastnode;
		DMFNewNode(tree);
	} else
	{
		tree->nodes[actnode].left = -1;
	}
	tree->lastnode = tree->nodecount;
	if (isright)
	{
		tree->nodes[actnode].right = tree->lastnode;
		DMFNewNode(tree);
	} else
	{
		tree->nodes[actnode].right = -1;
	}
}


int DMFUnpack(LPBYTE psample, LPBYTE ibuf, LPBYTE ibufmax, UINT maxlen)
//----------------------------------------------------------------------
{
	DMF_HTREE tree;
	UINT actnode;
	BYTE value, sign, delta = 0;
	
	memset(&tree, 0, sizeof(tree));
	tree.ibuf = ibuf;
	tree.ibufmax = ibufmax;
	DMFNewNode(&tree);
	value = 0;
	for (UINT i=0; i<maxlen; i++)
	{
		actnode = 0;
		sign = DMFReadBits(&tree, 1);
		do
		{
			if (DMFReadBits(&tree, 1))
				actnode = tree.nodes[actnode].right;
			else
				actnode = tree.nodes[actnode].left;
			if (actnode > 255) break;
			delta = tree.nodes[actnode].value;
			if ((tree.ibuf >= tree.ibufmax) && (!tree.bitnum)) break;
		} while ((tree.nodes[actnode].left >= 0) && (tree.nodes[actnode].right >= 0));
		if (sign) delta ^= 0xFF;
		value += delta;
		psample[i] = (i) ? value : 0;
	}
#ifdef DMFLOG
//	Log("DMFUnpack: %d remaining bytes\n", tree.ibufmax-tree.ibuf);
#endif
	return tree.ibuf - ibuf;
}




