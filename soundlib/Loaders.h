/*
 * Loaders.h
 * ---------
 * Purpose: Common functions for module loaders
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "Endianness.h"
#include "../common/StringFixer.h"

// Execute "action" if "request_bytes" bytes cannot be read from stream at position "position"
// DEPRECATED. Use FileReader instead.
#define ASSERT_CAN_READ_PROTOTYPE(position, length, request_bytes, action) \
	if((position) > (length) || (request_bytes) > (length) - (position)) action;

// "Default" macro for checking if x bytes can be read from stream.
// DEPRECATED. Use FileReader instead.
#define ASSERT_CAN_READ(x) ASSERT_CAN_READ_PROTOTYPE(dwMemPos, dwMemLength, x, return false);


//==============
class FileReader
//==============
{
private:
	const char *streamData;		// Pointer to memory-mapped file
	size_t streamLength;		// Size of memory-mapped file in bytes
	size_t streamPos;			// Cursor location in the file
	
public:
	FileReader(const char *data, size_t length) : streamData(data), streamLength(length), streamPos(0) { }

	// Returns true if the object points to a valid stream.
	bool IsValid() const
	{
		return streamData != nullptr;
	}

	// Reset cursor to first byte in file.
	void Rewind()
	{
		streamPos = 0;
	}

	// Seek to a position in the mapped file.
	// Returns false if position is invalid.
	bool Seek(size_t position)
	{
		if(position < streamLength)
		{
			streamPos = position;
			return true;
		} else
		{
			return false;
		}
	}

	// Increases position by skipBytes.
	// Returns true if skipBytes could be skipped or false if the file end was reached earlier.
	bool Skip(size_t skipBytes)
	{
		if(BytesLeft() >= skipBytes)
		{
			streamPos += skipBytes;
			return true;
		} else
		{
			streamPos = streamLength;
			return false;
		}
	}

	// Returns cursor position in the mapped file.
	size_t GetPosition() const
	{
		return streamPos;
	}

	// Returns size of the mapped file in bytes.
	size_t GetLength() const
	{
		return streamLength;
	}

	// Return byte count between cursor position and end of file, i.e. how many bytes can still be read.
	size_t BytesLeft() const
	{
		ASSERT(streamPos <= streamLength);
		return streamLength - streamPos;
	}

	// Check if "amount" bytes can be read from the current position in the stream.
	bool CanRead(size_t amount) const
	{
		return (amount <= BytesLeft());
	}

	// Create a new FileReader object for parsing a sub chunk at a given position with a given length.
	// The file cursor is not modified.
	FileReader GetChunk(size_t position, size_t length) const
	{
		if(position < streamLength)
		{
			return FileReader(streamData + position, min(length, streamLength - position));
		} else
		{
			return FileReader(nullptr, 0);
		}
	}

	// Create a new FileReader object for parsing a sub chunk at the current position with a given length.
	// The file cursor is advanced by "length" bytes.
	FileReader GetChunk(size_t length)
	{
		size_t position = streamPos;
		Skip(length);
		return GetChunk(position, length);
	}

	// Returns raw stream data at cursor position.
	// Should only be used if absolutely necessary, for example for sample reading.
	const char *GetRawData() const
	{
		return streamData + streamPos;
	}

	// Read a "T" object from the stream.
	// If not enough bytes can be read, false is returned.
	// If successful, the file cursor is advanced by the size of "T".
	template <typename T>
	bool Read(T &target)
	{
		if(CanRead(sizeof(T)))
		{
			target = *reinterpret_cast<const T *>(streamData + streamPos);
			streamPos += sizeof(T);
			return true;
		} else
		{
			return false;
		}
	}

protected:
	// Read some kind of integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename T>
	T ReadIntLE()
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Target type is a not an integer");
		T target;
		if(Read(target))
		{
			SwapBytesLE(target);
			return target;
		} else
		{
			return 0;
		}
	}

	// Read some kind of integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename T>
	T ReadIntBE()
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Target type is a not an integer");
		T target;
		if(Read(target))
		{
			SwapBytesBE(target);
			return target;
		} else
		{
			return 0;
		}
	}

public:
	// Read unsigned 32-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	uint32 ReadUint32LE()
	{
		return ReadIntLE<uint32>();
	}

	// Read unsigned 32-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	uint32 ReadUint32BE()
	{
		return ReadIntBE<uint32>();
	}

	// Read signed 32-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	int32 ReadInt32LE()
	{
		return ReadIntLE<int32>();
	}

	// Read signed 32-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	int32 ReadInt32BE()
	{
		return ReadIntBE<int32>();
	}

	// Read unsigned 16-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	uint16 ReadUint16LE()
	{
		return ReadIntLE<uint16>();
	}

	// Read unsigned 16-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	uint16 ReadUint16BE()
	{
		return ReadIntBE<uint16>();
	}

	// Read signed 16-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	int16 ReadInt16LE()
	{
		return ReadIntLE<int16>();
	}

	// Read signed 16-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	int16 ReadInt16BE()
	{
		return ReadIntBE<int16>();
	}

	// Read unsigned 8-Bit integer.
	// If successful, the file cursor is advanced by the size of the integer.
	uint8 ReadUint8()
	{
		uint8 target;
		if(Read(target))
		{
			return target;
		} else
		{
			return 0;
		}
	}

	// Read signed 8-Bit integer. If successful, the file cursor is advanced by the size of the integer.
	int8 ReadInt8()
	{
		int8 target;
		if(Read(target))
		{
			return target;
		} else
		{
			return 0;
		}
	}

	// Read 32-Bit float in little-endian format.
	// If successful, the file cursor is advanced by the size of the float.
	float ReadFloatLE()
	{
		float target;
		if(Read(target))
		{
			uint32 temp = LittleEndian(*reinterpret_cast<uint32 *>(&target));
			return *reinterpret_cast<float *>(&temp);
		} else
		{
			return 0.0f;
		}
	}

	// Read 32-Bit float in big-endian format.
	// If successful, the file cursor is advanced by the size of the float.
	float ReadFloatBE()
	{
		float target;
		if(Read(target))
		{
			uint32 temp = BigEndian(*reinterpret_cast<uint32 *>(&target));
			return *reinterpret_cast<float *>(&temp);
		} else
		{
			return 0.0f;
		}
	}

	// Read a struct.
	// If successful, the file cursor is advanced by the size of the struct. Otherwise, the target is zeroed.
	template <typename T>
	bool ReadStruct(T &target)
	{
		if(Read(target))
		{
			return true;
		} else
		{
			MemsetZero(target);
			return false;
		}
	}

	// Allow to read a struct partially (if there's less memory available than the struct's size, fill it up with zeros).
	// The file cursor is advanced by "partialSize" bytes.
	template <typename T>
	bool ReadStructPartial(T &target, size_t partialSize = sizeof(target))
	{
		const size_t copyBytes = min(min(partialSize, sizeof(target)), BytesLeft());

		memcpy(&target, streamData + streamPos, copyBytes);
		memset(reinterpret_cast<const char *>(&target) + copyBytes, 0, sizeof(target) - copyBytes);
		Skip(partialSize);

		return true;
	}

	// Read a "T" object from the stream.
	// If not enough bytes can be read, false is returned.
	// If successful, the file cursor is advanced by the size of "T" and the object's "ConvertEndianness()" method is called.
	template <typename T>
	bool ReadConvertEndianness(T &target)
	{
		if(Read(target))
		{
			target.ConvertEndianness();
			return true;
		} else
		{
			return false;
		}
	}

	// Read a string of length srcSize into fixed-length char array destBuffer using a given read mode.
	// The file cursor is advanced by "srcSize" bytes.
	template<StringFixer::ReadWriteMode mode, size_t destSize>
	bool ReadString(char (&destBuffer)[destSize], const size_t srcSize)
	{
		if(CanRead(srcSize))
		{
			StringFixer::ReadString<mode, destSize>(destBuffer, streamData + streamPos, srcSize);
			streamPos += srcSize;
			return true;
		} else
		{
			return false;
		}
	}

	// Read an array.
	// If successful, the file cursor is advanced by the size of the array.
	// Otherwise, the target is zeroed.
	template<typename T, size_t destSize>
	bool ReadArray(T (&destArray)[destSize])
	{
		static_assert(sizeof(destArray) == sizeof(T) * destSize, "Huh?");
		if(CanRead(sizeof(destArray)))
		{
			memcpy(destArray, streamData  + streamPos, sizeof(destArray));
			streamPos += sizeof(destArray);
			return true;
		} else
		{
			MemsetZero(destArray);
			return false;
		}
	}

	// Compare a magic string with the current stream position.
	// Returns true if they are identical.
	// The file cursor is advanced by the the length of the "magic" string.
	bool ReadMagic(const char *magic)
	{
		const size_t magicLength = strlen(magic);
		if(!CanRead(magicLength))
		{
			return false;
		} else
		{
			bool result = !memcmp(streamData + streamPos, magic, magicLength);
			streamPos += magicLength;
			return result;
		}
	}

	// Read variable-length integer (as found in MIDI files).
	// If successful, the file cursor is advanced by the size of the integer and true is returned.
	// False is returned if not enough bytes were left to finish reading of the integer or if an overflow happened (source doesn't fit into target integer).
	// In case of an overflow, the target is also set to the maximum value supported by its data type.
	template<typename T>
	bool ReadVarInt(T &target)
	{
		static_assert(std::numeric_limits<T>::is_integer == true
			&& std::numeric_limits<T>::is_signed == false,
			"Target type is a not an unsigned integer");

		target = 0;
		size_t writtenBits = 0;
		uint8 b = 0x80;
		while(BytesLeft() && (b & 0x80) != 0)
		{
			b = ReadUint8();
			target <<= 7;
			target |= (b & 0x7F);
			writtenBits += 7;
		};

		if(writtenBits > sizeof(target) * 8)
		{
			// Overflow
			target = std::numeric_limits<T>::max;
			return false;
		} else if((b & 0x80) != 0)
		{
			// Reached EOF
			return false;
		}
		return true;
	}

};

#include "Sndfile.h"
