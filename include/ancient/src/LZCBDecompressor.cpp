/* Copyright (C) Teemu Suutari */

#include <array>

#include "LZCBDecompressor.hpp"
#include "RangeDecoder.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

template<size_t T>
class FrequencyTree
{
public:
	FrequencyTree()
	{
		for (uint32_t i=0;i<_size;i++)
			_tree[i]=0;
	}

	~FrequencyTree()
	{
		// nothing needed
	}

	uint16_t decode(uint16_t value,uint16_t &low,uint16_t &freq) const
	{
		if (value>=_tree[_size-1])
			throw Decompressor::DecompressionError();
		uint16_t symbol=0;
		low=0;
		for (uint32_t i=_levels-2;;i--)
		{
			uint16_t tmp=_tree[_levelOffsets[i]+symbol];
			if (uint32_t(symbol+1)<_levelSizes[i] && value>=tmp)
			{
				symbol++;
				low+=tmp;
				value-=tmp;
			}
			if (!i) break;
			symbol<<=1;
		}
		freq=_tree[symbol];
		return symbol;
	}

	bool exists(uint16_t symbol) const
	{
		return _tree[symbol];
	}
	
	void increment(uint16_t symbol)
	{
		for (uint16_t i=0;i<_levels;i++)
		{
			_tree[_levelOffsets[i]+symbol]++;
			symbol>>=1;
		}
	}

	void halve()
	{
		// non-standard way
		for (uint32_t i=0;i<T;i++)
			_tree[i]>>=1;
		for (uint32_t i=T;i<_size;i++)
			_tree[i]=0;
		for (uint32_t i=0,length=T;i<_levels-1;i++,length=(length+1)>>1)
		{
			for (uint32_t j=0;j<length;j++)
				_tree[_levelOffsets[i+1]+(j>>1)]+=_tree[_levelOffsets[i]+j];
		}
	}

	uint32_t getTotal() const
	{
		return _tree[_size-1];
	}

private:
	static constexpr uint32_t levelSize(uint32_t level)
	{
		uint32_t ret=T;
		for (uint32_t i=0;i<level;i++)
		{
			ret=(ret+1)>>1;
		}
		return ret;
	}

	static constexpr uint32_t levels()
	{
		uint32_t ret=0;
		while (levelSize(ret)!=1) ret++;
		return ret+1;
	}

	static constexpr uint32_t size()
	{
		uint32_t ret=0;
		for (uint32_t i=0;i<levels();i++)
			ret+=levelSize(i);
		return ret;
	}

	static constexpr uint32_t levelOffset(uint32_t level)
	{
		uint32_t ret=0;
		for (uint32_t i=0;i<level;i++)
			ret+=levelSize(i);
		return ret;
	}

	template<uint32_t... I>
	static constexpr auto makeLevelOffsetSequence(std::integer_sequence<uint32_t,I...>)
	{
		return std::integer_sequence<uint32_t,levelOffset(I)...>{};
	}

	template<uint32_t... I>
	static constexpr auto makeLevelSizeSequence(std::integer_sequence<uint32_t,I...>)
	{
		return std::integer_sequence<uint32_t,levelSize(I)...>{};
	}
 	
	template<uint32_t... I>
	static constexpr std::array<uint32_t,sizeof...(I)> makeArray(std::integer_sequence<uint32_t,I...>)
	{
		return std::array<uint32_t,sizeof...(I)>{{I...}};
	}

	static constexpr uint32_t			_size=size();
	static constexpr uint32_t			_levels=levels();
	static constexpr std::array<uint32_t,_levels>	_levelOffsets=makeArray(makeLevelOffsetSequence(std::make_integer_sequence<uint32_t,levels()>{}));
	static constexpr std::array<uint32_t,_levels>	_levelSizes=makeArray(makeLevelSizeSequence(std::make_integer_sequence<uint32_t,levels()>{}));

	uint16_t					_tree[size()];
};

template<size_t T>
class FrequencyDecoder
{
public:
	FrequencyDecoder(RangeDecoder &decoder) :
		_decoder(decoder)
	{
		// nothing needed
	}

	~FrequencyDecoder()
	{
		// nothing needed
	}

	template<typename F>
	uint16_t decode(F readFunc)
	{
		uint16_t freq=0,symbol,value=_decoder.decode(_threshold+_tree.getTotal());
		if (value>=_threshold)
		{
			uint16_t low;
			symbol=_tree.decode(value-_threshold,low,freq);
			_decoder.scale(_threshold+low,_threshold+low+freq,_threshold+_tree.getTotal());
			if (freq==1 && _threshold>1)
				_threshold--;
		} else {
			_decoder.scale(0,_threshold,_threshold+_tree.getTotal());
			symbol=readFunc();
			// A bug in the encoder
			if (!symbol && _tree.exists(symbol)) symbol=T;
			_threshold++;
		}
		_tree.increment(symbol);
		if (_threshold+_tree.getTotal()>=0x3ffdU)
		{
			_tree.halve();
			_threshold=(_threshold>>1)+1;
		}
		return symbol;
	}

private:
	RangeDecoder					&_decoder;
	FrequencyTree<T+1>				_tree;
	uint16_t					_threshold=1;
};

bool LZCBDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("LZCB");
}

std::unique_ptr<XPKDecompressor> LZCBDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<LZCBDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

LZCBDecompressor::LZCBDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (packedData.size()<2) throw Decompressor::InvalidFormatError();
}

LZCBDecompressor::~LZCBDecompressor()
{
	// nothing needed
}

const std::string &LZCBDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-LZCB: LZ-compressor";
	return name;
}

void LZCBDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	class BitReader : public RangeDecoder::BitReader
	{
	public:
		BitReader(ForwardInputStream &stream) :
			_reader(stream)
		{
			// nothing needed
		}

		virtual ~BitReader()
		{
			// nothing needed
		}

		virtual uint32_t readBit() override final
		{
			return _reader.readBitsBE32(1);
		}

		uint32_t readBits(uint32_t bitCount)
		{
			return _reader.readBitsBE32(bitCount);
		}

	private:
		MSBBitReader<ForwardInputStream>	_reader;
	};

	ForwardInputStream inputStream(_packedData,0,_packedData.size(),true);
	ForwardOutputStream outputStream(rawData,0,rawData.size());
	BitReader bitReader(inputStream);

	RangeDecoder rangeDecoder(bitReader,bitReader.readBits(16));

	// Ugly duplicates
	auto readByte=[&]()->uint16_t
	{
		uint16_t ret=rangeDecoder.decode(0x100U);
		rangeDecoder.scale(ret,ret+1,0x100U);
		return ret;
	};

	auto readCount=[&]()->uint16_t
	{
		uint16_t ret=rangeDecoder.decode(0x101U);
		rangeDecoder.scale(ret,ret+1,0x101U);
		return ret;
	};

	FrequencyDecoder<256> baseLiteralDecoder(rangeDecoder);
	FrequencyDecoder<257> repeatCountDecoder(rangeDecoder);
	FrequencyDecoder<257> literalCountDecoder(rangeDecoder);
	FrequencyDecoder<256> distanceDecoder(rangeDecoder);

	std::unique_ptr<FrequencyDecoder<256>> literalDecoders[256];

	uint8_t ch=baseLiteralDecoder.decode(readByte);
	outputStream.writeByte(ch);
	bool lastIsLiteral=true;
	while (!outputStream.eof())
	{
		uint32_t count=repeatCountDecoder.decode(readCount);
		if (count)
		{
			if (count==0x100U)
			{
				uint32_t tmp;
				do
				{
					tmp=readByte();
					count+=tmp;
				} while (tmp==0xffU);
			}
			count+=lastIsLiteral?5:4;

			uint32_t distance=distanceDecoder.decode(readByte)<<8;
			distance|=readByte();

			ch=outputStream.copy(distance,count);
			lastIsLiteral=false;
		} else {
			uint16_t literalCount;
			do
			{
				literalCount=literalCountDecoder.decode(readCount);
				if (!literalCount) throw Decompressor::DecompressionError();

				for (uint32_t i=0;i<literalCount;i++)
				{
					auto &literalDecoder=literalDecoders[ch];
					if (!literalDecoder) literalDecoder=std::make_unique<FrequencyDecoder<256>>(rangeDecoder);
					ch=literalDecoder->decode([&]()
					{
						return baseLiteralDecoder.decode(readByte);
					});
					outputStream.writeByte(ch);
				}
			} while (literalCount==0x100U);
			lastIsLiteral=true;
		}
	}
}

XPKDecompressor::Registry<LZCBDecompressor> LZCBDecompressor::_XPKregistration;

}
