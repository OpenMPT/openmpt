/*
 * Purpose: Load ULT (UltraTracker) modules
 * Authors: Storlek (Original author - http://schismtracker.org/)
 *			Johannes Schultz (OpenMPT Port, tweaks)
 *
 * Thanks to Storlek for allowing me to use this code!
 */

#include "stdafx.h"
#include "sndfile.h"

enum
{
	ULT_16BIT = 4,
	ULT_LOOP  = 8,
	ULT_PINGPONGLOOP = 16,
};

#pragma pack(1)
struct ULT_SAMPLE
{
	char   name[32];
	char   filename[12];
	uint32 loop_start;
	uint32 loop_end;
	uint32 size_start;
	uint32 size_end;
	uint8  volume;		// 0-255, apparently prior to 1.4 this was logarithmic?
	uint8  flags;		// above
	uint16 speed;		// only exists for 1.4+
	int16  finetune;
};
STATIC_ASSERT(sizeof(ULT_SAMPLE) >= 64);
#pragma pack()

#define ASSERT_CAN_READ(x) \
	if( dwMemPos > dwMemLength || x > dwMemLength - dwMemPos ) return false;

/* Unhandled effects:
5x1 - do not loop sample (x is unused)
5xC - end loop and finish sample
9xx - set sample offset to xx * 1024
    with 9yy: set sample offset to xxyy * 4
E0x - set vibrato strength (2 is normal)

The logarithmic volume scale used in older format versions here, or pretty
much anywhere for that matter. I don't even think Ultra Tracker tries to
convert them. */

static const uint8 ult_efftrans[] =
{
	CMD_ARPEGGIO,
	CMD_PORTAMENTOUP,
	CMD_PORTAMENTODOWN,
	CMD_TONEPORTAMENTO,
	CMD_VIBRATO,
	CMD_NONE,
	CMD_NONE,
	CMD_TREMOLO,
	CMD_NONE,
	CMD_OFFSET,
	CMD_VOLUMESLIDE,
	CMD_PANNING8,
	CMD_VOLUME,
	CMD_PATTERNBREAK,
	CMD_NONE, // extended effects, processed separately
	CMD_SPEED,
};

static void TranslateULTCommands(uint8 *pe, uint8 *pp)
//----------------------------------------------------
{
	uint8 e = *pe & 0x0F;
	uint8 p = *pp;

	*pe = ult_efftrans[e];

	switch (e)
	{
	case 0x00:
		if (!p)
			*pe = CMD_NONE;
		break;
	case 0x05:
		// play backwards
		if((p & 0x0F) == 0x02)
		{
			*pe = CMD_S3MCMDEX;
			p = 0x9F;
		}
		break;
	case 0x0A:
		// blah, this sucks
		if (p & 0xF0)
			p &= 0xF0;
		break;
	case 0x0B:
		// mikmod does this wrong, resulting in values 0-225 instead of 0-255
		p = (p & 0x0F) * 0x11;
		break;
	case 0x0C: // volume
		p >>= 2;
		break;
	case 0x0D: // pattern break
		p = 10 * (p >> 4) + (p & 0x0F);
	case 0x0E: // special
		switch (p >> 4)
		{
		case 0x01:
			*pe = CMD_PORTAMENTOUP;
			p = 0xF0 | (p & 0x0F);
			break;
		case 0x02:
			*pe = CMD_PORTAMENTODOWN;
			p = 0xF0 | (p & 0x0F);
			break;
		case 0x08:
			*pe = CMD_S3MCMDEX;
			p = 0x60 | (p & 0x0F);
			break;
		case 0x09:
			*pe = CMD_RETRIG;
			p &= 0x0F;
			break;
		case 0x0A:
			*pe = CMD_VOLUMESLIDE;
			p = ((p & 0x0F) << 4) | 0x0F;
			break;
		case 0x0B:
			*pe = CMD_VOLUMESLIDE;
			p = 0xF0 | (p & 0x0F);
			break;
		case 0x0C: case 0x0D:
			*pe = CMD_S3MCMDEX;
			break;
		}
		break;
	case 0x0F:
		if (p > 0x2F)
			*pe = CMD_TEMPO;
		break;
	}

	*pp = p;
}

static int ReadULTEvent(MODCOMMAND *note, const BYTE *lpStream, DWORD *dwMP, const DWORD dwMemLength)
//---------------------------------------------------------------------------------------------------
{
	DWORD dwMemPos = *dwMP;
	uint8 b, repeat = 1;
	uint8 cmd1, cmd2;	// 1 = vol col, 2 = fx col in the original schismtracker code
	uint8 param1, param2;

	ASSERT_CAN_READ(1)
	b = lpStream[dwMemPos++];
	if (b == 0xFC)	// repeat event
	{
		ASSERT_CAN_READ(2);
		repeat = lpStream[dwMemPos++];
		b = lpStream[dwMemPos++];
	}
	ASSERT_CAN_READ(4)
	note->note = (b > 0 && b < 61) ? b + 36 : NOTE_NONE;
	note->instr = lpStream[dwMemPos++];
	b = lpStream[dwMemPos++];
	cmd1 = b & 0x0F;
	cmd2 = b >> 4;
	param1 = lpStream[dwMemPos++];
	param2 = lpStream[dwMemPos++];
	TranslateULTCommands(&cmd1, &param1);
	TranslateULTCommands(&cmd2, &param2);

	// sample offset -- this is even more special than digitrakker's
	if(cmd1 == CMD_OFFSET && cmd2 == CMD_OFFSET)
	{
		uint32 off = ((param1 << 8) | param2) >> 6;
		cmd1 = CMD_NONE;
		param1 = (uint8)min(off, 0xFF);
	} else if(cmd1 == CMD_OFFSET)
	{
		uint32 off = param1 * 4;
		param1 = (uint8)min(off, 0xFF);
	} else if(cmd2 == CMD_OFFSET)
	{
		uint32 off = param2 * 4;
		param2 = (uint8)min(off, 0xFF);
	} else if(cmd1 == cmd2)
	{
		// don't try to figure out how ultratracker does this, it's quite random
		cmd2 = CMD_NONE;
	}
	if (cmd2 == CMD_VOLUME || (cmd2 == CMD_NONE && cmd1 != CMD_VOLUME))
	{
		// swap commands
		std::swap(cmd1, cmd2);
		std::swap(param1, param2);
	}

	// Do that dance.
	// Maybe I should quit rewriting this everywhere and make a generic version :P
	int n;
	for (n = 0; n < 4; n++)
	{
		if(CSoundFile::ConvertVolEffect(&cmd1, &param1, (n >> 1) ? true : false))
		{
			n = 5;
			break;
		}
		std::swap(cmd1, cmd2);
		std::swap(param1, param2);
	}
	if (n < 5)
	{
		if (CSoundFile::GetEffectWeight((MODCOMMAND::COMMAND)cmd1) > CSoundFile::GetEffectWeight((MODCOMMAND::COMMAND)cmd2))
		{
			std::swap(cmd1, cmd2);
			std::swap(param1, param2);
		}
		cmd1 = CMD_NONE;
	}
	if (!cmd1)
		param1 = 0;
	if (!cmd2)
		param2 = 0;

	note->volcmd = cmd1;
	note->vol = param1;
	note->command = cmd2;
	note->param = param2;
	
	*dwMP = dwMemPos;
	return repeat;
}

// Functor for postfixing ULT patterns (this is easier than just remembering everything WHILE we're reading the pattern events)
struct PostFixUltCommands
//=======================
{
	PostFixUltCommands(CHANNELINDEX numChannels)
	{
		this->numChannels = numChannels;
		curChannel = 0;
		writeT125 = false;
		isPortaActive.resize(numChannels, false);
	}

	void operator()(MODCOMMAND& m)
	{
		// Attempt to fix portamentos.
		// UltraTracker will slide until the destination note is reached or 300 is encountered.

		// Stop porta?
		if(m.command == CMD_TONEPORTAMENTO && m.param == 0)
		{
			isPortaActive[curChannel] = false;
			m.command = CMD_NONE;
		}
		if(m.volcmd == VOLCMD_TONEPORTAMENTO && m.vol == 0)
		{
			isPortaActive[curChannel] = false;
			m.volcmd = VOLCMD_NONE;
		}

		// Apply porta?
		if(m.note == NOTE_NONE && isPortaActive[curChannel])
		{
			if(m.command == CMD_NONE && m.vol != VOLCMD_TONEPORTAMENTO)
			{
				m.command = CMD_TONEPORTAMENTO;
				m.param = 0;
			} else if(m.volcmd == VOLCMD_NONE && m.command != CMD_TONEPORTAMENTO)
			{
				m.volcmd = VOLCMD_TONEPORTAMENTO;
				m.vol = 0;
			}
		} else	// new note -> stop porta (or initialize again)
		{
			isPortaActive[curChannel] = (m.command == CMD_TONEPORTAMENTO || m.volcmd == VOLCMD_TONEPORTAMENTO);
		}

		// attempt to fix F00 (reset to tempo 125, speed 6)
		if(writeT125 && m.command == CMD_NONE)
		{
			m.command = CMD_TEMPO;
			m.param = 125;
		}
		if(m.command == CMD_SPEED && m.param == 0)
		{
			m.param = 6;
			writeT125 = true;
		}
		if(m.command == CMD_TEMPO)	// don't try to fix this anymore if the tempo has already changed.
		{
			writeT125 = false;
		}
		curChannel = (curChannel + 1) % numChannels;
	}

	vector<bool> isPortaActive;
	bool writeT125;
	CHANNELINDEX numChannels, curChannel;
};


bool CSoundFile::ReadUlt(const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
	DWORD dwMemPos = 0;
	uint8 ult_version;

	// Tracker ID
	ASSERT_CAN_READ(15);
	if (memcmp(lpStream, "MAS_UTrack_V00", 14) != 0)
		return false;
	dwMemPos += 14;
	ult_version = lpStream[dwMemPos++];
	if (ult_version < '1' || ult_version > '4')
		return false;
	ult_version -= '0';

	ASSERT_CAN_READ(32);
	memcpy(m_szNames[0], lpStream + dwMemPos, 32);
	SetNullTerminator(m_szNames[0]);
	dwMemPos += 32;

	m_nType = MOD_TYPE_ULT;
	m_dwSongFlags = SONG_ITCOMPATMODE | SONG_ITOLDEFFECTS;	// this will be converted to IT format by MPT.
	SetModFlag(MSF_COMPATIBLE_PLAY, true);

	ASSERT_CAN_READ(1);
	uint8 nNumLines = (uint8)lpStream[dwMemPos++];
	ASSERT_CAN_READ((DWORD)(nNumLines * 32));
	// read "nNumLines" lines, each containing 32 characters.
	if(m_lpszSongComments != nullptr)
		delete(m_lpszSongComments);
	m_lpszSongComments = new char[(nNumLines * 33) + 1];
	if(m_lpszSongComments)
	{
		for(size_t nLine = 0; nLine < nNumLines; nLine++)
		{
			memcpy(m_lpszSongComments + nLine * 33, lpStream + dwMemPos + nLine * 32, 32);
			m_lpszSongComments[nLine * 33 + 32] = 0x0D;
		}
		m_lpszSongComments[nNumLines * 33] = 0;
	}
	dwMemPos += nNumLines * 32;

	ASSERT_CAN_READ(1);
	m_nSamples = (SAMPLEINDEX)lpStream[dwMemPos++];
	if(m_nSamples >= MAX_SAMPLES)
		return false;

	for(SAMPLEINDEX nSmp = 0; nSmp < m_nSamples; nSmp++)
	{
		ULT_SAMPLE ultSmp;
		MODSAMPLE *pSmp = &(Samples[nSmp + 1]);
		// annoying: v4 added a field before the end of the struct
		if(ult_version >= 4)
		{
			ASSERT_CAN_READ(sizeof(ULT_SAMPLE));
			memcpy(&ultSmp, lpStream + dwMemPos, sizeof(ULT_SAMPLE));
			dwMemPos += sizeof(ULT_SAMPLE);

			ultSmp.speed = LittleEndianW(ultSmp.speed);
		} else
		{
			ASSERT_CAN_READ(sizeof(64));
			memcpy(&ultSmp, lpStream + dwMemPos, 64);
			dwMemPos += 64;

			ultSmp.finetune = ultSmp.speed;
			ultSmp.speed = 8363;
		}
		ultSmp.finetune = LittleEndianW(ultSmp.finetune);
		ultSmp.loop_start = LittleEndian(ultSmp.loop_start);
		ultSmp.loop_end = LittleEndian(ultSmp.loop_end);
		ultSmp.size_start = LittleEndian(ultSmp.size_start);
		ultSmp.size_end = LittleEndian(ultSmp.size_end);

		memcpy(m_szNames[nSmp + 1], ultSmp.name, 32);
		SetNullTerminator(m_szNames[nSmp + 1]);
		memcpy(pSmp->filename, ultSmp.filename, 12);
		SpaceToNullStringFixed(pSmp->filename, 12);

		if(ultSmp.size_end <= ultSmp.size_start)
			continue;
		pSmp->nLength = ultSmp.size_end - ultSmp.size_start;
		pSmp->nLoopStart = ultSmp.loop_start;
		pSmp->nLoopEnd = min(ultSmp.loop_end, pSmp->nLength);
		pSmp->nVolume = ultSmp.volume;
		pSmp->nGlobalVol = 64;

		/* mikmod does some weird integer math here, but it didn't really work for me */
		pSmp->nC5Speed = ultSmp.speed;
		if(ultSmp.finetune)
		{
			pSmp->nC5Speed = (UINT)(((double)pSmp->nC5Speed) * pow(2, (((double)ultSmp.finetune) / (12.0 * 32768))));
		}

		if(ultSmp.flags & ULT_LOOP)
			pSmp->uFlags |= CHN_LOOP;
		if(ultSmp.flags & ULT_PINGPONGLOOP)
			pSmp->uFlags |= CHN_PINGPONGLOOP;
		if(ultSmp.flags & ULT_16BIT)
		{
			pSmp->uFlags |= CHN_16BIT;
			pSmp->nLoopStart >>= 1;
			pSmp->nLoopEnd >>= 1;
		}
	}

	// ult just so happens to use 255 for its end mark, so there's no need to fiddle with this
	Order.ReadAsByte(lpStream + dwMemPos, 256, dwMemLength - dwMemPos);
	dwMemPos += 256;

	ASSERT_CAN_READ(2);
	m_nChannels = lpStream[dwMemPos++] + 1;
	PATTERNINDEX nNumPats = lpStream[dwMemPos++] + 1;

	if(m_nChannels > MAX_BASECHANNELS || nNumPats > MAX_PATTERNS)
		return false;

	if(ult_version >= 3)
	{
		ASSERT_CAN_READ(m_nChannels);
		for(CHANNELINDEX nChn = 0; nChn < m_nChannels; nChn++)
		{
			ChnSettings[nChn].nPan = ((lpStream[dwMemPos + nChn] & 0x0F) << 4) + 8;
		}
		dwMemPos += m_nChannels;
	} else
	{
		for(CHANNELINDEX nChn = 0; nChn < m_nChannels; nChn++)
		{
			ChnSettings[nChn].nPan = (nChn & 1) ? 192 : 64;
		}
	}

	for(PATTERNINDEX nPat = 0; nPat < nNumPats; nPat++)
	{
		if(Patterns.Insert(nPat, 64))
			return false;
	}

	for(CHANNELINDEX nChn = 0; nChn < m_nChannels; nChn++)
	{
		MODCOMMAND evnote;
		MODCOMMAND *note;
		int repeat;
		evnote.Clear();

		for(PATTERNINDEX nPat = 0; nPat < nNumPats; nPat++)
		{
			note = Patterns[nPat] + nChn;
			ROWINDEX nRow = 0;
			while(nRow < 64)
			{
				repeat = ReadULTEvent(&evnote, lpStream, &dwMemPos, dwMemLength);
				if(repeat + nRow > 64)
					repeat = 64 - nRow;
				if(repeat == 0) break;
				while (repeat--)
				{
					*note = evnote;
					note += m_nChannels;
					nRow++;
				}
			}
		}
	}

	// Post-fix some effects.
	Patterns.ForEachModCommand(PostFixUltCommands(m_nChannels));

	for(SAMPLEINDEX nSmp = 0; nSmp < m_nSamples; nSmp++)
	{
		dwMemPos += ReadSample(&Samples[nSmp + 1], (Samples[nSmp + 1].uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S, (LPCSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
	}
	return true;
}

#undef ASSERT_CAN_READ
