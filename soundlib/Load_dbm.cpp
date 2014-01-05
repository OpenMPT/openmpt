/*
 * Load_dbm.cpp
 * ------------
 * Purpose: DigiBooster Pro module Loader (DBM)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "ChunkReader.h"

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED DBMFileHeader
{
	char  dbm0[4];
	uint8 trkVerHi;
	uint8 trkVerLo;
	char  reserved[2];
};


// RIFF-style Chunk
struct PACKED DBMChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idNAME	= 0x454D414E,
		idINFO	= 0x4F464E49,
		idSONG	= 0x474E4F53,
		idINST	= 0x54534E49,
		idVENV	= 0x564E4556,
		idPENV	= 0x564E4550,
		idPATT	= 0x54544150,
		idPNAM	= 0x4D414E50,
		idSMPL	= 0x4c504d53,
		idMPEG	= 0x4745504D,
	};

	typedef ChunkIdentifiers id_type;

	uint32 id;
	uint32 length;

	size_t GetLength() const
	{
		return SwapBytesReturnBE(length);
	}

	id_type GetID() const
	{
		return static_cast<id_type>(SwapBytesReturnLE(id));
	}
};

STATIC_ASSERT(sizeof(DBMChunk) == 8);


struct PACKED DBMInfoChunk
{
	uint16 instruments;
	uint16 samples;
	uint16 songs;
	uint16 patterns;
	uint16 channels;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(instruments);
		SwapBytesBE(samples);
		SwapBytesBE(songs);
		SwapBytesBE(patterns);
		SwapBytesBE(channels);
	}
};

STATIC_ASSERT(sizeof(DBMInfoChunk) == 10);


// Instrument header
struct PACKED DBMInstrument
{
	enum DBMInstrFlags
	{
		smpLoop			= 0x01,
		smpPingPongLoop	= 0x02,
	};

	char   name[30];
	uint16 sample;		// Sample reference
	uint16 volume;		// 0...64
	uint32 sampleRate;
	uint32 loopStart;
	uint32 loopLength;
	int16  panning;		// -128...128
	uint16 flags;		// See DBMInstrFlags

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(sample);
		SwapBytesBE(volume);
		SwapBytesBE(sampleRate);
		SwapBytesBE(loopStart);
		SwapBytesBE(loopLength);
		SwapBytesBE(panning);
		SwapBytesBE(flags);
	}
};

STATIC_ASSERT(sizeof(DBMInstrument) == 50);


// Volume or panning envelope
struct PACKED DBMEnvelope
{
	enum DBMEnvelopeFlags
	{
		envEnabled	= 0x01,
		envSustain	= 0x02,
		envLoop		= 0x04,
	};

	uint16 instrument;
	uint8  flags;		// See DBMEnvelopeFlags
	uint8  numSegments;	// Number of envelope points - 1
	uint8  sustain1;
	uint8  loopBegin;
	uint8  loopEnd;
	uint8  sustain2;	// Second sustain point
	uint16 data[2 * 32];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(instrument);
		for(size_t i = 0; i < CountOf(data); i++)
		{
			SwapBytesBE(data[i]);
		}
	}
};

STATIC_ASSERT(sizeof(DBMEnvelope) == 136);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


// Note: Unlike in MOD, 1Fx, 2Fx, 5Fx / 5xF, 6Fx / 6xF and AFx / AxF are fine slides.
static const ModCommand::COMMAND dbmEffects[] =
{
	CMD_ARPEGGIO, CMD_PORTAMENTOUP, CMD_PORTAMENTODOWN, CMD_TONEPORTAMENTO,
	CMD_VIBRATO, CMD_TONEPORTAVOL, CMD_VIBRATOVOL, CMD_TREMOLO,
	CMD_PANNING8, CMD_OFFSET, CMD_VOLUMESLIDE, CMD_POSITIONJUMP,
	CMD_VOLUME, CMD_PATTERNBREAK, CMD_MODCMDEX, CMD_TEMPO,
	CMD_GLOBALVOLUME, CMD_GLOBALVOLSLIDE, CMD_KEYOFF, CMD_SETENVPOSITION,
	CMD_CHANNELVOLUME, CMD_CHANNELVOLSLIDE, CMD_PANNINGSLIDE,
};


static void ConvertDBMEffect(uint8 &command, uint8 &param)
//--------------------------------------------------------
{
	if(command < CountOf(dbmEffects))
		command = dbmEffects[command];
	else
		command = CMD_NONE;

	switch (command)
	{
	case CMD_ARPEGGIO:
		if(param == 0)
			command = CMD_NONE;
		break;

	// Volume slide nibble priority - first nibble (slide up) has precedence.
	case CMD_VOLUMESLIDE:
	case CMD_TONEPORTAVOL:
	case CMD_VIBRATOVOL:
		if((param & 0xF0) != 0x00 && (param & 0xF0) != 0xF0 && (param & 0x0F) != 0x0F)
			param &= 0xF0;
		break;

	case CMD_GLOBALVOLUME:
		if(param <= 64)
			param *= 2;
		else
			param = 128;
		break;

	case CMD_MODCMDEX:
		switch(param & 0xF0)
		{
		case 0x00:	// set filter
			command = CMD_NONE;
			break;
		case 0x30:	// play backwards
			command = CMD_S3MCMDEX;
			param = 0x9F;
			break;
		case 0x40:	// turn off sound in channel (volume / portamento commands after this can't pick up the note anymore)
			command = CMD_S3MCMDEX;
			param = 0xC0;
			break;
		case 0x50:	// turn on/off channel
			// TODO: Apparently this should also kill the playing note.
			if((param & 0x0F) <= 0x01)
			{
				command = CMD_CHANNELVOLUME;
				param = (param == 0x50) ? 0x00 : 0x40;
			}
			break;
		case 0x60:	// set loop begin / loop
			// TODO
			break;
		case 0x70:	// set offset
			// TODO
			break;
		default:
			// Rest will be converted later from CMD_MODCMDEX to CMD_S3MCMDEX.
			break;
		}
		break;

	case CMD_TEMPO:
		if(param <= 0x1F) command = CMD_SPEED;
		break;

	case CMD_KEYOFF:
		if (param == 0)
		{
			// TODO key of at tick 0
		}
		break;
	}
}


// Read a chunk of volume or panning envelopes
static void ReadDBMEnvelopeChunk(FileReader chunk, enmEnvelopeTypes envType, CSoundFile &sndFile, bool scaleEnv)
//--------------------------------------------------------------------------------------------------------------
{
	uint16 numEnvs = chunk.ReadUint16BE();
	for(uint16 i = 0; i < numEnvs; i++)
	{
		DBMEnvelope dbmEnv;
		chunk.ReadConvertEndianness(dbmEnv);

		ModInstrument *mptIns;
		if(dbmEnv.instrument && dbmEnv.instrument < MAX_INSTRUMENTS && (mptIns = sndFile.Instruments[dbmEnv.instrument]) != nullptr)
		{
			InstrumentEnvelope &mptEnv = mptIns->GetEnvelope(envType);

			if(dbmEnv.numSegments)
			{
				if(dbmEnv.flags & DBMEnvelope::envEnabled) mptEnv.dwFlags.set(ENV_ENABLED);
				if(dbmEnv.flags & DBMEnvelope::envSustain) mptEnv.dwFlags.set(ENV_SUSTAIN);
				if(dbmEnv.flags & DBMEnvelope::envLoop) mptEnv.dwFlags.set(ENV_LOOP);
			}

			mptEnv.nNodes = std::min<uint32>(dbmEnv.numSegments + 1, MAX_ENVPOINTS);

			mptEnv.nLoopStart = dbmEnv.loopBegin;
			mptEnv.nLoopEnd = dbmEnv.loopEnd;
			mptEnv.nSustainStart = mptEnv.nSustainEnd = dbmEnv.sustain1;

			for(uint32 i = 0; i < mptEnv.nNodes; i++)
			{
				mptEnv.Ticks[i] = dbmEnv.data[i * 2];
				uint16 val = dbmEnv.data[i * 2 + 1];
				if(scaleEnv)
				{
					// Panning envelopes are -128...128 in DigiBooster Pro 3.x
					val = (val + 128) / 4;
				}
				LimitMax(val, uint16(64));
				mptEnv.Values[i] = static_cast<uint8>(val);
			}
		}
	}
}


bool CSoundFile::ReadDBM(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	DBMFileHeader fileHeader;

	file.Rewind();
	if(!file.Read(fileHeader)
		|| memcmp(fileHeader.dbm0, "DBM0", 4)
		|| fileHeader.trkVerHi > 3)
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	ChunkReader chunkFile(file);
	ChunkReader::ChunkList<DBMChunk> chunks = chunkFile.ReadChunks<DBMChunk>(1);

	// Globals
	FileReader infoChunk = chunks.GetChunk(DBMChunk::idINFO);
	DBMInfoChunk infoData;
	if(!infoChunk.ReadConvertEndianness(infoData))
	{
		return false;
	}

	InitializeGlobals();
	InitializeChannels();
	m_nType = MOD_TYPE_DBM;
	m_SongFlags = SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS;
	SetModFlag(MSF_COMPATIBLE_PLAY, true);
	m_nChannels = Clamp(infoData.channels, uint16(1), uint16(MAX_BASECHANNELS));	// note: MAX_BASECHANNELS is currently 127, but DBPro 2 supports up to 128 channels, DBPro 3 apparently up to 254.
	m_nInstruments = std::min<INSTRUMENTINDEX>(infoData.instruments, MAX_INSTRUMENTS - 1);
	m_nSamples = std::min<SAMPLEINDEX>(infoData.samples, MAX_SAMPLES - 1);
	madeWithTracker = mpt::String::Print("DigiBooster Pro %1.%2", mpt::fmt::hex(fileHeader.trkVerHi), mpt::fmt::hex(fileHeader.trkVerLo));

	// Name chunk
	FileReader nameChunk = chunks.GetChunk(DBMChunk::idNAME);
	nameChunk.ReadString<mpt::String::maybeNullTerminated>(songName, nameChunk.GetLength());

	// Song chunk
	FileReader songChunk = chunks.GetChunk(DBMChunk::idSONG);
	Order.clear();
	for(size_t i = 0; i < infoData.songs; i++)
	{
		char name[44];
		songChunk.ReadString<mpt::String::maybeNullTerminated>(name, 44);
		if(songName.empty())
		{
			songName = name;
		}
#ifdef DBM_USE_REAL_SUBSONGS
		if(i > 0) Order.AddSequence(false);
		Order.SetSequence(i);
		Order.m_sName = name;
#endif // DBM_USE_REAL_SUBSONGS

		const uint16 numOrders = songChunk.ReadUint16BE();
		const ORDERINDEX startIndex = Order.GetLength();
		Order.resize(startIndex + numOrders + 1, Order.GetInvalidPatIndex());

		for(uint16 ord = 0; ord < numOrders; ord++)
		{
			Order[startIndex + ord] = static_cast<PATTERNINDEX>(songChunk.ReadUint16BE());
		}
	}
#ifdef DBM_USE_REAL_SUBSONGS
	Order.SetSequence(0);
#endif // DBM_USE_REAL_SUBSONGS

	// Read instruments
	FileReader instChunk = chunks.GetChunk(DBMChunk::idINST);
	if(instChunk.IsValid())
	{
		for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++)
		{
			DBMInstrument instrHeader;
			instChunk.ReadConvertEndianness(instrHeader);

			ModInstrument *mptIns = AllocateInstrument(i, instrHeader.sample);
			if(mptIns == nullptr || instrHeader.sample >= MAX_SAMPLES)
			{
				continue;
			}
			ModSample &mptSmp = Samples[instrHeader.sample];

			mpt::String::Read<mpt::String::maybeNullTerminated>(mptIns->name, instrHeader.name);
			mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[instrHeader.sample], instrHeader.name);

			mptIns->nFadeOut = 0;
			mptIns->nPan = static_cast<uint16>(instrHeader.panning + 128);
			LimitMax(mptIns->nPan, uint32(256));
			mptIns->dwFlags.set(INS_SETPANNING);

			// Sample Info
			mptSmp.Initialize();
			mptSmp.nVolume = std::min(instrHeader.volume, uint16(64)) * 4u;
			mptSmp.nC5Speed = instrHeader.sampleRate;

			if(instrHeader.loopLength && (instrHeader.flags & (DBMInstrument::smpLoop | DBMInstrument::smpPingPongLoop)))
			{
				mptSmp.nLoopStart = instrHeader.loopStart;
				mptSmp.nLoopEnd = mptSmp.nLoopStart + instrHeader.loopLength;
				mptSmp.uFlags.set(CHN_LOOP);
				if(instrHeader.flags & DBMInstrument::smpPingPongLoop) mptSmp.uFlags.set(CHN_PINGPONGLOOP);
			}
		}
	}

	// Read envelopes
	ReadDBMEnvelopeChunk(chunks.GetChunk(DBMChunk::idVENV), ENV_VOLUME, *this, false);
	ReadDBMEnvelopeChunk(chunks.GetChunk(DBMChunk::idPENV), ENV_PANNING, *this, fileHeader.trkVerHi > 2);

	// Note-Off cuts samples if there's no envelope.
	for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++)
	{
		if(Instruments[i] != nullptr && !Instruments[i]->VolEnv.dwFlags[ENV_ENABLED])
		{
			Instruments[i]->nFadeOut = 32767;
		}
	}

	// Patterns
	FileReader patternChunk = chunks.GetChunk(DBMChunk::idPATT);
	if(patternChunk.IsValid() && (loadFlags & loadPatternData))
	{
		FileReader patternNameChunk = chunks.GetChunk(DBMChunk::idPNAM);
		patternNameChunk.Skip(1);	// Encoding, should be UTF-8 or ASCII

		for(PATTERNINDEX pat = 0; pat < infoData.patterns; pat++)
		{
			uint16 numRows = patternChunk.ReadUint16BE();
			uint32 packedSize = patternChunk.ReadUint32BE();
			FileReader chunk = patternChunk.GetChunk(packedSize);

			if(Patterns.Insert(pat, numRows))
			{
				continue;
			}

			std::string patName;
			patternNameChunk.ReadString<mpt::String::maybeNullTerminated>(patName, patternNameChunk.ReadUint8());
			Patterns[pat].SetName(patName);

			PatternRow patRow = Patterns[pat].GetRow(0);
			ROWINDEX row = 0;
			while(chunk.AreBytesLeft() && row < numRows)
			{
				const uint8 ch = chunk.ReadUint8();

				if(!ch)
				{
					row++;
					patRow = Patterns[pat].GetRow(row);
					continue;
				}

				ModCommand dummy;
				ModCommand &m = ch <= GetNumChannels() ? patRow[ch - 1] : dummy;

				const uint8 b = chunk.ReadUint8();

				if(b & 0x01)
				{
					uint8 note = chunk.ReadUint8();

					if(note == 0x1F)
						note = NOTE_KEYOFF;
					else if(note > 0 && note < 0xFE)
					{
						note = ((note >> 4) * 12) + (note & 0x0F) + 13;
					}
					m.note = note;
				}
				if(b & 0x02)
				{
					m.instr = chunk.ReadUint8();
				}
				if(b & 0x3C)
				{
					uint8 cmd1 = CMD_NONE, cmd2 = CMD_NONE;
					uint8 param1 = 0, param2 = 0;
					if(b & 0x04) cmd2 = chunk.ReadUint8();
					if(b & 0x08) param2 = chunk.ReadUint8();
					if(b & 0x10) cmd1 = chunk.ReadUint8();
					if(b & 0x20) param1 = chunk.ReadUint8();
					ConvertDBMEffect(cmd1, param1);
					ConvertDBMEffect(cmd2, param2);

					// this is the same conversion algorithm as in the ULT loader. maybe this should be merged at some point...
					if (cmd2 == CMD_VOLUME || (cmd2 == CMD_NONE && cmd1 != CMD_VOLUME))
					{
						std::swap(cmd1, cmd2);
						std::swap(param1, param2);
					}

					int n;
					for(n = 0; n < 4; n++)
					{
						if(ModCommand::ConvertVolEffect(cmd1, param1, (n >> 1) != 0))
						{
							n = 5;
							break;
						}
						std::swap(cmd1, cmd2);
						std::swap(param1, param2);
					}
					if(n < 5)
					{
						if (ModCommand::GetEffectWeight((ModCommand::COMMAND)cmd1) > ModCommand::GetEffectWeight((ModCommand::COMMAND)cmd2))
						{
							std::swap(cmd1, cmd2);
							std::swap(param1, param2);
						}
						cmd1 = CMD_NONE;
					}
					if(cmd1 == CMD_NONE)
						param1 = 0;
					if(cmd2 == CMD_NONE)
						param2 = 0;

					m.volcmd = cmd1;
					m.vol = param1;
					m.command = cmd2;
					m.param = param2;
#ifdef MODPLUG_TRACKER
					m.ExtendedMODtoS3MEffect();
#endif // MODPLUG_TRACKER
				}
			}
		}
	}

	// Samples
	FileReader sampleChunk = chunks.GetChunk(DBMChunk::idSMPL);
	if(sampleChunk.IsValid() && (loadFlags & loadSampleData))
	{
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			uint32 sampleFlags = sampleChunk.ReadUint32BE();
			uint32 sampleLength = sampleChunk.ReadUint32BE();

			ModSample &sample = Samples[smp];
			sample.nLength = sampleLength;

			if(sampleFlags & 7)
			{
				SampleIO(
					(sampleFlags & 4) ? SampleIO::_32bit : ((sampleFlags & 2) ? SampleIO::_16bit : SampleIO::_8bit),
					SampleIO::mono,
					SampleIO::bigEndian,
					SampleIO::signedPCM)
					.ReadSample(sample, sampleChunk);
			}
		}
	}

#if !defined(NO_MP3_SAMPLES) && 0
	// Compressed samples - this does not quite work yet...
	FileReader mpegChunk = chunks.GetChunk(DBMChunk::idMPEG);
	if(mpegChunk.IsValid() && (loadFlags & loadSampleData))
	{
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			Samples[smp].nLength = mpegChunk.ReadUint32BE();
		}
		mpegChunk.Skip(2);	// 0x00 0x40

		// Read whole MPEG stream into one sample and then split it up.
		FileReader chunk = mpegChunk.GetChunk(mpegChunk.BytesLeft());
		if(ReadMP3Sample(0, chunk))
		{
			ModSample &srcSample = Samples[0];
			const uint8 *smpData = static_cast<uint8 *>(srcSample.pSample);

			for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
			{
				ModSample &sample = Samples[smp];
				sample.uFlags.set(srcSample.uFlags);
				sample.nLength *= 2;
				LimitMax(sample.nLength, srcSample.nLength);
				if(sample.nLength)
				{
					sample.AllocateSample();
					memcpy(sample.pSample, smpData, sample.GetSampleSizeInBytes());
					CSoundFile::AdjustSampleLoop(sample);
					smpData += sample.GetSampleSizeInBytes();
					srcSample.nLength -= sample.nLength;
				}
			}
			srcSample.FreeSample();
		}
	}
#endif // NO_MP3_SAMPLES
	
	return true;
}
