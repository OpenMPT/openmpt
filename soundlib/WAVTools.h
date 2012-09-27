/*
 * WAVTools.h
 * ----------
 * Purpose: Definition of WAV file structures and helper functions
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "Loaders.h"
#include "ChunkReader.h"

#pragma pack(push, 1)

// RIFF header
struct RIFFHeader
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


// General RIFF Chunk header
struct RIFFChunk
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
		idINAM	= 0x4D414E49,	// "INAM"
		idISFT	= 0x54465349,	// "ISFT"
	};

	typedef ChunkIdentifiers id_type;

	uint32 id;		// See ChunkIdentifiers
	uint32 length;	// Chunk size without header

	size_t GetLength() const
	{
		uint32 l = length;
		return SwapBytesLE(l);
	}

	id_type GetID() const
	{
		uint32 i = id;
		return static_cast<id_type>(SwapBytesLE(i));
	}

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(id);
		SwapBytesLE(length);
	}
};


// Format Chunk
struct WAVFormatChunk
{
	// Sample formats
	enum SampleFormats
	{
		fmtPCM			= 1,
		fmtFloat		= 3,
		fmtIMA_ADPCM	= 17,
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


// Extension of the WAVFormatChunk structure, used if format == formatExtensible
struct WAVFormatChunkExtension
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


// Sample information chunk
struct WAVSampleInfoChunk
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
};


// Sample loop information chunk (found after WAVSampleInfoChunk in "smpl" chunk)
struct WAVSampleLoop
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

	void ApplyToSample(SmpLength &start, SmpLength &end, uint32 sampleLength, FlagSet<ChannelFlags, uint16> &flags, ChannelFlags enableFlag, ChannelFlags bidiFlag, bool mptLoopFix) const;

	// Set up a loop.
	void ConvertToWAV(SmpLength start, SmpLength end, bool bidi)
	{
		identifier = 0;
		loopType = bidi ? loopBidi : loopForward;
		loopStart = start;
		// Loop ends are *inclusive* in the RIFF standard, while they're *exclusive* in OpenMPT.
		if(end> start)
		{
			loopEnd = end - 1;
		} else
		{
			loopEnd = start;
		}
		fraction = 0;
		playCount = 0;
	}
};


// MPT-specific "xtra" chunk
struct WAVExtraChunk
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
};


// Sample cue point structure for the "cue " chunk
struct WAVCuePoint
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
};


#pragma pack(pop)


//=============
class WAVReader
//=============
{
protected:
	ChunkReader file;
	FileReader sampleData, smplChunk, xtraChunk, wsmpChunk;
	ChunkReader::ChunkList<RIFFChunk> infoChunk;

	size_t sampleLength;
	bool isDLS;
	WAVFormatChunk formatInfo;

public:
	WAVReader(FileReader &inputFile);

	bool IsValid() const { return sampleData.IsValid(); }

	// Self-explanatory getters.
	WAVFormatChunk::SampleFormats GetSampleFormat() const { return static_cast<WAVFormatChunk::SampleFormats>(formatInfo.format); }
	uint16 GetNumChannels() const { return formatInfo.numChannels; }
	uint16 GetBitsPerSample() const { return formatInfo.bitsPerSample; }
	uint32 GetSampleRate() const { return formatInfo.sampleRate; }
	uint16 GetBlockAlign() const { return formatInfo.blockAlign; }
	FileReader GetSampleData() const { return sampleData; }
	FileReader GetWsmpChunk() const { return wsmpChunk; }

	// Get size of a single sample point, in bytes.
	uint16 GetSampleSize() const { return ((GetNumChannels() * GetBitsPerSample()) + 7) / 8; }

	// Get sample length (in samples)
	uint32 GetSampleLength() const { return sampleLength; }

	// Apply sample settings from file (loop points, MPT extra settings, ...) to a sample.
	void ApplySampleSettings(ModSample &sample, char (&sampleName)[MAX_SAMPLENAME]);
};
