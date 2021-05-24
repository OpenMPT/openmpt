/* Copyright (C) Teemu Suutari */

#include "LH3Decompressor.hpp"

#include "../HuffmanDecoder.hpp"
#include "../InputStream.hpp"
#include "../OutputStream.hpp"


namespace ancient::internal
{

LH3Decompressor::LH3Decompressor(const Buffer &packedData) :
	_packedData(packedData)
{
	// nothing needed
}

LH3Decompressor::~LH3Decompressor()
{
	// nothing needed
}

size_t LH3Decompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t LH3Decompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &LH3Decompressor::getName() const noexcept
{
	static std::string name="LHA: LH3";
	return name;
}

void LH3Decompressor::decompressImpl(Buffer &rawData,bool verify)
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

	OptionalHuffmanDecoder<uint32_t> decoder;

	static const uint8_t distanceHighBits[128]={
		2,4,4,5,5,5,6,6, 6,6,6,6,6,7,7,7,
		7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,8,
		8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,

		8,8,8,8,8,8,8,8, 8,8,8,8,8,8,9,9,
		9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,
		9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,
		9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9
	};

	OptionalHuffmanDecoder<uint8_t> distanceDecoder;

	uint32_t blockRemaining=0;
	while (!outputStream.eof())
	{
		if (!blockRemaining)
		{
			blockRemaining=readBits(16);
			if (!blockRemaining) blockRemaining=0x10000;

			// not strictly needed as a lambda, but cleaner this way
			auto createDecoderTable=[&]()
			{
				decoder.reset();

				uint8_t symbolBits[286];
				uint32_t oneCount=0;
				for (uint32_t i=0;i<286;i++)
				{
					if (readBit()) symbolBits[i]=readBits(4)+1;
						else symbolBits[i]=0;
					if (symbolBits[i]==1) oneCount++;
					if (i==2 && oneCount==3)
					{
						decoder.setEmpty(readBits(9));
						return;
					}
				}
				decoder.createOrderlyHuffmanTable(symbolBits,286);
			};

			// ditto
			auto createDistanceDecoderTable=[&]()
			{
				distanceDecoder.reset();

				if (readBit())
				{
					uint8_t symbolBits[128];
					uint32_t oneCount=0;
					for (uint32_t i=0;i<128;i++)
					{
						symbolBits[i]=readBits(4);
						if (symbolBits[i]==1) oneCount++;
						if (i==2 && oneCount==3)
						{
							// 7 bits would be fine, but whatever
							distanceDecoder.setEmpty(readBits(9));
							return;
						}
					}
					distanceDecoder.createOrderlyHuffmanTable(symbolBits,128);
				} else distanceDecoder.createOrderlyHuffmanTable(distanceHighBits,128);
			};

			createDecoderTable();
			createDistanceDecoderTable();
		}
		blockRemaining--;

		uint32_t code=decoder.decode(readBit);

		if (code<256)
		{
			outputStream.writeByte(code);
		} else {
			if (code==285) code+=readBits(8);
			uint32_t distance=distanceDecoder.decode(readBit);
			distance=(distance<<6)|readBits(6);
			distance++;

			uint32_t count=code-253;

			outputStream.copy(distance,count,0x20);
		}
	}
}

}
