/* Copyright (C) Teemu Suutari */

#include <cstdint>
#include <cstring>

#include "ReduceDecompressor.hpp"
#include "../InputStream.hpp"
#include "../OutputStream.hpp"


namespace ancient::internal
{

ReduceDecompressor::ReduceDecompressor(const Buffer &packedData,uint32_t mode) :
	_packedData(packedData),
	_mode(mode)
{
	if (mode<2 || mode>5) throw InvalidFormatError();
}

ReduceDecompressor::~ReduceDecompressor()
{
	// nothing needed
}

size_t ReduceDecompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t ReduceDecompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &ReduceDecompressor::getName() const noexcept
{
	static std::string name="Zip: Reduce";
	return name;
}

void ReduceDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	ForwardInputStream inputStream(_packedData,0,_packedData.size());
	LSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());

	uint8_t SLengths[256];
	auto SBuf=std::make_unique<uint8_t[]>(256*64);
	uint8_t (&S)[256][64]=*reinterpret_cast<uint8_t(*)[256][64]>(SBuf.get());

	static const uint8_t bitLengths[64]={
		8,1,1,2,2,3,3,3, 3,4,4,4,4,4,4,4,
		4,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,
		5,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6};

	for (int32_t i=255;i>=0;i--)
	{
		uint32_t length=SLengths[i]=readBits(6);
		for (uint32_t j=0;j<length;j++) S[i][j]=readBits(8);
	}

	uint8_t lengthBits=9U-_mode;

	uint8_t lastChar=0;
	while (!outputStream.eof())
	{
		auto readByte=[&]()->uint8_t
			{
			uint8_t ret;
			if (!SLengths[lastChar] || readBits(1))
			{
				ret=readBits(8);
			} else {
				uint8_t tmp=readBits(bitLengths[SLengths[lastChar]]);
				if (tmp>=SLengths[lastChar]) throw DecompressionError();
				ret=S[lastChar][tmp];
			}
			lastChar=ret;
			return ret;
		};

		uint8_t ch=readByte();
		if (ch!=0x90U)
		{
			outputStream.writeByte(ch);
		} else {
			ch=readByte();
			if (!ch)
			{
				outputStream.writeByte(0x90U);
			} else {
				uint8_t lengthMask=0xffU>>(8U-lengthBits);
				uint32_t count=ch&lengthMask;
				if (count==lengthMask)
					count+=uint32_t(readByte());
				count+=3;
				uint32_t distance=(uint32_t(ch>>lengthBits)<<8)|uint32_t(readByte());
				distance++;

				outputStream.copy(distance,count,0);
			}
		}
	}
}

}
