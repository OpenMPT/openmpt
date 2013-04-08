/*
 * EQ.h
 * ----
 * Purpose: Mixing code for equalizer.
 * Notes  : Ugh... This should really be removed at some point.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#include "../soundlib/SoundFilePlayConfig.h"


#define REAL			float
#define MAX_EQ_BANDS	6

#pragma pack(push, 4)
typedef struct _EQBANDSTRUCT
{
	REAL a0, a1, a2, b1, b2;
	REAL x1, x2, y1, y2;
	REAL Gain, CenterFrequency;
	BOOL bEnable;
} EQBANDSTRUCT, *PEQBANDSTRUCT;
#pragma pack(pop)


//=======
class CEQ
//=======
{
private:
	EQBANDSTRUCT gEQ[MAX_EQ_BANDS*2];
public:
	CEQ();
	~CEQ() {}
public:
	void Initialize(BOOL bReset, DWORD MixingFreq);
	void ProcessStereo(int *pbuffer, float *MixFloatBuffer, UINT nCount, CSoundFilePlayConfig &config, DWORD SoundSetupFlags, DWORD SysInfoFlags);
	void ProcessMono(int *pbuffer, float *MixFloatBuffer, UINT nCount, CSoundFilePlayConfig &config);
	void SetEQGains(const UINT *pGains, UINT nGains, const UINT *pFreqs, BOOL bReset, DWORD MixingFreq);
};

