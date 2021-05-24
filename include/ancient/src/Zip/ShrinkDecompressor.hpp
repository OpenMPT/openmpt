/* Copyright (C) Teemu Suutari */

#ifndef SHRINKDECOMPRESSOR_HPP
#define SHRINKDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class ShrinkDecompressor : public Decompressor
{
public:
	ShrinkDecompressor(const Buffer &packedData);
	virtual ~ShrinkDecompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

private:
	const Buffer	&_packedData;
};

}

#endif
