/* Copyright (C) Teemu Suutari */

#ifndef SXSCDECOMPRESSOR_HPP
#define SXSCDECOMPRESSOR_HPP

#include <cstdint>

#include "XPKDecompressor.hpp"
#include "InputStream.hpp"
#include "RangeDecoder.hpp"

namespace ancient::internal
{

class SXSCDecompressor : public XPKDecompressor
{
public:
	SXSCDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~SXSCDecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	class SXSCReader : public RangeDecoder::BitReader
	{
	public:
		SXSCReader(ForwardInputStream &stream);
		virtual ~SXSCReader();

		virtual uint32_t readBit() override final;

	private:
		MSBBitReader<ForwardInputStream>	_reader;
	};

	void decompressASC(Buffer &rawData,ForwardInputStream &inputStream);
	void decompressHSC(Buffer &rawData,ForwardInputStream &inputStream);

	const Buffer					&_packedData;
	bool						_isHSC;

	static XPKDecompressor::Registry<SXSCDecompressor> _XPKregistration;
};

}

#endif
