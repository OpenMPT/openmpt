/* Copyright (C) Teemu Suutari */

#ifndef LHXDECOMPRESSOR_HPP
#define LHXDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class LHXDecompressor : public Decompressor
{
public:
	// by default LHX
	LHXDecompressor(const Buffer &packedDatam);
	LHXDecompressor(const Buffer &packedDatam,uint32_t method);
	virtual ~LHXDecompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

private:
	const Buffer	&_packedData;

	uint32_t	_method;
};

}

#endif
