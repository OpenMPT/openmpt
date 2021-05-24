/* Copyright (C) Teemu Suutari */

#ifndef LZ5DECOMPRESSOR_HPP
#define LZ5DECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class LZ5Decompressor : public Decompressor
{
public:
	LZ5Decompressor(const Buffer &packedData);
	virtual ~LZ5Decompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

private:
	const Buffer	&_packedData;
};

}

#endif
