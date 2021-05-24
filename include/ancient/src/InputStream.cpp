/* Copyright (C) Teemu Suutari */

#include "InputStream.hpp"
// for exceptions
#include "Decompressor.hpp"
#include "common/OverflowCheck.hpp"


namespace ancient::internal
{

ForwardInputStream::ForwardInputStream(const Buffer &buffer,size_t startOffset,size_t endOffset,bool allowOverrun) :
	_bufPtr(buffer.data()),
	_currentOffset(startOffset),
	_endOffset(endOffset),
	_allowOverrun(allowOverrun)
{
	if (_currentOffset>_endOffset || _currentOffset>buffer.size() || _endOffset>buffer.size()) throw Decompressor::DecompressionError();
}

ForwardInputStream::~ForwardInputStream()
{
	// nothing needed
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
	uint8_t ret=_bufPtr[_currentOffset++];
	if (_linkedInputStream) _linkedInputStream->setOffset(_currentOffset);
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
				buffer[i]=(_currentOffset<_endOffset)?_bufPtr[_currentOffset]:0;
				_currentOffset++;
			}
			return buffer;
		}
		throw Decompressor::DecompressionError();
	}
	const uint8_t *ret=&_bufPtr[_currentOffset];
	_currentOffset+=bytes;
	if (_linkedInputStream) _linkedInputStream->setOffset(_currentOffset);
	return ret;
}

BackwardInputStream::BackwardInputStream(const Buffer &buffer,size_t startOffset,size_t endOffset,bool allowOverrun) :
	_bufPtr(buffer.data()),
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
	uint8_t ret=_bufPtr[--_currentOffset];
	if (_linkedInputStream) _linkedInputStream->setOffset(_currentOffset);
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
				buffer[i-1]=(_currentOffset>_endOffset)?_bufPtr[_currentOffset-1]:0;
				--_currentOffset;
			}
			return buffer;
		}
		throw Decompressor::DecompressionError();
	}
	_currentOffset-=bytes;
	if (_linkedInputStream) _linkedInputStream->setOffset(_currentOffset);
	return &_bufPtr[_currentOffset];
}

}
