/* Copyright (C) Teemu Suutari */

#include <cstdint>
#include <cstring>

#include "ImplodeDecompressor.hpp"
#include "../InputStream.hpp"
#include "../OutputStream.hpp"
#include "../HuffmanDecoder.hpp"


namespace ancient::internal
{

ImplodeDecompressor::ImplodeDecompressor(const Buffer &packedData,uint32_t flags) :
	_packedData(packedData),
	_flags(flags)
{

}

ImplodeDecompressor::~ImplodeDecompressor()
{
	// nothing needed
}

size_t ImplodeDecompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t ImplodeDecompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &ImplodeDecompressor::getName() const noexcept
{
	static std::string name="Zip: Implode";
	return name;
}

void ImplodeDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	ForwardInputStream inputStream(_packedData,0,_packedData.size());
	LSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBits8(1);
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());

	// Although Implode uses Fannon-Shano, Huffman decoder can be used
	typedef HuffmanDecoder<uint8_t> ImplodeHuffmanDecoder;

	auto createDecoder=[&](ImplodeHuffmanDecoder &dec,uint32_t maxValue)
	{
		uint32_t length=readBits(8)+1;
		uint8_t bitLengths[257];
		uint8_t counts[257];
		uint8_t offsets[257];
		uint8_t minDepth=32,maxDepth=0;

		for (uint32_t i=0;i<length;i++)
		{
			bitLengths[i]=readBits(4)+1;
			counts[i]=readBits(4)+1;
			offsets[i]=i?offsets[i-1]+counts[i-1]:0;
			if (bitLengths[i]<minDepth) minDepth=bitLengths[i];
			if (bitLengths[i]>maxDepth) maxDepth=bitLengths[i];
		}
		if (static_cast<uint32_t>(offsets[length-1]+counts[length-1])>maxValue) throw DecompressionError();

		uint32_t code=0;
		for (uint32_t depth=maxDepth;depth>=minDepth;depth--)
		{
			for (int32_t i=length-1;i>=0;i--)
			{
				if (depth!=bitLengths[i]) continue;
				for (uint32_t j=counts[i];j>0;j--)
				{
					dec.insert(HuffmanCode<uint8_t>{depth,code>>(maxDepth-depth),uint8_t(offsets[i]+j-1)});
					code+=1<<(maxDepth-depth);
				}
			}
		}
	};

	ImplodeHuffmanDecoder lengthDecoder,distanceDecoder,litDecoder;

	uint32_t extraDistanceBits=(_flags&2)?7:6;
	bool literalDecPresent=_flags&4;
	uint32_t lengthAddition=2;

	if (literalDecPresent)
	{
		createDecoder(litDecoder,256);
		lengthAddition++;
	}
	createDecoder(lengthDecoder,64);
	createDecoder(distanceDecoder,64);

	while (!outputStream.eof())
	{
		if (readBit())
		{
			if (literalDecPresent) outputStream.writeByte(litDecoder.decode(readBit));
				else outputStream.writeByte(readBits(8));
		} else {
			uint32_t distance=readBits(extraDistanceBits);
			distance|=uint32_t(distanceDecoder.decode(readBit))<<extraDistanceBits;
			distance++;

			uint32_t count=lengthDecoder.decode(readBit);
			if (count==63) count+=readBits(8);
			count+=lengthAddition;

			outputStream.copy(distance,count,0);
		}
	}
}

}
