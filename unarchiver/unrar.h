/*
 * unrar.h
 * -------
 * Purpose: Header file for .rar loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "archive.h"

struct RARData;

//====================================
class CRarArchive : public ArchiveBase
//====================================
{
protected:
	RARData *rarData;

public:
	CRarArchive(FileReader &file);
	virtual ~CRarArchive();
	
	virtual bool ExtractFile(std::size_t index);

	void WriteData(const char *d, size_t s);
};
