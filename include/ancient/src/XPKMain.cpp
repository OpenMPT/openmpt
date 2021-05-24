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

std::vector<std::pair<bool(*)(uint32_t),std::unique_ptr<XPKDecompressor>(*)(uint32_t,uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&,bool)>> *XPKMain::_XPKDecompressors=nullptr;

void XPKMain::registerDecompressor(bool(*detect)(uint32_t),std::unique_ptr<XPKDecompressor>(*create)(uint32_t,uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&,bool))
{
	static std::vector<std::pair<bool(*)(uint32_t),std::unique_ptr<XPKDecompressor>(*)(uint32_t,uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&,bool)>> _list;
	if (!_XPKDecompressors) _XPKDecompressors=&_list;
	_XPKDecompressors->emplace_back(detect,create);
}


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
	for (auto &it : *_XPKDecompressors)
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
	for (auto &it : *_XPKDecompressors)
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

Decompressor::Registry<XPKMain> XPKMain::_registration;

}
