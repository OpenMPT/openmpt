/*
 * SampleEditorDialogs.h
 * ---------------------
 * Purpose: Header file for misc sample editor dialog functionality
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 */

#ifndef SAMPLEEDITORDIALOGS_H
#define SAMPLEEDITORDIALOGS_H
#pragma once

//////////////////////////////////////////////////////////////////////////
// Sample amplification dialog

//===========================
class CAmpDlg: public CDialog
//===========================
{
public:
	int16 m_nFactor, m_nFactorMin, m_nFactorMax;
	bool m_bFadeIn, m_bFadeOut;

public:
	CAmpDlg(CWnd *parent, int16 nFactor=100, int16 nFactorMin = int16_min, int16 nFactorMax = int16_max);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
};


//////////////////////////////////////////////////////////////////////////
// Sample import dialog

#define ER_8BIT			0x01
#define ER_16BIT		0x02

#define ER_SIGNED		0x10
#define ER_UNSIGNED		0x20

#define ER_MONO			0x0100
#define ER_STEREO		0x0200

//=================================
class CRawSampleDlg: public CDialog
//=================================
{
protected:
	static UINT m_nFormat;
	bool m_bRememberFormat;

public:
	static const UINT GetSampleFormat() { return m_nFormat; }
	static void SetSampleFormat(UINT nFormat) { m_nFormat = nFormat; }
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
	addsilence_at_beginning = 1,	// Add at beginning of sample
	addsilence_at_end,				// Add at end of sample
	addsilence_resize,				// Resize sample
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
	int m_nSegments, m_nMaxSegments;

protected:
	CEdit m_EditSegments;
	CSpinButtonCtrl m_SpinSegments;

public:
	CSampleGridDlg(CWnd *parent, int nSegments, int nMaxSegments) : CDialog(IDD_SAMPLE_GRID_SIZE, parent) { m_nSegments = nSegments; m_nMaxSegments = nMaxSegments; };

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
	UINT m_nSamples, m_nMaxSamples;

protected:
	CEdit m_EditSamples;
	CSpinButtonCtrl m_SpinSamples;

public:
	CSampleXFadeDlg(CWnd *parent, UINT nSamples, UINT nMaxSamples) : CDialog(IDD_SAMPLE_XFADE, parent) { m_nSamples = nSamples; m_nMaxSamples = nMaxSamples; };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
};


#endif
