/*
 * This source code is public domain.
 *
 * Copied to OpenMPT from libmodplug.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>,
 *          Adam Goode       <adam@evdebs.org> (endian and char fixes for PPC)
 *			OpenMPT dev(s)	(miscellaneous modifications)
*/

#include "stdafx.h"
#include "Loaders.h"
#include "../mptrack/version.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/moddoc.h"
#endif // MODPLUG_TRACKER

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

//////////////////////////////////////////////////////
// ScreamTracker S3M file support

typedef struct tagS3MSAMPLESTRUCT
{
	BYTE type;
	CHAR dosname[12];
	BYTE hmem;
	WORD memseg;
	DWORD length;
	DWORD loopbegin;
	DWORD loopend;
	BYTE vol;
	BYTE bReserved;
	BYTE pack;
	BYTE flags;
	DWORD finetune;
	DWORD dwReserved;
	WORD intgp;
	WORD int512;
	DWORD lastused;
	CHAR name[28];
	CHAR scrs[4];
} S3MSAMPLESTRUCT;


typedef struct tagS3MFILEHEADER
{
	CHAR name[28];
	BYTE b1A;
	BYTE type;
	WORD reserved1;
	WORD ordnum;
	WORD insnum;
	WORD patnum;
	WORD flags;
	WORD cwtv;
	WORD version;
	DWORD scrm;	// "SCRM" = 0x4D524353
	BYTE globalvol;
	BYTE speed;
	BYTE tempo;
	BYTE mastervol;
	BYTE ultraclicks;
	BYTE panning_present;
	BYTE reserved2[8];
	WORD special;
	BYTE channels[32];
} S3MFILEHEADER;

enum
{
	S3I_TYPE_NONE = 0,
	S3I_TYPE_PCM = 1,
	S3I_TYPE_ADMEL = 2,
};

void CSoundFile::S3MConvert(MODCOMMAND *m, bool bIT) const
//--------------------------------------------------------
{
	UINT command = m->command;
	UINT param = m->param;
	switch (command | 0x40)
	{
	case 'A':	command = CMD_SPEED; break;
	case 'B':	command = CMD_POSITIONJUMP; break;
	case 'C':	command = CMD_PATTERNBREAK; if (!bIT) param = (param >> 4) * 10 + (param & 0x0F); break;
	case 'D':	command = CMD_VOLUMESLIDE; break;
	case 'E':	command = CMD_PORTAMENTODOWN; break;
	case 'F':	command = CMD_PORTAMENTOUP; break;
	case 'G':	command = CMD_TONEPORTAMENTO; break;
	case 'H':	command = CMD_VIBRATO; break;
	case 'I':	command = CMD_TREMOR; break;
	case 'J':	command = CMD_ARPEGGIO; break;
	case 'K':	command = CMD_VIBRATOVOL; break;
	case 'L':	command = CMD_TONEPORTAVOL; break;
	case 'M':	command = CMD_CHANNELVOLUME; break;
	case 'N':	command = CMD_CHANNELVOLSLIDE; break;
	case 'O':	command = CMD_OFFSET; break;
	case 'P':	command = CMD_PANNINGSLIDE; break;
	case 'Q':	command = CMD_RETRIG; break;
	case 'R':	command = CMD_TREMOLO; break;
	case 'S':	command = CMD_S3MCMDEX; break;
	case 'T':	command = CMD_TEMPO; break;
	case 'U':	command = CMD_FINEVIBRATO; break;
	case 'V':	command = CMD_GLOBALVOLUME; break;
	case 'W':	command = CMD_GLOBALVOLSLIDE; break;
	case 'X':	command = CMD_PANNING8; break;
	case 'Y':	command = CMD_PANBRELLO; break;
	case 'Z':	command = CMD_MIDI; break;
	case '\\':	command = CMD_SMOOTHMIDI; break; //rewbs.smoothVST
	// Chars under 0x40 don't save properly, so map : to ] and # to [.
	case ']':	command = CMD_DELAYCUT; break;
	case '[':	command = CMD_XPARAM; break;
	default:	command = 0;
	}
	m->command = command;
	m->param = param;
}


void CSoundFile::S3MSaveConvert(UINT *pcmd, UINT *pprm, bool bIT, bool bCompatibilityExport) const
//------------------------------------------------------------------------------------------------
{
	UINT command = *pcmd;
	UINT param = *pprm;
	switch(command)
	{
	case CMD_SPEED:				command = 'A'; break;
	case CMD_POSITIONJUMP:		command = 'B'; break;
	case CMD_PATTERNBREAK:		command = 'C'; if (!bIT) param = ((param / 10) << 4) + (param % 10); break;
	case CMD_VOLUMESLIDE:		command = 'D'; break;
	case CMD_PORTAMENTODOWN:	command = 'E'; if ((param >= 0xE0) && (m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))) param = 0xDF; break;
	case CMD_PORTAMENTOUP:		command = 'F'; if ((param >= 0xE0) && (m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))) param = 0xDF; break;
	case CMD_TONEPORTAMENTO:	command = 'G'; break;
	case CMD_VIBRATO:			command = 'H'; break;
	case CMD_TREMOR:			command = 'I'; break;
	case CMD_ARPEGGIO:			command = 'J'; break;
	case CMD_VIBRATOVOL:		command = 'K'; break;
	case CMD_TONEPORTAVOL:		command = 'L'; break;
	case CMD_CHANNELVOLUME:		command = 'M'; break;
	case CMD_CHANNELVOLSLIDE:	command = 'N'; break;
	case CMD_OFFSET:			command = 'O'; break;
	case CMD_PANNINGSLIDE:		command = 'P'; break;
	case CMD_RETRIG:			command = 'Q'; break;
	case CMD_TREMOLO:			command = 'R'; break;
	case CMD_S3MCMDEX:			command = 'S'; break;
	case CMD_TEMPO:				command = 'T'; break;
	case CMD_FINEVIBRATO:		command = 'U'; break;
	case CMD_GLOBALVOLUME:		command = 'V'; break;
	case CMD_GLOBALVOLSLIDE:	command = 'W'; break;
	case CMD_PANNING8:			
		command = 'X';
		if (bIT && !(m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)))
		{
			if (param == 0xA4) { command = 'S'; param = 0x91; }	else
			if (param <= 0x80) { param <<= 1; if (param > 255) param = 255; } else
			command = param = 0;
		} else
		if (!bIT && (m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)))
		{
			param >>= 1;
		}
		break;
	case CMD_PANBRELLO:			command = 'Y'; break;
	case CMD_MIDI:				command = 'Z'; break;
	case CMD_SMOOTHMIDI:  //rewbs.smoothVST
		if(bCompatibilityExport)
			command = 'Z';
		else
			command = '\\';
		break;
	case CMD_XFINEPORTAUPDOWN:
		if (param & 0x0F) switch(param & 0xF0)
		{
		case 0x10:	command = 'F'; param = (param & 0x0F) | 0xE0; break;
		case 0x20:	command = 'E'; param = (param & 0x0F) | 0xE0; break;
		case 0x90:	command = 'S'; break;
		default:	command = param = 0;
		} else command = param = 0;
		break;
	case CMD_MODCMDEX:
		command = 'S';
		switch(param & 0xF0)
		{
		case 0x00:	command = param = 0; break;
		case 0x10:	command = 'F'; param |= 0xF0; break;
		case 0x20:	command = 'E'; param |= 0xF0; break;
		case 0x30:	param = (param & 0x0F) | 0x10; break;
		case 0x40:	param = (param & 0x0F) | 0x30; break;
		case 0x50:	param = (param & 0x0F) | 0x20; break;
		case 0x60:	param = (param & 0x0F) | 0xB0; break;
		case 0x70:	param = (param & 0x0F) | 0x40; break;
		case 0x90:	command = 'Q'; param &= 0x0F; break;
		case 0xA0:	if (param & 0x0F) { command = 'D'; param = (param << 4) | 0x0F; } else command=param=0; break;
		case 0xB0:	if (param & 0x0F) { command = 'D'; param |= 0xF0; } else command=param=0; break;
		}
		break;
	// Chars under 0x40 don't save properly, so map : to ] and # to [.	
	case CMD_DELAYCUT: 
		if(bCompatibilityExport || !bIT)
			command = param = 0;
		else
			command = ']';
		break;
	case CMD_XPARAM:
		if(bCompatibilityExport || !bIT)
			command = param = 0;
		else
			command = '[';
		break;
	default:	command = param = 0;
	}
	command &= ~0x40;
	*pcmd = command;
	*pprm = param;
}


bool CSoundFile::ReadS3M(const BYTE *lpStream, const DWORD dwMemLength)
//---------------------------------------------------------------------
{
	if ((!lpStream) || (dwMemLength <= sizeof(S3MFILEHEADER) + 64)) return false;

	UINT insnum, patnum, nins, npat;
	BYTE s[1024];
	DWORD dwMemPos;
	vector<DWORD> smpdatapos;
	vector<WORD> smppos;
	vector<WORD> patpos;
	vector<BYTE> insflags;
	vector<BYTE> inspack;
	S3MFILEHEADER psfh = *(S3MFILEHEADER *)lpStream;
	bool bKeepMidiMacros = false, bHasAdlibPatches = false;

	psfh.reserved1 = LittleEndianW(psfh.reserved1);
	psfh.ordnum = LittleEndianW(psfh.ordnum);
	psfh.insnum = LittleEndianW(psfh.insnum);
	psfh.patnum = LittleEndianW(psfh.patnum);
	psfh.flags = LittleEndianW(psfh.flags);
	psfh.cwtv = LittleEndianW(psfh.cwtv);
	psfh.version = LittleEndianW(psfh.version);
	psfh.scrm = LittleEndian(psfh.scrm);
	psfh.special = LittleEndianW(psfh.special);

	if (psfh.scrm != 0x4D524353) return false;

	if((psfh.cwtv & 0xF000) == 0x5000) // OpenMPT Version number (Major.Minor)
	{
		m_dwLastSavedWithVersion = (psfh.cwtv & 0x0FFF) << 16;
		bKeepMidiMacros = true; // simply load Zxx commands
	}
	if(psfh.cwtv  == 0x1320 && psfh.special == 0 && (psfh.ordnum & 0x0F) == 0 && psfh.ultraclicks == 0 && (psfh.flags & ~0x50) == 0)
	{
		// MPT 1.16 and older versions of OpenMPT
		m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 16, 00, 00);
		bKeepMidiMacros = true; // simply load Zxx commands
	}

	if((psfh.cwtv & 0xF000) >= 0x2000)
	{
		// 2xyy - Orpheus, 3xyy - IT, 4xyy - Schism, 5xyy - OpenMPT
		bKeepMidiMacros = true; // simply load Zxx commands
	}

	if(!bKeepMidiMacros) // Remove macros so they don't interfere with other tunes
		memset(&m_MidiCfg, 0, sizeof(m_MidiCfg));

	dwMemPos = 0x60;
	m_nType = MOD_TYPE_S3M;
	memset(m_szNames, 0, sizeof(m_szNames));
	memcpy(m_szNames[0], psfh.name, 28);
	SpaceToNullStringFixed<28>(m_szNames[0]);
	// Speed
	m_nDefaultSpeed = psfh.speed;
	if (!m_nDefaultSpeed || m_nDefaultSpeed == 255) m_nDefaultSpeed = 6;
	// Tempo
	m_nDefaultTempo = psfh.tempo;
	//m_nDefaultTempo = CLAMP(m_nDefaultTempo, 32, 255);
	if(m_nDefaultTempo < 33) m_nDefaultTempo = 125;
	// Global Volume
	m_nDefaultGlobalVolume = psfh.globalvol << 2;
	if(!m_nDefaultGlobalVolume && psfh.cwtv < 0x1320) m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME; // not very reliable, but it fixes a few tunes
	if(m_nDefaultGlobalVolume > MAX_GLOBAL_VOLUME) m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	m_nSamplePreAmp = CLAMP(psfh.mastervol & 0x7F, 0x10, 0x7F); // Bit 8 = Stereo (we always use stereo)
	// Channels
	m_nChannels = 4;
	for (UINT ich=0; ich<32; ich++)
	{
		ChnSettings[ich].nVolume = 64;

		if(psfh.channels[ich] == 0xFF)
		{
			ChnSettings[ich].nPan = 128;
			ChnSettings[ich].dwFlags = CHN_MUTE;
		} else
		{
			m_nChannels = ich + 1;
			ChnSettings[ich].nPan = (psfh.channels[ich] & 8) ? 0xC0 : 0x40;
			if (psfh.channels[ich] & 0x80)
			{
				ChnSettings[ich].dwFlags = CHN_MUTE;
				/* Detect Adlib channels:
				   c = channels[ich] ^ 0x80;
				   if(c >= 16 && c < 32) adlibChannel = true;
				*/
			}
		}
	}
	if (m_nChannels < 1) m_nChannels = 1;
	if ((psfh.cwtv < 0x1320) || (psfh.flags & 0x40)) m_dwSongFlags |= SONG_FASTVOLSLIDES;
	// Reading pattern order
	UINT iord = psfh.ordnum;
	iord = CLAMP(iord, 1, MAX_ORDERS);
	if (iord)
	{
		Order.ReadAsByte(lpStream+dwMemPos, iord, dwMemLength-dwMemPos);
		dwMemPos += iord;
	}
	if ((iord & 1) && (lpStream[dwMemPos] == 0xFF)) dwMemPos++;
	// Reading file pointers
	insnum = nins = psfh.insnum;
	if (insnum >= MAX_SAMPLES) insnum = MAX_SAMPLES-1;
	m_nSamples = insnum;
	patnum = npat = psfh.patnum;
	if (patnum > MAX_PATTERNS) patnum = MAX_PATTERNS;

	// Read sample header offsets
	smppos.resize(nins, 0);
	for(UINT i = 0; i < nins; i++, dwMemPos += 2)
	{
		WORD ptr = *((WORD *)(lpStream + dwMemPos));
		smppos[i] = LittleEndianW(ptr);
	}
	// Read pattern offsets
	patpos.resize(npat, 0);
	for(UINT i = 0; i < npat; i++, dwMemPos += 2)
	{
		WORD ptr = *((WORD *)(lpStream + dwMemPos));
		patpos[i] = LittleEndianW(ptr);
	}
	// Read channel panning
	if (psfh.panning_present == 0xFC)
	{
		const BYTE *chnpan = lpStream+dwMemPos;
		for (UINT i=0; i<32; i++) if (chnpan[i] & 0x20)
		{
			ChnSettings[i].nPan = ((chnpan[i] & 0x0F) << 4) + 8;
		}
	}

	// Reading instrument headers
	smpdatapos.resize(insnum, 0);
	inspack.resize(insnum, 0);
	insflags.resize(insnum, 0);
	for (UINT iSmp=1; iSmp<=insnum; iSmp++)
	{
		UINT nInd = ((DWORD)smppos[iSmp - 1]) * 16;
		if (nInd + 0x50 > dwMemLength) continue;

		memcpy(s, lpStream + nInd, 0x50);
		memcpy(Samples[iSmp].filename, s+1, 12);
		SpaceToNullStringFixed<12>(Samples[iSmp].filename);

		insflags[iSmp - 1] = s[0x1F];
		inspack[iSmp - 1] = s[0x1E];
		s[0x4C] = 0;
		lstrcpy(m_szNames[iSmp], (LPCSTR)&s[0x30]);
		SpaceToNullStringFixed<28>(m_szNames[iSmp]);

		if ((s[0] == S3I_TYPE_PCM) && (s[0x4E] == 'R') && (s[0x4F] == 'S'))
		{
			Samples[iSmp].nLength = CLAMP(LittleEndian(*((LPDWORD)(s + 0x10))), 4, MAX_SAMPLE_LENGTH);
			Samples[iSmp].nLoopStart = CLAMP(LittleEndian(*((LPDWORD)(s + 0x14))), 0, Samples[iSmp].nLength - 1);
			Samples[iSmp].nLoopEnd = CLAMP(LittleEndian(*((LPDWORD)(s+0x18))), 0, Samples[iSmp].nLength);
			Samples[iSmp].nVolume = CLAMP(s[0x1C], 0, 64) << 2;
			Samples[iSmp].nGlobalVol = 64;
			if (s[0x1F] & 1) Samples[iSmp].uFlags |= CHN_LOOP;

			UINT c5Speed;
			c5Speed = LittleEndian(*((LPDWORD)(s+0x20)));
			if (!c5Speed) c5Speed = 8363;
			if (c5Speed < 1024) c5Speed = 1024;
			Samples[iSmp].nC5Speed = c5Speed;

			smpdatapos[iSmp - 1] = (s[0x0E] << 4) | (s[0x0F] << 12) | (s[0x0D] << 20);

			if(Samples[iSmp].nLoopEnd < 2)
				Samples[iSmp].nLoopStart = Samples[iSmp].nLoopEnd = 0;

			if ((Samples[iSmp].nLoopStart >= Samples[iSmp].nLoopEnd) || (Samples[iSmp].nLoopEnd - Samples[iSmp].nLoopStart < 1))
 				Samples[iSmp].nLoopStart = Samples[iSmp].nLoopEnd = 0;
			Samples[iSmp].nPan = 0x80;

		} else if(s[0] >= S3I_TYPE_ADMEL)
		{
			bHasAdlibPatches = true;
		}
	}

	if (!m_nChannels) return true;

	/* Try to find out if Zxx commands are supposed to be panning commands (PixPlay).
	   We won't convert if there are not enough Zxx commands, too "high" Zxx commands
	   or there are only "left" or "right" pannings (we assume that stereo should be somewhat balanced) */
	bool bDoNotConvertZxx = false;
	int iZxxCountRight = 0, iZxxCountLeft = 0;

	// Reading patterns
	for (UINT iPat = 0; iPat < patnum; iPat++)
	{
		bool fail = Patterns.Insert(iPat, 64);
		UINT nInd = ((DWORD)patpos[iPat]) * 16;
		if (nInd == 0 || nInd + 0x40 > dwMemLength) continue;
		WORD len = LittleEndianW(*((WORD *)(lpStream + nInd)));
		nInd += 2;
		if ((!len) || (nInd + len > dwMemLength - 6)
		 || (fail) ) continue;
		LPBYTE src = (LPBYTE)(lpStream+nInd);
		// Unpacking pattern
		MODCOMMAND *p = Patterns[iPat];
		UINT row = 0;
		UINT j = 0;
		while (row < 64) // this fixes ftp://us.aminet.net/pub/aminet/mods/8voic/s3m_hunt.lha (was: while (j < len))
		{
			if(j + nInd + 1 >= dwMemLength) break;
			BYTE b = src[j++];
			if (!b)
			{
				if (++row >= 64) break;
			} else
			{
				UINT chn = b & 0x1F;
				if (chn < m_nChannels)
				{
					MODCOMMAND *m = &p[row * m_nChannels + chn];
					if (b & 0x20)
					{
						if(j + nInd + 2 >= dwMemLength) break;
						m->note = src[j++];
						if (m->note < 0xF0) m->note = (m->note & 0x0F) + 12*(m->note >> 4) + 13;
						else if (m->note == 0xFF) m->note = NOTE_NONE;
						m->instr = src[j++];
					}
					if (b & 0x40)
					{
						if(j + nInd + 1 >= dwMemLength) break;
						UINT vol = src[j++];
						if ((vol >= 128) && (vol <= 192))
						{
							vol -= 128;
							m->volcmd = VOLCMD_PANNING;
						} else
						{
							if (vol > 64) vol = 64;
							m->volcmd = VOLCMD_VOLUME;
						}
						m->vol = vol;
					}
					if (b & 0x80)
					{
						if(j + nInd + 2 >= dwMemLength) break;
						m->command = src[j++];
						m->param = src[j++];
						if (m->command) S3MConvert(m, false);
						if(m->command == CMD_MIDI)
						{
							if(m->param > 0x0F)
							{
								// PixPlay has Z00 to Z0F panning, so we ignore this.
								bDoNotConvertZxx = true;
							}
							else
							{
								if(m->param < 0x08) iZxxCountLeft++;
								if(m->param > 0x08) iZxxCountRight++;
							}
						}
					}
				} else
				{
					if (b & 0x20) j += 2;
					if (b & 0x40) j++;
					if (b & 0x80) j += 2;
				}
			}
		}
	}

	if((UINT)(iZxxCountLeft + iZxxCountRight) >= m_nChannels && !bDoNotConvertZxx && (iZxxCountLeft - iZxxCountRight > -(int)m_nChannels))
	{
		// there are enough Zxx commands, so let's assume this was made to be played with PixPlay
		for(PATTERNINDEX nPat = 0; nPat < Patterns.Size(); nPat++) if(Patterns[nPat])
		{
			MODCOMMAND *m = Patterns[nPat];
			for(UINT len = Patterns[nPat].GetNumRows() * m_nChannels; len; m++, len--)
			{
				if(m->command == CMD_MIDI)
				{
					m->command = CMD_S3MCMDEX;
					m->param |= 0x80;
				}
			}
		}
	}

	// Reading samples
	for (UINT iRaw = 1; iRaw <= insnum; iRaw++) if (Samples[iRaw].nLength)
	{
		UINT flags = (psfh.version == 1) ? RS_PCM8S : RS_PCM8U;
		if (insflags[iRaw-1] & 4) flags += 5;
		if (insflags[iRaw-1] & 2) flags |= RSF_STEREO;
		if (inspack[iRaw-1] == 4) flags = RS_ADPCM4;
		if(smpdatapos[iRaw - 1] < dwMemLength)
		{
			dwMemPos = smpdatapos[iRaw - 1];
		}
		if(dwMemPos < dwMemLength)
		{
			dwMemPos += ReadSample(&Samples[iRaw], flags, (LPSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
		}
	}
	m_nMinPeriod = 64;
	m_nMaxPeriod = 32767;
	if (psfh.flags & 0x10) m_dwSongFlags |= SONG_AMIGALIMITS;

#ifdef MODPLUG_TRACKER
	if(bHasAdlibPatches && GetpModDoc() != nullptr)
	{
		GetpModDoc()->AddToLog("This track uses Adlib instruments, which are not supported by OpenMPT.");
	}
#endif // MODPLUG_TRACKER

	return true;
}


#ifndef MODPLUG_NO_FILESAVE
#pragma warning(disable:4100) //unreferenced formal parameter

static const BYTE S3MFiller[16] =
{
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};


bool CSoundFile::SaveS3M(LPCSTR lpszFileName, UINT nPacking)
//----------------------------------------------------------
{
	FILE *f;
	BYTE header[0x60];
	UINT nbo,nbi,nbp,i;
	WORD patptr[128];
	WORD insptr[128];
	S3MSAMPLESTRUCT insex[128];

	if ((!m_nChannels) || (!lpszFileName)) return false;
	if ((f = fopen(lpszFileName, "wb")) == NULL) return false;
	// Writing S3M header
	memset(header, 0, sizeof(header));
	memset(insex, 0, sizeof(insex));
	memcpy(header, m_szNames[0], 0x1C);
	header[0x1B] = 0;
	header[0x1C] = 0x1A;
	header[0x1D] = 0x10;

	nbo = Order.GetLengthTailTrimmed();
	if (nbo < 2)
		nbo = 2;
	else if (nbo & 1) // number of orders must be even
		nbo++;
	if(nbo > 0xF0) nbo = 0xF0; // sequence too long
	header[0x20] = nbo & 0xFF;
	header[0x21] = nbo >> 8;
	nbi = m_nInstruments;
	if (!nbi) nbi = m_nSamples;
	if (nbi > 99) nbi = 99;
	header[0x22] = nbi & 0xFF;
	header[0x23] = nbi >> 8;
	nbp = 0;
	for (i = 0; Patterns[i]; i++) { nbp = i+1; if (nbp >= MAX_PATTERNS) break; }
	for (ORDERINDEX nOrd = 0; nOrd < Order.GetLengthTailTrimmed(); nOrd++) if ((Order[nOrd] < MAX_PATTERNS) && (Order[nOrd] >= nbp)) nbp = Order[nOrd] + 1;
	header[0x24] = nbp & 0xFF;
	header[0x25] = nbp >> 8;
	if (m_dwSongFlags & SONG_FASTVOLSLIDES) header[0x26] |= 0x40;
	if ((m_nMaxPeriod < 20000) || (m_dwSongFlags & SONG_AMIGALIMITS)) header[0x26] |= 0x10;

	// Version info following: ST3.20 = 0x1320
	// Most significant nibble: 1 = ST3, 2 = Orpheus, 3 = IT, 4 = Schism, 5 = MPT
	// Following: One nibble = Major version, one byte = Minor version (hex)
	MptVersion::VersionNum vVersion = MptVersion::num;
	header[0x28] = (BYTE)((vVersion >> 16) & 0xFF); // the "17" in OpenMPT 1.17
	header[0x29] = 0x50 | (BYTE)((vVersion >> 24) & 0x0F); // the "1." in OpenMPT 1.17 + OpenMPT Identifier 5 (works only for versions up to 9.99 :))
	header[0x2A] = 0x02; // Version = 1 => Signed samples
	header[0x2B] = 0x00;
	header[0x2C] = 'S';
	header[0x2D] = 'C';
	header[0x2E] = 'R';
	header[0x2F] = 'M';
	header[0x30] = m_nDefaultGlobalVolume >> 2;
	header[0x31] = CLAMP(m_nDefaultSpeed, 1, 254);
	header[0x32] = CLAMP(m_nDefaultTempo, 33, 255);
	header[0x33] = CLAMP(m_nSamplePreAmp, 0x10, 0x7F) | 0x80;	// Bit 8 = Stereo
	header[0x34] = 0x08; // 8 Channels for UltraClick removal (default)
	header[0x35] = 0xFC; // Write pan positions
	for (i=0; i<32; i++)
	{
		if (i < m_nChannels)
		{
			UINT tmp = (i & 0x0F) >> 1;
			tmp = (i & 0x10) | ((i & 1) ? 8 + tmp : tmp);
			if((ChnSettings[i].dwFlags & CHN_MUTE) != 0) tmp |= 0x80;
			header[0x40+i] = tmp;
		} else header[0x40+i] = 0xFF;
	}
	fwrite(header, 0x60, 1, f);
	nbo = Order.WriteAsByte(f, nbo);
	memset(patptr, 0, sizeof(patptr));
	memset(insptr, 0, sizeof(insptr));
	UINT ofs0 = 0x60 + nbo;
	UINT ofs1 = ((0x60 + nbo + nbi * 2 + nbp * 2 + 15) & 0xFFF0) + ((header[0x35] == 0xFC) ? 0x20 : 0);
	UINT ofs = ofs1;
	for (i=0; i<nbi; i++) insptr[i] = (WORD)((ofs + i*0x50) / 16);
	for (i=0; i<nbp; i++) patptr[i] = (WORD)((ofs + nbi*0x50) / 16);
	fwrite(insptr, nbi, 2, f);
	fwrite(patptr, nbp, 2, f);
	if (header[0x35] == 0xFC)
	{
		BYTE chnpan[32];
		for (i=0; i<32; i++)
		{
			UINT nPan = ((ChnSettings[i].nPan+7) < 0xF0) ? ChnSettings[i].nPan+7 : 0xF0;
			chnpan[i] = (i<m_nChannels) ? 0x20 | (nPan >> 4) : 0x08;
		}
		fwrite(chnpan, 0x20, 1, f);
	}
	if ((nbi*2+nbp*2) & 0x0F)
	{
		fwrite(S3MFiller, 0x10 - ((nbi*2+nbp*2) & 0x0F), 1, f);
	}
	fseek(f, ofs1, SEEK_SET);
	ofs1 = ftell(f);
	fwrite(insex, nbi, 0x50, f);
	// Packing patterns
	ofs += nbi * 0x50;
	for (i = 0; i < nbp; i++)
	{
		WORD len = 64;
		vector<BYTE> buffer(5 * 1024, 0);
		patptr[i] = ofs / 16;
		if (Patterns[i])
		{
			len = 2;
			MODCOMMAND *p = Patterns[i];
			for (UINT row=0; row<64; row++) if (row < Patterns[i].GetNumRows())
			{
				for (UINT j=0; j<m_nChannels; j++)
				{
					UINT b = j;
					MODCOMMAND *m = &p[row*m_nChannels+j];
					UINT note = m->note;
					UINT volcmd = m->volcmd;
					UINT vol = m->vol;
					UINT command = m->command;
					UINT param = m->param;

					if ((note) || (m->instr)) b |= 0x20;

					if (!note) note = 0xFF; // no note
					else if (note >= NOTE_MIN_SPECIAL) note = 0xFE; // special notes (notecut, noteoff etc)
					else if (note < 13) note = NOTE_NONE; // too low
					else note -= 13;

					if (note < 0xFE) note = (note % 12) + ((note / 12) << 4);
					if (command == CMD_VOLUME)
					{
						command = 0;
						volcmd = VOLCMD_VOLUME;
						vol = min(param, 64);
					}
					if (volcmd == VOLCMD_VOLUME) b |= 0x40; else
					if (volcmd == VOLCMD_PANNING) { vol |= 0x80; b |= 0x40; }
					if (command)
					{
						S3MSaveConvert(&command, &param, false, true);
						if (command) b |= 0x80;
					}
					if (b & 0xE0)
					{
						buffer[len++] = b;
						if (b & 0x20)
						{
							buffer[len++] = note;
							buffer[len++] = m->instr;
						}
						if (b & 0x40)
						{
							buffer[len++] = vol;
						}
						if (b & 0x80)
						{
							buffer[len++] = command;
							buffer[len++] = param;
						}
						if (len > buffer.size() - 20)
						{   //Buffer running out? Make it larger.
							buffer.resize(buffer.size() + 1024, 0);
						}
					}
				}
				buffer[len++] = 0;
				if (len > buffer.size() - 20)
				{   //Buffer running out? Make it larger.
					buffer.resize(buffer.size() + 1024, 0);
				}
			}
		}
		buffer[0] = (len) & 0xFF;
		buffer[1] = (len) >> 8;
		len = (len + 15) & (~0x0F);
		fwrite(&buffer[0], len, 1, f);
		ofs += len;
	}
	// Writing samples
	for (i=1; i<=nbi; i++)
	{
		MODSAMPLE *pSmp = &Samples[i];
		if (m_nInstruments)
		{
			pSmp = Samples;
			if (Instruments[i])
			{
				for (UINT j=0; j<128; j++)
				{
					UINT n = Instruments[i]->Keyboard[j];
					if ((n) && (n < MAX_INSTRUMENTS))
					{
						pSmp = &Samples[n];
						break;
					}
				}
			}
		}
		memcpy(insex[i-1].dosname, pSmp->filename, 12);
		memcpy(insex[i-1].name, m_szNames[i], 28);
		memcpy(insex[i-1].scrs, "SCRS", 4);
		insex[i-1].hmem = (BYTE)((DWORD)ofs >> 20);
		insex[i-1].memseg = (WORD)((DWORD)ofs >> 4);
		if (pSmp->pSample)
		{
			insex[i-1].type = 1;
			insex[i-1].length = pSmp->nLength;
			insex[i-1].loopbegin = pSmp->nLoopStart;
			insex[i-1].loopend = pSmp->nLoopEnd;
			insex[i-1].vol = pSmp->nVolume / 4;
			insex[i-1].flags = (pSmp->uFlags & CHN_LOOP) ? 1 : 0;
			if (pSmp->nC5Speed)
				insex[i-1].finetune = min(pSmp->nC5Speed, 0xFFFF);
			else
				insex[i-1].finetune = TransposeToFrequency(pSmp->RelativeTone, pSmp->nFineTune);
			UINT flags = RS_PCM8U;
#ifndef NO_PACKING
			if (nPacking)
			{
				if ((!(pSmp->uFlags & (CHN_16BIT|CHN_STEREO)))
				 && (CanPackSample(pSmp->pSample, pSmp->nLength, nPacking)))
				{
					insex[i-1].pack = 4;
					flags = RS_ADPCM4;
				}
			} else
#endif // NO_PACKING
			{
				if (pSmp->uFlags & CHN_16BIT)
				{
					insex[i-1].flags |= 4;
					flags = RS_PCM16U;
				}
				if (pSmp->uFlags & CHN_STEREO)
				{
					insex[i-1].flags |= 2;
					flags = (pSmp->uFlags & CHN_16BIT) ? RS_STPCM16U : RS_STPCM8U;
				}
			}
			DWORD len = WriteSample(f, pSmp, flags);
			if (len & 0x0F)
			{
				fwrite(S3MFiller, 0x10 - (len & 0x0F), 1, f);
			}
			ofs += (len + 15) & (~0x0F);
		} else
		{
			insex[i-1].length = 0;
		}
	}
	// Updating parapointers
	fseek(f, ofs0, SEEK_SET);
	fwrite(insptr, nbi, 2, f);
	fwrite(patptr, nbp, 2, f);
	fseek(f, ofs1, SEEK_SET);
	fwrite(insex, 0x50, nbi, f);
	fclose(f);
	return true;
}

#pragma warning(default:4100)
#endif // MODPLUG_NO_FILESAVE
