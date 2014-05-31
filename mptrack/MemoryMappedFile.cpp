/*
 * MemoryMappedFile.cpp
 * --------------------
 * Purpose: Wrapper class for memory-mapped files
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "MemoryMappedFile.h"


OPENMPT_NAMESPACE_BEGIN


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
	return true;
}


void CMappedFile::Close()
//-----------------------
{
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


FileReader CMappedFile::GetFile()
//-------------------------------
{
	return FileReader(Lock(), GetLength());
}


OPENMPT_NAMESPACE_END
