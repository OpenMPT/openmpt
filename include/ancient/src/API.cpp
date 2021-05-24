/* Copyright (C) Teemu Suutari */

#include "ancient.hpp"
#include "Decompressor.hpp"
#include "common/Buffer.hpp"
#include "common/StaticBuffer.hpp"
#include "common/WrappedVectorBuffer.hpp"

#include <memory>

namespace ancient
{

namespace internal
{

namespace APIv2
{

class DecompressorImpl
{
public:
	ConstStaticBuffer _buffer;
	std::unique_ptr<Decompressor> _decompressor;
public:
	DecompressorImpl(const std::vector<uint8_t> &packedData,bool exactSizeKnown,bool verify) :
		_buffer(packedData.data(), packedData.size()),
		_decompressor(Decompressor::create(_buffer, exactSizeKnown, verify))
	{
		return;
	}
	DecompressorImpl(const uint8_t *packedData,size_t packedSize,bool exactSizeKnown,bool verify) :
		_buffer(packedData, packedSize),
		_decompressor(Decompressor::create(_buffer, exactSizeKnown, verify))
	{
		return;
	}
};

}

}

inline namespace APIv2
{

Error::Error() noexcept
{
	// nothing needed
}

Error::~Error()
{
	// nothing needed
}

InvalidFormatError::InvalidFormatError() noexcept
{
	// nothing needed
}

InvalidFormatError::~InvalidFormatError()
{
	// nothing needed
}

DecompressionError::DecompressionError() noexcept
{
	// nothing needed
}

DecompressionError::~DecompressionError()
{
	// nothing needed
}

VerificationError::VerificationError() noexcept
{
	// nothing needed
}

VerificationError::~VerificationError()
{
	// nothing needed
}

// ---

bool Decompressor::detect(const std::vector<uint8_t> &packedData) noexcept
{
	return internal::Decompressor::detect(internal::ConstStaticBuffer(packedData.data(), packedData.size()));
}

bool Decompressor::detect(const uint8_t *packedData, size_t packedSize) noexcept
{
	return internal::Decompressor::detect(internal::ConstStaticBuffer(packedData, packedSize));
}

Decompressor::Decompressor(const std::vector<uint8_t> &packedData,bool exactSizeKnown,bool verify) :
	m_impl(std::make_unique<internal::APIv2::DecompressorImpl>(packedData, exactSizeKnown, verify))
{
	return;
}

Decompressor::Decompressor(const uint8_t *packedData,size_t packedSize,bool exactSizeKnown,bool verify) :
	m_impl(std::make_unique<internal::APIv2::DecompressorImpl>(packedData, packedSize, exactSizeKnown, verify))
{
	return;
}

const std::string &Decompressor::getName() const noexcept
{
	return m_impl->_decompressor->getName();
}

size_t Decompressor::getMaxPackedSize() noexcept
{
	return internal::Decompressor::getMaxPackedSize();
}

size_t Decompressor::getMaxRawSize() noexcept
{
	return internal::Decompressor::getMaxRawSize();
}

std::optional<size_t> Decompressor::getPackedSize() const noexcept
{
	size_t packedSize=m_impl->_decompressor->getPackedSize();
	if (packedSize==0)
	{
		return std::nullopt;
	}
	return packedSize;
}

std::optional<size_t> Decompressor::getRawSize() const noexcept
{
	size_t rawSize=m_impl->_decompressor->getRawSize();
	if (rawSize==0)
	{
		return std::nullopt;
	}
	return rawSize;
}

std::optional<size_t> Decompressor::getImageSize() const noexcept
{
	size_t imageSize=m_impl->_decompressor->getImageSize();
	size_t imageOffset=m_impl->_decompressor->getImageOffset();
	bool isImage=((imageSize>0)||(imageOffset>0));
	if (!isImage)
	{
		return std::nullopt;
	}
	return imageSize;
}

std::optional<size_t> Decompressor::getImageOffset() const noexcept
{
	size_t imageSize=m_impl->_decompressor->getImageSize();
	size_t imageOffset=m_impl->_decompressor->getImageOffset();
	bool isImage=((imageSize>0)||(imageOffset>0));
	if (!isImage)
	{
		return std::nullopt;
	}
	return imageOffset;
}

std::vector<uint8_t> Decompressor::decompress(bool verify)
{
	std::vector<uint8_t> result((m_impl->_decompressor->getRawSize())?m_impl->_decompressor->getRawSize():m_impl->_decompressor->getMaxRawSize());
	{
		internal::WrappedVectorBuffer buffer(result);
		m_impl->_decompressor->decompress(buffer, verify);
	}
	result.resize(m_impl->_decompressor->getRawSize());
	result.shrink_to_fit();
	return result;
}

Decompressor::~Decompressor()
{
	// nothing needed
}

}

}
