/*
 * WAVTools.h
 * ----------
 * Purpose: Definition of WAV file structures and helper functions
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "ChunkReader.h"
#include "Tagging.h"

OPENMPT_NAMESPACE_BEGIN

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

// RIFF header
struct PACKED RIFFHeader
{
	// 32-Bit chunk identifiers
	enum RIFFMagic
	{
		idRIFF	= 0x46464952,	// magic for WAV files
		idLIST	= 0x5453494C,	// magic for samples in DLS banks
		idWAVE	= 0x45564157,	// type for WAV files
		idwave	= 0x65766177,	// type for samples in DLS banks
	};

	uint32 magic;	// RIFF (in WAV files) or LIST (in DLS banks)
	uint32 length;	// Size of the file, not including magic and length
	uint32 type;	// WAVE (in WAV files) or wave (in DLS banks)

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(magic);
		SwapBytesLE(length);
		SwapBytesLE(type);
	}
};

STATIC_ASSERT(sizeof(RIFFHeader) == 12);


// General RIFF Chunk header
struct PACKED RIFFChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idfmt_	= 0x20746D66,	// "fmt "
		iddata	= 0x61746164,	// "data"
		idpcm_	= 0x206d6370,	// "pcm " (IMA ADPCM samples)
		idfact	= 0x74636166,	// "fact" (compressed samples)
		idsmpl	= 0x6C706D73,	// "smpl"
		idLIST	= 0x5453494C,	// "LIST"
		idxtra	= 0x61727478,	// "xtra"
		idcue_	= 0x20657563,	// "cue "
		idwsmp	= 0x706D7377,	// "wsmp" (DLS bank samples)
		id____	= 0x00000000,	// Found when loading buggy MPT samples

		// Identifiers in "LIST" chunk
		idINAM	= 0x4D414E49, // title
		idISFT	= 0x54465349, // software
		idICOP	= 0x504F4349, // copyright
		idIART	= 0x54524149, // artist
		idIPRD	= 0x44525049, // product (album)
		idICMT	= 0x544D4349, // comment
		idIENG	= 0x474E4549, // engineer
		idISBJ	= 0x4A425349, // subject
		idIGNR	= 0x524E4749, // genre
		idICRD	= 0x44524349, // date created

		idYEAR  = 0x52414559, // year
		idTRCK  = 0x4B435254, // track number
		idTURL  = 0x4C535554, // url
	};

	typedef ChunkIdentifiers id_type;

	uint32 id;		// See ChunkIdentifiers
	uint32 length;	// Chunk size without header

	size_t GetLength() const
	{
		return SwapBytesReturnLE(length);
	}

	id_type GetID() const
	{
		return static_cast<id_type>(SwapBytesReturnLE(id));
	}

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(id);
		SwapBytesLE(length);
	}
};

STATIC_ASSERT(sizeof(RIFFChunk) == 8);


// Format Chunk
struct PACKED WAVFormatChunk
{
	// Sample formats
	enum SampleFormats
	{
		fmtPCM			= 1,
		fmtFloat		= 3,
		fmtALaw			= 6,
		fmtULaw			= 7,
		fmtIMA_ADPCM	= 17,
		fmtMP3			= 85,
		fmtExtensible	= 0xFFFE,
	};

	uint16 format;			// Sample format, see SampleFormats
	uint16 numChannels;		// Number of audio channels
	uint32 sampleRate;		// Sample rate in Hz
	uint32 byteRate;		// Bytes per second (should be freqHz * blockAlign)
	uint16 blockAlign;		// Size of a sample, in bytes (do not trust this value, it's incorrect in some files)
	uint16 bitsPerSample;	// Bits per sample

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(format);
		SwapBytesLE(numChannels);
		SwapBytesLE(sampleRate);
		SwapBytesLE(byteRate);
		SwapBytesLE(blockAlign);
		SwapBytesLE(bitsPerSample);
	}
};

STATIC_ASSERT(sizeof(WAVFormatChunk) == 16);


// Extension of the WAVFormatChunk structure, used if format == formatExtensible
struct PACKED WAVFormatChunkExtension
{
	uint16 size;
	uint16 validBitsPerSample;
	uint32 channelMask;
	uint16 subFormat;
	uint8  guid[14];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(size);
		SwapBytesLE(validBitsPerSample);
		SwapBytesLE(channelMask);
		SwapBytesLE(subFormat);
	}
};

STATIC_ASSERT(sizeof(WAVFormatChunkExtension) == 24);


// Sample information chunk
struct PACKED WAVSampleInfoChunk
{
	uint32 manufacturer;
	uint32 product;
	uint32 samplePeriod;	// 1000000000 / sampleRate
	uint32 baseNote;		// MIDI base note of sample
	uint32 pitchFraction;
	uint32 SMPTEFormat;
	uint32 SMPTEOffset;
	uint32 numLoops;		// number of loops
	uint32 samplerData;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(manufacturer);
		SwapBytesLE(product);
		SwapBytesLE(samplePeriod);
		SwapBytesLE(baseNote);
		SwapBytesLE(pitchFraction);
		SwapBytesLE(SMPTEFormat);
		SwapBytesLE(SMPTEOffset);
		SwapBytesLE(numLoops);
		SwapBytesLE(samplerData);
	}

	// Set up information
	void ConvertToWAV(uint32 freq)
	{
		manufacturer = 0;
		product = 0;
		samplePeriod = 1000000000 / freq;
		baseNote = NOTE_MIDDLEC - NOTE_MIN;
		pitchFraction = 0;
		SMPTEFormat = 0;
		SMPTEOffset = 0;
		numLoops = 0;
		samplerData = 0;
	}
};

STATIC_ASSERT(sizeof(WAVSampleInfoChunk) == 36);


// Sample loop information chunk (found after WAVSampleInfoChunk in "smpl" chunk)
struct PACKED WAVSampleLoop
{
	// Sample Loop Types
	enum LoopType
	{
		loopForward		= 0,
		loopBidi		= 1,
		loopBackward	= 2,
	};

	uint32 identifier;
	uint32 loopType;		// See LoopType
	uint32 loopStart;		// Loop start in samples
	uint32 loopEnd;			// Loop end in samples
	uint32 fraction;
	uint32 playCount;		// Loop Count, 0 = infinite

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(identifier);
		SwapBytesLE(loopType);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(fraction);
		SwapBytesLE(playCount);
	}

	// Apply WAV loop information to a mod sample.
	void ApplyToSample(SmpLength &start, SmpLength &end, SmpLength sampleLength, SampleFlags &flags, ChannelFlags enableFlag, ChannelFlags bidiFlag, bool mptLoopFix) const;

	// Convert internal loop information into a WAV loop.
	void ConvertToWAV(SmpLength start, SmpLength end, bool bidi);
};

STATIC_ASSERT(sizeof(WAVSampleLoop) == 24);


// MPT-specific "xtra" chunk
struct PACKED WAVExtraChunk
{
	enum Flags
	{
		setPanning	= 0x20,
	};

	uint32 flags;
	uint16 defaultPan;
	uint16 defaultVolume;
	uint16 globalVolume;
	uint16 reserved;
	uint8  vibratoType;
	uint8  vibratoSweep;
	uint8  vibratoDepth;
	uint8  vibratoRate;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(flags);
		SwapBytesLE(defaultPan);
		SwapBytesLE(defaultVolume);
		SwapBytesLE(globalVolume);
	}

	// Set up sample information
	void ConvertToWAV(const ModSample &sample, MODTYPE modType)
	{
		if(sample.uFlags[CHN_PANNING])
		{
			flags = WAVExtraChunk::setPanning;
		} else
		{
			flags = 0;
		}

		defaultPan = sample.nPan;
		defaultVolume = sample.nVolume;
		globalVolume = sample.nGlobalVol;
		vibratoType = sample.nVibType;
		vibratoSweep = sample.nVibSweep;
		vibratoDepth = sample.nVibDepth;
		vibratoRate = sample.nVibRate;

		if((modType & MOD_TYPE_XM) && (vibratoDepth | vibratoRate))
		{
			// XM vibrato is upside down
			vibratoSweep = 255 - vibratoSweep;
		}
	}
};

STATIC_ASSERT(sizeof(WAVExtraChunk) == 16);


// Sample cue point structure for the "cue " chunk
struct PACKED WAVCuePoint
{
	uint32 id;			// Unique identification value
	uint32 position;	// Play order position
	uint32 riffChunkID;	// RIFF ID of corresponding data chunk
	uint32 chunkStart;	// Byte Offset of Data Chunk
	uint32 blockStart;	// Byte Offset to sample of First Channel
	uint32 offset;		// Byte Offset to sample byte of First Channel

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(id);
		SwapBytesLE(position);
		SwapBytesLE(riffChunkID);
		SwapBytesLE(chunkStart);
		SwapBytesLE(blockStart);
		SwapBytesLE(offset);
	}

	// Set up sample information
	void ConvertToWAV(uint32 id_, SmpLength offset_)
	{
		id = id_;
		position = offset_;
		riffChunkID = static_cast<uint32>(RIFFChunk::iddata);
		chunkStart = 0;	// we use no Wave List Chunk (wavl) as we have only one data block, so this should be 0.
		blockStart = 0;	// ditto
		offset = offset_;
	}
};

STATIC_ASSERT(sizeof(WAVCuePoint) == 24);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


//=============
class WAVReader
//=============
{
protected:
	ChunkReader file;
	FileReader sampleData, smplChunk, xtraChunk, wsmpChunk, cueChunk;
	ChunkReader::ChunkList<RIFFChunk> infoChunk;

	FileReader::off_t sampleLength;
	bool isDLS;
	WAVFormatChunk formatInfo;
	uint16 extFormat;
	bool mayBeCoolEdit16_8;

public:
	WAVReader(FileReader &inputFile);

	bool IsValid() const { return sampleData.IsValid(); }

	void FindMetadataChunks(ChunkReader::ChunkList<RIFFChunk> &chunks);

	// Self-explanatory getters.
	WAVFormatChunk::SampleFormats GetSampleFormat() const { return IsExtensibleFormat() ? static_cast<WAVFormatChunk::SampleFormats>(extFormat) : static_cast<WAVFormatChunk::SampleFormats>(formatInfo.format); }
	uint16 GetNumChannels() const { return formatInfo.numChannels; }
	uint16 GetBitsPerSample() const { return formatInfo.bitsPerSample; }
	uint32 GetSampleRate() const { return formatInfo.sampleRate; }
	uint16 GetBlockAlign() const { return formatInfo.blockAlign; }
	FileReader GetSampleData() const { return sampleData; }
	FileReader GetWsmpChunk() const { return wsmpChunk; }
	bool IsExtensibleFormat() const { return formatInfo.format == WAVFormatChunk::fmtExtensible; }
	bool MayBeCoolEdit16_8() const { return mayBeCoolEdit16_8; }

	// Get size of a single sample point, in bytes.
	uint16 GetSampleSize() const { return ((GetNumChannels() * GetBitsPerSample()) + 7) / 8; }

	// Get sample length (in samples)
	SmpLength GetSampleLength() const { return sampleLength; }

	// Apply sample settings from file (loop points, MPT extra settings, ...) to a sample.
	void ApplySampleSettings(ModSample &sample, char (&sampleName)[MAX_SAMPLENAME]);
};


#ifndef MODPLUG_NO_FILESAVE

//=============
class WAVWriter
//=============
{
protected:
	// When writing to a stream: Stream pointer
	std::ostream *s;
	// When writing to memory: Memory address + length
	uint8 *memory;
	size_t memSize;

	// Cursor position
	size_t position;
	// Total number of bytes written to file / memory
	size_t totalSize;

	// Currently written chunk
	size_t chunkStartPos;
	RIFFChunk chunkHeader;

public:
	// Output to stream: Initialize with std::ostream*.
	WAVWriter(std::ostream *stream);
	// Output to clipboard: Initialize with pointer to memory and size of reserved memory.
	WAVWriter(void *mem, size_t size);

	~WAVWriter();

	// Check if anything can be written to the file.
	bool IsValid() const { return s != nullptr || memory != nullptr; }

	// Finalize the file by closing the last open chunk and updating the file header. Returns total size of file.
	size_t Finalize();
	// Begin writing a new chunk to the file.
	void StartChunk(RIFFChunk::id_type id);

	// Skip some bytes... For example after writing sample data.
	void Skip(size_t numBytes) { Seek(position + numBytes); }
	// Get position in file (not counting any changes done to the file from outside this class, i.e. through GetFile())
	size_t GetPosition() const { return position; }

	// Shrink file size to current position.
	void Truncate() { totalSize = position; }

	// Write some data to the file.
	template<typename T>
	void Write(const T &data)
	{
		Write(&data, sizeof(T));
	}

	// Write a buffer to the file.
	void WriteBuffer(const char *data, size_t size)
	{
		Write(data, size);
	}

	// Write an array to the file.
	template<typename T, size_t size>
	void WriteArray(const T (&data)[size])
	{
		Write(data, sizeof(T) * size);
	}

	// Write the WAV format to the file.
	void WriteFormat(uint32 sampleRate, uint16 bitDepth, uint16 numChannels, WAVFormatChunk::SampleFormats encoding);
	// Write text tags to the file.
	void WriteMetatags(const FileTags &tags);
	// Write a sample loop information chunk to the file.
	void WriteLoopInformation(const ModSample &sample);
	// Write a sample's cue points to the file.
	void WriteCueInformation(const ModSample &sample);
	// Write MPT's sample information chunk to the file.
	void WriteExtraInformation(const ModSample &sample, MODTYPE modType, const char *sampleName = nullptr);

protected:
	void Init();
	// Seek to a position in file.
	void Seek(size_t pos);
	// End current chunk by updating the chunk header and writing a padding byte if necessary.
	void FinalizeChunk();

	// Write some data to the file.
	void Write(const void *data, size_t numBytes);

	// Write a single tag into a open idLIST chunk
	void WriteTag(RIFFChunk::id_type id, const mpt::ustring &utext);
};

#endif // MODPLUG_NO_FILESAVE

OPENMPT_NAMESPACE_END
