/*
 * BitReader.h
 * -----------
 * Purpose: An extended FileReader to read bit-oriented rather than byte-oriented streams.
 * Notes  : The current implementation can only read bit widths up to 8 bits, and it always
 *          reads bits starting from the least significant bit, as this is all that is
 *          required by the class users at the moment.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/FileReader.h"
#include <stdexcept>


OPENMPT_NAMESPACE_BEGIN


//==================================
class BitReader : private FileReader
//==================================
{
protected:
	off_t bufPos = 0, bufSize = 0;
	uint32 bitBuf = 0;
	int bitNum = 0;
	mpt::byte buffer[16384];

public:

	class eof : public std::range_error
	{
	public:
		eof() : std::range_error("Truncated bit buffer") { }
	};

	BitReader(mpt::span<const mpt::byte> bytedata) : FileReader(bytedata) { }
	BitReader(const FileReader &other) : FileReader(other) { }

	off_t GetLength() const
	{
		return FileReader::GetLength();
	}

	off_t GetPosition() const
	{
		return FileReader::GetPosition() - bufSize + bufPos;
	}

	uint8 ReadBits(int numBits)
	{
		if(bitNum < numBits)
		{
			// Fetch more bits
			if(bufPos >= bufSize)
			{
				bufSize = ReadRaw(buffer, sizeof(buffer));
				bufPos = 0;
				if(!bufSize)
				{
					throw eof();
				}
			}
			bitBuf |= (static_cast<uint32>(buffer[bufPos++]) << bitNum);
			bitNum += 8;
		}

		uint8 v = static_cast<uint8>(bitBuf & ((1 << numBits) - 1));
		bitBuf >>= numBits;
		bitNum -= numBits;
		return v;
	}
};


OPENMPT_NAMESPACE_END
