/* Copyright (C) Teemu Suutari */

#include <cstring>

#include "MMCMPDecompressor.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/OverflowCheck.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool MMCMPDecompressor::detectHeader(uint32_t hdr) noexcept
{
	return hdr==FourCC("ziRC");
}

std::unique_ptr<Decompressor> MMCMPDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_unique<MMCMPDecompressor>(packedData,exactSizeKnown,verify);
}

MMCMPDecompressor::MMCMPDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify) :
	_packedData(packedData)
{
	if (!detectHeader(packedData.readBE32(0)) || packedData.readBE32(4U)!=FourCC("ONia") ||
		packedData.readLE16(8U)!=14U || packedData.size()<24U)
		throw InvalidFormatError();
	_version=packedData.readLE16(10U);
	_blocks=packedData.readLE16(12U);
	_blocksOffset=packedData.readLE32(18U);
	_rawSize=packedData.readLE32(14U);
	if (OverflowCheck::sum(_blocksOffset,uint32_t(_blocks)*4U)>packedData.size())
		throw InvalidFormatError();

	_packedSize=0;
	for (uint32_t i=0;i<_blocks;i++)
	{
		uint32_t blockAddr=packedData.readLE32(OverflowCheck::sum(_blocksOffset,i*4U));
		if (OverflowCheck::sum(blockAddr,20U)>=packedData.size())
			throw InvalidFormatError();
		uint32_t blockSize=packedData.readLE32(blockAddr+4U)+uint32_t(packedData.readLE16(blockAddr+12U))*8U+20U;
		_packedSize=std::max(_packedSize,OverflowCheck::sum(blockAddr,blockSize));
	}
	if (_packedSize>packedData.size())
		throw InvalidFormatError();
}

MMCMPDecompressor::~MMCMPDecompressor()
{
	// nothing needed
}

const std::string &MMCMPDecompressor::getName() const noexcept
{
	static std::string name="MMCMP: Music Module Compressor";
	return name;
}

size_t MMCMPDecompressor::getPackedSize() const noexcept
{
	return _packedSize;
}

size_t MMCMPDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

void MMCMPDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	if (rawData.size()<_rawSize) throw DecompressionError();
	// MMCMP allows gaps in data. Although not used in practice still we memset before decompressing to be sure
	std::memset(rawData.data(),0,rawData.size());

	uint8_t *rawDataPtr=rawData.data();
	for (uint32_t i=0;i<_blocks;i++)
	{
		uint32_t blockAddr=_packedData.readLE32(_blocksOffset+i*4U);

		uint32_t unpackedBlockSize=_packedData.readLE32(blockAddr);
		uint32_t packedBlockSize=_packedData.readLE32(blockAddr+4U);
		uint32_t fileChecksum=_packedData.readLE32(blockAddr+8U);
		uint32_t subBlocks=_packedData.readLE16(blockAddr+12U);
		uint16_t flags=_packedData.readLE16(blockAddr+14U);

		uint32_t packTableSize=_packedData.readLE16(blockAddr+16U);
		if (packTableSize>packedBlockSize)
			throw DecompressionError();
		uint16_t bitCount=_packedData.readLE16(blockAddr+18U);

		ForwardInputStream inputStream(_packedData,OverflowCheck::sum(blockAddr,subBlocks*8U,20U,packTableSize),OverflowCheck::sum(blockAddr,subBlocks*8U,20U,packedBlockSize));
		LSBBitReader<ForwardInputStream> bitReader(inputStream);
		auto readBits=[&](uint32_t count)->uint32_t
		{
			return bitReader.readBits8(count);
		};

		uint32_t currentSubBlock=0;
		uint32_t outputOffset=0,outputSize=0;
		auto readNextSubBlock=[&]()
		{
			if (currentSubBlock>=subBlocks)
				throw DecompressionError();
			outputOffset=_packedData.readLE32(blockAddr+currentSubBlock*8U+20U);
			outputSize=_packedData.readLE32(blockAddr+currentSubBlock*8U+24U);
			if (OverflowCheck::sum(outputOffset,outputSize)>_rawSize)
				throw DecompressionError();
			currentSubBlock++;
		};

		uint32_t checksum=0,checksumPartial=0,checksumRot=0;
		auto writeByte=[&](uint8_t value,bool allowOverrun=false)
		{
			while (!outputSize)
			{
				if (allowOverrun && currentSubBlock>=subBlocks) return;
				readNextSubBlock();
			}
			outputSize--;
			rawDataPtr[outputOffset++]=value;
			if (verify)
			{
				if (_version>=0x1310)
				{
					checksum^=value;
					checksum=(checksum<<1)|(checksum>>31);
				} else {
					checksumPartial|=((uint32_t)value)<<checksumRot;
					checksumRot+=8U;
					if (checksumRot==32U)
					{
						checksum^=checksumPartial;
						checksumPartial=0;
						checksumRot=0;
					}
				}
			}
		};
		
		// flags are
		// 0 = compressed
		// 1 = delta mode
		// 2 = 16 bit mode
		// 8 = stereo
		// 9 = abs16
		// 10 = endian
		// flags do not combine nicely
		// no compress - no other flags
		// compressed 8 bit - only delta (and presumably stereo matters)
		// compressed 16 bit - all flags matter
		if (!(flags&0x1U))
		{
			// not compressed
			for (uint32_t j=0;j<packedBlockSize;j++)
				writeByte(inputStream.readByte());
		} else if (!(flags&0x4U)) {
			// 8 bit compression

			// in case the bit-count is not enough to store a value, symbol at the end
			// of the codemap is created and this marks as a new bitCount
			static const uint8_t valueThresholds[16]={0x1U, 0x3U, 0x7U, 0xfU,0x1eU,0x3cU,0x78U,0xf8U};
			static const uint8_t extraBits[8]={3,3,3,3, 2,1,0,0};
			
			if (bitCount>=8)
				throw DecompressionError();
			uint8_t oldValue[2]={0,0};
			uint32_t chIndex=0;
			const uint8_t *tablePtr=&_packedData[blockAddr+subBlocks*8U+20U];
			for (uint32_t j=0;j<unpackedBlockSize;)
			{
				uint8_t value=readBits(bitCount+1);
				if (value>=valueThresholds[bitCount])
				{
					uint32_t newBitCount=readBits(extraBits[bitCount])+((value-valueThresholds[bitCount])<<extraBits[bitCount]);
					if (bitCount!=newBitCount)
					{
						bitCount=newBitCount&0x7U;
						continue;
					}
					value=0xf8U+readBits(3U);
					if (value==0xffU && readBits(1U)) break;
				}
				if (value>=packTableSize)
					throw DecompressionError();
				value=tablePtr[value];
				if (flags&0x2U)
				{
					// delta
					value+=oldValue[chIndex];
					oldValue[chIndex]=value;
					if (flags&0x100U) chIndex^=1U;		// stereo
				}
				writeByte(value);
				j++;
			}
		} else {
			// 16 bit compression

			// shameless copy-paste from 8-bit variant, with minor changes
			static const uint16_t valueThresholds[16]={
				  0x1U,   0x3U,   0x7U,   0xfU,  0x1eU,  0x3cU,  0x78U,  0xf0U,
				0x1f0U, 0x3f0U, 0x7f0U, 0xff0U,0x1ff0U,0x3ff0U,0x7ff0U,0xfff0U
			};
			static const uint8_t extraBits[16]={4,4,4,4, 3,2,1,0, 0,0,0,0, 0,0,0,0};
			
			if (bitCount>=16)
				throw DecompressionError();
			int16_t oldValue[2]={0,0};
			uint32_t chIndex=0;
			for (uint32_t j=0;j<unpackedBlockSize;)
			{
				int32_t value=readBits(bitCount+1);
				if (value>=valueThresholds[bitCount])
				{
					uint32_t newBitCount=readBits(extraBits[bitCount])+((value-valueThresholds[bitCount])<<extraBits[bitCount]);
					if (bitCount!=newBitCount)
					{
						bitCount=newBitCount&0xfU;
						continue;
					}
					value=0xfff0U+readBits(4U);
					if (value==0xffffU && readBits(1U)) break;
				}
				if (value&1U) value=-value-1;
				value>>=1;
				if (flags&0x2U)
				{
					// delta
					value+=oldValue[chIndex];
					oldValue[chIndex]=value;
					if (flags&0x100U) chIndex^=1U;		// stereo
				} else if (!(flags&0x200U)) value^=0x8000U;	// abs16
				if (flags&0x400U)
				{
					// big ending
					writeByte(value>>8U);
					writeByte(value,true);
				} else {
					// little endian
					writeByte(value);
					writeByte(value>>8U,true);
				}
				j+=2;
			}
		}
		if (verify && checksum!=fileChecksum)
			throw VerificationError();
	}
}

}
