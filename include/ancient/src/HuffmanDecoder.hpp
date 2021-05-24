/* Copyright (C) Teemu Suutari */

#ifndef HUFFMANDECODER_HPP
#define HUFFMANDECODER_HPP

#include <cstddef>
#include <cstdint>

#include <vector>
#include <utility>

// For exception
#include "Decompressor.hpp"

#include "common/MemoryBuffer.hpp"

namespace ancient::internal
{

template<typename T>
struct HuffmanCode
{
	uint32_t	length;
	uint32_t	code;

	T		value;
};

template<typename T> class OptionalHuffmanDecoder;

template<typename T>
class HuffmanDecoder
{
friend class OptionalHuffmanDecoder<T>;
private:
	struct Node
	{
		uint32_t	sub[2];
		T		value;

		Node(uint32_t _sub0,uint32_t _sub1,T _value) :
			sub{_sub0,_sub1},
			value(_value)
		{
			// nothing needed
		}

		Node(Node &&source) :
			sub{source.sub[0],source.sub[1]},
			value(source.value)
		{
			// nothing needed
		}

		Node& operator=(Node &&source)
		{
			if (this!=&source)
			{
				sub[0]=source.sub[0];
				sub[1]=source.sub[1];
				value=source.value;
			}
			return *this;
		}
	};

public:
	HuffmanDecoder()
	{
		// nothing needed
	}

	template<typename ...Args>
	HuffmanDecoder(const Args&& ...args) :
		HuffmanDecoder()
	{
		const HuffmanCode<T> list[sizeof...(args)]={args...};
		for (auto &item : list)
			insert(item);
	}

	~HuffmanDecoder()
	{
	}

	void reset()
	{
		_table.clear();
	}

	template<typename F>
	const T &decode(F bitReader) const
	{
		if (!_table.size()) throw Decompressor::DecompressionError();
		uint32_t i=0;
		while (_table[i].sub[0] || _table[i].sub[1])
		{
			i=_table[i].sub[bitReader()?1:0];
			if (!i) throw Decompressor::DecompressionError();
		}
		return _table[i].value;
	}

	void insert(const HuffmanCode<T> &code)
	{
		uint32_t i=0,length=uint32_t(_table.size());
		for (int32_t currentBit=code.length;currentBit>=0;currentBit--)
		{
			uint32_t codeBit=(currentBit && ((code.code>>(currentBit-1U))&1U))?1U:0;
			if (i!=length)
			{
				if (!currentBit || (!_table[i].sub[0] && !_table[i].sub[1])) throw Decompressor::DecompressionError();
				uint32_t &tmp=_table[i].sub[codeBit];
				if (!tmp) tmp=i=length;
					else i=tmp;
			} else {
				_table.emplace_back((currentBit&&!codeBit)?length+1:0,(currentBit&&codeBit)?length+1:0,currentBit?T():code.value);
				length++;
				i++;
			}
		}
	}

	// create orderly Huffman table, as used by Deflate and Bzip2
	void createOrderlyHuffmanTable(const uint8_t *bitLengths,uint32_t bitTableLength)
	{
		uint8_t minDepth=32,maxDepth=0;
		// some optimization: more tables
		uint16_t firstIndex[33],lastIndex[33];
		MemoryBuffer nextIndexBuffer(bitTableLength*sizeof(uint16_t));
		uint16_t *nextIndex=nextIndexBuffer.cast<uint16_t>();
		for (uint32_t i=1;i<33;i++)
			firstIndex[i]=0xffffU;

		uint32_t realItems=0;
		for (uint32_t i=0;i<bitTableLength;i++)
		{
			uint8_t length=bitLengths[i];
			if (length>32) throw Decompressor::DecompressionError();
			if (length)
			{
				if (length<minDepth) minDepth=length;
				if (length>maxDepth) maxDepth=length;
				if (firstIndex[length]==0xffffU)
				{
					firstIndex[length]=i;
					lastIndex[length]=i;
				} else {
					nextIndex[lastIndex[length]]=i;
					lastIndex[length]=i;
				}
				realItems++;
			}
		}
		if (!maxDepth) throw Decompressor::DecompressionError();
		// optimization, the multiple depends how sparse the tree really is. (minimum is *2)
		// usually it is sparse.
		_table.reserve(realItems*3);

		uint32_t code=0;
		for (uint32_t depth=minDepth;depth<=maxDepth;depth++)
		{
			if (firstIndex[depth]!=0xffffU)
				nextIndex[lastIndex[depth]]=bitTableLength;

			for (uint32_t i=firstIndex[depth];i<bitTableLength;i=nextIndex[i])
			{
				insert(HuffmanCode<T>{depth,code>>(maxDepth-depth),(T)i});
				code+=1<<(maxDepth-depth);
			}
		}
	}

private:
	std::vector<Node>	_table;
};

template<typename T>
class OptionalHuffmanDecoder
{
public:
	OptionalHuffmanDecoder() :
		_base()
	{
		// nothing needed
	}

	~OptionalHuffmanDecoder()
	{
		// nothing needed
	}

	void reset()
	{
		_base.reset();
	}

	void setEmpty(T value)
	{
		reset();
		_emptyValue=value;
	}

	template<typename F>
	T decode(F bitReader) const
	{
		if (!_base._table.size()) return _emptyValue;
			else return _base.decode(bitReader);
	}

	void insert(const HuffmanCode<T> &code)
	{
		_base.insert(code);
	}

	void createOrderlyHuffmanTable(const uint8_t *bitLengths,uint32_t bitTableLength)
	{
		_base.createOrderlyHuffmanTable(bitLengths,bitTableLength);
	}

private:
	HuffmanDecoder<T>	_base;
	T			_emptyValue=0;
};

}

#endif
