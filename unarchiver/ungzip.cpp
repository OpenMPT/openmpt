/*
 * ungzip.cpp
 * ----------
 * Purpose: Implementation file for extracting modules from .gz archives
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "../common/FileReader.h"
#include "ungzip.h"

#if defined(MPT_WITH_ZLIB) || defined(MPT_WITH_MINIZ)

#if defined(MPT_WITH_ZLIB)
#if MPT_COMPILER_MSVC
#include <zlib/zlib.h>
#else
#include <zlib.h>
#endif
#elif defined(MPT_WITH_MINIZ)
#define MINIZ_HEADER_FILE_ONLY
#include <miniz/miniz.c>
#endif

#endif // MPT_WITH_ZLIB || MPT_WITH_MINIZ


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_WITH_ZLIB) || defined(MPT_WITH_MINIZ)

	
CGzipArchive::CGzipArchive(FileReader &file) : ArchiveBase(file)
//--------------------------------------------------------------
{
	inFile.Rewind();
	inFile.ReadConvertEndianness(header);

	// Check header data + file size
	if(header.magic1 != GZ_HMAGIC1 || header.magic2 != GZ_HMAGIC2 || header.method != GZ_HMDEFLATE || (header.flags & GZ_FRESERVED) != 0
		|| inFile.GetLength() <= sizeof(GZheader) + sizeof(GZtrailer))
	{
		return;
	}
	ArchiveFileInfo info;
	info.type = ArchiveFileNormal;
	contents.push_back(info);
}


CGzipArchive::~CGzipArchive()
//---------------------------
{
	return;
}


bool CGzipArchive::ExtractFile(std::size_t index)
//-----------------------------------------------
{
	if(index >= contents.size())
	{
		return false;
	}

	// Read trailer
	GZtrailer trailer;
	inFile.Seek(inFile.GetLength() - sizeof(GZtrailer));
	inFile.ReadConvertEndianness(trailer);

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

	// Well, this is a bit small when inflated / deflated.
	if(trailer.isize == 0 || !inFile.CanRead(sizeof(GZtrailer)))
	{
		return false;
	}

	try
	{
		data.resize(trailer.isize);
	} catch(...)
	{
		return false;
	}

	// Inflate!
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = mpt::saturate_cast<uInt>(inFile.BytesLeft() - sizeof(GZtrailer));
	strm.next_in = (Bytef *)(inFile.GetRawData());
	if(inflateInit2(&strm, -15) != Z_OK)
	{
		return false;
	}
	strm.avail_out = trailer.isize;
	strm.next_out = (Bytef *)&data[0];

	int retVal = inflate(&strm, Z_NO_FLUSH);
	inflateEnd(&strm);

	// Everything went OK? Check return code, number of written bytes and CRC32.
	return (retVal == Z_STREAM_END && trailer.isize == strm.total_out && trailer.crc32_ == crc32(0, (Bytef *)&data[0], trailer.isize));
}


#endif // MPT_WITH_ZLIB || MPT_WITH_MINIZ


OPENMPT_NAMESPACE_END
