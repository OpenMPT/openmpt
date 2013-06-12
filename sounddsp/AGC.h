/*
 * AGC.h
 * -----
 * Purpose: Automatic Gain Control
 * Notes  : Ugh... This should really be removed at some point.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once


#ifndef NO_AGC

//========
class CAGC
//========
{
private:
	UINT m_nAGC;
	DWORD m_nAGCRecoverCount;
	UINT m_Timeout;
public:
	CAGC();
	void Initialize(BOOL bReset, DWORD MixingFreq);
public:
	void Process(int *MixSoundBuffer, int *RearSoundBuffer, int count, UINT nChannels);
	void Adjust(UINT oldVol, UINT newVol);
};

#endif // NO_AGC
