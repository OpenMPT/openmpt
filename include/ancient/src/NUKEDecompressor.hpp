/* Copyright (C) Teemu Suutari */

#ifndef NUKEDECOMPRESSOR_HPP
#define NUKEDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class NUKEDecompressor : public XPKDecompressor
{
public:
	NUKEDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~NUKEDecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	bool		_isDUKE=false;

	static XPKDecompressor::Registry<NUKEDecompressor> _XPKregistration;
};

}

#endif
