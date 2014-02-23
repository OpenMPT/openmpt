/*
 * Load_amf.cpp
 * ------------
 * Purpose: AMF module loader
 * Notes  : There are two types of AMF files, the ASYLUM Music Format (used in Crusader: No Remorse and Crusader: No Regret)
 *          and Advanced Music Format (DSMI / Digital Sound And Music Interface, used in various games such as Pinball World).
 *          Both module types are handled here.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

// ASYLUM AMF File Header
struct PACKED AsylumFileHeader
{
	char  signature[32];
	uint8 defaultSpeed;
	uint8 defaultTempo;
	uint8 numSamples;
	uint8 numPatterns;
	uint8 numOrders;
	uint8 restartPos;
};

STATIC_ASSERT(sizeof(AsylumFileHeader) == 38);


// ASYLUM AMF Sample Header
struct PACKED AsylumSampleHeader
{
	char   name[22];
	uint8  finetune;
	uint8  defaultVolume;
	int8   transpose;
	uint32 length;
	uint32 loopStart;
	uint32 loopLength;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopLength);
	}

	// Convert an AMF sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();
		mptSmp.nFineTune = MOD2XMFineTune(finetune);
		mptSmp.nVolume = std::min(defaultVolume, uint8(64)) * 4u;
		mptSmp.RelativeTone = transpose;
		mptSmp.nLength = length;

		if(loopLength > 2 && loopStart + loopLength <= length)
		{
			mptSmp.uFlags.set(CHN_LOOP);
			mptSmp.nLoopStart = loopStart;
			mptSmp.nLoopEnd = loopStart + loopLength;
		}
	}
};

STATIC_ASSERT(sizeof(AsylumSampleHeader) == 37);


// DSMI AMF File Header
struct PACKED AMFFileHeader
{
	char   amf[3];
	uint8  version;
	char   title[32];
	uint8  numSamples;
	uint8  numOrders;
	uint16 numTracks;
	uint8  numChannels;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(numTracks);
	}
};

STATIC_ASSERT(sizeof(AMFFileHeader) == 41);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadAMF_Asylum(FileReader &file, ModLoadingFlags loadFlags)
//--------------------------------------------------------------------------
{
	file.Rewind();

	AsylumFileHeader fileHeader;
	if(!file.Read(fileHeader)
		|| strncmp(fileHeader.signature, "ASYLUM Music Format V1.0", 25)
		|| fileHeader.numSamples > 64
		|| !file.CanRead(256 + 64 * sizeof(AsylumSampleHeader) + 64 * 4 * 8 * fileHeader.numPatterns))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals();
	InitializeChannels();
	m_nType = MOD_TYPE_AMF0;
	m_nChannels = 8;
	m_nDefaultSpeed = fileHeader.defaultSpeed;
	m_nDefaultTempo = fileHeader.defaultTempo;
	m_nSamples = fileHeader.numSamples;
	if(fileHeader.restartPos < fileHeader.numOrders)
	{
		m_nRestartPos = fileHeader.restartPos;
	}
	songName = "";

	Order.ReadAsByte(file, 256, fileHeader.numOrders);

	// Read Sample Headers
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		AsylumSampleHeader sampleHeader;
		file.ReadConvertEndianness(sampleHeader);
		sampleHeader.ConvertToMPT(Samples[smp]);
		mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[smp], sampleHeader.name);
	}

	file.Skip((64 - fileHeader.numSamples) * sizeof(AsylumSampleHeader));

	// Read Patterns
	for(PATTERNINDEX pat = 0; pat < fileHeader.numPatterns; pat++)
	{
		if(!(loadFlags & loadPatternData) || Patterns.Insert(pat, 64))
		{
			file.Skip(64 * 4 * 8);
			continue;
		}

		ModCommand *p = Patterns[pat];
		for(size_t i = 0; i < 8 * 64; i++, p++)
		{
			uint8 data[4];
			file.ReadArray(data);

			p->note = NOTE_NONE;
			if(data[0] && data[0] + 12 + NOTE_MIN <= NOTE_MAX)
			{
				p->note = data[0] + 12 + NOTE_MIN;
			}
			p->instr = data[1];
			p->command = data[2];
			p->param = data[3];
			ConvertModCommand(*p);
		}
	}

	if(loadFlags & loadSampleData)
	{
		// Read Sample Data
		const SampleIO sampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::signedPCM);

		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			sampleIO.ReadSample(Samples[smp], file);
		}
	}

	return true;
}


// Read a single AMF track (channel) into a pattern.
static void AMFReadPattern(CPattern &pattern, CHANNELINDEX chn, FileReader &fileChunk)
//------------------------------------------------------------------------------------
{
	fileChunk.Rewind();
	ModCommand::INSTR lastInstr = 0;
	while(fileChunk.AreBytesLeft())
	{
		const uint8 row = fileChunk.ReadUint8();
		const uint8 command = fileChunk.ReadUint8();
		const uint8 value = fileChunk.ReadUint8();
		if(row >= pattern.GetNumRows())
		{
			break;
		}

		ModCommand &m = *pattern.GetpModCommand(row, chn);
		if(command < 0x7F)
		{
			// Note + Volume
			if(command == 0 && value == 0)
			{
				m.note = NOTE_NOTECUT;
			} else
			{
				m.note = command + NOTE_MIN;
				if(value != 0xFF)
				{
					if(!m.instr) m.instr = lastInstr;
					m.volcmd = VOLCMD_VOLUME;
					m.vol = value;
				}
			}
		} else if(command == 0x7F)
		{
			// Duplicate row
			int8 rowDelta = static_cast<int8>(value);
			int16 copyRow = static_cast<int16>(row) + rowDelta;
			if(copyRow >= 0 && copyRow < static_cast<int16>(pattern.GetNumRows()))
			{
				m = *pattern.GetpModCommand(copyRow, chn);
			}
		} else if(command == 0x80)
		{
			// Instrument
			m.instr = value + 1;
			lastInstr = m.instr;
		} else
		{
			// Effect
			static const ModCommand::COMMAND effTrans[] =
			{
				CMD_NONE,			CMD_SPEED,			CMD_VOLUMESLIDE,		CMD_VOLUME,
				CMD_PORTAMENTOUP,	CMD_NONE,			CMD_TONEPORTAMENTO,		CMD_TREMOR,
				CMD_ARPEGGIO,		CMD_VIBRATO,		CMD_TONEPORTAVOL,		CMD_VIBRATOVOL,
				CMD_PATTERNBREAK,	CMD_POSITIONJUMP,	CMD_NONE,				CMD_RETRIG,
				CMD_OFFSET,			CMD_VOLUMESLIDE,	CMD_PORTAMENTOUP,		CMD_S3MCMDEX,
				CMD_S3MCMDEX,		CMD_TEMPO,			CMD_PORTAMENTOUP,		CMD_PANNING8,
			};

			uint8 cmd = (command & 0x7F);
			uint8 param = value;

			if(cmd < CountOf(effTrans))
			{
				cmd = effTrans[cmd];
			} else
			{
				cmd = CMD_NONE;
			}

			// Fix some commands...
			switch(command & 0x7F)
			{
			// 02: Volume Slide
			// 0A: Tone Porta + Vol Slide
			// 0B: Vibrato + Vol Slide
			case 0x02:
			case 0x0A:
			case 0x0B:
				if(param & 0x80)
					param = (-static_cast<int8>(param)) & 0x0F;
				else
					param = (param & 0x0F) << 4;
				break;

			// 03: Volume
			case 0x03:
				param = std::min(param, uint8(64));
				if(m.volcmd == VOLCMD_NONE || m.volcmd == VOLCMD_VOLUME)
				{
					m.volcmd = VOLCMD_VOLUME;
					m.vol = param;
					cmd = CMD_NONE;
				}
				break;

			// 04: Porta Up/Down
			case 0x04:
				if(param & 0x80)
					param = (-static_cast<int8>(param)) & 0x7F;
				else
					cmd = CMD_PORTAMENTODOWN;
				break;

			// 11: Fine Volume Slide
			case 0x11:
				if(param)
				{
					if(param & 0x80)
						param = 0xF0 | ((-static_cast<int8>(param)) & 0x0F);
					else
						param = 0x0F | ((param & 0x0F) << 4);
				} else
				{
					cmd = CMD_NONE;
				}
				break;

			// 12: Fine Portamento
			// 16: Extra Fine Portamento
			case 0x12:
			case 0x16:
				if(param)
				{
					cmd = static_cast<uint8>((param & 0x80) ? CMD_PORTAMENTOUP : CMD_PORTAMENTODOWN);
					if(param & 0x80)
					{
						param = ((-static_cast<int8>(param)) & 0x0F);
					}
					param |= (command == 0x16) ? 0xE0 : 0xF0;
				} else
				{
					cmd = CMD_NONE;
				}
				break;

			// 13: Note Delay
			case 0x13:
				param = 0xD0 | (param & 0x0F);
				break;

			// 14: Note Cut
			case 0x14:
				param = 0xC0 | (param & 0x0F);
				break;

			// 17: Panning
			case 0x17:
				param = (param + 64) & 0x7F;
				if(m.command != CMD_NONE)
				{
					if(m.volcmd == VOLCMD_NONE || m.volcmd == VOLCMD_PANNING)
					{
						m.volcmd = VOLCMD_PANNING;
						m.vol = param / 2;
					}
					cmd = CMD_NONE;
				}
				break;
			}

			if(cmd != CMD_NONE)
			{
				m.command = cmd;
				m.param = param;
			}
		}
	}
}


bool CSoundFile::ReadAMF_DSMI(FileReader &file, ModLoadingFlags loadFlags)
//------------------------------------------------------------------------
{
	file.Rewind();

	AMFFileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| memcmp(fileHeader.amf, "AMF", 3)
		|| fileHeader.version < 8 || fileHeader.version > 14
		|| ((fileHeader.numChannels < 1 || fileHeader.numChannels > 32) && fileHeader.version >= 10))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals();
	InitializeChannels();

	m_nType = MOD_TYPE_AMF;
	m_nChannels = fileHeader.numChannels;
	m_nSamples = fileHeader.numSamples;

	mpt::String::Read<mpt::String::maybeNullTerminated>(songName, fileHeader.title);

	if(fileHeader.version < 10)
	{
		// Old format revisions are fixed to 4 channels
		m_nChannels = 4;
		file.SkipBack(1);
		SetupMODPanning(true);
	}

	// Setup Channel Pan Positions
	if(fileHeader.version >= 11)
	{
		const CHANNELINDEX readChannels = fileHeader.version >= 12 ? 32 : 16;
		for(CHANNELINDEX chn = 0; chn < readChannels; chn++)
		{
			int16 pan = (file.ReadInt8() + 64) * 2;
			if(pan < 0) pan = 0;
			if(pan > 256)
			{
				pan = 128;
				ChnSettings[chn].dwFlags = CHN_SURROUND;
			}
			ChnSettings[chn].nPan = static_cast<uint16>(pan);
		}
	} else if(fileHeader.version == 10)
	{
		uint8 panPos[16];
		file.ReadArray(panPos);
		for(CHANNELINDEX chn = 0; chn < 16; chn++)
		{
			ChnSettings[chn].nPan = (panPos[chn] & 1) ? 0x40 : 0xC0;
		}
	}
	// To check: Was the channel table introduced in revision 1.0 or 0.9? I only have 0.8 files, in which it is missing...
	ASSERT(fileHeader.version != 9);

	// Get Tempo/Speed
	if(fileHeader.version >= 13)
	{
		m_nDefaultTempo = file.ReadUint8();
		m_nDefaultSpeed = file.ReadUint8();
		if(m_nDefaultTempo < 32)
		{
			m_nDefaultTempo = 125;
		}
	} else
	{
		m_nDefaultTempo = 125;
		m_nDefaultSpeed = 6;
	}

	// Setup Order List
	Order.resize(fileHeader.numOrders);
	std::vector<ROWINDEX> patternLength(fileHeader.numOrders, 64);
	const FileReader::off_t trackStartPos = file.GetPosition() + (fileHeader.version >= 14 ? 2 : 0);

	for(ORDERINDEX ord = 0; ord < fileHeader.numOrders; ord++)
	{
		Order[ord] = ord;
		if(fileHeader.version >= 14)
		{
			patternLength[ord] = file.ReadUint16LE();
		}
		// Track positions will be read as needed.
		file.Skip(m_nChannels * 2);
	}

	// Read Sample Headers
	std::vector<uint32> samplePos(GetNumSamples(), 0);
	uint32 maxSamplePos = 0;

	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		ModSample &sample = Samples[smp];
		sample.Initialize();

		uint8 type = file.ReadUint8();
		file.ReadString<mpt::String::maybeNullTerminated>(m_szNames[smp], 32);
		file.ReadString<mpt::String::nullTerminated>(sample.filename, 13);
		samplePos[smp - 1] = file.ReadUint32LE();
		if(fileHeader.version < 10)
		{
			sample.nLength = file.ReadUint16LE();
		} else
		{
			sample.nLength = file.ReadUint32LE();
		}

		sample.nC5Speed = file.ReadUint16LE();
		sample.nVolume = std::min(file.ReadUint8(), uint8(64)) * 4u;

		if(fileHeader.version < 10)
		{
			// Various sources (Miodrag Vallat's amf.txt, old ModPlug code) suggest that the loop information
			// format revision 1.0 should only consist of a 16-bit value for the loop start (loop end would
			// automatically equal sample length), but the only v1.0 files I have ("the tribal zone" and
			// "the way its gonna b" by Maelcum) do not confirm this - the sample headers are laid out exactly
			// as in the newer revisions in these two files. Even in format revision 0.8 (output by MOD2AMF v1.02)
			// There are loop start and loop end values (although they are 16-Bit). Maybe this only applies to
			// even older revision of the format?
			sample.nLoopStart = file.ReadUint16LE();
			sample.nLoopEnd = file.ReadUint16LE();
		} else
		{
			sample.nLoopStart = file.ReadUint32LE();
			sample.nLoopEnd = file.ReadUint32LE();
		}

		// Length of v1.0+ sample header: 65 bytes
		// Length of old sample header: 59 bytes

		if(type != 0)
		{
			if(sample.nLoopEnd > sample.nLoopStart + 2 && sample.nLoopEnd <= sample.nLength)
			{
				sample.uFlags.set(CHN_LOOP);
			} else
			{
				sample.nLoopStart = sample.nLoopEnd = 0;
			}

			maxSamplePos = std::max(maxSamplePos, samplePos[smp - 1]);
		}
	}
	
	// Read Track Mapping Table
	std::vector<uint16> trackMap;
	file.ReadVectorLE(trackMap, fileHeader.numTracks);
	uint16 trackCount = 0;
	for(std::vector<uint16>::const_iterator i = trackMap.begin(); i != trackMap.end(); i++)
	{
		trackCount = std::max(trackCount, *i);
	}

	// Store Tracks Positions
	std::vector<FileReader> trackData(trackCount);
	for(uint16 i = 0; i < trackCount; i++)
	{
		// Track size is a 24-Bit value describing the number of byte triplets in this track.
		uint32 trackSize = file.ReadUint16LE() | (file.ReadUint8() << 16);
		trackData[i] = file.GetChunk(trackSize * 3);
	}

	if(loadFlags & loadSampleData)
	{
		// Read Sample Data
		const SampleIO sampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::unsignedPCM);

		// Why is all of this sample loading business so dumb in AMF?
		// Surely there must be some great idea behind it which isn't handled here (re-using the same sample data for different sample slots maybe?)
		for(uint32 seekPos = 1; seekPos <= maxSamplePos; seekPos++)
		{
			for(SAMPLEINDEX smp = 0; smp < GetNumSamples(); smp++)
			{
				if(seekPos == samplePos[smp])
				{
					sampleIO.ReadSample(Samples[smp + 1], file);
					break;
				}
			}
			if(file.NoBytesLeft())
			{
				break;
			}
		}
	}

	if(!(loadFlags & loadPatternData))
	{
		return true;
	}

	// Create the patterns from the list of tracks
	for(PATTERNINDEX pat = 0; pat < fileHeader.numOrders; pat++)
	{
		if(Patterns.Insert(pat, patternLength[pat]))
		{
			continue;
		}

		// Get table with per-channel track assignments
		file.Seek(trackStartPos + pat * (GetNumChannels() * 2 + (fileHeader.version >= 14 ? 2 : 0)));
		std::vector<uint16> tracks;
		file.ReadVectorLE(tracks, GetNumChannels());

		for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++)
		{
			if(tracks[chn] > 0 && tracks[chn] <= fileHeader.numTracks)
			{
				uint16 realTrack = trackMap[tracks[chn] - 1];
				if(realTrack > 0 && realTrack <= trackCount)
				{
					realTrack--;
					AMFReadPattern(Patterns[pat], chn, trackData[realTrack]);
				}
			}
		}
	}

	return true;
}
