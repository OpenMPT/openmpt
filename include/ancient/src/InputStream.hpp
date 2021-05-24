/* Copyright (C) Teemu Suutari */

#ifndef INPUTSTREAM_HPP
#define INPUTSTREAM_HPP

#include <cstddef>
#include <cstdint>

#include <algorithm>

#include "common/Buffer.hpp"

namespace ancient::internal
{

class BackwardInputStream;

class ForwardInputStream
{
	friend class BackwardInputStream;

public:
	ForwardInputStream(const Buffer &buffer,size_t startOffset,size_t endOffset,bool allowOverrun=false);
	~ForwardInputStream();

	uint8_t readByte();
	const uint8_t *consume(size_t bytes,uint8_t *buffer=nullptr);

	bool eof() const { return _currentOffset==_endOffset; }
	size_t getOffset() const { return _currentOffset; }
	size_t getEndOffset() const { return _endOffset; }
	void link(BackwardInputStream &stream) { _linkedInputStream=&stream; }

private:
	void setOffset(size_t offset) { _endOffset=offset; }

	const uint8_t		*_bufPtr;
	size_t			_currentOffset;
	size_t			_endOffset;
	bool			_allowOverrun;

	BackwardInputStream	*_linkedInputStream=nullptr;
};


class BackwardInputStream
{
	friend class ForwardInputStream;
public:
	BackwardInputStream(const Buffer &buffer,size_t startOffset,size_t endOffset,bool allowOverrun=false);
	~BackwardInputStream();

	uint8_t readByte();
	const uint8_t *consume(size_t bytes,uint8_t *buffer=nullptr);

	bool eof() const { return _currentOffset==_endOffset; }
	size_t getOffset() const { return _currentOffset; }
	void link(ForwardInputStream &stream) { _linkedInputStream=&stream; }

private:
	void setOffset(size_t offset) { _endOffset=offset; }

	const uint8_t		*_bufPtr;
	size_t			_currentOffset;
	size_t			_endOffset;
	bool			_allowOverrun;

	ForwardInputStream	*_linkedInputStream=nullptr;
};


template<typename T>
class LSBBitReader
{
public:
	LSBBitReader(T &inputStream) :
		_inputStream(inputStream)
	{
		// nothing needed
	}

	~LSBBitReader()
	{
		// nothing needed
	}

	uint32_t readBits8(uint32_t count)
	{
		return readBitsInternal(count,[&](){
			_bufContent=_inputStream.readByte();
			_bufLength=8;
		});
	}

	uint32_t readBitsBE16(uint32_t count)
	{
		return readBitsInternal(count,[&](){
			uint8_t tmp[2];
			const uint8_t *buf=_inputStream.consume(2,tmp);
			_bufContent=(uint32_t(buf[0])<<8)|uint32_t(buf[1]);
			_bufLength=16;
		});
	}

	uint32_t readBitsBE32(uint32_t count)
	{
		return readBitsInternal(count,[&](){
			uint8_t tmp[4];
			const uint8_t *buf=_inputStream.consume(4,tmp);
			_bufContent=(uint32_t(buf[0])<<24)|(uint32_t(buf[1])<<16)|
				(uint32_t(buf[2])<<8)|uint32_t(buf[3]);
			_bufLength=32;
		});
	}

	// RNC
	uint32_t readBits16Limit(uint32_t count)
	{
		return readBitsInternal(count,[&](){
			_bufContent=_inputStream.readByte();
			if (_inputStream.eof())
			{
				_bufLength=8;
			} else {
				_bufContent=_bufContent|(uint32_t(_inputStream.readByte())<<8);
				_bufLength=16;
			}
		});
	}

	void reset(uint32_t bufContent=0,uint8_t bufLength=0)
	{
		_bufContent=bufContent;
		_bufLength=bufLength;
	}

private:
	template<typename F>
	uint32_t readBitsInternal(uint32_t count,F readWord)
	{
		uint32_t ret=0,pos=0;
		while (count)
		{
			if (!_bufLength)
				readWord();
			uint8_t maxCount=std::min(uint8_t(count),_bufLength);
			ret|=(_bufContent&((1<<maxCount)-1))<<pos;
			_bufContent>>=maxCount;
			_bufLength-=maxCount;
			count-=maxCount;
			pos+=maxCount;
		}
		return ret;
	}

	T			&_inputStream;
	uint32_t		_bufContent=0;
	uint8_t			_bufLength=0;
};


template<typename T>
class MSBBitReader
{
public:
	MSBBitReader(T &inputStream) :
		_inputStream(inputStream)
	{
		// nothing needed
	}

	~MSBBitReader()
	{
		// nothing needed
	}

	uint32_t readBits8(uint32_t count)
	{
		return readBitsInternal(count,[&](){
			_bufContent=_inputStream.readByte();
			_bufLength=8;
		});
	}

	uint32_t readBitsBE16(uint32_t count)
	{
		return readBitsInternal(count,[&](){
			uint8_t tmp[2];
			const uint8_t *buf=_inputStream.consume(2,tmp);
			_bufContent=(uint32_t(buf[0])<<8)|uint32_t(buf[1]);
			_bufLength=16;
		});
	}

	uint32_t readBitsBE32(uint32_t count)
	{
		return readBitsInternal(count,[&](){
			uint8_t tmp[4];
			const uint8_t *buf=_inputStream.consume(4,tmp);
			_bufContent=(uint32_t(buf[0])<<24)|(uint32_t(buf[1])<<16)|
				(uint32_t(buf[2])<<8)|uint32_t(buf[3]);
			_bufLength=32;
		});
	}

	void reset(uint32_t bufContent=0,uint8_t bufLength=0)
	{
		_bufContent=bufContent;
		_bufLength=bufLength;
	}

private:
	template<typename F>
	uint32_t readBitsInternal(uint32_t count,F readWord)
	{
		uint32_t ret=0;
		while (count)
		{
			if (!_bufLength)
				readWord();
			uint8_t maxCount=std::min(uint8_t(count),_bufLength);
			_bufLength-=maxCount;
			ret=(ret<<maxCount)|((_bufContent>>_bufLength)&((1<<maxCount)-1));
			count-=maxCount;
		}
		return ret;
	}

	T			&_inputStream;
	uint32_t		_bufContent=0;
	uint8_t			_bufLength=0;
};

}

#endif
