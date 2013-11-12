/*
 * Load_ptm.cpp
 * ------------
 * Purpose: PTM (PolyTracker) module loader
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED PTMFileHeader
{
	char   songname[28];	// Name of song, asciiz string
	uint8  dosEOF;			// 26
	uint8  versionLo;		// 03 version of file, currently 0203h
	uint8  versionHi;		// 02
	uint8  reserved1;		// Reserved, set to 0
	uint16 numOrders;		// Number of orders (0..256)
	uint16 numSamples;		// Number of instruments (1..255)
	uint16 numPatterns;		// Number of patterns (1..128)
	uint16 numChannels;		// Number of channels (voices) used (1..32)
	uint16 flags;			// Set to 0
	uint8  reserved2[2];	// Reserved, set to 0
	char   magic[4];		// Song identification, 'PTMF'
	uint8  reserved3[16];	// Reserved, set to 0
	uint8  chnPan[32];		// Channel panning settings, 0..15, 0 = left, 7 = middle, 15 = right
	uint8  orders[256];		// Order list, valid entries 0..nOrders-1
	uint16 patOffsets[128];	// Pattern offsets (*16)

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(numOrders);
		SwapBytesLE(numSamples);
		SwapBytesLE(numPatterns);
		SwapBytesLE(numChannels);
		SwapBytesLE(flags);
		for(std::size_t i = 0; i < CountOf(patOffsets); i++)
		{
			SwapBytesLE(patOffsets[i]);
		}
	}
};

STATIC_ASSERT(sizeof(PTMFileHeader) == 608);

struct PACKED PTMSampleHeader
{
	enum SampleFlags
	{
		smpTypeMask	= 0x03,
		smpPCM		= 0x01,

		smpLoop		= 0x04,
		smpPingPong	= 0x08,
		smp16Bit	= 0x10,
	};

	uint8  flags;			// Sample type (see SampleFlags)
	char   filename[12];	// Name of external sample file
	uint8  volume;			// Default volume
	uint16 c4speed;			// C-4 speed (yep, not C-5)
	uint8  smpSegment[2];	// Sample segment (used internally)
	uint32 dataOffset;		// Offset of sample data
	uint32 length;			// Sample size (in bytes)
	uint32 loopStart;		// Start of loop
	uint32 loopEnd;			// End of loop
	uint8  gusdata[14];
	char   samplename[28];	// Name of sample, ASCIIZ
	char   magic[4];		// Sample identification, 'PTMS'

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(c4speed);
		SwapBytesLE(dataOffset);
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
	}

	// Convert an PTM sample header to OpenMPT's internal sample header.
	SampleIO ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize(MOD_TYPE_S3M);
		mptSmp.nVolume = std::min(volume, uint8(64)) * 4;
		mptSmp.nC5Speed = c4speed * 2;

		mpt::String::Read<mpt::String::maybeNullTerminated>(mptSmp.filename, filename);

		SampleIO sampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::deltaPCM);

		if((flags & smpTypeMask) == smpPCM)
		{
			mptSmp.nLength = length;
			mptSmp.nLoopStart = loopStart;
			mptSmp.nLoopEnd = loopEnd;
			if(mptSmp.nLoopEnd > mptSmp.nLoopStart)
				mptSmp.nLoopEnd--;

			if(flags & smpLoop) mptSmp.uFlags.set(CHN_LOOP);
			if(flags & smpPingPong) mptSmp.uFlags.set(CHN_PINGPONGLOOP);
			if(flags & smp16Bit)
			{
				sampleIO |= SampleIO::_16bit;
				sampleIO |= SampleIO::PTM8Dto16;

				mptSmp.nLength /= 2;
				mptSmp.nLoopStart /= 2;
				mptSmp.nLoopEnd /= 2;
			}
		}

		return sampleIO;
	}
};

STATIC_ASSERT(sizeof(PTMSampleHeader) == 80);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadPTM(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	PTMFileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| memcmp(fileHeader.magic, "PTMF", 4)
		|| fileHeader.dosEOF != 26
		|| fileHeader.versionHi > 2
		|| fileHeader.flags != 0
		|| !fileHeader.numChannels
		|| fileHeader.numChannels > 32
		|| !fileHeader.numOrders || fileHeader.numOrders > 256
		|| !fileHeader.numSamples || fileHeader.numSamples > 255
		|| !fileHeader.numPatterns || fileHeader.numPatterns > 128
		|| !file.CanRead(fileHeader.numSamples * sizeof(PTMSampleHeader)))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	mpt::String::Read<mpt::String::maybeNullTerminated>(songName, fileHeader.songname);

	InitializeGlobals();
	madeWithTracker = mpt::String::Print("PolyTracker %1.%2", fileHeader.versionHi, mpt::fmt::hex0<2>(fileHeader.versionLo));
	m_nType = MOD_TYPE_PTM;
	SetModFlag(MSF_COMPATIBLE_PLAY, true);
	m_SongFlags = SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS;
	m_nChannels = fileHeader.numChannels;
	m_nSamples = std::min<SAMPLEINDEX>(fileHeader.numSamples, MAX_SAMPLES - 1);
	Order.ReadFromArray(fileHeader.orders, fileHeader.numOrders);

	// Reading channel panning
	for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
	{
		ChnSettings[chn].Reset();
		ChnSettings[chn].nPan = ((fileHeader.chnPan[chn] & 0x0F) << 4) + 4;
	}

	// Reading samples
	FileReader sampleHeaderChunk = file.GetChunk(fileHeader.numSamples * sizeof(PTMSampleHeader));
	for(SAMPLEINDEX smp = 0; smp < m_nSamples; smp++)
	{
		PTMSampleHeader sampleHeader;
		sampleHeaderChunk.ReadConvertEndianness(sampleHeader);

		ModSample &sample = Samples[smp + 1];
		mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[smp + 1], sampleHeader.samplename);
		SampleIO sampleIO = sampleHeader.ConvertToMPT(sample);

		if((loadFlags & loadSampleData) && sample.nLength && file.Seek(sampleHeader.dataOffset))
		{
			sampleIO.ReadSample(sample, file);
		}
	}

	// Reading Patterns
	if(!(loadFlags & loadPatternData))
	{
		return true;
	}

	for(PATTERNINDEX pat = 0; pat < fileHeader.numPatterns; pat++)
	{
		if(Patterns.Insert(pat, 64)
			|| fileHeader.patOffsets[pat] == 0
			|| !file.Seek(fileHeader.patOffsets[pat] << 4))
		{
			continue;
		}

		ModCommand *rowBase = Patterns[pat];
		ROWINDEX row = 0;
		while(row < 64 && file.AreBytesLeft())
		{
			uint8 b = file.ReadUint8();

			if(b == 0)
			{
				row++;
				rowBase += m_nChannels;
				continue;
			}
			CHANNELINDEX chn = (b & 0x1F);
			ModCommand dummy;
			ModCommand &m = chn < GetNumChannels() ? rowBase[chn] : dummy;

			if(b & 0x20)
			{
				m.note = file.ReadUint8();
				m.instr = file.ReadUint8();
				if(m.note == 254)
					m.note = NOTE_NOTECUT;
				else if(!m.note || m.note > 120)
					m.note = NOTE_NONE;
			}
			if(b & 0x40)
			{
				m.command = file.ReadUint8();
				m.param = file.ReadUint8();

				const ModCommand::COMMAND effTrans[] = { CMD_GLOBALVOLUME, CMD_RETRIG, CMD_FINEVIBRATO, CMD_NOTESLIDEUP, CMD_NOTESLIDEDOWN, CMD_NOTESLIDEUPRETRIG, CMD_NOTESLIDEDOWNRETRIG, CMD_REVERSEOFFSET };
				if(m.command < 0x10)
				{
					// Beware: Effect letters are as in MOD, but portamento and volume slides behave like in S3M (i.e. fine slides share the same effect letters)
					ConvertModCommand(m);
				} else if(m.command < 0x10 + CountOf(effTrans))
				{
					m.command = effTrans[m.command - 0x10];
				} else
				{
					m.command = CMD_NONE;
				}
				switch(m.command)
				{
				case CMD_PANNING8:
					// My observations of this weird command...
					// 800...80F and 880...88F are panned dead centre.
					// 810...87F and 890...8FF pan from hard left to hard right.
					// A default center panning or using 800 is a bit louder than using 848, for whatever reason.
					m.param &= 0x7F;
					if(m.param < 0x10)
					{
						m.param = 0x80;
					} else
					{
						m.param = (m.param - 0x10) * 0xFF / 0x6F;
					}
					break;
				case CMD_GLOBALVOLUME:
					m.param = std::min(m.param, uint8(0x40)) * 2u;
					break;
				}
			}
			if(b & 0x80)
			{
				m.volcmd = VOLCMD_VOLUME;
				m.vol = file.ReadUint8();
			}
		}
	}
	return true;
}
