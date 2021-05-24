/* Copyright (C) Teemu Suutari */

#ifndef HFMNDECOMPRESSOR_HPP
#define HFMNDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class HFMNDecompressor : public XPKDecompressor
{
public:
	HFMNDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~HFMNDecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	size_t		_headerSize;
	size_t		_rawSize;

	static XPKDecompressor::Registry<HFMNDecompressor> _XPKregistration;
};

}

#endif
