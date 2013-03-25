/*
 * AGC.h
 * -----
 * Purpose: Mixing code for various DSPs (EQ, Mega-Bass, ...)
 * Notes  : Ugh... This should really be removed at some point.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

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
public:
	CAGC();
	~CAGC() {}
public:
	void Initialize(BOOL bReset, DWORD MixingFreq, DWORD SoundSetupFlags);
	void Process(int * MixSoundBuffer, int count, DWORD MixingFreq, UINT nChannels);
	void Adjust(UINT oldVol, UINT newVol);
	void Reset();
private:
};

#endif // NO_AGC
