/* Copyright (C) Teemu Suutari */

#include <cstdint>
#include <cstring>

#include "ZIPDecompressor.hpp"

#include "../BZIP2Decompressor.hpp"
#include "../DEFLATEDecompressor.hpp"
#include "ImplodeDecompressor.hpp"
#include "ReduceDecompressor.hpp"
#include "ShrinkDecompressor.hpp"


namespace ancient::internal
{

ZIPDecompressor::ZIPDecompressor(const Buffer &packedData,uint32_t method,uint32_t flags) :
	_packedData(packedData),
	_method(method),
	_flags(flags)
{

}

ZIPDecompressor::~ZIPDecompressor()
{
	// nothing needed
}

size_t ZIPDecompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t ZIPDecompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &ZIPDecompressor::getName() const noexcept
{
	static std::string name="Zip";
	return name;
}

void ZIPDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	switch (_method)
	{
		case 0:
		if (rawData.size()!=_packedData.size()) throw DecompressionError();
		std::memcpy(rawData.data(),_packedData.data(),rawData.size());
		break;

		case 1:
		{
			ShrinkDecompressor dec(_packedData);
			dec.decompress(rawData,verify);
		}
		break;

		case 2:
		case 3:
		case 4:
		case 5:
		{
			ReduceDecompressor dec(_packedData,_method);
			dec.decompress(rawData,verify);
		}
		break;

		case 6:
		{
			ImplodeDecompressor dec(_packedData,_flags);
			dec.decompress(rawData,verify);
		}
		break;

		case 8:
		case 9:
		{
			DEFLATEDecompressor dec(_packedData,_packedData.size(),rawData.size(),false,verify,_method==9);
			dec.decompress(rawData,verify);
		}
		break;

		case 12:
		{
			BZIP2Decompressor dec(_packedData,true,verify);
			dec.decompress(rawData,verify);
		}
		break;

		case 14:
		// LZMA
		throw DecompressionError();
		break;

		case 97:
		// WavPack
		throw DecompressionError();
		break;

		case 98:
		// PPMd
		throw DecompressionError();
		break;

		default:
		throw DecompressionError();
	}
}

}
