/*
 * FileReader.h
 * ------------
 * Purpose: A basic class for transparent reading of memory-based files.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/typedefs.h"
#include "../common/StringFixer.h"
#include "Endianness.h"
#include <vector>
#include <algorithm>
#include <assert.h>


//==============
class FileReader
//==============
{
public:
	typedef size_t off_t;	// Type for file offsets and sizes

private:
	const char *streamData;	// Pointer to memory-mapped file
	off_t streamLength;		// Size of memory-mapped file in bytes
	off_t streamPos;		// Cursor location in the file

public:
	// Initialize invalid file reader object.
	FileReader() : streamData(nullptr), streamLength(0), streamPos(0) { }
	// Initialize file reader object with pointer to data and data length.
	FileReader(const char *data, off_t length) : streamData(data), streamLength(length), streamPos(0) { }
	// Initialize file reader object based on an existing file reader object. The other object's stream position is copied.
	FileReader(const FileReader &other) : streamData(other.streamData), streamLength(other.streamLength), streamPos(other.streamPos) { }

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
	bool Seek(off_t position)
	{
		if(position <= streamLength)
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
	bool Skip(off_t skipBytes)
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

	// Decreases position by skipBytes.
	// Returns true if skipBytes could be skipped or false if the file start was reached earlier.
	bool SkipBack(off_t skipBytes)
	{
		if(streamPos >= skipBytes)
		{
			streamPos -= skipBytes;
			return true;
		} else
		{
			streamPos = 0;
			return false;
		}
	}

	// Returns cursor position in the mapped file.
	off_t GetPosition() const
	{
		return streamPos;
	}

	// Returns size of the mapped file in bytes.
	off_t GetLength() const
	{
		return streamLength;
	}

	// Return byte count between cursor position and end of file, i.e. how many bytes can still be read.
	off_t BytesLeft() const
	{
		assert(streamPos <= streamLength);
		return streamLength - streamPos;
	}

	// Check if "amount" bytes can be read from the current position in the stream.
	bool CanRead(off_t amount) const
	{
		return (amount <= BytesLeft());
	}

	// Create a new FileReader object for parsing a sub chunk at a given position with a given length.
	// The file cursor is not modified.
	FileReader GetChunk(off_t position, off_t length) const
	{
		if(position < streamLength)
		{
			return FileReader(streamData + position, (std::min)(length, streamLength - position));
		} else
		{
			return FileReader();
		}
	}

	// Create a new FileReader object for parsing a sub chunk at the current position with a given length.
	// The file cursor is advanced by "length" bytes.
	FileReader GetChunk(off_t length)
	{
		off_t position = streamPos;
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
			return SwapBytesLE(target);
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
			return SwapBytesBE(target);
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
			uint32 temp = *reinterpret_cast<uint32 *>(&target);
			SwapBytesLE(temp);
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
			uint32 temp = *reinterpret_cast<uint32 *>(&target);
			SwapBytesBE(temp);
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
	bool ReadStructPartial(T &target, off_t partialSize = sizeof(T))
	{
		const off_t copyBytes = Util::Min(partialSize, sizeof(target), BytesLeft());

		memcpy(&target, streamData + streamPos, copyBytes);
		memset(reinterpret_cast<char *>(&target) + copyBytes, 0, sizeof(target) - copyBytes);
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
	template<StringFixer::ReadWriteMode mode, off_t destSize>
	bool ReadString(char (&destBuffer)[destSize], const off_t srcSize)
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
	template<typename T, off_t destSize>
	bool ReadArray(T (&destArray)[destSize])
	{
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

	// Read destSize elements of type T into a vector.
	// If successful, the file cursor is advanced by the size of the vector.
	// Otherwise, the vector is cleared.
	template<typename T>
	bool ReadVector(std::vector<T> &destVector, size_t destSize)
	{
		const off_t readSize = sizeof(T) * destSize;
		if(CanRead(readSize))
		{
			destVector.resize(destSize);
			if(readSize)
			{
				memcpy(&destVector[0], streamData  + streamPos, readSize);
				streamPos += readSize;
			}
			return true;
		} else
		{
			destVector.clear();
			return false;
		}
	}

	// Compare a magic string with the current stream position.
	// Returns true if they are identical and advances the file cursor by the the length of the "magic" string.
	// Returns false if the string could not be found. The file cursor is not advanced in this case.
	bool ReadMagic(const char *const magic)
	{
		const off_t magicLength = strlen(magic);
		if(CanRead(magicLength) && !memcmp(streamData + streamPos, magic, magicLength))
		{
			streamPos += magicLength;
			return true;
		} else
		{
			return false;
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

		if(!BytesLeft())
		{
			target = 0;
			return false;
		}

		off_t writtenBits = 0;
		uint8 b = ReadUint8();
		target = (b & 0x7F);

		// Count actual bits used in most significant byte (i.e. this one)
		for(off_t bit = 0; bit < 7; bit++)
		{
			if((b & (1 << bit)) != 0)
			{
				writtenBits = bit + 1;
			}
		}

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
			target = Util::MaxValueOfType<T>(target);
			return false;
		} else if((b & 0x80) != 0)
		{
			// Reached EOF
			return false;
		}
		return true;
	}

};
