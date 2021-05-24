/* Copyright (C) Teemu Suutari */

#include "PPDecompressor.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

PPDecompressor::PPState::PPState(uint32_t mode) :
	_cachedMode(mode)
{
	// nothing needed
}

PPDecompressor::PPState::~PPState()
{
	// nothing needed
}

bool PPDecompressor::detectHeader(uint32_t hdr) noexcept
{
	return (hdr==FourCC("PP11") || hdr==FourCC("PP20"));
}

bool PPDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("PWPK");
}

std::unique_ptr<Decompressor> PPDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_unique<PPDecompressor>(packedData,exactSizeKnown,verify);
}

std::unique_ptr<XPKDecompressor> PPDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<PPDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

PPDecompressor::PPDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify) :
	_packedData(packedData)
{
	if (!exactSizeKnown || packedData.size()<0x10)
		throw InvalidFormatError();		// no scanning support
	_dataStart=_packedData.size()-4;

	uint32_t hdr=packedData.readBE32(0);
	if (!detectHeader(hdr)) throw InvalidFormatError(); 
	uint32_t mode=packedData.readBE32(4);
	if (mode!=0x9090909 && mode!=0x90a0a0a && mode!=0x90a0b0b && mode!=0x90a0c0c && mode!=0x90a0c0d) throw InvalidFormatError();
	for (uint32_t i=0;i<4;i++)
	{
		_modeTable[i]=mode>>24;
		mode<<=8;
	}

	uint32_t tmp=packedData.readBE32(_dataStart);

	_rawSize=tmp>>8;
	_startShift=tmp&0xff;
	if (!_rawSize || _startShift>=0x20 ||
		_rawSize>getMaxRawSize()) throw InvalidFormatError();
}

PPDecompressor::PPDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr) || packedData.size()<0x10)
		throw InvalidFormatError(); 
	_dataStart=_packedData.size()-4;

	uint32_t mode;
	if (state.get())
	{
		mode=static_cast<PPState*>(state.get())->_cachedMode;
	} else {
		mode=packedData.readBE32(_dataStart);
		if (mode>4) throw InvalidFormatError();
		state.reset(new PPState(mode));
		_dataStart-=4;
	}

	static const uint32_t modeMap[5]={0x9090909,0x90a0a0a,0x90a0b0b,0x90a0c0c,0x90a0c0d};
	mode=modeMap[mode];
	for (uint32_t i=0;i<4;i++)
	{
		_modeTable[i]=mode>>24;
		mode<<=8;
	}

	uint32_t tmp=packedData.readBE32(_dataStart);

	_rawSize=tmp>>8;
	_startShift=tmp&0xff;
	if (!_rawSize || _startShift>=0x20 || _rawSize>getMaxRawSize())
		throw InvalidFormatError();

	_isXPK=true;
}

PPDecompressor::~PPDecompressor()
{
	// nothing needed
}

const std::string &PPDecompressor::getName() const noexcept
{
	static std::string name="PP: PowerPacker";
	return name;
}

const std::string &PPDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-PWPK: PowerPacker";
	return name;
}

size_t PPDecompressor::getPackedSize() const noexcept
{
	return 0;
}

size_t PPDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

void PPDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	if (rawData.size()<_rawSize) throw DecompressionError();

	BackwardInputStream inputStream(_packedData,_isXPK?0:8,_dataStart);
	LSBBitReader<BackwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return rotateBits(bitReader.readBitsBE32(count),count);
	};
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBitsBE32(1);
	};
	readBits(_startShift);

	BackwardOutputStream outputStream(rawData,0,_rawSize);

	for (;;)
	{
		if (!readBit())
		{
			uint32_t count=1;
			// This does not make much sense I know. But it is what it is...
			for (;;)
			{
				uint32_t tmp=readBits(2);
				count+=tmp;
				if (tmp<3) break;
			}
			for (uint32_t i=0;i<count;i++) outputStream.writeByte(readBits(8));
		}
		if (outputStream.eof()) break;
		uint32_t modeIndex=readBits(2);
		uint32_t count,distance;
		if (modeIndex==3)
		{
			distance=readBits(readBit()?_modeTable[modeIndex]:7)+1;
			// ditto
			count=5;
			for (;;)
			{
				uint32_t tmp=readBits(3);
				count+=tmp;
				if (tmp<7) break;
			}
		} else {
			count=modeIndex+2;
			distance=readBits(_modeTable[modeIndex])+1;
		}
		outputStream.copy(distance,count);
	}
}

void PPDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	if (_rawSize!=rawData.size()) throw DecompressionError();
	decompressImpl(rawData,verify);
}

Decompressor::Registry<PPDecompressor> PPDecompressor::_registration;
XPKDecompressor::Registry<PPDecompressor> PPDecompressor::_XPKregistration;

}
