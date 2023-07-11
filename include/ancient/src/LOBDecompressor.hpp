/* Copyright (C) Teemu Suutari */

#ifndef LOBDECOMPRESSOR_HPP
#define LOBDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class LOBDecompressor : public Decompressor
{
public:
	LOBDecompressor(const Buffer &packedData,bool verify);

	virtual ~LOBDecompressor();

	virtual const std::string &getName() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;
	virtual size_t getRawSize() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;
	static std::shared_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);

private:
	static void decompressRound(Buffer &rawData,const Buffer &packedData);

	const Buffer	&_packedData;

	uint32_t	_rawSize=0;
	uint32_t	_packedSize=0;

	uint32_t	_methodCount;
};

}

#endif
