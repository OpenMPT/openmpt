/*
 * BitReader.h
 * -----------
 * Purpose: An extended FileReader to read bit-oriented rather than byte-oriented streams.
 * Notes  : The current implementation can only read bit widths up to 24 bits, and it always
 *          reads bits starting from the least significant bit, as this is all that is
 *          required by the class users at the moment.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "../common/FileReader.h"
#include <stdexcept>


OPENMPT_NAMESPACE_BEGIN


//==================================
class BitReader : private FileReader
//==================================
{
protected:
	off_t  m_bufPos = 0, m_bufSize = 0;
	uint32 m_bitBuf = 0; // Current bit buffer
	uint8  m_bitNum = 0; // Currently available number of bits
	std::byte m_buffer[mpt::IO::BUFFERSIZE_TINY];

public:

	class eof : public std::range_error
	{
	public:
		eof() : std::range_error("Truncated bit buffer") { }
	};

	BitReader(mpt::span<const std::byte> bytedata) : FileReader(bytedata) { }
	BitReader(const FileReader &other = FileReader()) : FileReader(other) { }

	off_t GetLength() const
	{
		return FileReader::GetLength();
	}

	off_t GetPosition() const
	{
		return FileReader::GetPosition() - m_bufSize + m_bufPos;
	}

	uint32 ReadBits(uint8 numBits)
	{
		while(m_bitNum < numBits)
		{
			// Fetch more bits
			if(m_bufPos >= m_bufSize)
			{
				m_bufSize = ReadRaw(m_buffer, sizeof(m_buffer));
				m_bufPos = 0;
				if(!m_bufSize)
				{
					throw eof();
				}
			}
			m_bitBuf |= (static_cast<uint32>(m_buffer[m_bufPos++]) << m_bitNum);
			m_bitNum += 8;
		}

#if 0
		static constexpr uint32 bitsToMask[] =
		{
			0,
			0x1,      0x3,      0x7,      0xF,
			0x1F,     0x3F,     0x7F,     0xFF,
			0x1FF,    0x3FF,    0x7FF,    0xFFF,
			0x1FFF,   0x3FFF,   0x7FFF,   0xFFFF,
			0x1FFFF,  0x3FFFF,  0x7FFFF,  0xFFFFF,
			0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
		};
#endif

		uint32 v = m_bitBuf & ((1 << numBits) - 1);
		m_bitBuf >>= numBits;
		m_bitNum -= numBits;
		return v;
	}
};


OPENMPT_NAMESPACE_END
