/* Copyright (C) Teemu Suutari */

#include "SXSCDecompressor.hpp"

#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "DLTADecode.hpp"
#include "common/MemoryBuffer.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

SXSCDecompressor::SXSCReader::SXSCReader(ForwardInputStream &stream) :
	_reader(stream)
{
	// nothing needed
}

SXSCDecompressor::SXSCReader::~SXSCReader()
{
	// nothing needed
}

uint32_t SXSCDecompressor::SXSCReader::readBit()
{
	return _reader.readBits8(1);
}


bool SXSCDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("SASC")||hdr==FourCC("SHSC");
}

std::unique_ptr<XPKDecompressor> SXSCDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<SXSCDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

SXSCDecompressor::SXSCDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData),
	_isHSC(hdr==FourCC("SHSC"))
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

SXSCDecompressor::~SXSCDecompressor()
{
	// nothing needed
}

const std::string &SXSCDecompressor::getSubName() const noexcept
{
	static std::string nameASC="XPK-SASC: LZ-compressor with arithmetic and delta encoding";
	static std::string nameHSC="XPK-SHSC: Context modeling compressor";
	return _isHSC?nameHSC:nameASC;
}

void SXSCDecompressor::decompressASC(Buffer &rawData,ForwardInputStream &inputStream)
{
	ForwardOutputStream outputStream(rawData,0,rawData.size());

	uint16_t bitReaderInitialValue;
	bitReaderInitialValue=inputStream.readByte()<<8;
	bitReaderInitialValue|=inputStream.readByte();
	SXSCReader bitReader(inputStream);
	RangeDecoder arithDecoder(bitReader,bitReaderInitialValue);

	// decoder for literal, copy, end decision
	// two thresholds -> 3 symbols, last symbol is break with size of 1
	uint16_t bitThreshold1[4]={40,40,40,40};
	uint16_t bitThreshold2[4]={40,40,40,40};
	uint32_t bitPos=0;

	// generics for the other decoder

	auto tableElements=[](auto &table)->uint16_t
	{
		return (sizeof(table)/sizeof(*table)+1)>>1;
	};

	auto initTable=[](auto &table,uint16_t initialValue)
	{
		constexpr uint32_t length=(sizeof(table)/sizeof(*table)+1)>>1;
		for (uint32_t i=0;i<length;i++) table[i]=initialValue;
		for (uint32_t j=0,i=length;i<length*2-1;i++,j+=2)
			table[i]=table[j]+table[j+1];
	};

	auto updateTable=[](auto &table,uint16_t max,uint16_t index,int16_t value)
	{
		constexpr uint32_t length=(sizeof(table)/sizeof(*table)+1)>>1;
		for (uint32_t i=index;i<length*2-1;i=(i>>1)+length)
			table[i]+=value;
		if (table[length*2-2]>=max)
		{
			for (uint32_t i=0;i<length;i++)
				if (table[i]>1) table[i]>>=1;
			for (uint32_t j=0,i=length;i<length*2-1;i++,j+=2)
				table[i]=table[j]+table[j+1];
		}
	};

	auto tableSize=[](auto &table)->uint16_t
	{
		constexpr uint32_t length=(sizeof(table)/sizeof(*table)+1)>>1;
		return table[length*2-2];
	};

	auto decodeSymbol=[](auto &table,uint16_t &value)->uint16_t
	{
		constexpr uint32_t length=(sizeof(table)/sizeof(*table)+1)>>1;
		uint32_t threshold=0;
		uint32_t i=length*2-4;
		while (i>=length)
		{
			uint32_t child=(i-length)<<1;
			if (value-threshold>=table[i])
			{
				threshold+=table[i];
				child+=2;
			}
			i=child;
		}
		if (value-threshold>=table[i])
		{
			threshold+=table[i];
			i++;
		}
		value=threshold;
		return i;
	};

	// literal decoder
	uint16_t litInitial[256*2-1];
	uint16_t litDynamic[256*2-1];
	uint16_t litThreshold=1;

	initTable(litInitial,1);
	initTable(litDynamic,0);

	// distance / length decoder
	uint16_t distanceCodes[16*2-1];
	uint16_t countInitial[64*2-1];
	uint16_t countDynamic[64*2-1];
	uint16_t countThreshold=8;

	initTable(distanceCodes,0);
	initTable(countInitial,1);
	initTable(countDynamic,0);

	updateTable(distanceCodes,6000,0,24);
	uint32_t distanceIndex=0;

	auto twoStepArithDecoder=[&](auto &initialTable,auto &dynamicTable,uint16_t &threshold,uint16_t max,uint16_t step,uint16_t updateRange)->uint16_t
	{
		uint16_t value=arithDecoder.decode(tableSize(dynamicTable)+threshold);
		uint16_t ret;
		if (value<tableSize(dynamicTable))
		{
			ret=decodeSymbol(dynamicTable,value);
			arithDecoder.scale(value,value+dynamicTable[ret],tableSize(dynamicTable)+threshold);
		} else {
			arithDecoder.scale(tableSize(dynamicTable),tableSize(dynamicTable)+threshold,tableSize(dynamicTable)+threshold);

			value=arithDecoder.decode(tableSize(initialTable));
			ret=decodeSymbol(initialTable,value);
			arithDecoder.scale(value,value+initialTable[ret],tableSize(initialTable));
			updateTable(initialTable,65535,ret,-initialTable[ret]);
			if (tableSize(initialTable)) threshold+=step;
				else threshold=0;
			for (uint32_t i=ret>updateRange?ret-updateRange:0;i<std::min(uint16_t(ret+updateRange),uint16_t(tableElements(initialTable)-1U));i++)
				if (initialTable[i]) updateTable(initialTable,max,i,1);
		}
		updateTable(dynamicTable,max,ret,step);
		if (dynamicTable[ret]==step*3) threshold=threshold>step?threshold-step:1;
		return ret;
	};

	for (;;)
	{
		uint16_t bitSize=bitThreshold1[bitPos]+bitThreshold2[bitPos];
		uint16_t bitValue=arithDecoder.decode(bitSize+1);
		if (bitValue==bitSize) break;
		bool bit=bitValue<bitThreshold1[bitPos];
		arithDecoder.scale(bit?0:bitThreshold1[bitPos],bit?bitThreshold1[bitPos]:bitSize,bitSize+1);
		(bit?bitThreshold1:bitThreshold2)[bitPos]+=40;
		if (bitSize>=6000)
		{
			if (!(bitThreshold1[bitPos]>>=1)) bitThreshold1[bitPos]=1;
			if (!(bitThreshold2[bitPos]>>=1)) bitThreshold2[bitPos]=1;
		}
		bitPos=(bitPos<<1&2)|(bit?0:1);
		if (bit)
		{
			// literal
			outputStream.writeByte(twoStepArithDecoder(litInitial,litDynamic,litThreshold,1000,1,8));
		} else {
			// copy
			while (outputStream.getOffset()>(1ULL<<distanceIndex) && distanceIndex<15U)
				updateTable(distanceCodes,6000,++distanceIndex,24);
			uint16_t distanceValue=arithDecoder.decode(tableSize(distanceCodes));
			uint16_t distanceBits=decodeSymbol(distanceCodes,distanceValue);
			arithDecoder.scale(distanceValue,distanceValue+distanceCodes[distanceBits],tableSize(distanceCodes));
			updateTable(distanceCodes,6000,distanceBits,24);
			uint32_t distance=distanceBits;
			if (distanceBits>=2)
			{
				uint16_t minRange=1<<(distanceBits-1);
				uint16_t range=distanceIndex==distanceBits?static_cast<uint16_t>(std::min(outputStream.getOffset(),size_t(31200U)))-minRange:minRange;
				distance=arithDecoder.decode(range);
				arithDecoder.scale(distance,distance+1,range);
				distance+=minRange;
			}
			distance++;
			uint32_t count=twoStepArithDecoder(countInitial,countDynamic,countThreshold,6000,8,4);
			if (count==15)
			{
				count=783;
			} else if (count>=16) {
				uint16_t value=arithDecoder.decode(16);
				arithDecoder.scale(value,value+1,16);
				count=((count-16)<<4)+value+15;
			}
			count+=3;
			outputStream.copy(distance,count);
		}
	}
	if (!outputStream.eof()) throw Decompressor::DecompressionError();
}

template<typename T,size_t length>
class CheckedArray
{
public:
	CheckedArray() :
		_memory(length*sizeof(T))
	{
		// nothing needed
	}

	~CheckedArray()
	{
		// nothing needed
	}

	T &operator[](size_t i)
	{
		if (i>=length) throw Decompressor::DecompressionError();
		return _memory.cast<T>()[i];
	}

	const T &operator[](size_t i) const
	{
		if (i>=length) throw Decompressor::DecompressionError();
		return _memory.cast<T>()[i];
	}

private:
	MemoryBuffer _memory;
};

// The horror. It needs to follow exactly the original logic, even if that logic is not very good.
void SXSCDecompressor::decompressHSC(Buffer &rawData,ForwardInputStream &inputStream)
{
	struct Model
	{
		uint8_t		context[4];

		uint16_t	hashPointer;
		uint16_t	expiryPrevious;
		uint16_t	expiryNext;
		uint16_t	frequencyTotal;
		uint16_t	escapeFrequency;

		uint8_t		contextLength;
		uint8_t		characterCount;
		uint8_t		refreshCounter;
	};

	struct Frequency
	{
		uint16_t	frequency;
		uint16_t	next;
		uint8_t		character;
	};

	struct HashItem
	{
		uint16_t data;
		uint16_t random;
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());

	uint16_t bitReaderInitialValue;
	bitReaderInitialValue=inputStream.readByte()<<8;
	bitReaderInitialValue|=inputStream.readByte();
	SXSCReader bitReader(inputStream);
	RangeDecoder arithDecoder(bitReader,bitReaderInitialValue);

	uint8_t maxContextLength=4;
	int16_t dropCount=2500;
	int16_t contextSearchLength=0;

	CheckedArray<Model,10000> models{};
	for (uint32_t i=0;i<10000;i++)
	{
		auto &m=models[i];

		for (uint32_t j=0;j<4;j++)
			m.context[j]=0;

		m.hashPointer=0;
		m.expiryPrevious=i-1;
		m.expiryNext=i+1;
		m.frequencyTotal=0;
		m.escapeFrequency=0;

		m.contextLength=0xffU;
		m.characterCount=0;
		m.refreshCounter=0;
	}

	uint8_t currentContext[4];
	for (uint32_t i=0;i<4;i++) currentContext[i]=0;
	uint16_t firstExpiry=0,lastExpiry=9999;
	uint16_t freeBlockPointer=10000;
	uint16_t releaseBlock=0;

	CheckedArray<Frequency,32760> frequencies{};
	for (uint32_t i=0;i<32760;i++)
	{
		auto &f=frequencies[i];
		f.frequency=0;
		f.next=(i>=10000&&i<32759)?i+1:0xffffU;
		f.character=0;
	}

	CheckedArray<HashItem,0x4000> hashes{};
	for (uint32_t i=0,j=10;i<0x4000U;i++)
	{
		auto &h=hashes[i];
		h.data=0xffffU;
		// constants used are 2147483647 % / 16807
		int32_t integerPart=j/127773U;
		int32_t fractionalPart=j%127773U;
		int32_t tmp=16807*fractionalPart-2836*integerPart;
		j=tmp<0?tmp+0x7fff'ffff:tmp;
		h.random=j&0x3fffU;
	}
	uint16_t hashStack[5];
	for (uint32_t i=0;i<5;i++)
		hashStack[i]=0;

	bool characterMask[256];
	uint8_t characterMaskStack[256];
	for (uint32_t i=0;i<256;i++)
	{
		characterMask[i]=false;
		characterMaskStack[i]=0;
	}
	int16_t characterMaskStackPointer=0;

	uint8_t initialEscapeChar[5];
	for (uint32_t i=0;i<5;i++)
		initialEscapeChar[i]=i?15:16;
	uint8_t escapeCharacterCounter=0;

	uint16_t stackPointer=0;
	uint16_t contextPointer[5];
	uint16_t frequencyArrayIndex[5];
	for (uint32_t i=0;i<5;i++)
	{
		contextPointer[i]=0;
		frequencyArrayIndex[i]=0;
	}

	auto loopBreaker=[](uint16_t i) {
		if (i>=0x8000U) throw Decompressor::DecompressionError();
	};

	auto findNext=[&]()->uint16_t
	{
		for (int32_t i=contextSearchLength-1;i>=0;i--)
		{
			for (uint32_t lb=0,j=hashes[hashStack[i]].data;j!=0xffffU;j=models[j].hashPointer,loopBreaker(lb++))
			{
				if (i==models[j].contextLength)
				{
					if ([&]()->bool
					{
						for (int32_t k=0;k<i;k++)
							if (currentContext[k]!=models[j].context[k]) return false;
						return true;
					}()) {
						contextSearchLength=i;
						return j;
					}
				}
			}
		}
		return 0xffffU;
	};

	for (;;)
	{
		for (uint32_t i=0;i<4;i++)
			hashStack[i+1]=hashes[(currentContext[i]+hashStack[i])&0x3fffU].random;
		stackPointer=0;
		while (characterMaskStackPointer)
			characterMask[characterMaskStack[--characterMaskStackPointer]]=false;
		contextSearchLength=5;
		uint16_t index=findNext();

		uint8_t minLength=index!=0xffffU?models[index].contextLength+1:0;
		uint16_t ch;
		for (;;index=findNext())
		{
			if (index==0xffffU)
			{
				uint16_t size=257-characterMaskStackPointer;
				uint16_t value=arithDecoder.decode(size);
				// logic from original, could be improved a lot
				// however, lets not optimize unless it is a problem...
				uint16_t i=0;
				for (ch=0;ch<256;ch++)
				{
					if (characterMask[ch]) continue;
					if (i>=value) break;
					i++;
				}
				arithDecoder.scale(i,i+1,size);
				break;
			}

			// madness!!!
			auto getEscFrequency=[&](uint16_t value,uint16_t i)->uint16_t
			{
				auto &model=models[i];
				if (model.frequencyTotal==1)
					return initialEscapeChar[model.contextLength]>=16?2:1;
				if (model.characterCount==0xffU) return 1;
				uint16_t tmp=uint16_t(model.characterCount)*2+2;
				if (model.characterCount && tmp>=model.frequencyTotal)
				{
					value=int32_t(value)*tmp/model.frequencyTotal;
					if (model.characterCount+1==model.frequencyTotal) value+=tmp>>2;
				}
				if (!value) value++;
				return value;
			};

			int16_t freq=0,escapeFreq=0;
			int16_t currentFrequency=0,frequencyTotal=0;
			auto decodeCf=[&](uint16_t shift,bool cmCondition)->uint16_t
			{
				freq<<=shift;
				uint16_t i,value=arithDecoder.decode(freq+escapeFreq)>>shift;
				uint32_t lb=0;
				for (i=index;i!=0xffffU;i=frequencies[i].next,loopBreaker(lb++))
				{
					auto &frequency=frequencies[i];
					if (cmCondition||!characterMask[frequency.character])
					{
						if (frequencyTotal+frequency.frequency<=value)
						{
							frequencyTotal+=frequency.frequency;
						} else {
							currentFrequency=frequency.frequency<<shift;
							break;
						}
					}
				}
				frequencyTotal<<=shift;
				return i;
			};

			auto decodeCh=[&](uint16_t chPos,uint16_t insertPos,bool cmCondition)->bool
			{
				if (chPos==0xffffU)
				{
					arithDecoder.scale(freq,freq+escapeFreq,freq+escapeFreq);
					if (models[index].frequencyTotal==1 && initialEscapeChar[models[index].contextLength]<32)
						initialEscapeChar[models[index].contextLength]++;
					uint16_t prevI=0;
					for (uint16_t lb=0,i=index;i!=0xffffU;prevI=i,i=frequencies[i].next,loopBreaker(lb++))
					{
						auto &frequency=frequencies[i];
						if (cmCondition||!characterMask[frequency.character])
						{
							if (characterMaskStackPointer==256) throw Decompressor::DecompressionError();
							characterMaskStack[characterMaskStackPointer++]=frequency.character;
							characterMask[frequency.character]=true;
						}
					}
					contextPointer[insertPos]=index|0x8000U;
					frequencyArrayIndex[insertPos]=prevI;
					ch=256;
					return true;
				} else {
					arithDecoder.scale(frequencyTotal,frequencyTotal+currentFrequency,freq+escapeFreq);
					if (models[index].frequencyTotal==1 && initialEscapeChar[models[index].contextLength])
						initialEscapeChar[models[index].contextLength]--;
					contextPointer[insertPos]=index;
					frequencyArrayIndex[insertPos]=chPos;
					ch=frequencies[chPos].character;
					return false;
				}
			};

			if (characterMaskStackPointer)
			{
				for (uint16_t lb=0,i=index;i!=0xffffU;i=frequencies[i].next,loopBreaker(lb++))
				{
					auto &frequency=frequencies[i];
					if (!characterMask[frequency.character])
					{
						freq+=frequency.frequency;
						if (frequency.frequency<3) escapeFreq++;
					}
				}
				escapeFreq=getEscFrequency(escapeFreq,index);

				uint16_t chPos=decodeCf(0,false);
				if (stackPointer==5) throw Decompressor::DecompressionError();
				if (!decodeCh(chPos,stackPointer,false))
				{
					if (escapeCharacterCounter==10) throw Decompressor::DecompressionError();
					escapeCharacterCounter++;
				}
				stackPointer++;

			} else {
				freq=models[index].frequencyTotal;
				escapeFreq=getEscFrequency(models[index].escapeFrequency,index);

				uint16_t chPos=decodeCf((escapeCharacterCounter>=5)?(freq<5 && escapeCharacterCounter==10)?2:1:0,true);

				stackPointer=1;
				if (decodeCh(chPos,0,true))
				{
					escapeCharacterCounter=0;
				} else {
					if (escapeCharacterCounter<10) escapeCharacterCounter++;
				}
			}

			if (ch!=256)
			{
				if (index!=firstExpiry)
				{
					auto &model=models[index];
					if (index==lastExpiry)
					{
						lastExpiry=model.expiryPrevious;
					} else {
						models[model.expiryNext].expiryPrevious=model.expiryPrevious;
						models[model.expiryPrevious].expiryNext=model.expiryNext;
					}
					models[firstExpiry].expiryPrevious=index;
					model.expiryNext=firstExpiry;
					firstExpiry=index;
				}
				break;
			}
		}

		if (ch==256) break;

		while (stackPointer)
		{
			uint16_t freqIndex=frequencyArrayIndex[--stackPointer];
			uint16_t pointer=contextPointer[stackPointer];
			auto &model=models[pointer&0x7fffU];
			if (pointer&0x8000U)
			{
				if (freeBlockPointer==0xffffU)
				{
					for (uint16_t i=0;i<=stackPointer;)
					{
						// yuck
						uint32_t lb=0;
						do
						{
							releaseBlock=(releaseBlock!=9999U)?releaseBlock+1:0;
							loopBreaker(lb++);
						} while (frequencies[releaseBlock].next==0xffffU);
						for (i=0;i<=stackPointer;i++)
							if ((contextPointer[i]&0x7fffU)==releaseBlock) break;
					}
					auto &frequencyRB=frequencies[releaseBlock];
					auto &modelRB=models[releaseBlock];
					uint16_t f=frequencyRB.frequency;
					for (uint16_t lb=0,i=frequencyRB.next;i!=0xffffU;i=frequencies[i].next,loopBreaker(lb++))
						if (frequencies[i].frequency<f) f=frequencies[i].frequency;
					bool stopProcess=false;
					if (frequencyRB.frequency<++f)
					{
						uint16_t i,lb=0;
						for (lb=0,i=frequencyRB.next;frequencies[i].frequency<f&&frequencies[i].next!=0xffffU;i=frequencies[i].next,loopBreaker(lb++));
						auto &frequencyI=frequencies[i];
						frequencyRB.frequency=frequencyI.frequency;
						frequencyRB.character=frequencyI.character;
						uint16_t j=frequencyI.next;
						frequencyI.next=freeBlockPointer;
						freeBlockPointer=frequencyRB.next;
						frequencyRB.next=j;
						if (j==0xffffU)
						{
							modelRB.characterCount=0;
							uint16_t tmp=modelRB.frequencyTotal=frequencyRB.frequency;
							modelRB.escapeFrequency=tmp<3?1:0;
							stopProcess=true;
						}
					}
					if (!stopProcess)
					{
						{
							modelRB.characterCount=0;
							uint16_t tmp=frequencyRB.frequency/=f;
							modelRB.frequencyTotal=tmp;
							modelRB.escapeFrequency=tmp<3?1:0;
						}
						for (uint16_t lb=0,i=frequencyRB.next,j=releaseBlock;i!=0xffffU;i=frequencies[j].next,loopBreaker(lb++))
						{
							if (frequencies[i].frequency<f)
							{
								frequencies[j].next=frequencies[i].next;
								frequencies[i].next=freeBlockPointer;
								freeBlockPointer=i;
							} else {
								modelRB.characterCount++;
								uint16_t tmp=frequencies[i].frequency/=f;
								if (tmp<3) modelRB.escapeFrequency++;
								modelRB.frequencyTotal+=tmp;
								j=i;
							}
						}
					}
				}
				freqIndex=frequencies[freqIndex].next=freeBlockPointer;
				freeBlockPointer=frequencies[freeBlockPointer].next;
				auto &frequencyFI=frequencies[freqIndex];
				frequencyFI.frequency=1U;
				frequencyFI.next=0xffffU;
				frequencyFI.character=ch;
				model.escapeFrequency++;
				model.characterCount++;
			} else if (++frequencies[freqIndex].frequency==3) {
				model.escapeFrequency--;
			}
			if ((++model.frequencyTotal)/(model.characterCount+1)>frequencies[freqIndex].frequency*2)
			{
				model.refreshCounter--;
			} else if (model.refreshCounter<4) {
				model.refreshCounter++;
			}
			// one ugly scaler
			if (!model.refreshCounter||model.frequencyTotal>=8000)
			{
				model.refreshCounter++;
				model.escapeFrequency=0;
				model.frequencyTotal=0;
				for (uint16_t lb=0,i=pointer&0x7fffU;i!=0xffffU;i=frequencies[i].next,loopBreaker(lb++))
				{
					if (frequencies[i].frequency>1)
					{
						uint16_t tmp=frequencies[i].frequency>>=1;
						model.frequencyTotal+=tmp;
						if (tmp<3) model.escapeFrequency++;
					} else {
						model.escapeFrequency++;
						model.frequencyTotal++;
					}
				}
			}
		}

		uint8_t maxLength=maxContextLength+1;
		while (maxLength-->minLength)
		{
			auto hash=[&](const uint8_t *block,uint32_t length)->uint16_t
			{
				uint16_t ret=0;
				for (uint32_t i=0;i<length;i++)
					ret=hashes[(block[i]+ret)&0x3fffU].random;
				return ret;
			};
			uint16_t le=lastExpiry;
			auto &model=models[le];
			auto &frequency=frequencies[le];
			lastExpiry=model.expiryPrevious;

			models[firstExpiry].expiryPrevious=le;
			model.expiryNext=firstExpiry;
			firstExpiry=le;
			if (model.contextLength!=0xffU)
			{
				if (model.contextLength==4 && !--dropCount)
					maxContextLength=3;
				uint16_t h=hash(model.context,model.contextLength);
				if (hashes[h].data==le)
				{
					hashes[h].data=model.hashPointer;
				} else {
					uint16_t i=hashes[h].data;
					uint32_t lb=0;
					while (le!=models[i].hashPointer)
					{
						i=models[i].hashPointer;
						loopBreaker(lb++);
					}
					models[i].hashPointer=model.hashPointer;
				}
				if (frequency.next!=0xffffU)
				{
					uint16_t i=frequency.next;
					uint32_t lb=0;
					while (frequencies[i].next!=0xffffU)
					{
						i=frequencies[i].next;
						loopBreaker(lb++);
					}
					frequencies[i].next=freeBlockPointer;
					freeBlockPointer=frequency.next;
				}
			}
			frequency.next=0xffffU;
			frequency.frequency=1;
			frequency.character=ch;
			model.escapeFrequency=1;
			model.frequencyTotal=1;
			model.contextLength=maxLength;
			model.characterCount=0;
			model.refreshCounter=4;
			for (uint32_t i=0;i<4;i++) model.context[i]=currentContext[i];
			uint16_t hashValue=hash(currentContext,maxLength);
			model.hashPointer=hashes[hashValue].data;
			hashes[hashValue].data=le;
		}

		outputStream.writeByte(ch);
		for (uint16_t i=3;i!=0;i--)
		{
			currentContext[i]=currentContext[i-1];
		}
		currentContext[0]=ch;
	}
	if (!outputStream.eof()) throw Decompressor::DecompressionError();
}

void SXSCDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	ForwardInputStream inputStream(_packedData,0,_packedData.size(),true);

	uint8_t mode=inputStream.readByte();

	bool needsTmpBuffer=(mode>=2);
	std::unique_ptr<MemoryBuffer> tmpBuffer;
	if (needsTmpBuffer) tmpBuffer=std::make_unique<MemoryBuffer>(rawData.size());
	if (_isHSC) decompressHSC(needsTmpBuffer?*tmpBuffer:rawData,inputStream);
		else decompressASC(needsTmpBuffer?*tmpBuffer:rawData,inputStream);

	// Mono, High byte only
	// also includes de-interleaving
	auto deltaDecode16BE=[&]()
	{
		size_t length=rawData.size();
		const uint8_t *src=tmpBuffer->data();
		const uint8_t *midSrc=&src[length>>1];
		uint8_t *dest=rawData.data();

		uint8_t ctr=0;
		for (size_t i=0,j=0;j<length;i++,j+=2)
		{
			ctr+=src[i];
			dest[j]=ctr;
			dest[j+1]=midSrc[i];
		}
		if (length&1) dest[length-1]=src[length-1];
	};

	// Stereo, High byte only
	// also includes de-interleaving
	auto deltaDecode16LE=[&]()
	{
		size_t length=rawData.size();
		const uint8_t *src=tmpBuffer->data();
		const uint8_t *midSrc=&src[length>>1];
		uint8_t *dest=rawData.data();

		uint8_t ctr=0;
		for (size_t i=0,j=0;j<length;i++,j+=2)
		{
			dest[j]=midSrc[i];
			ctr+=src[i];
			dest[j+1]=ctr;
		}
		if (length&1) dest[length-1]=src[length-1];
	};

	switch (mode)
	{
		case 0:
		// no delta
		break;

		case 1:
		DLTADecode::decode(rawData,rawData,0,rawData.size());
		break;
		
		case 2:
		deltaDecode16BE();
		break;

		case 3:
		deltaDecode16LE();
		break;

		default:
		throw Decompressor::DecompressionError();
	}
}

}
