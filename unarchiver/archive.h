/*
 * archive.h
 * ---------
 * Purpose: archive loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#include "../common/FileReader.h"
#include <string>
#include <vector>

OPENMPT_NAMESPACE_BEGIN

enum ArchiveFileType
{
	ArchiveFileInvalid,
	ArchiveFileNormal,
	ArchiveFileSpecial,
};

struct ArchiveFileInfo
{
	mpt::PathString name;
	ArchiveFileType type = ArchiveFileInvalid;
	uint64 size = 0;
	mpt::ustring comment;
	uint64 cookie1 = 0;
	uint64 cookie2 = 0;
};

class IArchive
{
public:
	using const_iterator = std::vector<ArchiveFileInfo>::const_iterator;
protected:
	IArchive() {}
public:
	virtual ~IArchive() {}

public:
	virtual bool IsArchive() const = 0;
	virtual mpt::ustring GetComment() const = 0;
	virtual bool ExtractFile(std::size_t index) = 0;
	virtual FileReader GetOutputFile() const = 0;
	virtual std::size_t size() const = 0;
	virtual IArchive::const_iterator begin() const = 0;
	virtual IArchive::const_iterator end() const = 0;
	virtual const ArchiveFileInfo & at(std::size_t index) const = 0;
	virtual const ArchiveFileInfo & operator [] (std::size_t index) const = 0;
};

class ArchiveBase
 : public IArchive
{
protected:
	FileReader inFile;
	mpt::ustring comment;
	std::vector<ArchiveFileInfo> contents;
	std::vector<char> data;
public:
	ArchiveBase(const FileReader &inFile)
		: inFile(inFile)
	{
		return;
	}
	~ArchiveBase() override
	{
		return;
	}
	bool ExtractFile(std::size_t index) override { MPT_UNREFERENCED_PARAMETER(index); return false; } // overwrite this
public:
	bool IsArchive() const override
	{
		return !contents.empty();
	}
	mpt::ustring GetComment() const override
	{
		return comment;
	}
	FileReader GetOutputFile() const override
	{
		return FileReader(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(data)));
	}
	std::size_t size() const override { return contents.size(); }
	IArchive::const_iterator begin() const override { return contents.begin(); }
	IArchive::const_iterator end() const override { return contents.end(); }
	const ArchiveFileInfo & at(std::size_t index) const override { return contents.at(index); }
	const ArchiveFileInfo & operator [] (std::size_t index) const override { return contents[index]; }
};


OPENMPT_NAMESPACE_END
