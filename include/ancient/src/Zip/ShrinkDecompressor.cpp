/* Copyright (C) Teemu Suutari */

#include <cstdint>
#include <cstring>
#include <vector>

#include "ShrinkDecompressor.hpp"
#include "../InputStream.hpp"
#include "../OutputStream.hpp"


namespace ancient::internal
{

ShrinkDecompressor::ShrinkDecompressor(const Buffer &packedData) :
	_packedData(packedData)
{

}

ShrinkDecompressor::~ShrinkDecompressor()
{
	// nothing needed
}

size_t ShrinkDecompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t ShrinkDecompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &ShrinkDecompressor::getName() const noexcept
{
	static std::string name="Zip: Shrink";
	return name;
}

void ShrinkDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	ForwardInputStream inputStream(_packedData,0,_packedData.size());
	LSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());

	const uint32_t maxCode=0x2000U;
	auto prefix=std::make_unique<uint32_t[]>(maxCode-257);
	auto suffix=std::make_unique<uint8_t[]>(maxCode-257);
	auto stack=std::make_unique<uint8_t[]>(maxCode-257);

	uint32_t freeIndex,codeBits,prevCode,newCode;

	auto suffixLookup=[&](uint32_t code)->uint32_t
	{
		// main protection against negative values
		if (code>=maxCode) throw DecompressionError();
		return (code<257)?code:suffix[code-257];
	};

	auto insert=[&](uint32_t code)
	{
		uint32_t stackPos=0;
		newCode=suffixLookup(code);
		while (code>=257)
		{
			if (stackPos+1>=maxCode-257) throw DecompressionError();
			stack[stackPos++]=newCode;
			code=prefix[code-257];
			newCode=suffixLookup(code);
		}
		stack[stackPos++]=newCode;
		while (stackPos) outputStream.writeByte(stack[--stackPos]);
	};

	for (uint32_t i=0;i<maxCode-257;i++) prefix[i]=~0U;
	codeBits=9;
	freeIndex=257;
	prevCode=readBits(codeBits);
	insert(prevCode);

	while (!outputStream.eof())
	{
		uint32_t code=readBits(codeBits);
		if (code==256)
		{
			uint32_t tmp=readBits(codeBits);
			if (tmp==1)
			{
				if (codeBits!=13) codeBits++;
			} else if (tmp==2) {
				std::vector<bool> usageMap(freeIndex-257,false);
				for (uint32_t i=257;i<freeIndex;i++) {
					uint32_t tmp=prefix[i-257];
					if (!(tmp&0x8000'0000U) && tmp>=257) usageMap[tmp-257]=true;
				}
				uint32_t firstEmpty=freeIndex;
				for (uint32_t i=257;i<freeIndex;i++)
				{
					if (!usageMap[i-257])
					{
						prefix[i-257]=~0U;
						suffix[i-257]=0;
						if (i<firstEmpty) firstEmpty=i;
					}
				}
				freeIndex=firstEmpty;
			} else throw DecompressionError();
		} else {
			if (code>=257 && (prefix[code-257]&0x8000'0000U))
			{
				uint32_t tmp=newCode;
				insert(prevCode);
				outputStream.writeByte(tmp);
			} else insert(code);
			while (!(prefix[freeIndex-257]&0x8000'0000U) && freeIndex<maxCode)
				freeIndex++;
			if (freeIndex<maxCode)
			{
				suffix[freeIndex-257]=newCode;
				prefix[freeIndex-257]=prevCode;
				freeIndex++;
			}
			prevCode=code;
		}
	}
}

}
