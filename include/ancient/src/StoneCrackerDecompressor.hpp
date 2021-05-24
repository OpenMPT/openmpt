/* Copyright (C) Teemu Suutari */

#ifndef STONECRACKERDECOMPRESSOR_HPP
#define STONECRACKERDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class StoneCrackerDecompressor : public Decompressor
{
public:
	StoneCrackerDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify);

	virtual ~StoneCrackerDecompressor();

	virtual const std::string &getName() const noexcept override final;

	virtual size_t getPackedSize() const noexcept override final;
	virtual size_t getRawSize() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;

	static std::unique_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);

private:
	static bool detectHeaderAndGeneration(uint32_t hdr,uint32_t &generation) noexcept;

	void initialize(const Buffer &packedData,uint32_t hdr);
	void decompressGen1(Buffer &rawData);
	void decompressGen23(Buffer &rawData);
	void decompressGen456(Buffer &rawData);
	void decompressGen7(Buffer &rawData);
	void decompressGen8(Buffer &rawData);

	const Buffer	&_packedData;

	uint32_t	_rawSize=0;
	uint32_t	_packedSize=0;
	uint32_t	_rleSize=0;
	uint8_t		_modes[4];
	uint8_t		_rle[3];
	uint32_t	_generation;
	uint32_t	_dataOffset;

	static Decompressor::Registry<StoneCrackerDecompressor> _registration;
};

}

#endif
