/* Copyright (C) Teemu Suutari */

#ifndef RAKEDECOMPRESSOR_HPP
#define RAKEDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class RAKEDecompressor : public XPKDecompressor
{
public:
	RAKEDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~RAKEDecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	bool		_isRAKE;
	size_t		_midStreamOffset=0;

	static XPKDecompressor::Registry<RAKEDecompressor> _XPKregistration;
};

}

#endif
