/* Copyright (C) Teemu Suutari */

#ifndef DLTADECODE_HPP
#define DLTADECODE_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{


class DLTADecode : public XPKDecompressor
{
public:
	DLTADecode(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~DLTADecode();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	// static method for easy external usage. Buffers can be the same for in-place replacement
	static void decode(Buffer &bufferDest,const Buffer &bufferSrc,size_t offset,size_t size);

private:
	const Buffer	&_packedData;

	static XPKDecompressor::Registry<DLTADecode> _XPKregistration;
};

}

#endif
