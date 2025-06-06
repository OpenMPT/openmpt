/*
 * unrar.cpp
 * ---------
 * Purpose: Implementation file for extracting modules from .rar archives
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "unrar.h"

#ifdef MPT_WITH_UNRAR

#include "mpt/string_transcode/transcode.hpp"
#include "mpt/uuid/uuid.hpp"
#include "../common/mptRandom.h"
#include "../common/mptFileIO.h"
#include "../common/mptFileTemporary.h"

#if MPT_OS_WINDOWS
 #include <windows.h>
#else // !MPT_OS_WINDOWS
 #ifdef _UNIX
  #define MPT_UNRAR_UNIX_WAS_DEFINED
 #else
  #define _UNIX
 #endif
#endif // MPT_OS_WINDOWS

#include "unrar/dll.hpp"

#if !MPT_OS_WINDOWS
 #ifndef MPT_UNRAR_UNIX_WAS_DEFINED
  #undef _UNIX
  #undef MPT_UNRAR_UNIX_WAS_DEFINED
 #endif
#endif // !MPT_OS_WINDOWS

#endif // MPT_WITH_UNRAR


OPENMPT_NAMESPACE_BEGIN


#ifdef MPT_WITH_UNRAR


struct RARHandle // RAII
{
	HANDLE rar = nullptr;
	explicit RARHandle(HANDLE rar_) : rar(rar_) { return; }
	RARHandle(const RARHandle &) = delete;
	~RARHandle() { if(rar) RARCloseArchive(rar); }

	operator HANDLE () const { return rar; }
};


static int CALLBACK RARCallback(unsigned int msg, LPARAM userData, LPARAM p1, LPARAM p2)
{
	int result = 0;
	CRarArchive &that = *mpt::void_ptr<CRarArchive>(mpt::pointer_cast<void*>(userData));
	switch(msg)
	{
	case UCM_PROCESSDATA:
		// Receive extracted data
		that.RARCallbackProcessData(mpt::pointer_cast<const char*>(p1), p2);
		result = 1;
		break;
	default:
		// No support for passwords or volumes
		result = -1;
		break;
	}
	return result;
}


void CRarArchive::RARCallbackProcessData(const char * buf, std::size_t size)
{
	if(!captureCurrentFile)
	{
		return;
	}
	mpt::append(data, buf, buf + size);
}


CRarArchive::CRarArchive(FileReader &file)
	: ArchiveBase(file)
{
	// NOTE:
	//  We open the archive twice, once for listing the contents in the
	// constructor and once for actual decompression in ExtractFile.
	//  For solid archives, listing the contents via RAR_OM_LIST is way faster.
	// The overhead of opening twice for non-solid archives is negligable if the
	// archive does not contain a lot of files (and archives with large amount of
	// files are pretty useless for OpenMPT anyway).

	// Early reject files with no Rar! magic
	// so that we do not have to instantiate FileAdapter.
	inFile.Rewind();
	if(!inFile.ReadMagic("Rar!\x1A"))
	{
		return;
	}
	inFile.Rewind();

	diskFile = std::make_unique<mpt::IO::FileAdapter<FileCursor>>(inFile, mpt::TemporaryPathname{P_("rar")}.GetPathname());
	if(!diskFile->IsValid())
	{
		return;
	}

	std::wstring ArcName = mpt::transcode<std::wstring>(diskFile->GetFilename());
	std::vector<wchar_t> ArcNameBuf(ArcName.c_str(), ArcName.c_str() + ArcName.length() + 1);
	std::vector<wchar_t> CmtBuf(65536);
	RAROpenArchiveDataEx ArchiveData = {};
	ArchiveData.OpenMode = RAR_OM_LIST;
	ArchiveData.ArcNameW = ArcNameBuf.data();
	ArchiveData.CmtBufW = CmtBuf.data();
	ArchiveData.CmtBufSize = static_cast<unsigned int>(CmtBuf.size());
	RARHandle rar(RAROpenArchiveEx(&ArchiveData));
	if(!rar)
	{
		Reset();
		return;
	}

	switch(ArchiveData.CmtState)
	{
	case 1:
		if(ArchiveData.CmtSize)
		{
			comment = mpt::ToUnicode(std::wstring(ArchiveData.CmtBufW, ArchiveData.CmtBufW + ArchiveData.CmtSize - 1));
			break;
		}
		[[fallthrough]];
	case 0:
		comment = mpt::ustring();
		break;
	default:
		Reset();
		return;
		break;
	}

	bool eof = false;
	int RARResult = 0;
	while(!eof)
	{
		RARHeaderDataEx HeaderData = {};
		RARResult = RARReadHeaderEx(rar, &HeaderData);
		switch(RARResult)
		{
		case ERAR_SUCCESS:
			break;
		case ERAR_END_ARCHIVE:
			eof = true;
			continue;
			break;
		default:
			Reset();
			return;
			break;
		}
		ArchiveFileInfo fileInfo;
		fileInfo.name = mpt::PathString::FromWide(HeaderData.FileNameW);
		fileInfo.type = ArchiveFileType::Normal;
		fileInfo.size = HeaderData.UnpSize;
		contents.push_back(fileInfo);
		RARResult = RARProcessFileW(rar, RAR_SKIP, NULL, NULL);
		switch(RARResult)
		{
		case ERAR_SUCCESS:
			break;
		default:
			Reset();
			return;
			break;
		}
	}

}


CRarArchive::~CRarArchive()
{
}


bool CRarArchive::ExtractFile(std::size_t index)
{

	if(!diskFile || !diskFile->IsValid())
	{
		return false;
	}

	if(index >= contents.size())
	{
		return false;
	}

	std::wstring ArcName = mpt::transcode<std::wstring>(diskFile->GetFilename());
	std::vector<wchar_t> ArcNameBuf(ArcName.c_str(), ArcName.c_str() + ArcName.length() + 1);
	RAROpenArchiveDataEx ArchiveData = {};
	ArchiveData.OpenMode = RAR_OM_EXTRACT;
	ArchiveData.ArcNameW = ArcNameBuf.data();
	ArchiveData.Callback = RARCallback;
	ArchiveData.UserData = mpt::pointer_cast<LPARAM>(this);
	RARHandle rar(RAROpenArchiveEx(&ArchiveData));
	if(!rar)
	{
		ResetFile();
		return false;
	}

	std::size_t i = 0;
	int RARResult = 0;
	bool eof = false;
	while(!eof)
	{
		RARHeaderDataEx HeaderData = {};
		RARResult = RARReadHeaderEx(rar, &HeaderData);
		switch(RARResult)
		{
		case ERAR_SUCCESS:
			break;
		case ERAR_END_ARCHIVE:
			eof = true;
			continue;
			break;
		default:
			ResetFile();
			return false;
			break;
		}
		captureCurrentFile = (i == index);
		RARResult = RARProcessFileW(rar, captureCurrentFile ? RAR_TEST : RAR_SKIP, NULL, NULL);
		switch(RARResult)
		{
		case ERAR_SUCCESS:
			break;
		default:
			ResetFile();
			return false;
			break;
		}
		if(captureCurrentFile)
		{ // done
			return true;
		}
		captureCurrentFile = false;
		++i;
	}

	return false;

}


void CRarArchive::Reset()
{
	captureCurrentFile = false;
	comment = mpt::ustring();
	contents = std::vector<ArchiveFileInfo>();
	data = std::vector<char>();
}


void CRarArchive::ResetFile()
{
	captureCurrentFile = false;
	data = std::vector<char>();
}


#endif // MPT_WITH_UNRAR


OPENMPT_NAMESPACE_END
