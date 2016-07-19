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

#include "../soundlib/SampleIO.h"
#include "FadeLaws.h"
#include "CDecimalSupport.h"

OPENMPT_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// Sample amplification dialog

//===========================
class CAmpDlg: public CDialog
//===========================
{
public:
	Fade::Law m_fadeLaw;
	int16 m_nFactor, m_nFactorMin, m_nFactorMax;
	bool m_bFadeIn, m_bFadeOut;

protected:
	CComboBoxEx m_fadeBox;
	CImageList m_list;
	CNumberEdit m_edit;

public:
	CAmpDlg(CWnd *parent, int16 factor, Fade::Law fadeLaw, int16 factorMin = int16_min, int16 factorMax = int16_max);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnDestroy();
	virtual void OnOK();
};


//////////////////////////////////////////////////////////////////////////
// Sample import dialog

//=================================
class CRawSampleDlg: public CDialog
//=================================
{
protected:
	static SampleIO m_nFormat;
	bool m_bRememberFormat;

public:
	static const SampleIO GetSampleFormat() { return m_nFormat; }
	static void SetSampleFormat(SampleIO nFormat) { m_nFormat = nFormat; }
	const bool GetRemeberFormat() { return m_bRememberFormat; };
	void SetRememberFormat(bool bRemember) { m_bRememberFormat = bRemember; };

public:
	CRawSampleDlg(CWnd *parent = NULL):CDialog(IDD_LOADRAWSAMPLE, parent)
	{ 
		m_bRememberFormat = false;
	}

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	void UpdateDialog();
};


/////////////////////////////////////////////////////////////////////////
// Add silence dialog - add silence to a sample

enum enmAddSilenceOptions
{
	addsilence_at_beginning,	// Add at beginning of sample
	addsilence_at_end,			// Add at end of sample
	addsilence_resize,			// Resize sample
};

//==================================
class CAddSilenceDlg: public CDialog
//==================================
{
protected:
	enmAddSilenceOptions GetEditMode();
	afx_msg void OnEditModeChanged();
	DECLARE_MESSAGE_MAP()

public:
	UINT m_nSamples;	// Add x samples (also containes the return value in all cases)
	UINT m_nLength;		// Set size to x samples (init value: current sample size)
	enmAddSilenceOptions m_nEditOption;	// See above

public:
	CAddSilenceDlg(CWnd *parent, UINT nSamples = 32, UINT nOrigLength = 64) : CDialog(IDD_ADDSILENCE, parent)
	{
		m_nSamples = nSamples;
		if(nOrigLength > 0)
		{
			m_nLength = nOrigLength;
			m_nEditOption = addsilence_at_end;
		} else
		{
			m_nLength = 64;
			m_nEditOption = addsilence_resize;
		}
	}

	virtual BOOL OnInitDialog();
	virtual void OnOK();
};


/////////////////////////////////////////////////////////////////////////
// Sample grid dialog

//==================================
class CSampleGridDlg: public CDialog
//==================================
{
public:
	SmpLength m_nSegments, m_nMaxSegments;

protected:
	CEdit m_EditSegments;
	CSpinButtonCtrl m_SpinSegments;

public:
	CSampleGridDlg(CWnd *parent, SmpLength nSegments, SmpLength nMaxSegments) : CDialog(IDD_SAMPLE_GRID_SIZE, parent) { m_nSegments = nSegments; m_nMaxSegments = nMaxSegments; };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
};


/////////////////////////////////////////////////////////////////////////
// Sample cross-fade dialog

//===================================
class CSampleXFadeDlg: public CDialog
//===================================
{
public:
	static uint32 m_fadeLength;
	static uint32 m_fadeLaw;
	static bool m_afterloopFade;
	static bool m_useSustainLoop;
	SmpLength m_loopLength, m_maxLength;

protected:
	CSliderCtrl m_SliderLength, m_SliderFadeLaw;
	CEdit m_EditSamples;
	CSpinButtonCtrl m_SpinSamples;
	CButton m_RadioNormalLoop, m_RadioSustainLoop;
	ModSample &m_sample;
	bool m_editLocked : 1;

public:
	CSampleXFadeDlg(CWnd *parent, ModSample &sample)
		: CDialog(IDD_SAMPLE_XFADE, parent)
		, m_loopLength(0)
		, m_maxLength(0)
		, m_sample(sample)
		, m_editLocked(true) { };

	SmpLength PercentToSamples(uint32 percent) const { return Util::muldivr_unsigned(percent, m_loopLength, 100000); }
	uint32 SamplesToPercent(SmpLength samples) const { return Util::muldivr_unsigned(samples, 100000, m_loopLength); }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnLoopTypeChanged();
	afx_msg void OnFadeLengthChanged();
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg BOOL OnToolTipText(UINT, NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Resampling dialog

//==================================
class CResamplingDlg: public CDialog
//==================================
{
protected:
	enum ResamplingOption
	{
		Upsample,
		Downsample,
		Custom
	};

	ResamplingMode srcMode;
	uint32 frequency;
	static uint32 lastFrequency;
	static ResamplingOption lastChoice;

public:
	CResamplingDlg(CWnd *parent, uint32 frequency, ResamplingMode srcMode) : CDialog(IDD_RESAMPLE, parent), frequency(frequency), srcMode(srcMode) { };
	uint32 GetFrequency() const { return frequency; }
	ResamplingMode GetFilter() const { return srcMode; }

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnFocusEdit() { CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO3); }

	DECLARE_MESSAGE_MAP()
};


OPENMPT_NAMESPACE_END
