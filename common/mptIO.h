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
inline bool IsValid(std::ostream & f) { return f; }
inline bool IsValid(std::istream & f) { return f; }
inline bool IsValid(std::iostream & f) { return f; }
inline IO::Offset TellRead(std::istream & f) { return f.tellg(); }
inline IO::Offset TellWrite(std::ostream & f) { return f.tellp(); }
inline bool SeekBegin(std::ostream & f) { return f.seekp(0); }
inline bool SeekBegin(std::istream & f) { return f.seekg(0); }
inline bool SeekBegin(std::iostream & f) { return f.seekg(0) && f.seekp(0); }
inline bool SeekEnd(std::ostream & f) { return f.seekp(0, std::ios::end); }
inline bool SeekEnd(std::istream & f) { return f.seekg(0, std::ios::end); }
inline bool SeekEnd(std::iostream & f) { return f.seekg(0, std::ios::end) && f.seekp(0, std::ios::end); }
inline bool SeekAbsolute(std::ostream & f, IO::Offset pos) { return f.seekp(pos, std::ios::beg); }
inline bool SeekAbsolute(std::istream & f, IO::Offset pos) { return f.seekg(pos, std::ios::beg); }
inline bool SeekAbsolute(std::iostream & f, IO::Offset pos) { return f.seekg(pos, std::ios::beg) && f.seekp(pos, std::ios::beg); }
inline bool SeekRelative(std::ostream & f, IO::Offset off) { return f.seekp(off, std::ios::cur); }
inline bool SeekRelative(std::istream & f, IO::Offset off) { return f.seekg(off, std::ios::cur); }
inline bool SeekRelative(std::iostream & f, IO::Offset off) { return f.seekg(off, std::ios::cur) && f.seekp(off, std::ios::cur); }
inline IO::Offset ReadRaw(std::istream & f, uint8 * data, std::size_t size) { return f.read(reinterpret_cast<char *>(data), size) ? f.gcount() : std::streamsize(0); }
inline IO::Offset ReadRaw(std::istream & f, char * data, std::size_t size) { return f.read(data, size) ? f.gcount() : std::streamsize(0); }
inline IO::Offset ReadRaw(std::istream & f, void * data, std::size_t size) { return f.read(reinterpret_cast<char *>(data), size) ? f.gcount() : std::streamsize(0); }
inline bool WriteRaw(std::ostream & f, const uint8 * data, std::size_t size) { return f.write(reinterpret_cast<const char *>(data), size); }
inline bool WriteRaw(std::ostream & f, const char * data, std::size_t size) { return f.write(data, size); }
inline bool WriteRaw(std::ostream & f, const void * data, std::size_t size) { return f.write(reinterpret_cast<const char *>(data), size); }
inline bool IsEof(std::istream & f) { return f.eof(); }
inline bool Flush(std::ostream & f) { return f.flush(); }

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
inline bool Write(Tfile & f, const Tbinary & v)
{
	return IO::WriteRaw(f, mpt::GetRawBytes(v), sizeof(Tbinary));
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
