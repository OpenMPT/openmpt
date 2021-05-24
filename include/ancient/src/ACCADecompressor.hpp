/* Copyright (C) Teemu Suutari */

#ifndef ACCADECOMPRESSOR_HPP
#define ACCADECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class ACCADecompressor : public XPKDecompressor
{
public:
	ACCADecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~ACCADecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	static XPKDecompressor::Registry<ACCADecompressor> _XPKregistration;
};

}

#endif
