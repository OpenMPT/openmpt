/* Copyright (C) Teemu Suutari */

#ifndef TPWMDECOMPRESSOR_HPP
#define TPWMDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class TPWMDecompressor : public Decompressor
{
public:
	TPWMDecompressor(const Buffer &packedData,bool verify);

	virtual ~TPWMDecompressor();

	virtual const std::string &getName() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;
	virtual size_t getRawSize() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;
	static std::unique_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);

private:
	const Buffer	&_packedData;

	uint32_t	_rawSize=0;
	size_t		_decompressedPackedSize=0;

	static Decompressor::Registry<TPWMDecompressor> _registration;
};

}

#endif
