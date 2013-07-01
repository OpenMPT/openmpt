/*
 * Load_far.cpp
 * ------------
 * Purpose: Farandole (FAR) module loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs (partly inspired by Storlek's FAR loader from Schism Tracker)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

// FAR File Header
struct PACKED FARFileHeader
{
	uint8  magic[4];
	char   songName[40];
	uint8  eof[3];
	uint16 headerLength;
	uint8  version;
	uint8  onOff[16];
	uint8  editingState[9];	// Stuff we don't care about
	uint8  defaultSpeed;
	uint8  chnPanning[16];
	uint8  patternState[4];	// More stuff we don't care about
	uint16 messageLength;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(headerLength);
		SwapBytesLE(messageLength);
	}
};

STATIC_ASSERT(sizeof(FARFileHeader) == 98);


struct PACKED FAROrderHeader
{
	uint8  orders[256];
	uint8  numPatterns;	// supposed to be "number of patterns stored in the file"; apparently that's wrong
	uint8  numOrders;
	uint8  restartPos;
	uint16 patternSize[256];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		for(size_t i = 0; i < CountOf(patternSize); i++)
		{
			SwapBytesLE(patternSize[i]);
		}
	}
};

STATIC_ASSERT(sizeof(FAROrderHeader) == 771);


// FAR Sample header
struct PACKED FARSampleHeader
{
	// Sample flags
	enum SampleFlags
	{
		smp16Bit	= 0x01,
		smpLoop		= 0x08,
	};

	char   name[32];
	uint32 length;
	uint8  finetune;
	uint8  volume;
	uint32 loopStart;
	uint32 loopEnd;
	uint8  type;
	uint8  loop;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
	}

	// Convert sample header to OpenMPT's internal format.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();

		mptSmp.nLength = length;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;
		mptSmp.nC5Speed = 8363 * 2;
		mptSmp.nVolume = volume * 16;

		if(type & smp16Bit)
		{
			mptSmp.nLength /= 2;
			mptSmp.nLoopStart /= 2;
			mptSmp.nLoopEnd /= 2;
		}

		if((loop & 8) && mptSmp.nLoopEnd > mptSmp.nLoopStart)
		{
			mptSmp.uFlags |= CHN_LOOP;
		}
	}

	// Retrieve the internal sample format flags for this sample.
	SampleIO GetSampleFormat() const
	{
		return SampleIO(
			(type & smp16Bit) ? SampleIO::_16bit : SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::signedPCM);
	}
};

STATIC_ASSERT(sizeof(FARSampleHeader) == 48);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadFAR(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	FARFileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| memcmp(fileHeader.magic, "FAR\xFE", 4) != 0
		|| memcmp(fileHeader.eof, "\x0D\x0A\x1A", 3)
		|| file.GetLength() < static_cast<size_t>(fileHeader.headerLength))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	// Globals
	InitializeGlobals();
	m_nType = MOD_TYPE_FAR;
	m_nChannels = 16;
	m_nSamplePreAmp = 32;
	m_nDefaultSpeed = fileHeader.defaultSpeed;
	m_nDefaultTempo = 80;
	m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;

	mpt::String::Read<mpt::String::maybeNullTerminated>(songName, fileHeader.songName);

	// Read channel settings
	for(CHANNELINDEX chn = 0; chn < 16; chn++)
	{
		ChnSettings[chn].Reset();
		ChnSettings[chn].dwFlags = fileHeader.onOff[chn] ? ChannelFlags(0) : CHN_MUTE;
		ChnSettings[chn].nPan = ((fileHeader.chnPanning[chn] & 0x0F) << 4) + 8;
	}

	// Read song message
	if(fileHeader.messageLength != 0)
	{
		songMessage.ReadFixedLineLength(file, fileHeader.messageLength, 132, 0);	// 132 characters per line... wow. :)
	}

	// Read orders
	FAROrderHeader orderHeader;
	if(!file.ReadConvertEndianness(orderHeader))
	{
		return false;
	}
	Order.ReadFromArray(orderHeader.orders, orderHeader.numOrders);
	m_nRestartPos = orderHeader.restartPos;

	file.Seek(fileHeader.headerLength);
	
	// Pattern effect LUT
	static const uint8 farEffects[] =
	{
		CMD_NONE,
		CMD_PORTAMENTOUP,
		CMD_PORTAMENTODOWN,
		CMD_TONEPORTAMENTO,
		CMD_RETRIG,
		CMD_VIBRATO,		// depth
		CMD_VIBRATO,		// speed
		CMD_VOLUMESLIDE,	// up
		CMD_VOLUMESLIDE,	// down
		CMD_VIBRATO,		// sustained (?)
		CMD_NONE,			// actually slide-to-volume
		CMD_S3MCMDEX,		// panning
		CMD_S3MCMDEX,		// note offset => note delay?
		CMD_NONE,			// fine tempo down
		CMD_NONE,			// fine tempo up
		CMD_SPEED,
	};
	
	// Read patterns
	for(PATTERNINDEX pat = 0; pat < 256; pat++)
	{
		if(!orderHeader.patternSize[pat])
		{
			continue;
		}

		FileReader patternChunk = file.GetChunk(orderHeader.patternSize[pat]);

		// Calculate pattern length in rows (every event is 4 bytes, and we have 16 channels)
		ROWINDEX numRows = (orderHeader.patternSize[pat] - 2) / (16 * 4);
		if(!(loadFlags & loadPatternData) || !numRows || numRows > MAX_PATTERN_ROWS || Patterns.Insert(pat, numRows))
		{
			continue;
		}

		// Read break row and unused value
		ROWINDEX breakRow = patternChunk.ReadUint8();
		patternChunk.Skip(1);
		if(breakRow > 0 && breakRow < numRows - 2)
		{
			breakRow++;
		} else
		{
			breakRow = ROWINDEX_INVALID;
		}

		// Read pattern data
		for(ROWINDEX row = 0; row < numRows; row++)
		{
			PatternRow rowBase = Patterns[pat].GetRow(row);
			for(CHANNELINDEX chn = 0; chn < 16; chn++)
			{
				ModCommand &m = rowBase[chn];

				uint8 data[4];
				patternChunk.ReadArray(data);

				if(data[0] > 0 && data[0] < 85)
				{
					m.note = data[0] + 35 + NOTE_MIN;
					m.instr = data[1] + 1;
				}

				if(data[2] & 0x0F)
				{
					m.volcmd = VOLCMD_VOLUME;
					m.vol = (data[2] & 0x0F) << 2;
				}
				
				m.param = data[3] & 0x0F;

				switch(data[3] >> 4)
				{
				case 0x03:	// Porta to note
					m.param <<= 2;
					break;
				case 0x04:	// Retrig
					m.param = 6 / (1 + (m.param & 0xf)) + 1; // ugh?
					break;
				case 0x06:	// Vibrato speed
				case 0x07:	// Volume slide up
					m.param *= 8;
					break;
				case 0x0A:	// Volume-portamento (what!)
					m.volcmd = VOLCMD_VOLUME;
					m.vol = (m.param << 2) + 4;
					break;
				case 0x0B:	// Panning
					m.param |= 0x80;
					break;
				case 0x0C:	// Note offset
					m.param = 6 / (1 + m.param) + 1;
					m.param |= 0x0D;
				}
				m.command = farEffects[data[3] >> 4];
			}
		}

		Patterns[pat].WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(breakRow).Retry(EffectWriter::rmTryNextRow));
	}
	
	if(!(loadFlags & loadSampleData))
	{
		return true;
	}

	// Read samples
	uint8 sampleMap[8];	// Sample usage bitset
	file.ReadArray(sampleMap);

	for(SAMPLEINDEX smp = 0; smp < 64; smp++)
	{
		if(!(sampleMap[smp >> 3] & (1 << (smp & 7))))
		{
			continue;
		}

		FARSampleHeader sampleHeader;
		if(!file.ReadConvertEndianness(sampleHeader))
		{
			return true;
		}

		m_nSamples = smp + 1;
		ModSample &sample = Samples[m_nSamples];
		mpt::String::Read<mpt::String::nullTerminated>(m_szNames[m_nSamples], sampleHeader.name);
		sampleHeader.ConvertToMPT(sample);
		sampleHeader.GetSampleFormat().ReadSample(sample, file);
	}
	return true;
}
