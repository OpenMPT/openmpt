/* Copyright (C) Teemu Suutari */

#include "ZENODecompressor.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool ZENODecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("ZENO");
}

std::unique_ptr<XPKDecompressor> ZENODecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<ZENODecompressor>(hdr,recursionLevel,packedData,state,verify);
}

ZENODecompressor::ZENODecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr) || _packedData.size()<6)
		throw Decompressor::InvalidFormatError();
	// first 4 bytes is checksum for password. It needs to be zero
	if (_packedData.readBE32(0)) throw Decompressor::InvalidFormatError();
	_maxBits=_packedData.read8(4);
	if (_maxBits<9 || _maxBits>20) throw Decompressor::InvalidFormatError();
	_startOffset=uint32_t(_packedData.read8(5))+6;
	if (_startOffset>=_packedData.size()) throw Decompressor::InvalidFormatError();
}

ZENODecompressor::~ZENODecompressor()
{
	// nothing needed
}

const std::string &ZENODecompressor::getSubName() const noexcept
{
	static std::string name="XPK-ZENO: LZW-compressor";
	return name;
}

void ZENODecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	ForwardInputStream inputStream(_packedData,_startOffset,_packedData.size());
	MSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());

	uint32_t maxCode=1<<_maxBits;
	uint32_t stackLength=5000;				// magic constant
	auto prefix=std::make_unique<uint32_t[]>(maxCode-258);
	auto suffix=std::make_unique<uint8_t[]>(maxCode-258);
	auto stack=std::make_unique<uint8_t[]>(stackLength);

	uint32_t freeIndex,codeBits,prevCode,newCode;

	auto init=[&]()
	{
		codeBits=9;
		freeIndex=258;
	};

	init();
	prevCode=readBits(9);
	newCode=prevCode;
	suffix[freeIndex-258]=0;
	prefix[freeIndex-258]=0;
	freeIndex++;
	outputStream.writeByte(newCode);
	
	while (!outputStream.eof())
	{
		if (freeIndex+3>=(1U<<codeBits) && codeBits<_maxBits) codeBits++;
		uint32_t code=readBits(codeBits);
		switch (code)
		{
			case 256:
			throw Decompressor::DecompressionError();
			break;

			case 257:
			init();
			break;

			default:
			{
				uint32_t stackPos=0;
				uint32_t tmp=code;
				if (tmp==freeIndex)
				{
					stack[stackPos++]=newCode;
					tmp=prevCode;
				}
				if (tmp>=258)
				{
					do {
						if (stackPos+1>=stackLength || tmp>=freeIndex) throw Decompressor::DecompressionError();
						stack[stackPos++]=suffix[tmp-258];
						tmp=prefix[tmp-258];
					} while (tmp>=258);
					stack[stackPos++]=newCode=tmp;
					while (stackPos) outputStream.writeByte(stack[--stackPos]);
				} else {
					newCode=tmp;
					outputStream.writeByte(tmp);
					if (stackPos) outputStream.writeByte(stack[0]);
				}
			}
			if (freeIndex<maxCode)
			{
				suffix[freeIndex-258]=newCode;
				prefix[freeIndex-258]=prevCode;
				freeIndex++;
			}
			prevCode=code;
			break;
		}
	}
}

XPKDecompressor::Registry<ZENODecompressor> ZENODecompressor::_XPKregistration;

}
