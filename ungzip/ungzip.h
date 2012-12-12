/*
 * ungzip.h
 * --------
 * Purpose: Header file for .gz loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

//================
class CGzipArchive
//================
{
protected:
	FileReader inFile, outFile;

#pragma pack(push, 1)

	struct GZheader
	{
		uint8  magic1;	// 0x1F
		uint8  magic2;	// 0x8B
		uint8  method;	// 0-7 = reserved, 8 = deflate
		uint8  flags;	// See GZ_F* constants
		uint32 mtime;	// UNIX time
		uint8  xflags;	// Available for use by specific compression methods. We ignore this.
		uint8  os;		// Which OS was used to compress the file? We also ignore this.
	};

	struct GZtrailer
	{
		uint32 crc32;	// CRC32 of decompressed data
		uint32 isize;	// Size of decompressed data

		void ConvertEndianness()
		{
			SwapBytesLE(crc32);
			SwapBytesLE(isize);
		}
	};

#pragma pack(pop)

	enum MagicBytes
	{
		GZ_HMAGIC1		= 0x1F,
		GZ_HMAGIC2		= 0x8B,
		GZ_HMDEFLATE	= 0x08,
	};

	enum HeaderFlags
	{
		GZ_FTEXT		= 0x01,	// File is probably ASCII text (who cares)
		GZ_FHCRC		= 0x02,	// CRC16 present
		GZ_FEXTRA		= 0x04,	// Extra fields present
		GZ_FNAME		= 0x08,	// Original filename present
		GZ_FCOMMENT		= 0x10,	// Comment is present
		GZ_FRESERVED	= (~(GZ_FTEXT | GZ_FHCRC | GZ_FEXTRA | GZ_FNAME | GZ_FCOMMENT))
	};

	GZheader header;

public:

	FileReader GetOutputFile() const { return outFile; }
	bool IsArchive() const;
	bool ExtractFile();

	CGzipArchive(FileReader &file);
	~CGzipArchive();
};
