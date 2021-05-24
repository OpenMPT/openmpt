/*
 * SampleEditorDialogs.h
 * ---------------------
 * Purpose: Code for various dialogs that are used in the sample editor.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "../common/FileReaderFwd.h"
#include "../soundlib/SampleIO.h"
#include "../tracklib/FadeLaws.h"
#include "CDecimalSupport.h"

OPENMPT_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// Sample amplification dialog

class CAmpDlg: public CDialog
{
public:
	struct AmpSettings
	{
		Fade::Law fadeLaw;
		int fadeInStart, fadeOutEnd;
		int16 factor;
		bool fadeIn, fadeOut;
	};

	AmpSettings &m_settings;
	int16 m_nFactorMin, m_nFactorMax;

protected:
	CComboBoxEx m_fadeBox;
	CImageList m_list;
	CNumberEdit m_edit, m_editFadeIn, m_editFadeOut;
	bool m_locked = true;

public:
	CAmpDlg(CWnd *parent, AmpSettings &settings, int16 factorMin = int16_min, int16 factorMax = int16_max);

protected:
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;
	void OnDestroy();

	afx_msg void EnableFadeIn() { if(!m_locked) CheckDlgButton(IDC_CHECK1, BST_CHECKED); }
	afx_msg void EnableFadeOut() { if(!m_locked) CheckDlgButton(IDC_CHECK2, BST_CHECKED); }

	DECLARE_MESSAGE_MAP()
};


//////////////////////////////////////////////////////////////////////////
// Sample import dialog

class CRawSampleDlg: public CDialog
{
	friend class AutodetectFormatDlg;

protected:
	static SampleIO m_format;
	static SmpLength m_offset;

	CSpinButtonCtrl m_SpinOffset;
	FileReader &m_file;
	bool m_rememberFormat = false;

public:
	SampleIO GetSampleFormat() const { return m_format; }
	void SetSampleFormat(SampleIO nFormat) { m_format = nFormat; }

	bool GetRemeberFormat() const { return m_rememberFormat; };
	void SetRememberFormat(bool remember) { m_rememberFormat = remember; };

	SmpLength GetOffset() const { return m_offset; }
	void SetOffset(SmpLength offset) { m_offset = offset; }

public:
	CRawSampleDlg(FileReader &file, CWnd *parent = nullptr)
		: CDialog(IDD_LOADRAWSAMPLE, parent)
		, m_file(file) {}

protected:
	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;
	void UpdateDialog();

	void OnBitDepthChanged(UINT id);
	void OnEncodingChanged(UINT id);
	void OnAutodetectFormat();

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Add silence dialog - add silence to a sample

class AddSilenceDlg: public CDialog
{
public:
	enum AddSilenceOptions
	{
		kSilenceAtBeginning, // Add at beginning of sample
		kSilenceAtEnd,       // Add at end of sample
		kResize,             // Resize sample
		kOPLInstrument,      // Initialize as OPL instrument
	};

	enum Unit
	{
		kSamples = 0,
		kMilliseconds,
	};

	SmpLength m_numSamples; // Add x samples (also containes the return value in all cases)
	SmpLength m_length;  // Set size to x samples (init value: current sample size)
	AddSilenceOptions m_editOption; // See above

protected:
	static SmpLength m_addSamples;
	static SmpLength m_createSamples;
	uint32 m_sampleRate;
	Unit m_unit = kSamples;
	bool m_allowOPL;

public:
	AddSilenceDlg(CWnd *parent, SmpLength origLength, uint32 sampleRate, bool allowOPL);

	BOOL OnInitDialog() override;
	void OnOK() override;
	
protected:
	AddSilenceOptions GetEditMode() const;
	afx_msg void OnEditModeChanged();
	afx_msg void OnUnitChanged();
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Sample grid dialog

class CSampleGridDlg: public CDialog
{
public:
	SmpLength m_nSegments, m_nMaxSegments;

protected:
	CEdit m_EditSegments;
	CSpinButtonCtrl m_SpinSegments;

public:
	CSampleGridDlg(CWnd *parent, SmpLength nSegments, SmpLength nMaxSegments) : CDialog(IDD_SAMPLE_GRID_SIZE, parent) { m_nSegments = nSegments; m_nMaxSegments = nMaxSegments; };

protected:
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;
};


/////////////////////////////////////////////////////////////////////////
// Sample cross-fade dialog

class CSampleXFadeDlg: public CDialog
{
public:
	static uint32 m_fadeLength;
	static uint32 m_fadeLaw;
	static bool m_afterloopFade;
	static bool m_useSustainLoop;
	SmpLength m_loopLength = 0, m_maxLength = 0;

protected:
	CSliderCtrl m_SliderLength, m_SliderFadeLaw;
	CEdit m_EditSamples;
	CSpinButtonCtrl m_SpinSamples;
	CButton m_RadioNormalLoop, m_RadioSustainLoop;
	ModSample &m_sample;
	bool m_editLocked = true;

public:
	CSampleXFadeDlg(CWnd *parent, ModSample &sample)
	    : CDialog(IDD_SAMPLE_XFADE, parent)
	    , m_sample(sample) {}

	SmpLength PercentToSamples(uint32 percent) const { return Util::muldivr_unsigned(percent, m_loopLength, 100000); }
	uint32 SamplesToPercent(SmpLength samples) const { return Util::muldivr_unsigned(samples, 100000, m_loopLength); }

protected:
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;

	afx_msg void OnLoopTypeChanged();
	afx_msg void OnFadeLengthChanged();
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg BOOL OnToolTipText(UINT, NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Resampling dialog

class CResamplingDlg: public CDialog
{
public:
	enum ResamplingOption
	{
		Upsample,
		Downsample,
		Custom
	};

protected:
	ResamplingMode m_srcMode;
	uint32 m_frequency;
	bool m_resampleAll;
	static uint32 lastFrequency;
	static ResamplingOption lastChoice;

public:
	CResamplingDlg(CWnd *parent, uint32 frequency, ResamplingMode srcMode, bool resampleAll) : CDialog(IDD_RESAMPLE, parent), m_srcMode(srcMode), m_frequency(frequency), m_resampleAll(resampleAll) { };
	uint32 GetFrequency() const { return m_frequency; }
	ResamplingMode GetFilter() const { return m_srcMode; }
	static ResamplingOption GetResamplingOption() { return lastChoice; }

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;

	afx_msg void OnFocusEdit() { CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO3); }

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Sample mix dialog

class CMixSampleDlg: public CDialog
{
protected:
	// Dialog controls
	CEdit m_EditOffset;
	CNumberEdit m_EditVolOriginal, m_EditVolMix;
	CSpinButtonCtrl m_SpinOffset, m_SpinVolOriginal, m_SpinVolMix;

public:
	static SmpLength sampleOffset;
	static int amplifyOriginal;
	static int amplifyMix;

public:
	CMixSampleDlg(CWnd *parent);

protected:
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;
};


OPENMPT_NAMESPACE_END
