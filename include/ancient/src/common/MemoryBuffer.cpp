/* Copyright (C) Teemu Suutari */

#include <cstring>
#include <cstdlib>

#include <memory>
#include <new>

#include "MemoryBuffer.hpp"
#include "OverflowCheck.hpp"


namespace ancient::internal
{

MemoryBuffer::MemoryBuffer(size_t size) :
	_data(reinterpret_cast<uint8_t*>(std::malloc(size))),
	_size(size)
{
	if (!_data) throw std::bad_alloc();
}

MemoryBuffer::MemoryBuffer(const Buffer &src,size_t offset,size_t size) :
	MemoryBuffer(size)
{
	if(OverflowCheck::sum(offset,size)>src.size()) throw InvalidOperationError();
	std::memcpy(_data,src.data()+offset,size);
}


MemoryBuffer::~MemoryBuffer()
{
	std::free(_data);
}

const uint8_t *MemoryBuffer::data() const noexcept
{
	return _data;
}

uint8_t *MemoryBuffer::data()
{
	return _data;
}

size_t MemoryBuffer::size() const noexcept
{
	return _size;
}

bool MemoryBuffer::isResizable() const noexcept
{
	return true;
}

void MemoryBuffer::resize(size_t newSize) 
{
	_data=reinterpret_cast<uint8_t*>(std::realloc(_data,newSize));
	_size=newSize;
	if (!_data)
	{
		_size=0;
		throw std::bad_alloc();
	}
}

}
