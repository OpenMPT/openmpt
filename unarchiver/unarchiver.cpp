/*
 * unarchiver.cpp
 * --------------
 * Purpose: archive loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "unarchiver.h"
#include "../common/FileReader.h"


OPENMPT_NAMESPACE_BEGIN


CUnarchiver::CUnarchiver(FileReader &file)
//----------------------------------------
	: impl(nullptr)
	, inFile(file)
	, emptyArchive(inFile)
#if (defined(MPT_WITH_ZLIB) && defined(MPT_WITH_MINIZIP)) || defined(MPT_WITH_MINIZ)
	, zipArchive(inFile)
#endif
#ifdef UNLHA_SUPPORT
	, lhaArchive(inFile)
#endif
#ifdef UNGZIP_SUPPORT
	, gzipArchive(inFile)
#endif
#ifdef UNRAR_SUPPORT
	, rarArchive(inFile)
#endif
{
	inFile.Rewind();
#if (defined(MPT_WITH_ZLIB) && defined(MPT_WITH_MINIZIP)) || defined(MPT_WITH_MINIZ)
	if(zipArchive.IsArchive()) { impl = &zipArchive; return; }
#endif
#ifdef UNLHA_SUPPORT
	if(lhaArchive.IsArchive()) { impl = &lhaArchive; return; }
#endif
#ifdef UNGZIP_SUPPORT
	if(gzipArchive.IsArchive()) { impl = &gzipArchive; return; }
#endif
#ifdef UNRAR_SUPPORT
	if(rarArchive.IsArchive()) { impl = &rarArchive; return; }
#endif
	impl = &emptyArchive;
}


CUnarchiver::~CUnarchiver()
//-------------------------
{
	return;
}


struct find_str
{
	find_str(const char *str): s1(str) { }
	bool operator() (const char *s2) const
	{
		return !strcmp(s1, s2);
	}
	const char *s1;
};


static inline std::string GetExtension(const std::string &filename)
//-----------------------------------------------------------------
{
	if(filename.find_last_of(".") != std::string::npos)
	{
		std::string ext = filename.substr(filename.find_last_of(".") + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
		return ext;
	}
	return std::string();
}


std::size_t CUnarchiver::FindBestFile(const std::vector<const char *> &extensions)
//--------------------------------------------------------------------------------
{
	if(!IsArchive())
	{
		return failIndex;
	}
	uint64 biggestSize = 0;
	std::size_t bestIndex = failIndex;
	for(std::size_t i = 0; i < size(); ++i)
	{
		if(at(i).type != ArchiveFileNormal)
		{
			continue;
		}
		const std::string ext = GetExtension(at(i).name.ToUTF8());

		// Compare with list of preferred extensions
		if(std::find_if(extensions.begin(), extensions.end(), find_str(ext.c_str())) != extensions.end())
		{
			bestIndex = i;
			break;
		}

		if(ext == "diz" || ext == "nfo" || ext == "txt")
		{
			// we do not want these
			continue;
		}

		if(at(i).size >= biggestSize)
		{
			biggestSize = at(i).size;
			bestIndex = i;
		}
	}
	return bestIndex;
}


bool CUnarchiver::ExtractBestFile(const std::vector<const char *> &extensions)
//----------------------------------------------------------------------------
{
	std::size_t bestFile = FindBestFile(extensions);
	if(bestFile == failIndex)
	{
		return false;
	}
	return ExtractFile(bestFile);
}


bool CUnarchiver::IsArchive() const
//---------------------------------
{
	return impl->IsArchive();
}


mpt::ustring CUnarchiver::GetComment() const
//------------------------------------------
{
	return impl->GetComment();
}


bool CUnarchiver::ExtractFile(std::size_t index)
//----------------------------------------------
{
	return impl->ExtractFile(index);
}


FileReader CUnarchiver::GetOutputFile() const
//-------------------------------------------
{
	return impl->GetOutputFile();
}


std::size_t CUnarchiver::size() const
//-----------------------------------
{
	return impl->size();
}


IArchive::const_iterator CUnarchiver::begin() const
//-------------------------------------------------
{
	return impl->begin();
}


IArchive::const_iterator CUnarchiver::end() const
//-----------------------------------------------
{
	return impl->end();
}


const ArchiveFileInfo & CUnarchiver::at(std::size_t index) const
//--------------------------------------------------------------
{
	return impl->at(index);
}


const ArchiveFileInfo & CUnarchiver::operator [] (std::size_t index) const
//------------------------------------------------------------------------
{
	return impl->operator[](index);
}


OPENMPT_NAMESPACE_END
