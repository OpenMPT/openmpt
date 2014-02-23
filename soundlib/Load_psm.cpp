/*
 * Load_psm.cpp
 * ------------
 * Purpose: PSM16 and new PSM (ProTracker Studio) module loader
 * Notes  : This is partly based on http://www.shikadi.net/moddingwiki/ProTracker_Studio_Module
 *          and partly reverse-engineered. Also thanks to the author of foo_dumb, the source code gave me a few clues. :)
 *
 *          What's playing?
 *          - Epic Pinball - Perfect! (menu and order song are pitched up a bit in the PSM16 format for unknown reasons, but that shouldn't bother anyone)
 *          - Extreme Pinball - Perfect! (subtunes included!)
 *          - Jazz Jackrabbit - Perfect!
 *          - One Must Fall! - Perfect! (it helped a lot to have the original MTM files...)
 *          - Silverball - Seems to work (I don't have all tables so I can't compare)
 *          - Sinaria - Seems to work (never played the game, so I can't really tell...)
 *
 *          Effect conversion should be about right...
 *          If OpenMPT will ever support subtunes properly (with restart position, channel setup, etc. for each subtune), the subtune crap should be rewritten completely.
 * Authors: Johannes Schultz
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "ChunkReader.h"

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

////////////////////////////////////////////////////////////
//
//  New PSM support starts here. PSM16 structs are below.
//

// PSM File Header
struct PACKED PSMFileHeader
{
	char   formatID[4];		// "PSM " (new format)
	uint32 fileSize;		// Filesize - 12
	char   fileInfoID[4];	// "FILE"

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(fileSize);
	}
};

STATIC_ASSERT(sizeof(PSMFileHeader) == 12);

// RIFF-style Chunk
struct PACKED PSMChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idTITL	= MAGIC4LE('T','I','T','L'),
		idSDFT	= MAGIC4LE('S','D','F','T'),
		idPBOD	= MAGIC4LE('P','B','O','D'),
		idSONG	= MAGIC4LE('S','O','N','G'),
		idDATE	= MAGIC4LE('D','A','T','E'),
		idOPLH	= MAGIC4LE('O','P','L','H'),
		idPPAN	= MAGIC4LE('P','P','A','N'),
		idPATT	= MAGIC4LE('P','A','T','T'),
		idDSAM	= MAGIC4LE('D','S','A','M'),
		idDSMP	= MAGIC4LE('D','S','M','P'),
	};

	typedef ChunkIdentifiers id_type;

	uint32 id;
	uint32 length;

	size_t GetLength() const
	{
		return SwapBytesReturnLE(length);
	}

	id_type GetID() const
	{
		return static_cast<id_type>(SwapBytesReturnLE(id));
	}
};

STATIC_ASSERT(sizeof(PSMChunk) == 8);

// Song Information
struct PACKED PSMSongHeader
{
	char  songType[9];		// Mostly "MAINSONG " (But not in Extreme Pinball!)
	uint8 compression;		// 1 - uncompressed
	uint8 numChannels;		// Number of channels, usually 4

};

STATIC_ASSERT(sizeof(PSMSongHeader) == 11);

// Regular sample header
struct PACKED PSMOldSampleHeader
{
	uint8  flags;
	char   fileName[8];		// Filename of the original module (without extension)
	char   sampleID[4];		// INS0...INS9 (only last digit of sample ID, i.e. sample 1 and sample 11 are equal)
	char   sampleName[33];
	uint8  unknown1[6];		// 00 00 00 00 00 FF
	uint16 sampleNumber;
	uint32 sampleLength;
	uint32 loopStart;
	uint32 loopEnd;			// FF FF FF FF = end of sample
	uint8  unknown3;
	uint8  defaulPan;		// unused?
	uint8  defaultVolume;
	uint32 unknown4;
	uint16 c5Freq;
	uint8  unknown5[21];	// 00 ... 00

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(sampleNumber);
		SwapBytesLE(sampleLength);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(c5Freq);
	}

	// Convert header data to OpenMPT's internal format
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mpt::String::Read<mpt::String::maybeNullTerminated>(mptSmp.filename, fileName);

		mptSmp.nGlobalVol = 64;
		mptSmp.nC5Speed = c5Freq;
		mptSmp.nLength = sampleLength;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;	// Hmm... apparently we should add +1 for Extreme Pinball tunes here? See sample 8 in the medieval table music.
		mptSmp.nPan = 128;
		mptSmp.nVolume = (defaultVolume + 1) * 2;
		mptSmp.uFlags.set(CHN_LOOP, (flags & 0x80) != 0);
		LimitMax(mptSmp.nLoopEnd, mptSmp.nLength);
		LimitMax(mptSmp.nLoopStart, mptSmp.nLoopEnd);
	}

	// Retrieve the internal sample format flags for this sample.
	static SampleIO GetSampleFormat()
	{
		return SampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::deltaPCM);
	}
};

STATIC_ASSERT(sizeof(PSMOldSampleHeader) == 96);

// Sinaria sample header (and possibly other games)
struct PACKED PSMNewSampleHeader
{
	uint8  flags;
	char   fileName[8];		// Filename of the original module (without extension)
	char   sampleID[8];		// INS0...INS99999
	char   sampleName[33];
	uint8  unknown1[6];		// 00 00 00 00 00 FF
	uint16 sampleNumber;
	uint32 sampleLength;
	uint32 loopStart;
	uint32 loopEnd;
	uint16 unknown3;
	uint8  defaultPan;		// unused?
	uint8  defaultVolume;
	uint32 unknown4;
	uint16 c5Freq;
	char   unknown5[16];	// 00 ... 00

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(sampleNumber);
		SwapBytesLE(sampleLength);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(c5Freq);
	}

	// Convert header data to OpenMPT's internal format
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mpt::String::Read<mpt::String::maybeNullTerminated>(mptSmp.filename, fileName);

		mptSmp.nGlobalVol = 64;
		mptSmp.nC5Speed = c5Freq;
		mptSmp.nLength = sampleLength;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;
		mptSmp.nPan = 128;
		mptSmp.nVolume = (defaultVolume + 1) * 2;
		mptSmp.uFlags.set(CHN_LOOP, (flags & 0x80) != 0);
		LimitMax(mptSmp.nLoopEnd, mptSmp.nLength);
		LimitMax(mptSmp.nLoopStart, mptSmp.nLoopEnd);
	}

	// Retrieve the internal sample format flags for this sample.
	static SampleIO GetSampleFormat()
	{
		return SampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::deltaPCM);
	}
};

STATIC_ASSERT(sizeof(PSMNewSampleHeader) == 96);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


struct PSMSubSong // For internal use (pattern conversion)
{
	std::vector<uint8> channelPanning, channelVolume;
	std::vector<bool> channelSurround;
	uint8 defaultTempo, defaultSpeed;
	char  songName[10];
	ORDERINDEX startOrder, endOrder, restartPos;

	PSMSubSong()
	{
		channelPanning.assign(MAX_BASECHANNELS, 128);
		channelVolume.assign(MAX_BASECHANNELS, 64);
		channelSurround.assign(MAX_BASECHANNELS, false);
		MemsetZero(songName);
		defaultTempo = 125;
		defaultSpeed = 6;
		startOrder = endOrder = restartPos = ORDERINDEX_INVALID;
	}
};


// Portamento effect conversion (depending on format version)
static uint8 ConvertPSMPorta(uint8 param, bool newFormat)
//-------------------------------------------------------
{
	return (newFormat
		? param
		: ((param < 4)
			? (param | 0xF0)
			: (param >> 2)));
}


bool CSoundFile::ReadPSM(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();
	PSMFileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader))
	{
		return false;
	}

	bool newFormat = false; // The game "Sinaria" uses a slightly modified PSM structure

	// Check header
	if(memcmp(fileHeader.formatID, "PSM ", 4)
		|| fileHeader.fileSize != file.BytesLeft()
		|| memcmp(fileHeader.fileInfoID, "FILE", 4))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	// Yep, this seems to be a valid file.
	InitializeGlobals();
	m_nType = MOD_TYPE_PSM;
	m_SongFlags = SONG_ITOLDEFFECTS | SONG_ITCOMPATGXX;
	SetModFlag(MSF_COMPATIBLE_PLAY, true);

	// pattern offset and identifier
	PATTERNINDEX numPatterns = 0;		// used for setting up the orderlist - final pattern count
	std::vector<FileReader> patternChunks;	// pattern offsets (sorted as they occour in the file)
	std::vector<uint32> patternIDs;			// pattern IDs (sorted as they occour in the file)
	std::vector<FileReader *> orderOffsets;	// combine the upper two vectors to get the offsets for each order item
	Order.clear();
	// subsong setup
	std::vector<PSMSubSong> subsongs;
	bool subsongPanningDiffers = false; // do we have subsongs with different panning positions?

	ChunkReader chunkFile(file);
	ChunkReader::ChunkList<PSMChunk> chunks = chunkFile.ReadChunks<PSMChunk>(1);

	// "TITL" - Song Title
	FileReader titleChunk = chunks.GetChunk(PSMChunk::idTITL);
	titleChunk.ReadString<mpt::String::spacePadded>(songName, titleChunk.GetLength());

	// "SDFT" - Format info (song data starts here)
	if(!chunks.GetChunk(PSMChunk::idSDFT).ReadMagic("MAINSONG"))
	{
		return false;
	}

	// "PBOD" - Pattern data of a single pattern
	std::vector<FileReader> pattChunks = chunks.GetAllChunks(PSMChunk::idPBOD);
	for(std::vector<FileReader>::iterator patternIter = pattChunks.begin(); patternIter != pattChunks.end(); patternIter++)
	{
		ChunkReader chunk(*patternIter);
		if(chunk.GetLength() != chunk.ReadUint32LE()	// Same value twice
			|| chunk.GetLength() < 8)
		{
			continue;
		}

		// Pattern ID (something like "P0  " or "P13 ", or "PATT0   " in Sinaria) follows
		char patternID[5];
		if(!chunk.ReadString<mpt::String::spacePadded>(patternID, 4) || patternID[0] != 'P')
		{
			continue;
		}
		if(!memcmp(patternID, "PATT", 4))
		{
			// New format has four additional bytes - read patternID again.
			newFormat = true;
			chunk.ReadString<mpt::String::spacePadded>(patternID, 4);
		}
		patternIDs.push_back(ConvertStrTo<uint32>(&patternID[newFormat ? 0 : 1]));
		// We're going to read the rest of the pattern data later.
		patternChunks.push_back(chunk.GetChunk(chunk.BytesLeft()));

		// Convert later as we have to know how many channels there are.
	}

	// "SONG" - Subsong information (channel count etc)
	std::vector<FileReader> songChunks = chunks.GetAllChunks(PSMChunk::idSONG);
	if(songChunks.empty())
	{
		return false;
	}

	for(std::vector<FileReader>::iterator subsongIter = songChunks.begin(); subsongIter != songChunks.end(); subsongIter++)
	{
		ChunkReader chunk(*subsongIter);
		PSMSongHeader songHeader;
		if(!chunk.Read(songHeader))
		{
			return false;
		}
		if(songHeader.compression != 0x01)
		{
			// No compression for PSM files
			return false;
		}
		// Subsongs *might* have different channel count
		m_nChannels = Clamp(static_cast<CHANNELINDEX>(songHeader.numChannels), m_nChannels, MAX_BASECHANNELS);

		PSMSubSong subsong;
		subsong.restartPos = (ORDERINDEX)Order.size(); // restart order "offset": current orderlist length
		mpt::String::Read<mpt::String::nullTerminated>(subsong.songName, songHeader.songType); // subsong name

		// Read "Sub sub chunks"
		ChunkReader::ChunkList<PSMChunk> subChunks = chunk.ReadChunks<PSMChunk>(1);
		
		for(ChunkReader::ChunkList<PSMChunk>::iterator subChunkIter = subChunks.begin(); subChunkIter != subChunks.end(); subChunkIter++)
		{
			FileReader subChunk(subChunkIter->GetData());
			PSMChunk subChunkHead = subChunkIter->GetHeader();
			
			switch(subChunkHead.id)
			{
			case PSMChunk::idDATE: // "DATE" - Conversion date (YYMMDD)
				if(subChunkHead.length == 6)
				{
					char cversion[7];
					subChunk.ReadString<mpt::String::maybeNullTerminated>(cversion, 6);
					int version = ConvertStrTo<int>(cversion);
					// Sinaria song dates (just to go sure...)
					if(version == 800211 || version == 940902 || version == 940903 ||
						version == 940906 || version == 940914 || version == 941213)
						newFormat = true;
				}
				break;

			case PSMChunk::idOPLH: // "OPLH" - Order list, channel + module settings
				if(subChunkHead.length >= 9)
				{
					// First two bytes = Number of chunks that follow
					//uint16 totalChunks = subChunk.ReadUint16LE();
					subChunk.Skip(2);

					// Now, the interesting part begins!
					uint16 chunkCount = 0, firstOrderChunk = uint16_max;

					// "Sub sub sub chunks" (grrrr, silly format)
					while(subChunk.AreBytesLeft())
					{
						uint8 subChunkID = subChunk.ReadUint8();
						if(!subChunkID)
						{
							// Last chunk was reached.
							break;
						}

						switch(subChunkID)
						{
						case 0x01: // Order list item
							// Pattern name follows - find pattern (this is the orderlist)
							{
								char patternID[5]; // temporary
								if(newFormat)
								{
									subChunk.Skip(4);
									subChunk.ReadString<mpt::String::spacePadded>(patternID, 4);
								} else
								{
									subChunk.Skip(1);
									subChunk.ReadString<mpt::String::spacePadded>(patternID, 3);
								}
								uint32 pat = ConvertStrTo<uint32>(patternID);

								// seek which pattern has this ID
								for(size_t i = 0; i < patternIDs.size(); i++)
								{
									if(patternIDs[i] == pat)
									{
										// found the right pattern, copy offset + start / end positions.
										if(subsong.startOrder == ORDERINDEX_INVALID)
											subsong.startOrder = static_cast<ORDERINDEX>(orderOffsets.size());
										subsong.endOrder = static_cast<ORDERINDEX>(orderOffsets.size());

										// every pattern in the order will be unique, so store the pointer + pattern ID
										orderOffsets.push_back(&patternChunks[i]);
										Order.Append(numPatterns);
										numPatterns++;
										break;
									}
								}
							}
							// Decide whether this is the first order chunk or not (for finding out the correct restart position)
							if(firstOrderChunk == uint16_max) firstOrderChunk = chunkCount;
							break;

						case 0x04: // Restart position
							{
								uint16 restartChunk = subChunk.ReadUint16LE();
								ORDERINDEX restartPosition = 0;
								if(restartChunk >= firstOrderChunk) restartPosition = static_cast<ORDERINDEX>(restartChunk - firstOrderChunk);
								subsong.restartPos += restartPosition;
							}
							break;

						case 0x07: // Default Speed
							subsong.defaultSpeed = subChunk.ReadUint8();
							break;

						case 0x08: // Default Tempo
							subsong.defaultTempo =  subChunk.ReadUint8();
							break;

						case 0x0C: // Sample map table (???)
							// Never seems to be different, so...
							if (subChunk.ReadUint8() != 0x00 || subChunk.ReadUint8() != 0xFF ||
								subChunk.ReadUint8() != 0x00 || subChunk.ReadUint8() != 0x00 ||
								subChunk.ReadUint8() != 0x01 || subChunk.ReadUint8() != 0x00)
							{
								return false;
							}
							break;

						case 0x0D: // Channel panning table
							{
								uint8 chn = subChunk.ReadUint8();
								uint8 pan = subChunk.ReadUint8();
								uint8 type = subChunk.ReadUint8();
								if(chn < subsong.channelPanning.size())
								{
									switch(type)
									{
									case 0: // use panning
										subsong.channelPanning[chn] = pan ^ 128;
										subsong.channelSurround[chn] = false;
										break;

									case 2: // surround
										subsong.channelPanning[chn] = 128;
										subsong.channelSurround[chn] = true;
										break;

									case 4: // center
										subsong.channelPanning[chn] = 128;
										subsong.channelSurround[chn] = false;
										break;

									}
									if(subsongPanningDiffers == false && subsongs.size() > 0)
									{
										if(subsongs.back().channelPanning[chn] != subsong.channelPanning[chn]
										|| subsongs.back().channelSurround[chn] != subsong.channelSurround[chn])
											subsongPanningDiffers = true;
									}
								}
							}
							break;

						case 0x0E: // Channel volume table (0...255) - apparently always 255
							{
								uint8 chn = subChunk.ReadUint8();
								uint8 vol = subChunk.ReadUint8();
								if(chn < subsong.channelVolume.size())
								{
									subsong.channelVolume[chn] = (vol / 4) + 1;
								}
							}
							break;

						default: // How the hell should this happen? I've listened through almost all existing (original) PSM files. :)
							// anyway, in such cases, we have to quit as we don't know how big the chunk really is.
							return false;

						}
						chunkCount++;
					}
					// separate subsongs by "---" patterns
					orderOffsets.push_back(nullptr);
					Order.Append();
				}
			case PSMChunk::idPPAN: // PPAN - Channel panning table (used in Sinaria)
				// In some Sinaria tunes, this is actually longer than 2 * channels...
				ASSERT(subChunkHead.length >= m_nChannels * 2u);
				for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
				{
					if(!subChunk.CanRead(2))
					{
						break;
					}

					uint8 type = subChunk.ReadUint8();
					uint8 pan = subChunk.ReadUint8();

					switch(type)
					{
					case 0: // use panning
						subsong.channelPanning[chn] = pan ^ 128;
						subsong.channelSurround[chn] = false;
						break;

					case 2: // surround
						subsong.channelPanning[chn] = 128;
						subsong.channelSurround[chn] = true;
						break;

					case 4: // center
						subsong.channelPanning[chn] = 128;
						subsong.channelSurround[chn] = false;
						break;
					}
				}
				break;

			case PSMChunk::idPATT: // PATT - Pattern list
				// We don't really need this.
				break;

			case PSMChunk::idDSAM: // DSAM - Sample list
				// We don't need this either.
				break;

			default:
				break;

			}
		}

		// attach this subsong to the subsong list - finally, all "sub sub sub ..." chunks are parsed.
		subsongs.push_back(subsong);
	}

	// DSMP - Samples
	if(loadFlags & loadSampleData)
	{
		std::vector<FileReader> sampleChunks = chunks.GetAllChunks(PSMChunk::idDSMP);
		for(std::vector<FileReader>::iterator sampleIter = sampleChunks.begin(); sampleIter != sampleChunks.end(); sampleIter++)
		{
			FileReader chunk(*sampleIter);
			if(!newFormat)
			{
				// Original header
				PSMOldSampleHeader sampleHeader;
				if(!chunk.ReadConvertEndianness(sampleHeader))
				{
					continue;
				}

				SAMPLEINDEX smp = static_cast<SAMPLEINDEX>(sampleHeader.sampleNumber + 1);
				if(smp < MAX_SAMPLES)
				{
					m_nSamples = std::max(m_nSamples, smp);
					mpt::String::Read<mpt::String::nullTerminated>(m_szNames[smp], sampleHeader.sampleName);

					sampleHeader.ConvertToMPT(Samples[smp]);
					sampleHeader.GetSampleFormat().ReadSample(Samples[smp], chunk);
				}
			} else
			{
				// Sinaria uses a slightly different sample header
				PSMNewSampleHeader sampleHeader;
				if(!chunk.ReadConvertEndianness(sampleHeader))
				{
					continue;
				}

				SAMPLEINDEX smp = static_cast<SAMPLEINDEX>(sampleHeader.sampleNumber + 1);
				if(smp < MAX_SAMPLES)
				{
					m_nSamples = std::max(m_nSamples, smp);
					mpt::String::Read<mpt::String::nullTerminated>(m_szNames[smp], sampleHeader.sampleName);

					sampleHeader.ConvertToMPT(Samples[smp]);
					sampleHeader.GetSampleFormat().ReadSample(Samples[smp], chunk);
				}
			}
		}
	}

	// Make the default variables of the first subsong global
	m_nDefaultSpeed = subsongs[0].defaultSpeed;
	m_nDefaultTempo = subsongs[0].defaultTempo;
	m_nRestartPos = subsongs[0].restartPos;
	for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
	{
		ChnSettings[chn].Reset();
		ChnSettings[chn].nVolume = subsongs[0].channelVolume[chn];
		ChnSettings[chn].nPan = subsongs[0].channelPanning[chn];
		ChnSettings[chn].dwFlags.set(CHN_SURROUND, subsongs[0].channelSurround[chn]);
	}

	madeWithTracker = "Epic MegaGames MASI (";
	if(newFormat)
		madeWithTracker += "New Version / Sinaria)";
	else
		madeWithTracker += "New Version)";

	if(!(loadFlags & loadPatternData))
	{
		return true;
	}

	// Now that we know the number of channels, we can go through all the patterns.
	// This is a bit stupid since we will even read duplicate patterns twice, but hey, we do this just once... so who cares?
	PATTERNINDEX pat = 0;
	for(ORDERINDEX ord = 0; ord < Order.size(); ord++)
	{
		if(orderOffsets[ord] == nullptr)
		{
			// Separator pattern
			continue;
		}

		FileReader patternChunk = *orderOffsets[ord];

		uint16 numRows = patternChunk.ReadUint16LE();

		if(Patterns.Insert(pat, numRows))
		{
			break;
		}

		enum
		{
			noteFlag	= 0x80,
			instrFlag	= 0x40,
			volFlag		= 0x20,
			effectFlag	= 0x10,
		};

		// Read pattern.
		for(int row = 0; row < numRows; row++)
		{
			PatternRow rowBase = Patterns[pat].GetRow(row);
			uint16 rowSize = patternChunk.ReadUint16LE();
			if(rowSize < 2)
			{
				continue;
			}

			FileReader rowChunk = patternChunk.GetChunk(rowSize - 2);

			while(rowChunk.AreBytesLeft())
			{
				uint8 flags = rowChunk.ReadUint8();
				uint8 channel = rowChunk.ReadUint8();
				// Point to the correct channel
				ModCommand &m = rowBase[MIN(m_nChannels - 1, channel)];

				if(flags & noteFlag)
				{
					// Note present
					uint8 note = rowChunk.ReadUint8();
					if(!newFormat)
					{
						if(note == 0xFF)
							note = NOTE_NOTECUT;
						else
							if(note < 129) note = (note & 0x0F) + 12 * (note >> 4) + 13;
					} else
					{
						if(note < 85) note += 36;
					}
					m.note = note;
				}

				if(flags & instrFlag)
				{
					// Instrument present
					m.instr = rowChunk.ReadUint8() + 1;
				}

				if(flags & volFlag)
				{
					// Volume present
					uint8 vol = rowChunk.ReadUint8();
					m.volcmd = VOLCMD_VOLUME;
					m.vol = (MIN(vol, 127) + 1) / 2;
				}

				if(flags & effectFlag)
				{
					// Effect present - convert
					m.command = rowChunk.ReadUint8();
					m.param = rowChunk.ReadUint8();

					switch(m.command)
					{
					// Volslides
					case 0x01: // fine volslide up
						m.command = CMD_VOLUMESLIDE;
						if (newFormat) m.param = (m.param << 4) | 0x0F;
						else m.param = ((m.param & 0x1E) << 3) | 0x0F;
						break;
					case 0x02: // volslide up
						m.command = CMD_VOLUMESLIDE;
						if (newFormat) m.param = 0xF0 & (m.param << 4);
						else m.param = 0xF0 & (m.param << 3);
						break;
					case 0x03: // fine volslide down
						m.command = CMD_VOLUMESLIDE;
						if (newFormat) m.param |= 0xF0;
						else m.param = 0xF0 | (m.param >> 1);
						break;
					case 0x04: // volslide down
						m.command = CMD_VOLUMESLIDE;
						if (newFormat) m.param &= 0x0F;
						else if(m.param < 2) m.param |= 0xF0; else m.param = (m.param >> 1) & 0x0F;
						break;

					// Portamento
					case 0x0B: // fine portamento up
						m.command = CMD_PORTAMENTOUP;
						m.param = 0xF0 | ConvertPSMPorta(m.param, newFormat);
						break;
					case 0x0C: // portamento up
						m.command = CMD_PORTAMENTOUP;
						m.param = ConvertPSMPorta(m.param, newFormat);
						break;
					case 0x0D: // fine portamento down
						m.command = CMD_PORTAMENTODOWN;
						m.param = 0xF0 | ConvertPSMPorta(m.param, newFormat);
						break;
					case 0x0E: // portamento down
						m.command = CMD_PORTAMENTODOWN;
						m.param = ConvertPSMPorta(m.param, newFormat);
						break;
					case 0x0F: // tone portamento
						m.command = CMD_TONEPORTAMENTO;
						if(!newFormat) m.param >>= 2;
						break;
					case 0x11: // glissando control
						m.command = CMD_S3MCMDEX;
						m.param = 0x10 | (m.param & 0x01);
						break;
					case 0x10: // tone portamento + volslide up
						m.command = CMD_TONEPORTAVOL;
						m.param = m.param & 0xF0;
						break;
					case 0x12: // tone portamento + volslide down
						m.command = CMD_TONEPORTAVOL;
						m.param = (m.param >> 4) & 0x0F;
						break;

					// Vibrato
					case 0x15: // vibrato
						m.command = CMD_VIBRATO;
						break;
					case 0x16: // vibrato waveform
						m.command = CMD_S3MCMDEX;
						m.param = 0x30 | (m.param & 0x0F);
						break;
					case 0x17: // vibrato + volslide up
						m.command = CMD_VIBRATOVOL;
						m.param = 0xF0 | m.param;
						break;
					case 0x18: // vibrato + volslide down
						m.command = CMD_VIBRATOVOL;
						break;

					// Tremolo
					case 0x1F: // tremolo
						m.command = CMD_TREMOLO;
						break;
					case 0x20: // tremolo waveform
						m.command = CMD_S3MCMDEX;
						m.param = 0x40 | (m.param & 0x0F);
						break;

					// Sample commands
					case 0x29: // 3-byte offset - we only support the middle byte.
						m.command = CMD_OFFSET;
						m.param = rowChunk.ReadUint8();
						rowChunk.Skip(1);
						break;
					case 0x2A: // retrigger
						m.command = CMD_RETRIG;
						break;
					case 0x2B: // note cut
						m.command = CMD_S3MCMDEX;
						m.param = 0xC0 | (m.param & 0x0F);
						break;
					case 0x2C: // note delay
						m.command = CMD_S3MCMDEX;
						m.param = 0xD0 | (m.param & 0x0F);
						break;

					// Position change
					case 0x33: // position jump
						m.command = CMD_POSITIONJUMP;
						m.param >>= 1;
						rowChunk.Skip(1);
						break;
					case 0x34: // pattern break
						m.command = CMD_PATTERNBREAK;
						m.param >>= 1;
						break;
					case 0x35: // loop pattern
						m.command = CMD_S3MCMDEX;
						m.param = 0xB0 | (m.param & 0x0F);
						break;
					case 0x36: // pattern delay
						m.command = CMD_S3MCMDEX;
						m.param = 0xE0 | (m.param & 0x0F);
						break;

					// speed change
					case 0x3D: // set speed
						m.command = CMD_SPEED;
						break;
					case 0x3E: // set tempo
						m.command = CMD_TEMPO;
						break;

					// misc commands
					case 0x47: // arpeggio
						m.command = CMD_ARPEGGIO;
						break;
					case 0x48: // set finetune
						m.command = CMD_S3MCMDEX;
						m.param = 0x20 | (m.param & 0x0F);
						break;
					case 0x49: // set balance
						m.command = CMD_S3MCMDEX;
						m.param = 0x80 | (m.param & 0x0F);
						break;

					case CMD_MODCMDEX:
						// for some strange home-made tunes
						m.command = CMD_S3MCMDEX;
						break;

					default:
						m.command = CMD_NONE;
						break;

					}
				}
			}
		}

		pat++;
	}

	if(subsongs.size() > 1)
	{
		// write subsong "configuration" to patterns (only if there are multiple subsongs)
		for(size_t i = 0; i < subsongs.size(); i++)
		{
			PATTERNINDEX startPattern = Order[subsongs[i].startOrder], endPattern = Order[subsongs[i].endOrder];
			if(startPattern == PATTERNINDEX_INVALID || endPattern == PATTERNINDEX_INVALID) continue; // what, invalid subtune?

			// set the subsong name to all pattern names
			for(PATTERNINDEX pat = startPattern; pat <= endPattern; pat++)
			{
				Patterns[pat].SetName(subsongs[i].songName);
			}

			// subsongs with different panning setup -> write to pattern (MUSIC_C.PSM)
			if(subsongPanningDiffers)
			{
				for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
				{
					if(subsongs[i].channelSurround[chn])
					{
						Patterns[startPattern].WriteEffect(EffectWriter(CMD_S3MCMDEX, 0x91).Row(0).Channel(chn).Retry(EffectWriter::rmTryNextRow));
					} else
					{
						Patterns[startPattern].WriteEffect(EffectWriter(CMD_PANNING8, subsongs[i].channelPanning[chn]).Row(0).Channel(chn).Retry(EffectWriter::rmTryNextRow));
					}
				}
			}
			// write default tempo/speed to pattern
			Patterns[startPattern].WriteEffect(EffectWriter(CMD_SPEED, subsongs[i].defaultSpeed).Row(0).Retry(EffectWriter::rmTryNextRow));
			Patterns[startPattern].WriteEffect(EffectWriter(CMD_TEMPO, subsongs[i].defaultTempo).Row(0).Retry(EffectWriter::rmTryNextRow));

			// don't write channel volume for now, as it's always set to 100% anyway

			// there's a restart pos, so let's try to insert a Bxx command in the last pattern
			if(subsongs[i].restartPos != ORDERINDEX_INVALID)
			{
				ROWINDEX lastRow = Patterns[endPattern].GetNumRows() - 1;
				ModCommand *m;
				m = Patterns[endPattern];
				for(uint32 cell = 0; cell < m_nChannels * Patterns[endPattern].GetNumRows(); cell++, m++)
				{
					if(m->command == CMD_PATTERNBREAK || m->command == CMD_POSITIONJUMP)
					{
						lastRow = cell / m_nChannels;
						break;
					}
				}
				Patterns[endPattern].WriteEffect(EffectWriter(CMD_POSITIONJUMP, static_cast<ModCommand::PARAM>(subsongs[i].restartPos)).Row(lastRow).Retry(EffectWriter::rmTryPreviousRow));
			}
		}
	}

	return true;
}

////////////////////////////////
//
//  PSM16 support starts here.
//

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED PSM16FileHeader
{
	// 32-Bit chunk identifiers
	enum PSM16Magic
	{
		idPORD	= MAGIC4LE('P','O','R','D'),
		idPPAN	= MAGIC4LE('P','P','A','N'),
		idPSAH	= MAGIC4LE('P','S','A','H'),
		idPPAT	= MAGIC4LE('P','P','A','T'),
	};

	char   formatID[4];		// "PSM\xFE" (PSM16)
	char   songName[59];	// Song title, padded with nulls
	uint8  lineEnd;			// $1A
	uint8  songType;		// Song Type bitfield
	uint8  formatVersion;	// $10
	uint8  patternVersion;  // 0 or 1
	uint8  songSpeed;		// 1 ... 255
	uint8  songTempo;		// 32 ... 255
	uint8  masterVolume;	// 0 ... 255
	uint16 songLength;		// 0 ... 255 (number of patterns to play in the song)
	uint16 songOrders;		// 0 ... 255 (same as previous value as no subsongs are present)
	uint16 numPatterns;		// 1 ... 255
	uint16 numSamples;		// 1 ... 255
	uint16 numChannelsPlay;	// 0 ... 32 (max. number of channels to play)
	uint16 numChannelsReal;	// 0 ... 32 (max. number of channels to process)
	uint32 orderOffset;		// Pointer to order list
	uint32 panOffset;		// Pointer to pan table
	uint32 patOffset;		// Pointer to pattern data
	uint32 smpOffset;		// Pointer to sample headers
	uint32 commentsOffset;	// Pointer to song comment
	uint32 patSize;			// Size of all patterns
	uint8  filler[40];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(songLength);
		SwapBytesLE(songOrders);
		SwapBytesLE(numPatterns);
		SwapBytesLE(numSamples);
		SwapBytesLE(numChannelsPlay);
		SwapBytesLE(numChannelsReal);
		SwapBytesLE(orderOffset);
		SwapBytesLE(panOffset);
		SwapBytesLE(patOffset);
		SwapBytesLE(smpOffset);
		SwapBytesLE(commentsOffset);
		SwapBytesLE(patSize);
	}
};

STATIC_ASSERT(sizeof(PSM16FileHeader) == 146);

struct PACKED PSM16SampleHeader
{
	enum SampleFlags
	{
		smpMask		= 0x7F,
		smp16Bit	= 0x04,
		smpUnsigned	= 0x08,
		smpDelta	= 0x10,
		smpPingPong	= 0x20,
		smpLoop		= 0x80,
	};

	char   filename[13];	// null-terminated
	char   name[24];		// dito
	uint32 offset;			// in file
	uint32 memoffset;		// not used
	uint16 sampleNumber;	// 1 ... 255
	uint8  flags;			// sample flag bitfield
	uint32 length;			// in bytes
	uint32 loopStart;		// in samples?
	uint32 loopEnd;			// in samples?
	int8   finetune;		// Low nibble = MOD finetune, high nibble = transpose (7 = center)
	uint8  volume;			// default volume
	uint16 c2freq;			// Middle-C frequency, which has to be combined with the finetune and transpose.

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(offset);
		SwapBytesLE(memoffset);
		SwapBytesLE(sampleNumber);
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(c2freq);
	}

	// Convert sample header to OpenMPT's internal format
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mpt::String::Read<mpt::String::nullTerminated>(mptSmp.filename, filename);

		mptSmp.nLength = length;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;
		mptSmp.nC5Speed = c2freq;

		// It seems like that finetune and transpose are added to the already given c2freq... That's a double WTF!
		// Why on earth would you want to use both systems at the same time?
		mptSmp.FrequencyToTranspose();
		mptSmp.nC5Speed = ModSample::TransposeToFrequency(mptSmp.RelativeTone + (finetune >> 4) - 7, MOD2XMFineTune(finetune & 0x0F));

		mptSmp.nVolume = volume << 2;
		mptSmp.nGlobalVol = 256;

		mptSmp.uFlags.reset();
		if(flags & PSM16SampleHeader::smp16Bit)
		{
			mptSmp.uFlags |= CHN_16BIT;
			mptSmp.nLength /= 2;
		}
		if(flags & PSM16SampleHeader::smpPingPong)
		{
			mptSmp.uFlags |= CHN_PINGPONGLOOP;
		}
		if(flags & PSM16SampleHeader::smpLoop)
		{
			mptSmp.uFlags |= CHN_LOOP;
		}
	}

	// Retrieve the internal sample format flags for this sample.
	SampleIO GetSampleFormat() const
	{
		SampleIO sampleIO(
			(flags & PSM16SampleHeader::smp16Bit) ? SampleIO::_16bit : SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::signedPCM);

		if(flags & PSM16SampleHeader::smpUnsigned)
		{
			sampleIO |= SampleIO::unsignedPCM;
		} else if((flags & PSM16SampleHeader::smpDelta) || (flags & PSM16SampleHeader::smpMask) == 0)
		{
			sampleIO |= SampleIO::deltaPCM;
		}

		return sampleIO;
	}
};

STATIC_ASSERT(sizeof(PSM16SampleHeader) == 64);

struct PACKED PSM16PatternHeader
{
	uint16 size;		// includes header bytes
	uint8  numRows;		// 1 ... 64
	uint8  numChans;	// 1 ... 32

	void ConvertEndianness()
	{
		SwapBytesLE(size);
	}
};

STATIC_ASSERT(sizeof(PSM16PatternHeader) == 4);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadPSM16(FileReader &file, ModLoadingFlags loadFlags)
//---------------------------------------------------------------------
{
	file.Rewind();

	// Is it a valid PSM16 file?
	PSM16FileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| memcmp(fileHeader.formatID, "PSM\xFE", 4)
		|| fileHeader.lineEnd != 0x1A
		|| (fileHeader.formatVersion != 0x10 && fileHeader.formatVersion != 0x01) // why is this sometimes 0x01?
		|| fileHeader.patternVersion != 0 // 255ch pattern version not supported (did anyone use this?)
		|| (fileHeader.songType & 3) != 0
		|| std::max(fileHeader.numChannelsPlay, fileHeader.numChannelsReal) == 0)
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	// Seems to be valid!
	InitializeGlobals();
	madeWithTracker = "Epic MegaGames MASI (Old Version)";
	m_nType = MOD_TYPE_S3M;
	m_nChannels = Clamp(CHANNELINDEX(fileHeader.numChannelsPlay), CHANNELINDEX(fileHeader.numChannelsReal), MAX_BASECHANNELS);
	m_nSamplePreAmp = fileHeader.masterVolume;
	if(m_nSamplePreAmp == 255)
	{
		// Most of the time, the master volume value makes sense... Just not when it's 255.
		m_nSamplePreAmp = 48;
	}
	m_nDefaultSpeed = fileHeader.songSpeed;
	m_nDefaultTempo = fileHeader.songTempo;

	mpt::String::Read<mpt::String::spacePadded>(songName, fileHeader.songName);

	// Read orders
	if(fileHeader.orderOffset > 4 && file.Seek(fileHeader.orderOffset - 4) && file.ReadUint32LE() == PSM16FileHeader::idPORD)
	{
		Order.ReadAsByte(file, fileHeader.songOrders);
	}

	// Read pan positions
	if(fileHeader.panOffset > 4 && file.Seek(fileHeader.panOffset - 4) && file.ReadUint32LE() == PSM16FileHeader::idPPAN)
	{
		for(CHANNELINDEX i = 0; i < 32; i++)
		{
			ChnSettings[i].Reset();
			ChnSettings[i].nPan = ((15 - (file.ReadUint8() & 0x0F)) * 256 + 8) / 15;	// 15 seems to be left and 0 seems to be right...
			// ChnSettings[i].dwFlags = (i >= fileHeader.numChannelsPlay) ? CHN_MUTE : 0; // don't mute channels, as muted channels are completely ignored in S3M
		}
	}

	// Read samples
	if(fileHeader.smpOffset > 4 && file.Seek(fileHeader.smpOffset - 4) && file.ReadUint32LE() == PSM16FileHeader::idPSAH)
	{
		FileReader sampleChunk = file.GetChunk(file.BytesLeft());

		for(SAMPLEINDEX fileSample = 0; fileSample < fileHeader.numSamples; fileSample++)
		{
			PSM16SampleHeader sampleHeader;
			if(!sampleChunk.ReadConvertEndianness(sampleHeader))
			{
				break;
			}

			SAMPLEINDEX smp = sampleHeader.sampleNumber;
			m_nSamples = std::max(m_nSamples, smp);

			mpt::String::Read<mpt::String::nullTerminated>(m_szNames[smp], sampleHeader.name);
			sampleHeader.ConvertToMPT(Samples[smp]);

			file.Seek(sampleHeader.offset);
			sampleHeader.GetSampleFormat().ReadSample(Samples[smp], file);
		}
	}

	// Read patterns
	if(!(loadFlags & loadPatternData))
	{
		return true;
	}
	if(fileHeader.patOffset > 4 && file.Seek(fileHeader.patOffset - 4) && file.ReadUint32LE() == PSM16FileHeader::idPPAT)
	{
		for(PATTERNINDEX pat = 0; pat < fileHeader.numPatterns; pat++)
		{
			PSM16PatternHeader patternHeader;
			if(!file.ReadConvertEndianness(patternHeader))
			{
				break;
			}

			if(patternHeader.size < sizeof(PSM16PatternHeader))
			{
				continue;
			}

			// Patterns are padded to 16 Bytes
			FileReader patternChunk = file.GetChunk(((patternHeader.size + 15) & ~15) - sizeof(PSM16PatternHeader));

			if(Patterns.Insert(pat, patternHeader.numRows))
			{
				continue;
			}

			enum
			{
				channelMask	= 0x1F,
				noteFlag	= 0x80,
				volFlag		= 0x40,
				effectFlag	= 0x20,
			};

			ROWINDEX curRow = 0;

			while(patternChunk.AreBytesLeft() && curRow < patternHeader.numRows)
			{
				uint8 chnFlag = patternChunk.ReadUint8();
				if(chnFlag == 0)
				{
					curRow++;
					continue;
				}

				ModCommand &m = *Patterns[pat].GetpModCommand(curRow, MIN(chnFlag & channelMask, m_nChannels - 1));

				if(chnFlag & noteFlag)
				{
					// note + instr present
					m.note = patternChunk.ReadUint8() + 36;
					m.instr = patternChunk.ReadUint8();
				}
				if(chnFlag & volFlag)
				{
					// volume present
					m.volcmd = VOLCMD_VOLUME;
					m.vol = std::min(patternChunk.ReadUint8(), uint8(64));
				}
				if(chnFlag & effectFlag)
				{
					// effect present - convert
					m.command = patternChunk.ReadUint8();
					m.param = patternChunk.ReadUint8();

					switch(m.command)
					{
					// Volslides
					case 0x01: // fine volslide up
						m.command = CMD_VOLUMESLIDE;
						m.param = (m.param << 4) | 0x0F;
						break;
					case 0x02: // volslide up
						m.command = CMD_VOLUMESLIDE;
						m.param = (m.param << 4) & 0xF0;
						break;
					case 0x03: // fine voslide down
						m.command = CMD_VOLUMESLIDE;
						m.param = 0xF0 | m.param;
						break;
					case 0x04: // volslide down
						m.command = CMD_VOLUMESLIDE;
						m.param = m.param & 0x0F;
						break;

					// Portamento
					case 0x0A: // fine portamento up
						m.command = CMD_PORTAMENTOUP;
						m.param |= 0xF0;
						break;
					case 0x0B: // portamento down
						m.command = CMD_PORTAMENTOUP;
						break;
					case 0x0C: // fine portamento down
						m.command = CMD_PORTAMENTODOWN;
						m.param |= 0xF0;
						break;
					case 0x0D: // portamento down
						m.command = CMD_PORTAMENTODOWN;
						break;
					case 0x0E: // tone portamento
						m.command = CMD_TONEPORTAMENTO;
						break;
					case 0x0F: // glissando control
						m.command = CMD_S3MCMDEX;
						m.param |= 0x10;
						break;
					case 0x10: // tone portamento + volslide up
						m.command = CMD_TONEPORTAVOL;
						m.param <<= 4;
						break;
					case 0x11: // tone portamento + volslide down
						m.command = CMD_TONEPORTAVOL;
						m.param &= 0x0F;
						break;

					// Vibrato
					case 0x14: // vibrato
						m.command = CMD_VIBRATO;
						break;
					case 0x15: // vibrato waveform
						m.command = CMD_S3MCMDEX;
						m.param |= 0x30;
						break;
					case 0x16: // vibrato + volslide up
						m.command = CMD_VIBRATOVOL;
						m.param <<= 4;
						break;
					case 0x17: // vibrato + volslide down
						m.command = CMD_VIBRATOVOL;
						m.param &= 0x0F;
						break;

					// Tremolo
					case 0x1E: // tremolo
						m.command = CMD_TREMOLO;
						break;
					case 0x1F: // tremolo waveform
						m.command = CMD_S3MCMDEX;
						m.param |= 0x40;
						break;

					// Sample commands
					case 0x28: // 3-byte offset - we only support the middle byte.
						m.command = CMD_OFFSET;
						m.param = patternChunk.ReadUint8();
						patternChunk.Skip(1);
						break;
					case 0x29: // retrigger
						m.command = CMD_RETRIG;
						m.param &= 0x0F;
						break;
					case 0x2A: // note cut
						m.command = CMD_S3MCMDEX;
						if(m.param == 0)	// in S3M mode, SC0 is ignored, so we convert it to a note cut.
						{
							if(m.note == NOTE_NONE)
							{
								m.note = NOTE_NOTECUT;
								m.command = CMD_NONE;
							} else
							{
								m.param = 1;
							}
						}
						m.param |= 0xC0;
						break;
					case 0x2B: // note delay
						m.command = CMD_S3MCMDEX;
						m.param |= 0xD0;
						break;

					// Position change
					case 0x32: // position jump
						m.command = CMD_POSITIONJUMP;
						break;
					case 0x33: // pattern break
						m.command = CMD_PATTERNBREAK;
						break;
					case 0x34: // loop pattern
						m.command = CMD_S3MCMDEX;
						m.param |= 0xB0;
						break;
					case 0x35: // pattern delay
						m.command = CMD_S3MCMDEX;
						m.param |= 0xE0;
						break;

					// speed change
					case 0x3C: // set speed
						m.command = CMD_SPEED;
						break;
					case 0x3D: // set tempo
						m.command = CMD_TEMPO;
						break;

					// misc commands
					case 0x46: // arpeggio
						m.command = CMD_ARPEGGIO;
						break;
					case 0x47: // set finetune
						m.command = CMD_S3MCMDEX;
						m.param |= 0x20;
						break;
					case 0x48: // set balance (panning?)
						m.command = CMD_PANNING8;
						m.param = (m.param << 4) + 8;
						break;

					default:
						m.command = CMD_NONE;
						break;
					}
				}
			}
			// Pattern break for short patterns (so saving the modules as S3M won't break it)
			if(patternHeader.numRows != 64)
			{
				Patterns[pat].WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(patternHeader.numRows - 1).Retry(EffectWriter::rmTryNextRow));
			}
		}
	}

	if(fileHeader.commentsOffset != 0)
	{
		file.Seek(fileHeader.commentsOffset);
		songMessage.Read(file, file.ReadUint16LE(), SongMessage::leAutodetect);
	}

	return true;
}
