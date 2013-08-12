/*
 * MemoryMappedFile.h
 * ------------------
 * Purpose: Header file for memory-mapped file wrapper
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../soundlib/FileReader.h"

//////////////////////////////////////////////////////////////////
// File Mapping Class

//===============
class CMappedFile
//===============
{
protected:
	CFile m_File;
	HANDLE m_hFMap;
	void *m_pData;

public:
	CMappedFile() : m_hFMap(nullptr), m_pData(nullptr) { }
	~CMappedFile();

public:
	bool Open(LPCSTR lpszFileName);
	void Close();
	size_t GetLength();
	const void *Lock();
	FileReader GetFile();
	void Unlock();
};
