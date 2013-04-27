/*
 * unzip.cpp
 * ---------
 * Purpose: Implementation file for extracting modules from .zip archives, making use of MiniZip (from the zlib contrib package)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "../soundlib/FileReader.h"
#include <vector>
#include "unzip.h"
#include "../common/misc_util.h"
#include <algorithm>

#ifndef ZLIB_WINAPI
#define ZLIB_WINAPI
#endif // ZLIB_WINAPI
#include <contrib/minizip/unzip.h>


// Low-level file abstractions for in-memory file handling
struct ZipFileAbstraction
{
	static voidpf ZCALLBACK fopen_mem(voidpf opaque, const char *, int mode)
	{
		FileReader &file = *static_cast<FileReader *>(opaque);
		if((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_WRITE)
		{
			return nullptr;
		} else
		{
			file.Rewind();
			return opaque;
		}
	}

	static uLong ZCALLBACK fread_mem(voidpf opaque, voidpf, void *buf, uLong size)
	{
		FileReader &file = *static_cast<FileReader *>(opaque);
		if(size > file.BytesLeft())
		{
			size = file.BytesLeft();
		}
		memcpy(buf, file.GetRawData(), size);
		file.Skip(size);
		return size;
	}

	static uLong ZCALLBACK fwrite_mem(voidpf, voidpf, const void *, uLong)
	{
		return 0;
	}

	static long ZCALLBACK ftell_mem(voidpf opaque, voidpf)
	{
		FileReader &file = *static_cast<FileReader *>(opaque);
		return file.GetPosition();
	}

	static long ZCALLBACK fseek_mem(voidpf opaque, voidpf, uLong offset, int origin)
	{
		FileReader &file = *static_cast<FileReader *>(opaque);

		switch(origin)
		{
		case ZLIB_FILEFUNC_SEEK_CUR:
			offset += file.GetPosition();
			break;
		case ZLIB_FILEFUNC_SEEK_END:
			offset += file.GetLength();
			break;
		case ZLIB_FILEFUNC_SEEK_SET:
			break;
		default:
			return -1;
		}
		return (file.Seek(offset) ? 0 : 1);
	}

	static int ZCALLBACK fclose_mem(voidpf, voidpf)
	{
		return 0;
	}

	static int ZCALLBACK ferror_mem(voidpf, voidpf)
	{
		return 0;
	}
};


CZipArchive::CZipArchive(FileReader &file, const std::vector<const char *> &ext) : inFile(file), extensions(ext)
//--------------------------------------------------------------------------------------------------------------
{
	zlib_filefunc_def functions =
	{
		ZipFileAbstraction::fopen_mem,
		ZipFileAbstraction::fread_mem,
		ZipFileAbstraction::fwrite_mem,
		ZipFileAbstraction::ftell_mem,
		ZipFileAbstraction::fseek_mem,
		ZipFileAbstraction::fclose_mem,
		ZipFileAbstraction::ferror_mem,
		&inFile
	};
	zipFile = unzOpen2(nullptr, &functions);
}


CZipArchive::~CZipArchive()
//-------------------------
{
	unzClose(zipFile);
	delete[] outFile.GetRawData();
}


bool CZipArchive::IsArchive() const
//---------------------------------
{
	return (zipFile != nullptr);
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


bool CZipArchive::ExtractFile()
//-----------------------------
{
	unz_file_pos bestFile;
	unz_file_info info;
	uLong biggestFile = 0;

	int status = unzGoToFirstFile(zipFile);
	unzGetFilePos(zipFile, &bestFile);

	while(status == UNZ_OK)
	{
		char name[256];
		unzGetCurrentFileInfo(zipFile, &info, name, sizeof(name), nullptr, 0, nullptr, 0);

		// Extract file extension
		char *ext = name + info.size_filename;
		while(ext > name)
		{
			ext--;
			*ext = tolower(*ext);
			if(*ext == '.')
			{
				ext++;
				break;
			}
		}

		// Compare with list of preferred extensions
		if(std::find_if(extensions.begin(), extensions.end(), find_str(ext)) != extensions.end())
		{
			// File has a preferred extension: use it.
			unzGetFilePos(zipFile, &bestFile);
			break;
		}

		if(strcmp(ext, "diz")
			&& strcmp(ext, "nfo")
			&& strcmp(ext, "txt")
			&& info.uncompressed_size >= biggestFile)
		{
			// If this isn't some kind of info file, we should maybe pick it.
			unzGetFilePos(zipFile, &bestFile);
			biggestFile = info.uncompressed_size;
		}

		status = unzGoToNextFile(zipFile);
	}

	if(unzGoToFilePos(zipFile, &bestFile) == UNZ_OK && unzOpenCurrentFile(zipFile) == UNZ_OK)
	{
		unzGetCurrentFileInfo(zipFile, &info, nullptr, 0, nullptr, 0, nullptr, 0);
		
		delete[] outFile.GetRawData();
		char *data = new (std::nothrow) char[info.uncompressed_size];
		if(data != nullptr)
		{
			unzReadCurrentFile(zipFile, data, info.uncompressed_size);
			outFile = FileReader(data, info.uncompressed_size);
		}
		unzCloseCurrentFile(zipFile);

		return (data != nullptr);
	}

	return false;
}


const char *CZipArchive::GetComments(bool get)
//--------------------------------------------
{
	unz_global_info info;
	if(zipFile == nullptr || unzGetGlobalInfo(zipFile, &info) != UNZ_OK)
	{
		return nullptr;
	}

	if(!get)
	{
		return reinterpret_cast<char *>((info.size_comment > 0) ? 1 : 0);
	} else if(info.size_comment > 0)
	{
		if(info.size_comment < Util::MaxValueOfType(info.size_comment))
		{
			info.size_comment++;
		}
		char *comment = new (std::nothrow) char[info.size_comment];
		if(comment != nullptr && unzGetGlobalComment(zipFile, comment, info.size_comment) >= 0)
		{
			comment[info.size_comment - 1] = '\0';
			return comment;
		} else
		{
			delete[] comment;
		}
	}
	return nullptr;
}
