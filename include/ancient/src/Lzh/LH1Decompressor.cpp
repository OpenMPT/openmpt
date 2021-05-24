/* Copyright (C) Teemu Suutari */

#include "LH1Decompressor.hpp"

#include "../HuffmanDecoder.hpp"
#include "../DynamicHuffmanDecoder.hpp"
#include "../InputStream.hpp"
#include "../OutputStream.hpp"

namespace ancient::internal
{

LH1Decompressor::LH1Decompressor(const Buffer &packedData) :
	_packedData(packedData)
{
	// nothing needed
}

LH1Decompressor::~LH1Decompressor()
{
	// nothing needed
}

size_t LH1Decompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t LH1Decompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &LH1Decompressor::getName() const noexcept
{
	static std::string name="LHA: LH1";
	return name;
}

void LH1Decompressor::decompressImpl(Buffer &rawData,bool verify)
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

	DynamicHuffmanDecoder<314> decoder;

	static const uint8_t distanceHighBits[64]={
		3,4,4,4,5,5,5,5, 5,5,5,5,6,6,6,6,
		6,6,6,6,6,6,6,6, 7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8
	};

	HuffmanDecoder<uint8_t> distanceDecoder;
	distanceDecoder.createOrderlyHuffmanTable(distanceHighBits,64);

	while (!outputStream.eof())
	{
		uint32_t code=decoder.decode(readBit);
		if (decoder.getMaxFrequency()==0x8000U) decoder.halve();
		decoder.update(code);

		if (code<256)
		{
			outputStream.writeByte(code);
		} else {
			uint32_t distance=distanceDecoder.decode(readBit);
			distance=(distance<<6)|readBits(6);
			distance++;

			uint32_t count=code-253;

			outputStream.copy(distance,count,0x20);
		}
	}
}

}
