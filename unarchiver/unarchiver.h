/*
 * unarchiver.h
 * ------------
 * Purpose: archive loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "../soundlib/FileReader.h"

#include "archive.h"

#define UNGZIP_SUPPORT
#define UNLHA_SUPPORT
#define UNRAR_SUPPORT
#define ZIPPED_MOD_SUPPORT

#ifdef ZIPPED_MOD_SUPPORT
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


//=================================
class CUnarchiver : public IArchive
//=================================
{

private:

	IArchive *impl;

	FileReader inFile;

	ArchiveBase emptyArchive;
#ifdef ZIPPED_MOD_SUPPORT
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
	virtual std::wstring GetComment() const;
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
