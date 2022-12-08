/*
 * unrar.h
 * -------
 * Purpose: Header file for .rar loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#ifdef MPT_WITH_UNRAR

#include "mpt/io_file_adapter/fileadapter.hpp"
#include "../common/FileReader.h"

#endif // MPT_WITH_UNRAR

#include "archive.h"

OPENMPT_NAMESPACE_BEGIN

#ifdef MPT_WITH_UNRAR

class CRarArchive
	: public ArchiveBase
{

protected:

	std::unique_ptr<mpt::IO::FileAdapter<FileCursor>> diskFile;
	bool captureCurrentFile = false;

public:
	CRarArchive(FileReader &file);
	~CRarArchive() override;
	
	bool ExtractFile(std::size_t index) override;

public:

	void RARCallbackProcessData(const char * data, std::size_t size);

private:

	void Reset();
	void ResetFile();

};

#endif // MPT_WITH_UNRAR

OPENMPT_NAMESPACE_END
