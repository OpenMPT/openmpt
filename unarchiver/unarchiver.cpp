/*
 * unarchiver.cpp
 * --------------
 * Purpose: archive loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include <Windows.h>

#include "unarchiver.h"
#include "../soundlib/FileReader.h"

CUnarchiver::CUnarchiver(FileReader &file, const std::vector<const char *> &extensions) :
inFile(file),
zipArchive(inFile, extensions),
rarArchive((LPBYTE)inFile.GetRawData(), inFile.GetLength()),
lhaArchive((LPBYTE)inFile.GetRawData(), inFile.GetLength()),
gzipArchive(inFile)
//---------------------------------------------------------------------------------
{
	inFile.Rewind();
}


CUnarchiver::~CUnarchiver()
//-------------------------
{
	return;
}


bool CUnarchiver::IsArchive() const
//---------------------------------
{
	return false
#ifdef ZIPPED_MOD_SUPPORT
		|| zipArchive.IsArchive()
#endif
#ifdef UNRAR_SUPPORT
		|| rarArchive.IsArchive()
#endif
#ifdef UNLHA_SUPPORT
		|| lhaArchive.IsArchive()
#endif
#ifdef UNGZIP_SUPPORT
		|| gzipArchive.IsArchive()
#endif
		;
}


bool CUnarchiver::ExtractFile()
//-----------------------------
{
#ifdef ZIPPED_MOD_SUPPORT
	if(zipArchive.IsArchive())
	{
		if(!zipArchive.ExtractFile()) return false;
		outFile = zipArchive.GetOutputFile();
		return outFile.GetRawData()?true:false;
	}
#endif
#ifdef UNRAR_SUPPORT
	if(rarArchive.IsArchive())
	{
		if(!rarArchive.ExtrFile()) return false;
		outFile = FileReader(rarArchive.GetOutputFile(), rarArchive.GetOutputFileLength());
		return outFile.GetRawData()?true:false;
	}
#endif
#ifdef UNLHA_SUPPORT
	if(lhaArchive.IsArchive())
	{
		if(!lhaArchive.ExtractFile()) return false;
		outFile = FileReader(lhaArchive.GetOutputFile(), lhaArchive.GetOutputFileLength());
		return outFile.GetRawData()?true:false;
	}
#endif
#ifdef UNGZIP_SUPPORT
	if(gzipArchive.IsArchive())
	{
		if(!gzipArchive.ExtractFile()) return false;
		outFile = gzipArchive.GetOutputFile();
		return outFile.GetRawData()?true:false;
	}
#endif
	return false;
}


const char *CUnarchiver::GetComments(bool get)
{
#ifdef ZIPPED_MOD_SUPPORT
	if(!zipArchive.IsArchive()) return nullptr;
	return zipArchive.GetComments(get);
#else
	return nullptr;
#endif
}
