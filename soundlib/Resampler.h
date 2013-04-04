/*
 * Resampler.h
 * -----------
 * Purpose: Holds the tables for all available resamplers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once


#include "WindowedFIR.h"
#include "MixerSettings.h"


#define SINC_PHASES		4096


//======================
class CResamplerSettings
//======================
{
public:
	ResamplingMode SrcMode;
	double gdWFIRCutoff;
	uint8 gbWFIRType;
public:
	CResamplerSettings()
	{
		SrcMode = SRCMODE_FIRFILTER;
		gdWFIRCutoff = 0.97;
		gbWFIRType = 7; //WFIR_KAISER4T;
	}
};


//==============
class CResampler
//==============
{
public:
	CResamplerSettings m_Settings;
	CWindowedFIR m_WindowedFIR;
	short int gKaiserSinc[SINC_PHASES*8];		// Upsampling
	short int gDownsample13x[SINC_PHASES*8];	// Downsample 1.333x
	short int gDownsample2x[SINC_PHASES*8];		// Downsample 2x
public:
	CResampler() {}
	~CResampler() {}
	void InitializeTables();
	bool IsHQ() const { return m_Settings.SrcMode >= SRCMODE_SPLINE && m_Settings.SrcMode < SRCMODE_DEFAULT; }
	bool IsUltraHQ() const { return m_Settings.SrcMode >= SRCMODE_POLYPHASE && m_Settings.SrcMode < SRCMODE_DEFAULT; }
	bool Is(ResamplingMode cmp) const { return m_Settings.SrcMode == cmp; }
};
