/* Copyright (C) Teemu Suutari */

#ifndef LH2DECOMPRESSOR_HPP
#define LH2DECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class LH2Decompressor : public Decompressor
{
public:
	LH2Decompressor(const Buffer &packedData);
	virtual ~LH2Decompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

private:
	const Buffer	&_packedData;
};

}

#endif
