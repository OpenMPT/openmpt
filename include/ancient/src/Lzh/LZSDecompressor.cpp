/* Copyright (C) Teemu Suutari */

#include "LZSDecompressor.hpp"

#include "../InputStream.hpp"
#include "../OutputStream.hpp"


namespace ancient::internal
{

LZSDecompressor::LZSDecompressor(const Buffer &packedData) :
	_packedData(packedData)
{
	// nothing needed
}

LZSDecompressor::~LZSDecompressor()
{
	// nothing needed
}

size_t LZSDecompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t LZSDecompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &LZSDecompressor::getName() const noexcept
{
	static std::string name="LHA: LZS";
	return name;
}

void LZSDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	ForwardInputStream inputStream(_packedData,0,_packedData.size());
	MSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBits8(1);
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());

	while (!outputStream.eof())
	{
		if (readBit())
		{
			outputStream.writeByte(readBits(8));
		} else {
			uint32_t distance=((outputStream.getOffset()-readBits(11)-18)&0x7ffU)+1;
			uint32_t count=readBits(4)+2;

			outputStream.copy(distance,count,0x20);
		}
	}
}

}
