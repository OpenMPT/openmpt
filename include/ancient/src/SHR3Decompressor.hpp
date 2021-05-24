/* Copyright (C) Teemu Suutari */

#ifndef SHR3DECOMPRESSOR_HPP
#define SHR3DECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class SHR3Decompressor : public XPKDecompressor
{
private:
	class SHR3State : public XPKDecompressor::State
	{
	public:
		SHR3State() noexcept;
		virtual ~SHR3State();

		uint32_t vlen=0;
		uint32_t vnext=0;
		uint32_t shift=0;
		uint32_t ar[999];
	};

public:
	SHR3Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~SHR3Decompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer				&_packedData;

	uint32_t				_ver=0;

	std::unique_ptr<XPKDecompressor::State>	&_state;	// reference!!!

	static XPKDecompressor::Registry<SHR3Decompressor> _XPKregistration;
};

}

#endif
