/*
 * unzip.h
 * -------
 * Purpose: Header file for .zip loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#include "archive.h"

OPENMPT_NAMESPACE_BEGIN

class CZipArchive : public ArchiveBase
{
protected:
	void *zipFile;
public:
	CZipArchive(FileReader &file);
	virtual ~CZipArchive();
public:
	virtual bool ExtractFile(std::size_t index);
};

OPENMPT_NAMESPACE_END
