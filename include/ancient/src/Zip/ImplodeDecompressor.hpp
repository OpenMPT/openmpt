/* Copyright (C) Teemu Suutari */

#ifndef IMPLODEDECOMPRESSOR_HPP
#define IMPLODEDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class ImplodeDecompressor : public Decompressor
{
public:
	ImplodeDecompressor(const Buffer &packedData,uint32_t flags);
	virtual ~ImplodeDecompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

private:
	const Buffer	&_packedData;

	uint32_t	_flags;
};

}

#endif
