/* Copyright (C) Teemu Suutari */

#ifndef ZIPDECOMPRESSOR_HPP
#define ZIPDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class ZIPDecompressor : public Decompressor
{
public:
	ZIPDecompressor(const Buffer &packedData,uint32_t method,uint32_t flags);
	virtual ~ZIPDecompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

private:
	const Buffer	&_packedData;

	uint32_t	_method;
	uint32_t	_flags;
};

}

#endif
