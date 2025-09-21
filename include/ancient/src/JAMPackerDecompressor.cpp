/* Copyright (C) Teemu Suutari */

#include "JAMPackerDecompressor.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "DynamicHuffmanDecoder.hpp"
#include "VariableLengthCodeDecoder.hpp"
#include "common/Common.hpp"
#include "common/OverflowCheck.hpp"


namespace ancient::internal
{

bool JAMPackerDecompressor::detectHeader(uint32_t hdr,uint32_t footer) noexcept
{
	return hdr==FourCC("LSD!") || hdr==FourCC("LZH!") || hdr==FourCC("LZW!");
}

std::shared_ptr<Decompressor> JAMPackerDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_shared<JAMPackerDecompressor>(packedData);
}

JAMPackerDecompressor::JAMPackerDecompressor(const Buffer &packedData) :
	_packedData{packedData}
{
	if (packedData.size()<12U)
		throw InvalidFormatError();

	uint32_t hdr{packedData.readBE32(0)};
	if (!detectHeader(hdr,0))
		throw InvalidFormatError();

	_packedSize=packedData.readBE32(8U);
	_rawSize=packedData.readBE32(4U);
	switch (hdr)
	{
		case FourCC("LSD!"):
		_variant=Variant::JAM_V2;
		_packedSize=OverflowCheck::sum(_packedSize,4U);
		break;

		case FourCC("LZH!"):
		_variant=Variant::JAM_LZH;
		_packedSize=OverflowCheck::sum(_packedSize,12U);
		break;

		case FourCC("LZW!"):
		_variant=Variant::JAM_LZW;
		break;

		default:
		throw InvalidFormatError();
	}

	if (_packedSize>packedData.size() || _packedSize>getMaxPackedSize())
		throw InvalidFormatError();
	if (!_rawSize || _rawSize>getMaxRawSize())
		throw InvalidFormatError();
}

const std::string &JAMPackerDecompressor::getName() const noexcept
{
	static std::string names[3]={
		{"LSD: JAMPacker V2 v2.x+"},
		{"LZH: JAMPacker LZH v3.0 / v4.0"},
		{"LZW: JAMPacker LZW v4.0"}};
	return names[static_cast<uint32_t>(_variant)];
}

size_t JAMPackerDecompressor::getPackedSize() const noexcept
{
	return _packedSize;
}

size_t JAMPackerDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

void JAMPackerDecompressor::decompressInternalV2(Buffer &rawData)
{
	// Almost Ice
	BackwardInputStream inputStream{_packedData,12U,_packedSize};
	MSBBitReader<BackwardInputStream> bitReader{inputStream};
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};
	auto readByte=[&]()->uint8_t
	{
		return inputStream.readByte();
	};

	// anchor-bit handling
	{
		if (inputStream.readBE16()&0x8000U)
			readByte();		// align byte
		uint32_t tmp{readByte()};
		for (uint32_t i=1U,j=7U;i<0x80U;i<<=1U,j--) if (tmp&i)
		{
			bitReader.reset(tmp>>(8U-j),j);
			break;
		}
	}

	BackwardOutputStream outputStream{rawData,0,_rawSize};

	VariableLengthCodeDecoder litVlcDecoder{1,2,2,3,10};
	VariableLengthCodeDecoder countBaseDecoder{1,1,1,1};
	VariableLengthCodeDecoder countDecoder{0,0,1,2,10};
	VariableLengthCodeDecoder distanceBaseDecoder{1,1};
	VariableLengthCodeDecoder distanceDecoder{5,8,12};

	for (;;)
	{
		if (readBits(1U))
		{
			uint32_t litLength{litVlcDecoder.decodeCascade(readBits)+1U};
			for (uint32_t i=0;i<litLength;i++)
				outputStream.writeByte(readByte());
		}
		// exit criteria
		if (outputStream.eof()) break;

		uint32_t countBase{countBaseDecoder.decodeCascade(readBits)};
		uint32_t count{countDecoder.decode(readBits,countBase)+2U};
		uint32_t distance;
		if (count==2U)
		{
			if (readBits(1U)) distance=readBits(9U)+0x40U;
				else distance=readBits(6U);
		} else {
			uint32_t distanceBase{distanceBaseDecoder.decodeCascade(readBits)};
			if (distanceBase<2U) distanceBase^=1U;
			distance=distanceDecoder.decode(readBits,distanceBase);
		}
		distance+=count;
		outputStream.copy(distance,count);
	}
	if (!inputStream.eof())
		throw DecompressionError();
}

void JAMPackerDecompressor::decompressInternalLZH(Buffer &rawData)
{
	// Smells like backwards LH
	BackwardInputStream inputStream{_packedData,12U,_packedSize};
	MSBBitReader<BackwardInputStream> bitReader{inputStream};
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBits8(1U);
	};
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};

	BackwardOutputStream outputStream{rawData,0,_rawSize};

	DynamicHuffmanDecoder<314> decoder;
	VariableLengthCodeDecoder vlcDecoder{5,5,6,6,6,7,7,7,7,8,8,8,9,9,9,10};

	while (!outputStream.eof())
	{
		uint16_t symbol=decoder.decode(readBit);
		if (decoder.getMaxFrequency()==0x8000U)
		{
			decoder.halve();
		}
		decoder.update(symbol);
		if (symbol<256U)
		{
			outputStream.writeByte(uint8_t(symbol));
		} else {
			uint32_t distance{vlcDecoder.decode(readBits,readBits(4U))+1U};
			uint32_t count{symbol-253U};
			outputStream.copy(distance,count,0x20U);
		}
	}
	if (!inputStream.eof())
		throw DecompressionError();
}

void JAMPackerDecompressor::decompressInternalLZW(Buffer &rawData)
{
	// Nothing do to with LZW
	BackwardInputStream inputStream{_packedData,12U,_packedSize};
	LSBBitReader<BackwardInputStream> bitReader{inputStream};
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBits8(1U);
	};
	auto readByte=[&]()->uint8_t
	{
		return inputStream.readByte();
	};

	BackwardOutputStream outputStream{rawData,0,_rawSize};
	while (!outputStream.eof())
	{
		if (readBit())
		{
			outputStream.writeByte(readByte());
		} else {
			uint8_t byte1{readByte()};
			uint8_t byte2{readByte()};
			uint32_t code{uint32_t{byte1}|(uint32_t(byte2&0xf0U)<<4U)};
			uint32_t count{(byte2&0xfU)+3};
			// where does the magic offset 0xfed come from?
			uint32_t distance{((0xfedU+_rawSize-uint32_t(outputStream.getOffset())-code)&0xfffU)+1U};
			// no partial over-the-bounds block
			if (distance+uint32_t(outputStream.getOffset())>=_rawSize)
			{
				for (uint32_t i=0;i<count;i++) outputStream.writeByte(0x20U);
			} else outputStream.copy(distance,count);
		}
	}
	if (!inputStream.eof())
		throw DecompressionError();
}

// V1 handled by ByteKiller
// Jam Ice-variant handled by Ice
void JAMPackerDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	switch (_variant)
	{
		case Variant::JAM_V2:
		decompressInternalV2(rawData);
		break;

		case Variant::JAM_LZH:
		decompressInternalLZH(rawData);
		break;

		case Variant::JAM_LZW:
		decompressInternalLZW(rawData);
		break;

		default:
		throw DecompressionError();
	}
}

}
