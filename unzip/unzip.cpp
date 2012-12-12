/*
 * unzip.cpp
 * ---------
 * Purpose: Implementation file for extracting modules from .zip archives
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#define static_assert	// DIRTY!
#include "../soundlib/FileReader.h"
#include "unzip.h"
#include "../common/misc_util.h"

#ifndef ZLIB_WINAPI
#define ZLIB_WINAPI
#endif // ZLIB_WINAPI
#include <contrib/minizip/unzip.h>


// Low-level file abstractions for in-memory file handling
struct ZipFileAbstraction
{
	static voidpf ZCALLBACK fopen_mem(voidpf opaqe, const char *filename, int mode)
	{
		FileReader &file = *static_cast<FileReader *>(opaqe);
		if((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_WRITE)
		{
			return nullptr;
		} else
		{
			file.Rewind();
			return opaqe;
		}
	}

	static uLong ZCALLBACK fread_mem(voidpf opaqe, voidpf, void *buf, uLong size)
	{
		FileReader &file = *static_cast<FileReader *>(opaqe);
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

	static long ZCALLBACK ftell_mem(voidpf opaqe, voidpf)
	{
		FileReader &file = *static_cast<FileReader *>(opaqe);
		return file.GetPosition();
	}

	static long ZCALLBACK fseek_mem(voidpf opaqe, voidpf, uLong offset, int origin)
	{
		FileReader &file = *static_cast<FileReader *>(opaqe);

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


CZipArchive::CZipArchive(FileReader &file, const char *ext) : inFile(file), extensions(ext)
//-----------------------------------------------------------------------------------------
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


bool CZipArchive::ExtractFile()
//-----------------------------
{
	unz_file_pos bestFile;
	unz_file_info info;

	int status = unzGoToFirstFile(zipFile);
	unzGetFilePos(zipFile, &bestFile);

	while(status == UNZ_OK)
	{
		char name[256], *ext = name;
		unzGetCurrentFileInfo(zipFile, &info, name, sizeof(name), nullptr, 0, nullptr, 0);
		// Extract file extension
		while(ext != '\0')
		{
			if(*(ext++) == '.')
			{
				break;
			}
		}

		// Compare with list of preferred extensions
		char extCmp[16];
		size_t i = 0, j = 0;
		bool foundPreferred = false;
		while(extensions[i])
		{
			char c = extensions[i++];
			extCmp[j] = c;
			if(c == '|')
			{
				extCmp[j] = 0;
				j = 0;
				if(!lstrcmpi(ext, extCmp))
				{
					// File has a preferred extension: use it.
					unzGetFilePos(zipFile, &bestFile);
					foundPreferred = true;
					break;
				}
			} else
			{
				j++;
			}
		}
		if(foundPreferred)
		{
			break;
		}

		if(lstrcmpi(ext, "diz")
			&& lstrcmpi(ext, "nfo")
			&& lstrcmpi(ext, "txt"))
		{
			// If this isn't some kind of info file, we should maybe pick it.
			unzGetFilePos(zipFile, &bestFile);
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


void *CZipArchive::GetComments(bool get)
//--------------------------------------
{
	unz_global_info info;
	if(zipFile == nullptr || unzGetGlobalInfo(zipFile, &info) != UNZ_OK)
	{
		return nullptr;
	}

	if(!get)
	{
		return reinterpret_cast<void *>((info.size_comment > 0) ? 1 : 0);
	} else if(info.size_comment > 0)
	{
		if(info.size_comment < Util::MaxValueOfType(info.size_comment))
		{
			info.size_comment++;
		}
		char *comment = new char[info.size_comment];
		if(unzGetGlobalComment(zipFile, comment, info.size_comment) >= 0)
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
