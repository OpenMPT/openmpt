
/* Copyright (C) Teemu Suutari */

#include <cstdint>
#include <cstring>

#include "PackDecompressor.hpp"
#include "HuffmanDecoder.hpp"
#include "DynamicHuffmanDecoder.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool PackDecompressor::detectHeader(uint32_t hdr) noexcept
{
	return ((hdr>>16)==0x1f1eU||(hdr>>16)==0x1f1fU);
}

std::shared_ptr<Decompressor> PackDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_shared<PackDecompressor>(packedData,exactSizeKnown,verify);
}

PackDecompressor::PackDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify) :
	_packedData(packedData)
{
	if (_packedData.size()<6U)
		throw InvalidFormatError();
	uint32_t hdr=_packedData.readBE16(0);
	if (!detectHeader(hdr<<16U))
		throw InvalidFormatError();
	_isOldVersion=hdr==0x1f1fU;
	if (exactSizeKnown) _packedSize=packedData.size();
	if (_isOldVersion)
	{
		// PDP endian!!!
		_rawSize=(uint32_t(_packedData.readLE16(2U))<<16U)|_packedData.readLE16(4);
	} else {
		_rawSize=_packedData.readBE32(2U);
	}
	if (_rawSize>getMaxRawSize() || (_isOldVersion && !_rawSize))
		throw InvalidFormatError();
}

PackDecompressor::~PackDecompressor()
{
	// nothing needed
}


const std::string &PackDecompressor::getName() const noexcept
{
	static std::string names[2]={
		"z: Pack (Old)",
		"z: Pack"};
	return names[_isOldVersion?0:1U];
}

size_t PackDecompressor::getPackedSize() const noexcept
{
	// no way to know before decompressing
	return _packedSize;
}


size_t PackDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

void PackDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	ForwardInputStream inputStream(_packedData,6,_packedSize?_packedSize:_packedData.size());
	ForwardOutputStream outputStream(rawData,0,rawData.size());
	MSBBitReader<ForwardInputStream> bitReader(inputStream);

	if (_isOldVersion)
	{
		HuffmanDecoder<uint8_t> decoder;
		{
			auto readWord=[&]()->uint16_t
			{
				uint16_t ret=inputStream.readByte();
				return ret|=uint16_t(inputStream.readByte())<<8U;
			};

			uint16_t tree[1024];
			uint32_t count=readWord();
			if (count>=1024U)
				throw DecompressionError();
			for (uint32_t i=0;i<count;i++)
			{
				uint8_t tmp=inputStream.readByte();
				if (tmp<255U) tree[i]=tmp;
					else tree[i]=readWord();
			}

			auto branch=[&](uint32_t node,uint32_t length,uint32_t bits,auto branch)->void
			{
				if (node>=count)
					throw DecompressionError();
				if (tree[node])
				{
					length++;
					bits<<=1U;
					if (length>24U)
						throw DecompressionError();
					branch(node+tree[node],length,bits,branch);
					if (node+1>=count)
						throw DecompressionError();
					branch(node+tree[node+1],length,bits|1U,branch);
				} else {
					if (!length)
						throw DecompressionError();
					decoder.insert(HuffmanCode<uint8_t>{length,bits,uint8_t(tree[node+1])});
				}
			};
			branch(0,0,0,branch);
		}

		auto readBit=[&]()->uint32_t
		{
			return bitReader.readBitsLE16(1);
		};

		while (outputStream.getOffset()!=_rawSize)
			outputStream.writeByte(decoder.decode(readBit));
	} else {
		HuffmanDecoder<uint16_t> decoder;
		// interesting ordering...
		{
			uint32_t maxLevel=inputStream.readByte();
			if (!maxLevel || maxLevel>24U)
				throw DecompressionError();
			uint16_t levelCounts[24];
			for (uint32_t i=0;i<maxLevel;i++)
				levelCounts[i]=inputStream.readByte();
			levelCounts[maxLevel-1U]+=2U;
			uint32_t code=0x100'0000U;
			for (uint32_t i=0;i<maxLevel;i++)
			{
				code-=levelCounts[i]<<(23U-i);
				for (uint32_t j=0;j<levelCounts[i];j++)
				{
					uint16_t symbol=(i==maxLevel-1&&j==levelCounts[i]-1U)?256U:inputStream.readByte();
					decoder.insert(HuffmanCode<uint16_t>{i+1U,code>>(23U-i),symbol});
					code+=1U<<(23U-i);
				}
				code-=levelCounts[i]<<(23U-i);
			}
		}

		auto readBit=[&]()->uint32_t
		{
			return bitReader.readBits8(1);
		};

		while (outputStream.getOffset()!=_rawSize)
		{
			uint16_t code=decoder.decode(readBit);
			if (code==0x100U)
			{
				if (outputStream.getOffset()!=_rawSize)
					throw DecompressionError();
				break;
			}
			outputStream.writeByte(uint8_t(code));
		}
	}

	// we do not verify the exact packed length here since official encoder
	// tends to add few bytes at the end
	_packedSize=inputStream.getOffset();
}

}
