/*
 * Load_dsm.cpp
 * ------------
 * Purpose: Digisound Interface Kit (DSIK) Internal Format (DSM v2 / RIFF) module loader
 * Notes  : 1. There is also another fundamentally different DSIK DSM v1 module format, not handled here.
 *          MilkyTracker can load it, but the only files of this format seen in the wild are also
 *          available in their original format, so I did not bother implementing it so far.
 *
 *          2. Using PLAY.EXE v1.02, commands not supported in MOD do not seem to do anything at all.
 *          In particular commands 0x11-0x13 handled below are ignored, and no files have been spotted
 *          in the wild using any commands > 0x0F at all.
 *          S3M-style retrigger does not seem to exist - it is translated to volume slides by CONV.EXE,
 *          and J00 in S3M files is not converted either.
 *          Command 8 (set panning) uses 00-80 for regular panning and A4 for surround, probably
 *          making DSIK one of the first applications to use this particular encoding scheme still
 *          used in "extended" S3Ms today.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

OPENMPT_NAMESPACE_BEGIN

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED DSMChunk
{
	char   magic[4];
	uint32 size;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(size);
	}
};

STATIC_ASSERT(sizeof(DSMChunk) == 8);


struct PACKED DSMSongHeader
{
	char   songName[28];
	char   reserved1[2];
	uint16 flags;
	char   reserved2[4];
	uint16 numOrders;
	uint16 numSamples;
	uint16 numPatterns;
	uint16 numChannels;
	uint8  globalVol;
	uint8  mastervol;
	uint8  speed;
	uint8  bpm;
	uint8  panPos[16];
	uint8  orders[128];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(flags);
		SwapBytesLE(numOrders);
		SwapBytesLE(numSamples);
		SwapBytesLE(numPatterns);
		SwapBytesLE(numChannels);
	}
};

STATIC_ASSERT(sizeof(DSMSongHeader) == 192);


struct PACKED DSMSampleHeader
{
	char   filename[13];
	uint8  flags;
	char   reserved1;
	uint8  volume;
	uint32 length;
	uint32 loopStart;
	uint32 loopEnd;
	char   reserved2[4];
	uint32 sampleRate;
	char   sampleName[28];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(sampleRate);
	}

	// Convert a DSM sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();
		mpt::String::Read<mpt::String::nullTerminated>(mptSmp.filename, filename);

		mptSmp.nC5Speed = sampleRate;
		mptSmp.uFlags.set(CHN_LOOP, (flags & 1) != 0);
		mptSmp.nLength = length;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;
		mptSmp.nVolume = std::min(volume, uint8(64)) * 4;
	}

	// Retrieve the internal sample format flags for this sample.
	SampleIO GetSampleFormat() const
	{
		SampleIO sampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::unsignedPCM);
		if(flags & 0x40)
			sampleIO |= SampleIO::deltaPCM;	// fairlight.dsm by Comrade J
		else if(flags & 0x02)
			sampleIO |= SampleIO::signedPCM;
		return sampleIO;
	}
};

STATIC_ASSERT(sizeof(DSMSampleHeader) == 64);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadDSM(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	char fileMagic0[4];
	char fileMagic1[4];
	char fileMagic2[4];

	if(!file.ReadArray(fileMagic0)) return false;
	if(!file.ReadArray(fileMagic1)) return false;
	if(!file.ReadArray(fileMagic2)) return false;

	if(!memcmp(fileMagic0, "RIFF", 4)
		&& !memcmp(fileMagic2, "DSMF", 4))
	{
		// "Normal" DSM files with RIFF header
		// <RIFF> <file size> <DSMF>
	} else if(!memcmp(fileMagic0, "DSMF", 4))
	{
		// DSM files with alternative header
		// <DSMF> <4 bytes, usually 4x NUL or RIFF> <file size> <4 bytes, usually DSMF but not always>
		file.Skip(4);
	} else
	{
		return false;
	}

	DSMChunk chunkHeader;

	file.ReadConvertEndianness(chunkHeader);
	// Technically, the song chunk could be anywhere in the file, but we're going to simplify
	// things by not using a chunk header here and just expect it to be right at the beginning.
	if(memcmp(chunkHeader.magic, "SONG", 4))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	DSMSongHeader songHeader;
	file.ReadStructPartial(songHeader, chunkHeader.size);
	songHeader.ConvertEndianness();

	InitializeGlobals(MOD_TYPE_DSM);
	mpt::String::Read<mpt::String::maybeNullTerminated>(m_songName, songHeader.songName);
	m_nChannels = Clamp(songHeader.numChannels, uint16(1), uint16(16));
	m_nDefaultSpeed = songHeader.speed;
	m_nDefaultTempo.Set(songHeader.bpm);
	m_nDefaultGlobalVolume = std::min(songHeader.globalVol, uint8(64)) * 4u;
	if(!m_nDefaultGlobalVolume) m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	if(songHeader.mastervol == 0x80)
	{
		m_nSamplePreAmp = std::min(256u / m_nChannels, 128u);
	} else
	{
		m_nSamplePreAmp = songHeader.mastervol & 0x7F;
	}

	// Read channel panning
	for(CHANNELINDEX chn = 0; chn < 16; chn++)
	{
		ChnSettings[chn].Reset();
		if(songHeader.panPos[chn] <= 0x80)
		{
			ChnSettings[chn].nPan = songHeader.panPos[chn] * 2;
		}
	}

	Order.ReadFromArray(songHeader.orders, songHeader.numOrders, 0xFF, 0xFE);

	// Read pattern and sample chunks
	PATTERNINDEX patNum = 0;
	while(file.ReadConvertEndianness(chunkHeader))
	{
		FileReader chunk = file.ReadChunk(chunkHeader.size);

		if(!memcmp(chunkHeader.magic, "PATT", 4) && (loadFlags & loadPatternData))
		{
			// Read pattern
			if(!Patterns.Insert(patNum, 64))
			{
				continue;
			}
			chunk.Skip(2);

			ROWINDEX row = 0;
			PatternRow rowBase = Patterns[patNum];
			while(chunk.CanRead(1) && row < 64)
			{
				uint8 flag = chunk.ReadUint8();
				if(!flag)
				{
					row++;
					rowBase = Patterns[patNum].GetRow(row);
					continue;
				}

				CHANNELINDEX chn = (flag & 0x0F);
				ModCommand dummy;
				ModCommand &m = (chn < GetNumChannels() ? rowBase[chn] : dummy);

				if(flag & 0x80)
				{
					uint8 note = chunk.ReadUint8();
					if(note)
					{
						if(note <= 12 * 9) note += 11 + NOTE_MIN;
						m.note = note;
					}
				}
				if(flag & 0x40)
				{
					m.instr = chunk.ReadUint8();
				}
				if (flag & 0x20)
				{
					m.volcmd = VOLCMD_VOLUME;
					m.vol = std::min(chunk.ReadUint8(), uint8(64));
				}
				if(flag & 0x10)
				{
					uint8 command = chunk.ReadUint8();
					uint8 param = chunk.ReadUint8();
					switch(command)
					{
						// Portamentos
					case 0x11:
					case 0x12:
						command &= 0x0F;
						break;
						// 3D Sound (?)
					case 0x13:
						command = 'X' - 55;
						param = 0x91;
						break;
					default:
						// Volume + Offset (?)
						if(command > 0x10)
							command = ((command & 0xF0) == 0x20) ? 0x09 : 0xFF;
					}
					m.command = command;
					m.param = param;
					ConvertModCommand(m);
				}
			}
			patNum++;
		} else if(!memcmp(chunkHeader.magic, "INST", 4) && GetNumSamples() < SAMPLEINDEX(MAX_SAMPLES - 1))
		{
			// Read sample
			m_nSamples++;
			ModSample &sample = Samples[m_nSamples];

			DSMSampleHeader sampleHeader;
			chunk.ReadConvertEndianness(sampleHeader);
			sampleHeader.ConvertToMPT(sample);

			mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[m_nSamples], sampleHeader.sampleName);

			if(loadFlags & loadSampleData)
			{
				sampleHeader.GetSampleFormat().ReadSample(sample, chunk);
			}
		}
	}

	return true;
}


OPENMPT_NAMESPACE_END
