/*
 * mptFileIO.cpp
 * -------------
 * Purpose: File I/O wrappers
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptFileIO.h"


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_WITH_FILEIO)

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

#endif // MPT_WITH_FILEIO


OPENMPT_NAMESPACE_END
