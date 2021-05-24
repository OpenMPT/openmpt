/* Copyright (C) Teemu Suutari */

#ifndef MMCMPDECOMPRESSOR_HPP
#define MMCMPDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class MMCMPDecompressor : public Decompressor
{
public:
	MMCMPDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify);

	virtual ~MMCMPDecompressor();

	virtual const std::string &getName() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;
	virtual size_t getRawSize() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;
	static std::unique_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);

private:
	const Buffer	&_packedData;

	uint32_t	_packedSize=0;
	uint32_t	_rawSize=0;
	uint32_t	_blocksOffset=0;
	uint32_t	_blocks=0;

	static Decompressor::Registry<MMCMPDecompressor> _registration;
};

}

#endif
