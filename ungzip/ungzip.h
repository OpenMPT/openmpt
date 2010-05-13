/*
 * ungzip.h
 * --------
 * Purpose: Header file for .gz loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */

#pragma once

#ifndef ZLIB_WINAPI
#define ZLIB_WINAPI
#endif // ZLIB_WINAPI
#include "../zlib/zlib.h"
#include "../mptrack/typedefs.h"

#pragma pack(1)

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
};

#pragma pack()

// magic bytes
#define GZ_HMAGIC1		0x1F
#define GZ_HMAGIC2		0x8B
#define GZ_HMDEFLATE	0x08
// header flags
#define GZ_FTEXT		0x01	// File is probably ASCII text (who cares)
#define GZ_FHCRC		0x02	// CRC16 present
#define GZ_FEXTRA		0x04	// Extra fields present
#define GZ_FNAME		0x08	// Original filename present
#define GZ_FCOMMENT		0x10	// Comment is present
#define GZ_FRESERVED	(~(GZ_FTEXT | GZ_FHCRC | GZ_FEXTRA | GZ_FNAME | GZ_FCOMMENT))


//================
class CGzipArchive
//================
{
public:

	LPBYTE GetOutputFile() const { return m_pOutputFile; }
	DWORD GetOutputFileLength() const { return m_dwOutputLen; }
	bool IsArchive();
	bool ExtractFile();

	CGzipArchive(LPBYTE lpStream, DWORD dwMemLength);
	~CGzipArchive();

protected:
	// in
	LPBYTE m_lpStream;
	DWORD m_dwStreamLen;
	// out
	Bytef *m_pOutputFile;
	DWORD m_dwOutputLen;
};
