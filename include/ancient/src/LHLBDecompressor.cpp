/* Copyright (C) Teemu Suutari */

#include "LHLBDecompressor.hpp"

#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "DynamicHuffmanDecoder.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool LHLBDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("LHLB");
}

std::unique_ptr<XPKDecompressor> LHLBDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<LHLBDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

LHLBDecompressor::LHLBDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

LHLBDecompressor::~LHLBDecompressor()
{
	// nothing needed
}

const std::string &LHLBDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-LHLB: LZRW-compressor";
	return name;
}

void LHLBDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
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

	// Same logic as in Choloks pascal implementation
	// Differences to LH1:
	// - LHLB does not halve probabilities at 32k
	// - 314 vs. 317 sized huffman entry
	// - no end code
	// - different distance/count logic

	DynamicHuffmanDecoder<317> decoder;

	while (!outputStream.eof())
	{
		uint32_t code=decoder.decode(readBit);
		if (code==316) break;
		if (decoder.getMaxFrequency()<0x8000U) decoder.update(code);

		if (code<256)
		{
			outputStream.writeByte(code);
		} else {
			static const uint8_t distanceHighBits[256]={
				 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
				 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
				 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2,
				 3, 3, 3, 3, 3, 3, 3, 3,  3, 3, 3, 3, 3, 3, 3, 3,
				 4, 4, 4, 4, 4, 4, 4, 4,  5, 5, 5, 5, 5, 5, 5, 5,
				 6, 6, 6, 6, 6, 6, 6, 6,  7, 7, 7, 7, 7, 7, 7, 7,
				 8, 8, 8, 8, 8, 8, 8, 8,  9, 9, 9, 9, 9, 9, 9, 9,

				10,10,10,10,10,10,10,10, 11,11,11,11,11,11,11,11,
				12,12,12,12,13,13,13,13, 14,14,14,14,15,15,15,15,
				16,16,16,16,17,17,17,17, 18,18,18,18,19,19,19,19,
				20,20,20,20,21,21,21,21, 22,22,22,22,23,23,23,23,
				24,24,25,25,26,26,27,27, 28,28,29,29,30,30,31,31,
				32,32,33,33,34,34,35,35, 36,36,37,37,38,38,39,39,
				40,40,41,41,42,42,43,43, 44,44,45,45,46,46,47,47,
				48,49,50,51,52,53,54,55, 56,57,58,59,60,61,62,63};
			static const uint8_t distanceBits[16]={1,1,2,2,2,3,3,3,3,4,4,4,5,5,5,6};
				
			uint32_t tmp=readBits(8);
			uint32_t distance=uint32_t(distanceHighBits[tmp])<<6;
			uint32_t bits=distanceBits[tmp>>4];
			tmp=(tmp<<bits)|readBits(bits);
			distance|=tmp&63;
			uint32_t count=code-255;

			outputStream.copy(distance,count);
		}
	}
}

XPKDecompressor::Registry<LHLBDecompressor> LHLBDecompressor::_XPKregistration;

}
