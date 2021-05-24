/* Copyright (C) Teemu Suutari */

#ifndef LIN2DECOMPRESSOR_HPP
#define LIN2DECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class LIN2Decompressor : public XPKDecompressor
{
public:
	LIN2Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~LIN2Decompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	uint32_t	_ver=0;
	size_t		_endStreamOffset=0;
	size_t		_midStreamOffset=0;

	static XPKDecompressor::Registry<LIN2Decompressor> _XPKregistration;
};

}

#endif
