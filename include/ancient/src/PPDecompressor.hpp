/* Copyright (C) Teemu Suutari */

#ifndef PPDECOMPRESSOR_HPP
#define PPDECOMPRESSOR_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class PPDecompressor : public Decompressor, public XPKDecompressor
{
private:
	class PPState : public XPKDecompressor::State
	{
	public:
		PPState(uint32_t mode);
		virtual ~PPState();

		uint32_t _cachedMode;
	};

public:
	PPDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify);
	PPDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);
	virtual ~PPDecompressor();

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

	size_t		_dataStart=0;
	size_t		_rawSize=0;
	uint8_t		_startShift=0;
	uint8_t		_modeTable[4];
	bool		_isXPK=false;

	static Decompressor::Registry<PPDecompressor> _registration;
	static XPKDecompressor::Registry<PPDecompressor> _XPKregistration;
};

}

#endif
