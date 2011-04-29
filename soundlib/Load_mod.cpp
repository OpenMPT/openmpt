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

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

extern WORD ProTrackerPeriodTable[6*12];

//////////////////////////////////////////////////////////
// ProTracker / NoiseTracker MOD/NST file support

void CSoundFile::ConvertModCommand(MODCOMMAND *m) const
//-----------------------------------------------------
{
	UINT command = m->command, param = m->param;

	switch(command)
	{
	case 0x00:	if (param) command = CMD_ARPEGGIO; break;
	case 0x01:	command = CMD_PORTAMENTOUP; break;
	case 0x02:	command = CMD_PORTAMENTODOWN; break;
	case 0x03:	command = CMD_TONEPORTAMENTO; break;
	case 0x04:	command = CMD_VIBRATO; break;
	case 0x05:	command = CMD_TONEPORTAVOL; if (param & 0xF0) param &= 0xF0; break;
	case 0x06:	command = CMD_VIBRATOVOL; if (param & 0xF0) param &= 0xF0; break;
	case 0x07:	command = CMD_TREMOLO; break;
	case 0x08:	command = CMD_PANNING8; break;
	case 0x09:	command = CMD_OFFSET; break;
	case 0x0A:	command = CMD_VOLUMESLIDE; if (param & 0xF0) param &= 0xF0; break;
	case 0x0B:	command = CMD_POSITIONJUMP; break;
	case 0x0C:	command = CMD_VOLUME; break;
	case 0x0D:	command = CMD_PATTERNBREAK; param = ((param >> 4) * 10) + (param & 0x0F); break;
	case 0x0E:	command = CMD_MODCMDEX; break;
	case 0x0F:	command = (param <= ((m_nType & (MOD_TYPE_MOD)) ? 0x20 : 0x1F)) ? CMD_SPEED : CMD_TEMPO;
				if ((param == 0xFF) && (m_nSamples == 15) && (m_nType & MOD_TYPE_MOD)) command = 0; break; //<rewbs> what the hell is this?! :) //<jojo> it's the "stop tune" command! :-P
	// Extension for XM extended effects
	case 'G' - 55:	command = CMD_GLOBALVOLUME; break;		//16
	case 'H' - 55:	command = CMD_GLOBALVOLSLIDE; if (param & 0xF0) param &= 0xF0; break;
	case 'K' - 55:	command = CMD_KEYOFF; break;
	case 'L' - 55:	command = CMD_SETENVPOSITION; break;
	case 'M' - 55:	command = CMD_CHANNELVOLUME; break;
	case 'N' - 55:	command = CMD_CHANNELVOLSLIDE; break;
	case 'P' - 55:	command = CMD_PANNINGSLIDE; if (param & 0xF0) param &= 0xF0; break;
	case 'R' - 55:	command = CMD_RETRIG; break;
	case 'T' - 55:	command = CMD_TREMOR; break;
	case 'X' - 55:	command = CMD_XFINEPORTAUPDOWN;	break;
	case 'Y' - 55:	command = CMD_PANBRELLO; break;			//34
	case 'Z' - 55:	command = CMD_MIDI;	break;				//35
	case '\\' - 56:	command = CMD_SMOOTHMIDI;	break;		//rewbs.smoothVST: 36 - note: this is actually displayed as "-" in FT2, but seems to be doing nothing.
	//case ':' - 21:	command = CMD_DELAYCUT;	break;		//37
	case '#' + 3:	command = CMD_XPARAM;	break;			//rewbs.XMfixes - XParam is 38
	default:		command = CMD_NONE;
	}
	m->command = command;
	m->param = param;
}


WORD CSoundFile::ModSaveCommand(const MODCOMMAND *m, const bool bXM, const bool bCompatibilityExport) const
//---------------------------------------------------------------------------------------------------------
{
	UINT command = m->command, param = m->param;

	switch(command)
	{
	case CMD_NONE:				command = param = 0; break;
	case CMD_ARPEGGIO:			command = 0; break;
	case CMD_PORTAMENTOUP:
		if (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_STM|MOD_TYPE_MPT))
		{
			if ((param & 0xF0) == 0xE0) { command = 0x0E; param = ((param & 0x0F) >> 2) | 0x10; break; }
			else if ((param & 0xF0) == 0xF0) { command = 0x0E; param &= 0x0F; param |= 0x10; break; }
		}
		command = 0x01;
		break;
	case CMD_PORTAMENTODOWN:
		if (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_STM|MOD_TYPE_MPT))
		{
			if ((param & 0xF0) == 0xE0) { command = 0x0E; param= ((param & 0x0F) >> 2) | 0x20; break; }
			else if ((param & 0xF0) == 0xF0) { command = 0x0E; param &= 0x0F; param |= 0x20; break; }
		}
		command = 0x02;
		break;
	case CMD_TONEPORTAMENTO:	command = 0x03; break;
	case CMD_VIBRATO:			command = 0x04; break;
	case CMD_TONEPORTAVOL:		command = 0x05; break;
	case CMD_VIBRATOVOL:		command = 0x06; break;
	case CMD_TREMOLO:			command = 0x07; break;
	case CMD_PANNING8:			
		command = 0x08;
		if(m_nType & MOD_TYPE_S3M)
		{
			if(param <= 0x80)
			{
				param = min(param << 1, 0xFF);
			}
			else if(param == 0xA4)	// surround
			{
				if(bCompatibilityExport || !bXM)
				{
					command = param = 0;
				}
				else
				{
					command = 'X' - 55;
					param = 91;
				}
			}
		}
		break;
	case CMD_OFFSET:			command = 0x09; break;
	case CMD_VOLUMESLIDE:		command = 0x0A; break;
	case CMD_POSITIONJUMP:		command = 0x0B; break;
	case CMD_VOLUME:			command = 0x0C; break;
	case CMD_PATTERNBREAK:		command = 0x0D; param = ((param / 10) << 4) | (param % 10); break;
	case CMD_MODCMDEX:			command = 0x0E; break;
	case CMD_SPEED:				command = 0x0F; param = min(param, (bXM) ? 0x1F : 0x20); break;
	case CMD_TEMPO:				command = 0x0F; param = max(param, (bXM) ? 0x20 : 0x21); break;
	case CMD_GLOBALVOLUME:		command = 'G' - 55; break;
	case CMD_GLOBALVOLSLIDE:	command = 'H' - 55; break;
	case CMD_KEYOFF:			command = 'K' - 55; break;
	case CMD_SETENVPOSITION:	command = 'L' - 55; break;
	case CMD_CHANNELVOLUME:		command = 'M' - 55; break;
	case CMD_CHANNELVOLSLIDE:	command = 'N' - 55; break;
	case CMD_PANNINGSLIDE:		command = 'P' - 55; break;
	case CMD_RETRIG:			command = 'R' - 55; break;
	case CMD_TREMOR:			command = 'T' - 55; break;
	case CMD_XFINEPORTAUPDOWN:
		if(bCompatibilityExport && (param & 0xF0) > 2)	// X1x and X2x are legit, everything above are MPT extensions, which don't belong here.
			command = param = 0;
		else
			command = 'X' - 55;
		break;
	case CMD_PANBRELLO:
		if(bCompatibilityExport)
			command = param = 0;
		else
			command = 'Y' - 55;
		break;
	case CMD_MIDI:				
		if(bCompatibilityExport)
			command = param = 0;
		else
			command = 'Z' - 55;
		break;
	case CMD_SMOOTHMIDI: //rewbs.smoothVST: 36
		if(bCompatibilityExport)
			command = param = 0;
		else
			command = '\\' - 56;
		break;
	case CMD_XPARAM: //rewbs.XMfixes - XParam is 38
		if(bCompatibilityExport)
			command = param = 0;
		else
			command = '#' + 3;
		break;
	case CMD_S3MCMDEX:
		switch(param & 0xF0)
		{
		case 0x10:	command = 0x0E; param = (param & 0x0F) | 0x30; break;
		case 0x20:	command = 0x0E; param = (param & 0x0F) | 0x50; break;
		case 0x30:	command = 0x0E; param = (param & 0x0F) | 0x40; break;
		case 0x40:	command = 0x0E; param = (param & 0x0F) | 0x70; break;
		case 0x90:  
			if(bCompatibilityExport)
				command = param = 0;
			else
				command = 'X' - 55;
			break;
		case 0xB0:	command = 0x0E; param = (param & 0x0F) | 0x60; break;
		case 0xA0:
		case 0x50:
		case 0x70:
		case 0x60:	command = param = 0; break;
		default:	command = 0x0E; break;
		}
		break;
	default:
		command = param = 0;
	}

	// don't even think about saving XM effects in MODs...
	if(command > 0x0F && !bXM)
		command = param = 0;

	return (WORD)((command << 8) | (param));
}


#pragma pack(1)

typedef struct _MODSAMPLEHEADER
{
	CHAR name[22];
	WORD length;
	BYTE finetune;
	BYTE volume;
	WORD loopstart;
	WORD looplen;
} MODSAMPLEHEADER, *PMODSAMPLEHEADER;

typedef struct _MODMAGIC
{
	BYTE nOrders;
	BYTE nRestartPos;
	BYTE Orders[128];
    char Magic[4];
} MODMAGIC, *PMODMAGIC;

#pragma pack()

bool IsMagic(LPCSTR s1, LPCSTR s2)
{
	return ((*(DWORD *)s1) == (*(DWORD *)s2)) ? true : false;
}

// Functor for fixing VBlank MODs and MODs with 7-bit panning
struct FixMODPatterns
//===================
{
	FixMODPatterns(bool bVBlank, bool bPanning)
	{
		this->bVBlank = bVBlank;
		this->bPanning = bPanning;
	}

	void operator()(MODCOMMAND& m)
	{
		// Fix VBlank MODs
		if(m.command == CMD_TEMPO && this->bVBlank)
		{
			m.command = CMD_SPEED;
		}
		// Fix MODs with 7-bit + surround panning
		if(m.command == CMD_PANNING8 && this->bPanning)
		{
			if(m.param == 0xA4)
			{
				m.command = CMD_S3MCMDEX;
				m.param = 0x91;
			} else
			{
				m.param = min(m.param * 2, 0xFF);
			}
		}
	}

	bool bVBlank, bPanning;
};


bool CSoundFile::ReadMod(const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
    char s[1024];
	DWORD dwMemPos, dwTotalSampleLen;
	PMODMAGIC pMagic;
	UINT nErr;

	if ((!lpStream) || (dwMemLength < 0x600)) return false;
	dwMemPos = 20;
	m_nSamples = 31;
	m_nChannels = 4;
	pMagic = (PMODMAGIC)(lpStream + dwMemPos + sizeof(MODSAMPLEHEADER) * 31);
	// Check Mod Magic
	memcpy(s, pMagic->Magic, 4);
	if ((IsMagic(s, "M.K.")) || (IsMagic(s, "M!K!"))
	 || (IsMagic(s, "M&K!")) || (IsMagic(s, "N.T.")) || (IsMagic(s, "FEST"))) m_nChannels = 4; else
	if ((IsMagic(s, "CD81")) || (IsMagic(s, "OKTA"))) m_nChannels = 8; else
	if ((s[0]=='F') && (s[1]=='L') && (s[2]=='T') && (s[3]>='4') && (s[3]<='9')) m_nChannels = s[3] - '0'; else
	if ((s[0]>='4') && (s[0]<='9') && (s[1]=='C') && (s[2]=='H') && (s[3]=='N')) m_nChannels = s[0] - '0'; else
	if ((s[0]=='1') && (s[1]>='0') && (s[1]<='9') && (s[2]=='C') && (s[3]=='H')) m_nChannels = s[1] - '0' + 10; else
	if ((s[0]=='2') && (s[1]>='0') && (s[1]<='9') && (s[2]=='C') && (s[3]=='H')) m_nChannels = s[1] - '0' + 20; else
	if ((s[0]=='3') && (s[1]>='0') && (s[1]<='2') && (s[2]=='C') && (s[3]=='H')) m_nChannels = s[1] - '0' + 30; else
	if ((s[0]=='T') && (s[1]=='D') && (s[2]=='Z') && (s[3]>='4') && (s[3]<='9')) m_nChannels = s[3] - '0'; else
	if (IsMagic(s,"16CN")) m_nChannels = 16; else
	if (IsMagic(s,"32CN")) m_nChannels = 32; else m_nSamples = 15;
	// Startrekker 8 channel mod (needs special treatment, see below)
	bool bFLT8 = IsMagic(s, "FLT8") ? true : false;
	// Only apply VBlank tests to M.K. (ProTracker) modules.
	const bool bMdKd = IsMagic(s, "M.K.") ? true : false;

	// Load Samples
	nErr = 0;
	dwTotalSampleLen = 0;
	for	(UINT i=1; i<=m_nSamples; i++)
	{
		PMODSAMPLEHEADER pms = (PMODSAMPLEHEADER)(lpStream+dwMemPos);
		MODSAMPLE *psmp = &Samples[i];
		UINT loopstart, looplen;

		memcpy(m_szNames[i], pms->name, 22);
		SpaceToNullStringFixed<22>(m_szNames[i]);
		psmp->uFlags = 0;
		psmp->nLength = BigEndianW(pms->length)*2;
		dwTotalSampleLen += psmp->nLength;
		psmp->nFineTune = MOD2XMFineTune(pms->finetune & 0x0F);
		psmp->nVolume = 4*pms->volume;
		if (psmp->nVolume > 256) { psmp->nVolume = 256; nErr++; }
		psmp->nGlobalVol = 64;
		psmp->nPan = 128;
		loopstart = BigEndianW(pms->loopstart)*2;
		looplen = BigEndianW(pms->looplen)*2;
		// Fix loops
		if ((looplen > 2) && (loopstart+looplen > psmp->nLength)
		 && (loopstart/2+looplen <= psmp->nLength))
		{
			loopstart /= 2;
		}
		psmp->nLoopStart = loopstart;
		psmp->nLoopEnd = loopstart + looplen;
		if (psmp->nLength < 4) psmp->nLength = 0;
		if (psmp->nLength)
		{
			UINT derr = 0;
			if (psmp->nLoopStart >= psmp->nLength) { psmp->nLoopStart = psmp->nLength-1; derr|=1; }
			if (psmp->nLoopEnd > psmp->nLength) { psmp->nLoopEnd = psmp->nLength; derr |= 1; }
			if (psmp->nLoopStart > psmp->nLoopEnd) derr |= 1;
			if ((psmp->nLoopStart > psmp->nLoopEnd) || (psmp->nLoopEnd < 4)
			 || (psmp->nLoopEnd - psmp->nLoopStart < 4))
			{
				psmp->nLoopStart = 0;
				psmp->nLoopEnd = 0;
			}
			// Fix for most likely broken sample loops. This fixes super_sufm_-_new_life.mod which has a long sample which is looped from 0 to 4.
			if(psmp->nLoopEnd <= 8 && psmp->nLoopStart == 0 && psmp->nLength > psmp->nLoopEnd)
			{
				psmp->nLoopEnd = 0;
			}
			if (psmp->nLoopEnd > psmp->nLoopStart)
			{
				psmp->uFlags |= CHN_LOOP;
			}
		}
		dwMemPos += sizeof(MODSAMPLEHEADER);
	}
	if ((m_nSamples == 15) && (dwTotalSampleLen > dwMemLength * 4)) return false;
	pMagic = (PMODMAGIC)(lpStream+dwMemPos);
	dwMemPos += sizeof(MODMAGIC);
	if (m_nSamples == 15) dwMemPos -= 4;
	Order.ReadAsByte(pMagic->Orders, 128, 128);

	UINT nbp, nbpbuggy, nbpbuggy2, norders;

	norders = pMagic->nOrders;
	if ((!norders) || (norders > 0x80))
	{
		norders = 0x80;
		while ((norders > 1) && (!Order[norders-1])) norders--;
	}
	nbpbuggy = 0;
	nbpbuggy2 = 0;
	nbp = 0;
	for (UINT iord=0; iord<128; iord++)
	{
		UINT i = Order[iord];
		if ((i < 0x80) && (nbp <= i))
		{
			nbp = i+1;
			if (iord<norders) nbpbuggy = nbp;
		}
		if (i >= nbpbuggy2) nbpbuggy2 = i+1;

		// from mikmod: if the file says FLT8, but the orderlist has odd numbers, it's probably really an FLT4
		if(bFLT8 && (Order[iord] & 1))
		{
			m_nChannels = 4;
			bFLT8 = false;
		}

		// chances are very high that we're dealing with a non-MOD file here.
		if(m_nSamples == 15 && i >= 0x80)
			return false;
	}

	if(bFLT8)
	{
		// FLT8 has only equal order items, so divide by two.
		for(ORDERINDEX nOrd = 0; nOrd < Order.GetLength(); nOrd++)
			Order[nOrd] >>= 1;
	}

	for(UINT iend = norders; iend < 0x80; iend++) Order[iend] = Order.GetInvalidPatIndex();

	norders--;
	m_nRestartPos = pMagic->nRestartPos;
	if (m_nRestartPos >= 0x78) m_nRestartPos = 0;
	if (m_nRestartPos + 1 >= (UINT)norders) m_nRestartPos = 0;
	if (!nbp) return false;
	DWORD dwWowTest = dwTotalSampleLen+dwMemPos;
	if ((IsMagic(pMagic->Magic, "M.K.")) && (dwWowTest + nbp*8*256 == dwMemLength)) m_nChannels = 8;
	if ((nbp != nbpbuggy) && (dwWowTest + nbp*m_nChannels*256 != dwMemLength))
	{
		if (dwWowTest + nbpbuggy*m_nChannels*256 == dwMemLength) nbp = nbpbuggy;
		else nErr += 8;
	} else
	if ((nbpbuggy2 > nbp) && (dwWowTest + nbpbuggy2*m_nChannels*256 == dwMemLength))
	{
		nbp = nbpbuggy2;
	}
	if ((dwWowTest < 0x600) || (dwWowTest > dwMemLength)) nErr += 8;
	if ((m_nSamples == 15) && (nErr >= 16)) return false;
	// Default settings	
	m_nType = MOD_TYPE_MOD;
	m_nDefaultSpeed = 6;
	m_nDefaultTempo = 125;
	m_nMinPeriod = 14 << 2;
	m_nMaxPeriod = 3424 << 2;
	memcpy(m_szNames[0], lpStream, 20);
	SpaceToNullStringFixed<20>(m_szNames[0]);
	// Setup channel pan positions and volume
	SetupMODPanning();

	const CHANNELINDEX nMaxChn = (bFLT8) ? 4 : m_nChannels; // 4 channels per pattern in FLT8 format.
	if(bFLT8) nbp++; // as one logical pattern consists of two real patterns in FLT8 format, the highest pattern number has to be increased by one.
	bool bHasTempoCommands = false;	// for detecting VBlank MODs
	bool bLeftPanning = false, bExtendedPanning = false;	// for detecting 800-880 panning

	// Reading patterns
	for (PATTERNINDEX ipat = 0; ipat < nbp; ipat++)
	{
		if (ipat < MAX_PATTERNS)
		{
			if (dwMemPos + nMaxChn * 256 > dwMemLength) break;

			MODCOMMAND *m;
			if(bFLT8)
			{
				if((ipat & 1) == 0)
				{
					// only create "even" patterns
					if(Patterns.Insert(ipat >> 1, 64)) break;
				}
				m = Patterns[ipat >> 1];
			} else
			{
				if(Patterns.Insert(ipat, 64)) break;
				m = Patterns[ipat];
			}

			size_t instrWithoutNoteCount = 0;	// For detecting PT1x mode
			vector<MODCOMMAND::INSTR> lastInstrument(m_nChannels, 0);

			const BYTE *p = lpStream + dwMemPos;

			for(ROWINDEX nRow = 0; nRow < 64; nRow++)
			{
				if(bFLT8)
				{
					// FLT8: either write to channel 1 to 4 (even patterns) or 5 to 8 (odd patterns).
					m = Patterns[ipat >> 1] + nRow * 8 + ((ipat & 1) ? 4 : 0);
				}
				for(CHANNELINDEX nChn = 0; nChn < nMaxChn; nChn++, m++, p += 4)
				{
					BYTE A0 = p[0], A1 = p[1], A2 = p[2], A3 = p[3];
					UINT n = ((((UINT)A0 & 0x0F) << 8) | (A1));
					if ((n) && (n != 0xFFF)) m->note = GetNoteFromPeriod(n << 2);
					m->instr = ((UINT)A2 >> 4) | (A0 & 0x10);
					m->command = A2 & 0x0F;
					m->param = A3;
					if ((m->command) || (m->param)) ConvertModCommand(m);

					if (m->command == CMD_TEMPO && m->param < 100)
						bHasTempoCommands = true;
					if (m->command == CMD_PANNING8 && m->param < 0x80)
						bLeftPanning = true;
					if (m->command == CMD_PANNING8 && m->param > 0x80 && m->param != 0xA4)
						bExtendedPanning = true;
					if (m->note == NOTE_NONE && m->instr > 0 && !bFLT8)
					{
						if(lastInstrument[nChn] > 0 && lastInstrument[nChn] != m->instr)
						{
							instrWithoutNoteCount++;
						}
					}
					if (m->instr != 0)
					{
						lastInstrument[nChn] = m->instr;
					}
				}
			}
			// Arbitrary thershold for going into PT1x mode: 16 "sample swaps" in one pattern.
			if(instrWithoutNoteCount > 16)
			{
				m_dwSongFlags |= SONG_PT1XMODE;
			}
		}
		dwMemPos += nMaxChn * 256;
	}

	// Reading samples
	bool bSamplesPresent = false;
	for (UINT ismp = 1; ismp <= m_nSamples; ismp++) if (Samples[ismp].nLength)
	{
		LPSTR p = (LPSTR)(lpStream + dwMemPos);
		UINT flags = 0;
		if (dwMemPos + 5 <= dwMemLength)
		{
			if (!_strnicmp(p, "ADPCM", 5))
			{
				flags = 3;
				p += 5;
				dwMemPos += 5;
			}
		}
		DWORD dwSize = ReadSample(&Samples[ismp], flags, p, dwMemLength - dwMemPos);
		if (dwSize)
		{
			dwMemPos += dwSize;
			bSamplesPresent = true;
		}
	}

	// Fix VBlank MODs. Arbitrary threshold: 10 minutes.
	// Basically, this just converts all tempo commands into speed commands
	// for MODs which are supposed to have VBlank timing (instead of CIA timing).
	// There is no perfect way to do this, since both MOD types look the same,
	// but the most reliable way is to simply check for extremely long songs
	// (as this would indicate that f.e. a F30 command was really meant to set
	// the ticks per row to 48, and not the tempo to 48 BPM).
	// In the pattern loader above, a second condition is used: Only tempo commands
	// below 100 BPM are taken into account. Furthermore, only M.K. (ProTracker)
	// modules are checked.
	// The same check is also applied to original Ultimate Soundtracker 15 sample mods.
	const bool bVBlank = ((bMdKd && bHasTempoCommands && GetSongTime() >= 10 * 60) || m_nSamples == 15) ? true : false;
	const bool b7BitPanning = bLeftPanning && !bExtendedPanning;
	if(bVBlank || b7BitPanning)
	{
		Patterns.ForEachModCommand(FixMODPatterns(bVBlank, b7BitPanning));
	}

#ifdef MODPLUG_TRACKER
	return true;
#else
	return bSamplesPresent;
#endif	// MODPLUG_TRACKER
}


#ifndef MODPLUG_NO_FILESAVE

#ifdef MODPLUG_TRACKER
#include "../mptrack/moddoc.h"
#endif	// MODPLUG_TRACKER

bool CSoundFile::SaveMod(LPCSTR lpszFileName, UINT nPacking, const bool bCompatibilityExport)
//-------------------------------------------------------------------------------------------
{
	BYTE insmap[32];
	UINT inslen[32];
	BYTE bTab[32];
	BYTE ord[128];
	FILE *f;

	if ((!m_nChannels) || (!lpszFileName)) return false;
	if ((f = fopen(lpszFileName, "wb")) == NULL) return false;
	memset(ord, 0, sizeof(ord));
	memset(inslen, 0, sizeof(inslen));
	if (m_nInstruments)
	{
		memset(insmap, 0, sizeof(insmap));
		for (UINT i=1; i<32; i++) if (Instruments[i])
		{
			for (UINT j=0; j<128; j++) if (Instruments[i]->Keyboard[j])
			{
				insmap[i] = Instruments[i]->Keyboard[j];
				break;
			}
		}
	} else
	{
		for (UINT i=0; i<32; i++) insmap[i] = (BYTE)i;
	}
	// Writing song name
	fwrite(m_szNames, 20, 1, f);
	// Writing instrument definition
	for (UINT iins=1; iins<=31; iins++)
	{
		MODSAMPLE *pSmp = &Samples[insmap[iins]];
		memcpy(bTab, m_szNames[iins],22);
		inslen[iins] = pSmp->nLength;
		// if the sample size is odd, we have to add a padding byte, as all sample sizes in MODs are even.
		if(inslen[iins] & 1)
			inslen[iins]++;
		if (inslen[iins] > 0x1fff0) inslen[iins] = 0x1fff0;
		bTab[22] = inslen[iins] >> 9;
		bTab[23] = inslen[iins] >> 1;
		if (pSmp->RelativeTone < 0) bTab[24] = 0x08; else
		if (pSmp->RelativeTone > 0) bTab[24] = 0x07; else
		bTab[24] = (BYTE)XM2MODFineTune(pSmp->nFineTune);
		bTab[25] = pSmp->nVolume >> 2;
		UINT repstart = 0, replen = 2;
		if(pSmp->uFlags & CHN_LOOP)
		{
			repstart = pSmp->nLoopStart;
			replen = pSmp->nLoopEnd - pSmp->nLoopStart;
		}
		bTab[26] = repstart >> 9;
		bTab[27] = repstart >> 1;
		if(replen < 2) // ensure PT will load it properly
			replen = 2; 
		bTab[28] = replen >> 9;
		bTab[29] = replen >> 1;
		fwrite(bTab, 30, 1, f);
	}
	// Writing number of patterns
	UINT nbp = 0, norders = 128;
	for (UINT iord=0; iord<128; iord++)
	{
		if (Order[iord] == Order.GetInvalidPatIndex() || Order[iord] == Order.GetIgnoreIndex())
		{
			norders = iord;
			break;
		}
		if ((Order[iord] < 0x80) && (nbp <= Order[iord])) nbp = Order[iord] + 1;
	}
	bTab[0] = norders;
	bTab[1] = m_nRestartPos;
	fwrite(bTab, 2, 1, f);
	// Writing pattern list
	if (norders) Order.WriteToByteArray(ord, norders, 128);

	fwrite(ord, 128, 1, f);
	// Writing signature
	if (m_nChannels == 4)
	{
		if(nbp < 64)
			lstrcpy((LPSTR)&bTab, "M.K.");
		else	// more than 64 patterns
			lstrcpy((LPSTR)&bTab, "M!K!");
	} else
	{
		sprintf((LPSTR)&bTab, "%luCHN", m_nChannels);
	}
	fwrite(bTab, 4, 1, f);
	// Writing patterns
	for (UINT ipat=0; ipat<nbp; ipat++)		//for all patterns
	{
		BYTE s[64*4];
		if (Patterns[ipat])					//if pattern exists
		{
			MODCOMMAND *m = Patterns[ipat];
			for (UINT i=0; i<64; i++) {				//for all rows 
				if (i < Patterns[ipat].GetNumRows()) {			//if row exists
					LPBYTE p=s;
					for (UINT c=0; c<m_nChannels; c++,p+=4,m++)
					{
						UINT param = ModSaveCommand(m, false, true);
						UINT command = param >> 8;
						param &= 0xFF;
						if (command > 0x0F) command = param = 0;
						if ((m->vol >= 0x10) && (m->vol <= 0x50) && (!command) && (!param)) { command = 0x0C; param = m->vol - 0x10; }
						UINT period = m->note;
						if (period)
						{
							if (period < 37) period = 37;
							period -= 37;
							if (period >= 6*12) period = 6*12-1;
							period = ProTrackerPeriodTable[period];
						}
						UINT instr = (m->instr > 31) ? 0 : m->instr;
						p[0] = ((period >> 8) & 0x0F) | (instr & 0x10);
						p[1] = period & 0xFF;
						p[2] = ((instr & 0x0F) << 4) | (command & 0x0F);
						p[3] = param;
					}
					fwrite(s, m_nChannels, 4, f);
				} else {								//if row does not exist
					memset(s, 0, m_nChannels*4);		//invent blank row
					fwrite(s, m_nChannels, 4, f);
				}
			}										//end for all rows
		} else	{								
			memset(s, 0, m_nChannels*4);		//if pattern does not exist
			for (UINT i=0; i<64; i++) {			//invent blank pattern
				fwrite(s, m_nChannels, 4, f);
			}
		}
	}										//end for all patterns
	
	//Check for unsaved patterns
#ifdef MODPLUG_TRACKER
	if(GetpModDoc() != nullptr)
	{
		for(UINT ipat = nbp; ipat < MAX_PATTERNS; ipat++)
		{
			if(Patterns[ipat])
			{
				GetpModDoc()->AddToLog(_T("Warning: This track contains at least one pattern after the highest pattern number referred to in the sequence. Such patterns are not saved in the .mod format.\n"));
				break;
			}
		}
	}
#endif

	// Writing instruments
	for (UINT ismpd = 1; ismpd <= 31; ismpd++) if (inslen[ismpd])
	{
		MODSAMPLE *pSmp = &Samples[insmap[ismpd]];
		if(bCompatibilityExport == true) // first two bytes have to be 0 due to PT's one-shot loop ("no loop")
		{
			int iOverwriteLen = 2 * pSmp->GetBytesPerSample();
			memset(pSmp->pSample, 0, min(iOverwriteLen, pSmp->GetSampleSizeInBytes()));
		}
		UINT flags = RS_PCM8S;
#ifndef NO_PACKING
		if (!(pSmp->uFlags & (CHN_16BIT|CHN_STEREO)))
		{
			if ((nPacking) && (CanPackSample(pSmp->pSample, inslen[ismpd], nPacking)))
			{
				fwrite("ADPCM", 1, 5, f);
				flags = RS_ADPCM4;
			}
		}
#endif
		WriteSample(f, pSmp, flags, inslen[ismpd]);
		// write padding byte if the sample size is odd.
		if((pSmp->nLength & 1) && !nPacking)
		{
			int8 padding = 0;
			fwrite(&padding, 1, 1, f);
		}
	}
	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE
