/* Copyright (C) Teemu Suutari */

#include "TDCSDecompressor.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool TDCSDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("TDCS");
}

std::unique_ptr<XPKDecompressor> TDCSDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<TDCSDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

TDCSDecompressor::TDCSDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

TDCSDecompressor::~TDCSDecompressor()
{
	// nothing needed
}

const std::string &TDCSDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-TDCS: LZ77-compressor";
	return name;
}

void TDCSDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	ForwardInputStream inputStream(_packedData,0,_packedData.size());
	MSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto read2Bits=[&]()->uint32_t
	{
		return bitReader.readBitsBE32(2);
	};
	auto readByte=[&]()->uint8_t
	{
		return inputStream.readByte();
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());

	while (!outputStream.eof())
	{
		uint32_t distance=0;
		uint32_t count=0;
		uint32_t tmp;
		switch (read2Bits())
		{
			case 0:
			outputStream.writeByte(readByte());
			break;

			case 1:
			tmp=uint32_t(readByte())<<8;
			tmp|=uint32_t(readByte());
			count=(tmp&3)+3;
			distance=((tmp>>2)^0x3fff)+1;
			break;

			case 2:
			tmp=uint32_t(readByte())<<8;
			tmp|=uint32_t(readByte());
			count=(tmp&0xf)+3;
			distance=((tmp>>4)^0xfff)+1;
			break;

			case 3:
			distance=uint32_t(readByte())<<8;
			distance|=uint32_t(readByte());
			count=uint32_t(readByte())+3;
			if (!distance) throw Decompressor::DecompressionError();
			distance=(distance^0xffff)+1;
			break;
			
			default:
			throw Decompressor::DecompressionError();
		}
		if (count && distance)
			outputStream.copy(distance,count);
	}
}

XPKDecompressor::Registry<TDCSDecompressor> TDCSDecompressor::_XPKregistration;

}
