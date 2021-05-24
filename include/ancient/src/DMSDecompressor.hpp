/* Copyright (C) Teemu Suutari */

#ifndef DMSDECOMPRESSOR_HPP
#define DMSDECOMPRESSOR_HPP

#include "Decompressor.hpp"

#include "common/MemoryBuffer.hpp"

namespace ancient::internal
{


class DMSDecompressor : public Decompressor
{
public:
	DMSDecompressor(const Buffer &packedData,bool verify);

	virtual ~DMSDecompressor();

	virtual const std::string &getName() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;
	virtual size_t getRawSize() const noexcept override final;

	virtual size_t getImageSize() const noexcept override final;
	virtual size_t getImageOffset() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;
	static std::unique_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);

private:
	void decompressImpl(Buffer &rawData,bool verify,uint32_t &restartPosition);

	class ShortInputError : public Error
	{
		// nothing needed
	};

	const Buffer	&_packedData;

	uint32_t	_packedSize=0;
	uint32_t	_rawSize=0;
	uint32_t	_contextBufferSize=0;
	uint32_t	_tmpBufferSize=0;
	uint32_t	_imageSize;
	uint32_t	_rawOffset;
	uint32_t	_minTrack;
	bool		_isHD;
	bool		_isObsfuscated;

	static Decompressor::Registry<DMSDecompressor> _registration;
};

}

#endif
