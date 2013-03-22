/*
 * SampleIO.h
 * ----------
 * Purpose: Central code for reading and writing samples. Create your SampleIO object and have a go at the ReadSample and WriteSample functions!
 * Notes  : Not all combinations of possible sample format combinations are implemented, especially for WriteSample.
 *          Using the existing generic sample conversion functors in SampleFormatConverters.h, it should be quite easy to extend the code, though.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

class FileReader;

// Sample import / export formats
//============
class SampleIO
//============
{
protected:
	typedef uint32 format_type;
	format_type format;

	// Internal bitmasks
	enum Offsets
	{
		bitOffset		= 0,
		channelOffset	= 8,
		endianOffset	= 16,
		encodingOffset	= 24,

		bitMask			= 0xFF << bitOffset,
		channelMask		= 0xFF << channelOffset,
		endianMask		= 0xFF << endianOffset,
		encodingMask	= 0xFF << encodingOffset,
	};

public:
	// Bits per sample
	enum Bitdepth
	{
		_8bit	= 8,
		_16bit	= 16,
		_24bit	= 24,
		_32bit	= 32,
	};

	// Number of channels + channel format
	enum Channels
	{
		mono = 0,
		stereoInterleaved,	// LRLRLR...
		stereoSplit,		// LLL...RRR...
	};

	// Sample byte order
	enum Endianness
	{
		littleEndian = 0,
		bigEndian,
	};

	// Sample encoding
	enum Encoding
	{
		signedPCM = 0,	// Integer PCM, signed
		unsignedPCM,	// Integer PCM, unsigned
		deltaPCM,		// Integer PCM, delta-encoded
		floatPCM,		// Floating point PCM
		IT214,			// Impulse Tracker 2.14 compressed
		IT215,			// Impulse Tracker 2.15 compressed
		AMS,			// AMS / Velvet Studio packed
		DMF,			// DMF Huffman compression
		MDL,			// MDL Huffman compression
		PTM8Dto16,		// PTM 8-Bit delta value -> 16-Bit sample
		PCM7to8,		// 8-Bit sample data with unused high bit
		ADPCM,			// 4-Bit ADPCM-packed
	};

	SampleIO(Bitdepth bits = _8bit, Channels channels = mono, Endianness endianness = littleEndian, Encoding encoding = signedPCM)
	{
		format = (bits << bitOffset) | (channels << channelOffset) | (endianness << endianOffset) | (encoding << encodingOffset);
	}

	SampleIO(const SampleIO &other) : format(other.format) { }

	bool operator== (const SampleIO &other) const
	{
		return format == other.format;
	}
	bool operator!= (const SampleIO &other) const
	{
		return !(*this == other);
	}

	void operator|= (Bitdepth bits)
	{
		format = (format & ~bitMask) | (bits << bitOffset);
	}

	void operator|= (Channels channels)
	{
		format = (format & ~channelMask) | (channels << channelOffset);
	}

	void operator|= (Endianness endianness)
	{
		format = (format & ~endianMask) | (endianness << endianOffset);
	}

	void operator|= (Encoding encoding)
	{
		format = (format & ~encodingMask) | (encoding << encodingOffset);
	}

	// Get bits per sample
	uint8 GetBitDepth() const
	{
		return static_cast<uint8>((format & bitMask) >> bitOffset);
	}
	// Get channel layout
	Channels GetChannelFormat() const
	{
		return static_cast<Channels>((format & channelMask) >> channelOffset);
	}
	// Get number of channels
	uint8 GetNumChannels() const
	{
		return GetChannelFormat() == mono ? 1u : 2u;
	}
	// Get sample byte order
	Endianness GetEndianness() const
	{
		return static_cast<Endianness>((format & endianMask) >> endianOffset);
	}
	// Get sample format / encoding
	Encoding GetEncoding() const
	{
		return static_cast<Encoding>((format & encodingMask) >> encodingOffset);
	}

	// Read a sample from memory
	size_t ReadSample(ModSample &sample, FileReader &file) const;

#ifndef MODPLUG_NO_FILESAVE
	// Write a sample to file
	size_t WriteSample(FILE *f, const ModSample &sample, SmpLength maxSamples = 0) const;
#endif // MODPLUG_NO_FILESAVE
};
