/* Copyright (C) Teemu Suutari */

#ifndef ZENODECOMPRESSOR_HPP
#define ZENODECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class ZENODecompressor : public XPKDecompressor
{
public:
	ZENODecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~ZENODecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	uint32_t	_maxBits=0;
	size_t		_startOffset=0;

	static XPKDecompressor::Registry<ZENODecompressor> _XPKregistration;
};

}

#endif
