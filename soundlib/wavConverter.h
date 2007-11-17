#ifndef WAVCONVERTER_H
#define WAVCONVERTER_H
//Very rough wav converting functions used when loading some wav-files.

typedef void (*DATACONV)(const char* const, char*, const double);
//srcBuffer, destBuffer, maximum value in srcBuffer.

typedef double MAXFINDER(const char* const, const size_t);
//Buffer, bufferSize.

template<BYTE INBYTES>
double MaxFinderSignedInt(const char* const buffer, const size_t bs)
//----------------------------------------------------------------------
{
	if(INBYTES > 4) return 0;
	if(bs < INBYTES) return 0;
	
	uint32 max = 0;
	for(size_t i = 0; i <= bs-INBYTES; i += INBYTES)
	{
		int32 temp = 0;
		memcpy((char*)(&temp)+(4-INBYTES), buffer + i, INBYTES);
		if(temp < 0) temp = -temp;
		if(temp < 0)
		{
			max = static_cast<uint32>(int32_min);
			max >>= 8*(4-INBYTES);
			break; //This is the max possible value so no need to look for bigger one.
		}
		temp >>= 8*(4-INBYTES);
		if(static_cast<uint32>(temp) > max)
			max = static_cast<uint32>(temp);
	}
	return static_cast<double>(max);
}


inline double MaxFinderFloat32(const char* const buffer, const size_t bs)
//----------------------------------------------------------------------
{
	if(bs < 4) return 0;

	float max = 0;
	for(size_t i = 0; i<=bs-4; i+=4)
	{
		float temp = *reinterpret_cast<const float*>(buffer+i);
		temp = fabs(temp);
		if(temp > max)
			max = temp;
	}
	return max;
}


inline void WavSigned24To16(const char* const inBuffer, char* outBuffer, const double max)
//--------------------------------------------------------------------------------
{
	int32 val = 0;
	memcpy((char*)(&val)+1, inBuffer, 3);
	//Reading 24 bit data to three last bytes in 32 bit int.

	bool negative = (val < 0) ? true : false;
	if(negative) val = -val;
	double absval = (val < 0) ? -int32_min : val;

	absval /= 256;

	const double NC = (negative) ? 32768 : 32767; //Normalisation Constant
	absval = static_cast<int32>((absval/max)*NC);

	ASSERT(absval - 32768 <= 0);

	int16 outVal = static_cast<int16>(absval);

	if(negative) outVal = -outVal;

	memcpy(outBuffer, &outVal, 2);
}

inline void WavFloat32To16(const char* const inBuffer, char* outBuffer, double max)
//----------------------------------------------------------------------
{
	int16* pOut = reinterpret_cast<int16*>(outBuffer);
	const float* pIn = reinterpret_cast<const float*>(inBuffer);
	const double NC = (*pIn < 0) ? 32768 : 32767;
	*pOut = static_cast<int16>((*pIn / max) * NC);
}

inline void WavSigned32To16(const char* const inBuffer, char* outBuffer, double max)
//----------------------------------------------------------------------
{
	int16& out = *reinterpret_cast<int16*>(outBuffer);
	double temp = *reinterpret_cast<const int32*>(inBuffer);
	const double NC = (temp < 0) ? 32768 : 32767;
	out = static_cast<int16>(temp / max * NC);
}


template<size_t inBytes, size_t outBytes, DATACONV CopyAndConvert, MAXFINDER MaxFinder>
bool CopyWavBuffer(const char* const readBuffer, const size_t rbSize, char* writeBuffer, const size_t wbSize)
//----------------------------------------------------------------------
{
	if(inBytes > rbSize || outBytes > wbSize) return true;
	if(inBytes > 4) return true;
	size_t rbCounter = 0;
	size_t wbCounter = 0;

	//Finding max value
	const double max = MaxFinder(readBuffer, rbSize);

	if(max == 0) return true;
	//Silence sample won't(?) get loaded because of this.

	//Copying buffer.
	while(rbCounter <= rbSize-inBytes && wbCounter <= wbSize-outBytes)
	{
		CopyAndConvert(readBuffer+rbCounter, writeBuffer+wbCounter, max);
		rbCounter += inBytes;
		wbCounter += outBytes;
	}
	return false;
}



#endif
