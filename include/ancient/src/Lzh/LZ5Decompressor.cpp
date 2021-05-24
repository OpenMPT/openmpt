/* Copyright (C) Teemu Suutari */

#include "LZ5Decompressor.hpp"

#include "common/StaticBuffer.hpp"

#include "../InputStream.hpp"
#include "../OutputStream.hpp"


namespace ancient::internal
{

LZ5Decompressor::LZ5Decompressor(const Buffer &packedData) :
	_packedData(packedData)
{
	// nothing needed
}

LZ5Decompressor::~LZ5Decompressor()
{
	// nothing needed
}

size_t LZ5Decompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t LZ5Decompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &LZ5Decompressor::getName() const noexcept
{
	static std::string name="LHA: LZ5";
	return name;
}

void LZ5Decompressor::decompressImpl(Buffer &rawData,bool verify)
{
	ForwardInputStream inputStream(_packedData,0,_packedData.size());
	LSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBits8(1);
	};
	auto readByte=[&]()->uint32_t
	{
		return inputStream.readByte();
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());
	StaticBuffer<4096> prevBuffer;
	{
		uint8_t *bufPtr=prevBuffer.data();

		for (uint32_t i=0;i<18;i++) *(bufPtr++)=0;
		for (uint32_t i=0;i<256;i++)
			for (uint32_t j=0;j<13;j++) *(bufPtr++)=i;
		for (uint32_t i=0;i<256;i++) *(bufPtr++)=i;
		for (uint32_t i=0;i<256;i++) *(bufPtr++)=255-i;
		for (uint32_t i=0;i<128;i++) *(bufPtr++)=0;
		for (uint32_t i=0;i<110;i++) *(bufPtr++)=' ';
	}

	while (!outputStream.eof())
	{
		if (readBit())
		{
			outputStream.writeByte(readByte());
		} else {
			uint32_t byte1=readByte();
			uint32_t byte2=readByte();
			uint32_t distance=((outputStream.getOffset()-byte1-((byte2&0xf0U)<<4)-19)&0xfffU)+1;
			uint32_t count=(byte2&0xfU)+3;

			outputStream.copy(distance,count,prevBuffer);
		}
	}
}

}
