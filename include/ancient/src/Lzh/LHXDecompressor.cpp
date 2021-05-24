/* Copyright (C) Teemu Suutari */

#include "LHXDecompressor.hpp"

#include "../HuffmanDecoder.hpp"
#include "../InputStream.hpp"
#include "../OutputStream.hpp"


namespace ancient::internal
{

LHXDecompressor::LHXDecompressor(const Buffer &packedData) :
	_packedData(packedData),
	_method(0)
{
	// nothing needed
}

LHXDecompressor::LHXDecompressor(const Buffer &packedData,uint32_t method) :
	_packedData(packedData),
	_method(method-3)
{
	if (method<4 || method>8) throw InvalidFormatError();
}

LHXDecompressor::~LHXDecompressor()
{
	// nothing needed
}

size_t LHXDecompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t LHXDecompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &LHXDecompressor::getName() const noexcept
{
	static std::string name="LHA: LH4, LH5, LH6, LH7, LH8, LHX";
	return name;
}

void LHXDecompressor::decompressImpl(Buffer &rawData,bool verify)
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
	OptionalHuffmanDecoder<uint32_t> distanceDecoder;

	static const struct {
		uint32_t	mask;
		uint32_t	distanceTableSize;
		uint32_t	distanceBits;
	} methodTable[6] = {
		{0x7ffffU,20,5},	// LHX
		{0xfffU,14,4},		// LH4
		{0x1fffU,14,4},		// LH5
		{0x7fffU,16,5},		// LH6
		{0xffffU,17,5},		// LH7
		{0xffffU,17,5}		// LH8
		// LH8 is the only Joe Jared method seen.
		// It exists in early versions of LH7-tool
		// In those versions LH8 is just synonym for LH7
		// However, LH9+ is something I have not seen at
		// all (i.e. not in DLH7021Q/WLH7021Q.)
		// There would be one more version (0.21R) but
		// that seems lost. Most likely minor bump from Q->R
		// do not enable those formats either and the LH9+
		// formats are nothing but hopes for "future work"
		// that never materialized (until I'm proven wrong ofc)
	};

	uint32_t blockRemaining=0;
	while (!outputStream.eof())
	{
		if (!blockRemaining)
		{
			blockRemaining=readBits(16);
			if (!blockRemaining) blockRemaining=0x10000;

			auto createTable=[&](OptionalHuffmanDecoder<uint32_t> &dest,uint32_t count,uint32_t bits,bool enableHole)
			{
				uint8_t symbolBits[20];
				uint32_t length=readBits(bits);
				if (!length)
				{
					dest.setEmpty(readBits(bits));
				} else if (length<=count) {
					for (uint32_t i=0;i<length;)
					{
						uint32_t value=readBits(3);
						if (value==7)
							while (readBit()) value++;
						if (value>32) throw DecompressionError();
						symbolBits[i++]=value;
						if (i==3 && enableHole)
						{
							uint32_t zeros=readBits(2);
							if (i+zeros>length) throw DecompressionError();
							for (uint32_t j=0;j<zeros;j++) symbolBits[i++]=0;
						}
					}
					dest.createOrderlyHuffmanTable(symbolBits,length);
				} else throw DecompressionError();
			};

			OptionalHuffmanDecoder<uint32_t> tmpDecoder;
			createTable(tmpDecoder,19,5,true);

			decoder.reset();

			uint8_t symbolBits[511];
			uint32_t length=readBits(9);
			if (!length)
			{
				decoder.setEmpty(readBits(9));
			} else {
				for (uint32_t i=0;i<length;)
				{
					uint32_t value=tmpDecoder.decode(readBit);
					uint32_t rep;
					switch (value)
					{
						case 0:
						value=0;
						rep=1;
						break;

						case 1:
						value=0;
						rep=readBits(4)+3;
						break;

						case 2:
						value=0;
						rep=readBits(9)+20;
						break;

						default:
						value-=2;
						rep=1;
						break;
					}
					if (i+rep>length) throw DecompressionError();
					for (uint32_t j=0;j<rep;j++) symbolBits[i++]=value;
				}
				decoder.createOrderlyHuffmanTable(symbolBits,length);
			}

			distanceDecoder.reset();
			createTable(distanceDecoder,methodTable[_method].distanceTableSize,methodTable[_method].distanceBits,false);
		}
		blockRemaining--;

		uint32_t code=decoder.decode(readBit);

		if (code<256)
		{
			outputStream.writeByte(code);
		} else {
			uint32_t distanceBits=distanceDecoder.decode(readBit);
			uint32_t distance=distanceBits?((1<<(distanceBits-1))|readBits(distanceBits-1)):0;
			distance&=methodTable[_method].mask;		// this is theoretical, but original LH has it
			distance++;

			uint32_t count=code-253;

			outputStream.copy(distance,count,0x20);
		}
	}
}

}
