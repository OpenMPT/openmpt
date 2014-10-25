/*
 * mptIO.cpp
 * ---------
 * Purpose: Basic functions for reading/writing binary and endian safe data to/from files/streams.
 * Notes  : This is work-in-progress.
 *          Some useful functions for reading and writing are still missing.
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "mptIO.h"

#include <ios>
#include <istream>
#include <ostream>

#include <cstdio>

#include <stdio.h>


OPENMPT_NAMESPACE_BEGIN


namespace mpt {

namespace IO {



//STATIC_ASSERT(sizeof(std::streamoff) == 8); // Assert 64bit file support.
bool IsValid(std::ostream & f) { return !f.fail(); }
bool IsValid(std::istream & f) { return !f.fail(); }
bool IsValid(std::iostream & f) { return !f.fail(); }
IO::Offset TellRead(std::istream & f) { return f.tellg(); }
IO::Offset TellWrite(std::ostream & f) { return f.tellp(); }
bool SeekBegin(std::ostream & f) { f.seekp(0); return !f.fail(); }
bool SeekBegin(std::istream & f) { f.seekg(0); return !f.fail(); }
bool SeekBegin(std::iostream & f) { f.seekg(0); f.seekp(0); return !f.fail(); }
bool SeekEnd(std::ostream & f) { f.seekp(0, std::ios::end); return !f.fail(); }
bool SeekEnd(std::istream & f) { f.seekg(0, std::ios::end); return !f.fail(); }
bool SeekEnd(std::iostream & f) { f.seekg(0, std::ios::end); f.seekp(0, std::ios::end); return !f.fail(); }
bool SeekAbsolute(std::ostream & f, IO::Offset pos) { if(!OffsetFits<std::streamoff>(pos)) { return false; } f.seekp(static_cast<std::streamoff>(pos), std::ios::beg); return !f.fail(); }
bool SeekAbsolute(std::istream & f, IO::Offset pos) { if(!OffsetFits<std::streamoff>(pos)) { return false; } f.seekg(static_cast<std::streamoff>(pos), std::ios::beg); return !f.fail(); }
bool SeekAbsolute(std::iostream & f, IO::Offset pos) { if(!OffsetFits<std::streamoff>(pos)) { return false; } f.seekg(static_cast<std::streamoff>(pos), std::ios::beg); f.seekp(static_cast<std::streamoff>(pos), std::ios::beg); return !f.fail(); }
bool SeekRelative(std::ostream & f, IO::Offset off) { if(!OffsetFits<std::streamoff>(off)) { return false; } f.seekp(static_cast<std::streamoff>(off), std::ios::cur); return !f.fail(); }
bool SeekRelative(std::istream & f, IO::Offset off) { if(!OffsetFits<std::streamoff>(off)) { return false; } f.seekg(static_cast<std::streamoff>(off), std::ios::cur); return !f.fail(); }
bool SeekRelative(std::iostream & f, IO::Offset off) { if(!OffsetFits<std::streamoff>(off)) { return false; } f.seekg(static_cast<std::streamoff>(off), std::ios::cur); f.seekp(static_cast<std::streamoff>(off), std::ios::cur); return !f.fail(); }
IO::Offset ReadRaw(std::istream & f, uint8 * data, std::size_t size) { return f.read(reinterpret_cast<char *>(data), size) ? f.gcount() : std::streamsize(0); }
IO::Offset ReadRaw(std::istream & f, char * data, std::size_t size) { return f.read(data, size) ? f.gcount() : std::streamsize(0); }
IO::Offset ReadRaw(std::istream & f, void * data, std::size_t size) { return f.read(reinterpret_cast<char *>(data), size) ? f.gcount() : std::streamsize(0); }
bool WriteRaw(std::ostream & f, const uint8 * data, std::size_t size) { f.write(reinterpret_cast<const char *>(data), size); return !f.fail(); }
bool WriteRaw(std::ostream & f, const char * data, std::size_t size) { f.write(data, size); return !f.fail(); }
bool WriteRaw(std::ostream & f, const void * data, std::size_t size) { f.write(reinterpret_cast<const char *>(data), size); return !f.fail(); }
bool IsEof(std::istream & f) { return f.eof(); }
bool Flush(std::ostream & f) { f.flush(); return !f.fail(); }



#if defined(MPT_WITH_PATHSTRING)

bool IsValid(FILE* & f) { return f != NULL; }

#if MPT_COMPILER_MSVC

IO::Offset TellRead(FILE* & f) { return _ftelli64(f); }
IO::Offset TellWrite(FILE* & f) { return _ftelli64(f); }
bool SeekBegin(FILE* & f) { return _fseeki64(f, 0, SEEK_SET) == 0; }
bool SeekEnd(FILE* & f) { return _fseeki64(f, 0, SEEK_END) == 0; }
bool SeekAbsolute(FILE* & f, IO::Offset pos) { return _fseeki64(f, pos, SEEK_SET) == 0; }
bool SeekRelative(FILE* & f, IO::Offset off) { return _fseeki64(f, off, SEEK_CUR) == 0; }

#elif defined(_POSIX_SOURCE) && (_POSIX_SOURCE > 0) 

//STATIC_ASSERT(sizeof(off_t) == 8);
IO::Offset TellRead(FILE* & f) { return ftello(f); }
IO::Offset TellWrite(FILE* & f) { return ftello(f); }
bool SeekBegin(FILE* & f) { return fseeko(f, 0, SEEK_SET) == 0; }
bool SeekEnd(FILE* & f) { return fseeko(f, 0, SEEK_END) == 0; }
bool SeekAbsolute(FILE* & f, IO::Offset pos) { return OffsetFits<off_t>(pos) && (fseek(f, mpt::saturate_cast<off_t>(pos), SEEK_SET) == 0); }
bool SeekRelative(FILE* & f, IO::Offset off) { return OffsetFits<off_t>(off) && (fseek(f, mpt::saturate_cast<off_t>(off), SEEK_CUR) == 0); }

#else

//STATIC_ASSERT(sizeof(long) == 8); // Fails on 32bit non-POSIX systems for now.
IO::Offset TellRead(FILE* & f) { return ftell(f); }
IO::Offset TellWrite(FILE* & f) { return ftell(f); }
bool SeekBegin(FILE* & f) { return fseek(f, 0, SEEK_SET) == 0; }
bool SeekEnd(FILE* & f) { return fseek(f, 0, SEEK_END) == 0; }
bool SeekAbsolute(FILE* & f, IO::Offset pos) { return OffsetFits<long>(pos) && (fseek(f, mpt::saturate_cast<long>(pos), SEEK_SET) == 0); }
bool SeekRelative(FILE* & f, IO::Offset off) { return OffsetFits<long>(off) && (fseek(f, mpt::saturate_cast<long>(off), SEEK_CUR) == 0); }

#endif

IO::Offset ReadRaw(FILE * & f, uint8 * data, std::size_t size) { return fread(data, 1, size, f); }
IO::Offset ReadRaw(FILE * & f, char * data, std::size_t size) { return fread(data, 1, size, f); }
IO::Offset ReadRaw(FILE * & f, void * data, std::size_t size) { return fread(data, 1, size, f); }
bool WriteRaw(FILE* & f, const uint8 * data, std::size_t size) { return fwrite(data, 1, size, f) == size; }
bool WriteRaw(FILE* & f, const char * data, std::size_t size) { return fwrite(data, 1, size, f) == size; }
bool WriteRaw(FILE* & f, const void * data, std::size_t size) { return fwrite(data, 1, size, f) == size; }
bool IsEof(FILE * & f) { return feof(f) != 0; }
bool Flush(FILE* & f) { return fflush(f) == 0; }

#endif // MPT_WITH_PATHSTRING



} // namespace IO

} // namespace mpt



#if defined(MPT_FILEREADER_STD_ISTREAM)

FileDataContainerStdStream::FileDataContainerStdStream(std::istream *s)
	: streamFullyCached(false), stream(s)
{
	return;
}

FileDataContainerStdStream::~FileDataContainerStdStream()
{
	return;
}

void FileDataContainerStdStream::CacheStream() const
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

void FileDataContainerStdStream::CacheStreamUpTo(std::streampos pos) const
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
	cache.resize(cache.size() - needcount + readcount);
	if(*stream)
	{
		// can read further
		return;
	}
	streamFullyCached = true;
}

void FileDataContainerStdStream::ReadCached(char *dst, IFileDataContainer::off_t pos, IFileDataContainer::off_t count) const
{
	std::copy(cache.begin() + pos, cache.begin() + pos + count, dst);
}

bool FileDataContainerStdStream::IsValid() const
{
	return true;
}

const char *FileDataContainerStdStream::GetRawData() const
{
	CacheStream();
	return &cache[0];
}

IFileDataContainer::off_t FileDataContainerStdStream::GetLength() const
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
					return static_cast<IFileDataContainer::off_t>(length);
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

IFileDataContainer::off_t FileDataContainerStdStream::Read(char *dst, IFileDataContainer::off_t pos, IFileDataContainer::off_t count) const
{
	CacheStreamUpTo(pos + count);
	if(pos >= IFileDataContainer::off_t(cache.size()))
	{
		return 0;
	}
	IFileDataContainer::off_t cache_avail = std::min<IFileDataContainer::off_t>(IFileDataContainer::off_t(cache.size()) - pos, count);
	ReadCached(dst, pos, cache_avail);
	return cache_avail;
}

const char *FileDataContainerStdStream::GetPartialRawData(IFileDataContainer::off_t pos, IFileDataContainer::off_t length) const
{
	CacheStreamUpTo(pos + length);
	if(pos + length > IFileDataContainer::off_t(cache.size()))
	{
		return nullptr;
	}
	return &cache[pos];
}

bool FileDataContainerStdStream::CanRead(IFileDataContainer::off_t pos, IFileDataContainer::off_t length) const
{
	CacheStreamUpTo(pos + length);
	return pos + length <= IFileDataContainer::off_t(cache.size());
}

IFileDataContainer::off_t FileDataContainerStdStream::GetReadableLength(IFileDataContainer::off_t pos, IFileDataContainer::off_t length) const
{
	CacheStreamUpTo(pos + length);
	return std::min<IFileDataContainer::off_t>(cache.size() - pos, length);
}

#endif



OPENMPT_NAMESPACE_END



// following ode should be moved to common/mptFileIO.cpp and common/mptFstream.h should be renamed to common/mptFileIO.h.

#include "mptFstream.h"


OPENMPT_NAMESPACE_BEGIN


#ifdef MODPLUG_TRACKER

CMappedFile::~CMappedFile()
//-------------------------
{
	Close();
}


bool CMappedFile::Open(const mpt::PathString &filename)
//-----------------------------------------------------
{
	m_hFile = CreateFileW(
		filename.AsNative().c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if(m_hFile == INVALID_HANDLE_VALUE)
	{
		m_hFile = nullptr;
		return false;
	}
	m_FileName = filename;
	return true;
}


void CMappedFile::Close()
//-----------------------
{
	m_FileName = mpt::PathString();
	// Unlock file
	if(m_hFMap)
	{
		if(m_pData)
		{
			UnmapViewOfFile(m_pData);
			m_pData = nullptr;
		}
		CloseHandle(m_hFMap);
		m_hFMap = nullptr;
	} else if(m_pData)
	{
		free(m_pData);
		m_pData = nullptr;
	}

	// Close file handle
	if(m_hFile)
	{
		CloseHandle(m_hFile);
		m_hFile = nullptr;
	}
}


size_t CMappedFile::GetLength()
//-----------------------------
{
	LARGE_INTEGER size;
	if(GetFileSizeEx(m_hFile, &size) == FALSE)
	{
		return 0;
	}
	return mpt::saturate_cast<size_t>(size.QuadPart);
}


const void *CMappedFile::Lock()
//-----------------------------
{
	size_t length = GetLength();
	if(!length) return nullptr;

	void *lpStream;

	HANDLE hmf = CreateFileMapping(
		m_hFile,
		NULL,
		PAGE_READONLY,
		0, 0,
		NULL);

	// Try memory-mapping first
	if(hmf)
	{
		lpStream = MapViewOfFile(
			hmf,
			FILE_MAP_READ,
			0, 0,
			length);
		if(lpStream)
		{
			m_hFMap = hmf;
			m_pData = lpStream;
			return lpStream;
		}
		CloseHandle(hmf);
		hmf = nullptr;
	}
	
	// Fallback if memory-mapping fails for some weird reason
	if((lpStream = malloc(length)) == nullptr) return nullptr;
	memset(lpStream, 0, length);
	size_t bytesToRead = length;
	size_t bytesRead = 0;
	while(bytesToRead > 0)
	{
		DWORD chunkToRead = mpt::saturate_cast<DWORD>(length);
		DWORD chunkRead = 0;
		if(ReadFile(m_hFile, (char*)lpStream + bytesRead, chunkToRead, &chunkRead, NULL) == FALSE)
		{
			// error
			free(lpStream);
			return nullptr;
		}
		bytesRead += chunkRead;
		bytesToRead -= chunkRead;
	}
	m_pData = lpStream;
	return lpStream;
}


#endif // MODPLUG_TRACKER



InputFile::InputFile()
{
	return;
}

InputFile::InputFile(const mpt::PathString &filename)
	: m_Filename(filename)
{
	#if defined(MPT_FILEREADER_STD_ISTREAM)
		m_File.open(m_Filename, std::ios::binary | std::ios::in);
	#else
		m_File.Open(m_Filename);
	#endif
}

InputFile::~InputFile()
{
	return;
}


bool InputFile::Open(const mpt::PathString &filename)
{
	m_Filename = filename;
	#if defined(MPT_FILEREADER_STD_ISTREAM)
		m_File.open(m_Filename, std::ios::binary | std::ios::in);
		return m_File.good();
	#else
		return m_File.Open(m_Filename);
	#endif
}


bool InputFile::IsValid() const
{
	#if defined(MPT_FILEREADER_STD_ISTREAM)
		return m_File.good();
	#else
		return m_File.IsOpen();
	#endif
}

#if defined(MPT_FILEREADER_STD_ISTREAM)

InputFile::ContentsRef InputFile::Get()
{
	InputFile::ContentsRef result;
	result.first = &m_File;
	result.second = m_File.good() ? &m_Filename : nullptr;
	return result;
}

#else

InputFile::ContentsRef InputFile::Get()
{
	InputFile::ContentsRef result;
	result.first.data = nullptr;
	result.first.size = 0;
	result.second = nullptr;
	if(!m_File.IsOpen())
	{
		return result;
	}
	result.first.data = reinterpret_cast<const char*>(m_File.Lock());
	result.first.size = m_File.GetLength();
	result.second = &m_Filename;
	return result;
}

#endif



OPENMPT_NAMESPACE_END
