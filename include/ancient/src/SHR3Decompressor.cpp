/* Copyright (C) Teemu Suutari */

#include "SHR3Decompressor.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

SHR3Decompressor::SHR3State::SHR3State() noexcept
{
	for (uint32_t i=0;i<999;i++) ar[i]=0;
}

SHR3Decompressor::SHR3State::~SHR3State()
{
	// nothing needed
}

bool SHR3Decompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("SHR3");
}

std::unique_ptr<XPKDecompressor> SHR3Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<SHR3Decompressor>(hdr,recursionLevel,packedData,state,verify);
}

SHR3Decompressor::SHR3Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData),
	_state(state)
{
	if (!detectHeaderXPK(hdr) || _packedData.size()<6) throw Decompressor::InvalidFormatError();
	_ver=_packedData.read8(0);
	if (!_ver || _ver>2) throw Decompressor::InvalidFormatError();

	if (!_state)
	{
		if (_ver==2) throw Decompressor::InvalidFormatError();
		_state.reset(new SHR3State());
	}
}

SHR3Decompressor::~SHR3Decompressor()
{
	// nothing needed
}

const std::string &SHR3Decompressor::getSubName() const noexcept
{
	static std::string name="XPK-SHR3: LZ-compressor with arithmetic encoding";
	return name;
}

void SHR3Decompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	ForwardInputStream inputStream(_packedData,1,_packedData.size());
	auto readByte=[&]()->uint8_t
	{
		return inputStream.readByte();
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());

	// This follows quite closely Choloks pascal reference
	uint32_t ar[999];

	auto resum=[&]()
	{
		for (uint32_t i=498;i;i--)
			ar[i]=ar[i*2]+ar[i*2+1];
	};

	auto init=[&]()
	{
		for (uint32_t i=0;i<499;i++) ar[i]=0;
		for (uint32_t i=0;i<256;i++) ar[i+499]=(i<32||i>126)?1:3;
		for (uint32_t i=256+499;i<999;i++) ar[i]=0;
		resum();
	};

	auto update=[&](uint32_t updateIndex,uint32_t increment)
	{
		if (updateIndex>=499) return;
		updateIndex+=499;
		while (updateIndex)
		{
			ar[updateIndex]+=increment;
			updateIndex>>=1;
		}
		if (ar[1]>=0x2000)
		{
			for (uint32_t i=499;i<998;i++)
				if (ar[i]) ar[i]=(ar[i]>>1)+1;
			resum();
		}
	};

	auto scale=[&](uint32_t b,uint32_t mult)->uint32_t
	{
		if (!b) throw Decompressor::DecompressionError();
		uint32_t tmp=(0x10000U/b);
		uint32_t tmp2=((0x10000U%b)<<16)/b;
		return ((mult&0xffffU)*tmp>>16)+((mult>>16)*tmp2>>16)+(mult>>16)*tmp;
	};

	uint32_t vlen=0,vnext=0;

	auto upgrade=[&]()
	{
		if (vnext>=65532)
		{
			vnext=~0U;
		} else if (!vlen) {
			vnext=1;
		} else {
			uint32_t vvalue=vnext-1;
			if (vvalue<48) update(vvalue+256,1);
			uint32_t bits=0,compare=4;
			while (vvalue>=compare)
			{
				vvalue-=compare;
				compare<<=1;
				bits++;
			}
			if (bits>=14)
			{
				vnext=~0U;
			} else {
				if (!vvalue)
				{
					if (bits<7)
					{
						for (uint32_t i=304;i<=307;i++)
							update((bits<<2)+i,1);
					}
					if (bits<13)
					{
						for (uint32_t i=332;i<=333;i++)
							update((bits<<1)+i,1);
					}
					static const uint32_t updates1[6]={358,359,386,387,414,415};
					static const uint32_t updates2[4]={442,456,470,484};
					for (auto it : updates1)
						update((bits<<1)+it,1);
					for (auto it : updates2)
						update(bits+it,1);
				}
				if (vnext<49)
				{
					vnext++;
				} else if (vnext==49) {
					vnext=61;
				} else {
					vnext=(vnext<<1)+3;
				}
			}
		}
	};

	uint32_t stream=0,shift=0;

	auto refillStream=[&]()
	{
		while (shift<0x100'0000)
		{
			stream=(stream<<8)|uint32_t(readByte());
			shift<<=8;
		}
	};

	auto getSymbol=[&]()->uint32_t
	{
		if (!(shift>>16)) throw Decompressor::DecompressionError();
		uint32_t vvalue=(stream/(shift>>16))&0xffff;
		uint32_t threshold=(ar[1]*vvalue)>>16;
		uint32_t arIndex=1;
		uint32_t result=0;
		do {
			arIndex<<=1;
			uint32_t tmp=ar[arIndex]+result;
			if (threshold>=tmp)
			{
				result=tmp;
				arIndex++;
			}
		} while (arIndex<499);
		uint32_t rawValue=scale(ar[1],shift);
		uint32_t newValue=rawValue*result;
		if (newValue>stream)
		{
			while (newValue>stream)
			{
				if (--arIndex<499) arIndex+=499;
				result-=ar[arIndex];
				newValue=rawValue*result;
			}
		} else {
			result+=ar[arIndex];
			while (result<ar[1])
			{
				uint32_t compare=rawValue*result;
				if (stream<compare) break;
				if (++arIndex>=998) arIndex-=499;
				result+=ar[arIndex];
				newValue=compare;
			}
		}
		stream-=newValue;
		shift=rawValue*ar[arIndex];
		uint32_t addition=(ar[1]>>10)+3;
		arIndex-=499;
		update(arIndex,addition);
		refillStream();
		return arIndex;
	};
	
	auto getCode=[&](uint32_t size)->uint32_t
	{
		uint32_t ret=0;
		while (size--)
		{
			ret<<=1;
			shift>>=1;
			if (stream>=shift)
			{
				ret++;
				stream-=shift;
			}
			refillStream();
		}
		return ret;
	};

	if (_ver==1)
	{
		init();
		update(498,1);

		shift=0x8000'0000U;
	} else {
		SHR3State *state=static_cast<SHR3State*>(_state.get());
		vlen=state->vlen;
		vnext=state->vnext;
		shift=state->shift;
		for (uint32_t i=0;i<999;i++) ar[i]=state->ar[i];
	}

	{
		const uint8_t *buf=inputStream.consume(4);
		stream=(uint32_t(buf[0])<<24)|(uint32_t(buf[1])<<16)|
			(uint32_t(buf[2])<<8)|uint32_t(buf[3]);
	}

	while (!outputStream.eof())
	{
		while (vlen>=vnext) upgrade();
		uint32_t code=getSymbol();
		if (code<256)
		{
			outputStream.writeByte(code);
			vlen++;
		} else {
		 	auto distanceAddition=[](uint32_t i)->uint32_t
		 	{
		 		return ((1<<(i+2))-1)&~0x3U;
		 	};

		 	uint32_t count,distance;
			if (code<304)
			{
				count=2;
				distance=code-255;
			} else if (code<332) {
				uint32_t tmp=code-304;
				uint32_t extra=getCode(tmp>>2);
				distance=((extra<<2)|(tmp&3))+distanceAddition(tmp>>2)+1;
				count=3;
			} else if (code<358) {
				uint32_t tmp=code-332;
				uint32_t extra=getCode((tmp>>1)+1);
				distance=((extra<<1)|(tmp&1))+distanceAddition(tmp>>1)+1;
				count=4;
			} else if (code<386) {
				uint32_t tmp=code-358;
				uint32_t extra=getCode((tmp>>1)+1);
				distance=((extra<<1)|(tmp&1))+distanceAddition(tmp>>1)+1;
				count=5;
			} else if (code<414) {
				uint32_t tmp=code-386;
				uint32_t extra=getCode((tmp>>1)+1);
				distance=((extra<<1)|(tmp&1))+distanceAddition(tmp>>1)+1;
				count=6;
			} else if (code<442) {
				uint32_t tmp=code-414;
				uint32_t extra=getCode((tmp>>1)+1);
				distance=((extra<<1)|(tmp&1))+distanceAddition(tmp>>1)+1;
				count=7;
			} else if (code<498) {
				uint32_t tmp=code-442;
				uint32_t d=tmp/14;
				uint32_t m=tmp%14;
				count=getCode(d+2)+distanceAddition(d)+8;
				distance=getCode(m+2)+distanceAddition(m)+1;
			} else {
				count=getCode(16);
				distance=getCode(16);
			}
			vlen+=count;
			if (!count) throw Decompressor::DecompressionError();
			outputStream.copy(distance,count,previousData);
		}
	}

	SHR3State *state=static_cast<SHR3State*>(_state.get());
	state->vlen=vlen;
	state->vnext=vnext;
	state->shift=shift;
	for (uint32_t i=0;i<999;i++) state->ar[i]=ar[i];
}

XPKDecompressor::Registry<SHR3Decompressor> SHR3Decompressor::_XPKregistration;

}
