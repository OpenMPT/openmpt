/* Copyright (C) Teemu Suutari */

#ifndef OUTPUTSTREAM_HPP
#define OUTPUTSTREAM_HPP

#include <cstddef>
#include <cstdint>

#include "common/Buffer.hpp"

namespace ancient::internal
{

class ForwardOutputStream
{
public:
	ForwardOutputStream(Buffer &buffer,size_t startOffset,size_t endOffset);
	~ForwardOutputStream();

	void writeByte(uint8_t value);

	uint8_t copy(size_t distance,size_t count);
	uint8_t copy(size_t distance,size_t count,const Buffer &prevBuffer);
	uint8_t copy(size_t distance,size_t count,uint8_t defaultChar);
	const uint8_t *history(size_t distance) const;
	void produce(const uint8_t *src,size_t bytes);

	bool eof() const { return _currentOffset==_endOffset; }
	size_t getOffset() const { return _currentOffset; }
	size_t getEndOffset() const { return _endOffset; }

private:
	uint8_t		*_bufPtr;
	size_t		_startOffset;
	size_t		_currentOffset;
	size_t		_endOffset;
};

class BackwardOutputStream
{
public:
	BackwardOutputStream(Buffer &buffer,size_t startOffset,size_t endOffset);
	~BackwardOutputStream();

	void writeByte(uint8_t value);

	uint8_t copy(size_t distance,size_t count);

	bool eof() const { return _currentOffset==_startOffset; }
	size_t getOffset() const { return _currentOffset; }

private:
	uint8_t		*_bufPtr;
	size_t		_startOffset;
	size_t		_currentOffset;
	size_t		_endOffset;
};

}

#endif
