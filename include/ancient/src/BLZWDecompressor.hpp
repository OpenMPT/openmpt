/* Copyright (C) Teemu Suutari */

#ifndef BLZWDECOMPRESSOR_HPP
#define BLZWDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class BLZWDecompressor : public XPKDecompressor
{
public:
	BLZWDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~BLZWDecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr);
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	uint32_t	_maxBits=0;
	size_t		_stackLength=0;

	static XPKDecompressor::Registry<BLZWDecompressor> _XPKregistration;
};

}

#endif
