#ifndef WAVCONVERTER_H
#define WAVCONVERTER_H

#include <limits>

typedef void (*DATACONV)(char* const, char*, const double);
//srcBuffer, destBuffer, maximum value in srcBuffer.

typedef double MAXFINDER(const char* const, const size_t);
//Buffer, bufferSize.

inline void WavSigned8To16(char* const inBuffer, char* outBuffer, const double max)
//----------------------------------------------------------------------
{
	outBuffer[0] = 0;
	outBuffer[1] = inBuffer[0];
}


template<BYTE INBYTES>
inline double MaxFinderInt(const char* const buffer, const size_t bs)
//----------------------------------------------------------------------
{
	if(INBYTES > 8) return 0;
	if(bs < INBYTES) return 0;
	
	__int64 max = 0;
	for(size_t i = 0; i <= bs-INBYTES; i += INBYTES)
	{
		__int64 temp = 0;
		memcpy((char*)(&temp)+(8-INBYTES), buffer + i, INBYTES);
		if(temp < 0) temp = -temp;
		temp >>= 8*INBYTES;
		if(temp > max)
			max = temp;
	}
	return static_cast<double>(max);
}


inline double MaxFinderFloat32(const char* const buffer, const size_t bs)
//----------------------------------------------------------------------
{
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


inline void WavSigned24To16(char* const inBuffer, char* outBuffer, const double max)
//--------------------------------------------------------------------------------
{
	__int32 val = 0;
	memcpy((char*)(&val)+1, inBuffer, 3);
	//Reading 24 bit data to three last bytes in 32 bit int.

	ASSERT((val << 24) == 0);

	bool negative = (val < 0) ? true : false;
	if(negative) val = -val;
	if(val < 0) val = (std::numeric_limits<__int32>::max)();
	//Handling special case that val is at minimum of int32 limit.

	val >>= 8;

	const __int64 NC = 32766; //Normalisation constant (2^16 / 2)-1.
	const __int64 maxInt = static_cast<__int64>(max);
	val = static_cast<__int32>(val*NC/maxInt);

	ASSERT((val >> 15) == 0);

	__int16 outVal = static_cast<__int16>(val);

	if(negative) outVal = -outVal;

	memcpy(outBuffer, &outVal, 2);
}

inline void WavFloat32To16(char* const inBuffer, char* outBuffer, double max)
//----------------------------------------------------------------------
{
	__int16* pOut = reinterpret_cast<__int16*>(outBuffer);
	float* pIn = reinterpret_cast<float*>(inBuffer);
	*pOut = static_cast<__int16>((*pIn / max) * 32766);
}

inline void WavSigned32To16(char* const inBuffer, char* outBuffer, double max)
//----------------------------------------------------------------------
{
	if(max >= 1)
	{
		double temp = *reinterpret_cast<const __int32*>(inBuffer);
		__int16& pOut = *reinterpret_cast<__int16*>(outBuffer);
		ASSERT(static_cast<__int16>(fabs(temp) / max * 32766) <= 32766);
		pOut = static_cast<__int16>(temp / max * 32766);
	}
	else //No normalisation
	{
		outBuffer[0] = inBuffer[2];
		outBuffer[1] = inBuffer[3];
	}
}


template<size_t inBytes, size_t outBytes, DATACONV CopyAndConvert, MAXFINDER MaxFinder>
bool CopyWavBuffer(char* const readBuffer, const size_t rbSize, char* writeBuffer, const size_t wbSize)
//----------------------------------------------------------------------
{
	if(inBytes > rbSize || outBytes > wbSize) return true;
	if(inBytes > 8) return true;
	size_t rbCounter = 0;
	size_t wbCounter = 0;

	//Finding max value
	const double max = MaxFinder(readBuffer, rbSize);

	if(max == 0) return true;

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
