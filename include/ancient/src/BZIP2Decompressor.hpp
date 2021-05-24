/* Copyright (C) Teemu Suutari */

#ifndef BZIP2DECOMPRESSOR_HPP
#define BZIP2DECOMPRESSOR_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class BZIP2Decompressor : public Decompressor, public XPKDecompressor
{
public:
	BZIP2Decompressor(const Buffer &packedData,bool exactSizeKnown,bool verify);
	BZIP2Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);
	virtual ~BZIP2Decompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;
	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;
	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;
	static bool detectHeaderXPK(uint32_t hdr) noexcept;

	static std::unique_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer		&_packedData;

	size_t			_blockSize=0;
	size_t			_packedSize=0;
	size_t			_rawSize=0;

	static Decompressor::Registry<BZIP2Decompressor> _registration;
	static XPKDecompressor::Registry<BZIP2Decompressor> _XPKregistration;
};

}

#endif
