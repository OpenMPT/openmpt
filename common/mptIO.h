/*
 * mptIO.h
 * -------
 * Purpose: Basic functions for reading/writing binary and endian safe data to/from files/streams.
 * Notes  : This is work-in-progress.
 *          Some useful functions for reading and writing are still missing.
 * Authors: Joern Heusipp
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#include "../common/typedefs.h"
#include "../common/Endianness.h"
#include <ios>
#include <iostream>
#include <istream>
#include <limits>
#include <ostream>
#if defined(HAS_TYPE_TRAITS)
#include <type_traits>
#endif
#include <cstdio>
#include <cstring>
#include <stdio.h>


OPENMPT_NAMESPACE_BEGIN


namespace mpt {

namespace IO {

typedef int64 Offset;

STATIC_ASSERT(sizeof(std::streamoff) == 8); // Assert 64bit file support.
inline bool IsValid(std::ostream & f) { f; return !f.fail(); }
inline bool IsValid(std::istream & f) { f; return !f.fail(); }
inline bool IsValid(std::iostream & f) { f; return !f.fail(); }
inline IO::Offset TellRead(std::istream & f) { return f.tellg(); }
inline IO::Offset TellWrite(std::ostream & f) { return f.tellp(); }
inline bool SeekBegin(std::ostream & f) { f.seekp(0); return !f.fail(); }
inline bool SeekBegin(std::istream & f) { f.seekg(0); return !f.fail(); }
inline bool SeekBegin(std::iostream & f) { f.seekg(0); f.seekp(0); return !f.fail(); }
inline bool SeekEnd(std::ostream & f) { f.seekp(0, std::ios::end); return !f.fail(); }
inline bool SeekEnd(std::istream & f) { f.seekg(0, std::ios::end); return !f.fail(); }
inline bool SeekEnd(std::iostream & f) { f.seekg(0, std::ios::end); f.seekp(0, std::ios::end); return !f.fail(); }
inline bool SeekAbsolute(std::ostream & f, IO::Offset pos) { f.seekp(pos, std::ios::beg); return !f.fail(); }
inline bool SeekAbsolute(std::istream & f, IO::Offset pos) { f.seekg(pos, std::ios::beg); return !f.fail(); }
inline bool SeekAbsolute(std::iostream & f, IO::Offset pos) { f.seekg(pos, std::ios::beg); f.seekp(pos, std::ios::beg); return !f.fail(); }
inline bool SeekRelative(std::ostream & f, IO::Offset off) { f.seekp(off, std::ios::cur); return !f.fail(); }
inline bool SeekRelative(std::istream & f, IO::Offset off) { f.seekg(off, std::ios::cur); return !f.fail(); }
inline bool SeekRelative(std::iostream & f, IO::Offset off) { f.seekg(off, std::ios::cur); f.seekp(off, std::ios::cur); return !f.fail(); }
inline IO::Offset ReadRaw(std::istream & f, uint8 * data, std::size_t size) { return f.read(reinterpret_cast<char *>(data), size) ? f.gcount() : std::streamsize(0); }
inline IO::Offset ReadRaw(std::istream & f, char * data, std::size_t size) { return f.read(data, size) ? f.gcount() : std::streamsize(0); }
inline IO::Offset ReadRaw(std::istream & f, void * data, std::size_t size) { return f.read(reinterpret_cast<char *>(data), size) ? f.gcount() : std::streamsize(0); }
inline bool WriteRaw(std::ostream & f, const uint8 * data, std::size_t size) { f.write(reinterpret_cast<const char *>(data), size); return !f.fail(); }
inline bool WriteRaw(std::ostream & f, const char * data, std::size_t size) { f.write(data, size); return !f.fail(); }
inline bool WriteRaw(std::ostream & f, const void * data, std::size_t size) { f.write(reinterpret_cast<const char *>(data), size); return !f.fail(); }
inline bool IsEof(std::istream & f) { return f.eof(); }
inline bool Flush(std::ostream & f) { f.flush(); return !f.fail(); }

inline bool IsValid(FILE* & f) { return f != NULL; }
#if MPT_COMPILER_MSVC
inline IO::Offset TellRead(FILE* & f) { return _ftelli64(f); }
inline IO::Offset TellWrite(FILE* & f) { return _ftelli64(f); }
inline bool SeekBegin(FILE* & f) { return _fseeki64(f, 0, SEEK_SET) == 0; }
inline bool SeekEnd(FILE* & f) { return _fseeki64(f, 0, SEEK_END) == 0; }
inline bool SeekAbsolute(FILE* & f, IO::Offset pos) { return _fseeki64(f, pos, SEEK_SET) == 0; }
inline bool SeekRelative(FILE* & f, IO::Offset off) { return _fseeki64(f, off, SEEK_CUR) == 0; }
#elif defined(_POSIX_SOURCE) && (_POSIX_SOURCE == 1) 
STATIC_ASSERT(sizeof(off_t) == 8);
inline IO::Offset TellRead(FILE* & f) { return ftello(f); }
inline IO::Offset TellWrite(FILE* & f) { return ftello(f); }
inline bool SeekBegin(FILE* & f) { return fseeko(f, 0, SEEK_SET) == 0; }
inline bool SeekEnd(FILE* & f) { return fseeko(f, 0, SEEK_END) == 0; }
inline bool SeekAbsolute(FILE* & f, IO::Offset pos) { return fseeko(f, pos, SEEK_SET) == 0; }
inline bool SeekRelative(FILE* & f, IO::Offset off) { return fseeko(f, off, SEEK_CUR) == 0; }
#else
STATIC_ASSERT(sizeof(long) == 8); // Fails on 32bit non-POSIX systems for now.
inline IO::Offset TellRead(FILE* & f) { return ftell(f); }
inline IO::Offset TellWrite(FILE* & f) { return ftell(f); }
inline bool SeekBegin(FILE* & f) { return fseek(f, 0, SEEK_SET) == 0; }
inline bool SeekEnd(FILE* & f) { return fseek(f, 0, SEEK_END) == 0; }
inline bool SeekAbsolute(FILE* & f, IO::Offset pos) { return fseek(f, pos, SEEK_SET) == 0; }
inline bool SeekRelative(FILE* & f, IO::Offset off) { return fseek(f, off, SEEK_CUR) == 0; }
#endif
inline IO::Offset ReadRaw(FILE * & f, uint8 * data, std::size_t size) { return fread(data, 1, size, f); }
inline IO::Offset ReadRaw(FILE * & f, char * data, std::size_t size) { return fread(data, 1, size, f); }
inline IO::Offset ReadRaw(FILE * & f, void * data, std::size_t size) { return fread(data, 1, size, f); }
inline bool WriteRaw(FILE* & f, const uint8 * data, std::size_t size) { return fwrite(data, 1, size, f) == size; }
inline bool WriteRaw(FILE* & f, const char * data, std::size_t size) { return fwrite(data, 1, size, f) == size; }
inline bool WriteRaw(FILE* & f, const void * data, std::size_t size) { return fwrite(data, 1, size, f) == size; }
inline bool IsEof(FILE * & f) { return feof(f) != 0; }
inline bool Flush(FILE* & f) { return fflush(f) == 0; }

template <typename Tbinary, typename Tfile>
inline bool Read(Tfile & f, Tbinary & v)
{
	return IO::ReadRaw(f, mpt::GetRawBytes(v), sizeof(Tbinary)) == sizeof(Tbinary);
}

template <typename Tbinary, typename Tfile>
inline bool Write(Tfile & f, const Tbinary & v)
{
	return IO::WriteRaw(f, mpt::GetRawBytes(v), sizeof(Tbinary));
}

template <typename T, typename Tfile>
inline bool ReadBinaryLE(Tfile & f, T & v)
{
	bool result = false;
	#ifdef HAS_TYPE_TRAITS
		static_assert(std::is_trivial<T>::value == true, "");
	#endif
	uint8 bytes[sizeof(T)];
	std::memset(bytes, 0, sizeof(T));
	result = IO::ReadRaw(f, bytes, sizeof(T)) == sizeof(T);
	#ifdef MPT_PLATFORM_BIG_ENDIAN
		std::reverse(bytes, bytes + sizeof(T));
	#endif
	std::memcpy(&v, bytes, sizeof(T));
	return result;
}

template <typename T, typename Tfile>
inline bool ReadBinaryTruncatedLE(Tfile & f, T & v, std::size_t size)
{
	bool result = false;
	#ifdef HAS_TYPE_TRAITS
		static_assert(std::is_trivial<T>::value == true, "");
	#endif
	uint8 bytes[sizeof(T)];
	std::memset(bytes, 0, sizeof(T));
	result = IO::ReadRaw(f, bytes, std::min(size, sizeof(T))) == std::min(size, sizeof(T));
	#ifdef MPT_PLATFORM_BIG_ENDIAN
		std::reverse(bytes, bytes + sizeof(T));
	#endif
	std::memcpy(&v, bytes, sizeof(T));
	return result;
}

template <typename T, typename Tfile>
inline bool ReadIntLE(Tfile & f, T & v)
{
	bool result = false;
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	uint8 bytes[sizeof(T)];
	std::memset(bytes, 0, sizeof(T));
	result = (IO::ReadRaw(f, bytes, sizeof(T)) == sizeof(T));
	T val = 0;
	std::memcpy(&val, bytes, sizeof(T));
	v = SwapBytesReturnLE(val);
	return result;
}

template <typename T, typename Tfile>
inline bool ReadIntBE(Tfile & f, T & v)
{
	bool result = false;
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	uint8 bytes[sizeof(T)];
	std::memset(bytes, 0, sizeof(T));
	result = (IO::ReadRaw(f, bytes, sizeof(T)) == sizeof(T));
	T val = 0;
	std::memcpy(&val, bytes, sizeof(T));
	v = SwapBytesReturnBE(val);
	return result;
}

template <typename Tfile>
inline bool ReadAdaptiveInt16LE(Tfile & f, uint16 & v)
{
	bool result = true;
	uint8 byte = 0;
	std::size_t additionalBytes = 0;
	v = 0;
	byte = 0;
	if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
	additionalBytes = (byte & 0x01);
	v = byte >> 1;
	for(std::size_t i = 0; i < additionalBytes; ++i)
	{
		byte = 0;
		if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
		v |= (static_cast<uint16>(byte) << (((i+1)*8) - 1));		
	}
	return result;
}

template <typename Tfile>
inline bool ReadAdaptiveInt32LE(Tfile & f, uint32 & v)
{
	bool result = true;
	uint8 byte = 0;
	std::size_t additionalBytes = 0;
	v = 0;
	byte = 0;
	if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
	additionalBytes = (byte & 0x03);
	v = byte >> 2;
	for(std::size_t i = 0; i < additionalBytes; ++i)
	{
		byte = 0;
		if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
		v |= (static_cast<uint32>(byte) << (((i+1)*8) - 2));		
	}
	return result;
}

template <typename Tfile>
inline bool ReadAdaptiveInt64LE(Tfile & f, uint64 & v)
{
	bool result = true;
	uint8 byte = 0;
	std::size_t additionalBytes = 0;
	v = 0;
	byte = 0;
	if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
	additionalBytes = (1 << (byte & 0x03)) - 1;
	v = byte >> 2;
	for(std::size_t i = 0; i < additionalBytes; ++i)
	{
		byte = 0;
		if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
		v |= (static_cast<uint64>(byte) << (((i+1)*8) - 2));		
	}
	return result;
}

template <typename T, typename Tfile>
inline bool WriteBinaryLE(Tfile & f, const T & v)
{
	bool result = false;
	#ifdef HAS_TYPE_TRAITS
		static_assert(std::is_trivial<T>::value == true, "");
	#endif
	uint8 bytes[sizeof(T)];
	std::memcpy(bytes, &v, sizeof(T));
	#ifdef MPT_PLATFORM_BIG_ENDIAN
		std::reverse(bytes, bytes + sizeof(T));
	#endif
	result = IO::WriteRaw(f, bytes, sizeof(T));
	return result;	
}

template <typename T, typename Tfile>
inline bool WriteBinaryTruncatedLE(Tfile & f, const T & v, std::size_t size)
{
	bool result = false;
	#ifdef HAS_TYPE_TRAITS
		static_assert(std::is_trivial<T>::value == true, "");
	#endif
	uint8 bytes[sizeof(T)];
	std::memcpy(bytes, &v, sizeof(T));
	#ifdef MPT_PLATFORM_BIG_ENDIAN
		std::reverse(bytes, bytes + sizeof(T));
	#endif
	result = IO::WriteRaw(f, bytes, std::min(size, sizeof(T)));
	return result;	
}

template <typename T, typename Tfile>
inline bool WriteIntLE(Tfile & f, const T & v)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	const T val = SwapBytesReturnLE(v);
	uint8 bytes[sizeof(T)];
	std::memcpy(bytes, &val, sizeof(T));
	return IO::WriteRaw(f, bytes, sizeof(T));
}

template <typename T, typename Tfile>
inline bool WriteIntBE(Tfile & f, const T & v)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	const T val = SwapBytesReturnBE(v);
	uint8 bytes[sizeof(T)];
	std::memcpy(bytes, &val, sizeof(T));
	return IO::WriteRaw(f, bytes, sizeof(T));
}

template <typename Tfile>
inline bool WriteAdaptiveInt16LE(Tfile & f, const uint16 & v)
{
	if(v < 0x80)
	{
		return IO::WriteIntLE<uint8>(f, static_cast<uint8>(v << 1) | 0x00);
	} else if(v < 0x8000)
	{
		return IO::WriteIntLE<uint16>(f, static_cast<uint16>(v << 1) | 0x01);
	} else
	{
		ASSERT(false);
		return false;
	}
}

template <typename Tfile>
inline bool WriteAdaptiveInt32LE(Tfile & f, const uint32 & v)
{
	if(v < 0x40)
	{
		return IO::WriteIntLE<uint8>(f, static_cast<uint8>(v << 2) | 0x00);
	} else if(v < 0x4000)
	{
		return IO::WriteIntLE<uint16>(f, static_cast<uint16>(v << 2) | 0x01);
	} else if(v < 0x400000)
	{
		return IO::WriteBinaryTruncatedLE<uint32>(f, static_cast<uint32>(v << 2) | 0x02, 3);
	} else if(v < 0x40000000)
	{
		return IO::WriteIntLE<uint32>(f, static_cast<uint32>(v << 2) | 0x03);
	} else
	{
		ASSERT(false);
		return false;
	}
}

template <typename Tfile>
inline bool WriteAdaptiveInt64LE(Tfile & f, const uint64 & v)
{
	if(v < 0x40)
	{
		return IO::WriteIntLE<uint8>(f, static_cast<uint8>(v << 2) | 0x00);
	} else if(v < 0x4000)
	{
		return IO::WriteIntLE<uint16>(f, static_cast<uint16>(v << 2) | 0x01);
	} else if(v < 0x40000000)
	{
		return IO::WriteIntLE<uint32>(f, static_cast<uint32>(v << 2) | 0x02);
	} else if(v < 0x4000000000000000ull)
	{
		return IO::WriteIntLE<uint64>(f, static_cast<uint64>(v << 2) | 0x03);
	} else
	{
		ASSERT(false);
		return false;
	}
}

template <typename T, typename Tfile>
inline bool WriteConvertEndianness(Tfile & f, T & v)
{
	v.ConvertEndianness();
	bool result = IO::WriteRaw(f, reinterpret_cast<const uint8 *>(&v), sizeof(T));
	v.ConvertEndianness();
	return result;
}

} // namespace IO

} // namespace mpt


OPENMPT_NAMESPACE_END
