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


CMappedFile::~CMappedFile()
//-------------------------
{
	Close();
}


bool CMappedFile::Open(LPCSTR lpszFileName)
//-----------------------------------------
{
	return m_File.Open(lpszFileName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite) != FALSE;
}


void CMappedFile::Close()
//-----------------------
{
	if(m_pData) Unlock();
	m_File.Close();
}


size_t CMappedFile::GetLength()
//-----------------------------
{
	return mpt::saturate_cast<size_t>(m_File.GetLength());
}


const void *CMappedFile::Lock()
//-----------------------------
{
	size_t length = GetLength();
	if(!length) return nullptr;

	void *lpStream;

	HANDLE hmf = CreateFileMapping(
		m_File.m_hFile,
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
			0);
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
	m_File.Read(lpStream, length);
	m_pData = lpStream;
	return lpStream;
}


FileReader CMappedFile::GetFile()
//-------------------------------
{
	return FileReader(Lock(), GetLength());
}


void CMappedFile::Unlock()
//------------------------
{
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
}