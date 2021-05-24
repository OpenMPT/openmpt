/* Copyright (C) Teemu Suutari */

#include <cstdint>
#include <cstring>

#include "LZXDecompressor.hpp"
#include "HuffmanDecoder.hpp"
#include "DLTADecode.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/CRC32.hpp"
#include "common/Common.hpp"
#include "common/OverflowCheck.hpp"


namespace ancient::internal
{

bool LZXDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("ELZX") || hdr==FourCC("SLZX");
}

std::unique_ptr<XPKDecompressor> LZXDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<LZXDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

LZXDecompressor::LZXDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
	if (hdr==FourCC("SLZX")) _isSampled=true;
	// There is no good spec on the LZX header content -> lots of unknowns here
	if (_packedData.size()<41) throw Decompressor::InvalidFormatError();
	// XPK LZX compression is embedded single file of LZX -> read first file. Ignore rest
	// this will include flags, which need to be zero anyway
	uint32_t streamHdr=_packedData.readBE32(0);
	if (streamHdr!=FourCC("LZX\0")) throw Decompressor::InvalidFormatError();

	_rawSize=_packedData.readLE32(12);
	_packedSize=_packedData.readLE32(16);

	_rawCRC=_packedData.readLE32(32);
	uint32_t headerCRC=_packedData.readLE32(36);

	uint8_t tmp=_packedData.read8(21);
	if (tmp && tmp!=2) throw Decompressor::InvalidFormatError();
	if (tmp==2) _isCompressed=true;

	_packedOffset=41U+_packedData.read8(40U);
	_packedOffset+=_packedData.read8(24U);
	_packedSize+=_packedOffset;

	if (_packedSize>_packedData.size()) throw Decompressor::InvalidFormatError();
	if (verify)
	{
		uint32_t crc=CRC32(_packedData,10,26,0);
		for (uint32_t i=0;i<4;i++) crc=CRC32Byte(0,crc);
		crc=CRC32(_packedData,40,_packedOffset-40,crc);
		if (crc!=headerCRC) throw Decompressor::VerificationError(); 
	}
}

LZXDecompressor::~LZXDecompressor()
{
	// nothing needed
}

const std::string &LZXDecompressor::getSubName() const noexcept
{
	static std::string nameE="XPK-ELZX: LZX-compressor";
	static std::string nameS="XPK-SLZX: LZX-compressor with delta encoding";
	return (_isSampled)?nameS:nameE;
}

void LZXDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	if (rawData.size()!=_rawSize) throw Decompressor::DecompressionError();
	if (!_isCompressed)
	{
		if (_packedSize!=_rawSize) throw Decompressor::DecompressionError();
		std::memcpy(rawData.data(),_packedData.data()+_packedOffset,_rawSize);
		return;
	}

	ForwardInputStream inputStream(_packedData,_packedOffset,_packedSize);
	LSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBitsBE16(count);
	};
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBitsBE16(1);
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());

	typedef HuffmanDecoder<uint32_t> LZXDecoder;

	// possibly padded/reused later if multiple blocks
	uint8_t literalTable[768];
	for (uint32_t i=0;i<768;i++) literalTable[i]=0;
	LZXDecoder literalDecoder;
	uint32_t previousDistance=1;

	while (!outputStream.eof())
	{

		auto createHuffmanTable=[&](LZXDecoder &dec,const uint8_t *bitLengths,uint32_t bitTableLength)
		{
			uint8_t minDepth=16,maxDepth=0;
			for (uint32_t i=0;i<bitTableLength;i++)
			{
				if (bitLengths[i] && bitLengths[i]<minDepth) minDepth=bitLengths[i];
				if (bitLengths[i]>maxDepth) maxDepth=bitLengths[i];
			}
			if (!maxDepth) return;

			dec.createOrderlyHuffmanTable(bitLengths,bitTableLength);
		};

		uint32_t method=readBits(3);
		if (method<1 || method>3) throw Decompressor::DecompressionError();

		LZXDecoder distanceDecoder;
		if (method==3)
		{
			uint8_t bitLengths[8];
			for (uint32_t i=0;i<8;i++) bitLengths[i]=readBits(3);
			createHuffmanTable(distanceDecoder,bitLengths,8);
		}

		size_t blockLength=readBits(8)<<16;
		blockLength|=readBits(8)<<8;
		blockLength|=readBits(8);
		if (OverflowCheck::sum(blockLength,outputStream.getOffset())>_rawSize) throw Decompressor::DecompressionError();

		if (method!=1)
		{
			literalDecoder.reset();
			for (uint32_t pos=0,block=0;block<2;block++)
			{
				uint32_t adjust=(block)?0:1;
				uint32_t maxPos=(block)?768:256;
				LZXDecoder bitLengthDecoder;
				{
					uint8_t lengthTable[20];
					for (uint32_t i=0;i<20;i++) lengthTable[i]=readBits(4);
					createHuffmanTable(bitLengthDecoder,lengthTable,20);
				}
				while (pos<maxPos)
				{
					uint32_t symbol=bitLengthDecoder.decode(readBit);

					auto doRepeat=[&](uint32_t count,uint8_t value)
					{
						if (count>maxPos-pos) count=maxPos-pos;
						while (count--) literalTable[pos++]=value;
					};
					
					auto symDecode=[&](uint32_t value)->uint32_t
					{
						return (literalTable[pos]+17-value)%17;
					};

					switch (symbol)
					{
						case 17:
						doRepeat(readBits(4)+3+adjust,0);
						break;

						case 18:
						doRepeat(readBits(6-adjust)+19+adjust,0);
						break;

						case 19:
						{
							uint32_t count=readBit()+3+adjust;
							doRepeat(count,symDecode(bitLengthDecoder.decode(readBit)));
						}
						break;

						default:
						literalTable[pos++]=symDecode(symbol);
						break;
					}
				}
			}
			createHuffmanTable(literalDecoder,literalTable,768);
		}
		
		while (blockLength)
		{
			uint32_t symbol=literalDecoder.decode(readBit);
			if (symbol<256) {
				outputStream.writeByte(symbol);
				blockLength--;
			} else {
				// both of these tables are almost too regular to be tables...
				static const uint8_t ldBits[32]={
					0,0,0,0,1,1,2,2,
					3,3,4,4,5,5,6,6,
					7,7,8,8,9,9,10,10,
					11,11,12,12,13,13,14,14};
				static const uint32_t ldAdditions[32]={
					   0x0,
					   0x1,   0x2,   0x3,   0x4,   0x6,   0x8,   0xc, 0x10,
					  0x18,  0x20,  0x30,  0x40,  0x60,  0x80,  0xc0, 0x100,
					 0x180, 0x200, 0x300, 0x400, 0x600, 0x800, 0xc00,0x1000,
					0x1800,0x2000,0x3000,0x4000,0x6000,0x8000,0xc000};

				symbol-=256;
				uint32_t bits=ldBits[symbol&0x1f];
				uint32_t distance=ldAdditions[symbol&0x1f];
				if (bits>=3 && method==3)
				{
					distance+=readBits(bits-3)<<3;
					uint32_t tmp=distanceDecoder.decode(readBit);
					distance+=tmp;
				} else {
					distance+=readBits(bits);
					if (!distance) distance=previousDistance;
				}
				previousDistance=distance;

				uint32_t count=ldAdditions[symbol>>5]+readBits(ldBits[symbol>>5])+3;
				if (count>blockLength) throw Decompressor::DecompressionError();
				outputStream.copy(distance,count);
				blockLength-=count;
			}
		}
	}
	if (verify)
	{
		uint32_t crc=CRC32(rawData,0,_rawSize,0);
		if (crc!=_rawCRC) throw Decompressor::VerificationError();
	}
	if (_isSampled)
		DLTADecode::decode(rawData,rawData,0,_rawSize);
}

XPKDecompressor::Registry<LZXDecompressor> LZXDecompressor::_XPKregistration;

}
