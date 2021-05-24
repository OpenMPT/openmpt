/* Copyright (C) Teemu Suutari */

#include "LIN1Decompressor.hpp"

#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool LIN1Decompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("LIN1") || hdr==FourCC("LIN3");
}

std::unique_ptr<XPKDecompressor> LIN1Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<LIN1Decompressor>(hdr,recursionLevel,packedData,state,verify);
}

LIN1Decompressor::LIN1Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
	_ver=(hdr==FourCC("LIN1"))?1:3;
	if (packedData.size()<5) throw Decompressor::InvalidFormatError();

	uint32_t tmp=packedData.readBE32(0);
	if (tmp) throw Decompressor::InvalidFormatError();	// password set
}

LIN1Decompressor::~LIN1Decompressor()
{
	// nothing needed
}

const std::string &LIN1Decompressor::getSubName() const noexcept
{
	static std::string name1="XPK-LIN1: LIN1 LINO packer";
	static std::string name3="XPK-LIN3: LIN3 LINO packer";
	return (_ver==1)?name1:name3;
}

void LIN1Decompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	ForwardInputStream inputStream(_packedData,5,_packedData.size());
	MSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};
	auto readByte=[&]()->uint8_t
	{
		return inputStream.readByte();
	};

	size_t rawSize=rawData.size();
	ForwardOutputStream outputStream(rawData,0,rawSize);

	while (!outputStream.eof())
	{
		if (!readBits(1))
		{
			outputStream.writeByte(readByte()^0x55);
		} else {
			uint32_t count=3;
			if (readBits(1))
			{
				count=readBits(2);
				if (count==3)
				{
					count=readBits(3);
					if (count==7)
					{
						count=readBits(4);
						if (count==15)
						{
							count=readByte();
							if (count==0xff) throw Decompressor::DecompressionError();
							count+=3;
						} else count+=14;
					} else count+=7;
				} else count+=4;
			}
			uint32_t distance = 0;
			switch (readBits(2))
			{
				case 0:
				distance=readByte()+1;
				break;

				case 1:
				distance=uint32_t(readBits(2))<<8;
				distance|=readByte();
				distance+=0x101;
				break;

				case 2:
				distance=uint32_t(readBits(4))<<8;
				distance|=readByte();
				distance+=0x501;
				break;

				case 3:
				distance=uint32_t(readBits(6))<<8;
				distance|=readByte();
				distance+=0x1501;
				break;
			}

			// buggy compressors
			count=std::min(count,uint32_t(rawSize-outputStream.getOffset()));
			if (!count) throw Decompressor::DecompressionError();

			outputStream.copy(distance,count);
		}
	}
}

XPKDecompressor::Registry<LIN1Decompressor> LIN1Decompressor::_XPKregistration;

}
