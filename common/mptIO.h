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

template <typename Tfile>
struct Traits
{
	// empty
}; // struct Traits<Tfile>

template<>
struct Traits<std::ostream>
{
	typedef std::ostream file_t;
	typedef char byte_t;
	static inline bool IsValid(file_t & f) { return f; }
	static inline IO::pos_t Tell(file_t & f) { return f.tellp(); }
	static inline bool SeekBegin(file_t & f) { return f.seekp(0); }
	static inline bool SeekEnd(file_t & f) { return f.seekp(0, std::ios::end); }
	static inline bool SeekAbsolute(file_t & f, IO::pos_t pos) { return f.seekp(pos, std::ios::beg); }
	static inline bool SeekRelative(file_t & f, IO::off_t off) { return f.seekp(off, std::ios::cur); }
	static inline bool WriteRaw(file_t & f, const byte_t * data, std::size_t size) { return f.write(data, size); }
	static inline bool Flush(file_t & f) { return f.flush(); }
}; // struct Traits<std::ostream&>

template<>
struct Traits<FILE*>
{
	typedef FILE* file_t;
	typedef char byte_t;
	static inline bool IsValid(file_t & f) { return f != NULL; }
	static inline IO::pos_t Tell(file_t & f) { return ftell(f); }
	static inline bool SeekBegin(file_t & f) { return fseek(f, 0, SEEK_SET) == 0; }
	static inline bool SeekEnd(file_t & f) { return fseek(f, 0, SEEK_END) == 0; }
	static inline bool SeekAbsolute(file_t & f, IO::pos_t pos) { return fseek(f, pos, SEEK_SET) == 0; }
	static inline bool SeekRelative(file_t & f, IO::off_t off) { return fseek(f, off, SEEK_CUR) == 0; }
	static inline bool WriteRaw(file_t & f, const byte_t * data, std::size_t size) { return fwrite(data, 1, size, f) == size; }
	static inline bool Flush(file_t & f) { return fflush(f); }
}; // struct Traits<FILE*&>

template<typename Tfile> inline bool IsValid(Tfile & f)                                        { return Traits<Tfile>::IsValid(f); }
template<typename Tfile> inline IO::pos_t Tell(Tfile & f)                                      { return Traits<Tfile>::Tell(f); }
template<typename Tfile> inline bool SeekBegin(Tfile & f)                                      { return Traits<Tfile>::SeekBegin(f); }
template<typename Tfile> inline bool SeekEnd(Tfile & f)                                        { return Traits<Tfile>::SeekEnd(f); }
template<typename Tfile> inline bool SeekAbsolute(Tfile & f, IO::pos_t pos)                    { return Traits<Tfile>::SeekAbsolute(f, pos); }
template<typename Tfile> inline bool SeekRelative(Tfile & f, IO::off_t off)                    { return Traits<Tfile>::SeekRelative(f, off); }
template<typename Tfile> inline bool WriteRaw(Tfile & f, const uint8 * data, std::size_t size) { return Traits<Tfile>::WriteRaw(f, reinterpret_cast<const typename Traits<Tfile>::byte_t *>(data), size); }
template<typename Tfile> inline bool WriteRaw(Tfile & f, const int8 * data, std::size_t size)  { return Traits<Tfile>::WriteRaw(f, reinterpret_cast<const typename Traits<Tfile>::byte_t *>(data), size); }
template<typename Tfile> inline bool WriteRaw(Tfile & f, const char * data, std::size_t size)  { return Traits<Tfile>::WriteRaw(f, reinterpret_cast<const typename Traits<Tfile>::byte_t *>(data), size); }
template<typename Tfile> inline bool WriteRaw(Tfile & f, const void * data, std::size_t size)  { return Traits<Tfile>::WriteRaw(f, reinterpret_cast<const typename Traits<Tfile>::byte_t *>(data), size); }
template<typename Tfile> inline bool Flush(Tfile & f)                                          { return Traits<Tfile>::Flush(f); }

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

} // namespace IO

} // namespace mpt


OPENMPT_NAMESPACE_END
