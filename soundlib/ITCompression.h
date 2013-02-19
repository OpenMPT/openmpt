/*
 * ITCompression.cpp
 * -----------------
 * Purpose: Code for IT sample compression and decompression.
 * Notes  : The original Python compression code was written by GreaseMonkey and has been released into the public domain.
 * Authors: OpenMPT Devs
 *          Ben "GreaseMonkey" Russell
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma  once

#include "Snd_defs.h"
#include <vector>
#include "ModSample.h"
#include "FileReader.h"


//=================
class ITCompression
//=================
{
public:
	ITCompression(const ModSample &sample, bool it215, FILE *f);
	size_t GetCompressedSize() const { return packedTotalLength; }

protected:
	static const size_t bufferSize = 2 + 0xFFFF;	// Our output buffer can't be longer than this.

	std::vector<int> bwt;							// Bit width table
	std::vector<uint8> packedData;					// Compressed data for current sample block
	FILE *file;
	void *sampleData;								// Pre-processed sample data for currently compressed sample block
	const ModSample &mptSample;
	size_t packedLength;							// Size of currently compressed sample block
	size_t packedTotalLength;						// Size of all compressed data so far
	SmpLength baseLength;							// Length of the currently compressed sample block (in samples)

	// Bit writer
	int bitPos;		// Current bit position in this byte
	int remBits;	// Remaining bits in this byte
	uint8 byteVal;	// Current byte value to be written

	bool is215;		// Use IT2.15 compression (double deltas)

	template<typename T>
	static void CopySample(void *target, const void *source, SmpLength offset, SmpLength length, SmpLength skip);

	template<typename T>
	void Deltafy();

	template<typename Properties>
	void Compress(const void *data, SmpLength offset, SmpLength actualLength);
	void Verify(const void *data, void *sampleData, SmpLength offset);

	static int GetWidthChangeSize(int w, bool is16);

	template<typename Properties>
	void SquishRecurse(int sWidth, int lWidth, int rWidth, int width, SmpLength offset, SmpLength length);

	static int ConvertWidth(int curWidth, int newWidth);
	void WriteBits(int width, int v);

	void WriteByte(uint8 v);
};


//===================
class ITDecompression
//===================
{
public:
	ITDecompression(FileReader &file, ModSample &sample, bool it215);

protected:
	FileReader chunk;
	ModSample &mptSample;

	// Bit reader
	SmpLength writtenSamples, writePos, curLength;
	FileReader::off_t dataPos;
	int mem1, mem2;
	int bitPos;
	int remBits;

	bool is215;		// Use IT2.15 compression (double deltas)

	template<typename Properties>
	void Uncompress(void *target);
	static void ChangeWidth(int &curWidth, int width);
	int ReadBits(int width);

	template<typename Properties>
	void Write(int v, int topbit, void *target);
};
