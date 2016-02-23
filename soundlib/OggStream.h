/*
 * OggStream.h
 * -----------
 * Purpose: Basic Ogg stream parsing functionality
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "../common/Endianness.h"
#include "../common/mptIO.h"


OPENMPT_NAMESPACE_BEGIN


class FileReader;


namespace Ogg
{


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED PageHeader
{
	char   capture_pattern[4]; // "OggS"
	uint8  version;
	uint8  header_type;
	uint64 granule_position;
	uint32 bitstream_serial_number;
	uint32 page_seqauence_number;
	uint32 CRC_checksum;
	uint8  page_segments;

	void ConvertEndianness()
	{
		SwapBytesLE(granule_position);
		SwapBytesLE(bitstream_serial_number);
		SwapBytesLE(page_seqauence_number);
		SwapBytesLE(CRC_checksum);
	}

};

STATIC_ASSERT(sizeof(PageHeader) == 27);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


struct PageInfo
{
	PageHeader header;
	uint8 segment_table[255];
	PageInfo()
	{
		MemsetZero(header);
		MemsetZero(segment_table);
	}
	uint16 GetPagePhysicalSize() const;
	uint16 GetPageDataSize() const;
};


// returns false on EOF
bool AdvanceToPageMagic(FileReader &file);

bool ReadPage(FileReader &file, PageInfo &pageInfo, std::vector<uint8> &pageData);

bool ReadPageAndSkipJunk(FileReader &file, PageInfo &pageInfo, std::vector<uint8> &pageData);


bool UpdatePageCRC(PageInfo &pageInfo, const std::vector<uint8> &pageData);


template <typename Tfile>
bool WritePage(Tfile & f, const PageInfo &pageInfo, const std::vector<uint8> &pageData)
{
	if(!mpt::IO::WriteConvertEndianness(f, pageInfo.header))
	{
		return false;
	}
	if(!mpt::IO::WriteRaw(f, pageInfo.segment_table, pageInfo.header.page_segments))
	{
		return false;
	}
	if(!mpt::IO::WriteRaw(f, &pageData[0], pageData.size()))
	{
		return false;
	}
	return true;
}


} // namespace Ogg


OPENMPT_NAMESPACE_END
