/* Copyright (C) Teemu Suutari */

#include <cstdint>
#include <cstring>

#include "DMSDecompressor.hpp"

#include "HuffmanDecoder.hpp"
#include "DynamicHuffmanDecoder.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"

#include "common/MemoryBuffer.hpp"
#include "common/CRC16.hpp"
#include "common/OverflowCheck.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool DMSDecompressor::detectHeader(uint32_t hdr) noexcept
{
	return hdr==FourCC("DMS!");
}

std::unique_ptr<Decompressor> DMSDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_unique<DMSDecompressor>(packedData,verify);
}

DMSDecompressor::DMSDecompressor(const Buffer &packedData,bool verify) :
	_packedData(packedData)
{
	uint32_t hdr=packedData.readBE32(0);
	if (!detectHeader(hdr) || packedData.size()<56) throw InvalidFormatError();

	if (verify && CRC16(packedData,4,50,0)!=packedData.readBE16(54))
		throw VerificationError();

	uint16_t info=packedData.readBE16(10);
	_isObsfuscated=info&2;				// using 16 bit key is not encryption, it is obsfuscation
	_isHD=info&16;
	if (info&32) throw InvalidFormatError();	// MS-DOS disk

	// packed data in header is useless, as is rawsize and track numbers
	// they are not always filled

	if (packedData.readBE16(50)>6) throw InvalidFormatError(); // either FMS or unknown

	// now calculate the real packed size, including headers
	uint32_t offset=56;
	uint32_t accountedSize=0;
	uint32_t lastTrackSize=0;
	uint32_t numTracks=0;
	uint32_t minTrack=80;
	uint32_t prevTrack=0;
	while (offset+20<packedData.size())
	{
		if (_packedData.readBE16(offset)!=MultiChar2("TR"))
		{
			// secondary exit criteria, should not be like this, if the header would be trustworthy
			if (!accountedSize) throw InvalidFormatError();
			break;
		}
		uint32_t trackNo=_packedData.readBE16(offset+2);
		// lets not go backwards on tracks!
		if (trackNo<prevTrack) break;

		// header check
		if (verify && CRC16(packedData,offset,18,0)!=packedData.readBE16(offset+18))
			throw VerificationError();

		uint8_t mode=_packedData.read8(offset+13);
		if (mode>6) throw InvalidFormatError();
		static const uint32_t contextSizes[7]={0,0,256,16384,16384,4096,8192};
		_contextBufferSize=std::max(_contextBufferSize,contextSizes[mode]);

		uint8_t flags=_packedData.read8(offset+12);
		if ((mode>=2 && mode<=4) || (mode>=5 && (flags&4)))
		{
			_tmpBufferSize=std::max(_tmpBufferSize,uint32_t(_packedData.readBE16(offset+8)));
		}
		uint32_t packedChunkLength=packedData.readBE16(offset+6);
		if (OverflowCheck::sum(offset,20U,packedChunkLength)>packedData.size())
			throw InvalidFormatError();
		if (verify && CRC16(packedData,offset+20,packedChunkLength,0)!=packedData.readBE16(offset+16))
			throw VerificationError();

		if (trackNo<80)
		{
			if (trackNo>=numTracks) lastTrackSize=_packedData.readBE16(offset+10); 
			minTrack=std::min(minTrack,trackNo);
			numTracks=std::max(numTracks,trackNo);
			prevTrack=trackNo;
		}

		offset+=packedChunkLength+20;
		accountedSize+=packedChunkLength;
		// this is the real exit critea, unfortunately
		if (trackNo>=79 && trackNo<0x8000U) break;
	}
	uint32_t trackSize=(_isHD)?22528:11264;
	_rawOffset=minTrack*trackSize;
	if (minTrack>=numTracks)
		throw InvalidFormatError();
	_minTrack=minTrack;
	_rawSize=(numTracks-minTrack)*trackSize+lastTrackSize;
	_imageSize=trackSize*80;

	_packedSize=offset;
	if (_packedSize>getMaxPackedSize())
		throw InvalidFormatError();
}

DMSDecompressor::~DMSDecompressor()
{
	// nothing needed
}

const std::string &DMSDecompressor::getName() const noexcept
{
	static std::string name="DMS: Disk Masher System";
	return name;
}

size_t DMSDecompressor::getPackedSize() const noexcept
{
	return _packedSize;
}

size_t DMSDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

size_t DMSDecompressor::getImageSize() const noexcept
{
	return _imageSize;
}

size_t DMSDecompressor::getImageOffset() const noexcept
{
	return _rawOffset;
}

void DMSDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	uint32_t restartPosition=0;
	if (!_isObsfuscated)
	{
		decompressImpl(rawData,verify,restartPosition);
	} else {
		while (restartPosition<0x20000U)
		{
			// more than single run here is really rare. It means that first track CRC succeeds
			// but later something else fails
			try
			{
				decompressImpl(rawData,verify,restartPosition);
				return;
			} catch (const Buffer::Error &) {
				// just continue
			} catch (const Decompressor::Error &) {
				// just continue
			}
			restartPosition++;
		}
		throw DecompressionError();
	}
}

// TODO: Too much state for a single method. too convoluted
// needs to be split
void DMSDecompressor::decompressImpl(Buffer &rawData,bool verify,uint32_t &restartPosition)
{
	if (rawData.size()<_rawSize) throw DecompressionError();
	MemoryBuffer contextBuffer(_contextBufferSize);
	MemoryBuffer tmpBuffer(_tmpBufferSize);
	uint32_t limitedDecompress=~0U;

	class UnObsfuscator
	{
	public:
		UnObsfuscator(ForwardInputStream &inputStream) :
			_inputStream(inputStream)
		{
			// nothing needed
		}

		uint8_t readByte()
		{
			if (_inputStream.eof()) throw ShortInputError();
			uint8_t ch=_inputStream.readByte();
			if (!_obsfuscate)
			{
				return ch;
			} else {
				uint8_t ret=ch^_passAccumulator;
				_passAccumulator=(_passAccumulator>>1)+uint16_t(ch);
				return ret;
			}
		}

		void setCode(uint16_t passAccumulator)
		{
			_passAccumulator=passAccumulator;
		}

		void setObsfuscate(bool obsfuscate) { _obsfuscate=obsfuscate; }
		bool eof() const { return _inputStream.getOffset()==_inputStream.getEndOffset(); }

		// not cool, but works (does not need wraparound check)
		bool eofMinus1() const { return _inputStream.getOffset()+1==_inputStream.getEndOffset(); }
		bool eofMinus1Plus() const { return _inputStream.getOffset()+1>=_inputStream.getEndOffset(); }
		bool eofMinus2Plus() const { return _inputStream.getOffset()+2>=_inputStream.getEndOffset(); }

	private:
		ForwardInputStream	&_inputStream;
		bool			_obsfuscate=false;
		uint16_t		_passAccumulator=0;
	};

	ForwardInputStream inputStream(_packedData,0,0);
	UnObsfuscator inputUnObsfuscator(inputStream);
	MSBBitReader<UnObsfuscator> bitReader(inputUnObsfuscator);
	auto initInputStream=[&](const Buffer &buffer,uint32_t start,uint32_t length,bool obsfuscate)
	{
		inputStream=ForwardInputStream(buffer,start,OverflowCheck::sum(start,length));
		inputUnObsfuscator.setObsfuscate(obsfuscate);
		bitReader.reset();
	};
	auto finishStream=[&]()
	{
		if (_isObsfuscated && limitedDecompress==~0U)
			while (!inputUnObsfuscator.eof())
				inputUnObsfuscator.readByte();
	};
	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBits8(count);
	};
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBits8(1);
	};

	ForwardOutputStream outputStream(rawData,0,0);
	auto initOutputStream=[&](Buffer &buffer,uint32_t start,uint32_t length)
	{
		outputStream=ForwardOutputStream(buffer,start,OverflowCheck::sum(start,length));
	};

	// fill unused tracks with zeros
	std::memset(rawData.data(),0,_rawSize);

	auto checksum=[](const uint8_t *src,uint32_t srcLength)->uint16_t
	{
		uint16_t ret=0;
		for (uint32_t i=0;i<srcLength;i++) ret+=uint16_t(src[i]);
		return ret;
	};

	auto unpackNone=[&]()
	{
		for (uint32_t i=0;i<limitedDecompress&&!inputUnObsfuscator.eof();i++)
			outputStream.writeByte(inputUnObsfuscator.readByte());
	};

	// same as simple
	auto unRLE=[&](bool lastCharMissing)->uint32_t
	{
		// hacky, hacky, hacky
		while (!inputUnObsfuscator.eof())
		{
			if (outputStream.getOffset()>=limitedDecompress) return 0;
			if (lastCharMissing && inputUnObsfuscator.eofMinus1())
			{
				if (outputStream.getOffset()+1!=outputStream.getEndOffset())
					throw DecompressionError();
				return 1;
			}
			uint8_t ch=inputUnObsfuscator.readByte();
			uint32_t count=1;
			if (ch==0x90U)
			{
				if (inputUnObsfuscator.eof()) throw DecompressionError();
				if (lastCharMissing && inputUnObsfuscator.eofMinus1())
				{
					// only possible way this can work
					if (outputStream.getOffset()+1!=outputStream.getEndOffset()) throw DecompressionError();
					count=0;
				} else count=inputUnObsfuscator.readByte();
				if (!count)
				{
					count=1;
				} else {
					
					if (inputUnObsfuscator.eof()) throw DecompressionError();
					if (lastCharMissing && inputUnObsfuscator.eofMinus1())
					{
						if (count==0xffU || outputStream.getOffset()+count!=outputStream.getEndOffset()) throw DecompressionError();
						return count;
					}
					ch=inputUnObsfuscator.readByte();
				}
				if (count==0xffU)
				{
					if (inputUnObsfuscator.eofMinus1Plus()) throw DecompressionError();
					if (lastCharMissing && inputUnObsfuscator.eofMinus2Plus())
					{
						count=uint32_t(outputStream.getEndOffset()-outputStream.getOffset());
					} else {
						count=uint32_t(inputUnObsfuscator.readByte())<<8;
						count|=inputUnObsfuscator.readByte();
					}
				}
			}
			for (uint32_t i=0;i<count;i++) outputStream.writeByte(ch);
		}
		if (!outputStream.eof()) throw DecompressionError();
		return 0;
	};

	bool doInitContext=true;
	uint8_t *contextBufferPtr=contextBuffer.data();
	// context used is 256 bytes
	uint8_t quickContextLocation;
	// context used is 16384 bytes
	uint32_t mediumContextLocation;
	uint32_t deepContextLocation;
	// context used is 4096/8192 bytes
	uint32_t heavyContextLocation;
	std::unique_ptr<DynamicHuffmanDecoder<314>> deepDecoder;
	auto initContext=[&]()
	{
		if (doInitContext)
		{
			if (_contextBufferSize) std::memset(contextBuffer.data(),0,_contextBufferSize);
			quickContextLocation=251;
			mediumContextLocation=16318;
			deepContextLocation=16324;
			deepDecoder.reset();
			heavyContextLocation=0;
			doInitContext=false;
		}
	};

	auto unpackQuick=[&]()
	{
		initContext();

		while (!outputStream.eof())
		{
			if (outputStream.getOffset()>=limitedDecompress) return;
			if (readBits(1))
			{
				outputStream.writeByte(contextBufferPtr[quickContextLocation++]=readBits(8));
			} else {
				uint32_t count=readBits(2)+2;
				uint8_t offset=quickContextLocation-readBits(8)-1;
				for (uint32_t i=0;i<count;i++)
					outputStream.writeByte(contextBufferPtr[quickContextLocation++]=contextBufferPtr[(i+offset)&0xffU]);
			}
		}
		quickContextLocation+=5;
	};

	static const uint8_t lengthTable[256]={
		 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
		 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
		 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2,
		 3, 3, 3, 3, 3, 3, 3, 3,  3, 3, 3, 3, 3, 3, 3, 3,
		 4, 4, 4, 4, 4, 4, 4, 4,  5, 5, 5, 5, 5, 5, 5, 5,
		 6, 6, 6, 6, 6, 6, 6, 6,  7, 7, 7, 7, 7, 7, 7, 7,
		 8, 8, 8, 8, 8, 8, 8, 8,  9, 9, 9, 9, 9, 9, 9, 9,
		10,10,10,10,10,10,10,10, 11,11,11,11,11,11,11,11,
		12,12,12,12,13,13,13,13, 14,14,14,14,15,15,15,15,
		16,16,16,16,17,17,17,17, 18,18,18,18,19,19,19,19,
		20,20,20,20,21,21,21,21, 22,22,22,22,23,23,23,23,
		24,24,25,25,26,26,27,27, 28,28,29,29,30,30,31,31,
		32,32,33,33,34,34,35,35, 36,36,37,37,38,38,39,39,
		40,40,41,41,42,42,43,43, 44,44,45,45,46,46,47,47,
		48,49,50,51,52,53,54,55, 56,57,58,59,60,61,62,63};

	static const uint8_t bitLengthTable[256]={
		3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
		3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
		4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
		4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
		4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8};

	auto decodeLengthValueHalf=[&](uint8_t code)->uint32_t
	{
		return (((code<<bitLengthTable[code])|readBits(bitLengthTable[code]))&0xffU);
	};

	auto decodeLengthValueFull=[&](uint8_t code)->uint32_t
	{
		return (uint32_t(lengthTable[code])<<8)|
			uint32_t(((code<<bitLengthTable[code])|readBits(bitLengthTable[code]))&0xffU);
	};

	auto unpackMedium=[&]()
	{
		initContext();

		while (!outputStream.eof())
		{
			if (outputStream.getOffset()>=limitedDecompress) return;
			if (readBits(1))
			{
				outputStream.writeByte(contextBufferPtr[mediumContextLocation++]=readBits(8));
				mediumContextLocation&=0x3fffU;
			} else {
				uint32_t code=readBits(8);
				uint32_t count=lengthTable[code]+3;

				uint32_t tmp=decodeLengthValueFull(decodeLengthValueHalf(code));

				uint32_t offset=mediumContextLocation-tmp-1;
				for (uint32_t i=0;i<count;i++)
				{
					outputStream.writeByte(contextBufferPtr[mediumContextLocation++]=contextBufferPtr[(i+offset)&0x3fffU]);
					mediumContextLocation&=0x3fffU;
				}
			}
		}
		mediumContextLocation+=66;
		mediumContextLocation&=0x3fffU;
	};

	auto unpackDeep=[&]()
	{
		initContext();
		if (!deepDecoder) deepDecoder=std::make_unique<DynamicHuffmanDecoder<314>>();

		while (!outputStream.eof())
		{
			if (outputStream.getOffset()>=limitedDecompress) return;
			uint32_t symbol=deepDecoder->decode(readBit);
			if (deepDecoder->getMaxFrequency()==0x8000U) deepDecoder->halve();
			deepDecoder->update(symbol);
			if (symbol<256)
			{
				outputStream.writeByte(contextBufferPtr[deepContextLocation++]=symbol);
				deepContextLocation&=0x3fffU;
			} else {
				uint32_t count=symbol-253;	// minimum repeat is 3
				uint32_t offset=deepContextLocation-decodeLengthValueFull(readBits(8))-1;

				for (uint32_t i=0;i<count;i++)
				{
					outputStream.writeByte(contextBufferPtr[deepContextLocation++]=contextBufferPtr[(i+offset)&0x3fffU]);
					deepContextLocation&=0x3fffU;
				}
			}
		}
		deepContextLocation+=60;
		deepContextLocation&=0x3fffU;
	};

	// these are not part of the initContext like other methods
	std::unique_ptr<OptionalHuffmanDecoder<uint32_t>> symbolDecoder,offsetDecoder;
	bool heavyLastInitialized=false;		// this is part of initContext on some implementations. screwy!!!
	uint32_t heavyLastOffset;
	auto unpackHeavy=[&](bool initTables,bool use8kDict)
	{
		initContext();
		// well, this works. Why this works? dunno
		if (!heavyLastInitialized)
		{
			heavyLastOffset=use8kDict?0U:~0U;
			heavyLastInitialized=true;
		}

		auto readTable=[&](std::unique_ptr<OptionalHuffmanDecoder<uint32_t>> &decoder,uint32_t countBits,uint32_t valueBits)
		{
			decoder=std::make_unique<OptionalHuffmanDecoder<uint32_t>>();
			uint32_t count=readBits(countBits);
			if (count)
			{
				uint8_t lengthBuffer[512];
				// in order to speed up the deObsfuscation, do not send the hopeless
				// data into slow CreateOrderlyHuffmanTable
				uint64_t sum=0;
				for (uint32_t i=0;i<count;i++)
				{
					uint32_t bits=readBits(valueBits);
					if (bits)
					{
						sum+=uint64_t(1U)<<(32-bits);
						if (sum>(uint64_t(1U)<<32))
							throw DecompressionError();
					}
					lengthBuffer[i]=bits;
				}
				decoder->createOrderlyHuffmanTable(lengthBuffer,count);
			} else {
				uint32_t index=readBits(countBits);
				decoder->setEmpty(index);
			}
		};

		if (initTables)
		{
			readTable(symbolDecoder,9,5);
			readTable(offsetDecoder,5,4);
		}

		uint32_t mask=use8kDict?0x1fffU:0xfffU;
		uint32_t bitLength=use8kDict?14:13;

		while (!outputStream.eof())
		{
			if (outputStream.getOffset()>=limitedDecompress) return;
			uint32_t symbol=symbolDecoder->decode(readBit);
			if (symbol<256)
			{
				outputStream.writeByte(contextBufferPtr[heavyContextLocation++]=symbol);
				heavyContextLocation&=mask;
			} else {
				uint32_t count=symbol-253;	// minimum repeat is 3
				uint32_t offsetLength=offsetDecoder->decode(readBit);
				uint32_t rawOffset=heavyLastOffset;
				if (offsetLength!=bitLength)
				{
					if (offsetLength) rawOffset=(1<<(offsetLength-1))|readBits(offsetLength-1);
						else rawOffset=0;
					heavyLastOffset=rawOffset;
				}
				uint32_t offset=heavyContextLocation-rawOffset-1;
				for (uint32_t i=0;i<count;i++)
				{
					outputStream.writeByte(contextBufferPtr[heavyContextLocation++]=contextBufferPtr[(i+offset)&mask]);
					heavyContextLocation&=mask;
				}
			}
		}
	};


	uint32_t trackLength=(_isHD)?22528:11264;
	for (uint32_t packedOffset=56,packedChunkLength=0;packedOffset!=_packedSize;packedOffset=OverflowCheck::sum(packedOffset,20U,packedChunkLength))
	{
		// There are some info tracks, at -1 or 80. ignore those (if still present)
		uint16_t trackNo=_packedData.readBE16(packedOffset+2);
		packedChunkLength=_packedData.readBE16(packedOffset+6);
		if (trackNo==80) break;							// should not happen, this is already excluded
		// even though only -1 should be used I've seen -2 as well. ignore all negatives
		uint32_t tmpChunkLength=_packedData.readBE16(packedOffset+8);		// after the first unpack (if twostage)
		uint32_t rawChunkLength=_packedData.readBE16(packedOffset+10);		// after final unpack
		uint8_t flags=_packedData.read8(packedOffset+12);
		uint8_t mode=_packedData.read8(packedOffset+13);

		// could affect context, but in practice they are separate, even though there is no explicit reset
		// deal with decompression though...
		if (trackNo>=0x8000U)
		{
			initInputStream(_packedData,packedOffset+20,packedChunkLength,_isObsfuscated);
			finishStream();
			continue;
		}
		if (rawChunkLength>trackLength) throw DecompressionError();
		if (trackNo>80) throw DecompressionError();				// should not happen, already excluded

		uint32_t dataOffset=trackNo*trackLength;
		if (_rawOffset>dataOffset) throw DecompressionError();

		// this is screwy, but it is, what it is
		auto processBlock=[&](bool doRLE,auto func,auto&&... params)
		{
			initInputStream(_packedData,packedOffset+20,packedChunkLength,_isObsfuscated);
			if (doRLE)
			{
				try
				{
					initOutputStream(tmpBuffer,0,tmpChunkLength);
					func(params...);
					finishStream();
					initInputStream(tmpBuffer,0,tmpChunkLength,false);
					initOutputStream(rawData,dataOffset-_rawOffset,rawChunkLength);
					unRLE(false);
				} catch (const ShortInputError &) {
					// if this error happens on repeat/offset instead of char, though luck :(
					// missing last char on src we can fix :)
					initInputStream(tmpBuffer,0,tmpChunkLength,false);
					initOutputStream(rawData,dataOffset-_rawOffset,rawChunkLength);
					uint32_t missingNo=unRLE(true);
					if (missingNo)
					{
						uint32_t protoSum=checksum(&rawData[dataOffset-_rawOffset],rawChunkLength-missingNo);
						uint32_t fileSum=_packedData.readBE16(packedOffset+14);
						uint8_t ch=((fileSum>=protoSum)?fileSum-protoSum:(0x10000U+fileSum-protoSum))/missingNo;
						for (uint32_t i=0;i<missingNo;i++) outputStream.writeByte(ch);
					} else throw DecompressionError();
				}
			} else {
				try
				{
					initOutputStream(rawData,dataOffset-_rawOffset,rawChunkLength);
					func(params...);
				} catch (const ShortInputError &) {
					// same deal
					if (outputStream.getOffset()+1!=rawChunkLength || _isObsfuscated) throw DecompressionError();
					uint32_t protoSum=checksum(&rawData[dataOffset-_rawOffset],rawChunkLength-1);
					uint32_t fileSum=_packedData.readBE16(packedOffset+14);
					uint8_t ch=fileSum-protoSum;
					outputStream.writeByte(ch);
				}
			}
			finishStream();
		};

		auto processBlockCode=[&](bool doRLE,auto func,auto&&... params)
		{
			if (!_isObsfuscated || trackNo!=_minTrack) return processBlock(doRLE,func,params...);

			// fast try
			if (!trackNo && restartPosition<0x10000U) for (;restartPosition<0x10000U;restartPosition++)
			{
				try
				{
					doInitContext=true;
					inputUnObsfuscator.setCode(restartPosition);
					limitedDecompress=8;
					processBlock(doRLE,func,params...);
					if ((rawData.readBE32(0)&0xffff'ff00U)!=FourCC("DOS\0")) continue;

					// now see if the candidate is any good
					doInitContext=true;
					inputUnObsfuscator.setCode(restartPosition);
					limitedDecompress=~0U;
					processBlock(doRLE,func,params...);
					if (checksum(&rawData[dataOffset-_rawOffset],rawChunkLength)!=_packedData.readBE16(packedOffset+14)) continue;
					return;
				} catch (const Buffer::Error &) {
					// just continue
				} catch (const Decompressor::Error &) {
					// just continue
				}
			}

			// slow round
			limitedDecompress=~0U;
			for (;restartPosition<0x20000U;restartPosition++)
			{
				try
				{
					doInitContext=true;
					inputUnObsfuscator.setCode(restartPosition);
					processBlock(doRLE,func,params...);
					if (checksum(&rawData[dataOffset-_rawOffset],rawChunkLength)!=_packedData.readBE16(packedOffset+14)) continue;
					return;
				} catch (const Buffer::Error &) {
					// just continue
				} catch (const Decompressor::Error &) {
					// just continue
				}
			}
			throw DecompressionError();
		};

		switch (mode)
		{
			case 0:
			processBlockCode(false,unpackNone);
			rawChunkLength=packedChunkLength;
			break;

			case 1:
			processBlockCode(false,unRLE,false);
			break;

			case 2:
			processBlockCode(true,unpackQuick);
			break;

			case 3:
			processBlockCode(true,unpackMedium);
			break;

			case 4:
			processBlockCode(true,unpackDeep);
			break;

			// heavy flags:
			// 2: (re-)initialize/read tables
			// 4: do RLE
			// heavy1 uses 4k dictionary (mode 5), whereas heavy2 uses 8k dictionary
			case 5:
			case 6:
			processBlockCode(flags&4,unpackHeavy,flags&2,mode==6);
			break;

			default:
			throw DecompressionError();
		}
		if (!(flags&1)) doInitContext=true;

		if (verify && checksum(&rawData[dataOffset-_rawOffset],rawChunkLength)!=_packedData.readBE16(packedOffset+14))
			throw VerificationError();
	}
}

}
