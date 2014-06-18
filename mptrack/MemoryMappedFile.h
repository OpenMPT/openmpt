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

OPENMPT_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////
// File Mapping Class

//===============
class CMappedFile
//===============
{
protected:
	HANDLE m_hFile;
	HANDLE m_hFMap;
	void *m_pData;

public:
	CMappedFile() : m_hFile(nullptr), m_hFMap(nullptr), m_pData(nullptr) { }
	~CMappedFile();

public:
	bool Open(const mpt::PathString &filename);
	void Close();
	size_t GetLength();
	const void *Lock();
	FileReader GetFile();
};

OPENMPT_NAMESPACE_END
