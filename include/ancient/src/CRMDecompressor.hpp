/* Copyright (C) Teemu Suutari */

#ifndef CRMDECOMPRESSOR_HPP
#define CRMDECOMPRESSOR_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class CRMDecompressor : public Decompressor, public XPKDecompressor
{
public:
	CRMDecompressor(const Buffer &packedData,uint32_t recursionLevel,bool verify);
	CRMDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);
	virtual ~CRMDecompressor();

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

	uint32_t	_packedSize=0;
	uint32_t	_rawSize=0;
	bool		_isLZH=false;		// "normal" compression or LZH compression
	bool		_isSampled=false;	// normal or "sampled" i.e. obsfuscated
	bool		_isXPKDelta=false;	// If delta encoding defined in XPK

	static Decompressor::Registry<CRMDecompressor> _registration;
	static XPKDecompressor::Registry<CRMDecompressor> _XPKregistration;
};

}

#endif
