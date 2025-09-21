/*
 * ungzip.cpp
 * ----------
 * Purpose: Implementation file for extracting modules from .gz archives
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "ungzip.h"
#include "../common/FileReader.h"

#if defined(MPT_WITH_ZLIB) || defined(MPT_WITH_MINIZ)

#if defined(MPT_WITH_ZLIB)
#include <zlib.h>
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

#endif // MPT_WITH_ZLIB || MPT_WITH_MINIZ


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_WITH_ZLIB) || defined(MPT_WITH_MINIZ)


CGzipArchive::CGzipArchive(const FileReader &file) : ArchiveBase(file)
{
	inFile.Rewind();
	inFile.ReadStruct(header);

	// Check header data + file size
	if(header.magic1 != GZ_HMAGIC1 || header.magic2 != GZ_HMAGIC2 || header.method != GZ_HMDEFLATE || (header.flags & GZ_FRESERVED) != 0
		|| inFile.GetLength() <= sizeof(GZheader) + sizeof(GZtrailer))
	{
		return;
	}
	ArchiveFileInfo info;
	info.type = ArchiveFileType::Normal;

	// Extra block present? (skip the extra data)
	if(header.flags & GZ_FEXTRA)
	{
		inFile.Skip(inFile.ReadUint16LE());
	}

	// Filename present?
	std::string str;
	if(header.flags & GZ_FNAME)
	{
		inFile.ReadNullString(str);
		info.name = mpt::PathString::FromUnicode(mpt::ToUnicode(mpt::Charset::ISO8859_1, str));
	}

	// Comment present?
	if(header.flags & GZ_FCOMMENT)
	{
		inFile.ReadNullString(str);
		info.comment = mpt::ToUnicode(mpt::Charset::ISO8859_1, str);
	}

	contents.push_back(info);
}


CGzipArchive::~CGzipArchive()
{
	return;
}


bool CGzipArchive::ExtractFile(std::size_t index)
{
	if(index >= contents.size())
	{
		return false;
	}

	// Read trailer
	GZtrailer trailer;
	inFile.Seek(inFile.GetLength() - sizeof(GZtrailer));
	inFile.ReadStruct(trailer);

	// Continue reading header
	inFile.Seek(sizeof(GZheader));

	// Extra block present? (skip the extra data)
	if(header.flags & GZ_FEXTRA)
	{
		inFile.Skip(inFile.ReadUint16LE());
	}

	// Filename present? (ignore)
	if(header.flags & GZ_FNAME)
	{
		while(inFile.ReadUint8() != 0);
	}

	// Comment present? (ignore)
	if(header.flags & GZ_FCOMMENT)
	{
		while(inFile.ReadUint8() != 0);
	}

	// CRC16 present? (ignore)
	if(header.flags & GZ_FHCRC)
	{
		inFile.Skip(2);
	}

	// Well, this is a bit small when deflated.
	if(!inFile.CanRead(sizeof(GZtrailer)))
	{
		return false;
	}

	try
	{
		data.reserve(inFile.BytesLeft());
	} catch(...)
	{
		return false;
	}

	// Inflate!
	z_stream strm{};
	int zlib_errc = Z_OK;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	zlib_errc = inflateInit2(&strm, -15);
	if(zlib_errc == Z_MEM_ERROR)
	{
		return false;
	} else if(zlib_errc < Z_OK)
	{
		return false;
	}
	int retVal = Z_OK;
	uint32 crc = 0;
	auto bytesLeft = inFile.BytesLeft() - sizeof(GZtrailer);
	do
	{
		std::array<char, mpt::IO::BUFFERSIZE_SMALL> inBuffer, outBuffer;
		strm.avail_in = static_cast<uInt>(std::min(static_cast<FileReader::pos_type>(inBuffer.size()), bytesLeft));
		inFile.ReadStructPartial(inBuffer, strm.avail_in);
		strm.next_in = mpt::byte_cast<Bytef *>(inBuffer.data());
		bytesLeft -= strm.avail_in;
		do
		{
			strm.avail_out = static_cast<uInt>(outBuffer.size());
			strm.next_out = mpt::byte_cast<Bytef *>(outBuffer.data());
			zlib_errc = inflate(&strm, Z_NO_FLUSH);
			if(zlib_errc == Z_BUF_ERROR)
			{
				// expected
			} else if(zlib_errc == Z_MEM_ERROR)
			{
				inflateEnd(&strm);
				return false;
			} else if(zlib_errc < Z_OK)
			{
				inflateEnd(&strm);
				return false;
			}
			retVal = zlib_errc;
			const auto output = mpt::as_span(outBuffer.data(), outBuffer.data() + outBuffer.size() - strm.avail_out);
			crc = crc32(crc, mpt::byte_cast<Bytef *>(output.data()), static_cast<uInt>(output.size()));
			data.insert(data.end(), output.begin(), output.end());
		} while(strm.avail_out == 0);
	} while((retVal == Z_OK || retVal == Z_BUF_ERROR) && bytesLeft);
	inflateEnd(&strm);

	// Everything went OK? Check return code, number of written bytes and CRC32.
	return retVal == Z_STREAM_END && trailer.isize == static_cast<uint32>(strm.total_out) && trailer.crc32_ == crc;
}


#endif // MPT_WITH_ZLIB || MPT_WITH_MINIZ


OPENMPT_NAMESPACE_END
