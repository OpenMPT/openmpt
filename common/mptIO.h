/*
 * mptIO.h
 * -------
 * Purpose: Basic functions for reading/writing binary and endian safe data to/from files/streams.
 * Notes  : This is work-in-progress.
 *          Some useful functions for writing are stll missing.
 *          Reading is missing completely.
 * Authors: Joern Heusipp
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#include "../common/typedefs.h"
#include "../common/Endianness.h"
#include <ios>
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

typedef int64 pos_t;
typedef int64 off_t;

template <typename Tstream> inline bool IsValid(Tstream & f) { return f; }
template <typename Tstream> inline IO::pos_t Tell(Tstream & f) { return f.tellp(); }
template <typename Tstream> inline bool SeekBegin(Tstream & f) { return f.seekp(0); }
template <typename Tstream> inline bool SeekEnd(Tstream & f) { return f.seekp(0, std::ios::end); }
template <typename Tstream> inline bool SeekAbsolute(Tstream & f, IO::pos_t pos) { return f.seekp(pos, std::ios::beg); }
template <typename Tstream> inline bool SeekRelative(Tstream & f, IO::off_t off) { return f.seekp(off, std::ios::cur); }
template <typename Tstream> inline bool WriteRaw(Tstream & f, const uint8 * data, std::size_t size) { return f.write(reinterpret_cast<const char *>(data), size); }
template <typename Tstream> inline bool WriteRaw(Tstream & f, const char * data, std::size_t size) { return f.write(data, size); }
template <typename Tstream> inline bool WriteRaw(Tstream & f, const void * data, std::size_t size) { return f.write(reinterpret_cast<const char *>(data), size); }
template <typename Tstream> inline bool Flush(Tstream & f) { return f.flush(); }

inline bool IsValid(FILE* & f) { return f != NULL; }
inline IO::pos_t Tell(FILE* & f) { return ftell(f); }
inline bool SeekBegin(FILE* & f) { return fseek(f, 0, SEEK_SET) == 0; }
inline bool SeekEnd(FILE* & f) { return fseek(f, 0, SEEK_END) == 0; }
inline bool SeekAbsolute(FILE* & f, IO::pos_t pos) { return fseek(f, pos, SEEK_SET) == 0; }
inline bool SeekRelative(FILE* & f, IO::off_t off) { return fseek(f, off, SEEK_CUR) == 0; }
inline bool WriteRaw(FILE* & f, const uint8 * data, std::size_t size) { return fwrite(data, 1, size, f) == size; }
inline bool WriteRaw(FILE* & f, const char * data, std::size_t size) { return fwrite(data, 1, size, f) == size; }
inline bool WriteRaw(FILE* & f, const void * data, std::size_t size) { return fwrite(data, 1, size, f) == size; }
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
