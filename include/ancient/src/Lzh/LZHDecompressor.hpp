/* Copyright (C) Teemu Suutari */

#ifndef LZHDECOMPRESSOR_HPP
#define LZHDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class LZHDecompressor : public Decompressor
{
public:
	LZHDecompressor(const Buffer &packedData,const std::string &method);
	virtual ~LZHDecompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

private:
	const Buffer	&_packedData;

	std::string	_method;
};

}

#endif
