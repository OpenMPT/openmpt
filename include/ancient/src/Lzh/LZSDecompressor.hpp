/* Copyright (C) Teemu Suutari */

#ifndef LZSDECOMPRESSOR_HPP
#define LZSDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class LZSDecompressor : public Decompressor
{
public:
	LZSDecompressor(const Buffer &packedData);
	virtual ~LZSDecompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

private:
	const Buffer	&_packedData;
};

}

#endif
