/*
 * unancient.cpp
 * -------------
 * Purpose: Implementation file for extracting modules from compressed files supported by libancient
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "unancient.h"

#ifdef MPT_WITH_ANCIENT
#include <ancient/ancient.hpp>
#endif // MPT_WITH_ANCIENT


OPENMPT_NAMESPACE_BEGIN


#ifdef MPT_WITH_ANCIENT


CAncientArchive::CAncientArchive(FileReader &file)
	: ArchiveBase(file)
{
	inFile.Rewind();
	try
	{
		auto dataView = inFile.GetPinnedView();
		if(!ancient::Decompressor::detect(mpt::byte_cast<const std::uint8_t*>(dataView.data()), dataView.size()))
		{
			return;
		}
		ancient::Decompressor decompressor{mpt::byte_cast<const std::uint8_t*>(dataView.data()), dataView.size(), true, true};
		if(decompressor.getImageSize() || decompressor.getImageOffset())
		{
			// skip disk images
			return;
		}
		ArchiveFileInfo fileInfo;
		fileInfo.name = inFile.GetOptionalFileName().value_or(P_(""));
		fileInfo.type = ArchiveFileType::Normal;
		fileInfo.size = decompressor.getRawSize().value_or(0);
		contents.push_back(fileInfo);
	} catch(const ancient::Error &)
	{
		return;
	}
}


CAncientArchive::~CAncientArchive()
{
	return;
}


bool CAncientArchive::ExtractFile(std::size_t index)
{
	if(index >= contents.size())
	{
		return false;
	}
	data.clear();
	inFile.Rewind();
	try
	{
		auto dataView = inFile.GetPinnedView();
		ancient::Decompressor decompressor{mpt::byte_cast<const std::uint8_t*>(dataView.data()), dataView.size(), true, true};
		data = mpt::buffer_cast<std::vector<char>>(decompressor.decompress(true));
	} catch (const ancient::Error &)
	{
		return false;
	}
	return (data.size() > 0);
}


#endif // MPT_WITH_ANCIENT


OPENMPT_NAMESPACE_END
