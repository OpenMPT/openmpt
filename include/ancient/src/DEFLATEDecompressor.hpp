/* Copyright (C) Teemu Suutari */

#ifndef DEFLATEDECOMPRESSOR_HPP
#define DEFLATEDECOMPRESSOR_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class DEFLATEDecompressor : public Decompressor, public XPKDecompressor
{
public:
	DEFLATEDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify);
	DEFLATEDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);
	DEFLATEDecompressor(const Buffer &packedData,size_t packedSize,size_t rawSize,bool isZlib,bool verify,bool deflate64);		// zlib or completely raw stream
	virtual ~DEFLATEDecompressor();

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
	bool detectZLib();

	enum class Type
	{
		GZIP=0,
		ZLib,
		Raw
	};

	const Buffer	&_packedData;

	size_t		_packedSize=0;
	size_t		_packedOffset=0;
	size_t		_rawSize=0;
	Type		_type;
	bool		_exactSizeKnown=true;
	bool		_deflate64=false;

	static Decompressor::Registry<DEFLATEDecompressor> _registration;
	static XPKDecompressor::Registry<DEFLATEDecompressor> _XPKregistration;
};

}

#endif
