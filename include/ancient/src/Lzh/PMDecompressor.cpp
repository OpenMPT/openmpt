/* Copyright (C) Teemu Suutari */

#include "PMDecompressor.hpp"

#include "../HuffmanDecoder.hpp"
#include "../InputStream.hpp"
#include "../OutputStream.hpp"


namespace ancient::internal
{

PMDecompressor::PMDecompressor(const Buffer &packedData,uint32_t version) :
	_packedData(packedData),
	_version(version)
{
	if (version!=1 && version!=2) throw InvalidFormatError();
}

PMDecompressor::~PMDecompressor()
{
	// nothing needed
}

size_t PMDecompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t PMDecompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &PMDecompressor::getName() const noexcept
{
	static std::string name="LHA: PM1, PM2";
	return name;
}

uint8_t PMDecompressor::decodeMTF(uint8_t value,uint8_t map[])
{
	return map[value];
}

void PMDecompressor::updateMTF(uint8_t value,uint8_t map[])
{
	for (uint32_t i=0;;i++)
	{
		if (map[i]==value)
		{
			value=i;
			break;
		}
	}
	if (value)
	{
		uint8_t tmp=map[value];
		for (uint32_t i=value;i;i--)
			map[i]=map[i-1];
		map[0]=tmp;
	}
}

void PMDecompressor::createMTFMap(uint8_t map[])
{
	for (uint32_t i=0,j=0x20;j<0x80;i++,j++) map[i]=j;
	for (uint32_t i=0x60,j=0;j<0x20;i++,j++) map[i]=j;
	for (uint32_t i=0x80,j=0xa0;j<0xe0;i++,j++) map[i]=j;
	for (uint32_t i=0xc0,j=0x80;j<0xa0;i++,j++) map[i]=j;
	for (uint32_t i=0xe0,j=0xe0;j<0x100;i++,j++) map[i]=j;
}

void PMDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	if (_version==1) decompressImplPM1(rawData,verify);
		else decompressImplPM2(rawData,verify);
}

void PMDecompressor::decompressImplPM1(Buffer &rawData,bool verify)
{
	 static const struct {
		uint8_t		length;
		uint8_t 	code;
	 } treeDefinitions[32][6] {
		// This is madness, I had to write a special program to decode this.
		// Also, in the original there is a bug @ index 17, as identified by
		// lhasa (we fix it when creating the decoder)
		{{4, 0b0000},{4, 0b0001},{3,  0b001},{2,   0b01},{2,   0b10},{2,   0b11}},
		{{3,  0b000},{3,  0b001},{3,  0b010},{2,   0b10},{2,   0b11},{3,  0b011}},
		{{3,  0b000},{3,  0b001},{2,   0b01},{2,   0b10},{3,  0b110},{3,  0b111}},
		{{2,   0b00},{3,  0b010},{3,  0b011},{2,   0b10},{3,  0b110},{3,  0b111}},
		{{2,   0b00},{3,  0b010},{2,   0b10},{3,  0b011},{3,  0b110},{3,  0b111}},
		{{2,   0b00},{3,  0b010},{2,   0b10},{2,   0b11},{4, 0b0110},{4, 0b0111}},
		{{2,   0b00},{2,   0b01},{3,  0b100},{3,  0b101},{3,  0b110},{3,  0b111}},
		{{2,   0b00},{2,   0b01},{3,  0b100},{2,   0b11},{4, 0b1010},{4, 0b1011}},

		{{2,   0b00},{2,   0b01},{2,   0b10},{3,  0b110},{4, 0b1110},{4, 0b1111}},
		{{1,    0b0},{4, 0b1000},{3,  0b101},{3,  0b110},{3,  0b111},{4, 0b1001}},
		{{1,    0b0},{4, 0b1000},{3,  0b101},{2,   0b11},{5,0b10010},{5,0b10011}},
		{{1,    0b0},{4, 0b1000},{4, 0b1001},{3,  0b101},{3,  0b110},{3,  0b111}},
		{{1,    0b0},{3,  0b100},{4, 0b1010},{3,  0b110},{3,  0b111},{4, 0b1011}},
		{{1,    0b0},{3,  0b100},{3,  0b101},{3,  0b110},{4, 0b1110},{4, 0b1111}},
		{{1,    0b0},{3,  0b100},{2,   0b11},{4, 0b1010},{5,0b10110},{5,0b10111}},
		{{1,    0b0},{2,   0b10},{4, 0b1100},{4, 0b1101},{4, 0b1110},{4, 0b1111}},

		{{1,    0b0},{2,   0b10},{3,  0b110},{4, 0b1110},{5,0b11110},{5,0b11111}},
		{{3,  0b000},{3,  0b001},{2,   0b01},{2,   0b10},{2,   0b11},{0,    0b0}},
		{{2,   0b00},{3,  0b010},{2,   0b10},{2,   0b11},{3,  0b011},{0,    0b0}},
		{{2,   0b00},{2,   0b01},{2,   0b10},{3,  0b110},{3,  0b111},{0,    0b0}},
		{{1,    0b0},{4, 0b1000},{3,  0b101},{2,   0b11},{4, 0b1001},{0,    0b0}},
		{{1,    0b0},{3,  0b100},{3,  0b101},{3,  0b110},{3,  0b111},{0,    0b0}},
		{{1,    0b0},{3,  0b100},{2,   0b11},{4, 0b1010},{4, 0b1011},{0,    0b0}},
		{{1,    0b0},{2,   0b10},{3,  0b110},{4, 0b1110},{4, 0b1111},{0,    0b0}},

		{{3,  0b000},{3,  0b001},{2,   0b01},{1,    0b1},{0,    0b0},{0,    0b0}},
		{{2,   0b00},{3,  0b010},{1,    0b1},{3,  0b011},{0,    0b0},{0,    0b0}},
		{{2,   0b00},{2,   0b01},{2,   0b10},{2,   0b11},{0,    0b0},{0,    0b0}},
		{{1,    0b0},{3,  0b100},{2,   0b11},{3,  0b101},{0,    0b0},{0,    0b0}},
		{{1,    0b0},{2,   0b10},{3,  0b110},{3,  0b111},{0,    0b0},{0,    0b0}},
		{{1,    0b0},{2,   0b10},{2,   0b11},{0,    0b0},{0,    0b0},{0,    0b0}},
		{{1,    0b0},{1,    0b1},{0,    0b0},{0,    0b0},{0,    0b0},{0,    0b0}},
		{{0,    0b0},{1,    0b1},{0,    0b0},{0,    0b0},{0,    0b0},{0,    0b0}}
	 };
 
	ForwardInputStream inputStream(_packedData,0,_packedData.size(),true);

	MSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBits8(1);
	};

	size_t rawSize=rawData.size();
	ForwardOutputStream outputStream(rawData,0,rawSize);

	OptionalHuffmanDecoder<uint32_t> decoder;
	{
		uint32_t treeIndex=readBits(5);
		for (uint32_t i=0;i<6;i++)
		{
			uint32_t length=treeDefinitions[treeIndex][i].length;
			uint32_t code=treeDefinitions[treeIndex][i].code;
			if (!length)
			{
				if (!i) decoder.setEmpty(0);
				break;
			}
			if (treeIndex==17 && i<2) decoder.insert(HuffmanCode<uint32_t>{length,code,i+3});
				else decoder.insert(HuffmanCode<uint32_t>{length,code,i});
		}
	}

	uint8_t dataMTFMap[256];
	createMTFMap(dataMTFMap);

	auto processOutput=[&](uint8_t value)->uint8_t
	{
		updateMTF(value,dataMTFMap);
		return value;
	};

	while (!outputStream.eof())
	{
		bool doCopy=true;
		if (readBit())
		{
			uint32_t count=readBits(2)+1;
			if (count==4)
			{
				count=readBits(3)+4;
				if (count==11)
				{
					count=readBits(4)+11;
					if (count==25)
					{
						count=readBits(6)+25;
					} else if (count==26) {
						count=readBits(7)+89;
					}
				}
			}

			count=std::min(count,uint32_t(rawSize-outputStream.getOffset()));
			for (uint32_t i=0;i<count;i++)
			{
				uint32_t code=decoder.decode(readBit);
				static const uint32_t symbolAdditions[6]={0,16,32,64,128,192};
				static const uint32_t symbolBits[6]={4,4,5,6,6,6};
				outputStream.writeByte(processOutput(decodeMTF(readBits(symbolBits[code])+symbolAdditions[code],dataMTFMap)));
			}
			doCopy=count!=216&&!outputStream.eof();
		}
		if (doCopy)
		{
			uint32_t offset=uint32_t(outputStream.getOffset());
			uint32_t count,code;
			if (!readBit())
			{
				if (offset>=0x240?readBit():0)
				{
					code=4;
					if (offset<0x340) code=7;
					else if (offset<0x440) code=8;
					else if (offset<0x640) code=9;
				} else {
					code=offset>=0x40?readBit():0;
					count=2;
				}
			} else {
				if (offset>=0x40?!readBit():0)
				{
					code=3;
					if (offset<0x140) code=6;
				} else if (offset>=0xa40?readBit():1) {
					code=2;
				} else {
					code=5;
					if (offset<0xb40) code=10;
					else if (offset<0xc40) code=11;
					else if (offset<0xe40) code=12;
					else if (offset<0x1240) code=13;
					else if (offset<0x1a40) code=14;
				}
			}
			if (code>=2)
			{
				count=readBits(2)+3;
				if (count==6)
				{
					count=readBits(3)+6;
					if (count==11)
					{
						count=readBits(2)+11;
					} else if (count==12) {
						count=readBits(3)+15;
					} else if (count==13) {
						count=readBits(6)+23;
						if (count==85) {
							count=readBits(5)+85;
						} else if (count==86)
							count=readBits(7)+117;
					}
				}
			}
			static const uint32_t distanceAdditions[15]={
				1,0x41,1,0x41,0x241,0xa41,0x41,0x241,0x241,0x241,0xa41,0xa41,0xa41,0xa41,0xa41};
			static const uint32_t distanceBits[15]={
				6,8,6,9,11,13,8,8,9,10,8,9,10,11,12};
			uint32_t distance=readBits(distanceBits[code])+distanceAdditions[code];

			count=std::min(count,uint32_t(rawSize-outputStream.getOffset()));
			outputStream.copy(distance,count,0x20);
			const uint8_t *block=outputStream.history(count);
			for (uint32_t i=0;i<count;i++)
				processOutput(block[i]);
		}
	}
}

void PMDecompressor::decompressImplPM2(Buffer &rawData,bool verify)
{
 	ForwardInputStream inputStream(_packedData,0,_packedData.size());

	MSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBits8(1);
	};

	ForwardOutputStream outputStream(rawData,0,rawData.size());

	OptionalHuffmanDecoder<uint32_t> decoder;
	OptionalHuffmanDecoder<uint32_t> distanceDecoder;

	auto createDecoder=[&]()->bool
	{
		decoder.reset();

		// codes beyond 29 are going to fault anyway, but maybe they are not used?
		uint8_t symbols[31];
		uint32_t numCodes=readBits(5);
		uint32_t minLength=readBits(3);

		bool ret=(numCodes>=10)&&(numCodes!=29||minLength);

		if (!minLength)
		{
			if (!numCodes) throw DecompressionError();
			decoder.setEmpty(numCodes-1);
		} else {
			uint32_t codeLength=readBits(3);
			for (uint32_t i=0;i<numCodes;i++)
			{
				uint32_t value=readBits(codeLength);
				symbols[i]=value?minLength+value-1:0;
			}
			decoder.createOrderlyHuffmanTable(symbols,numCodes);
		}
		return ret;
	};

	auto createDistanceDecoder=[&](uint32_t numCodes)
	{
		distanceDecoder.reset();

		uint8_t distances[8];

		uint32_t last=0,total=0;
		for (uint32_t i=0;i<numCodes;i++)
		{
			uint32_t length=readBits(3);
			distances[i]=length;
			if (length)
			{
				total++;
				last=i;
			}
		}
		if (!total)
		{
			throw DecompressionError();
		} else if (total==1) {
			distanceDecoder.setEmpty(last);
		} else {
			distanceDecoder.createOrderlyHuffmanTable(distances,numCodes);
		}
	};

	uint8_t dataMTFMap[256];
	createMTFMap(dataMTFMap);

	bool distanceTreeRequired;
	auto processOutput=[&](size_t offset,uint8_t value)->uint8_t
	{
		offset++;	// we are interested offset after the update
		updateMTF(value,dataMTFMap);
		if (!(offset&0x3ffU))
		{
			switch (offset>>10)
			{
				case 1:
				if (distanceTreeRequired) createDistanceDecoder(6);
				break;

				case 2:
				if (distanceTreeRequired) createDistanceDecoder(7);
				break;

				case 4:
				if (readBit()) distanceTreeRequired=createDecoder();
				if (distanceTreeRequired) createDistanceDecoder(8);
				break;

				default:
				if (!(offset&0xfffU) && (offset>>12)>=2)
				{
					if (readBit())
					{
						distanceTreeRequired=createDecoder();
						if (distanceTreeRequired) createDistanceDecoder(8);
					}
				}
				break;
			}
		}
		return value;
	};

	readBit();	// ignore first bit
	distanceTreeRequired=createDecoder();
	if (distanceTreeRequired) createDistanceDecoder(5);
	while (!outputStream.eof())
	{
		uint32_t code=decoder.decode(readBit);
		if (code<8)
		{
			static const uint32_t symbolAdditions[8]={0,8,16,32,64,96,128,192};
			static const uint32_t symbolBits[8]={3,3,4,5,5,5,6,6};
			outputStream.writeByte(processOutput(outputStream.getOffset(),decodeMTF(readBits(symbolBits[code])+symbolAdditions[code],dataMTFMap)));
		} else {
			code-=8;

			uint32_t count;
			if (code<15)
			{
				count=code+2;
			} else {
				if (code>=21) throw DecompressionError();
				static const uint32_t countAdditions[6]={17,25,33,65,129,256};
				static const uint32_t countBits[6]={3,3,5,6,7,0};
				count=readBits(countBits[code-15])+countAdditions[code-15];
			}

			uint32_t distance;
			if (!code)
			{
				distance=readBits(6)+1;
			} else if (code<20) {
				uint32_t tmp=distanceDecoder.decode(readBit);
				if (!tmp) distance=readBits(6)+1;
					else distance=readBits(tmp+5)+(1<<(tmp+5))+1;
			} else distance=1;

			outputStream.copy(distance,count,0x20);
			const uint8_t *block=outputStream.history(count);
			size_t offset=outputStream.getOffset()-count;
			for (uint32_t i=0;i<count;i++)
				processOutput(offset+i,block[i]);
		}
	}
}

}
