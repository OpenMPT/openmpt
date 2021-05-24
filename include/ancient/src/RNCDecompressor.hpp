/* Copyright (C) Teemu Suutari */

#ifndef RNCDECOMPRESSOR_HPP
#define RNCDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class RNCDecompressor : public Decompressor
{
public:
	RNCDecompressor(const Buffer &packedData,bool verify);

	virtual ~RNCDecompressor();

	virtual const std::string &getName() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;
	virtual size_t getRawSize() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;

	static std::unique_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);

private:
	enum class Version
	{
		RNC1Old=0,
		RNC1New,
		RNC2
	};

	void RNC1DecompressOld(Buffer &rawData,bool verify);
	void RNC1DecompressNew(Buffer &rawData,bool verify);
	void RNC2Decompress(Buffer &rawData,bool verify);

	const Buffer	&_packedData;

	uint32_t	_rawSize=0;
	uint32_t	_packedSize=0;
	uint16_t	_rawCRC=0;
	uint8_t		_chunks=0;
	Version		_ver;

	static Decompressor::Registry<RNCDecompressor> _registration;
};

}

#endif
