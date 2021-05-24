/*
 * unzip.h
 * -------
 * Purpose: Header file for .zip loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "archive.h"

OPENMPT_NAMESPACE_BEGIN

class CZipArchive : public ArchiveBase
{
protected:
	void *zipFile;
public:
	CZipArchive(FileReader &file);
	~CZipArchive() override;
public:
	bool ExtractFile(std::size_t index) override;
};

OPENMPT_NAMESPACE_END
