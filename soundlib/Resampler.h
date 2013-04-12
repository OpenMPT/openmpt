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
	bool operator == (const CResamplerSettings &cmp) const
	{
		return SrcMode == cmp.SrcMode && gdWFIRCutoff == cmp.gdWFIRCutoff && gbWFIRType == cmp.gbWFIRType;
	}
	bool operator != (const CResamplerSettings &cmp) const { return !(*this == cmp); }
};


//==============
class CResampler
//==============
{
public:
	CResamplerSettings m_Settings;
	CWindowedFIR m_WindowedFIR;
	static const short int FastSincTable[256*4];
	short int gKaiserSinc[SINC_PHASES*8];		// Upsampling
#ifdef MODPLUG_TRACKER
	static bool StaticTablesInitialized;
	static short int gDownsample13x[SINC_PHASES*8];	// Downsample 1.333x
	static short int gDownsample2x[SINC_PHASES*8];		// Downsample 2x
#else
	// no global data which has to be initialized by hand in the library
	short int gDownsample13x[SINC_PHASES*8];	// Downsample 1.333x
	short int gDownsample2x[SINC_PHASES*8];		// Downsample 2x
#endif
private:
	CResamplerSettings m_OldSettings;
public:
	CResampler() { InitializeTables(true); }
	~CResampler() {}
	void InitializeTables(bool force=false);
	bool IsHQ() const { return m_Settings.SrcMode >= SRCMODE_SPLINE && m_Settings.SrcMode < SRCMODE_DEFAULT; }
	bool IsUltraHQ() const { return m_Settings.SrcMode >= SRCMODE_POLYPHASE && m_Settings.SrcMode < SRCMODE_DEFAULT; }
	bool Is(ResamplingMode cmp) const { return m_Settings.SrcMode == cmp; }
};
