/*
 * unzip.cpp
 * ---------
 * Purpose: Implementation file for extracting modules from .zip archives, making use of MiniZip (from the zlib contrib package)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "../common/FileReader.h"
#include "unzip.h"
#include "../common/misc_util.h"
#include <algorithm>
#include <vector>

#if defined(MPT_WITH_ZLIB) && defined(MPT_WITH_MINIZIP)
#include <contrib/minizip/unzip.h>
#elif defined(MPT_WITH_MINIZ)
#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced function with internal linkage has been removed
#elif MPT_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#elif MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
#include <miniz/miniz.h>
#if MPT_COMPILER_MSVC
//#pragma warning(pop)
#elif MPT_COMPILER_GCC
#pragma GCC diagnostic pop
#elif MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif
#endif


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_WITH_ZLIB) && defined(MPT_WITH_MINIZIP)


// Low-level file abstractions for in-memory file handling
struct ZipFileAbstraction
{
	static voidpf ZCALLBACK fopen64_mem(voidpf opaque, const void *, int mode)
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
		return mpt::saturate_cast<uLong>(file.ReadRaw(mpt::span(mpt::void_cast<std::byte*>(buf), size)).size());
	}

	static uLong ZCALLBACK fwrite_mem(voidpf, voidpf, const void *, uLong)
	{
		return 0;
	}

	static ZPOS64_T ZCALLBACK ftell64_mem(voidpf opaque, voidpf)
	{
		FileReader &file = *static_cast<FileReader *>(opaque);
		return file.GetPosition();
	}

	static long ZCALLBACK fseek64_mem(voidpf opaque, voidpf, ZPOS64_T offset, int origin)
	{
		FileReader &file = *static_cast<FileReader *>(opaque);
		ZPOS64_T destination = 0;
		switch(origin)
		{
		case ZLIB_FILEFUNC_SEEK_CUR:
			destination = static_cast<ZPOS64_T>(file.GetPosition()) + offset;
			break;
		case ZLIB_FILEFUNC_SEEK_END:
			destination = static_cast<ZPOS64_T>(file.GetLength()) + offset;
			break;
		case ZLIB_FILEFUNC_SEEK_SET:
			destination = offset;
			break;
		default:
			return -1;
		}
		if(!mpt::in_range<FileReader::off_t>(destination))
		{
			return 1;
		}
		return (file.Seek(static_cast<FileReader::off_t>(destination)) ? 0 : 1);
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


CZipArchive::CZipArchive(FileReader &file)
	: ArchiveBase(file)
	, zipFile(nullptr)
{
	zlib_filefunc64_def functions =
	{
		ZipFileAbstraction::fopen64_mem,
		ZipFileAbstraction::fread_mem,
		ZipFileAbstraction::fwrite_mem,
		ZipFileAbstraction::ftell64_mem,
		ZipFileAbstraction::fseek64_mem,
		ZipFileAbstraction::fclose_mem,
		ZipFileAbstraction::ferror_mem,
		&inFile
	};

	zipFile = unzOpen2_64(nullptr, &functions);

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
				if(unzGetGlobalComment(zipFile, commentData.data(), info.size_comment) >= 0)
				{
					commentData[info.size_comment - 1] = '\0';
					comment = mpt::ToUnicode(mpt::IsUTF8(commentData.data()) ? mpt::Charset::UTF8 : mpt::Charset::CP437, commentData.data());
				}
			}
		}
	}

	// read contents
	unz_file_pos curFile;
	int status = unzGoToFirstFile(zipFile);
	unzGetFilePos(zipFile, &curFile);

	while(status == UNZ_OK)
	{
		ArchiveFileInfo fileinfo;

		fileinfo.type = ArchiveFileType::Normal;
		
		unz_file_info info;
		char name[256];
		unzGetCurrentFileInfo(zipFile, &info, name, sizeof(name), nullptr, 0, nullptr, 0);
		fileinfo.name = mpt::PathString::FromUnicode(mpt::ToUnicode((info.flag & (1<<11)) ? mpt::Charset::UTF8 : mpt::Charset::CP437, std::string(name)));
		fileinfo.size = info.uncompressed_size;

		unzGetFilePos(zipFile, &curFile);
		fileinfo.cookie1 = curFile.pos_in_zip_directory;
		fileinfo.cookie2 = curFile.num_of_file;

		contents.push_back(fileinfo);

		status = unzGoToNextFile(zipFile);
	}

}


CZipArchive::~CZipArchive()
{
	unzClose(zipFile);
}


bool CZipArchive::ExtractFile(std::size_t index)
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
		unzReadCurrentFile(zipFile, data.data(), info.uncompressed_size);
		unzCloseCurrentFile(zipFile);

		return true;
	}

	return false;
}


#elif defined(MPT_WITH_MINIZ)


CZipArchive::CZipArchive(FileReader &file) : ArchiveBase(file)
{
	zipFile = new mz_zip_archive();
	
	mz_zip_archive *zip = static_cast<mz_zip_archive*>(zipFile);
	
	(*zip) = {};
	const auto fileData = file.GetRawData();
	if(!mz_zip_reader_init_mem(zip, fileData.data(), fileData.size(), 0))
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
		info.type = ArchiveFileType::Invalid;
		mz_zip_archive_file_stat stat = {};
		if(mz_zip_reader_file_stat(zip, i, &stat))
		{
			info.type = ArchiveFileType::Normal;
			info.name = mpt::PathString::FromUnicode(mpt::ToUnicode((stat.m_bit_flag & (1<<11)) ? mpt::Charset::UTF8 : mpt::Charset::CP437, stat.m_filename));
			info.size = stat.m_uncomp_size;
		}
		if(mz_zip_reader_is_file_a_directory(zip, i))
		{
			info.type = ArchiveFileType::Special;
		} else if(mz_zip_reader_is_file_encrypted(zip, i))
		{
			info.type = ArchiveFileType::Special;
		}
		contents.push_back(info);
	}

}


CZipArchive::~CZipArchive()
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
{
	mz_zip_archive *zip = static_cast<mz_zip_archive*>(zipFile);

	if(index >= contents.size())
	{
		return false;
	}

	mz_uint bestFile = index;

	mz_zip_archive_file_stat stat = {};
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
	if(!mz_zip_reader_extract_to_mem(zip, bestFile, data.data(), static_cast<std::size_t>(stat.m_uncomp_size), 0))
	{
		return false;
	}
	comment = mpt::ToUnicode(mpt::Charset::CP437, std::string(stat.m_comment, stat.m_comment + stat.m_comment_size));
	return true;
}


#endif // MPT_WITH_ZLIB || MPT_WITH_MINIZ


OPENMPT_NAMESPACE_END
