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


// External decompressors
extern void AMSUnpack(const char * const source, size_t sourceSize, void * const dest, const size_t destSize, char packCharacter);
extern uint16 MDLReadBits(uint32 &bitbuf, uint32 &bitnum, const uint8 *(&ibuf), int8 n);
extern int DMFUnpack(LPBYTE psample, const uint8 *ibuf, const uint8 *ibufmax, uint32 maxlen);


// Read a sample from memory
size_t SampleIO::ReadSample(ModSample &sample, FileReader &file) const
//--------------------------------------------------------------------
{
	if(sample.nLength < 1 || !file.IsValid())
	{
		return 0;
	}

	LimitMax(sample.nLength, MAX_SAMPLE_LENGTH);

	const uint8 * const sourceBuf = reinterpret_cast<const uint8 *>(file.GetRawData());
	const FileReader::off_t fileSize = file.BytesLeft(), filePosition = file.GetPosition();
	FileReader::off_t bytesRead = 0;	// Amount of memory that has been read from file

	sample.uFlags.set(CHN_16BIT, GetBitDepth() >= 16);
	sample.uFlags.set(CHN_STEREO, GetChannelFormat() != mono);
	size_t sampleSize = sample.AllocateSample();	// Target sample size in bytes

	if(sampleSize == 0)
	{
		sample.nLength = 0;
		return 0;
	}

	ASSERT(sampleSize >= sample.GetSampleSizeInBytes());

	//////////////////////////////////////////////////////
	// 8-Bit / Mono / PCM
	if(GetBitDepth() == 8 && GetChannelFormat() == mono)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 8-Bit / Mono / Signed / PCM
			bytesRead = CopyMonoSample<ReadInt8PCM<0> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 8-Bit / Mono / Unsigned / PCM
			bytesRead = CopyMonoSample<ReadInt8PCM<0x80u> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 8-Bit / Mono / Delta / PCM
			bytesRead = CopyMonoSample<ReadInt8DeltaPCM>(sample, sourceBuf, fileSize);
			break;
		case PCM7to8:		// 7 Bit stored as 8-Bit with highest bit unused / Mono / Signed / PCM
			bytesRead = CopyMonoSample<ReadInt7to8PCM<0> >(sample, sourceBuf, fileSize);
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
			bytesRead = CopyStereoSplitSample<ReadInt8PCM<0> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 8-Bit / Stereo Split / Unsigned / PCM
			bytesRead = CopyStereoSplitSample<ReadInt8PCM<0x80u> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 8-Bit / Stereo Split / Delta / PCM
			bytesRead = CopyStereoSplitSample<ReadInt8DeltaPCM>(sample, sourceBuf, fileSize);
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
			bytesRead = CopyStereoInterleavedSample<ReadInt8PCM<0> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 8-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyStereoInterleavedSample<ReadInt8PCM<0x80u> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 8-Bit / Stereo Interleaved / Delta / PCM
			bytesRead = CopyStereoInterleavedSample<ReadInt8DeltaPCM>(sample, sourceBuf, fileSize);
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
			bytesRead = CopyMonoSample<ReadInt16PCM<0, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyMonoSample<ReadInt16PCM<0x8000u, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Interleaved / Delta / PCM
			bytesRead = CopyMonoSample<ReadInt16DeltaPCM<littleEndian16> >(sample, sourceBuf, fileSize);
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
			bytesRead = CopyMonoSample<ReadInt16PCM<0, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Mono / Unsigned / PCM
			bytesRead = CopyMonoSample<ReadInt16PCM<0x8000u, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Mono / Delta / PCM
			bytesRead = CopyMonoSample<ReadInt16DeltaPCM<bigEndian16> >(sample, sourceBuf, fileSize);
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
			bytesRead = CopyStereoSplitSample<ReadInt16PCM<0, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Split / Unsigned / PCM
			bytesRead = CopyStereoSplitSample<ReadInt16PCM<0x8000u, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Split / Delta / PCM
			bytesRead = CopyStereoSplitSample<ReadInt16DeltaPCM<littleEndian16> >(sample, sourceBuf, fileSize);
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
			bytesRead = CopyStereoSplitSample<ReadInt16PCM<0, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Split / Unsigned / PCM
			bytesRead = CopyStereoSplitSample<ReadInt16PCM<0x8000u, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Split / Delta / PCM
			bytesRead = CopyStereoSplitSample<ReadInt16DeltaPCM<bigEndian16> >(sample, sourceBuf, fileSize);
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
			bytesRead = CopyStereoInterleavedSample<ReadInt16PCM<0, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyStereoInterleavedSample<ReadInt16PCM<0x8000u, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Interleaved / Delta / PCM
			bytesRead = CopyStereoInterleavedSample<ReadInt16DeltaPCM<littleEndian16> >(sample, sourceBuf, fileSize);
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
			bytesRead = CopyStereoInterleavedSample<ReadInt16PCM<0, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyStereoInterleavedSample<ReadInt16PCM<0x8000u, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Interleaved / Delta / PCM
			bytesRead = CopyStereoInterleavedSample<ReadInt16DeltaPCM<bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 24-Bit / Signed / Mono, Stereo Interleaved / PCM
	else if(GetBitDepth() == 24 && (GetChannelFormat() == mono || GetChannelFormat() == stereoInterleaved) && GetEncoding() == signedPCM)
	{
		// Normalize to 16-Bit
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyAndNormalizeSample<ReadBigIntToInt16PCMandNormalize<ReadInt24to32PCM<0, littleEndian24> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyAndNormalizeSample<ReadBigIntToInt16PCMandNormalize<ReadInt24to32PCM<0, bigEndian24> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Signed / Mono, Stereo Interleaved / PCM
	else if(GetBitDepth() == 32 && (GetChannelFormat() == mono || GetChannelFormat() == stereoInterleaved) && GetEncoding() == signedPCM)
	{
		// Normalize to 16-Bit
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyAndNormalizeSample<ReadBigIntToInt16PCMandNormalize<ReadInt32PCM<0, littleEndian32> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyAndNormalizeSample<ReadBigIntToInt16PCMandNormalize<ReadInt32PCM<0, bigEndian32> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Mono, Stereo Interleaved / PCM
	else if(GetBitDepth() == 32 && (GetChannelFormat() == mono || GetChannelFormat() == stereoInterleaved) && GetEncoding() == floatPCM)
	{
		// Normalize to 16-Bit
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyAndNormalizeSample<ReadFloat32to16PCMandNormalize<littleEndian32> >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyAndNormalizeSample<ReadFloat32to16PCMandNormalize<bigEndian32> >(sample, sourceBuf, fileSize);
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

			const uint8 *inBuf = sourceBuf + sizeof(compressionTable);
			int8 *outBuf = static_cast<int8 *>(sample.pSample);
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

			LimitMax(sourceSize, file.BytesLeft());
			bytesRead = 9 + sourceSize;

			AMSUnpack(reinterpret_cast<const char *>(sourceBuf) + 9, sourceSize, sample.pSample, sample.GetSampleSizeInBytes(), packCharacter);
		}
	} else if(GetEncoding() == PTM8Dto16 && GetChannelFormat() == mono && GetBitDepth() == 16)
	{
		// PTM 8-Bit delta to 16-Bit sample
		bytesRead = CopyMonoSample<ReadInt8to16DeltaPCM>(sample, sourceBuf, fileSize);
	} else if(GetEncoding() == MDL && GetChannelFormat() == mono && GetBitDepth() <= 16)
	{
		// Huffman MDL compressed samples
		if(fileSize > 4)
		{
			uint32 bitBuf = file.ReadUint32LE(), bitNum = 32;

			uint8 *outBuf8 = static_cast<uint8 *>(sample.pSample);
			uint16 *outBuf16 = static_cast<uint16 *>(sample.pSample);
			const uint8 *inBuf = sourceBuf + 4;

			uint8 dlt = 0, lowbyte = 0;
			for(SmpLength j = 0; j < sample.nLength; j++)
			{
				uint8 hibyte;
				uint8 sign;
				if(GetBitDepth() == 16)
				{
					lowbyte = static_cast<uint8>(MDLReadBits(bitBuf, bitNum, inBuf, 8));
				}
				sign = static_cast<uint8>(MDLReadBits(bitBuf, bitNum, inBuf, 1));
				if (MDLReadBits(bitBuf, bitNum, inBuf, 1))
				{
					hibyte = static_cast<uint8>(MDLReadBits(bitBuf, bitNum, inBuf, 3));
				} else
				{
					hibyte = 8;
					while (!MDLReadBits(bitBuf, bitNum, inBuf, 1)) hibyte += 0x10;
					hibyte += static_cast<uint8>(MDLReadBits(bitBuf, bitNum, inBuf, 4));
				}
				if (sign) hibyte = ~hibyte;
				dlt += hibyte;
				if(GetBitDepth() != 16)
				{
					outBuf8[j] = dlt;
				} else
				{
					outBuf16[j] = lowbyte | (dlt << 8);
				}
			}

			bytesRead = fileSize;
		}
	} else if(GetEncoding() == DMF && GetChannelFormat() == mono && GetBitDepth() <= 16)
	{
		// DMF Huffman compression
		if(fileSize > 4)
		{
			const uint8 *inBuf = sourceBuf;
			const uint8 *inBufMax = inBuf + fileSize;
			uint8 *outBuf = static_cast<uint8 *>(sample.pSample);
			bytesRead = DMFUnpack(outBuf, inBuf, inBufMax, sample.GetSampleSizeInBytes());
		}
	}

	if(bytesRead > 0)
	{
		CSoundFile::AdjustSampleLoop(sample);
	}

	ASSERT(filePosition + bytesRead <= file.GetLength());
	file.Seek(filePosition + bytesRead);
	return bytesRead;
}


#ifndef MODPLUG_NO_FILESAVE

// Write a sample to file
size_t SampleIO::WriteSample(FILE *f, const ModSample &sample, SmpLength maxSamples) const
//----------------------------------------------------------------------------------------
{
	size_t len = 0, bufcount = 0;
	char buffer[32768];
	int8 *pSample = (int8 *)sample.pSample;
	SmpLength numSamples = sample.nLength;

	if(maxSamples && numSamples > maxSamples) numSamples = maxSamples;
	if(pSample == nullptr || numSamples == 0) return 0;

	if(GetBitDepth() == 16 && GetChannelFormat() == mono && GetEndianness() == littleEndian &&
		(GetEncoding() == signedPCM || GetEncoding() == unsignedPCM || GetEncoding() == deltaPCM))
	{
		// 16-bit mono samples
		int16 *p = (int16 *)pSample;
		int s_old = 0, s_ofs;
		len = numSamples * 2;
		s_ofs = (GetEncoding() == unsignedPCM) ? 0x8000 : 0;
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
				*((int16 *)(&buffer[bufcount])) = (int16)(s_new - s_old);
				s_old = s_new;
			} else
			{
				*((int16 *)(&buffer[bufcount])) = (int16)(s_new + s_ofs);
			}
			bufcount += 2;
			if(bufcount >= sizeof(buffer) - 1)
			{
				if(f) fwrite(buffer, 1, bufcount, f);
				bufcount = 0;
			}
		}
		if (bufcount) if(f) fwrite(buffer, 1, bufcount, f);
	}

	else if(GetBitDepth() == 8 && GetChannelFormat() == stereoSplit &&
		(GetEncoding() == signedPCM || GetEncoding() == unsignedPCM || GetEncoding() == deltaPCM))
	{
		// 8-bit Stereo samples (not interleaved)
		int s_ofs = (GetEncoding() == unsignedPCM) ? 0x80 : 0;
		for (UINT iCh=0; iCh<2; iCh++)
		{
			int8 *p = ((int8 *)pSample) + iCh;
			int s_old = 0;

			bufcount = 0;
			for (UINT j=0; j<numSamples; j++)
			{
				int s_new = *p;
				p += 2;
				if (GetEncoding() == deltaPCM)
				{
					buffer[bufcount++] = (int8)(s_new - s_old);
					s_old = s_new;
				} else
				{
					buffer[bufcount++] = (int8)(s_new + s_ofs);
				}
				if (bufcount >= sizeof(buffer))
				{
					if(f) fwrite(buffer, 1, bufcount, f);
					bufcount = 0;
				}
			}
			if (bufcount) if(f) fwrite(buffer, 1, bufcount, f);
		}
		len = numSamples * 2;
	}

	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoSplit && GetEndianness() == littleEndian &&
		(GetEncoding() == signedPCM || GetEncoding() == unsignedPCM || GetEncoding() == deltaPCM))
	{
		// 16-bit Stereo samples (not interleaved)
		int s_ofs = (GetEncoding() == unsignedPCM) ? 0x8000 : 0;
		for (UINT iCh=0; iCh<2; iCh++)
		{
			int16 *p = ((int16 *)pSample) + iCh;
			int s_old = 0;

			bufcount = 0;
			for (UINT j=0; j<numSamples; j++)
			{
				int s_new = *p;
				p += 2;
				if (GetEncoding() == deltaPCM)
				{
					*((int16 *)(&buffer[bufcount])) = (int16)(s_new - s_old);
					s_old = s_new;
				} else
				{
					*((int16 *)(&buffer[bufcount])) = (int16)(s_new + s_ofs);
				}
				bufcount += 2;
				if (bufcount >= sizeof(buffer))
				{
					if(f) fwrite(buffer, 1, bufcount, f);
					bufcount = 0;
				}
			}
			if (bufcount) if(f) fwrite(buffer, 1, bufcount, f);
		}
		len = numSamples * 4;
	}

	else if((GetBitDepth() == 8 || (GetBitDepth() == 16 && GetEndianness() == littleEndian)) && GetChannelFormat() == stereoInterleaved && GetEncoding() == signedPCM)
	{
		//	Stereo signed interleaved
		if(f) fwrite(pSample, 1, sample.GetSampleSizeInBytes(), f);
	}

	else if(GetBitDepth() == 8 && GetChannelFormat() == stereoInterleaved && GetEncoding() == unsignedPCM)
	{
		// Stereo unsigned interleaved
		len = numSamples * 2;
		for(SmpLength j = 0; j < len; j++)
		{
			*((uint8 *)(&buffer[bufcount])) = *((uint8 *)(&pSample[j])) + 0x80;
			bufcount++;
			if (bufcount >= sizeof(buffer))
			{
				if(f) fwrite(buffer, 1, bufcount, f);
				bufcount = 0;
			}
		}
		if (bufcount) if(f) fwrite(buffer, 1, bufcount, f);
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
		int8 *p = (int8 *)pSample;
		int sinc = (sample.uFlags & CHN_16BIT) ? 2 : 1;
		int s_old = 0, s_ofs = (GetEncoding() == unsignedPCM) ? 0x80 : 0;
		if (sample.uFlags & CHN_16BIT) p++;

		for (UINT j=0; j<len; j++)
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
				buffer[bufcount++] = (int8)(s_new - s_old);
				s_old = s_new;
			} else
			{
				buffer[bufcount++] = (int8)(s_new + s_ofs);
			}
			if (bufcount >= sizeof(buffer))
			{
				if(f) fwrite(buffer, 1, bufcount, f);
				bufcount = 0;
			}
		}
		if (bufcount) if(f) fwrite(buffer, 1, bufcount, f);
	}
	return len;
}

#endif // MODPLUG_NO_FILESAVE
