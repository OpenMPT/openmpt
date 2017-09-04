/*
 * Load_dtm.cpp
 * ------------
 * Purpose: Digital Tracker module Loader (DTM)
 * Notes  : There is a newer version of this format that is currently not supported properly.
 *          It has a different pattern format ("2.06"), more kinds of chunks and
 *          generally a somewhat different structure here and there.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "ChunkReader.h"

OPENMPT_NAMESPACE_BEGIN

struct DTMFileHeader
{
	char     magic[4];
	uint32be headerSize;
	uint16be type;
	uint16be mono;      // FF 08 = mono, 00 08 = stereo?
	uint16be reserved;  // Usually 0, but not in unknown title 1.dtm and unknown title 2.dtm
	uint16be speed;
	uint16be tempo;
	uint32be unknown;   // Seems to be related to sample rates, often it's 0, 8400 or 24585
};

MPT_BINARY_STRUCT(DTMFileHeader, 22)


// IFF-style Chunk
struct DTMChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idS_Q_ = MAGIC4BE('S', '.', 'Q', '.'),
		idPATT = MAGIC4BE('P', 'A', 'T', 'T'),
		idINST = MAGIC4BE('I', 'N', 'S', 'T'),
		idDAPT = MAGIC4BE('D', 'A', 'P', 'T'),
		idDAIT = MAGIC4BE('D', 'A', 'I', 'T'),
		idTEXT = MAGIC4BE('T', 'E', 'X', 'T'),
		idVERS = MAGIC4BE('V', 'E', 'R', 'S'),
		idPATN = MAGIC4BE('P', 'A', 'T', 'N'), // Pattern names, 48 chars long?
		idTRKN = MAGIC4BE('T', 'R', 'K', 'N'), // Channel names, 32 chars long?
		idSV19 = MAGIC4BE('S', 'V', '1', '9'), // ?
	};

	uint32be id;
	uint32be length;

	size_t GetLength() const
	{
		return length;
	}

	ChunkIdentifiers GetID() const
	{
		return static_cast<ChunkIdentifiers>(id.get());
	}
};

MPT_BINARY_STRUCT(DTMChunk, 8)


struct DTMSample
{
	uint32be reserved;
	uint32be length;     // in bytes
	uint8be  finetune;   // -8....7
	uint8be  volume;     // 0...64
	uint32be loopStart;  // in bytes
	uint32be loopLength; // ditto
	char     name[22];
	uint8be  stereo;
	uint8be  bitDepth;
	uint32be midiNote;
	uint32be sampleRate;

	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize(MOD_TYPE_IT);
		mptSmp.nLength = length;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = mptSmp.nLoopStart + loopLength;
		mptSmp.nC5Speed = sampleRate;
		mptSmp.Transpose(MOD2XMFineTune(finetune) * (1.0 / (12.0 * 128.0)));
		mptSmp.nVolume = std::min<uint8>(volume, 64u) * 4u;
		if(stereo & 1)
		{
			mptSmp.uFlags.set(CHN_STEREO);
			mptSmp.nLength /= 2u;
			mptSmp.nLoopStart /= 2u;
			mptSmp.nLoopEnd /= 2u;
		}
		if(bitDepth > 8)
		{
			mptSmp.uFlags.set(CHN_16BIT);
			mptSmp.nLength /= 2u;
			mptSmp.nLoopStart /= 2u;
			mptSmp.nLoopEnd /= 2u;
		}
		if(mptSmp.nLoopEnd > mptSmp.nLoopStart + 1)
		{
			mptSmp.uFlags.set(CHN_LOOP);
		} else
		{
			mptSmp.nLoopStart = mptSmp.nLoopEnd = 0;
		}
		//mptSmp.rootNote = static_cast<uint8>((midiNote >> 16) - 0x30 + NOTE_MIDDLEC);
	}
};

MPT_BINARY_STRUCT(DTMSample, 50)


enum PatternFormats : uint32
{
	DTM_PT_PATTERN_FORMAT = 0,
	DTM_NEW_PATTERN_FORMAT = MAGIC4BE('2', '.', '0', '4'),
};


bool CSoundFile::ReadDTM(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	DTMFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader)
		|| memcmp(fileHeader.magic, "D.T.", 4)
		|| fileHeader.headerSize < sizeof(fileHeader) - 8u
		|| fileHeader.headerSize > 256 // Excessively long song title?
		|| fileHeader.type != 0)
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals(MOD_TYPE_DTM);
	InitializeChannels();
	m_madeWithTracker = MPT_USTRING("Digital Tracker");
	m_SongFlags.set(SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS);
	// Various files have a default speed or tempo of 0
	if(fileHeader.tempo)
		m_nDefaultTempo.Set(fileHeader.tempo);
	if(fileHeader.speed)
		m_nDefaultSpeed = fileHeader.speed;

	file.ReadString<mpt::String::maybeNullTerminated>(m_songName, fileHeader.headerSize - (sizeof(fileHeader) - 8u));

	auto chunks = ChunkReader(file).ReadChunks<DTMChunk>(1);

	// Read order list
	if(FileReader chunk = chunks.GetChunk(DTMChunk::idS_Q_))
	{
		uint16 ordLen = chunk.ReadUint16BE();
		uint16 restartPos = chunk.ReadUint16BE();
		chunk.Skip(4);	// Reserved
		ReadOrderFromFile<uint8>(Order(), chunk, ordLen);
		Order().SetRestartPos(restartPos);
	} else
	{
		return false;
	}

	// Read song message
	if(FileReader chunk = chunks.GetChunk(DTMChunk::idTEXT))
	{
		chunk.Skip(12);
		m_songMessage.Read(chunk, chunk.BytesLeft(), SongMessage::leCRLF);
	}

	// Read pattern properties
	uint32 patternFormat;
	if(FileReader chunk = chunks.GetChunk(DTMChunk::idPATT))
	{
		m_nChannels = chunk.ReadUint16BE();
		if(m_nChannels < 1 || m_nChannels > MAX_BASECHANNELS)
		{
			return false;
		}
		Patterns.ResizeArray(chunk.ReadUint16BE());	// Number of stored patterns, may be lower than highest pattern number
		patternFormat = chunk.ReadUint32BE();
		if(patternFormat != DTM_PT_PATTERN_FORMAT && patternFormat != DTM_NEW_PATTERN_FORMAT)
		{
			return false;
		}
	} else
	{
		return false;
	}

	// Read sample headers
	if(FileReader chunk = chunks.GetChunk(DTMChunk::idINST))
	{
		uint16 numSamples = chunk.ReadUint16BE();
		bool newSamples = (numSamples >= 0x8000);
		if(newSamples)
		{
			numSamples &= 0x7FFF;
		}
		if(numSamples >= MAX_SAMPLES || !chunk.CanRead(numSamples) * sizeof(DTMSample))
		{
			return false;
		}
		
		m_nSamples = numSamples;
		for(SAMPLEINDEX smp = 1; smp <= numSamples; smp++)
		{
			ModSample &mptSmp = Samples[smp];
			DTMSample dtmSample;
			if(newSamples)
			{
				chunk.Skip(2);
			}
			chunk.ReadStruct(dtmSample);
			dtmSample.ConvertToMPT(mptSmp);
			mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[smp], dtmSample.name);
		}
	}

	// Read pattern data
	for(auto &chunk : chunks.GetAllChunks(DTMChunk::idDAPT))
	{
		chunk.Skip(4);	// FF FF FF FF
		PATTERNINDEX patNum = chunk.ReadUint16BE();
		ROWINDEX numRows = chunk.ReadUint16BE();
		if(!(loadFlags & loadPatternData) || patNum > 255 || !Patterns.Insert(patNum, numRows))
		{
			continue;
		}

		ModCommand *m = Patterns[patNum].GetpModCommand(0, 0);
		for(ROWINDEX row = 0; row < numRows; row++)
		{
			for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++, m++)
			{
				uint8 data[4];
				chunk.ReadArray(data);
				if(patternFormat == DTM_NEW_PATTERN_FORMAT)
				{
					if(data[0] > 0 && data[0] < 0x80)
					{
						m->note = (data[0] >> 4) * 12 + (data[0] & 0x0F) + NOTE_MIN + 11;
					}
					uint8 vol = data[1] >> 2;
					if(vol)
					{
						m->volcmd = VOLCMD_VOLUME;
						m->vol = std::min<uint8>(vol - 1u, 64u);
					}
					m->instr = ((data[1] & 0x03) << 4) | (data[2] >> 4);
					m->command = data[2] & 0x0F;
					m->param = data[3];
				} else
				{
					ReadMODPatternEntry(data, *m);
					m->instr |= data[0] & 0x30;	// Allow more than 31 instruments
				}
				ConvertModCommand(*m);
				// Fix commands without memory and slide nibble precedence
				switch(m->command)
				{
				case CMD_PORTAMENTOUP:
				case CMD_PORTAMENTODOWN:
					if(!m->param)
					{
						m->command = CMD_NONE;
					}
					break;
				case CMD_VOLUMESLIDE:
				case CMD_TONEPORTAVOL:
				case CMD_VIBRATOVOL:
					if(m->param & 0xF0)
					{
						m->param &= 0xF0;
					} else if(!m->param)
					{
						m->command = CMD_NONE;
					}
					break;
				default:
					break;
				}
#ifdef MODPLUG_TRACKER
				m->Convert(MOD_TYPE_MOD, MOD_TYPE_IT, *this);
#endif
			}
		}
	}

	// Read sample data
	SAMPLEINDEX smp = 1;
	for(auto &chunk : chunks.GetAllChunks(DTMChunk::idDAIT))
	{
		if(smp > GetNumSamples() || !(loadFlags & loadSampleData))
		{
			break;
		}
		ModSample &mptSmp = Samples[smp];
		SampleIO(
			mptSmp.uFlags[CHN_16BIT] ? SampleIO::_16bit : SampleIO::_8bit,
			mptSmp.uFlags[CHN_STEREO] ? SampleIO::stereoInterleaved: SampleIO::mono,
			SampleIO::bigEndian,
			SampleIO::signedPCM).ReadSample(mptSmp, chunk);
		smp++;
	}

	return true;
}

OPENMPT_NAMESPACE_END
