/* Copyright (C) Teemu Suutari */

#include "LH2Decompressor.hpp"

#include "../HuffmanDecoder.hpp"
#include "../DynamicHuffmanDecoder.hpp"
#include "../InputStream.hpp"
#include "../OutputStream.hpp"


namespace ancient::internal
{

LH2Decompressor::LH2Decompressor(const Buffer &packedData) :
	_packedData(packedData)
{
	// nothing needed
}

LH2Decompressor::~LH2Decompressor()
{
	// nothing needed
}

size_t LH2Decompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t LH2Decompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &LH2Decompressor::getName() const noexcept
{
	static std::string name="LHA: LH2";
	return name;
}

// This is probably fishiest of the fishy formats I've worked with
// Basically it uses Dynamic Huffman decoder but instead of static
// size, it is dynamically growing. However, that part is not well
// defined. Thus
// a. It is very hard to use LH2 using generic implementation
//    instead of the specific one used by LHA
// b. There are bugs in encoder which need to be baked in the decoder
//    as well (Probably there is lots of unneccesary stuff in addCode,
//    but better safe than sorry)
// c. LH 1.9x and UNLHA32 refuse to use LH2 beyond 8k files. Thus
//    we can only guess if the wraparound is correct, since nothing
//    should use it
// jLHA in theory supports LH2 without limitations, but it produces
// broken bitstream.
//
// So far this works with the files I've tried from "official" LHA
void LH2Decompressor::decompressImpl(Buffer &rawData,bool verify)
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

	DynamicHuffmanDecoder<286> valueDecoder;
	DynamicHuffmanDecoder<128> distanceDecoder(0);
	uint32_t distancePosition=0;

	uint32_t distCount=0;
	while (!outputStream.eof())
	{
		uint32_t code=valueDecoder.decode(readBit);
		if (valueDecoder.getMaxFrequency()==0x8000U) valueDecoder.halve();
		valueDecoder.update(code);

		if (code<256)
		{
			outputStream.writeByte(code);
		} else {
			if (code==285) code+=readBits(8);
			uint32_t count=code-253;

			// Bug in LH2 where count does not match frequency.
			// Makes things more complicated
			auto updateDist=[&](uint32_t code)
			{
				if (distCount==0x8000U)
				{
					distanceDecoder.halve();
					distCount=distanceDecoder.getMaxFrequency();
				}
				distanceDecoder.update(code);
				distCount++;
			};

			uint32_t maxDist=std::min(uint32_t((outputStream.getOffset()+63)>>6),128U);
			if (distancePosition!=maxDist)
			{
				for (uint32_t i=distancePosition;i<maxDist;i++)
				{
					distanceDecoder.addCode();
					updateDist(i);
				}
				distancePosition=maxDist;
			}

			uint32_t distance=distanceDecoder.decode(readBit);
			updateDist(distance);
			distance=(distance<<6)|readBits(6);
			distance++;

			outputStream.copy(distance,count,0x20);
		}
	}
}

}
