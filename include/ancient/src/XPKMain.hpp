/* Copyright (C) Teemu Suutari */

#ifndef XPKMAIN_HPP
#define XPKMAIN_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"

#include <vector>

namespace ancient::internal
{

class XPKMain : public Decompressor
{
friend class XPKDecompressor;
public:
	XPKMain(const Buffer &packedData,bool verify,uint32_t recursionLevel);

	virtual ~XPKMain();

	virtual const std::string &getName() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;
	virtual size_t getRawSize() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;

	static std::unique_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);

	// Can be used to create directly decoder for chunk (needed by CYB2)
	static std::unique_ptr<XPKDecompressor> createDecompressor(uint32_t type,uint32_t recursionLevel,const Buffer &buffer,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	static void registerDecompressor(bool(*detect)(uint32_t),std::unique_ptr<XPKDecompressor>(*create)(uint32_t,uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&,bool));
	static constexpr uint32_t getMaxRecursionLevel() noexcept { return 4; }

	template <typename F>
	void forEachChunk(F func) const;

	const Buffer	&_packedData;

	uint32_t	_packedSize=0;
	uint32_t	_rawSize=0;
	uint32_t	_headerSize=0;
	uint32_t	_type=0;
	bool		_longHeaders=false;
	uint32_t	_recursionLevel=0;

	static std::vector<std::pair<bool(*)(uint32_t),std::unique_ptr<XPKDecompressor>(*)(uint32_t,uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&,bool)>> *_XPKDecompressors;

	static Decompressor::Registry<XPKMain> _registration;
};

}

#endif
