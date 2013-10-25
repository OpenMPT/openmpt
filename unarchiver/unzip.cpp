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
#include "unzip.h"
#include "../common/misc_util.h"
#include <algorithm>
#include <vector>

#if !defined(NO_ZLIB)
#include <contrib/minizip/unzip.h>
#elif !defined(NO_MINIZ)
#define MINIZ_HEADER_FILE_ONLY
#include <miniz/miniz.c>
#endif


#if !defined(NO_ZLIB)


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


CZipArchive::CZipArchive(FileReader &file) : ArchiveBase(file)
//------------------------------------------------------------
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

	if(zipFile == nullptr)
	{
		return;
	}

	// read comment
	{
		unz_global_info info;
		if(unzGetGlobalInfo(zipFile, &info) == UNZ_OK)
		{
			if(info.size_comment > 0)
			{
				if(info.size_comment < Util::MaxValueOfType(info.size_comment))
				{
					info.size_comment++;
				}
				std::vector<char> commentData(info.size_comment);
				if(unzGetGlobalComment(zipFile, &commentData[0], info.size_comment) >= 0)
				{
					commentData[info.size_comment - 1] = '\0';
					comment = &commentData[0];
				}
			}
		}
	}

	// read contents
	unz_file_pos bestFile;
	unz_file_info info;

	int status = unzGoToFirstFile(zipFile);
	unzGetFilePos(zipFile, &bestFile);

	while(status == UNZ_OK)
	{
		ArchiveFileInfo fileinfo;

		fileinfo.type = ArchiveFileNormal;
		
		char name[256];
		unzGetCurrentFileInfo(zipFile, &info, name, sizeof(name), nullptr, 0, nullptr, 0);
		fileinfo.name = name;
		fileinfo.size = info.uncompressed_size;

		unzGetFilePos(zipFile, &bestFile);
		fileinfo.cookie1 = bestFile.pos_in_zip_directory;
		fileinfo.cookie2 = bestFile.num_of_file;

		contents.push_back(fileinfo);

		status = unzGoToNextFile(zipFile);
	}

}


CZipArchive::~CZipArchive()
//-------------------------
{
	unzClose(zipFile);
}


bool CZipArchive::ExtractFile(std::size_t index)
//----------------------------------------------
{
	if(index >= contents.size())
	{
		return false;
	}

	data.clear();

	unz_file_pos bestFile;
	unz_file_info info;

	bestFile.pos_in_zip_directory = static_cast<uLong>(contents[index].cookie1);
	bestFile.num_of_file = static_cast<uLong>(contents[index].cookie2);

	if(unzGoToFilePos(zipFile, &bestFile) == UNZ_OK && unzOpenCurrentFile(zipFile) == UNZ_OK)
	{
		unzGetCurrentFileInfo(zipFile, &info, nullptr, 0, nullptr, 0, nullptr, 0);
		
		try
		{
			data.resize(info.uncompressed_size);
		} catch(...)
		{
			unzCloseCurrentFile(zipFile);
			return false;
		}
		unzReadCurrentFile(zipFile, &data[0], info.uncompressed_size);
		unzCloseCurrentFile(zipFile);

		return true;
	}

	return false;
}


#elif !defined(NO_MINIZ)


CZipArchive::CZipArchive(FileReader &file) : ArchiveBase(file)
//------------------------------------------------------------
{
	zipFile = new mz_zip_archive();
	
	mz_zip_archive *zip = static_cast<mz_zip_archive*>(zipFile);
	
	MemsetZero(*zip);
	if(!mz_zip_reader_init_mem(zip, file.GetRawData(), file.GetLength(), 0))
	{
		delete zip;
		zip = nullptr;
		zipFile = nullptr;
	}

	if(!zip)
	{
		return;
	}

	for(mz_uint i = 0; i < mz_zip_reader_get_num_files(zip); ++i)
	{
		ArchiveFileInfo info;
		info.type = ArchiveFileInvalid;
		mz_zip_archive_file_stat stat;
		MemsetZero(stat);
		if(mz_zip_reader_file_stat(zip, i, &stat))
		{
			info.type = ArchiveFileNormal;
			info.name = stat.m_filename;
			info.size = stat.m_uncomp_size;
		}
		if(mz_zip_reader_is_file_a_directory(zip, i))
		{
			info.type = ArchiveFileSpecial;
		} else if(mz_zip_reader_is_file_encrypted(zip, i))
		{
			info.type = ArchiveFileSpecial;
		}
		contents.push_back(info);
	}

}


CZipArchive::~CZipArchive()
//-------------------------
{
	mz_zip_archive *zip = static_cast<mz_zip_archive*>(zipFile);

	if(zip)
	{
		mz_zip_reader_end(zip);

		delete zip;
		zipFile = nullptr;
	}

}


bool CZipArchive::ExtractFile(std::size_t index)
//----------------------------------------------
{
	mz_zip_archive *zip = static_cast<mz_zip_archive*>(zipFile);

	if(index >= contents.size())
	{
		return false;
	}

	mz_uint bestFile = index;

	mz_zip_archive_file_stat stat;
	MemsetZero(stat);
	mz_zip_reader_file_stat(zip, bestFile, &stat);
	if(stat.m_uncomp_size >= std::numeric_limits<std::size_t>::max())
	{
		return false;
	}
	try
	{
		data.resize(static_cast<std::size_t>(stat.m_uncomp_size));
	} catch(...)
	{
		return false;
	}
	if(!mz_zip_reader_extract_to_mem(zip, bestFile, &data[0], static_cast<std::size_t>(stat.m_uncomp_size), 0))
	{
		return false;
	}
	comment = std::string(stat.m_comment, stat.m_comment + stat.m_comment_size);
	return true;
}


#endif // NO_ZLIB || NO_MINIZ
