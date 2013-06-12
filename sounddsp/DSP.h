/*
 * DSP.h
 * -----
 * Purpose: Mixing code for various DSPs (EQ, Mega-Bass, ...)
 * Notes  : Ugh... This should really be removed at some point.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#ifndef NO_DSP

// Buffer Sizes
#define SURROUNDBUFFERSIZE		2048	// 50ms @ 48kHz


//================
class CDSPSettings
//================
{
public:
	UINT m_nXBassDepth;
	UINT m_nXBassRange;
	UINT m_nProLogicDepth;
	UINT m_nProLogicDelay;
public:
	CDSPSettings();
};


//========
class CDSP
//========
{
public:
	CDSPSettings m_Settings;
private:

	// Noise Reduction: simple low-pass filter
	LONG nLeftNR;
	LONG nRightNR;

	// Surround Encoding: 1 delay line + low-pass filter + high-pass filter
	LONG nSurroundSize;
	LONG nSurroundPos;
	LONG nDolbyDepth;
	
	// Surround Biquads
	LONG nDolbyHP_Y1;
	LONG nDolbyHP_X1;
	LONG nDolbyLP_Y1;
	LONG nDolbyHP_B0;
	LONG nDolbyHP_B1;
	LONG nDolbyHP_A1;
	LONG nDolbyLP_B0;
	LONG nDolbyLP_B1;
	LONG nDolbyLP_A1;

	// Bass Expansion: low-pass filter
	LONG nXBassFlt_Y1;
	LONG nXBassFlt_X1;
	LONG nXBassFlt_B0;
	LONG nXBassFlt_B1;
	LONG nXBassFlt_A1;

	// DC Removal Biquad
	LONG nDCRFlt_Y1l;
	LONG nDCRFlt_X1l;
	LONG nDCRFlt_Y1r;
	LONG nDCRFlt_X1r;

	LONG SurroundBuffer[SURROUNDBUFFERSIZE];

public:
	CDSP();
public:
	void SetSettings(const CDSPSettings &settings) { m_Settings = settings; }
	// [XBass level 0(quiet)-100(loud)], [cutoff in Hz 10-100]
	BOOL SetXBassParameters(UINT nDepth, UINT nRange);
	// [Surround level 0(quiet)-100(heavy)] [delay in ms, usually 5-40ms]
	BOOL SetSurroundParameters(UINT nDepth, UINT nDelay);
	void Initialize(BOOL bReset, DWORD MixingFreq, DWORD DSPMask);
	void Process(int * MixSoundBuffer, int * MixRearBuffer, int count, UINT nChannels, DWORD DSPMask);
private:
	void ProcessStereoSurround(int * MixSoundBuffer, int count);
	void ProcessQuadSurround(int * MixSoundBuffer, int * MixRearBuffer, int count);
};

#endif // NO_DSP
