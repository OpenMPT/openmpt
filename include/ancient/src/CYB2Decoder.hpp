/* Copyright (C) Teemu Suutari */

#ifndef CYB2DECODER_HPP
#define CYB2DECODER_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class CYB2Decoder : public XPKDecompressor
{
public:
	CYB2Decoder(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~CYB2Decoder();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	uint32_t	_blockHeader;

	static XPKDecompressor::Registry<CYB2Decoder> _XPKregistration;
};

}

#endif
