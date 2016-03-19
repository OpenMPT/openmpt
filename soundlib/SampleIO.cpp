/*
 * SampleIO.cpp
 * ------------
 * Purpose: Central code for reading and writing samples. Create your SampleIO object and have a go at the ReadSample and WriteSample functions!
 * Notes  : Not all combinations of possible sample format combinations are implemented, especially for WriteSample.
 *          Using the existing generic functions, it should be quite easy to extend the code, though.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "SampleIO.h"
#include "SampleFormatConverters.h"
#include "ITCompression.h"
#include "../common/mptIO.h"
#ifndef MODPLUG_NO_FILESAVE
#include "../common/mptFileIO.h"
#endif


OPENMPT_NAMESPACE_BEGIN

// Sample decompression routines in other source files
void AMSUnpack(const int8 * const source, size_t sourceSize, void * const dest, const size_t destSize, char packCharacter);
uint16 MDLReadBits(uint32 &bitbuf, uint32 &bitnum, const uint8 *(&ibuf), size_t &bytesLeft, int8 n);
uintptr_t DMFUnpack(uint8 *psample, const uint8 *ibuf, const uint8 *ibufmax, uint32 maxlen);


#if MPT_COMPILER_GCC
#if MPT_GCC_AT_LEAST(4,6,0)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wswitch"
#elif MPT_COMPILER_CLANG
#pragma clang diagnostic push
#if MPT_CLANG_AT_LEAST(3,3,0)
#pragma clang diagnostic ignored "-Wswitch"
#else
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif
#endif

// Read a sample from memory
size_t SampleIO::ReadSample(ModSample &sample, FileReader &file) const
//--------------------------------------------------------------------
{
	if(sample.nLength < 1 || !file.IsValid())
	{
		return 0;
	}

	LimitMax(sample.nLength, MAX_SAMPLE_LENGTH);

	FileReader::off_t bytesRead = 0;	// Amount of memory that has been read from file

	FileReader::off_t filePosition = file.GetPosition();
	const mpt::byte * sourceBuf = nullptr;
	FileReader::PinnedRawDataView restrictedSampleDataView;
	FileReader::off_t fileSize = 0;
	if(UsesFileReaderForDecoding())
	{
		sourceBuf = nullptr;
		fileSize = file.BytesLeft();
	} else if(!IsVariableLengthEncoded())
	{
		restrictedSampleDataView = file.GetPinnedRawDataView(CalculateEncodedSize(sample.nLength));
		sourceBuf = restrictedSampleDataView.data();
		fileSize = restrictedSampleDataView.size();
	} else
	{
		// Only DMF or MDL sample compression encodings should fall in this case,
		MPT_ASSERT(GetEncoding() == DMF || GetEncoding() == MDL);
		// file is guaranteed by the caller to be ONLY data for this sample,
		// it is thus efficient to create a view to the whole file object.
		// See MPT_ASSERT with fileSize below.
		restrictedSampleDataView = file.GetPinnedRawDataView();
		sourceBuf = restrictedSampleDataView.data();
		fileSize = restrictedSampleDataView.size();
	}

	sample.uFlags.set(CHN_16BIT, GetBitDepth() >= 16);
	sample.uFlags.set(CHN_STEREO, GetChannelFormat() != mono);
	size_t sampleSize = sample.AllocateSample();	// Target sample size in bytes

	if(sampleSize == 0)
	{
		sample.nLength = 0;
		return 0;
	}

	MPT_ASSERT(sampleSize >= sample.GetSampleSizeInBytes());

	//////////////////////////////////////////////////////
	// 8-Bit / Mono / PCM
	if(GetBitDepth() == 8 && GetChannelFormat() == mono)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 8-Bit / Mono / Signed / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt8>(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 8-Bit / Mono / Unsigned / PCM
			bytesRead = CopyMonoSample<SC::DecodeUint8>(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 8-Bit / Mono / Delta / PCM
		case MT2:
			bytesRead = CopyMonoSample<SC::DecodeInt8Delta>(sample, sourceBuf, fileSize);
			break;
		case PCM7to8:		// 7 Bit stored as 8-Bit with highest bit unused / Mono / Signed / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt7>(sample, sourceBuf, fileSize);
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 8-Bit / Stereo Split / PCM
	else if(GetBitDepth() == 8 && GetChannelFormat() == stereoSplit)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 8-Bit / Stereo Split / Signed / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt8>(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 8-Bit / Stereo Split / Unsigned / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeUint8>(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 8-Bit / Stereo Split / Delta / PCM
		case MT2:
			bytesRead = CopyStereoSplitSample<SC::DecodeInt8Delta>(sample, sourceBuf, fileSize);
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 8-Bit / Stereo Interleaved / PCM
	else if(GetBitDepth() == 8 && GetChannelFormat() == stereoInterleaved)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 8-Bit / Stereo Interleaved / Signed / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt8>(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 8-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeUint8>(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 8-Bit / Stereo Interleaved / Delta / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt8Delta>(sample, sourceBuf, fileSize);
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Mono / Little Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == mono && GetEndianness() == littleEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Stereo Interleaved / Signed / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt16<0, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt16<0x8000u, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Interleaved / Delta / PCM
		case MT2:
			bytesRead = CopyMonoSample<SC::DecodeInt16Delta<littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Mono / Big Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == mono && GetEndianness() == bigEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Mono / Signed / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt16<0, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Mono / Unsigned / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt16<0x8000u, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Mono / Delta / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt16Delta<bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Stereo Split / Little Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoSplit && GetEndianness() == littleEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Stereo Split / Signed / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16<0, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Split / Unsigned / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16<0x8000u, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Split / Delta / PCM
		case MT2:
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16Delta<littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Stereo Split / Big Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoSplit && GetEndianness() == bigEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Stereo Split / Signed / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16<0, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Split / Unsigned / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16<0x8000u, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Split / Delta / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16Delta<bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Stereo Interleaved / Little Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoInterleaved && GetEndianness() == littleEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Stereo Interleaved / Signed / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16<0, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16<0x8000u, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Interleaved / Delta / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16Delta<littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Stereo Interleaved / Big Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoInterleaved && GetEndianness() == bigEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Stereo Interleaved / Signed / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16<0, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16<0x8000u, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Interleaved / Delta / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16Delta<bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 24-Bit / Signed / Mono / PCM
	else if(GetBitDepth() == 24 && GetChannelFormat() == mono && GetEncoding() == signedPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, bigEndian24> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 24-Bit / Signed / Stereo Interleaved / PCM
	else if(GetBitDepth() == 24 && GetChannelFormat() == stereoInterleaved && GetEncoding() == signedPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, bigEndian24> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Signed / Mono / PCM
	else if(GetBitDepth() == 32 && GetChannelFormat() == mono && GetEncoding() == signedPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, littleEndian32> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, bigEndian32> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Signed / Stereo Interleaved / PCM
	else if(GetBitDepth() == 32 && GetChannelFormat() == stereoInterleaved && GetEncoding() == signedPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, littleEndian32> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, bigEndian32> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Mono / PCM
	else if(GetBitDepth() == 32 && GetChannelFormat() == mono && GetEncoding() == floatPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<littleEndian32> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Stereo Interleaved / PCM
	else if(GetBitDepth() == 32 && GetChannelFormat() == stereoInterleaved && GetEncoding() == floatPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<littleEndian32> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 24-Bit / Signed / Mono, Stereo Interleaved / PCM
	else if(GetBitDepth() == 24 && (GetChannelFormat() == mono || GetChannelFormat() == stereoInterleaved) && GetEncoding() == signedPCMnormalize)
	{
		// Normalize to 16-Bit
		uint32 srcPeak = uint32(1)<<31;
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(sample, sourceBuf, fileSize, &srcPeak);
		} else
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, bigEndian24> > >(sample, sourceBuf, fileSize, &srcPeak);
		}
		if(bytesRead && srcPeak != uint32(1)<<31)
		{
			// Adjust sample volume so we do not affect relative volume of the sample. Normalizing is only done to increase precision.
			sample.nGlobalVol = static_cast<uint16>(Clamp(Util::muldivr_unsigned(sample.nGlobalVol, srcPeak, uint32(1)<<31), uint32(1), uint32(64)));
			sample.uFlags.set(SMP_MODIFIED);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Signed / Mono, Stereo Interleaved / PCM
	else if(GetBitDepth() == 32 && (GetChannelFormat() == mono || GetChannelFormat() == stereoInterleaved) && GetEncoding() == signedPCMnormalize)
	{
		// Normalize to 16-Bit
		uint32 srcPeak = uint32(1)<<31;
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, littleEndian32> > >(sample, sourceBuf, fileSize, &srcPeak);
		} else
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, bigEndian32> > >(sample, sourceBuf, fileSize, &srcPeak);
		}
		if(bytesRead && srcPeak != uint32(1)<<31)
		{
			// Adjust sample volume so we do not affect relative volume of the sample. Normalizing is only done to increase precision.
			sample.nGlobalVol = static_cast<uint16>(Clamp(Util::muldivr_unsigned(sample.nGlobalVol, srcPeak, uint32(1)<<31), uint32(1), uint32(64)));
			sample.uFlags.set(SMP_MODIFIED);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Mono, Stereo Interleaved / PCM
	else if(GetBitDepth() == 32 && (GetChannelFormat() == mono || GetChannelFormat() == stereoInterleaved) && GetEncoding() == floatPCMnormalize)
	{
		// Normalize to 16-Bit
		float32 srcPeak = 1.0f;
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, float32>, SC::DecodeFloat32<littleEndian32> > >(sample, sourceBuf, fileSize, &srcPeak);
		} else
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(sample, sourceBuf, fileSize, &srcPeak);
		}
		if(bytesRead && srcPeak != uint32(1)<<31)
		{
			// Adjust sample volume so we do not affect relative volume of the sample. Normalizing is only done to increase precision.
			sample.nGlobalVol = Util::Round<uint16>(Clamp(sample.nGlobalVol * srcPeak, 1.0f, 64.0f));
			sample.uFlags.set(SMP_MODIFIED);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Mono / PCM / full scale 2^15
	else if(GetBitDepth() == 32 && GetChannelFormat() == mono && GetEncoding() == floatPCM15)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<littleEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<littleEndian32>(1.0f / static_cast<float>(1<<15)))
				);
		} else
		{
			bytesRead = CopyMonoSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<bigEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<bigEndian32>(1.0f / static_cast<float>(1<<15)))
				);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Stereo Interleaved / PCM / full scale 2^15
	else if(GetBitDepth() == 32 && GetChannelFormat() == stereoInterleaved && GetEncoding() == floatPCM15)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<littleEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<littleEndian32>(1.0f / static_cast<float>(1<<15)))
				);
		} else
		{
			bytesRead = CopyStereoInterleavedSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<bigEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<bigEndian32>(1.0f / static_cast<float>(1<<15)))
				);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Stereo Interleaved / PCM / full scale 2^23
	else if(GetBitDepth() == 32 && GetChannelFormat() == mono && GetEncoding() == floatPCM23)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<littleEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<littleEndian32>(1.0f / static_cast<float>(1<<23)))
				);
		} else
		{
			bytesRead = CopyMonoSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<bigEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<bigEndian32>(1.0f / static_cast<float>(1<<23)))
				);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Stereo Interleaved / PCM / full scale 2^23
	else if(GetBitDepth() == 32 && GetChannelFormat() == stereoInterleaved && GetEncoding() == floatPCM23)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<littleEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<littleEndian32>(1.0f / static_cast<float>(1<<23)))
				);
		} else
		{
			bytesRead = CopyStereoInterleavedSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<bigEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<bigEndian32>(1.0f / static_cast<float>(1<<23)))
				);
		}
	}

	//////////////////////////////////////////////////////
	// Compressed samples
	if(*this == SampleIO(_8bit, mono, littleEndian, ADPCM))
	{
		// 4-Bit ADPCM data
		int8 compressionTable[16];	// ADPCM Compression LUT
		if(file.ReadArray(compressionTable))
		{
			size_t readLength = (sample.nLength + 1) / 2;
			LimitMax(readLength, file.BytesLeft());

			const uint8 *inBuf = mpt::byte_cast<const uint8*>(sourceBuf) + sizeof(compressionTable);
			int8 *outBuf = sample.pSample8;
			int8 delta = 0;

			for(size_t i = readLength; i != 0; i--)
			{
				delta += compressionTable[*inBuf & 0x0F];
				*(outBuf++) = delta;
				delta += compressionTable[(*inBuf >> 4) & 0x0F];
				*(outBuf++) = delta;
				inBuf++;
			}
			bytesRead = sizeof(compressionTable) + readLength;
		}
	} else if(GetEncoding() == IT214 || GetEncoding() == IT215)
	{
		// IT 2.14 / 2.15 compressed samples
		ITDecompression(file, sample, GetEncoding() == IT215);
		bytesRead = file.GetPosition() - filePosition;
	} else if(GetEncoding() == AMS && GetChannelFormat() == mono)
	{
		// AMS compressed samples
		if(fileSize > 9)
		{
			file.Skip(4);	// Target sample size (we already know this)
			uint32 sourceSize = file.ReadUint32LE();
			int8 packCharacter = file.ReadUint8();
			bytesRead += 9;
			
			FileReader::PinnedRawDataView packedDataView = file.ReadPinnedRawDataView(sourceSize);
			LimitMax(sourceSize, mpt::saturate_cast<uint32>(packedDataView.size()));
			bytesRead += sourceSize;

			AMSUnpack(reinterpret_cast<const int8 *>(packedDataView.data()), packedDataView.size(), sample.pSample, sample.GetSampleSizeInBytes(), packCharacter);
		}
	} else if(GetEncoding() == PTM8Dto16 && GetChannelFormat() == mono && GetBitDepth() == 16)
	{
		// PTM 8-Bit delta to 16-Bit sample
		bytesRead = CopyMonoSample<SC::DecodeInt16Delta8>(sample, sourceBuf, fileSize);
	} else if(GetEncoding() == MDL && GetChannelFormat() == mono && GetBitDepth() <= 16)
	{
		// Huffman MDL compressed samples
		if(fileSize > 4)
		{
			uint32 bitBuf = file.ReadUint32LE(), bitNum = 32;

			const uint8 *inBuf = reinterpret_cast<const uint8*>(sourceBuf) + 4;
			size_t bytesLeft = file.BytesLeft();

			uint8 dlt = 0, lowbyte = 0;
			for(SmpLength j = 0; j < sample.nLength; j++)
			{
				uint8 hibyte;
				uint8 sign;
				if(GetBitDepth() == 16)
				{
					lowbyte = static_cast<uint8>(MDLReadBits(bitBuf, bitNum, inBuf, bytesLeft, 8));
				}
				sign = static_cast<uint8>(MDLReadBits(bitBuf, bitNum, inBuf, bytesLeft, 1));
				if (MDLReadBits(bitBuf, bitNum, inBuf, bytesLeft, 1))
				{
					hibyte = static_cast<uint8>(MDLReadBits(bitBuf, bitNum, inBuf, bytesLeft, 3));
				} else
				{
					hibyte = 8;
					while (!MDLReadBits(bitBuf, bitNum, inBuf, bytesLeft, 1)) hibyte += 0x10;
					hibyte += static_cast<uint8>(MDLReadBits(bitBuf, bitNum, inBuf, bytesLeft, 4));
				}
				if (sign) hibyte = ~hibyte;
				dlt += hibyte;
				if(GetBitDepth() != 16)
				{
					sample.pSample8[j] = dlt;
				} else
				{
					sample.pSample16[j] = lowbyte | (dlt << 8);
				}
			}

			// This assertion ensures that, when using variable length samples,
			// the caller actually provided a trimmed chunk to read the sample data from.
			// This is required as we cannot know the encoded sample data size upfront
			// to construct a properly sized pinned view.
			MPT_ASSERT(bytesLeft == 0);

			bytesRead = fileSize;
		}
	} else if(GetEncoding() == DMF && GetChannelFormat() == mono && GetBitDepth() <= 16)
	{
		// DMF Huffman compression
		if(fileSize > 4)
		{
			const uint8 *inBuf = mpt::byte_cast<const uint8*>(sourceBuf);
			const uint8 *inBufMax = inBuf + fileSize;
			uint8 *outBuf = static_cast<uint8 *>(sample.pSample);
			bytesRead = DMFUnpack(outBuf, inBuf, inBufMax, sample.GetSampleSizeInBytes());

			// This assertion ensures that, when using variable length samples,
			// the caller actually provided a trimmed chunk to read the sample data from.
			// This is required as we cannot know the encoded sample data size upfront
			// to construct a properly sized pinned view.
			MPT_ASSERT(bytesRead == fileSize);

		}
	} else if(GetEncoding() == MT2 && GetChannelFormat() == stereoSplit && GetBitDepth() <= 16)
	{
		// MT2 stereo samples (right channel is stored as a difference from the left channel)
		if(GetBitDepth() == 8)
		{
			for(SmpLength i = 0; i <= sample.nLength * 2; i += 2)
			{
				sample.pSample8[i + 1] = static_cast<int8>(static_cast<uint8>(sample.pSample8[i + 1]) + static_cast<uint8>(sample.pSample8[i]));
			}
		} else
		{
			for(SmpLength i = 0; i <= sample.nLength * 2; i += 2)
			{
				sample.pSample16[i + 1] += static_cast<int16>(static_cast<uint16>(sample.pSample16[i + 1]) + static_cast<uint16>(sample.pSample16[i]));
			}
		}
	}

	MPT_ASSERT(filePosition + bytesRead <= file.GetLength());
	file.Seek(filePosition + bytesRead);
	return bytesRead;
}

#if MPT_COMPILER_GCC
#if MPT_GCC_AT_LEAST(4,6,0)
#pragma GCC diagnostic pop
#endif
#elif MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif


#ifndef MODPLUG_NO_FILESAVE


// Write a sample to file
size_t SampleIO::WriteSample(std::ostream *f, const ModSample &sample, SmpLength maxSamples) const
//------------------------------------------------------------------------------------------------
{
	if(!sample.HasSampleData()) return 0;

	size_t len = 0, bufcount = 0;
	union
	{
		int8 buffer8[4096];
		int16 buffer16[4096];
	};
	const void *const pSampleVoid = sample.pSample;
	const int8 *const pSample8 = sample.pSample8;
	const int16 *const pSample16 = sample.pSample16;
	SmpLength numSamples = sample.nLength;

	if(maxSamples && numSamples > maxSamples) numSamples = maxSamples;

	if(GetBitDepth() == 16 && GetChannelFormat() == mono && GetEndianness() == littleEndian &&
		(GetEncoding() == signedPCM || GetEncoding() == unsignedPCM || GetEncoding() == deltaPCM))
	{
		// 16-bit little-endian mono samples
		const int16 *p = pSample16;
		int s_old = 0;
		len = numSamples * 2;
		const int s_ofs = (GetEncoding() == unsignedPCM) ? 0x8000 : 0;
		for(SmpLength j = 0; j < numSamples; j++)
		{
			int s_new = *p;
			p++;
			if(sample.uFlags[CHN_STEREO])
			{
				// Downmix stereo
				s_new = (s_new + (*p) + 1) >> 1;
				p++;
			}
			if(GetEncoding() == deltaPCM)
			{
				buffer16[bufcount] = SwapBytesReturnLE((int16)(s_new - s_old));
				s_old = s_new;
			} else
			{
				buffer16[bufcount] = SwapBytesReturnLE((int16)(s_new + s_ofs));
			}
			bufcount++;
			if(bufcount >= CountOf(buffer16))
			{
				if(f) mpt::IO::WriteRaw(*f, reinterpret_cast<mpt::byte*>(buffer16), bufcount * 2);
				bufcount = 0;
			}
		}
		if (bufcount) if(f) mpt::IO::WriteRaw(*f, reinterpret_cast<mpt::byte*>(buffer16), bufcount * 2);
	}

	else if(GetBitDepth() == 8 && GetChannelFormat() == stereoSplit &&
		(GetEncoding() == signedPCM || GetEncoding() == unsignedPCM || GetEncoding() == deltaPCM))
	{
		// 8-bit Stereo samples (not interleaved)
		const int s_ofs = (GetEncoding() == unsignedPCM) ? 0x80 : 0;
		for (uint32 iCh=0; iCh<2; iCh++)
		{
			const int8 *p = pSample8 + iCh;
			int s_old = 0;

			bufcount = 0;
			for (uint32 j=0; j<numSamples; j++)
			{
				int s_new = *p;
				p += 2;
				if (GetEncoding() == deltaPCM)
				{
					buffer8[bufcount++] = (int8)(s_new - s_old);
					s_old = s_new;
				} else
				{
					buffer8[bufcount++] = (int8)(s_new + s_ofs);
				}
				if(bufcount >= CountOf(buffer8))
				{
					if(f) mpt::IO::WriteRaw(*f, reinterpret_cast<mpt::byte*>(buffer8), bufcount);
					bufcount = 0;
				}
			}
			if (bufcount) if(f) mpt::IO::WriteRaw(*f, reinterpret_cast<mpt::byte*>(buffer8), bufcount);
		}
		len = numSamples * 2;
	}

	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoSplit && GetEndianness() == littleEndian &&
		(GetEncoding() == signedPCM || GetEncoding() == unsignedPCM || GetEncoding() == deltaPCM))
	{
		// 16-bit little-endian Stereo samples (not interleaved)
		const int s_ofs = (GetEncoding() == unsignedPCM) ? 0x8000 : 0;
		for (uint32 iCh=0; iCh<2; iCh++)
		{
			const int16 *p = pSample16 + iCh;
			int s_old = 0;

			bufcount = 0;
			for (SmpLength j=0; j<numSamples; j++)
			{
				int s_new = *p;
				p += 2;
				if (GetEncoding() == deltaPCM)
				{
					buffer16[bufcount] = SwapBytesReturnLE((int16)(s_new - s_old));
					s_old = s_new;
				} else
				{
					buffer16[bufcount] = SwapBytesReturnLE((int16)(s_new + s_ofs));
				}
				bufcount++;
				if(bufcount >= CountOf(buffer16))
				{
					if(f) mpt::IO::WriteRaw(*f, reinterpret_cast<mpt::byte*>(buffer16), bufcount * 2);
					bufcount = 0;
				}
			}
			if (bufcount) if(f) mpt::IO::WriteRaw(*f, reinterpret_cast<mpt::byte*>(buffer16), bufcount * 2);
		}
		len = numSamples * 4;
	}

	else if((GetBitDepth() == 8 || (GetBitDepth() == 16 && GetEndianness() == littleEndian)) && GetChannelFormat() == stereoInterleaved && GetEncoding() == signedPCM)
	{
		// Stereo signed interleaved
		len = sample.GetSampleSizeInBytes();
		if(f) mpt::IO::WriteRaw(*f, mpt::void_cast<const mpt::byte*>(pSampleVoid), len);
	}

	else if(GetBitDepth() == 8 && GetChannelFormat() == stereoInterleaved && GetEncoding() == unsignedPCM)
	{
		// Stereo unsigned interleaved
		len = numSamples * 2;
		for(SmpLength j = 0; j < len; j++)
		{
			buffer8[bufcount] = (int8)((uint8)(pSample8[j]) + 0x80);
			bufcount++;
			if(bufcount >= CountOf(buffer8))
			{
				if(f) mpt::IO::WriteRaw(*f, reinterpret_cast<mpt::byte*>(buffer8), bufcount);
				bufcount = 0;
			}
		}
		if (bufcount) if(f) mpt::IO::WriteRaw(*f, reinterpret_cast<mpt::byte*>(buffer8), bufcount);
	}

	else if(GetEncoding() == IT214 || GetEncoding() == IT215)
	{
		// IT2.14-encoded samples
		ITCompression its(sample, GetEncoding() == IT215, f);
		len = its.GetCompressedSize();
	}

	// Default: assume 8-bit PCM data
	else
	{
		len = numSamples;
		const int8 *p = pSample8;
		int sinc = (sample.uFlags & CHN_16BIT) ? 2 : 1;
		int s_old = 0;
		const int s_ofs = (GetEncoding() == unsignedPCM) ? 0x80 : 0;
		if (sample.uFlags & CHN_16BIT) p++;

		for (SmpLength j=0; j<len; j++)
		{
			int s_new = (int8)(*p);
			p += sinc;
			if (sample.uFlags & CHN_STEREO)
			{
				s_new = (s_new + ((int)*p) + 1) >> 1;
				p += sinc;
			}
			if (GetEncoding() == deltaPCM)
			{
				buffer8[bufcount++] = (int8)(s_new - s_old);
				s_old = s_new;
			} else
			{
				buffer8[bufcount++] = (int8)(s_new + s_ofs);
			}
			if(bufcount >= CountOf(buffer8))
			{
				if(f) mpt::IO::WriteRaw(*f, reinterpret_cast<mpt::byte*>(buffer8), bufcount);
				bufcount = 0;
			}
		}
		if (bufcount) if(f) mpt::IO::WriteRaw(*f, reinterpret_cast<mpt::byte*>(buffer8), bufcount);
	}
	return len;
}


// Write a sample to file
size_t SampleIO::WriteSample(std::ostream &f, const ModSample &sample, SmpLength maxSamples) const
//------------------------------------------------------------------------------------------------
{
	return WriteSample(&f, sample, maxSamples);
}


// Write a sample to file
size_t SampleIO::WriteSample(FILE *f, const ModSample &sample, SmpLength maxSamples) const
//----------------------------------------------------------------------------------------
{
	mpt::FILE_ostream s(f);
	return WriteSample(f ? &s : nullptr, sample, maxSamples);
}


#endif // MODPLUG_NO_FILESAVE


OPENMPT_NAMESPACE_END
