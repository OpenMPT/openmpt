#pragma once

class CPerformanceCounter
{
protected: 
	LARGE_INTEGER lFreq, lStart;
	long lFreqRDTSC, lStartRDTSC;

	inline static unsigned __int64 _RDTSC()
	{
		_asm _emit 0x0F
		_asm _emit 0x31
	}


public:
	CPerformanceCounter() {
		QueryPerformanceFrequency(&lFreq);
/*
		lFreqRDTSC=0;
		while (lFreqRDTSC<=0) {
			long start = _RDTSC();
			Sleep(1000);
			long end = _RDTSC();
			lFreqRDTSC = end-start;
		}
*/
	}
	~CPerformanceCounter(void) {};

	inline void Start() {
		QueryPerformanceCounter(&lStart);
	}

	inline double Stop() {
		LARGE_INTEGER lEnd;
		QueryPerformanceCounter(&lEnd);
		return (double(lEnd.QuadPart - lStart.QuadPart) / lFreq.QuadPart);
	}


	inline double StartStop() {
		LARGE_INTEGER lEnd;
		QueryPerformanceCounter(&lEnd);
		
		double result = (double(lEnd.QuadPart - lStart.QuadPart) / lFreq.QuadPart);
		lStart.QuadPart = lEnd.QuadPart;

		return result;
	}

	inline void StartRDTSC() {
		lStartRDTSC = _RDTSC();
	}

	inline double StopRDTSC() {
		long lEndRDTSC = _RDTSC();
		return (double(lEndRDTSC-lStartRDTSC)/lFreqRDTSC);
	}

};
