/*
 * unarchiver.h
 * ------------
 * Purpose: archive loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "../common/FileReader.h"

#include "archive.h"

#if (defined(MPT_WITH_ZLIB) && defined(MPT_WITH_MINIZIP)) || defined(MPT_WITH_MINIZ)
#include "unzip.h"
#endif
#ifdef MPT_WITH_LHASA
#include "unlha.h"
#endif
#if defined(MPT_WITH_ZLIB) || defined(MPT_WITH_MINIZ)
#include "ungzip.h"
#endif
#ifdef MPT_WITH_UNRAR
#include "unrar.h"
#endif
#ifdef MPT_WITH_ANCIENT
#include "unancient.h"
#endif


OPENMPT_NAMESPACE_BEGIN


class CUnarchiver : public IArchive
{

private:

	IArchive *impl;

	FileReader inFile;

	ArchiveBase emptyArchive;
#if (defined(MPT_WITH_ZLIB) && defined(MPT_WITH_MINIZIP)) || defined(MPT_WITH_MINIZ)
	CZipArchive zipArchive;
#endif
#ifdef MPT_WITH_LHASA
	CLhaArchive lhaArchive;
#endif
#if defined(MPT_WITH_ZLIB) || defined(MPT_WITH_MINIZ)
	CGzipArchive gzipArchive;
#endif
#ifdef MPT_WITH_UNRAR
	CRarArchive rarArchive;
#endif
#ifdef MPT_WITH_ANCIENT
	CAncientArchive ancientArchive;
#endif

public:

	CUnarchiver(FileReader &file);
	~CUnarchiver() override;

	bool IsArchive() const override;
	mpt::ustring GetComment() const override;
	bool ExtractFile(std::size_t index) override;
	FileReader GetOutputFile() const override;
	std::size_t size() const override;
	IArchive::const_iterator begin() const override;
	IArchive::const_iterator end() const override;
	const ArchiveFileInfo & operator [] (std::size_t index) const override;

public:

	static const std::size_t failIndex = (std::size_t)-1;

	std::size_t FindBestFile(const std::vector<const char *> &extensions);
	bool ExtractBestFile(const std::vector<const char *> &extensions);

};


OPENMPT_NAMESPACE_END
