/* Copyright (C) Teemu Suutari */

#ifndef COMPACTDECOMPRESSOR_HPP
#define COMPACTDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class CompactDecompressor : public Decompressor
{
public:
	CompactDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify);
	virtual ~CompactDecompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;

	static std::shared_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);

private:
	const Buffer	&_packedData;

	size_t		_packedSize=0;
	size_t		_rawSize=0;

	bool		_exactSizeKnown;
};

}

#endif
