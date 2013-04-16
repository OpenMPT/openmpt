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


#define MAX_EQ_BANDS	6

typedef struct ALIGN(4) _EQBANDSTRUCT
{
	float32 a0;
	float32 a1;
	float32 a2;
	float32 b1;
	float32 b2;
	float32 x1;
	float32 x2;
	float32 y1;
	float32 y2;
	float32 Gain;
	float32 CenterFrequency;
	bool bEnable;
} EQBANDSTRUCT;

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

