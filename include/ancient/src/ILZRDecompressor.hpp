/* Copyright (C) Teemu Suutari */

#ifndef ILZRDECOMPRESSOR_HPP
#define ILZRDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class ILZRDecompressor : public XPKDecompressor
{
public:
	ILZRDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~ILZRDecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	size_t		_rawSize=0;

	static XPKDecompressor::Registry<ILZRDecompressor> _XPKregistration;
};

}

#endif
