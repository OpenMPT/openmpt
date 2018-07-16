/*
 * mptFileIO.h
 * -----------
 * Purpose: A wrapper around std::fstream, enforcing usage of mpt::PathString.
 * Notes  : You should only ever use these wrappers instead of plain std::fstream classes.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#if defined(MPT_ENABLE_FILEIO)

#include "../common/mptString.h"
#include "../common/mptPathString.h"
#include "../common/mptIO.h"

#include <fstream>
#include <ios>
#include <ostream>
#include <streambuf>
#include <utility>

#endif // MPT_ENABLE_FILEIO


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_FILEIO)


// Sets the NTFS compression attribute on the file or directory.
// Requires read and write permissions for already opened files.
// Returns true if the attribute has been set.
// In almost all cases, the return value should be ignored because most filesystems other than NTFS do not support compression.
#ifdef MODPLUG_TRACKER
#if MPT_OS_WINDOWS
bool SetFilesystemCompression(HANDLE hFile);
bool SetFilesystemCompression(int fd);
bool SetFilesystemCompression(const mpt::PathString &filename);
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER


namespace mpt
{

#if MPT_COMPILER_GCC && MPT_OS_WINDOWS
// GCC C++ library has no wchar_t overloads
#define MPT_FSTREAM_DO_CONVERSIONS
#define MPT_FSTREAM_DO_CONVERSIONS_ANSI
#endif

#ifdef MPT_FSTREAM_DO_CONVERSIONS
#define MPT_FSTREAM_OPEN(filename, mode) detail::fstream_open<Tbase>(*this, (filename), (mode))
#else
#define MPT_FSTREAM_OPEN(filename, mode) Tbase::open((filename), (mode))
#endif

namespace detail
{

template<typename Tbase>
inline void fstream_open(Tbase & base, const mpt::PathString & filename, std::ios_base::openmode mode)
{
#if defined(MPT_FSTREAM_DO_CONVERSIONS_ANSI)
	base.open(mpt::ToCharset(mpt::CharsetLocale, filename.AsNative()).c_str(), mode);
#else
	base.open(filename.AsNativePrefixed().c_str(), mode);
#endif
}

#ifdef MPT_FSTREAM_DO_CONVERSIONS

template<typename Tbase>
inline void fstream_open(Tbase & base, const std::wstring & filename, std::ios_base::openmode mode)
{
	detail::fstream_open<Tbase>(base, mpt::PathString::FromWide(filename), mode);
}

template<typename Tbase>
inline void fstream_open(Tbase & base, const wchar_t * filename, std::ios_base::openmode mode)
{
	detail::fstream_open<Tbase>(base, mpt::PathString::FromWide(filename ? std::wstring(filename) : std::wstring()), mode);
}

template<typename Tbase>
inline void fstream_open(Tbase & base, const std::string & filename, std::ios_base::openmode mode)
{
	detail::fstream_open<Tbase>(base, mpt::PathString::FromWide(mpt::ToWide(mpt::CharsetLocale, filename)), mode);
}

template<typename Tbase>
inline void fstream_open(Tbase & base, const char * filename, std::ios_base::openmode mode)
{
	detail::fstream_open<Tbase>(base, mpt::PathString::FromWide(mpt::ToWide(mpt::CharsetLocale, filename ? std::string(filename) : std::string())), mode);
}

#endif

} // namespace detail

class fstream
	: public std::fstream
{
private:
	typedef std::fstream Tbase;
public:
	fstream() {}
	fstream(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	void open(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const char * filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::string & filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#if MPT_OS_WINDOWS
	MPT_DEPRECATED_PATH void open(const wchar_t * filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::wstring & filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#endif
};

class ifstream
	: public std::ifstream
{
private:
	typedef std::ifstream Tbase;
public:
	ifstream() {}
	ifstream(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	void open(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const char * filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::string & filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#if MPT_OS_WINDOWS
	MPT_DEPRECATED_PATH void open(const wchar_t * filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::wstring & filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#endif
};

class ofstream
	: public std::ofstream
{
private:
	typedef std::ofstream Tbase;
public:
	ofstream() {}
	ofstream(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	void open(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const char * filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::string & filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#if MPT_OS_WINDOWS
	MPT_DEPRECATED_PATH void open(const wchar_t * filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::wstring & filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#endif
};

#undef MPT_FSTREAM_OPEN



// LazyFileRef is a simple reference to an on-disk file by the means of a
// filename which allows easy assignment of the whole file contents to and from
// byte buffers.
class LazyFileRef {
private:
	const mpt::PathString m_Filename;
public:
	LazyFileRef(const mpt::PathString &filename)
		: m_Filename(filename)
	{
		return;
	}
public:
	LazyFileRef & operator = (const std::vector<mpt::byte> &data);
	LazyFileRef & operator = (const std::vector<char> &data);
	LazyFileRef & operator = (const std::string &data);
	operator std::vector<mpt::byte> () const;
	operator std::vector<char> () const;
	operator std::string () const;
};


} // namespace mpt



#ifdef MODPLUG_TRACKER
#if MPT_OS_WINDOWS
class CMappedFile
{
protected:
	HANDLE m_hFile;
	HANDLE m_hFMap;
	void *m_pData;
	mpt::PathString m_FileName;

public:
	CMappedFile() : m_hFile(nullptr), m_hFMap(nullptr), m_pData(nullptr) { }
	~CMappedFile();

public:
	bool Open(const mpt::PathString &filename);
	bool IsOpen() const { return m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE; }
	const mpt::PathString * GetpFilename() const { return &m_FileName; }
	void Close();
	size_t GetLength();
	const mpt::byte *Lock();
};
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER


class InputFile
{
private:
	mpt::PathString m_Filename;
	#ifdef MPT_FILEREADER_STD_ISTREAM
		mpt::ifstream m_File;
	#else
		CMappedFile m_File;
	#endif
public:
	InputFile();
	InputFile(const mpt::PathString &filename);
	~InputFile();
	bool Open(const mpt::PathString &filename);
	bool IsValid() const;
#if defined(MPT_FILEREADER_STD_ISTREAM)
	typedef std::pair<std::istream*, const mpt::PathString*> ContentsRef;
#else
	struct Data
	{
		const mpt::byte *data;
		std::size_t size;
	};
	typedef std::pair<InputFile::Data, const mpt::PathString*> ContentsRef;
#endif
	InputFile::ContentsRef Get();
};


#endif // MPT_ENABLE_FILEIO


OPENMPT_NAMESPACE_END

