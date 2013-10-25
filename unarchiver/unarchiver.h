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
#endif
#ifdef UNRAR_SUPPORT
#include "unrar.h"
#endif
#ifdef UNLHA_SUPPORT
#include "unlha.h"
#endif
#ifdef UNGZIP_SUPPORT
#include "ungzip.h"
#endif

#ifdef UNRAR_SUPPORT
class CWrappedRarArchive : public IArchive
{
private:
	FileReader inFile;
	std::vector<ArchiveFileInfo> contents;
	mutable CRarArchive rar;
public:
	CWrappedRarArchive(FileReader &inFile)
		: inFile(inFile)
		, rar((LPBYTE)inFile.GetRawData(), inFile.GetLength())
	{
		if(rar.IsArchive())
		{
			ArchiveFileInfo info;
			info.type = ArchiveFileNormal;
			contents.push_back(info);
		}
	}
	virtual ~CWrappedRarArchive() { return; }
public:
	virtual bool IsArchive() const { return rar.IsArchive() == TRUE; }
	virtual std::string GetComment() const { return std::string(); }
	virtual bool ExtractFile(std::size_t index) { if(index >= contents.size()) { return false; } return rar.ExtrFile() == TRUE; }
	virtual FileReader GetOutputFile() const { return FileReader(rar.GetOutputFile(), rar.GetOutputFileLength()); }
	virtual std::size_t size() const { return contents.size(); }
	virtual IArchive::const_iterator begin() const { return contents.begin(); }
	virtual IArchive::const_iterator end() const { return contents.end(); }
	virtual const ArchiveFileInfo & at(std::size_t index) const { return contents.at(index); }
	virtual const ArchiveFileInfo & operator [] (std::size_t index) const { return contents[index]; }
};
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
#ifdef UNRAR_SUPPORT
	CWrappedRarArchive rarArchive;
#endif
#ifdef UNLHA_SUPPORT
	CLhaArchive lhaArchive;
#endif
#ifdef UNGZIP_SUPPORT
	CGzipArchive gzipArchive;
#endif

public:

	CUnarchiver(FileReader &file);
	virtual ~CUnarchiver();

	virtual bool IsArchive() const;
	virtual std::string GetComment() const;
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
