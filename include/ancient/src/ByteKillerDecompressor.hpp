/* Copyright (C) Teemu Suutari */

#ifndef BYTEKILLERDECOMPRESSOR_HPP
#define BYTEKILLERDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class ByteKillerDecompressor : public Decompressor
{
public:
	ByteKillerDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify);
	~ByteKillerDecompressor() noexcept=default;

	const std::string &getName() const noexcept final;
	size_t getPackedSize() const noexcept final;
	size_t getRawSize() const noexcept final;

	void decompressImpl(Buffer &rawData,bool verify) final;

	static bool detectHeader(uint32_t hdr,uint32_t footer) noexcept;
	static std::shared_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);

private:
	enum class Variant {
		BK_STD=0,
		BK_PRO,
		BK_ACE,		// ACE - header variant 1
		BK_GRAC,	// GRAC - header variant 2
		BK_ANC,		// different program - ANC Cruncher (header variant 2)
		BK_MD10,	// header variant 2 - with MD-specifics
		BK_MD11,	// header variant 2 - with MD-specifics
		BK_JEK		// JEK/JAM v1 - no header, only footer
	};

	static bool detectHeader_Int(uint32_t hdr,Variant &variant) noexcept;
	static bool detectFooter_Int(uint32_t footer,Variant &variant) noexcept;

	const Buffer	&_packedData;

	uint32_t	_rawSize{0};
	uint32_t	_packedSize{0};

	uint32_t	_streamOffset{0};
	uint32_t	_streamEndOffset{0};

	Variant		_variant{Variant::BK_STD};
};

}

#endif
