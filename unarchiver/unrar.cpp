/*
 * unrar.cpp
 * ---------
 * Purpose: Implementation file for extracting modules from .rar archives
 * Notes  : Based on modified UnRAR library to support in-memory files.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "unrar.h"

#if MPT_COMPILER_MSVC
// Disable "unreferenced formal parameter" and type conversion warnings
#pragma warning(disable:4100; disable:4244)
#endif
#include "../include/unrar/rar.hpp"
#if MPT_COMPILER_MSVC
#pragma warning(default:4100; default:4244)
#endif


struct RARData
{
	CommandData Cmd;
	Archive Arc;
	int64 firstBlock;

	RARData() : Arc(&Cmd) { }
};


static int CALLBACK ProcessRARDataProc(unsigned int msg, LPARAM userData, LPARAM p1, LPARAM p2)
//---------------------------------------------------------------------------------------------
{
	if(msg == UCM_PROCESSDATA)
	{
		// Receive extracted data
		CRarArchive *that = reinterpret_cast<CRarArchive *>(userData);
		that->WriteData(reinterpret_cast<const char *>(p1), p2);
		return 1;
	}
	// No support for passwords or volumes
	return 0;
}


void CRarArchive::WriteData(const char *d, size_t s)
//--------------------------------------------------
{
	data.insert(data.end(), d, d + s);
}


CRarArchive::CRarArchive(FileReader &file) : ArchiveBase(file)
//------------------------------------------------------------
{
	rarData = new (std::nothrow) RARData();
	if(rarData == nullptr)
	{
		return;
	}

	try
	{
		rarData->Cmd.FileArgs.AddString(MASKALL);
		rarData->Cmd.VersionControl = 1;

		rarData->Arc.Open(reinterpret_cast<wchar *>(&inFile), 0);
		if(!rarData->Arc.IsArchive(false))
		{
			return;
		}

		// Read comment
		Array<wchar> rarComment;
		if(rarData->Arc.GetComment(&rarComment))
		{
			comment = mpt::String::Encode(std::wstring(&rarComment[0], rarComment.Size()), mpt::CharsetLocale);
		}

		// Scan all files
		rarData->firstBlock = rarData->Arc.Tell();
		while(rarData->Arc.SearchBlock(HEAD_FILE) > 0)
		{
			ArchiveFileInfo fileInfo;
			fileInfo.name = mpt::String::Encode(rarData->Arc.FileHead.FileName, mpt::CharsetLocale);
			fileInfo.type = ArchiveFileNormal;
			fileInfo.size = rarData->Arc.FileHead.UnpSize;
			contents.push_back(fileInfo);

			rarData->Arc.SeekToNext();
		}
	} catch(RAR_EXIT)
	{
	}
}


CRarArchive::~CRarArchive()
//-------------------------
{
	delete rarData;
}


bool CRarArchive::ExtractFile(std::size_t index)
//----------------------------------------------
{
	if(index >= contents.size())
	{
		return false;
	}

	try
	{
		// Rewind...
		rarData->Arc.Seek(rarData->firstBlock, SEEK_SET);
		CmdExtract Extract(&rarData->Cmd);
		Extract.ExtractArchiveInit(&rarData->Cmd, rarData->Arc);

		// Don't extract, only test (and use the callback function for writing our data)
		rarData->Cmd.Test = true;
		rarData->Cmd.DllOpMode = RAR_TEST;
		rarData->Cmd.Callback = nullptr;
		rarData->Cmd.UserData = reinterpret_cast<LPARAM>(this);

		bool repeat = false;
		size_t headerSize;
		do
		{
			headerSize = rarData->Arc.SearchBlock(HEAD_FILE);
			ASSERT(headerSize);
			if(rarData->Arc.Solid && index)
			{
				// Skip solid files
				Extract.ExtractCurrentFile(&rarData->Cmd, rarData->Arc, headerSize, repeat);
			}
			rarData->Arc.SeekToNext();
		} while(index--);

		// Extract real file
		data.clear();
		data.reserve(static_cast<size_t>(rarData->Arc.FileHead.UnpSize));
		rarData->Cmd.Callback = ProcessRARDataProc;
		Extract.ExtractCurrentFile(&rarData->Cmd, rarData->Arc, headerSize, repeat);
		return !data.empty();
	} catch(...)
	{
		return false;
	}
}
