/* Copyright (C) Teemu Suutari */

#ifndef IMPDECOMPRESSOR_HPP
#define IMPDECOMPRESSOR_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class IMPDecompressor : public Decompressor, public XPKDecompressor
{
public:
	IMPDecompressor(const Buffer &packedData,bool verify);
	IMPDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);
	virtual ~IMPDecompressor();

	virtual const std::string &getName() const noexcept override final;
	virtual const std::string &getSubName() const noexcept override final;

	virtual size_t getPackedSize() const noexcept override final;
	virtual size_t getRawSize() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;
	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;
	static bool detectHeaderXPK(uint32_t hdr) noexcept;

	static std::unique_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	uint32_t	_rawSize=0;
	uint32_t	_endOffset=0;
	bool		_isXPK=false;

	static Decompressor::Registry<IMPDecompressor> _registration;
	static XPKDecompressor::Registry<IMPDecompressor> _XPKregistration;
};

}

#endif
