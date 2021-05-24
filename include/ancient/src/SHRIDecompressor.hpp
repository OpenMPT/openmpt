/* Copyright (C) Teemu Suutari */

#ifndef SHRIDECOMPRESSOR_HPP
#define SHRIDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class SHRIDecompressor : public XPKDecompressor
{
private:
	class SHRIState : public XPKDecompressor::State
	{
	public:
		SHRIState() noexcept;
		virtual ~SHRIState();

		uint32_t vlen=0;
		uint32_t vnext=0;
		uint32_t shift=0;
		uint32_t ar[999];
	};

public:
	SHRIDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~SHRIDecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer				&_packedData;

	uint32_t				_ver=0;
	size_t					_startOffset=0;
	size_t					_rawSize=0;

	std::unique_ptr<XPKDecompressor::State>	&_state;	// reference!!!

	static XPKDecompressor::Registry<SHRIDecompressor> _XPKregistration;
};

}

#endif
