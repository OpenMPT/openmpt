/*
 * SampleGenerator.h
 * -----------------
 * Purpose: Generate samples from math formulas using muParser
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifdef MPT_DISABLED_CODE

#include "mptrack.h"
#include "Mainfrm.h"
#include "Sndfile.h"
#include "../muParser/include/muParser.h"

// sample length
#define SMPGEN_MINLENGTH 1
#define SMPGEN_MAXLENGTH MAX_SAMPLE_LENGTH
// sample frequency
#define SMPGEN_MINFREQ 1
#define SMPGEN_MAXFREQ 96000 // MAX_SAMPLE_RATE
// 16-bit sample quality - when changing this, also change CSampleGenerator::sampling_type and 16-bit flags in SampleGenerator.cpp!
#define SMPGEN_MIXBYTES 2

enum smpgen_clip_methods
{
	smpgen_clip,
	smpgen_overflow,
	smpgen_normalize,
};

//====================
class CSampleGenerator
//====================
{
protected:
	
	// sample parameters
	static int sample_frequency;
	static int sample_length;
	static mu::string_type expression;
	static smpgen_clip_methods sample_clipping;

	// rendering helper variables (they're here for the callback functions)
	static mu::value_type *sample_buffer;
	static size_t samples_written;

	typedef int16 sampling_type; // has to match SMPGEN_MIXBYTES!
	static const sampling_type sample_maxvalue = (1 << ((SMPGEN_MIXBYTES << 3) - 1)) - 1;

	// muParser object for parsing the expression
	mu::Parser muParser;

	// Rendering callback functions
	// functions
	static mu::value_type ClipCallback(mu::value_type val, mu::value_type min, mu::value_type max) { return Clamp(val, min, max); };
	static mu::value_type PWMCallback(mu::value_type pos, mu::value_type duty, mu::value_type width) { if(width == 0) return 0; else return (fmod(pos, width) < ((duty / 100) * width)) ? 1 : -1; };
	static mu::value_type RndCallback(mu::value_type v) { return v * std::rand() / (mu::value_type)(RAND_MAX + 1.0); };
	static mu::value_type SampleDataCallback(mu::value_type v);
	static mu::value_type TriangleCallback(mu::value_type pos, mu::value_type width) { if((int)width == 0) return 0; else return mpt::abs(((int)pos % (int)(width)) - width / 2) / (width / 4) - 1; };

	// binary operators
	static mu::value_type ModuloCallback(mu::value_type x, mu::value_type y) { if(y == 0) return 0; else return fmod(x , y); };

	void ShowError(mu::Parser::exception_type *e);

public:

	bool ShowDialog();
	bool TestExpression();
	bool CanRenderSample() const;
	bool RenderSample(CSoundFile *pSndFile, SAMPLEINDEX nSample);

	CSampleGenerator();

};


//////////////////////////////////////////////////////////////////////////
// Sample Generator Formula Preset implementation


struct samplegen_expression
{
	std::string description;	// e.g. "Pulse"
	mu::string_type expression;	// e.g. "pwm(x,y,z)" - empty if this is a sub menu
};
#define MAX_SAMPLEGEN_PRESETS 100


//==================
class CSmpGenPresets
//==================
{
protected:
	vector<samplegen_expression> presets;

public:
	bool AddPreset(samplegen_expression new_preset) { if(GetNumPresets() >= MAX_SAMPLEGEN_PRESETS) return false; presets.push_back(new_preset); return true;};
	bool RemovePreset(size_t which) { if(which < GetNumPresets()) { presets.erase(presets.begin() + which); return true; } else return false; };
	samplegen_expression *GetPreset(size_t which) { if(which < GetNumPresets()) return &presets[which]; else return nullptr; };
	size_t GetNumPresets() { return presets.size(); };
	void Clear() { presets.clear(); };

	CSmpGenPresets() { Clear(); }
	~CSmpGenPresets() { Clear(); }
};


//////////////////////////////////////////////////////////////////////////
// Sample Generator Dialog implementation


//=================================
class CSmpGenDialog: public CDialog
//=================================
{
protected:

	// sample parameters
	int sample_frequency;
	int sample_length;
	double sample_seconds;
	mu::string_type expression;
	smpgen_clip_methods sample_clipping;
	// pressed "OK"?
	bool apply;
	// preset slots
	CSmpGenPresets presets;

	HFONT hButtonFont; // "Marlett" font for "dropdown" button

	void RecalcParameters(bool secondsChanged, bool forceRefresh = false);

	// function presets
	void CreateDefaultPresets();

public:

	const int GetFrequency() { return sample_frequency; };
	const int GetLength() { return sample_length; };
	const smpgen_clip_methods GetClipping() { return sample_clipping; }
	const mu::string_type GetExpression() { return expression; };
	bool CanApply() { return apply; };
	
	CSmpGenDialog(int freq, int len, smpgen_clip_methods clipping, mu::string_type expr):CDialog(IDD_SAMPLE_GENERATOR, CMainFrame::GetMainFrame())
	{
		sample_frequency = freq;
		sample_length = len;
		sample_clipping = clipping;
		expression = expr;
		apply = false;
	}

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	afx_msg void OnSampleLengthChanged();
	afx_msg void OnSampleSecondsChanged();
	afx_msg void OnSampleFreqChanged();
	afx_msg void OnExpressionChanged();
	afx_msg void OnShowExpressions();
	afx_msg void OnShowPresets();
	afx_msg void OnInsertExpression(UINT nId);
	afx_msg void OnSelectPreset(UINT nId);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//////////////////////////////////////////////////////////////////////////
// Sample Generator Preset Dialog implementation


//====================================
class CSmpGenPresetDlg: public CDialog
//====================================
{
protected:
	CSmpGenPresets *presets;
	size_t currentItem;	// first item is actually 1!

	void RefreshList();

public:
	CSmpGenPresetDlg(CSmpGenPresets *pPresets):CDialog(IDD_SAMPLE_GENERATOR_PRESETS, CMainFrame::GetMainFrame())
	{
		presets = pPresets;
		currentItem = 0;
	}

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnListSelChange();

	afx_msg void OnTextChanged();
	afx_msg void OnExpressionChanged();

	afx_msg void OnAddPreset();
	afx_msg void OnRemovePreset();

	DECLARE_MESSAGE_MAP()
};

#endif // MPT_DISABLED_CODE
