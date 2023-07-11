/* Copyright (C) Teemu Suutari */

#include "InputStream.hpp"
// for exceptions
#include "Decompressor.hpp"
#include "common/OverflowCheck.hpp"


namespace ancient::internal
{

ForwardInputStream::ForwardInputStream(const Buffer &buffer,size_t startOffset,size_t endOffset,bool allowOverrun) :
	_buffer(buffer),
	_currentOffset(startOffset),
	_endOffset(endOffset),
	_allowOverrun(allowOverrun)
{
	if (_currentOffset>_endOffset || _currentOffset>_buffer.size() || _endOffset>_buffer.size())
		throw Decompressor::DecompressionError();
}

ForwardInputStream::~ForwardInputStream()
{
	// nothing needed
}

void ForwardInputStream::reset(size_t startOffset,size_t endOffset)
{
	_currentOffset=startOffset;
	_endOffset=endOffset;
	if (_currentOffset>_endOffset || _currentOffset>_buffer.size() || _endOffset>_buffer.size())
		throw Decompressor::DecompressionError();

	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
}

uint8_t ForwardInputStream::readByte()
{
	if (_currentOffset>=_endOffset)
	{
		if (_allowOverrun)
		{
			_currentOffset++;
			return 0;
		}
		throw Decompressor::DecompressionError();
	}
	uint8_t ret=_buffer[_currentOffset++];
	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
	return ret;
}

const uint8_t *ForwardInputStream::consume(size_t bytes,uint8_t *buffer)
{
	if (OverflowCheck::sum(_currentOffset,bytes)>_endOffset)
	{
		if (_allowOverrun && buffer)
		{
			for (size_t i=0;i<bytes;i++)
			{
				buffer[i]=(_currentOffset<_endOffset)?_buffer[_currentOffset]:0;
				_currentOffset++;
			}
			return buffer;
		}
		throw Decompressor::DecompressionError();
	}
	const uint8_t *ret=&_buffer[_currentOffset];
	_currentOffset+=bytes;
	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
	return ret;
}

void ForwardInputStream::setOffset(size_t offset)
{
	if (offset>_endOffset)
		throw Decompressor::DecompressionError();
	_currentOffset=offset;
	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
}

BackwardInputStream::BackwardInputStream(const Buffer &buffer,size_t startOffset,size_t endOffset,bool allowOverrun) :
	_buffer(buffer),
	_currentOffset(endOffset),
	_endOffset(startOffset),
	_allowOverrun(allowOverrun)
{
	if (_currentOffset<_endOffset || _currentOffset>buffer.size() || _endOffset>buffer.size()) throw Decompressor::DecompressionError();
}

BackwardInputStream::~BackwardInputStream()
{
	// nothing needed
}

uint8_t BackwardInputStream::readByte()
{
	if (_currentOffset<=_endOffset)
	{
		if (_allowOverrun)
		{
			--_currentOffset;
			return 0;
		}
		throw Decompressor::DecompressionError();
	}
	uint8_t ret=_buffer[--_currentOffset];
	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
	return ret;
}

const uint8_t *BackwardInputStream::consume(size_t bytes,uint8_t *buffer)
{
	if (_currentOffset<OverflowCheck::sum(_endOffset,bytes))
	{
		if (_allowOverrun && buffer)
		{
			for (size_t i=bytes;i;i--)
			{
				buffer[i-1]=(_currentOffset>_endOffset)?_buffer[_currentOffset-1]:0;
				--_currentOffset;
			}
			return buffer;
		}
		throw Decompressor::DecompressionError();
	}
	_currentOffset-=bytes;
	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
	return &_buffer[_currentOffset];
}

void BackwardInputStream::setOffset(size_t offset)
{
	if (offset<_endOffset)
		throw Decompressor::DecompressionError();
	_currentOffset=offset;
	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
}

}
