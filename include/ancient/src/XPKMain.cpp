/* Copyright (C) Teemu Suutari */

#include <cstring>
#include <memory>
#include <algorithm>

#include "common/SubBuffer.hpp"
#include "common/OverflowCheck.hpp"
#include "common/Common.hpp"
#include "common/Common.hpp"
#include "XPKMain.hpp"
#include "XPKDecompressor.hpp"

#include "ACCADecompressor.hpp"
#include "ARTMDecompressor.hpp"
#include "BLZWDecompressor.hpp"
#include "BZIP2Decompressor.hpp"
#include "CBR0Decompressor.hpp"
#include "CRMDecompressor.hpp"
#include "CYB2Decoder.hpp"
#include "DEFLATEDecompressor.hpp"
#include "DLTADecode.hpp"
#include "FASTDecompressor.hpp"
#include "FBR2Decompressor.hpp"
#include "FRLEDecompressor.hpp"
#include "HFMNDecompressor.hpp"
#include "HUFFDecompressor.hpp"
#include "ILZRDecompressor.hpp"
#include "IMPDecompressor.hpp"
#include "LHLBDecompressor.hpp"
#include "LIN1Decompressor.hpp"
#include "LIN2Decompressor.hpp"
#include "LZBSDecompressor.hpp"
#include "LZCBDecompressor.hpp"
#include "LZW2Decompressor.hpp"
#include "LZW4Decompressor.hpp"
#include "LZW5Decompressor.hpp"
#include "LZXDecompressor.hpp"
#include "MASHDecompressor.hpp"
#include "NONEDecompressor.hpp"
#include "NUKEDecompressor.hpp"
#include "PPDecompressor.hpp"
#include "RAKEDecompressor.hpp"
#include "RDCNDecompressor.hpp"
#include "RLENDecompressor.hpp"
#include "SDHCDecompressor.hpp"
#include "SHR3Decompressor.hpp"
#include "SHRIDecompressor.hpp"
#include "SLZ3Decompressor.hpp"
#include "SMPLDecompressor.hpp"
#include "SQSHDecompressor.hpp"
#include "SXSCDecompressor.hpp"
#include "TDCSDecompressor.hpp"
#include "ZENODecompressor.hpp"

namespace ancient::internal
{

bool XPKMain::detectHeader(uint32_t hdr) noexcept
{
	return hdr==FourCC("XPKF");
}

std::unique_ptr<Decompressor> XPKMain::create(const Buffer &packedData,bool verify,bool exactSizeKnown)
{
	return std::make_unique<XPKMain>(packedData,verify,0);
}

static std::vector<std::pair<bool(*)(uint32_t),std::unique_ptr<XPKDecompressor>(*)(uint32_t,uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&,bool)>> XPKDecompressors={
	{ACCADecompressor::detectHeaderXPK,ACCADecompressor::create},
	{ARTMDecompressor::detectHeaderXPK,ARTMDecompressor::create},
	{BLZWDecompressor::detectHeaderXPK,BLZWDecompressor::create},
	{BZIP2Decompressor::detectHeaderXPK,BZIP2Decompressor::create},
	{CBR0Decompressor::detectHeaderXPK,CBR0Decompressor::create},
	{CRMDecompressor::detectHeaderXPK,CRMDecompressor::create},
	{CYB2Decoder::detectHeaderXPK,CYB2Decoder::create},
	{DEFLATEDecompressor::detectHeaderXPK,DEFLATEDecompressor::create},
	{DLTADecode::detectHeaderXPK,DLTADecode::create},
	{FASTDecompressor::detectHeaderXPK,FASTDecompressor::create},
	{FBR2Decompressor::detectHeaderXPK,FBR2Decompressor::create},
	{FRLEDecompressor::detectHeaderXPK,FRLEDecompressor::create},
	{HFMNDecompressor::detectHeaderXPK,HFMNDecompressor::create},
	{HUFFDecompressor::detectHeaderXPK,HUFFDecompressor::create},
	{ILZRDecompressor::detectHeaderXPK,ILZRDecompressor::create},
	{IMPDecompressor::detectHeaderXPK,IMPDecompressor::create},
	{LHLBDecompressor::detectHeaderXPK,LHLBDecompressor::create},
	{LIN1Decompressor::detectHeaderXPK,LIN1Decompressor::create},
	{LIN2Decompressor::detectHeaderXPK,LIN2Decompressor::create},
	{LZBSDecompressor::detectHeaderXPK,LZBSDecompressor::create},
	{LZCBDecompressor::detectHeaderXPK,LZCBDecompressor::create},
	{LZW2Decompressor::detectHeaderXPK,LZW2Decompressor::create},
	{LZW4Decompressor::detectHeaderXPK,LZW4Decompressor::create},
	{LZW5Decompressor::detectHeaderXPK,LZW5Decompressor::create},
	{LZXDecompressor::detectHeaderXPK,LZXDecompressor::create},
	{MASHDecompressor::detectHeaderXPK,MASHDecompressor::create},
	{NONEDecompressor::detectHeaderXPK,NONEDecompressor::create},
	{NUKEDecompressor::detectHeaderXPK,NUKEDecompressor::create},
	{PPDecompressor::detectHeaderXPK,PPDecompressor::create},
	{RAKEDecompressor::detectHeaderXPK,RAKEDecompressor::create},
	{RDCNDecompressor::detectHeaderXPK,RDCNDecompressor::create},
	{RLENDecompressor::detectHeaderXPK,RLENDecompressor::create},
	{SDHCDecompressor::detectHeaderXPK,SDHCDecompressor::create},
	{SHR3Decompressor::detectHeaderXPK,SHR3Decompressor::create},
	{SHRIDecompressor::detectHeaderXPK,SHRIDecompressor::create},
	{SLZ3Decompressor::detectHeaderXPK,SLZ3Decompressor::create},
	{SMPLDecompressor::detectHeaderXPK,SMPLDecompressor::create},
	{SQSHDecompressor::detectHeaderXPK,SQSHDecompressor::create},
	{SXSCDecompressor::detectHeaderXPK,SXSCDecompressor::create},
	{TDCSDecompressor::detectHeaderXPK,TDCSDecompressor::create},
	{ZENODecompressor::detectHeaderXPK,ZENODecompressor::create}};

XPKMain::XPKMain(const Buffer &packedData,bool verify,uint32_t recursionLevel) :
	_packedData(packedData)
{
	if (packedData.size()<44) throw InvalidFormatError();
	uint32_t hdr=packedData.readBE32(0);
	if (!detectHeader(hdr)) throw InvalidFormatError();

	_packedSize=packedData.readBE32(4);
	_type=packedData.readBE32(8);
	_rawSize=packedData.readBE32(12);

	if (!_rawSize || !_packedSize) throw InvalidFormatError();
	if (_rawSize>getMaxRawSize() || _packedSize>getMaxPackedSize()) throw InvalidFormatError();

	uint8_t flags=packedData.read8(32);
	_longHeaders=(flags&1)?true:false;
	if (flags&2) throw InvalidFormatError();	// needs password. we do not support that
	if (flags&4)						// extra header
	{
		_headerSize=38+uint32_t(packedData.readBE16(36));
	} else {
		_headerSize=36;
	}

	if (OverflowCheck::sum(_packedSize,8U)>packedData.size()) throw InvalidFormatError();

	bool found=false;
	for (auto &it : XPKDecompressors)
	{
		if (it.first(_type)) 
		{
			if (recursionLevel>=getMaxRecursionLevel()) throw InvalidFormatError();
			else {
				found=true;
				break;
			}
		}
	}
	if (!found) throw InvalidFormatError();

	auto headerChecksum=[](const Buffer &buffer,size_t offset,size_t len)->bool
	{
		if (!len || OverflowCheck::sum(offset,len)>buffer.size()) return false;
		const uint8_t *ptr=buffer.data()+offset;
		uint8_t tmp=0;
		for (size_t i=0;i<len;i++)
			tmp^=ptr[i];
		return !tmp;
	};

	// this implementation assumes align padding is zeros
	auto chunkChecksum=[](const Buffer &buffer,size_t offset,size_t len,uint16_t checkValue)->bool
	{
		if (!len || OverflowCheck::sum(offset,len)>buffer.size()) return false;
		const uint8_t *ptr=buffer.data()+offset;
		uint8_t tmp[2]={0,0};
		for (size_t i=0;i<len;i++)
			tmp[i&1]^=ptr[i];
		return tmp[0]==(checkValue>>8) && tmp[1]==(checkValue&0xff);
	};


	if (verify)
	{
		if (!headerChecksum(_packedData,0,36)) throw VerificationError();

		std::unique_ptr<XPKDecompressor::State> state;
		forEachChunk([&](const Buffer &header,const Buffer &chunk,uint32_t rawChunkSize,uint8_t chunkType)->bool
		{
			if (!headerChecksum(header,0,header.size())) throw VerificationError();

			uint16_t hdrCheck=header.readBE16(2);
			if (chunk.size() && !chunkChecksum(chunk,0,chunk.size(),hdrCheck)) throw VerificationError();

			if (chunkType==1)
			{
				auto sub=createDecompressor(_type,_recursionLevel,chunk,state,true);
			} else if (chunkType!=0 && chunkType!=15) throw InvalidFormatError();
			return true;
		});
	}
}

XPKMain::~XPKMain()
{
	// nothing needed
}

const std::string &XPKMain::getName() const noexcept
{
	std::unique_ptr<XPKDecompressor> sub;
	std::unique_ptr<XPKDecompressor::State> state;
	try
	{
		forEachChunk([&](const Buffer &header,const Buffer &chunk,uint32_t rawChunkSize,uint8_t chunkType)->bool
		{
			try
			{
				sub=createDecompressor(_type,_recursionLevel,chunk,state,false);
			} catch (const Error&) {
				// should not happen since the code is already tried out,
				// however, lets handle the case gracefully
			}
			return false;
		});
	} catch (const Buffer::Error&) {
		// ditto
	}
	static std::string invName="<invalid>";
	return (sub)?sub->getSubName():invName;
}

size_t XPKMain::getPackedSize() const noexcept
{
	return _packedSize+8;
}

size_t XPKMain::getRawSize() const noexcept
{
	return _rawSize;
}

void XPKMain::decompressImpl(Buffer &rawData,bool verify)
{
	if (rawData.size()<_rawSize) throw DecompressionError();

	uint32_t destOffset=0;
	std::unique_ptr<XPKDecompressor::State> state;
	forEachChunk([&](const Buffer &header,const Buffer &chunk,uint32_t rawChunkSize,uint8_t chunkType)->bool
	{
		if (OverflowCheck::sum(destOffset,rawChunkSize)>rawData.size()) throw DecompressionError();
		if (!rawChunkSize) return true;

		ConstSubBuffer previousBuffer(rawData,0,destOffset);
		SubBuffer DestBuffer(rawData,destOffset,rawChunkSize);
		switch (chunkType)
		{
			case 0:
			if (rawChunkSize!=chunk.size()) throw DecompressionError();;
			std::memcpy(DestBuffer.data(),chunk.data(),rawChunkSize);
			break;

			case 1:
			{
				try
				{
					auto sub=createDecompressor(_type,_recursionLevel,chunk,state,false);
					sub->decompressImpl(DestBuffer,previousBuffer,verify);
				} catch (const InvalidFormatError&) {
					// we should throw a correct error
					throw DecompressionError();
				}
			}
			break;

			case 15:
			break;
			
			default:
			return false;
		}

		destOffset+=rawChunkSize;
		return true;
	});

	if (destOffset!=_rawSize) throw DecompressionError();

	if (verify)
	{
		if (std::memcmp(_packedData.data()+16,rawData.data(),std::min(_rawSize,16U))) throw DecompressionError();
	}
}

std::unique_ptr<XPKDecompressor> XPKMain::createDecompressor(uint32_t type,uint32_t recursionLevel,const Buffer &buffer,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	// since this method is used externally, better check recursion level
	if (recursionLevel>=getMaxRecursionLevel()) throw InvalidFormatError();
	for (auto &it : XPKDecompressors)
	{
		if (it.first(type)) return it.second(type,recursionLevel,buffer,state,verify);
	}
	throw InvalidFormatError();
}

template <typename F>
void XPKMain::forEachChunk(F func) const
{
	uint32_t currentOffset=0,rawSize,packedSize;
	bool isLast=false;

	while (currentOffset<_packedSize+8 && !isLast)
	{
		auto readDualValue=[&](uint32_t offsetShort,uint32_t offsetLong,uint32_t &value)
		{
			if (_longHeaders)
			{
				value=_packedData.readBE32(currentOffset+offsetLong);
			} else {
				value=uint32_t(_packedData.readBE16(currentOffset+offsetShort));
			}
		};

		uint32_t chunkHeaderLen=_longHeaders?12:8;
		if (!currentOffset)
		{
			// return first;
			currentOffset=_headerSize;
		} else {
			uint32_t tmp;
			readDualValue(4,4,tmp);
			tmp=((tmp+3U)&~3U);
			if (OverflowCheck::sum(tmp,currentOffset,chunkHeaderLen)>_packedSize)
				throw InvalidFormatError();
			currentOffset+=chunkHeaderLen+tmp;
		}
		readDualValue(4,4,packedSize);
		readDualValue(6,8,rawSize);
		
		ConstSubBuffer hdr(_packedData,currentOffset,chunkHeaderLen);
		ConstSubBuffer chunk(_packedData,currentOffset+chunkHeaderLen,packedSize);

		uint8_t type=_packedData.read8(currentOffset);
		if (!func(hdr,chunk,rawSize,type)) return;
		
		if (type==15) isLast=true;
	}
	if (!isLast) throw InvalidFormatError();
}

}
