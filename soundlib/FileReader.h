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
#include "../common/misc_util.h"
#include "Endianness.h"
#include <algorithm>
#ifndef NO_FILEREADER_STD_ISTREAM
#include <ios>
#include <istream>
#endif
#include <limits>
#include <vector>


// change to show warnings for functions which trigger pre-caching the whole file for unseekable streams
//#define FILEREADER_DEPRECATED DEPRECATED
#define FILEREADER_DEPRECATED


#ifndef NO_FILEREADER_STD_ISTREAM

class IFileDataContainer {
public:
	typedef std::size_t off_t;
protected:
	IFileDataContainer() { }
public:
	virtual ~IFileDataContainer() { }
public:
	virtual bool IsValid() const = 0;
	virtual const char *GetRawData() const = 0;
	virtual off_t GetLength() const = 0;
	virtual off_t Read(char *dst, off_t pos, off_t count) const = 0;

	virtual const char *GetPartialRawData(off_t pos, off_t length) const // DO NOT USE!!! this is just for ReadMagic ... the pointer returned may be invalid after the next Read()
	{
		if(pos + length > GetLength())
		{
			return nullptr;
		}
		return GetRawData() + pos;
	}

	virtual bool CanRead(off_t pos, off_t length) const
	{
		return pos + length <= GetLength();
	}

	virtual off_t GetReadableLength(off_t pos, off_t length) const
	{
		if(pos >= GetLength())
		{
			return 0;
		}
		return std::min<off_t>(length, GetLength() - pos);
	}
};


class FileDataContainerDummy : public IFileDataContainer {
public:
	FileDataContainerDummy() { }
	virtual ~FileDataContainerDummy() { }
public:
	bool IsValid() const
	{
		return false;
	}

	const char *GetRawData() const
	{
		return nullptr;
	}

	off_t GetLength() const
	{
		return 0;
	}
	off_t Read(char * /*dst*/, off_t /*pos*/, off_t /*count*/) const
	{
		return 0;
	}
};


class FileDataContainerWindow : public IFileDataContainer
{
private:
	MPT_SHARED_PTR<IFileDataContainer> data;
	off_t dataOffset;
	off_t dataLength;
public:
	FileDataContainerWindow(MPT_SHARED_PTR<IFileDataContainer> src, off_t off, off_t len) : data(src), dataOffset(off), dataLength(len) { }
	virtual ~FileDataContainerWindow() { }

	bool IsValid() const
	{
		return data->IsValid();
	}
	const char *GetRawData() const {
		return data->GetRawData() + dataOffset;
	}
	off_t GetLength() const {
		return dataLength;
	}
	off_t Read(char *dst, off_t pos, off_t count) const
	{
		return data->Read(dst, dataOffset + pos, count);
	}
	const char *GetPartialRawData(off_t pos, off_t length) const
	{
		if(pos + length > dataLength)
		{
			return nullptr;
		}
		return data->GetPartialRawData(dataOffset + pos, length);
	}
	bool CanRead(off_t pos, off_t length) const {
		return pos + length <= dataLength;
	}
	off_t GetReadableLength(off_t pos, off_t length) const
	{
		if(pos >= dataLength)
		{
			return 0;
		}
		return std::min<off_t>(length, dataLength - pos);
	}
};


class FileDataContainerStdStream : public IFileDataContainer {
private:
	mutable std::vector<char> cache;
	mutable bool streamFullyCached;

	std::istream *stream;

public:
	FileDataContainerStdStream(std::istream *s) : streamFullyCached(false), stream(s) { }
	virtual ~FileDataContainerStdStream() { }

private:
	static const std::size_t buffer_size = 65536;
	void CacheStream() const
	{
		if(streamFullyCached)
		{
			return;
		}
		while(*stream)
		{
			cache.resize(cache.size() + buffer_size);
			stream->read(&cache[cache.size() - buffer_size], buffer_size);
			std::size_t readcount = static_cast<std::size_t>(stream->gcount());
			cache.resize(cache.size() - buffer_size + readcount);
		}
		streamFullyCached = true;
	}
	void CacheStreamUpTo(std::streampos pos) const
	{
		if(streamFullyCached)
		{
			return;
		}
		if(pos <= std::streampos(cache.size()))
		{
			return;
		}
		std::size_t needcount = static_cast<std::size_t>(pos - std::streampos(cache.size()));
		cache.resize(static_cast<std::size_t>(pos));
		stream->read(&cache[cache.size() - needcount], needcount);
		std::size_t readcount = static_cast<std::size_t>(stream->gcount());
		cache.resize(cache.size() - buffer_size + readcount);
		if(*stream)
		{
			// can read further
			return;
		}
		streamFullyCached = true;
	}
private:
	void ReadCached(char *dst, off_t pos, off_t count) const
	{
		std::copy(cache.begin() + pos, cache.begin() + pos + count, dst);
	}

public:

	bool IsValid() const
	{
		return true;
	}

	const char *GetRawData() const
	{
		CacheStream();
		return &cache[0];
	}

	off_t GetLength() const
	{
		if(streamFullyCached)
		{
			return cache.size();
		} else
		{
			stream->clear();
			std::streampos oldpos = stream->tellg();
			if(!stream->fail() && oldpos != std::streampos(-1))
			{
				stream->seekg(0, std::ios::end);
				if(!stream->fail())
				{
					std::streampos length = stream->tellg();
					if(!stream->fail() && length != std::streampos(-1))
					{
						stream->seekg(oldpos);
						stream->clear();
						return static_cast<off_t>(length);
					}
				}
				stream->clear();
				stream->seekg(oldpos);
			}
			// not seekable
			stream->clear();
			CacheStream();
			return cache.size();
		}
	}

	off_t Read(char *dst, off_t pos, off_t count) const
	{
		CacheStreamUpTo(pos + count);
		if(pos >= off_t(cache.size()))
		{
			return 0;
		}
		off_t cache_avail = std::min<off_t>(off_t(cache.size()) - pos, count);
		ReadCached(dst, pos, cache_avail);
		return cache_avail;
	}

	const char *GetPartialRawData(off_t pos, off_t length) const
	{
		CacheStreamUpTo(pos + length);
		if(pos + length > off_t(cache.size()))
		{
			return nullptr;
		}
		return &cache[pos];
	}

	bool CanRead(off_t pos, off_t length) const
	{
		CacheStreamUpTo(pos + length);
		return pos + length <= off_t(cache.size());
	}

	off_t GetReadableLength(off_t pos, off_t length) const
	{
		CacheStreamUpTo(pos + length);
		return std::min<off_t>(cache.size() - pos, length);
	}

};

#endif 


class FileDataContainerMemory
#ifndef NO_FILEREADER_STD_ISTREAM
	: public IFileDataContainer
#endif
{

#ifdef NO_FILEREADER_STD_ISTREAM
public:
	typedef std::size_t off_t;
#endif

private:

	const char *streamData;	// Pointer to memory-mapped file
	off_t streamLength;		// Size of memory-mapped file in bytes

public:
	FileDataContainerMemory() : streamData(nullptr), streamLength(0) { }
	FileDataContainerMemory(const char *data, off_t length) : streamData(data), streamLength(length) { }
#ifndef NO_FILEREADER_STD_ISTREAM
	virtual
#endif
		~FileDataContainerMemory() { }

public:

	bool IsValid() const
	{
		return streamData != nullptr;
	}

	const char *GetRawData() const
	{
		return streamData;
	}

	off_t GetLength() const
	{
		return streamLength;
	}

	off_t Read(char *dst, off_t pos, off_t count) const
	{
		if(pos >= streamLength)
		{
			return 0;
		}
		off_t avail = std::min<off_t>(streamLength - pos, count);
		std::copy(streamData + pos, streamData + pos + avail, dst);
		return avail;
	}

	const char *GetPartialRawData(off_t pos, off_t length) const
	{
		if(pos + length > streamLength)
		{
			return nullptr;
		}
		return streamData + pos;
	}

	bool CanRead(off_t pos, off_t length) const
	{
		return pos + length <= streamLength;
	}

	off_t GetReadableLength(off_t pos, off_t length) const
	{
		if(pos >= streamLength)
		{
			return 0;
		}
		return std::min<off_t>(length, streamLength - pos);
	}

};


//==============
class FileReader
//==============
{

public:

#ifndef NO_FILEREADER_STD_ISTREAM
	typedef IFileDataContainer::off_t off_t;
#else
	typedef FileDataContainerMemory::off_t off_t;
#endif

private:

#ifndef NO_FILEREADER_STD_ISTREAM
	const IFileDataContainer & DataContainer() const { return *data; }
	IFileDataContainer & DataContainer() { return *data; }
	MPT_SHARED_PTR<IFileDataContainer> data;
#else
	const FileDataContainerMemory & DataContainer() const { return data; }
	FileDataContainerMemory & DataContainer() { return data; }
	FileDataContainerMemory data;
#endif

	off_t streamPos;		// Cursor location in the file

public:

#ifndef NO_FILEREADER_STD_ISTREAM

	// Initialize invalid file reader object.
	FileReader() : data(new FileDataContainerDummy()), streamPos(0) { }

	// Initialize file reader object with pointer to data and data length.
	FileReader(const void *voiddata, off_t length) : data(new FileDataContainerMemory(static_cast<const char *>(voiddata), length)), streamPos(0) { }
	FileReader(const char *chardata, off_t length) : data(new FileDataContainerMemory(chardata, length)), streamPos(0) { }
	FileReader(const uint8 *uint8data, off_t length) : data(new FileDataContainerMemory(reinterpret_cast<const char *>(uint8data), length)), streamPos(0) { }

	// Initialize file reader object with a std::istream.
	FileReader(std::istream *s) : data(new FileDataContainerStdStream(s)), streamPos(0) { }

	// Initialize file reader object based on an existing file reader object window.
	FileReader(MPT_SHARED_PTR<IFileDataContainer> other) : data(other), streamPos(0) { }

	// Initialize file reader object based on an existing file reader object. The other object's stream position is copied.
	FileReader(const FileReader &other) : data(other.data), streamPos(other.streamPos) { }

#else

	// Initialize invalid file reader object.
	FileReader() : data(nullptr, 0), streamPos(0) { }

	// Initialize file reader object with pointer to data and data length.
	FileReader(const void *voiddata, off_t length) : data(static_cast<const char *>(voiddata), length), streamPos(0) { }
	FileReader(const char *chardata, off_t length) : data(chardata, length), streamPos(0) { }
	FileReader(const uint8 *uint8data, off_t length) : data(reinterpret_cast<const char *>(uint8data), length), streamPos(0) { }

	// Initialize file reader object based on an existing file reader object. The other object's stream position is copied.
	FileReader(const FileReader &other) : data(other.data), streamPos(other.streamPos) { }

#endif

	// Returns true if the object points to a valid stream.
	bool IsValid() const
	{
		return DataContainer().IsValid();
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
		if(position <= streamPos)
		{
			streamPos = position;
			return true;
		}
		if(position <= DataContainer().GetLength())
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
		if(CanRead(skipBytes))
		{
			streamPos += skipBytes;
			return true;
		} else
		{
			streamPos = DataContainer().GetLength();
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
	FILEREADER_DEPRECATED off_t GetLength() const
	{
		// deprecated because in case of an unseekable std::istream, this triggers caching of the whole file
		return DataContainer().GetLength();
	}

	// Return byte count between cursor position and end of file, i.e. how many bytes can still be read.
	FILEREADER_DEPRECATED off_t BytesLeft() const
	{
		// deprecated because in case of an unseekable std::istream, this triggers caching of the whole file
		return DataContainer().GetLength() - streamPos;
	}

	bool AreBytesLeft() const
	{
		return CanRead(1);
	}

	bool NoBytesLeft() const
	{
		return !CanRead(1);
	}

	// Check if "amount" bytes can be read from the current position in the stream.
	bool CanRead(off_t amount) const
	{
		return DataContainer().CanRead(streamPos, amount);
	}

	// Create a new FileReader object for parsing a sub chunk at a given position with a given length.
	// The file cursor is not modified.
	FileReader GetChunk(off_t position, off_t length) const
	{
		off_t readableLength = DataContainer().GetReadableLength(position, length);
		if(readableLength == 0)
		{
			return FileReader();
		}
		#ifndef NO_FILEREADER_STD_ISTREAM
			return FileReader(MPT_SHARED_PTR<IFileDataContainer>(new FileDataContainerWindow(data, position, (std::min)(length, DataContainer().GetLength() - position))));
		#else
			return FileReader(DataContainer().GetRawData() + position, (std::min)(length, DataContainer().GetLength() - position));
		#endif
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
	FILEREADER_DEPRECATED const char *GetRawData() const
	{
		// deprecated because in case of an unseekable std::istream, this triggers caching of the whole file
		return DataContainer().GetRawData() + streamPos;
	}

	// Read a "T" object from the stream.
	// If not enough bytes can be read, false is returned.
	// If successful, the file cursor is advanced by the size of "T".
	template <typename T>
	bool Read(T &target)
	{
		if(sizeof(T) != DataContainer().Read(reinterpret_cast<char*>(&target), streamPos, sizeof(T)))
		{
			return false;
		}
		streamPos += sizeof(T);
		return true;
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
		FloatInt32 target;
		if(Read(target))
		{
			SwapBytesLE(target.i);
			return target.f;
		} else
		{
			return 0.0f;
		}
	}

	// Read 32-Bit float in big-endian format.
	// If successful, the file cursor is advanced by the size of the float.
	float ReadFloatBE()
	{
		FloatInt32 target;
		if(Read(target))
		{
			SwapBytesBE(target.i);
			return target.f;
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
		const off_t copyBytes = DataContainer().Read(reinterpret_cast<char *>(&target), streamPos, partialSize);
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
			StringFixer::ReadString<mode, destSize>(destBuffer, DataContainer().GetPartialRawData(streamPos, srcSize), srcSize);
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
			for(std::size_t i = 0; i < destSize; ++i)
			{
				Read(destArray[i]);
			}
			return true;
		} else
		{
			MemsetZero(destArray);
			return false;
		}
	}

	// Read destSize elements of type T into a vector.
	// If successful, the file cursor is advanced by the size of the vector.
	// Otherwise, the vector is resized to destSize, but possibly existing contents are not cleared.
	template<typename T>
	bool ReadVector(std::vector<T> &destVector, size_t destSize)
	{
		const off_t readSize = sizeof(T) * destSize;
		destVector.resize(destSize);
		if(CanRead(readSize))
		{
			for(std::size_t i = 0; i < destSize; ++i)
			{
				Read(destVector[i]);
			}
			return true;
		} else
		{
			return false;
		}
	}

	// Compare a magic string with the current stream position.
	// Returns true if they are identical and advances the file cursor by the the length of the "magic" string.
	// Returns false if the string could not be found. The file cursor is not advanced in this case.
	bool ReadMagic(const char *const magic)
	{
		const off_t magicLength = strlen(magic);
		if(CanRead(magicLength))
		{
			if(!memcmp(DataContainer().GetPartialRawData(streamPos, magicLength), magic, magicLength))
			{
				streamPos += magicLength;
				return true;
			} else
			{
				return false;
			}
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

		if(NoBytesLeft())
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

		while(AreBytesLeft() && (b & 0x80) != 0)
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
