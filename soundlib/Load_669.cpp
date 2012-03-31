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
	enum MagicBytes
	{
		magic669	= 0x6669,	// 'if'
		magic669Ext	= 0x4E4A	// 'JN'
	};

	uint16 sig;					// 'if' or 'JN'
	char   songmessage[108];	// Song Message
	uint8  samples;				// number of samples (1-64)
	uint8  patterns;			// number of patterns (1-128)
	uint8  restartpos;
	uint8  orders[128];
	uint8  tempolist[128];
	uint8  breaks[128];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(sig);
	}
};

STATIC_ASSERT(sizeof(_669FileHeader) == 497);

struct _669Sample
{
	char   filename[13];
	uint32 length;
	uint32 loopStart;
	uint32 loopEnd;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
	}
};

STATIC_ASSERT(sizeof(_669Sample) == 25);

#pragma pack(pop)


bool CSoundFile::Read669(FileReader &file)
//----------------------------------------
{
	_669FileHeader fileHeader;
	
	file.Rewind();
	if(!file.ReadConvertEndianness(fileHeader))
	{
		return false;
	}

	if(fileHeader.sig != _669FileHeader::magic669 && fileHeader.sig != _669FileHeader::magic669Ext)
	{
		return false;
	}

	//bool has669Ext = fileHeader.sig == _669FileHeader::magic669Ext;
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

	m_nSamples = fileHeader.samples;
	for(SAMPLEINDEX smp = 1; smp <= m_nSamples; smp++)
	{
		_669Sample sample;
		if(!file.ReadConvertEndianness(sample))
		{
			return false;
		}

		SmpLength len = sample.length;
		SmpLength loopstart = sample.loopStart;
		SmpLength loopend = sample.loopEnd;
		if(len > MAX_SAMPLE_LENGTH) len = MAX_SAMPLE_LENGTH;
		if((loopend > len) && (!loopstart)) loopend = 0;
		if(loopend > len) loopend = len;
		if(loopstart + 4 >= loopend) loopstart = loopend = 0;
		Samples[smp].nLength = len;
		Samples[smp].nLoopStart = loopstart;
		Samples[smp].nLoopEnd = loopend;
		if(loopend) Samples[smp].uFlags |= CHN_LOOP;
		StringFixer::ReadString<StringFixer::maybeNullTerminated>(m_szNames[smp], sample.filename);
		Samples[smp].nVolume = 256;
		Samples[smp].nGlobalVol = 64;
		Samples[smp].nPan = 128;
	}

	// Copy first song message line into song title
	StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[0], fileHeader.songmessage, 36);
	// Song Message
	ReadFixedLineLengthMessage(reinterpret_cast<const BYTE *>(fileHeader.songmessage), 108, 36, 0);

	// Reading Orders
	Order.ReadAsByte(fileHeader.orders, 128, 128);
	m_nRestartPos = fileHeader.restartpos;
	if(Order[m_nRestartPos] >= fileHeader.patterns) m_nRestartPos = 0;

	// Set up panning
	for(CHANNELINDEX chn = 0; chn < 8; chn++)
	{
		ChnSettings[chn].nPan = (chn & 1) ? 0x30 : 0xD0;
		ChnSettings[chn].nVolume = 64;
	}

	// Reading Patterns
	for(PATTERNINDEX pat = 0; pat < fileHeader.patterns; pat++)
	{
		if(Patterns.Insert(pat, 64))
		{
			continue;
		}

		ModCommand *m = Patterns[pat];
		for(ROWINDEX row = 0; row < 64; row++)
		{
			ModCommand *mspeed = m;

			for(CHANNELINDEX n = 0; n < 8; n++, m++)
			{
				uint8 data[3];
				if(!file.ReadArray(data))
				{
					break;
				}

				uint8 note = data[0] >> 2;
				uint8 instr = ((data[0] & 0x03) << 4) | (data[1] >> 4);
				uint8 vol = data[1] & 0x0F;
				if (data[0] < 0xFE)
				{
					m->note = note + 36 + NOTE_MIN;
					m->instr = instr + 1;
				}
				if (data[0] <= 0xFE)
				{
					m->volcmd = VOLCMD_VOLUME;
					m->vol = (vol << 2) + 2;
				}
				if (data[2] != 0xFF)
				{
					uint8 command = data[2] >> 4;
					uint8 param = data[2] & 0x0F;
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
					mspeed[i].param = fileHeader.tempolist[pat] + 2;
					break;
				}
			}

			// Write pattern break
			if(fileHeader.breaks[pat] < 63)
			{
				TryWriteEffect(pat, fileHeader.breaks[pat], CMD_PATTERNBREAK, 0, false, CHANNELINDEX_INVALID, false, weTryPreviousRow);
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
