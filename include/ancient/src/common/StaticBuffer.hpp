/* Copyright (C) Teemu Suutari */

#ifndef STATICBUFFER_HPP
#define STATICBUFFER_HPP

#include <cstddef>
#include <cstdint>

#include "Buffer.hpp"

namespace ancient::internal
{

template<size_t N>
class StaticBuffer : public Buffer
{
public:
	StaticBuffer(const StaticBuffer&)=delete;
	StaticBuffer& operator=(const StaticBuffer&)=delete;

	StaticBuffer() { }
	
	virtual ~StaticBuffer() { }

	virtual const uint8_t *data() const noexcept override
	{
		return _data;
	}

	virtual uint8_t *data() override
	{
		return _data;
	}

	virtual size_t size() const noexcept override
	{
		return N;
	}

	virtual bool isResizable() const noexcept override
	{
		return false;
	}

private:
	uint8_t 	_data[N];
};


class ConstStaticBuffer : public Buffer
{
public:
	ConstStaticBuffer(const ConstStaticBuffer&)=delete;
	ConstStaticBuffer& operator=(const ConstStaticBuffer&)=delete;

	ConstStaticBuffer(const uint8_t *data,size_t length);
	virtual ~ConstStaticBuffer();

	virtual const uint8_t *data() const noexcept override;
	virtual uint8_t *data() override;

	virtual size_t size() const noexcept override;
	virtual bool isResizable() const noexcept override;

private:
	const uint8_t 	*_data;
	size_t		_length;
};

}

#endif
