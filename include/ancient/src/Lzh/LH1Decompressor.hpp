/* Copyright (C) Teemu Suutari */

#ifndef LH1DECOMPRESSOR_HPP
#define LH1DECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class LH1Decompressor : public Decompressor
{
public:
	LH1Decompressor(const Buffer &packedData);
	virtual ~LH1Decompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

private:
	const Buffer	&_packedData;
};

}

#endif
