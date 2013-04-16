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

//===============
class CUnarchiver
//===============
{
protected:
	FileReader inFile;

private:
	CZipArchive zipArchive;
	mutable CRarArchive rarArchive;
	mutable CLhaArchive lhaArchive;
	CGzipArchive gzipArchive;

protected:
	FileReader outFile;

public:

	FileReader GetOutputFile() const { return outFile; }
	bool IsArchive() const;
	bool ExtractFile();
	const char *GetComments(bool get);

	CUnarchiver(FileReader &file, const std::vector<const char *> & extensions);
	~CUnarchiver();
};
