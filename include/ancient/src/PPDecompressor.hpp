/* Copyright (C) Teemu Suutari */

#ifndef PPDECOMPRESSOR_HPP
#define PPDECOMPRESSOR_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"
#include "InputStream.hpp"

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
	PPDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::shared_ptr<XPKDecompressor::State> &state,bool verify);
	virtual ~PPDecompressor();

	virtual const std::string &getName() const noexcept override final;
	virtual const std::string &getSubName() const noexcept override final;

	virtual size_t getPackedSize() const noexcept override final;
	virtual size_t getRawSize() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;
	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;
	static bool detectHeaderXPK(uint32_t hdr) noexcept;

	static std::shared_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);
	static std::shared_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::shared_ptr<XPKDecompressor::State> &state,bool verify);

private:
	class DoneException : public std::exception
	{
	public:
		DoneException(uint32_t key) noexcept : _key(key) {}
		virtual ~DoneException() {}

		uint32_t getKey() const noexcept { return _key; }

	private:
		uint32_t	_key;
	};

	void findKeyRound(BackwardInputStream &inputStream,LSBBitReader<BackwardInputStream> &bitReader,uint32_t keyBits,uint32_t keyMask,uint32_t outputPosition);
	void findKey(uint32_t keyBits,uint32_t keyMask);

	const Buffer	&_packedData;

	size_t		_dataStart=0;
	size_t		_rawSize=0;
	uint8_t		_startShift=0;
	uint8_t		_modeTable[4];
	bool		_isObsfuscated=false;
	bool		_isXPK=false;
};

}

#endif
