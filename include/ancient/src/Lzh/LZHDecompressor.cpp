/* Copyright (C) Teemu Suutari */

#include <cstdint>
#include <cstring>

#include <map>

#include "LZHDecompressor.hpp"

#include "LH1Decompressor.hpp"
#include "LH2Decompressor.hpp"
#include "LH3Decompressor.hpp"
#include "LHXDecompressor.hpp"
#include "LZ5Decompressor.hpp"
#include "LZSDecompressor.hpp"
#include "PMDecompressor.hpp"


namespace ancient::internal
{

LZHDecompressor::LZHDecompressor(const Buffer &packedData,const std::string &method) :
	_packedData(packedData),
	_method(method)
{
	// nothing needed
}

LZHDecompressor::~LZHDecompressor()
{
	// nothing needed
}

size_t LZHDecompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t LZHDecompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &LZHDecompressor::getName() const noexcept
{
	static std::string name="Lzh";
	return name;
}

void LZHDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	enum class Compressor
	{
		LH0=0,
		LH1,
		LH2,
		LH3,
		LH4,
		LH5,
		LH6,
		LH7,
		LH8,
		LHX,
		LZ4,
		LZ5,
		LZS,
		PM0,
		PM1,
		PM2
	};

	static std::map<std::string,Compressor> compressorMap{
		{"-lh0-",Compressor::LH0},
		{"-lh1-",Compressor::LH1},
		{"-lh2-",Compressor::LH2},
		{"-lh3-",Compressor::LH3},
		{"-lh4-",Compressor::LH4},
		{"-lh5-",Compressor::LH5},
		{"-lh6-",Compressor::LH6},
		{"-lh7-",Compressor::LH7},
		{"-lh8-",Compressor::LH8},
		{"-lhx-",Compressor::LHX},
		{"-lz4-",Compressor::LZ4},
		{"-lz5-",Compressor::LZ5},
		{"-lzs-",Compressor::LZS},
		{"-pm0-",Compressor::PM0},
		{"-pm1-",Compressor::PM1},
		{"-pm2-",Compressor::PM2}
	};

	auto it=compressorMap.find(_method);
	if (it==compressorMap.end()) throw DecompressionError();
	switch (it->second)
	{
		case Compressor::LH0:
		case Compressor::LZ4:
		case Compressor::PM0:
		if (rawData.size()!=_packedData.size()) throw DecompressionError();
		std::memcpy(rawData.data(),_packedData.data(),rawData.size());
		break;

		case Compressor::LH1:
		{
			LH1Decompressor dec(_packedData);
			dec.decompress(rawData,verify);
		}
		break;

		case Compressor::LH2:
		{
			LH2Decompressor dec(_packedData);
			dec.decompress(rawData,verify);
		}
		break;

		case Compressor::LH3:
		{
			LH3Decompressor dec(_packedData);
			dec.decompress(rawData,verify);
		}
		break;

		case Compressor::LH4:
		case Compressor::LH5:
		case Compressor::LH6:
		case Compressor::LH7:
		case Compressor::LH8:
		{
			LHXDecompressor dec(_packedData,static_cast<uint32_t>(it->second)-static_cast<uint32_t>(Compressor::LH4)+4);
			dec.decompress(rawData,verify);
		}
		break;

		case Compressor::LHX:
		{
			LHXDecompressor dec(_packedData);
			dec.decompress(rawData,verify);
		}
		break;

		case Compressor::LZ5:
		{
			LZ5Decompressor dec(_packedData);
			dec.decompress(rawData,verify);
		}
		break;

		case Compressor::LZS:
		{
			LZSDecompressor dec(_packedData);
			dec.decompress(rawData,verify);
		}
		break;

		case Compressor::PM1:
		case Compressor::PM2:
		{
			PMDecompressor dec(_packedData,static_cast<uint32_t>(it->second)-static_cast<uint32_t>(Compressor::PM1)+1);
			dec.decompress(rawData,verify);
		}
		break;
	}
}

}
