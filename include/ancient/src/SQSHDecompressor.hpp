/* Copyright (C) Teemu Suutari */

#ifndef SQSHDECOMPRESSOR_HPP
#define SQSHDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class SQSHDecompressor : public XPKDecompressor
{
public:
	SQSHDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~SQSHDecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	uint32_t	_rawSize=0;

	static XPKDecompressor::Registry<SQSHDecompressor> _XPKregistration;
};

}

#endif
