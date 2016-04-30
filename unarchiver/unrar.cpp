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
	HANDLE rar;
	RARHandle() : rar(NULL) { return; }
	explicit RARHandle(HANDLE rar_) : rar(rar_) { return; }
	RARHandle & operator = (HANDLE rar_) { if(rar) { RARCloseArchive(rar); rar = NULL; } rar = rar_; return *this; }
	operator HANDLE () const { return rar; }
	~RARHandle() { if(rar) { RARCloseArchive(rar); rar = NULL; } }
private:
	RARHandle(const RARHandle &rar_);
	RARHandle & operator = (const RARHandle &rar_);
};


static int CALLBACK RARCallback(unsigned int msg, LPARAM userData, LPARAM p1, LPARAM p2)
{
	int result = 0;
	CRarArchive *that = reinterpret_cast<CRarArchive *>(userData);
	switch(msg)
	{
	case UCM_PROCESSDATA:
		// Receive extracted data
		that->RARCallbackProcessData(reinterpret_cast<const char *>(p1), p2);
		result = 1;
		break;
	default:
		// No support for passwords or volumes
		result = -1;
		break;
	}
	return result;
}


void CRarArchive::RARCallbackProcessData(const char * buf, std::intptr_t size)
//----------------------------------------------------------------------------
{
	if(!captureCurrentFile)
	{
		return;
	}
	data.insert(data.end(), buf, buf + size);
}


CRarArchive::CRarArchive(FileReader &file)
//----------------------------------------
	: ArchiveBase(file)
	, diskFile()
	, captureCurrentFile(false)
{

	// NOTE:
	//  We open the archive twice, once for listing the contents in the
	// constructor and once for actual decompression in ExtractFile.
	//  For solid archives, listing the contents via RAR_OM_LIST is way faster.
	// The overhead of opening twice for non-solid archives is negligable if the
	// archive does not contain a lot of files (and archives with large amount of
	// files are pretty useless for OpenMPT anyway).

	// Early reject files with no Rar! magic
	// so that we do not have to instantiate OnDiskFileWrapper.
	inFile.Rewind();
	if(!inFile.ReadMagic("Rar!"))
	{
		return;
	}
	inFile.Rewind();

	diskFile = MPT_SHARED_PTR<OnDiskFileWrapper>(new OnDiskFileWrapper(inFile));
	if(!diskFile->IsValid())
	{
		return;
	}

	RARHandle rar;
	int RARResult = 0;

	std::wstring ArcName = diskFile->GetFilename().AsNative();
	std::vector<wchar_t> ArcNameBuf(ArcName.c_str(), ArcName.c_str() + ArcName.length() + 1);
	std::vector<char> CmtBuf(65536);
	RAROpenArchiveDataEx ArchiveData;
	MemsetZero(ArchiveData);
	ArchiveData.OpenMode = RAR_OM_LIST;
	ArchiveData.ArcNameW = &(ArcNameBuf[0]);
	ArchiveData.CmtBuf = &(CmtBuf[0]);
	ArchiveData.CmtBufSize = CmtBuf.size() - 1;
	rar = RAROpenArchiveEx(&ArchiveData);
	if(!rar)
	{
		Reset();
		return;
	}

	#if 1
		// Use RARGetCommentW (OPENMPT ADDITION)
		{
			std::vector<wchar_t> CmtBuf(65536);
			unsigned int CmtSize = 0;
			RARResult = RARGetCommentW(rar, &(CmtBuf[0]), static_cast<unsigned int>(CmtBuf.size()), &CmtSize);
			switch(RARResult)
			{
			case 1:
				comment = mpt::ToUnicode(std::wstring(&(CmtBuf[0]), &(CmtBuf[0]) + CmtSize));
				break;
			case 0:
				comment = mpt::ustring();
				break;
			default:
				Reset();
				return;
				break;
			}
		}
	#else
		// unrar.dll interface does not allow reading comment as a wide string
		switch(ArchiveData.CmtState)
		{
		case 1:
			comment = mpt::ToUnicode(mpt::CharsetLocale, std::string(ArchiveData.CmtBuf, ArchiveData.CmtBuf + ArchiveData.CmtSize));
			break;
		case 0:
			comment = mpt::ustring();
			break;
		default:
			Reset();
			return;
			break;
		}
	#endif

	bool eof = false;
	while(!eof)
	{
		RARHeaderDataEx HeaderData;
		MemsetZero(HeaderData);
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
		fileInfo.type = ArchiveFileNormal;
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
//-------------------------
{
	return;
}


bool CRarArchive::ExtractFile(std::size_t index)
//----------------------------------------------
{

	if(!diskFile || !diskFile->IsValid())
	{
		return false;
	}

	if(index >= contents.size())
	{
		return false;
	}

	RARHandle rar;
	int RARResult = 0;

	std::wstring ArcName = diskFile->GetFilename().AsNative();
	std::vector<wchar_t> ArcNameBuf(ArcName.c_str(), ArcName.c_str() + ArcName.length() + 1);
	RAROpenArchiveDataEx ArchiveData;
	MemsetZero(ArchiveData);
	ArchiveData.OpenMode = RAR_OM_EXTRACT;
	ArchiveData.ArcNameW = &(ArcNameBuf[0]);
	ArchiveData.Callback = RARCallback;
	ArchiveData.UserData = reinterpret_cast<LPARAM>(reinterpret_cast<void*>(this));
	rar = RAROpenArchiveEx(&ArchiveData);
	if(!rar)
	{
		ResetFile();
		return false;
	}

	std::size_t i = 0;
	bool eof = false;
	while(!eof)
	{
		RARHeaderDataEx HeaderData;
		MemsetZero(HeaderData);
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
//-----------------------
{
	captureCurrentFile = false;
	comment = mpt::ustring();
	contents = std::vector<ArchiveFileInfo>();
	data = std::vector<char>();
}


void CRarArchive::ResetFile()
//---------------------------
{
	captureCurrentFile = false;
	data = std::vector<char>();
}


#endif // MPT_WITH_UNRAR


OPENMPT_NAMESPACE_END
