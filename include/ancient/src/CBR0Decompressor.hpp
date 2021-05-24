/* Copyright (C) Teemu Suutari */

#ifndef CBR0DECOMPRESSOR_HPP
#define CBR0DECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class CBR0Decompressor : public XPKDecompressor
{
public:
	CBR0Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~CBR0Decompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;
	bool		_isCBR0;

	static XPKDecompressor::Registry<CBR0Decompressor> _XPKregistration;
};

}

#endif
