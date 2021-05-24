/* Copyright (C) Teemu Suutari */

#ifndef PMDECOMPRESSOR_HPP
#define PMDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class PMDecompressor : public Decompressor
{
public:
	PMDecompressor(const Buffer &packedData,uint32_t version);
	virtual ~PMDecompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

private:
	static uint8_t decodeMTF(uint8_t value,uint8_t map[]);
	static void updateMTF(uint8_t value,uint8_t map[]);
	static void createMTFMap(uint8_t map[]);

	void decompressImplPM1(Buffer &rawData,bool verify);
	void decompressImplPM2(Buffer &rawData,bool verify);

	const Buffer	&_packedData;

	uint32_t	_version;
};

}

#endif
