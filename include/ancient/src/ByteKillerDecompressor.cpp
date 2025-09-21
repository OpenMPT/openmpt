/* Copyright (C) Teemu Suutari */

#include "ByteKillerDecompressor.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "HuffmanDecoder.hpp"
#include "common/Common.hpp"
#include "common/OverflowCheck.hpp"


namespace ancient::internal
{

bool ByteKillerDecompressor::detectHeader_Int(uint32_t hdr,Variant &variant) noexcept
{
	// Standard ByteKiller does not have a header
	switch (hdr)
	{
		case FourCC("ACE!"):		// Amiga Disk Magazine Resident #1
		variant=Variant::BK_ACE;
		return true;

		case FourCC("FVL0"):	 	// ANC Cruncher
		variant=Variant::BK_ANC;
		return true;

		case FourCC("GR20"):		// GR.A.C v2.0
		variant=Variant::BK_GRAC;
		return true;

		case FourCC("MD10"):		// Amiga Disk Magazine McDisk #1, #2
		variant=Variant::BK_MD10;
		return true;

		case FourCC("MD11"):		// Amiga Disk Magazine McDisk #3
		variant=Variant::BK_MD11;
		return true;

		default:
		return false;
	}
}

bool ByteKillerDecompressor::detectFooter_Int(uint32_t footer,Variant &variant) noexcept
{
	switch (footer)
	{
		case FourCC("data"):		// ByteKiller Pro
		variant=Variant::BK_PRO;
		return true;

		case FourCC("JEK!"):		// JEK / JAM v1
		variant=Variant::BK_JEK;
		return true;

		default:
		return false;
	}
}

bool ByteKillerDecompressor::detectHeader(uint32_t hdr,uint32_t footer) noexcept
{
	Variant dummy;
	if (detectHeader_Int(hdr,dummy) || detectFooter_Int(footer,dummy))
		return true;
	// lots of false positives here. (ByteKiller without header)
	return hdr&&hdr<=getMaxPackedSize()&&!(hdr&3U);
}

std::shared_ptr<Decompressor> ByteKillerDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_shared<ByteKillerDecompressor>(packedData,exactSizeKnown,verify);
}

ByteKillerDecompressor::ByteKillerDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify) :
	_packedData{packedData}
{
	auto processSize=[&](uint32_t offset)->uint32_t
	{
		uint32_t ret=packedData.readBE32(offset);
		// packedData size test is inaccurate (false positives), but it is useful as a filter
		if (!ret || (ret&3U) || ret>getMaxPackedSize() || ret>packedData.size())
			throw InvalidFormatError();
		return ret;
	};

	auto processStream=[&](uint32_t offset,uint32_t endOffset,uint32_t checksumOffset)
	{
		if (!_packedSize || _packedSize<offset || _packedSize>packedData.size() || _packedSize>getMaxPackedSize())
			throw InvalidFormatError();
		if (!_rawSize || _rawSize>getMaxRawSize())
			throw InvalidFormatError();

		_streamOffset=offset;
		_streamEndOffset=endOffset;
		uint32_t sum=0;
		for (uint32_t i=offset;i<endOffset;i+=4)
			sum^=packedData.readBE32(i);
		if (packedData.readBE32(checksumOffset)!=sum)
			throw InvalidFormatError();
	};

	uint32_t hdr{packedData.readBE32(0)};
	bool hasHdr=detectHeader_Int(hdr,_variant);
	bool hasFooter=(!hasHdr && exactSizeKnown)?detectFooter_Int(packedData.readBE32(packedData.size()-4U),_variant):false;
	switch (_variant)
	{
		case Variant::BK_STD:
		case Variant::BK_PRO:
		_packedSize=OverflowCheck::sum(processSize(0),12U);
		_rawSize=packedData.readBE32(4U);
		processStream(12U,_packedSize,8U);
		break;

		case Variant::BK_ACE:
		_packedSize=OverflowCheck::sum(processSize(4U),12U);
		_rawSize=packedData.readBE32(8U);
		processStream(16U,_packedSize,12U);
		break;

		case Variant::BK_ANC:
		_packedSize=OverflowCheck::sum(processSize(4U),8U);
		if (_packedSize<16U)
			throw InvalidFormatError();
		_rawSize=packedData.readBE32(_packedSize-4U);
		processStream(8U,_packedSize-8U,_packedSize-8U);
		break;

		case Variant::BK_GRAC:
		_packedSize=uint32_t(packedData.size());
		if (!exactSizeKnown || packedData.size()<16U)
			throw InvalidFormatError();
		_rawSize=packedData.readBE32(_packedSize-4U);
		processStream(8U,_packedSize-8U,_packedSize-8U);
		break;

		case Variant::BK_MD10:
		_packedSize=uint32_t(packedData.size());
		if (!exactSizeKnown || packedData.size()<12U)
			throw InvalidFormatError();
		_rawSize=packedData.readBE32(_packedSize-4U);
		processStream(4U,_packedSize-8U,_packedSize-8U);
		break;

		case Variant::BK_MD11:
		_packedSize=uint32_t(packedData.size());
		if (!exactSizeKnown || _packedSize<16U)
			throw InvalidFormatError();
		_rawSize=packedData.readBE32(4U);
		_rawSize^=FourCC("MD11");
		if ((_rawSize&0xffffU)!=(_rawSize>>16U))
			throw InvalidFormatError();
		_rawSize&=0xffffU;
		if (_rawSize!=packedData.readBE32(_packedSize-4U))
			throw InvalidFormatError();
		processStream(8U,_packedSize-8U,_packedSize-8U);
		break;

		case Variant::BK_JEK:
		_packedSize=uint32_t(packedData.size());
		_rawSize=packedData.readBE32(_packedSize-8U);
		processStream(0,_packedSize-12U,_packedSize-12U);
		break;

		default:
		throw InvalidFormatError();
	}
	if (!hasHdr && !hasFooter && OverflowCheck::sum(_packedSize,4U)<=packedData.size())
	{
		uint32_t footer=packedData.readBE32(_packedSize);
		if (detectFooter_Int(footer,_variant))
			_packedSize+=4U;
	}
}

const std::string &ByteKillerDecompressor::getName() const noexcept
{
	static std::string names[]={
		"BK: ByteKiller",
		"BK: ByteKiller Pro",
		"FVL0: ANC Cruncher",
		"JEK: Jek Packer v1.x - v2.x / JAM Packer v1.x"};
	static const uint32_t variantMapping[]={
		0, // BK_STD
		1, // BK_PRO
		0, // BK_ACE
		0, // BK_GRAC
		2, // BK_ANC
		0, // BK_MD10
		0, // BK_MD11
		3  // BK_JEK
	};
	return names[variantMapping[static_cast<uint32_t>(_variant)]];
}

size_t ByteKillerDecompressor::getPackedSize() const noexcept
{
	return _packedSize;
}

size_t ByteKillerDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

void ByteKillerDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	BackwardInputStream inputStream{_packedData,_streamOffset,_streamEndOffset};
	LSBBitReader<BackwardInputStream> bitReader{inputStream};
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBitsBE32(1U);
	};
	auto readBits=[&](uint32_t bits)->uint32_t
	{
		return rotateBits(bitReader.readBitsBE32(bits),bits);
	};
	BackwardOutputStream outputStream{rawData,0,_rawSize};

	// anchor bit
	{
		uint32_t tmp=inputStream.readBE32();
		for (uint32_t i=0x8000'0000U,j=31;i;i>>=1,j--) if (tmp&i)
		{
			bitReader.reset(tmp&(i-1),j);
			break;
		}
	}

	HuffmanDecoder<uint32_t> decoder
	{
		HuffmanCode{2,0b000,0U},
		HuffmanCode{2,0b001,1U},
		HuffmanCode{3,0b100,2U},
		HuffmanCode{3,0b101,3U},
		HuffmanCode{3,0b110,4U},
		HuffmanCode{3,0b111,5U}
	};

	while (!outputStream.eof())
	{
		uint32_t count,distance;
		switch (decoder.decode(readBit))
		{
			case 0:
			count=readBits(3U)+1U;
			for (uint32_t i=0;i<count;i++) outputStream.writeByte(readBits(8U));
			break;

			case 1:
			distance=readBits(8U);
			outputStream.copy(distance,2);
			break;

			case 2:
			distance=readBits(9U);
			outputStream.copy(distance,3);
			break;

			case 3:
			distance=readBits(10U);
			outputStream.copy(distance,4);
			break;

			case 4:
			count=readBits(8U)+1U;
			distance=readBits(12U);
			outputStream.copy(distance,count);
			break;

			case 5:
			count=readBits(8U)+9U;
			for (uint32_t i=0;i<count;i++) outputStream.writeByte(readBits(8U));
			break;

			default:
			throw DecompressionError();
		}
	}
	if (bitReader.available())
		throw DecompressionError();
}

}
