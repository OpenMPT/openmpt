/*
 * ITCompression.cpp
 * -----------------
 * Purpose: Code for IT sample compression and decompression.
 * Notes  : The original Python compression code was written by GreaseMonkey and has been released into the public domain.
 * Authors: OpenMPT Devs
 *          Ben "GreaseMonkey" Russell
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include <stdafx.h>
#include "ITCompression.h"
#include "../common/misc_util.h"

// Algorithm parameters for 16-Bit samples
struct IT16BitParams
{
	typedef int16 sample_t;
	static const int16 lowerTab[];
	static const int16 upperTab[];
	static const int fetchA = 4, lowerB = -8, upperB = 7, defWidth = 17;
	static int Clamp(sample_t v) { return int(v) & 0xFFFF; }
};

const int16 IT16BitParams::lowerTab[] = { 0, -1, -3, -7, -15, -31, -56, -120, -248, -504, -1016, -2040, -4088, -8184, -16376, -32760, -32768 };
const int16 IT16BitParams::upperTab[] = { 0, 1, 3, 7, 15, 31, 55, 119, 247, 503, 1015, 2039, 4087, 8183, 16375, 32759, 32767 };

// Algorithm parameters for 8-Bit samples
struct IT8BitParams
{
	typedef int8 sample_t;
	static const int8 lowerTab[];
	static const int8 upperTab[];
	static const int fetchA = 3, lowerB = -4, upperB = 3, defWidth = 9;
	static int Clamp(sample_t v) { return int(v) & 0xFF; }
};

const int8 IT8BitParams::lowerTab[] = { 0, -1, -3, -7, -15, -31, -60, -124, -128 };
const int8 IT8BitParams::upperTab[] = { 0, 1, 3, 7, 15, 31, 59, 123, 127 };

static const int ITWidthChangeSize[] = { 4, 5, 6, 7, 8, 9, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 };

//////////////////////////////////////////////////////////////////////////////
// IT 2.14 compression


ITCompression::ITCompression(const ModSample &sample, bool it215, FILE *f) : mptSample(sample), is215(it215), file(f)
//-------------------------------------------------------------------------------------------------------------------
{
	packedData = new (std::nothrow) uint8[bufferSize];
	sampleData = new (std::nothrow) uint8[blockSize];
	packedTotalLength = 0;
	if(packedData == nullptr || sampleData == nullptr)
	{
		return;
	}

	for(uint8 chn = 0; chn < mptSample.GetNumChannels(); chn++)
	{
		SmpLength offset = 0;
		SmpLength remain = mptSample.nLength;
		while(remain > 0)
		{
			// Initialise output buffer and bit writer positions
			packedLength = 2;
			bitPos = 0;
			remBits = 8;
			byteVal = 0;

			if(mptSample.GetElementarySampleSize() > 1)
				Compress<IT16BitParams>(sample.pSample + 2 * chn, offset, remain);
			else
				Compress<IT8BitParams>(sample.pSample + chn, offset, remain);

			if(file) fwrite(&packedData[0], packedLength, 1, file);
			packedTotalLength += packedLength;

			offset += baseLength;
			remain -= baseLength;
		}
	}

	delete[] packedData;
	delete[] sampleData;
}


template<typename T>
void ITCompression::CopySample(void *target, const void *source, SmpLength offset, SmpLength length, SmpLength skip)
//------------------------------------------------------------------------------------------------------------------
{
	T *out = static_cast<T *>(target);
	const T *in = static_cast<const T *>(source) + offset * skip;
	for(SmpLength i = 0, j = 0; j < length; i += skip, j++)
	{
		out[j] = in[i];
	}
}


// Convert sample to delta values.
template<typename T>
void ITCompression::Deltafy()
//---------------------------
{
	T *p = static_cast<T *>(sampleData);
	int oldVal = 0;
	for(SmpLength i = 0; i < baseLength; i++)
	{
		int newVal = p[i];
		p[i] = static_cast<T>(newVal - oldVal);
		oldVal = newVal;
	}
}


template<typename Properties>
void ITCompression::Compress(const void *data, SmpLength offset, SmpLength actualLength)
//--------------------------------------------------------------------------------------
{
	baseLength = Util::Min(actualLength, SmpLength(blockSize / sizeof(Properties::sample_t)));

	CopySample<Properties::sample_t>(sampleData, data, offset, baseLength, mptSample.GetNumChannels());

	Deltafy<Properties::sample_t>();
	if(is215)
	{
		Deltafy<Properties::sample_t>();
	}

	// Initialise bit width table with initial values
	bwt.assign(baseLength, Properties::defWidth);

	// Recurse!
	SquishRecurse<Properties>(Properties::defWidth, Properties::defWidth, Properties::defWidth, Properties::defWidth - 2, 0, baseLength);
	
	// Write those bits!
	const Properties::sample_t *p = static_cast<Properties::sample_t *>(sampleData);
	int width = Properties::defWidth;
	for(size_t i = 0; i < baseLength; i++)
	{
		if(bwt[i] != width)
		{
			if(width <= 6)
			{
				// Mode A
				ASSERT(width);
				WriteBits(width, (1 << (width - 1)));
				WriteBits(Properties::fetchA, ConvertWidth(width, bwt[i]));
			} else if(width < Properties::defWidth)
			{
				// Mode B
				int xv = (1 << (width - 1)) + Properties::lowerB + ConvertWidth(width, bwt[i]);
				WriteBits(width, xv);
			} else
			{
				// Mode C
				ASSERT((bwt[i] - 1) >= 0);
				WriteBits(width, (1 << (width - 1)) + bwt[i] - 1);
			}

			width = bwt[i];
		}
		WriteBits(width, Properties::Clamp(p[i]));
	}

	// Write last byte and update block length
	WriteByte(byteVal);
	packedData[0] = uint8((packedLength - 2) & 0xFF);
	packedData[1] = uint8((packedLength - 2) >> 8);

	Verify(data, sampleData, offset);
}


#ifdef MODPLUG_TRACKER
#include "../mptrack/Mptrack.h"	// For config filename
#endif // MODPLUG_TRACKER

// Check integrity of compressed data
void ITCompression::Verify(const void *data, void *sampleData, SmpLength offset)
//------------------------------------------------------------------------------
{
#ifdef MODPLUG_TRACKER
	if(::GetPrivateProfileInt("Misc", "ITCompressionVerification", 0, theApp.GetConfigFileName()) != 0)
	{
		int8 *newSampleData = new (std::nothrow) int8[baseLength * mptSample.GetElementarySampleSize()];
		// Load original sample data for this block again
		if(mptSample.GetElementarySampleSize() > 1)
		{
			CopySample<int16>(sampleData, data, offset, baseLength, mptSample.GetNumChannels());
		} else
		{
			CopySample<int8>(sampleData, data, offset, baseLength, mptSample.GetNumChannels());
		}

		FileReader data(&packedData[0], packedLength + 2);
		ModSample sample = mptSample;
		sample.uFlags.reset(CHN_STEREO);
		sample.pSample = newSampleData;
		sample.nLength = baseLength;
		ITDecompression(data, sample, is215);

		if(memcmp(sampleData, newSampleData, baseLength * mptSample.GetElementarySampleSize()))
		{
			Reporting::Error("CRITICAL ERROR! Sample compression failed for some sample!\nDisable IT compression NOW, find out which sample got broken and send the original sample to the OpenMPT devs!");
		}
		delete[] newSampleData;
	}
#endif // MODPLUG_TRACKER
}


int ITCompression::GetWidthChangeSize(int w, bool is16)
//-----------------------------------------------------
{
	ASSERT(w > 0 && w <= CountOf(ITWidthChangeSize));
	int wcs = ITWidthChangeSize[w - 1];
	if(w <= 6 && is16)
		wcs++;
	return wcs;
}


template<typename Properties>
void ITCompression::SquishRecurse(int sWidth, int lWidth, int rWidth, int width, SmpLength offset, SmpLength length)
//------------------------------------------------------------------------------------------------------------------
{
	if(width + 1 < 1)
	{
		for(SmpLength i = offset; i < offset + length; i++)
			bwt[i] = sWidth;
		return;
	}

	ASSERT(width < CountOf(Properties::lowerTab));

	SmpLength i = offset;
	SmpLength end = offset + length;
	const Properties::sample_t *p = static_cast<Properties::sample_t *>(sampleData);

	while(i < end)
	{
		if(p[i] >= Properties::lowerTab[width] && p[i] <= Properties::upperTab[width])
		{
			SmpLength start = i;
			// Check for how long we can keep this bit width
			while(i < end && p[i] >= Properties::lowerTab[width] && p[i] <= Properties::upperTab[width])
			{
				i++;
			}

			const SmpLength blockLength = i - start;
			const int xlwidth = start == offset ? lWidth : sWidth;
			const int xrwidth = i == end ? rWidth : sWidth;

			const bool is16 = sizeof(Properties::sample_t) > 1;
			const int wcsl = GetWidthChangeSize(xlwidth, is16);
			const int wcss = GetWidthChangeSize(sWidth, is16);
			const int wcsw = GetWidthChangeSize(width + 1, is16);

			bool comparison;
			if(i == baseLength)
			{
				SmpLength keepDown = wcsl + (width + 1) * blockLength;
				SmpLength levelLeft = wcsl + sWidth * blockLength;

				if(xlwidth == sWidth)
					levelLeft -= wcsl;

				comparison = (keepDown <= levelLeft);
			} else
			{
				SmpLength keepDown = wcsl + (width + 1) * blockLength + wcsw;
				SmpLength levelLeft = wcsl + sWidth * blockLength + wcss;

				if(xlwidth == sWidth)
					levelLeft -= wcsl;
				if(xrwidth == sWidth)
					levelLeft -= wcss;

				comparison = (keepDown <= levelLeft);
			}
			SquishRecurse<Properties>(comparison ? (width + 1) : sWidth, xlwidth, xrwidth, width - 1, start, blockLength);
		} else
		{
			bwt[i] = sWidth;
			i++;
		}
	}
}


int ITCompression::ConvertWidth(int curWidth, int newWidth)
//---------------------------------------------------------
{
	curWidth--;
	newWidth--;
	ASSERT(newWidth != curWidth);
	if(newWidth > curWidth)
		newWidth--;
	return newWidth;
}


void ITCompression::WriteBits(int width, int v)
//---------------------------------------------
{
	while(width > remBits)
	{
		byteVal |= (v << bitPos);
		width -= remBits;
		v >>= remBits;
		bitPos = 0;
		remBits = 8;
		WriteByte(byteVal);
		byteVal = 0;
	}

	if(width > 0)
	{
		byteVal |= (v & ((1 << width) - 1)) << bitPos;
		remBits -= width;
		bitPos += width;
	}
}


void ITCompression::WriteByte(uint8 v)
//------------------------------------
{
	if(packedLength < bufferSize)
	{
		packedData[packedLength++] = v;
	} else
	{
		// How could this happen, anyway?
		ASSERT(false);
	}
}


//////////////////////////////////////////////////////////////////////////////
// IT 2.14 decompression


ITDecompression::ITDecompression(FileReader &file, ModSample &sample, bool it215) : mptSample(sample), is215(it215)
//-----------------------------------------------------------------------------------------------------------------
{
	for(uint8 chn = 0; chn < mptSample.GetNumChannels(); chn++)
	{
		writtenSamples = writePos = 0;
		while(writtenSamples < sample.nLength && file.BytesLeft())
		{
			chunk = file.GetChunk(file.ReadUint16LE());

			if(mptSample.GetElementarySampleSize() > 1)
				Uncompress<IT16BitParams>(mptSample.pSample + 2 * chn);
			else
				Uncompress<IT8BitParams>(mptSample.pSample + chn);
		}
	}
}


template<typename Properties>
void ITDecompression::Uncompress(void *target)
//--------------------------------------------
{
	// Initialise bit reader
	dataPos = 0;
	bitPos = 0;
	remBits = 8;
	mem1 = mem2 = 0;

	curLength = Util::Min(mptSample.nLength - writtenSamples, SmpLength(ITCompression::blockSize / sizeof(Properties::sample_t)));

	int width = Properties::defWidth;
	while(curLength > 0)
	{
		if(width < 1 || width > Properties::defWidth || dataPos >= chunk.GetLength())
		{
			// Error!
			return;
		}

		int v = ReadBits(width);
		const int topBit = (1 << (width - 1));
		if(width <= 6)
		{
			// Mode A
			if(v == topBit)
				ChangeWidth(width, ReadBits(Properties::fetchA));
			else
				Write<Properties>(v, topBit, target);
		} else if(width < Properties::defWidth)
		{
			// Mode B
			if(v >= topBit + Properties::lowerB && v <= topBit + Properties::upperB)
				ChangeWidth(width, v - (topBit + Properties::lowerB));
			else
				Write<Properties>(v, topBit, target);
		} else
		{
			// Mode C
			if(v & topBit)
				width = (v & ~topBit) + 1;
			else
				Write<Properties>((v & ~topBit), 0, target);
		}
	}
}


void ITDecompression::ChangeWidth(int &curWidth, int width)
//---------------------------------------------------------
{
	width++;
	if(width >= curWidth)
		width++;

	ASSERT(curWidth != width);
	curWidth = width;
}


int ITDecompression::ReadBits(int width)
//--------------------------------------
{
	const uint8 *data = reinterpret_cast<const uint8 *>(chunk.GetRawData());
	int v = 0, vPos = 0, vMask = (1 << width) - 1;
	while(width >= remBits && dataPos < chunk.GetLength())
	{
		v |= (data[dataPos] >> bitPos) << vPos;
		vPos += remBits;
		width -= remBits;
		dataPos++;
		remBits = 8;
		bitPos = 0;
	}

	if(width > 0 && dataPos < chunk.GetLength())
	{
		v |= (data[dataPos] >> bitPos) << vPos;
		v &= vMask;
		remBits -= width;
		bitPos += width;
	}
	return v;
}


template<typename Properties>
void ITDecompression::Write(int v, int topBit, void *target)
//----------------------------------------------------------
{
	if(v & topBit)
		v -= (topBit << 1);
	mem1 += v;
	mem2 += mem1;
	static_cast<Properties::sample_t *>(target)[writePos] = static_cast<Properties::sample_t>(is215 ? mem2 : mem1);
	writtenSamples++;
	writePos += mptSample.GetNumChannels();
	curLength--;
}
