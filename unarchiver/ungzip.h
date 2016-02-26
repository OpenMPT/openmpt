/*
 * ungzip.h
 * --------
 * Purpose: Header file for .gz loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "archive.h"

OPENMPT_NAMESPACE_BEGIN

#if defined(MPT_WITH_ZLIB) || defined(MPT_WITH_MINIZ)

//=====================================
class CGzipArchive : public ArchiveBase
//=====================================
{
protected:

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

	struct PACKED GZheader
	{
		uint8  magic1;	// 0x1F
		uint8  magic2;	// 0x8B
		uint8  method;	// 0-7 = reserved, 8 = deflate
		uint8  flags;	// See GZ_F* constants
		uint32 mtime;	// UNIX time
		uint8  xflags;	// Available for use by specific compression methods. We ignore this.
		uint8  os;		// Which OS was used to compress the file? We also ignore this.

		void ConvertEndianness()
		{
			SwapBytesLE(mtime);
		}
	};

	STATIC_ASSERT(sizeof(GZheader) == 10);

	struct PACKED GZtrailer
	{
		uint32 crc32_;	// CRC32 of decompressed data
		uint32 isize;	// Size of decompressed data

		void ConvertEndianness()
		{
			SwapBytesLE(crc32_);
			SwapBytesLE(isize);
		}
	};

	STATIC_ASSERT(sizeof(GZtrailer) == 8);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif

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

	bool ExtractFile(std::size_t index);

	CGzipArchive(FileReader &file);
	virtual ~CGzipArchive();
};

#endif // MPT_WITH_ZLIB || MPT_WITH_MINIZ

OPENMPT_NAMESPACE_END
