/* Copyright (C) Teemu Suutari */

#ifndef JAMPACKERDECOMPRESSOR_HPP
#define JAMPACKERDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class JAMPackerDecompressor : public Decompressor
{
public:
	JAMPackerDecompressor(const Buffer &packedData);
	~JAMPackerDecompressor() noexcept=default;

	const std::string &getName() const noexcept final;
	size_t getPackedSize() const noexcept final;
	size_t getRawSize() const noexcept final;

	void decompressImpl(Buffer &rawData,bool verify) final;

	static bool detectHeader(uint32_t hdr,uint32_t footer) noexcept;
	static std::shared_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);

private:
	enum class Variant {
		JAM_V2=0,
		JAM_LZH,
		JAM_LZW
	};

	void decompressInternalV2(Buffer &rawData);
	void decompressInternalLZH(Buffer &rawData);
	void decompressInternalLZW(Buffer &rawData);

	const Buffer	&_packedData;

	uint32_t	_rawSize{0};
	uint32_t	_packedSize{0};
	Variant		_variant;
};

}

#endif
