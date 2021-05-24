/* Copyright (C) Teemu Suutari */

#include "ARTMDecompressor.hpp"
#include "RangeDecoder.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool ARTMDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("ARTM");
}

std::unique_ptr<XPKDecompressor> ARTMDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<ARTMDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

ARTMDecompressor::ARTMDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
	if (packedData.size()<2) throw Decompressor::InvalidFormatError(); 
}

ARTMDecompressor::~ARTMDecompressor()
{
	// nothing needed
}

const std::string &ARTMDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-ARTM: Arithmetic encoding compressor";
	return name;
}

// There really is not much to see here.
// Except maybe for the fact that there is extra symbol defined but never used.
// It is used in the original implementation as a terminator. In here it is considered as an error.
// Logically it would decode into null-character (practically it would be instant buffer overflow)
void ARTMDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	class BitReader : public RangeDecoder::BitReader
	{
	public:
		BitReader(ForwardInputStream &stream) :
			_reader(stream)
		{
			// nothing needed
		}

		virtual ~BitReader()
		{
			// nothing needed
		}

		virtual uint32_t readBit() override final
		{
			return _reader.readBits8(1);
		}

	private:
		LSBBitReader<ForwardInputStream>	_reader;
	};


	ForwardInputStream inputStream(_packedData,0,_packedData.size(),true);
	ForwardOutputStream outputStream(rawData,0,rawData.size());
	BitReader bitReader(inputStream);

	uint16_t initialValue=0;
	for (uint32_t i=0;i<16;i++) initialValue=(initialValue<<1)|bitReader.readBit();
	RangeDecoder decoder(bitReader,initialValue);

	uint8_t characters[256];
	uint16_t frequencies[256];
	uint16_t frequencySums[256];

	for (uint32_t i=0;i<256;i++)
	{
		characters[i]=i;
		frequencies[i]=1;
		frequencySums[i]=256-i;
	}
	uint16_t frequencyTotal=257;

	while (!outputStream.eof())
	{
		uint16_t value=decoder.decode(frequencyTotal);
		uint16_t symbol;
		for (symbol=0;symbol<256;symbol++)
			if (frequencySums[symbol]<=value) break;
		if (symbol==256) throw Decompressor::DecompressionError();
		decoder.scale(frequencySums[symbol],frequencySums[symbol]+frequencies[symbol],frequencyTotal);
		outputStream.writeByte(characters[symbol]);

		if (frequencyTotal==0x3fffU)
		{
			frequencyTotal=1;
			for (int32_t i=255;i>=0;i--)
			{
				frequencySums[i]=frequencyTotal;
				frequencyTotal+=frequencies[i]=(frequencies[i]+1)>>1;
			}
		}
		
		uint16_t i;
		for (i=symbol;i&&frequencies[i-1]==frequencies[i];i--);
		if (i!=symbol)
			std::swap(characters[symbol],characters[i]);
		frequencies[i]++;
		while (i--) frequencySums[i]++;
		frequencyTotal++;
	}
}

XPKDecompressor::Registry<ARTMDecompressor> ARTMDecompressor::_XPKregistration;

}
