/* Copyright (C) Teemu Suutari */

#ifndef REDUCEDECOMPRESSOR_HPP
#define REDUCEDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class ReduceDecompressor : public Decompressor
{
public:
	ReduceDecompressor(const Buffer &packedData,uint32_t mode);
	virtual ~ReduceDecompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

private:
	const Buffer	&_packedData;

	uint32_t	_mode;
};

}

#endif
