/* Copyright (C) Teemu Suutari */

#include <cstring>

#include "NUKEDecompressor.hpp"
#include "DLTADecode.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool NUKEDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("NUKE") || hdr==FourCC("DUKE");
}

std::unique_ptr<XPKDecompressor> NUKEDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<NUKEDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

NUKEDecompressor::NUKEDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
	if (hdr==FourCC("DUKE")) _isDUKE=true;
}

NUKEDecompressor::~NUKEDecompressor()
{
	// nothing needed
}

const std::string &NUKEDecompressor::getSubName() const noexcept
{
	static std::string nameN="XPK-NUKE: LZ77-compressor";
	static std::string nameD="XPK-DUKE: LZ77-compressor with delta encoding";
	return (_isDUKE)?nameD:nameN;
}

void NUKEDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	// there are 2 streams, reverse stream for bytes and
	// normal stream for bits, the bit stream is divided
	// into single bit, 2 bit, 4 bit and random accumulator
	ForwardInputStream forwardInputStream(_packedData,0,_packedData.size());
	BackwardInputStream backwardInputStream(_packedData,0,_packedData.size());
	forwardInputStream.link(backwardInputStream);
	backwardInputStream.link(forwardInputStream);
	MSBBitReader<ForwardInputStream> bit1Reader(forwardInputStream);
	MSBBitReader<ForwardInputStream> bit2Reader(forwardInputStream);
	LSBBitReader<ForwardInputStream> bit4Reader(forwardInputStream);
	MSBBitReader<ForwardInputStream> bitXReader(forwardInputStream);
	auto readBit=[&]()->uint32_t
	{
		return bit1Reader.readBitsBE16(1);
	};
	auto read2Bits=[&]()->uint32_t
	{
		return bit2Reader.readBitsBE16(2);
	};
	auto read4Bits=[&]()->uint32_t
	{
		return bit4Reader.readBitsBE32(4);
	};
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitXReader.readBitsBE16(count);
	};
	auto readByte=[&]()->uint8_t
	{
		return backwardInputStream.readByte();
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());

	for (;;)
	{
		if (!readBit())
		{
			uint32_t count=0;
			if (readBit())
			{
				count=1;
			} else {
				uint32_t tmp;
				do {
					tmp=read2Bits();
					if (tmp) count+=5-tmp;
						else count+=3;
				} while (!tmp);
			}
			for (uint32_t i=0;i<count;i++) outputStream.writeByte(readByte());
		}
		if (outputStream.eof()) break;
		uint32_t distanceIndex=read4Bits();
		static const uint8_t distanceBits[16]={
			4,6,8,9,
			4,7,9,11,13,14,
			5,7,9,11,13,14};
		static const uint32_t distanceAdditions[16]={
			0,0x10,0x50,0x150,
			0,0x10,0x90,0x290,0xa90,0x2a90,
			0,0x20,0xa0,0x2a0,0xaa0,0x2aa0};
		uint32_t distance=readBits(distanceBits[distanceIndex])+distanceAdditions[distanceIndex];
		uint32_t count=(distanceIndex<4)?2:(distanceIndex<10)?3:0;
		if (!count)
		{
			count=read2Bits();
			if (!count)
			{
				count=3+3;
				uint32_t tmp;
				do {
					tmp=read4Bits();
					if (tmp) count+=16-tmp;
						else count+=15;
				} while (!tmp);
			} else count=3+4-count;
		}
		outputStream.copy(distance,count);
	}
	if (_isDUKE)
		DLTADecode::decode(rawData,rawData,0,rawData.size());
}

XPKDecompressor::Registry<NUKEDecompressor> NUKEDecompressor::_XPKregistration;

}
