/* Copyright (C) Teemu Suutari */

#include "BLZWDecompressor.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool BLZWDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC("BLZW");
}

std::unique_ptr<XPKDecompressor> BLZWDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<BLZWDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

BLZWDecompressor::BLZWDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
	_maxBits=_packedData.readBE16(0);
	if (_maxBits<9 || _maxBits>20) throw Decompressor::InvalidFormatError();;
	_stackLength=uint32_t(_packedData.readBE16(2))+5;
}

BLZWDecompressor::~BLZWDecompressor()
{
	// nothing needed
}

const std::string &BLZWDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-BLZW: LZW-compressor";
	return name;
}

void BLZWDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	ForwardInputStream inputStream(_packedData,4,_packedData.size());
	MSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};
	
	ForwardOutputStream outputStream(rawData,0,rawData.size());

	uint32_t maxCode=1<<_maxBits;
	auto prefix=std::make_unique<uint32_t[]>(maxCode-259);
	auto suffix=std::make_unique<uint8_t[]>(maxCode-259);
	auto stack=std::make_unique<uint8_t[]>(_stackLength);

	uint32_t freeIndex,codeBits,prevCode,newCode;

	auto suffixLookup=[&](uint32_t code)->uint32_t
	{
		if (code>=freeIndex) throw Decompressor::DecompressionError();
		return (code<259)?code:suffix[code-259];
	};

	auto insert=[&](uint32_t code)
	{
		uint32_t stackPos=0;
		newCode=suffixLookup(code);
		while (code>=259)
		{
			if (stackPos+1>=_stackLength) throw Decompressor::DecompressionError();
			stack[stackPos++]=newCode;
			code=prefix[code-259];
			newCode=suffixLookup(code);
		}
		stack[stackPos++]=newCode;
		while (stackPos) outputStream.writeByte(stack[--stackPos]);
	};

	auto init=[&]()
	{
		codeBits=9;
		freeIndex=259;
		prevCode=readBits(codeBits);
		insert(prevCode);
	};

	init();
	while (!outputStream.eof())
	{
		uint32_t code=readBits(codeBits);
		switch (code)
		{
			case 256:
			throw Decompressor::DecompressionError();
			break;

			case 257:
			init();
			break;

			case 258:
			codeBits++;
			break;

			default:
			if (code>=freeIndex)
			{
				uint32_t tmp=newCode;
				insert(prevCode);
				outputStream.writeByte(tmp);
			} else insert(code);
			if (freeIndex<maxCode)
			{
				suffix[freeIndex-259]=newCode;
				prefix[freeIndex-259]=prevCode;
				freeIndex++;
			}
			prevCode=code;
			break;
		}
	}
}

XPKDecompressor::Registry<BLZWDecompressor> BLZWDecompressor::_XPKregistration;

}
