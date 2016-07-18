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

OPENMPT_NAMESPACE_BEGIN

#ifdef MPT_WITH_UNRAR

//===============
class CRarArchive
//===============
	: public ArchiveBase
{

protected:

	mpt::scoped_ptr<OnDiskFileWrapper> diskFile;
	bool captureCurrentFile;

public:
	CRarArchive(FileReader &file);
	virtual ~CRarArchive();
	
	virtual bool ExtractFile(std::size_t index);

public:

	void RARCallbackProcessData(const char * data, std::size_t size);

private:

	void Reset();
	void ResetFile();

};

#endif // MPT_WITH_UNRAR

OPENMPT_NAMESPACE_END
