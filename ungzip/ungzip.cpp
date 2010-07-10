/*
 * ungzip.cpp
 * ----------
 * Purpose: Implementation file for extracting modules from .gz archives
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */

#include "ungzip.h"
#include "../soundlib/Endianness.h"


CGzipArchive::CGzipArchive(LPBYTE lpStream, DWORD dwMemLength)
//------------------------------------------------------------
{
	m_lpStream = lpStream;
	m_dwStreamLen = dwMemLength;
	m_pOutputFile = nullptr;
	m_dwOutputLen = 0;
}


CGzipArchive::~CGzipArchive()
//---------------------------
{
	if(m_pOutputFile != nullptr)
	{
		delete[] m_pOutputFile;
	}
}


bool CGzipArchive::IsArchive() const
//----------------------------------
{
	if(m_lpStream == nullptr || m_dwStreamLen <= (sizeof(GZheader) + sizeof(GZtrailer)))
		return false;

	// Check header data
	GZheader *pHeader = (GZheader *)m_lpStream;
	if(pHeader->magic1 != GZ_HMAGIC1 || pHeader->magic2 != GZ_HMAGIC2 || pHeader->method != GZ_HMDEFLATE || (pHeader->flags & GZ_FRESERVED) != 0)
		return false;

	// Seems to be OK...
	return true;
}


bool CGzipArchive::ExtractFile()
//------------------------------
{
	#define ASSERT_CAN_READ(x) \
	if( dwMemPos > m_dwStreamLen || x > m_dwStreamLen - dwMemPos ) return false;

	if(!IsArchive())
		return false;

	DWORD dwMemPos = 0;
	GZheader *pHeader = (GZheader *)m_lpStream;
	GZtrailer *pTrailer = (GZtrailer *)(m_lpStream + m_dwStreamLen - sizeof(GZtrailer));

	dwMemPos += sizeof(GZheader);

	// Extra block present? (ignore)
	if(pHeader->flags & GZ_FEXTRA)
	{
		ASSERT_CAN_READ(sizeof(uint16));
		uint16 xlen = LittleEndianW(*((uint16 *)m_lpStream + dwMemPos));
		dwMemPos += sizeof(uint16);
		// We skip this.
		ASSERT_CAN_READ(xlen);
		dwMemPos += xlen;
	}

	// Filename present? (ignore)
	if(pHeader->flags & GZ_FNAME)
	{
		do 
		{
			ASSERT_CAN_READ(1);
		} while (m_lpStream[dwMemPos++] != 0);
	}

	// Comment present? (ignore)
	if(pHeader->flags & GZ_FCOMMENT)
	{
		do 
		{
			ASSERT_CAN_READ(1);
		} while (m_lpStream[dwMemPos++] != 0);
	}

	// CRC16 present?
	if(pHeader->flags & GZ_FHCRC)
	{
		ASSERT_CAN_READ(sizeof(uint16));
		uint16 crc16_h = LittleEndianW(*((uint16 *)m_lpStream + dwMemPos));
		uint16 crc16_f = (uint16)(crc32(0, m_lpStream, dwMemPos) & 0xFFFF);
		dwMemPos += sizeof(uint16);
		if(crc16_h != crc16_f)
			return false;
	}

	// Well, this is a bit small when inflated.
	if(pTrailer->isize == 0)
		return false;

	// Check if the deflated data is a bit small as well.
	ASSERT_CAN_READ(sizeof(GZtrailer) + 1);

	// Clear the output buffer, if necessary.
	if(m_pOutputFile != nullptr)
	{
		delete[] m_pOutputFile;
	}

	DWORD destSize = LittleEndian(pTrailer->isize);

	m_pOutputFile = new Bytef[destSize];
	if(m_pOutputFile == nullptr)
		return false;

	// Inflate!
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = m_dwStreamLen - (dwMemPos + sizeof(GZtrailer));
	strm.next_in = &m_lpStream[dwMemPos];
	if(inflateInit2(&strm, -15) != Z_OK)
		return false;
	strm.avail_out = destSize;
	strm.next_out = m_pOutputFile;

	int nRetVal = inflate(&strm, Z_NO_FLUSH);
	inflateEnd(&strm);

	// Everything went OK? Check return code, number of written bytes and CRC32.
	if(nRetVal == Z_STREAM_END && destSize == strm.total_out && LittleEndian(pTrailer->crc32) == crc32(0, m_pOutputFile, destSize))
	{
		// Success! :)
		m_dwOutputLen = destSize;
		return true;
	} else
	{
		// Fail :(
		if(m_pOutputFile != nullptr)
		{
			delete[] m_pOutputFile;
		}
		m_pOutputFile = nullptr;
		m_lpStream = nullptr;
		return false;
	}

	#undef ASSERT_CAN_READ
}
