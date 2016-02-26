/*
 * unarchiver.h
 * ------------
 * Purpose: archive loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "../common/FileReader.h"

#include "archive.h"

#if (defined(MPT_WITH_ZLIB) && defined(MPT_WITH_MINIZIP)) || defined(MPT_WITH_MINIZ)
#include "unzip.h"
#ifdef UNLHA_SUPPORT
#include "unlha.h"
#endif
#ifdef UNGZIP_SUPPORT
#include "ungzip.h"
#endif
#endif
#ifdef UNRAR_SUPPORT
#include "unrar.h"
#endif


OPENMPT_NAMESPACE_BEGIN


//=================================
class CUnarchiver : public IArchive
//=================================
{

private:

	IArchive *impl;

	FileReader inFile;

	ArchiveBase emptyArchive;
#if (defined(MPT_WITH_ZLIB) && defined(MPT_WITH_MINIZIP)) || defined(MPT_WITH_MINIZ)
	CZipArchive zipArchive;
#endif
#ifdef UNLHA_SUPPORT
	CLhaArchive lhaArchive;
#endif
#ifdef UNGZIP_SUPPORT
	CGzipArchive gzipArchive;
#endif
#ifdef UNRAR_SUPPORT
	CRarArchive rarArchive;
#endif

public:

	CUnarchiver(FileReader &file);
	virtual ~CUnarchiver();

	virtual bool IsArchive() const;
	virtual mpt::ustring GetComment() const;
	virtual bool ExtractFile(std::size_t index);
	virtual FileReader GetOutputFile() const;
	virtual std::size_t size() const;
	virtual IArchive::const_iterator begin() const;
	virtual IArchive::const_iterator end() const;
	virtual const ArchiveFileInfo & at(std::size_t index) const;
	virtual const ArchiveFileInfo & operator [] (std::size_t index) const;

public:

	static const std::size_t failIndex = (std::size_t)-1;

	std::size_t FindBestFile(const std::vector<const char *> &extensions);
	bool ExtractBestFile(const std::vector<const char *> &extensions);

};


OPENMPT_NAMESPACE_END
