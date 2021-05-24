/* Copyright (C) Teemu Suutari */

#include <cstdint>
#include <cstring>

#include "DEFLATEDecompressor.hpp"
#include "HuffmanDecoder.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/CRC32.hpp"
#include "common/OverflowCheck.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

static uint32_t Adler32(const Buffer &buffer,size_t offset,size_t len)
{
	if (!len || OverflowCheck::sum(offset,len)>buffer.size()) throw Buffer::OutOfBoundsError();
	const uint8_t *ptr=buffer.data()+offset;

	uint32_t s1=1,s2=0;
	for (size_t i=0;i<len;i++)
	{
		s1+=ptr[i];
		if (s1>=65521) s1-=65521;
		s2+=s1;
		if (s2>=65521) s2-=65521;
	}
	return (s2<<16)|s1;
}

bool DEFLATEDecompressor::detectHeader(uint32_t hdr) noexcept
{
	return ((hdr>>16)==0x1f8b);
}

bool DEFLATEDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return (hdr==FourCC("GZIP"));
}

std::unique_ptr<Decompressor> DEFLATEDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_unique<DEFLATEDecompressor>(packedData,exactSizeKnown,verify);
}

std::unique_ptr<XPKDecompressor> DEFLATEDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<DEFLATEDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

bool DEFLATEDecompressor::detectZLib()
{
	if (_packedData.size()<6) return false;

	// no knowledge about rawSize, before decompression
	// packedSize told by decompressor
	_packedSize=uint32_t(_packedData.size());
	_packedOffset=2;

	uint8_t cm=_packedData.read8(0);
	if ((cm&0xf)!=8) return false;
	if ((cm&0xf0)>0x70) return false;

	uint8_t flags=_packedData.read8(1);
	if (flags&0x20)
	{
		if (_packedSize<8) return false;
		_packedOffset+=4;
	}

	if (((uint16_t(cm)<<8)|uint16_t(flags))%31) return false;

	_type=Type::ZLib;
	return true;
}

DEFLATEDecompressor::DEFLATEDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify) :
	_packedData(packedData),
	_exactSizeKnown(exactSizeKnown)
{
	if (_packedData.size()<18) throw InvalidFormatError();
	uint32_t hdr=_packedData.readBE32(0);
	if (!detectHeader(hdr)) throw InvalidFormatError();

	uint8_t cm=_packedData.read8(2);
	if (cm!=8) throw InvalidFormatError();;

	uint8_t flags=_packedData.read8(3);
	if (flags&0xe0) throw InvalidFormatError();;
	
	uint32_t currentOffset=10;

	if (flags&4)
	{
		uint16_t xlen=_packedData.readLE16(currentOffset);
		currentOffset+=uint32_t(xlen)+2;
	}
	
	auto skipString=[&]()
	{
		uint8_t ch;
		do {
			ch=_packedData.read8(currentOffset);
			currentOffset++;
		} while (ch);
	};
	
	if (flags&8) skipString();		// FNAME
	if (flags&16) skipString();		// FCOMMENT

	if (flags&2) currentOffset+=2;		// FHCRC, not using that since it is only for header
	_packedOffset=currentOffset;

	if (OverflowCheck::sum(currentOffset,8U)>_packedData.size()) throw InvalidFormatError();

	if (_exactSizeKnown)
	{
		_packedSize=_packedData.size();
		_rawSize=_packedData.readLE32(_packedData.size()-4);
		if (!_rawSize) throw InvalidFormatError();
	}

	_type=Type::GZIP;
}

DEFLATEDecompressor::DEFLATEDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectZLib())
	{
		_packedSize=packedData.size();
		_packedOffset=0;
		_type=Type::Raw;
	}
}

DEFLATEDecompressor::DEFLATEDecompressor(const Buffer &packedData,size_t packedSize,size_t rawSize,bool isZlib,bool verify,bool deflate64) :
	_packedData(packedData),
	_deflate64(deflate64)
{
	_packedSize=packedSize;
	if (_packedSize>_packedData.size()) throw InvalidFormatError();
	if (isZlib)
	{
		// if it is not real zlib-stream fail.
		if (!detectZLib()) throw InvalidFormatError();
	} else {
		// raw stream
		_packedOffset=0;
		_rawSize=rawSize;
		_type=Type::Raw;
	}
}

DEFLATEDecompressor::~DEFLATEDecompressor()
{
	// nothing needed
}


const std::string &DEFLATEDecompressor::getName() const noexcept
{
	static std::string names[3]={
		"gzip: Deflate",
		"zlib: Deflate",
		"raw: Deflate/Deflate64"};
	return names[static_cast<uint32_t>(_type)];
}

const std::string &DEFLATEDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-GZIP: Deflate";
	return name;
}

size_t DEFLATEDecompressor::getPackedSize() const noexcept
{
	// no way to know before decompressing
	return _packedSize;
}


size_t DEFLATEDecompressor::getRawSize() const noexcept
{
	// same thing, decompression needed first
	return _rawSize;
}

void DEFLATEDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	size_t packedSize=_packedSize?_packedSize:_packedData.size();
	size_t rawSize=_rawSize?_rawSize:rawData.size();

	ForwardInputStream inputStream(_packedData,_packedOffset,packedSize);
	LSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBits8(1);
	};

	ForwardOutputStream outputStream(rawData,0,rawSize);

	bool final;
	do {
		final=readBit();
		uint8_t blockType=readBits(2);
		if (!blockType)
		{
			bitReader.reset();
			uint16_t len=inputStream.readByte();
			len|=uint16_t(inputStream.readByte())<<8;
			uint16_t nlen=inputStream.readByte();
			nlen|=uint16_t(inputStream.readByte())<<8;
			if (len!=(nlen^0xffffU)) throw DecompressionError();
			outputStream.produce(inputStream.consume(len),len);
		} else if (blockType==1 || blockType==2) {
			typedef HuffmanDecoder<int32_t> DEFLATEDecoder;
			DEFLATEDecoder llDecoder;
			DEFLATEDecoder distanceDecoder;

			if (blockType==1)
			{
				for (uint32_t i=0;i<24;i++) llDecoder.insert(HuffmanCode<int32_t>{7,i,int32_t(i+256)});
				for (uint32_t i=0;i<144;i++) llDecoder.insert(HuffmanCode<int32_t>{8,i+0x30,int32_t(i)});
				for (uint32_t i=0;i<8;i++) llDecoder.insert(HuffmanCode<int32_t>{8,i+0xc0,int32_t(i+280)});
				for (uint32_t i=0;i<112;i++) llDecoder.insert(HuffmanCode<int32_t>{9,i+0x190,int32_t(i+144)});

				for (uint32_t i=0;i<32;i++) distanceDecoder.insert(HuffmanCode<int32_t>{5,i,int32_t(i)});
			} else {
				uint32_t hlit=readBits(5)+257;
				// lets just error here, it is simpler
				if (hlit>=287) throw DecompressionError();
				uint32_t hdist=readBits(5)+1;
				uint32_t hclen=readBits(4)+4;

				uint8_t lengthTable[19];
				for (uint32_t i=0;i<19;i++) lengthTable[i]=0;
				static const uint8_t lengthTableOrder[19]={
					16,17,18, 0, 8, 7, 9, 6,
					10, 5,11, 4,12, 3,13, 2,
					14, 1,15};
				for (uint32_t i=0;i<hclen;i++) lengthTable[lengthTableOrder[i]]=readBits(3);

				DEFLATEDecoder bitLengthDecoder;
				bitLengthDecoder.createOrderlyHuffmanTable(lengthTable,19); // 19 and not hclen due to reordering

				// can the previous code flow from ll to distance table?
				// specification does not say and treats the two almost as combined.
				// So let previous code flow

				uint8_t llTableBits[286];
				uint8_t distanceTableBits[32];

				uint8_t prevValue=0;
				uint32_t i=0;
				while (i<hlit+hdist)
				{
					auto insert=[&](uint8_t value)
					{
						if (i>=hlit+hdist) throw DecompressionError();
						if (i>=hlit) distanceTableBits[i-hlit]=value;
							else llTableBits[i]=value;
						prevValue=value;
						i++;
					};

					int32_t code=bitLengthDecoder.decode(readBit);
					if (code<16) {
						insert(code);
					} else switch (code) {
						case 16:
						if (i) 
						{
							uint32_t count=readBits(2)+3;
							for (uint32_t j=0;j<count;j++) insert(prevValue);
						} else throw DecompressionError();
						break;

						case 17:
						for (uint32_t count=readBits(3)+3;count;count--) insert(0);
						break;

						case 18:
						for (uint32_t count=readBits(7)+11;count;count--) insert(0);
						break;

						default:
						throw DecompressionError();
					}
					
				}

				llDecoder.createOrderlyHuffmanTable(llTableBits,hlit);
				distanceDecoder.createOrderlyHuffmanTable(distanceTableBits,hdist);
			}

			// and now decode
			for (;;)
			{
				int32_t code=llDecoder.decode(readBit);
				if (code<256) {
					outputStream.writeByte(code);
				} else if (code==256) {
					break;
				} else {
					static const uint32_t lengthAdditions[29]={
						3,4,5,6,7,8,9,10,
						11,13,15,17,
						19,23,27,31,
						35,43,51,59,
						67,83,99,115,
						131,163,195,227,
						258};
					static const uint32_t lengthBits[29]={
						0,0,0,0,0,0,0,0,
						1,1,1,1,2,2,2,2,
						3,3,3,3,4,4,4,4,
						5,5,5,5,
						0};
					uint32_t count=(_deflate64&&code==285)?readBits(16)+3:(readBits(lengthBits[code-257])+lengthAdditions[code-257]);
					int32_t distCode=distanceDecoder.decode(readBit);
					if (distCode<0 || distCode>(_deflate64?31:29)) throw DecompressionError();
					static const uint32_t distanceAdditions[32]={
						1,2,3,4,5,7,9,13,
						0x11,0x19,0x21,0x31,0x41,0x61,0x81,0xc1,
						0x101,0x181,0x201,0x301,0x401,0x601,0x801,0xc01,
						0x1001,0x1801,0x2001,0x3001,0x4001,0x6001,
						0x8001,0xc001};
					static const uint32_t distanceBits[32]={
						0,0,0,0,1,1,2,2,
						3,3,4,4,5,5,6,6,
						7,7,8,8,9,9,10,10,
						11,11,12,12,13,13,
						14,14};
					uint32_t distance=readBits(distanceBits[distCode])+distanceAdditions[distCode];
					outputStream.copy(distance,count);
				}
			}
		} else {
			throw DecompressionError();
		}
	} while (!final);

	if (!_rawSize) _rawSize=outputStream.getOffset();
	if (_type==Type::GZIP)
	{
		if (OverflowCheck::sum(inputStream.getOffset(),8U)>packedSize) throw DecompressionError();
		if (!_packedSize)
			_packedSize=inputStream.getOffset()+8;
	} else if (_type==Type::ZLib) {
		if (OverflowCheck::sum(inputStream.getOffset(),4U)>packedSize) throw DecompressionError();
		if (!_packedSize)
			_packedSize=inputStream.getOffset()+4;
	} else {
		if (!_packedSize)
			_packedSize=inputStream.getOffset();
	}
	if (_rawSize!=outputStream.getOffset()) throw DecompressionError();

	if (verify)
	{
		if (_type==Type::GZIP)
		{
			uint32_t crc=_packedData.readLE32(inputStream.getOffset());
			if (CRC32(rawData,0,_rawSize,0)!=crc) throw VerificationError();
		} else if (_type==Type::ZLib) {
			uint32_t adler=_packedData.readBE32(inputStream.getOffset());
			if (Adler32(rawData,0,_rawSize)!=adler) throw VerificationError();
		}
	}
}

void DEFLATEDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	decompressImpl(rawData,verify);
}

Decompressor::Registry<DEFLATEDecompressor> DEFLATEDecompressor::_registration;
XPKDecompressor::Registry<DEFLATEDecompressor> DEFLATEDecompressor::_XPKregistration;

}
