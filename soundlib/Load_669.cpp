/*
 * Load_669.cpp
 * ------------
 * Purpose: 669 Composer / UNIS 669 module loader
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          Adam Goode (endian and char fixes for PPC)
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#pragma pack(push, 1)

struct _669FileHeader
{
	uint16 sig;					// 'if' or 'JN'
	char   songmessage[108];	// Song Message
	uint8  samples;				// number of samples (1-64)
	uint8  patterns;			// number of patterns (1-128)
	uint8  restartpos;
	uint8  orders[128];
	uint8  tempolist[128];
	uint8  breaks[128];
};

STATIC_ASSERT(sizeof(_669FileHeader) == 497);

struct _669Sample
{
	char   filename[13];
	uint32 length;	// when will somebody think about DWORD align ???
	uint32 loopStart;
	uint32 loopEnd;
};

STATIC_ASSERT(sizeof(_669Sample) == 25);

#pragma pack(pop)


bool CSoundFile::Read669(FileReader &file)
//----------------------------------------
{
	bool has669Ext;
	_669FileHeader fileHeader;
	
	file.Rewind();
	if(!file.Read(fileHeader))
	{
		return false;
	}

	if(fileHeader.sig != LittleEndianW(0x6669) && fileHeader.sig != LittleEndianW(0x4E4A))
	{
		return false;
	}

	has669Ext = fileHeader.sig == LittleEndianW(0x4E4A);
	if(fileHeader.samples > 64 || fileHeader.restartpos >= 128
		|| fileHeader.patterns > 128)
	{
		return false;
	}

	m_nType = MOD_TYPE_669;
	m_dwSongFlags |= SONG_LINEARSLIDES;
	m_nMinPeriod = 28 << 2;
	m_nMaxPeriod = 1712 << 3;
	m_nDefaultTempo = 125;
	m_nDefaultSpeed = 6;
	m_nChannels = 8;

	// Copy first song message line into song title
	StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[0], fileHeader.songmessage, 36);

	m_nSamples = fileHeader.samples;
	for(SAMPLEINDEX nSmp = 1; nSmp <= m_nSamples; nSmp++)
	{
		_669Sample sample;
		if(!file.Read(sample))
		{
			return false;
		}

		DWORD len = LittleEndian(sample.length);
		DWORD loopstart = LittleEndian(sample.loopStart);
		DWORD loopend = LittleEndian(sample.loopEnd);
		if (len > MAX_SAMPLE_LENGTH) len = MAX_SAMPLE_LENGTH;
		if ((loopend > len) && (!loopstart)) loopend = 0;
		if (loopend > len) loopend = len;
		if (loopstart + 4 >= loopend) loopstart = loopend = 0;
		Samples[nSmp].nLength = len;
		Samples[nSmp].nLoopStart = loopstart;
		Samples[nSmp].nLoopEnd = loopend;
		if (loopend) Samples[nSmp].uFlags |= CHN_LOOP;
		StringFixer::ReadString<StringFixer::nullTerminated>(m_szNames[nSmp], sample.filename);
		Samples[nSmp].nVolume = 256;
		Samples[nSmp].nGlobalVol = 64;
		Samples[nSmp].nPan = 128;
	}

	// Song Message
	ReadFixedLineLengthMessage(file, 108, 36, 0);

	// Reading Orders
	Order.ReadAsByte(fileHeader.orders, 128, 128);
	m_nRestartPos = fileHeader.restartpos;
	if(Order[m_nRestartPos] >= fileHeader.patterns) m_nRestartPos = 0;
	// Reading Pattern Break Locations
	for (UINT npan=0; npan<8; npan++)
	{
		ChnSettings[npan].nPan = (npan & 1) ? 0x30 : 0xD0;
		ChnSettings[npan].nVolume = 64;
	}

	// Reading Patterns
	for (UINT npat = 0; npat < fileHeader.patterns; npat++)
	{
		if(Patterns.Insert(npat, 64))
			break;

		ModCommand *m = Patterns[npat];
		for (UINT row=0; row<64; row++)
		{
			ModCommand *mspeed = m;
			if ((row == fileHeader.breaks[npat]) && (row != 63))
			{
				for (UINT i=0; i<8; i++)
				{
					m[i].command = CMD_PATTERNBREAK;
					m[i].param = 0;
				}
			}
			for(UINT n = 0; n < 8; n++, m++)
			{
				char data[3];
				if(!file.ReadArray(data))
				{
					break;
				}

				UINT note = data[0] >> 2;
				UINT instr = ((data[0] & 0x03) << 4) | (data[1] >> 4);
				UINT vol = data[1] & 0x0F;
				if (data[0] < 0xFE)
				{
					m->note = note + 37;
					m->instr = instr + 1;
				}
				if (data[0] <= 0xFE)
				{
					m->volcmd = VOLCMD_VOLUME;
					m->vol = (vol << 2) + 2;
				}
				if (data[2] != 0xFF)
				{
					UINT command = data[2] >> 4;
					UINT param = data[2] & 0x0F;
					switch(command)
					{
					case 0x00:	command = CMD_PORTAMENTOUP; break;
					case 0x01:	command = CMD_PORTAMENTODOWN; break;
					case 0x02:	command = CMD_TONEPORTAMENTO; break;
					case 0x03:	command = CMD_MODCMDEX; param |= 0x50; break;
					case 0x04:	command = CMD_VIBRATO; param |= 0x40; break;
					case 0x05:	if (param) command = CMD_SPEED; else command = 0; param += 2; break;
					case 0x06:	if (param == 0) { command = CMD_PANNINGSLIDE; param = 0xFE; } else
								if (param == 1) { command = CMD_PANNINGSLIDE; param = 0xEF; } else
								command = 0;
								break;
					default:	command = 0;
					}
					if (command)
					{
						if (command == CMD_SPEED) mspeed = NULL;
						m->command = command;
						m->param = param;
					}
				}
			}
			if ((!row) && (mspeed))
			{
				for (UINT i=0; i<8; i++) if (!mspeed[i].command)
				{
					mspeed[i].command = CMD_SPEED;
					mspeed[i].param = fileHeader.tempolist[npat] + 2;
					break;
				}
			}
		}
	}

	// Reading Samples
	for(SAMPLEINDEX n = 1; n <= m_nSamples; n++)
	{
		ReadSample(&Samples[n], RS_PCM8U, file);
	}
	return true;
}
