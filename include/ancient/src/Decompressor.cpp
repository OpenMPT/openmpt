/* Copyright (C) Teemu Suutari */

#include "Decompressor.hpp"

#include <memory>
#include <vector>

#include "BZIP2Decompressor.hpp"
#include "CRMDecompressor.hpp"
#include "DEFLATEDecompressor.hpp"
#include "DMSDecompressor.hpp"
#include "IMPDecompressor.hpp"
#include "MMCMPDecompressor.hpp"
#include "PPDecompressor.hpp"
#include "RNCDecompressor.hpp"
#include "StoneCrackerDecompressor.hpp"
#include "TPWMDecompressor.hpp"
#include "XPKMain.hpp"

namespace ancient::internal
{

// ---

static std::vector<std::pair<bool(*)(uint32_t),std::unique_ptr<Decompressor>(*)(const Buffer&,bool,bool)>> decompressors={
	{BZIP2Decompressor::detectHeader,BZIP2Decompressor::create},
	{CRMDecompressor::detectHeader,CRMDecompressor::create},
	{DEFLATEDecompressor::detectHeader,DEFLATEDecompressor::create},
	{DMSDecompressor::detectHeader,DMSDecompressor::create},
	{IMPDecompressor::detectHeader,IMPDecompressor::create},
	{MMCMPDecompressor::detectHeader,MMCMPDecompressor::create},
	{PPDecompressor::detectHeader,PPDecompressor::create},
	{RNCDecompressor::detectHeader,RNCDecompressor::create},
	{StoneCrackerDecompressor::detectHeader,StoneCrackerDecompressor::create},
	{TPWMDecompressor::detectHeader,TPWMDecompressor::create},
	{XPKMain::detectHeader,XPKMain::create}};

Decompressor::Decompressor() noexcept
{
	// nothing needed
}

Decompressor::~Decompressor()
{
	// nothing needed
}

std::unique_ptr<Decompressor> Decompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	try
	{
		uint32_t hdr=packedData.readBE32(0);
		for (auto &it : decompressors)
		{
			if (it.first(hdr)) return it.second(packedData,exactSizeKnown,verify);
		}
		throw InvalidFormatError();
	} catch (const Buffer::Error&) {
		throw InvalidFormatError();
	}
}

bool Decompressor::detect(const Buffer &packedData) noexcept
{
	try
	{
		uint32_t hdr=packedData.readBE32(0);
		for (auto &it : decompressors)
			if (it.first(hdr)) return true;
		return false;
	} catch (const Buffer::Error&) {
		return false;
	}
}

void Decompressor::decompress(Buffer &rawData,bool verify)
{
	// Simplifying the implementation of sub-decompressors. Just let the buffer-exception pass here,
	// and that will get translated into Decompressor exceptions
	try
	{
		decompressImpl(rawData,verify);
	} catch (const Buffer::Error&) {
		throw DecompressionError();
	}
}

size_t Decompressor::getImageSize() const noexcept
{
	return 0;
}

size_t Decompressor::getImageOffset() const noexcept
{
	return 0;
}

size_t Decompressor::getMaxPackedSize() noexcept
{
	return 0x100'0000U;
}

size_t Decompressor::getMaxRawSize() noexcept
{
	return 0x100'0000U;
}

}
