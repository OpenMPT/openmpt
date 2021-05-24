/* Copyright (C) Teemu Suutari */

#ifndef MEMORYBUFFER_HPP
#define MEMORYBUFFER_HPP

#include <memory>

#include "Buffer.hpp"

namespace ancient::internal
{

class MemoryBuffer : public Buffer
{
public:
	MemoryBuffer(size_t size);
	MemoryBuffer(const Buffer &src,size_t offset,size_t size);
	virtual ~MemoryBuffer() override final;

	virtual const uint8_t *data() const noexcept override final;
	virtual uint8_t *data() override final;
	virtual size_t size() const noexcept override final;

	virtual bool isResizable() const noexcept override final;
	virtual void resize(size_t newSize) override final;

private:
	uint8_t*			_data;
	size_t				_size;
};

}

#endif
