/* Copyright (C) Teemu Suutari */

#include <cstring>

#include "common/SubBuffer.hpp"
#include "SDHCDecompressor.hpp"
#include "XPKMain.hpp"
#include "DLTADecode.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool SDHCDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("SDHC");
}

std::unique_ptr<XPKDecompressor> SDHCDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<SDHCDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

SDHCDecompressor::SDHCDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr) || _packedData.size()<2)
		throw Decompressor::InvalidFormatError();
	_mode=_packedData.readBE16(0);
	if (verify && (_mode&0x8000U))
	{
		ConstSubBuffer src(_packedData,2,_packedData.size()-2);
		XPKMain main(src,_recursionLevel+1,true);
	}
}

SDHCDecompressor::~SDHCDecompressor()
{
	// nothing needed
}

const std::string &SDHCDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-SDHC: Sample delta huffman compressor";
	return name;
}

void SDHCDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	ConstSubBuffer src(_packedData,2,_packedData.size()-2);
	if (_mode&0x8000U)
	{
		XPKMain main(src,_recursionLevel+1,false);
		main.decompress(rawData,verify);
	} else {
		if (src.size()!=rawData.size()) throw Decompressor::DecompressionError();
		std::memcpy(rawData.data(),src.data(),src.size());
	}

	size_t length=rawData.size()&~3U;

	auto deltaDecodeMono=[&]()
	{
		uint8_t *buf=rawData.data();

		uint16_t ctr=0;
		for (size_t i=0;i<length;i+=2)
		{
			uint16_t tmp;
			tmp=(uint16_t(buf[i])<<8)|uint16_t(buf[i+1]);
			ctr+=tmp;
			buf[i]=ctr>>8;
			buf[i+1]=ctr&0xff;
		}
	};

	auto deltaDecodeStereo=[&]()
	{
		uint8_t *buf=rawData.data();

		uint16_t ctr1=0,ctr2=0;
		for (size_t i=0;i<length;i+=4)
		{
			uint16_t tmp;
			tmp=(uint16_t(buf[i])<<8)|uint16_t(buf[i+1]);
			ctr1+=tmp;
			tmp=(uint16_t(buf[i+2])<<8)|uint16_t(buf[i+3]);
			ctr2+=tmp;
			buf[i]=ctr1>>8;
			buf[i+1]=ctr1&0xff;
			buf[i+2]=ctr2>>8;
			buf[i+3]=ctr2&0xff;
		}
	};

	switch (_mode&15)
	{
		case 1:
		DLTADecode::decode(rawData,rawData,0,length);
		// intentional fall through
		case 0:
		DLTADecode::decode(rawData,rawData,0,length);
		break;
		
		case 3:
		deltaDecodeMono();
		// intentional fall through
		case 2:
		deltaDecodeMono();
		break;

		case 11:
		deltaDecodeStereo();
		// intentional fall through
		case 10:
		deltaDecodeStereo();
		break;

		default:
		throw Decompressor::DecompressionError();
	}
}

}
