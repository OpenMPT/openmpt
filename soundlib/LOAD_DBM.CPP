/*
 * This source code is public domain.
 *
 * Copied to OpenMPT from libmodplug.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>,
 *          Adam Goode       <adam@evdebs.org> (endian and char fixes for PPC)
 *			OpenMPT dev(s)	(miscellaneous modifications)
*/

///////////////////////////////////////////////////////////////
//
// DigiBooster Pro Module Loader (*.dbm)
//
// Note: this loader doesn't handle multiple songs
//
///////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sndfile.h"

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

#define DBM_FILE_MAGIC	0x304d4244
#define DBM_ID_NAME		0x454d414e
#define DBM_NAMELEN		0x2c000000
#define DBM_ID_INFO		0x4f464e49
#define DBM_INFOLEN		0x0a000000
#define DBM_ID_SONG		0x474e4f53
#define DBM_ID_INST		0x54534e49
#define DBM_ID_VENV		0x564e4556
#define DBM_ID_PATT		0x54544150
#define DBM_ID_SMPL		0x4c504d53

#pragma pack(1)

typedef struct DBMFILEHEADER
{
	DWORD dbm_id;		// "DBM0" = 0x304d4244
	WORD trkver;		// Tracker version: 02.15
	WORD reserved;
	DWORD name_id;		// "NAME" = 0x454d414e
	DWORD name_len;		// name length: always 44
	CHAR songname[44];
	DWORD info_id;		// "INFO" = 0x4f464e49
	DWORD info_len;		// 0x0a000000
	WORD instruments;
	WORD samples;
	WORD songs;
	WORD patterns;
	WORD channels;
	DWORD song_id;		// "SONG" = 0x474e4f53
	DWORD song_len;
	CHAR songname2[44];
	WORD orders;
//	WORD orderlist[0];	// orderlist[orders] in words
} DBMFILEHEADER;

typedef struct DBMINSTRUMENT
{
	CHAR name[30];
	WORD sampleno;
	WORD volume;
	DWORD finetune;
	DWORD loopstart;
	DWORD looplen;
	WORD panning;
	WORD flags;
} DBMINSTRUMENT;

typedef struct DBMENVELOPE
{
	WORD instrument;
	BYTE flags;
	BYTE numpoints;
	BYTE sustain1;
	BYTE loopbegin;
	BYTE loopend;
	BYTE sustain2;
	WORD volenv[2*32];
} DBMENVELOPE;

typedef struct DBMPATTERN
{
	WORD rows;
	DWORD packedsize;
	BYTE patterndata[2];	// [packedsize]
} DBMPATTERN;

typedef struct DBMSAMPLE
{
	DWORD flags;
	DWORD samplesize;
	BYTE sampledata[2];		// [samplesize]
} DBMSAMPLE;

#pragma pack()


bool CSoundFile::ReadDBM(const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
	const DBMFILEHEADER *pfh = (DBMFILEHEADER *)lpStream;
	DWORD dwMemPos;
	UINT nOrders, nSamples, nInstruments, nPatterns;
	
	if ((!lpStream) || (dwMemLength <= sizeof(DBMFILEHEADER)) || (!pfh->channels)
	 || (pfh->dbm_id != DBM_FILE_MAGIC) || (!pfh->songs) || (pfh->song_id != DBM_ID_SONG)
	 || (pfh->name_id != DBM_ID_NAME) || (pfh->name_len != DBM_NAMELEN)
	 || (pfh->info_id != DBM_ID_INFO) || (pfh->info_len != DBM_INFOLEN)) return false;
	dwMemPos = sizeof(DBMFILEHEADER);
	nOrders = BigEndianW(pfh->orders);
	if (dwMemPos + 2 * nOrders + 8*3 >= dwMemLength) return false;
	nInstruments = BigEndianW(pfh->instruments);
	nSamples = BigEndianW(pfh->samples);
	nPatterns = BigEndianW(pfh->patterns);
	m_nType = MOD_TYPE_DBM;
	m_nChannels = BigEndianW(pfh->channels);
	if (m_nChannels < 1) m_nChannels = 1;
	if (m_nChannels > 64) m_nChannels = 64;
	memcpy(m_szNames[0], (pfh->songname[0]) ? pfh->songname : pfh->songname2, 32);
	SpaceToNullStringFixed(m_szNames[0], 31);
	Order.resize(nOrders, Order.GetInvalidPatIndex());
	for (UINT iOrd=0; iOrd < nOrders; iOrd++)
	{
		if (iOrd >= MAX_ORDERS) break;
		Order[iOrd] = (PATTERNINDEX)BigEndianW(*((WORD *)(lpStream + dwMemPos + iOrd * 2)));
	}
	dwMemPos += 2*nOrders;
	while (dwMemPos + 10 < dwMemLength)
	{
		DWORD chunk_id = ((LPDWORD)(lpStream+dwMemPos))[0];
		DWORD chunk_size = BigEndian(((LPDWORD)(lpStream+dwMemPos))[1]);
		DWORD chunk_pos;
		
		dwMemPos += 8;
		chunk_pos = dwMemPos;
		if ((dwMemPos + chunk_size > dwMemLength) || (chunk_size > dwMemLength)) break;
		dwMemPos += chunk_size;
		// Instruments
		if (chunk_id == DBM_ID_INST)
		{
			if (nInstruments >= MAX_INSTRUMENTS) nInstruments = MAX_INSTRUMENTS-1;
			for (UINT iIns=0; iIns<nInstruments; iIns++)
			{
				MODSAMPLE *psmp;
				MODINSTRUMENT *pIns;
				DBMINSTRUMENT *pih;
				UINT nsmp;

				if (chunk_pos + sizeof(DBMINSTRUMENT) > dwMemPos) break;
				if ((pIns = new MODINSTRUMENT) == NULL) break;
				pih = (DBMINSTRUMENT *)(lpStream+chunk_pos);
				nsmp = BigEndianW(pih->sampleno);
				psmp = ((nsmp) && (nsmp < MAX_SAMPLES)) ? &Samples[nsmp] : NULL;
				memset(pIns, 0, sizeof(MODINSTRUMENT));
				memcpy(pIns->name, pih->name, 30);
				SpaceToNullStringFixed(pIns->name, 30);
				if (psmp)
				{
					memcpy(m_szNames[nsmp], pih->name, 30);
					SpaceToNullStringFixed(m_szNames[nsmp], 30);
				}
				Instruments[iIns+1] = pIns;
				pIns->nFadeOut = 1024;	// ???
				pIns->nGlobalVol = 64;
				pIns->nPan = BigEndianW(pih->panning);
				if ((pIns->nPan) && (pIns->nPan < 256))
					pIns->dwFlags = ENV_SETPANNING;
				else
					pIns->nPan = 128;
				pIns->nPPC = 5*12;
				SetDefaultInstrumentValues(pIns);
				for (UINT i=0; i<NOTE_MAX; i++)
				{
					pIns->Keyboard[i] = nsmp;
					pIns->NoteMap[i] = i+1;
				}
				// Sample Info
				if (psmp)
				{
					DWORD sflags = BigEndianW(pih->flags);
					psmp->nVolume = BigEndianW(pih->volume) * 4;
					if ((!psmp->nVolume) || (psmp->nVolume > 256)) psmp->nVolume = 256;
					psmp->nGlobalVol = 64;
					psmp->nC5Speed = BigEndian(pih->finetune);
					int f2t = FrequencyToTranspose(psmp->nC5Speed);
					psmp->RelativeTone = f2t >> 7;
					psmp->nFineTune = f2t & 0x7F;
					if ((pih->looplen) && (sflags & 3))
					{
						psmp->nLoopStart = BigEndian(pih->loopstart);
						psmp->nLoopEnd = psmp->nLoopStart + BigEndian(pih->looplen);
						psmp->uFlags |= CHN_LOOP;
						psmp->uFlags &= ~CHN_PINGPONGLOOP;
						if (sflags & 2) psmp->uFlags |= CHN_PINGPONGLOOP;
					}
				}
				chunk_pos += sizeof(DBMINSTRUMENT);
				m_nInstruments = iIns+1;
			}
		} else
		// Volume Envelopes
		if (chunk_id == DBM_ID_VENV)
		{
			UINT nEnvelopes = lpStream[chunk_pos+1];
			
			chunk_pos += 2;
			for (UINT iEnv=0; iEnv<nEnvelopes; iEnv++)
			{
				DBMENVELOPE *peh;
				UINT nins;
				
				if (chunk_pos + sizeof(DBMENVELOPE) > dwMemPos) break;
				peh = (DBMENVELOPE *)(lpStream+chunk_pos);
				nins = BigEndianW(peh->instrument);
				if ((nins) && (nins < MAX_INSTRUMENTS) && (Instruments[nins]) && (peh->numpoints))
				{
					MODINSTRUMENT *pIns = Instruments[nins];

					if (peh->flags & 1) pIns->dwFlags |= ENV_VOLUME;
					if (peh->flags & 2) pIns->dwFlags |= ENV_VOLSUSTAIN;
					if (peh->flags & 4) pIns->dwFlags |= ENV_VOLLOOP;
					pIns->VolEnv.nNodes = peh->numpoints + 1;
					if (pIns->VolEnv.nNodes > MAX_ENVPOINTS) pIns->VolEnv.nNodes = MAX_ENVPOINTS;
					pIns->VolEnv.nLoopStart = peh->loopbegin;
					pIns->VolEnv.nLoopEnd = peh->loopend;
					pIns->VolEnv.nSustainStart = pIns->VolEnv.nSustainEnd = peh->sustain1;
					for (UINT i=0; i<pIns->VolEnv.nNodes; i++)
					{
						pIns->VolEnv.Ticks[i] = BigEndianW(peh->volenv[i*2]);
						pIns->VolEnv.Values[i] = (BYTE)BigEndianW(peh->volenv[i*2+1]);
					}
				}
				chunk_pos += sizeof(DBMENVELOPE);
			}
		} else
		// Packed Pattern Data
		if (chunk_id == DBM_ID_PATT)
		{
			if (nPatterns > MAX_PATTERNS) nPatterns = MAX_PATTERNS;
			for (UINT iPat=0; iPat<nPatterns; iPat++)
			{
				DBMPATTERN *pph;
				DWORD pksize;
				UINT nRows;

				if (chunk_pos + sizeof(DBMPATTERN) > dwMemPos) break;
				pph = (DBMPATTERN *)(lpStream+chunk_pos);
				pksize = BigEndian(pph->packedsize);
				if ((chunk_pos + pksize + 6 > dwMemPos) || (pksize > dwMemPos)) break;
				nRows = BigEndianW(pph->rows);
				if ((nRows >= 4) && (nRows <= 256))
				{
					Patterns.Insert(iPat, nRows);
					MODCOMMAND *m = Patterns[iPat];
					if (m)
					{
						LPBYTE pkdata = (LPBYTE)&pph->patterndata;
						UINT row = 0;
						UINT i = 0;

						while ((i+3<pksize) && (row < nRows))
						{
							UINT ch = pkdata[i++];

							if (ch)
							{
								BYTE b = pkdata[i++];
								ch--;
								if (ch < m_nChannels)
								{
									if (b & 0x01)
									{
										UINT note = pkdata[i++];

										if (note == 0x1F) note = 0xFF; else
										if ((note) && (note < 0xFE))
										{
											note = ((note >> 4)*12) + (note & 0x0F) + 13;
										}
										m[ch].note = note;
									}
									if (b & 0x02) m[ch].instr = pkdata[i++];
									if (b & 0x3C)
									{
										UINT cmd1 = 0xFF, param1 = 0, cmd2 = 0xFF, param2 = 0;
										if (b & 0x04) cmd1 = (UINT)pkdata[i++];
										if (b & 0x08) param1 = pkdata[i++];
										if (b & 0x10) cmd2 = (UINT)pkdata[i++];
										if (b & 0x20) param2 = pkdata[i++];
										if (cmd1 == 0x0C)
										{
											m[ch].volcmd = VOLCMD_VOLUME;
											m[ch].vol = param1;
											cmd1 = 0xFF;
										} else
										if (cmd2 == 0x0C)
										{
											m[ch].volcmd = VOLCMD_VOLUME;
											m[ch].vol = param2;
											cmd2 = 0xFF;
										}
										if ((cmd1 > 0x13) || ((cmd1 >= 0x10) && (cmd2 < 0x10)))
										{
											cmd1 = cmd2;
											param1 = param2;
											cmd2 = 0xFF;
										}
										if (cmd1 <= 0x13)
										{
											m[ch].command = cmd1;
											m[ch].param = param1;
											ConvertModCommand(&m[ch]);
										}
									}
								} else
								{
									if (b & 0x01) i++;
									if (b & 0x02) i++;
									if (b & 0x04) i++;
									if (b & 0x08) i++;
									if (b & 0x10) i++;
									if (b & 0x20) i++;
								}
							} else
							{
								row++;
								m += m_nChannels;
							}
						}
					}
				}
				chunk_pos += 6 + pksize;
			}
		} else
		// Reading Sample Data
		if (chunk_id == DBM_ID_SMPL)
		{
			if (nSamples >= MAX_SAMPLES) nSamples = MAX_SAMPLES-1;
			m_nSamples = nSamples;
			for (UINT iSmp=1; iSmp<=nSamples; iSmp++)
			{
				MODSAMPLE *pSmp;
				DBMSAMPLE *psh;
				DWORD samplesize;
				DWORD sampleflags;

				if (chunk_pos + sizeof(DBMSAMPLE) >= dwMemPos) break;
				psh = (DBMSAMPLE *)(lpStream+chunk_pos);
				chunk_pos += 8;
				samplesize = BigEndian(psh->samplesize);
				sampleflags = BigEndian(psh->flags);
				pSmp = &Samples[iSmp];
				pSmp->nLength = samplesize;
				if (sampleflags & 2)
				{
					pSmp->uFlags |= CHN_16BIT;
					samplesize <<= 1;
				}
				if ((chunk_pos+samplesize > dwMemPos) || (samplesize > dwMemLength)) break;
				if (sampleflags & 3)
				{
					ReadSample(pSmp, (pSmp->uFlags & CHN_16BIT) ? RS_PCM16M : RS_PCM8S,
								(LPSTR)(psh->sampledata), samplesize);
				}
				chunk_pos += samplesize;
			}
		}
	}
	return true;
}

