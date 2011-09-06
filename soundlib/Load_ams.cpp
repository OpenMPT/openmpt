/*
 * This source code is public domain.
 *
 * Copied to OpenMPT from libmodplug.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *			OpenMPT dev(s)	(miscellaneous modifications)
 * Notes  : Extreme was renamed to Velvet Development at some point,
 *          and thus they also renamed their tracker from
 *          "Extreme's Tracker" to "Velvet Studio".
 *          While the two programs look rather similiar, the structure of both
 *          programs' "AMS" format is significantly different - Velvet Studio is a
 *          rather advanced tracker in comparison to Extreme's Tracker.
*/

//////////////////////////////////////////////
// AMS (Extreme's Tracker) module loader    //
//////////////////////////////////////////////
#include "stdafx.h"
#include "Loaders.h"

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

#pragma pack(1)

typedef struct AMSFILEHEADER
{
	char szHeader[7];	// "Extreme"
	BYTE verlo, verhi;	// 0x??,0x01
	BYTE chncfg;
	BYTE samples;
	WORD patterns;
	WORD orders;
	BYTE vmidi;
	WORD extra;
} AMSFILEHEADER;

typedef struct AMSSAMPLEHEADER
{
	DWORD length;
	DWORD loopstart;
	DWORD loopend;
	BYTE finetune_and_pan;
	WORD samplerate;	// C-2 = 8363
	BYTE volume;		// 0-127
	BYTE infobyte;
} AMSSAMPLEHEADER;


#pragma pack()


// Callback function for reading text
void Convert_AMS_Text_Chars(char &c)
//----------------------------------
{
	switch((unsigned char)c)
	{
	case 0x00:
	case 0x81: c = ' '; break;
	case 0x14: c = 'ö'; break;
	case 0x19: c = 'Ö'; break;
	case 0x04: c = 'ä'; break;
	case 0x0E: c = 'Ä'; break;
	case 0x06: c = 'å'; break;
	case 0x0F: c = 'Å'; break;
	default:
		if((unsigned char)c > 0x81)
			c = '\r';
		break;
	}
}


bool CSoundFile::ReadAMS(const LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------------------
{
	BYTE pkinf[MAX_SAMPLES];
	AMSFILEHEADER *pfh = (AMSFILEHEADER *)lpStream;
	DWORD dwMemPos;
	UINT tmp, tmp2;
	
	if ((!lpStream) || (dwMemLength < 126)) return false;
	if ((pfh->verhi != 0x01) || (strncmp(pfh->szHeader, "Extreme", 7))
	 || (!pfh->patterns) || (!pfh->orders) || (!pfh->samples) || (pfh->samples >= MAX_SAMPLES)
	 || (pfh->patterns > MAX_PATTERNS) || (pfh->orders > MAX_ORDERS))
	{
		return ReadAMS2(lpStream, dwMemLength);
	}
	dwMemPos = sizeof(AMSFILEHEADER) + pfh->extra;
	if (dwMemPos + pfh->samples * sizeof(AMSSAMPLEHEADER) >= dwMemLength) return false;
	m_nType = MOD_TYPE_AMS;
	m_nInstruments = 0;
	m_nChannels = (pfh->chncfg & 0x1F) + 1;
	m_nSamples = pfh->samples;
	for (UINT nSmp=1; nSmp <= m_nSamples; nSmp++, dwMemPos += sizeof(AMSSAMPLEHEADER))
	{
		AMSSAMPLEHEADER *psh = (AMSSAMPLEHEADER *)(lpStream + dwMemPos);
		MODSAMPLE *pSmp = &Samples[nSmp];
		pSmp->nLength = psh->length;
		pSmp->nLoopStart = psh->loopstart;
		pSmp->nLoopEnd = psh->loopend;
		pSmp->nGlobalVol = 64;
		pSmp->nVolume = psh->volume << 1;
		pSmp->nC5Speed = psh->samplerate;
		pSmp->nPan = (psh->finetune_and_pan & 0xF0);
		if (pSmp->nPan < 0x80) pSmp->nPan += 0x10;
		pSmp->nFineTune = MOD2XMFineTune(psh->finetune_and_pan & 0x0F);
		pSmp->uFlags = (psh->infobyte & 0x80) ? CHN_16BIT : 0;
		if ((pSmp->nLoopEnd <= pSmp->nLength) && (pSmp->nLoopStart+4 <= pSmp->nLoopEnd)) pSmp->uFlags |= CHN_LOOP;
		pkinf[nSmp] = psh->infobyte;
	}

	// Read Song Name
	if (dwMemPos + 1 >= dwMemLength) return true;
	tmp = lpStream[dwMemPos++];
	if (dwMemPos + tmp + 1 >= dwMemLength) return true;
	tmp2 = (tmp < 32) ? tmp : 31;
	if (tmp2) memcpy(m_szNames[0], lpStream + dwMemPos, tmp2);
	SpaceToNullStringFixed(m_szNames[0], tmp2);
	m_szNames[0][tmp2] = 0;
	dwMemPos += tmp;

	// Read sample names
	for (UINT sNam=1; sNam<=m_nSamples; sNam++)
	{
		if (dwMemPos + 32 >= dwMemLength) return true;
		tmp = lpStream[dwMemPos++];
		tmp2 = (tmp < 32) ? tmp : 31;
		if (tmp2) memcpy(m_szNames[sNam], lpStream+dwMemPos, tmp2);
		SpaceToNullStringFixed(m_szNames[sNam], tmp2);
		dwMemPos += tmp;
	}

	// Read Channel names
	for (UINT cNam=0; cNam<m_nChannels; cNam++)
	{
		if (dwMemPos + 32 >= dwMemLength) return true;
		BYTE chnnamlen = lpStream[dwMemPos++];
		if ((chnnamlen) && (chnnamlen < MAX_CHANNELNAME))
		{
			memcpy(ChnSettings[cNam].szName, lpStream + dwMemPos, chnnamlen);
			SpaceToNullStringFixed(ChnSettings[cNam].szName, chnnamlen);
		}
		dwMemPos += chnnamlen;
	}

	// Read Pattern Names
	for (UINT pNam = 0; pNam < pfh->patterns; pNam++)
	{
		if (dwMemPos + 1 >= dwMemLength) return true;
		tmp = lpStream[dwMemPos++];
		tmp2 = min(tmp, MAX_PATTERNNAME - 1);		// not counting null char
		if (dwMemPos + tmp >= dwMemLength) return true;
		Patterns.Insert(pNam, 64);	// Create pattern now, so that the name won't be overwritten later.
		if(tmp2)
		{
			Patterns[pNam].SetName((char *)(lpStream + dwMemPos), tmp2 + 1);
		}
		dwMemPos += tmp;
	}
	
	// Read Song Comments
	tmp = *((WORD *)(lpStream+dwMemPos));
	dwMemPos += 2;
	if (dwMemPos + tmp >= dwMemLength) return true;
	if (tmp)
	{
		ReadMessage(lpStream + dwMemPos, tmp, leCR, &Convert_AMS_Text_Chars);
	}
	dwMemPos += tmp;

	// Read Order List
	Order.resize(pfh->orders, Order.GetInvalidPatIndex());
	for (UINT iOrd=0; iOrd < pfh->orders; iOrd++, dwMemPos += 2)
	{
		Order[iOrd] = (PATTERNINDEX)*((WORD *)(lpStream + dwMemPos));
	}

	// Read Patterns
	for (UINT iPat=0; iPat<pfh->patterns; iPat++)
	{
		if (dwMemPos + 4 >= dwMemLength) return true;
		UINT len = *((DWORD *)(lpStream + dwMemPos));
		dwMemPos += 4;
		if ((len >= dwMemLength) || (dwMemPos + len > dwMemLength)) return true;
		// Pattern has been inserted when reading pattern names
		MODCOMMAND* m = Patterns[iPat];
		if (!m) return true;
		const BYTE *p = lpStream + dwMemPos;
		UINT row = 0, i = 0;
		while ((row < Patterns[iPat].GetNumRows()) && (i+2 < len))
		{
			BYTE b0 = p[i++];
			BYTE b1 = p[i++];
			BYTE b2 = 0;
			UINT ch = b0 & 0x3F;
			// Note+Instr
			if (!(b0 & 0x40))
			{
				b2 = p[i++];
				if (ch < m_nChannels)
				{
					if (b1 & 0x7F) m[ch].note = (b1 & 0x7F) + 25;
					m[ch].instr = b2;
				}
				if (b1 & 0x80)
				{
					b0 |= 0x40;
					b1 = p[i++];
				}
			}
			// Effect
			if (b0 & 0x40)
			{
			anothercommand:
				if (b1 & 0x40)
				{
					if (ch < m_nChannels)
					{
						m[ch].volcmd = VOLCMD_VOLUME;
						m[ch].vol = b1 & 0x3F;
					}
				} else
				{
					b2 = p[i++];
					if (ch < m_nChannels)
					{
						UINT cmd = b1 & 0x3F;
						if (cmd == 0x0C)
						{
							m[ch].volcmd = VOLCMD_VOLUME;
							m[ch].vol = b2 >> 1;
						} else
						if (cmd == 0x0E)
						{
							if (!m[ch].command)
							{
								UINT command = CMD_S3MCMDEX;
								UINT param = b2;
								switch(param & 0xF0)
								{
								case 0x00:	if (param & 0x08) { param &= 0x07; param |= 0x90; } else {command=param=0;} break;
								case 0x10:	command = CMD_PORTAMENTOUP; param |= 0xF0; break;
								case 0x20:	command = CMD_PORTAMENTODOWN; param |= 0xF0; break;
								case 0x30:	param = (param & 0x0F) | 0x10; break;
								case 0x40:	param = (param & 0x0F) | 0x30; break;
								case 0x50:	param = (param & 0x0F) | 0x20; break;
								case 0x60:	param = (param & 0x0F) | 0xB0; break;
								case 0x70:	param = (param & 0x0F) | 0x40; break;
								case 0x90:	command = CMD_RETRIG; param &= 0x0F; break;
								case 0xA0:	if (param & 0x0F) { command = CMD_VOLUMESLIDE; param = (param << 4) | 0x0F; } else command=param=0; break;
								case 0xB0:	if (param & 0x0F) { command = CMD_VOLUMESLIDE; param |= 0xF0; } else command=param=0; break;
								}
								m[ch].command = command;
								m[ch].param = param;
							}
						} else
						{
							m[ch].command = cmd;
							m[ch].param = b2;
							ConvertModCommand(&m[ch]);
						}
					}
				}
				if (b1 & 0x80)
				{
					b1 = p[i++];
					if (i <= len) goto anothercommand;
				}
			}
			if (b0 & 0x80)
			{
				row++;
				m += m_nChannels;
			}
		}
		dwMemPos += len;
	}

	// Read Samples
	for (UINT iSmp=1; iSmp<=m_nSamples; iSmp++) if (Samples[iSmp].nLength)
	{
		if (dwMemPos >= dwMemLength - 9) return true;
		UINT flags = (Samples[iSmp].uFlags & CHN_16BIT) ? RS_AMS16 : RS_AMS8;
		dwMemPos += ReadSample(&Samples[iSmp], flags, (LPSTR)(lpStream+dwMemPos), dwMemLength-dwMemPos);
	}
	return true;
}


/////////////////////////////////////////////////////////////////////
// AMS (Velvet Studio) 2.2 loader

#pragma pack(1)

typedef struct AMS2FILEHEADER
{
	DWORD dwHdr1;		// AMShdr
	WORD wHdr2;
	BYTE b1A;			// 0x1A
	BYTE titlelen;		// 30-bytes max
	CHAR szTitle[30];	// [titlelen]
} AMS2FILEHEADER;

typedef struct AMS2SONGHEADER
{
	WORD version;
	BYTE instruments;
	WORD patterns;
	WORD orders;
	WORD bpm;
	BYTE speed;
	BYTE channels;
	BYTE commands;
	BYTE rows;
	WORD flags;
} AMS2SONGHEADER;

typedef struct AMS2INSTRUMENT
{
	BYTE samples;
	BYTE notemap[NOTE_MAX];
} AMS2INSTRUMENT;

typedef struct AMS2ENVELOPE
{
	BYTE speed;
	BYTE sustain;
	BYTE loopbegin;
	BYTE loopend;
	BYTE points;
	BYTE info[3];
} AMS2ENVELOPE;

typedef struct AMS2SAMPLE
{
	DWORD length;
	DWORD loopstart;
	DWORD loopend;
	WORD frequency;
	BYTE finetune;
	WORD nC5Speed;
	CHAR transpose;
	BYTE volume;
	BYTE flags;
} AMS2SAMPLE;


#pragma pack()


bool CSoundFile::ReadAMS2(LPCBYTE /*lpStream*/, DWORD /*dwMemLength*/)
//------------------------------------------------------------
{
	return false;
#if 0
	const AMS2FILEHEADER *pfh = (AMS2FILEHEADER *)lpStream;
	AMS2SONGHEADER *psh;
	DWORD dwMemPos;
	BYTE smpmap[16];
	BYTE packedsamples[MAX_SAMPLES];

	if ((pfh->dwHdr1 != 0x68534D41) || (pfh->wHdr2 != 0x7264)
	 || (pfh->b1A != 0x1A) || (pfh->titlelen > 30)) return false;
	dwMemPos = pfh->titlelen + 8;
	psh = (AMS2SONGHEADER *)(lpStream + dwMemPos);
	if (((psh->version & 0xFF00) != 0x0200) || (!psh->instruments)
	 || (psh->instruments > MAX_INSTRUMENTS) || (!psh->patterns) || (!psh->orders)) return false;
	dwMemPos += sizeof(AMS2SONGHEADER);
	if (pfh->titlelen)
	{
		memcpy(m_szNames[0], pfh->szTitle, pfh->titlelen);
		SpaceToNullStringFixed(m_szNames[0], pfh->titlelen);
	}
	m_nType = MOD_TYPE_AMS;
	m_nChannels = 32;
	m_nDefaultTempo = psh->bpm >> 8;
	m_nDefaultSpeed = psh->speed;
	m_nInstruments = psh->instruments;
	m_nSamples = 0;
	if (psh->flags & 0x40) m_dwSongFlags |= SONG_LINEARSLIDES;
	for (UINT nIns=1; nIns<=m_nInstruments; nIns++)
	{
		UINT insnamelen = lpStream[dwMemPos];
		CHAR *pinsname = (CHAR *)(lpStream+dwMemPos+1);
		dwMemPos += insnamelen + 1;
		AMS2INSTRUMENT *pSmp = (AMS2INSTRUMENT *)(lpStream + dwMemPos);
		dwMemPos += sizeof(AMS2INSTRUMENT);
		if (dwMemPos + 1024 >= dwMemLength) return TRUE;
		AMS2ENVELOPE *volenv, *panenv, *pitchenv;
		volenv = (AMS2ENVELOPE *)(lpStream+dwMemPos);
		dwMemPos += 5 + volenv->points*3;
		panenv = (AMS2ENVELOPE *)(lpStream+dwMemPos);
		dwMemPos += 5 + panenv->points*3;
		pitchenv = (AMS2ENVELOPE *)(lpStream+dwMemPos);
		dwMemPos += 5 + pitchenv->points*3;
		MODINSTRUMENT *pIns = new MODINSTRUMENT;
		if (!pIns) return TRUE;
		memset(smpmap, 0, sizeof(smpmap));
		memset(pIns, 0, sizeof(MODINSTRUMENT));
		for (UINT ismpmap=0; ismpmap<pSmp->samples; ismpmap++)
		{
			if ((ismpmap >= 16) || (m_nSamples+1 >= MAX_SAMPLES)) break;
			m_nSamples++;
			smpmap[ismpmap] = m_nSamples;
		}
		pIns->nGlobalVol = 64;
		pIns->nPan = 128;
		pIns->nPPC = 60;
		SetDefaultInstrumentValues(pIns);
		Instruments[nIns] = pIns;
		if (insnamelen)
		{
			if (insnamelen > 31) insnamelen = 31;
			memcpy(pIns->name, pinsname, insnamelen);
			pIns->name[insnamelen] = 0;
		}
		for (UINT inotemap=0; inotemap<NOTE_MAX; inotemap++)
		{
			pIns->NoteMap[inotemap] = inotemap+1;
			pIns->Keyboard[inotemap] = smpmap[pSmp->notemap[inotemap] & 0x0F];
		}
		// Volume Envelope
		{
			UINT pos = 0;
			pIns->VolEnv.nNodes = (volenv->points > 16) ? 16 : volenv->points;
			pIns->VolEnv.nSustainStart = pIns->VolEnv.nSustainEnd = volenv->sustain;
			pIns->VolEnv.nLoopStart = volenv->loopbegin;
			pIns->VolEnv.nLoopEnd = volenv->loopend;
			for (UINT i=0; i<pIns->VolEnv.nNodes; i++)
			{
				pIns->VolEnv.Values[i] = (BYTE)((volenv->info[i*3+2] & 0x7F) >> 1);
				pos += volenv->info[i*3] + ((volenv->info[i*3+1] & 1) << 8);
				pIns->VolEnv.Ticks[i] = (WORD)pos;
			}
		}
		pIns->nFadeOut = (((lpStream[dwMemPos+2] & 0x0F) << 8) | (lpStream[dwMemPos+1])) << 3;
		UINT envflags = lpStream[dwMemPos+3];
		if (envflags & 0x01) pIns->VolEnv.dwFlags |= ENV_LOOP;
		if (envflags & 0x02) pIns->VolEnv.dwFlags |= ENV_SUSTAIN;
		if (envflags & 0x04) pIns->VolEnv.dwFlags |= ENV_ENABLED;
		dwMemPos += 5;
		// Read Samples
		for (UINT ismp=0; ismp<pSmp->samples; ismp++)
		{
			MODSAMPLE *psmp = ((ismp < 16) && (smpmap[ismp])) ? &Samples[smpmap[ismp]] : NULL;
			UINT smpnamelen = lpStream[dwMemPos];
			if ((psmp) && (smpnamelen) && (smpnamelen <= 22))
			{
				memcpy(m_szNames[smpmap[ismp]], lpStream+dwMemPos+1, smpnamelen);
				SpaceToNullStringFixed(m_szNames[smpmap[ismp]], smpnamelen);
			}
			dwMemPos += smpnamelen + 1;
			if (psmp)
			{
				AMS2SAMPLE *pams = (AMS2SAMPLE *)(lpStream+dwMemPos);
				psmp->nGlobalVol = 64;
				psmp->nPan = 128;
				psmp->nLength = pams->length;
				psmp->nLoopStart = pams->loopstart;
				psmp->nLoopEnd = pams->loopend;
				psmp->nC5Speed = pams->nC5Speed;
				psmp->RelativeTone = pams->transpose;
				psmp->nVolume = pams->volume / 2;
				packedsamples[smpmap[ismp]] = pams->flags;
				if (pams->flags & 0x04) psmp->uFlags |= CHN_16BIT;
				if (pams->flags & 0x08) psmp->uFlags |= CHN_LOOP;
				if (pams->flags & 0x10) psmp->uFlags |= CHN_PINGPONGLOOP;
			}
			dwMemPos += sizeof(AMS2SAMPLE);
		}
	}
	if (dwMemPos + 256 >= dwMemLength) return true;
	// Comments
	{
		UINT composernamelen = lpStream[dwMemPos];
		if (composernamelen)
		{
			ReadMessage(lpStream + dwMemPos + 1, composernamelen, leCR);
		}
		dwMemPos += composernamelen + 1;
		// channel names
		for (UINT i=0; i<32; i++)
		{
			UINT chnnamlen = lpStream[dwMemPos];
			if ((chnnamlen) && (chnnamlen < MAX_CHANNELNAME))
			{
				memcpy(ChnSettings[i].szName, lpStream+dwMemPos+1, chnnamlen);
				SpaceToNullStringFixed(ChnSettings[i].szName, chnnamlen);
			}
			dwMemPos += chnnamlen + 1;
			if (dwMemPos + chnnamlen + 256 >= dwMemLength) return true;
		}
		// packed comments (ignored)
		UINT songtextlen = *((LPDWORD)(lpStream+dwMemPos));
		dwMemPos += songtextlen;
		if (dwMemPos + 256 >= dwMemLength) return true;
	}
	// Order List
	{
		if ((dwMemPos + 2 * psh->orders) >= dwMemLength) return true;
		Order.resize(psh->orders, Order.GetInvalidPatIndex());
		for (UINT iOrd = 0; iOrd < psh->orders; iOrd++)
		{
			Order[iOrd] = (PATTERNINDEX)*((WORD *)(lpStream + dwMemPos));
			dwMemPos += 2;
		}
	}
	// Pattern Data
	for (UINT ipat=0; ipat<psh->patterns; ipat++)
	{
		if (dwMemPos+8 >= dwMemLength) return true;
		UINT packedlen = *((LPDWORD)(lpStream+dwMemPos));
		UINT numrows = 1 + (UINT)(lpStream[dwMemPos+4]);
		//UINT patchn = 1 + (UINT)(lpStream[dwMemPos+5] & 0x1F);
		//UINT patcmds = 1 + (UINT)(lpStream[dwMemPos+5] >> 5);
		UINT patnamlen = lpStream[dwMemPos+6];
		dwMemPos += 4;
		if ((ipat < MAX_PATTERNS) && (packedlen < dwMemLength-dwMemPos) && (numrows >= 8))
		{
			if ((patnamlen) && (patnamlen < MAX_PATTERNNAME))
			{
				char s[MAX_PATTERNNAME];
				memcpy(s, lpStream+dwMemPos+3, patnamlen);
				SpaceToNullStringFixed(s, patnamlen);
				SetPatternName(ipat, s);
			}
			if(Patterns.Insert(ipat, numrows)) return true;
			// Unpack Pattern Data
			LPCBYTE psrc = lpStream + dwMemPos;
			UINT pos = 3 + patnamlen;
			UINT row = 0;
			while ((pos < packedlen) && (row < numrows))
			{
				MODCOMMAND *m = Patterns[ipat] + row * m_nChannels;
				UINT byte1 = psrc[pos++];
				UINT ch = byte1 & 0x1F;
				// Read Note + Instr
				if (!(byte1 & 0x40))
				{
					UINT byte2 = psrc[pos++];
					UINT note = byte2 & 0x7F;
					if (note) m[ch].note = (note > 1) ? (note-1) : 0xFF;
					m[ch].instr = psrc[pos++];
					// Read Effect
					while (byte2 & 0x80)
					{
						byte2 = psrc[pos++];
						if (byte2 & 0x40)
						{
							m[ch].volcmd = VOLCMD_VOLUME;
							m[ch].vol = byte2 & 0x3F;
						} else
						{
							UINT command = byte2 & 0x3F;
							UINT param = psrc[pos++];
							if (command == 0x0C)
							{
								m[ch].volcmd = VOLCMD_VOLUME;
								m[ch].vol = param / 2;
							} else
							if (command < 0x10)
							{
								m[ch].command = command;
								m[ch].param = param;
								ConvertModCommand(&m[ch]);
							} else
							{
								// TODO: AMS effects
							}
						}
					}
				}
				if (byte1 & 0x80) row++;
			}
		}
		dwMemPos += packedlen;
	}
	// Read Samples
	for (UINT iSmp=1; iSmp<=m_nSamples; iSmp++) if (Samples[iSmp].nLength)
	{
		if (dwMemPos >= dwMemLength - 9) return true;
		UINT flags;
		if (packedsamples[iSmp] & 0x03)
		{
			flags = (Samples[iSmp].uFlags & CHN_16BIT) ? RS_AMS16 : RS_AMS8;
		} else
		{
			flags = (Samples[iSmp].uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S;
		}
		dwMemPos += ReadSample(&Samples[iSmp], flags, (LPSTR)(lpStream+dwMemPos), dwMemLength-dwMemPos);
	}
	return true;
#endif
}


/////////////////////////////////////////////////////////////////////
// AMS Sample unpacking

void AMSUnpack(const char *psrc, UINT inputlen, char *pdest, UINT dmax, char packcharacter)
//-----------------------------------------------------------------------------------------
{
	UINT tmplen = dmax;
	signed char *amstmp = new signed char[tmplen];
	
	if (!amstmp) return;
	// Unpack Loop
	{
		signed char *p = amstmp;
		UINT i=0, j=0;
		while ((i < inputlen) && (j < tmplen))
		{
			signed char ch = psrc[i++];
			if (ch == packcharacter)
			{
				BYTE ch2 = psrc[i++];
				if (ch2)
				{
					ch = psrc[i++];
					while (ch2--)
					{
						p[j++] = ch;
						if (j >= tmplen) break;
					}
				} else p[j++] = packcharacter;
			} else p[j++] = ch;
		}
	}
	// Bit Unpack Loop
	{
		signed char *p = amstmp;
		UINT bitcount = 0x80, dh;
		UINT k=0;
		for (UINT i=0; i<dmax; i++)
		{
			BYTE al = *p++;
			dh = 0;
			for (UINT count=0; count<8; count++)
			{
				UINT bl = al & bitcount;
				bl = ((bl|(bl<<8)) >> ((dh+8-count) & 7)) & 0xFF;
				bitcount = ((bitcount|(bitcount<<8)) >> 1) & 0xFF;
				pdest[k++] |= bl;
				if (k >= dmax)
				{
					k = 0;
					dh++;
				}
			}
			bitcount = ((bitcount|(bitcount<<8)) >> dh) & 0xFF;
		}
	}
	// Delta Unpack
	{
		signed char old = 0;
		for (UINT i=0; i<dmax; i++)
		{
			int pos = ((LPBYTE)pdest)[i];
			if ((pos != 128) && (pos & 0x80)) pos = -(pos & 0x7F);
			old -= (signed char)pos;
			pdest[i] = old;
		}
	}
	delete[] amstmp;
}

