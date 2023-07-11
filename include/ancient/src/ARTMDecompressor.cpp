/* Copyright (C) Teemu Suutari */

#include "ARTMDecompressor.hpp"
#include "RangeDecoder.hpp"
#include "FrequencyTree.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool ARTMDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("ARTM");
}

std::shared_ptr<XPKDecompressor> ARTMDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::shared_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_shared<ARTMDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

ARTMDecompressor::ARTMDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::shared_ptr<XPKDecompressor::State> &state,bool verify) :
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

	// first one will never be used, but doing it this way saves us on some nasty arith
	// on every place later
	FrequencyTree<uint16_t,uint16_t,257> tree;
	uint8_t characters[257];

	for (uint32_t i=0;i<257;i++)
	{
		tree.add(i,1);
		characters[i]=256U-i;
	}

	while (!outputStream.eof())
	{
		uint16_t value=decoder.decode(tree.getTotal());
		uint16_t low,freq;
		uint16_t symbol=tree.decode(value,low,freq);
		if (!symbol)
			throw Decompressor::DecompressionError();
		decoder.scale(low,low+freq,tree.getTotal());
		outputStream.writeByte(characters[symbol]);

		if (tree.getTotal()==0x3fffU)
		{
			for (uint32_t i=1;i<=256;i++)
				tree.set(i,(tree[i]+1)>>1);
		}
		
		uint16_t i;
		for (i=symbol;i<256&&tree[i+1]==tree[i];i++);
		if (i!=symbol)
			std::swap(characters[symbol],characters[i]);
		tree.add(i,1);
	}
}

}
