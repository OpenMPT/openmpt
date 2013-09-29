/*
 * ungzip.cpp
 * ----------
 * Purpose: Implementation file for extracting modules from .gz archives
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "../soundlib/FileReader.h"
#include "ungzip.h"

#include <zlib/zlib.h>


CGzipArchive::CGzipArchive(FileReader &file) : inFile(file)
//---------------------------------------------------------
{
	inFile.Rewind();
	inFile.Read(header);
}


CGzipArchive::~CGzipArchive()
//---------------------------
{
	delete[] outFile.GetRawData();
}


bool CGzipArchive::IsArchive() const
//----------------------------------
{
	// Check header data + file size
	if(header.magic1 != GZ_HMAGIC1 || header.magic2 != GZ_HMAGIC2 || header.method != GZ_HMDEFLATE || (header.flags & GZ_FRESERVED) != 0
		|| inFile.GetLength() <= sizeof(GZheader) + sizeof(GZtrailer))
	{
		return false;
	}
	return true;
}


bool CGzipArchive::ExtractFile()
//------------------------------
{
	if(!IsArchive())
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

	delete[] outFile.GetRawData();

	char *data = new (std::nothrow) char[trailer.isize];
	if(data == nullptr)
	{
		return false;
	}

	// Inflate!
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = inFile.BytesLeft() - sizeof(GZtrailer);
	strm.next_in = (Bytef *)(inFile.GetRawData());
	if(inflateInit2(&strm, -15) != Z_OK)
	{
		return false;
	}
	strm.avail_out = trailer.isize;
	strm.next_out = (Bytef *)data;

	int retVal = inflate(&strm, Z_NO_FLUSH);
	inflateEnd(&strm);

	// Everything went OK? Check return code, number of written bytes and CRC32.
	if(retVal == Z_STREAM_END && trailer.isize == strm.total_out && trailer.crc32 == crc32(0, (Bytef *)data, trailer.isize))
	{
		// Success! :)
		outFile = FileReader(data, trailer.isize);
		return true;
	} else
	{
		// Fail :(
		delete[] outFile.GetRawData();
		return false;
	}
}
