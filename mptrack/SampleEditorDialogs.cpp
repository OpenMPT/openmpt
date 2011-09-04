/*
 *
 * SampleEditorDialogs.cpp
 * -----------------------
 * Purpose: Code for various dialogs that are used in the sample editor
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 *
 */

#include "stdafx.h"
#include "mptrack.h"
#include "SampleEditorDialogs.h"


//////////////////////////////////////////////////////////////////////////
// Sample amplification dialog

CAmpDlg::CAmpDlg(CWnd *parent, int16 nFactor, int16 nFactorMin, int16 nFactorMax) 
//-------------------------------------------------------------------------------
: CDialog(IDD_SAMPLE_AMPLIFY, parent), m_nFactor(nFactor),
m_nFactorMin(nFactorMin), m_nFactorMax(nFactorMax),
m_bFadeIn(FALSE), m_bFadeOut(FALSE)
{}

BOOL CAmpDlg::OnInitDialog()
//--------------------------
{
	CDialog::OnInitDialog();
	CSpinButtonCtrl *spin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN1);
	if (spin)
	{
		spin->SetRange32(m_nFactorMin, m_nFactorMax);
		spin->SetPos32(m_nFactor);
	}
	SetDlgItemInt(IDC_EDIT1, m_nFactor);
	return TRUE;
}


void CAmpDlg::OnOK()
//------------------
{
	const int nVal = static_cast<int>(GetDlgItemInt(IDC_EDIT1));
	if(nVal < m_nFactorMin || nVal > m_nFactorMax)
	{
		CString str; str.Format(GetStrI18N(__TEXT("Value should be within [%d, %d]")), m_nFactorMin, m_nFactorMax);
		Reporting::Information(str);
		return;
	}
	m_nFactor = static_cast<int16>(nVal);
	m_bFadeIn = (IsDlgButtonChecked(IDC_CHECK1) != 0);
	m_bFadeOut = (IsDlgButtonChecked(IDC_CHECK2) != 0);
	CDialog::OnOK();
}


//////////////////////////////////////////////////////////////
// Sample import dialog

UINT CRawSampleDlg::m_nFormat = ER_8BIT | ER_UNSIGNED | ER_MONO;

BOOL CRawSampleDlg::OnInitDialog()
//--------------------------------
{
	CDialog::OnInitDialog();
	UpdateDialog();
	return TRUE;
}


void CRawSampleDlg::OnOK()
//------------------------
{
	m_nFormat = 0;
	if(IsDlgButtonChecked(IDC_RADIO1)) m_nFormat |= ER_8BIT;
	if(IsDlgButtonChecked(IDC_RADIO2)) m_nFormat |= ER_16BIT;
	if(IsDlgButtonChecked(IDC_RADIO3)) m_nFormat |= ER_UNSIGNED;
	if(IsDlgButtonChecked(IDC_RADIO4)) m_nFormat |= ER_SIGNED;
	if(IsDlgButtonChecked(IDC_RADIO5)) m_nFormat |= ER_MONO;
	if(IsDlgButtonChecked(IDC_RADIO6)) m_nFormat |= ER_STEREO;
	m_bRememberFormat = IsDlgButtonChecked(IDC_CHK_REMEMBERSETTINGS) ? true : false;
	CDialog::OnOK();
}


void CRawSampleDlg::UpdateDialog()
//--------------------------------
{
	CheckRadioButton(IDC_RADIO1, IDC_RADIO2, (m_nFormat & ER_8BIT) ? IDC_RADIO1 : IDC_RADIO2 );
	CheckRadioButton(IDC_RADIO3, IDC_RADIO4, (m_nFormat & ER_UNSIGNED) ? IDC_RADIO3 : IDC_RADIO4);
	CheckRadioButton(IDC_RADIO5, IDC_RADIO6, (m_nFormat & ER_MONO) ? IDC_RADIO5 : IDC_RADIO6);
	CheckDlgButton(IDC_CHK_REMEMBERSETTINGS, (m_bRememberFormat) ? MF_CHECKED : MF_UNCHECKED);
}


/////////////////////////////////////////////////////////////////////////
// Add silence dialog - add silence to a sample

BEGIN_MESSAGE_MAP(CAddSilenceDlg, CDialog)
	ON_COMMAND(IDC_RADIO_ADDSILENCE_BEGIN,		OnEditModeChanged)
	ON_COMMAND(IDC_RADIO_ADDSILENCE_END,		OnEditModeChanged)
	ON_COMMAND(IDC_RADIO_RESIZETO,				OnEditModeChanged)
END_MESSAGE_MAP()


BOOL CAddSilenceDlg::OnInitDialog()
//---------------------------------
{
	CDialog::OnInitDialog();

	CSpinButtonCtrl *spin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_ADDSILENCE);
	if (spin)
	{
		spin->SetRange(0, int16_max);
		spin->SetPos(m_nSamples);
	}

	int iRadioButton = IDC_RADIO_ADDSILENCE_END;
	switch(m_nEditOption)
	{
	case addsilence_at_beginning:
		iRadioButton = IDC_RADIO_ADDSILENCE_BEGIN;
		break;
	case addsilence_at_end:
		iRadioButton = IDC_RADIO_ADDSILENCE_END;
		break;
	case addsilence_resize:
		iRadioButton = IDC_RADIO_RESIZETO;
		break;
	}
	CButton *radioEnd = (CButton *)GetDlgItem(iRadioButton);
	radioEnd->SetCheck(true);

	SetDlgItemInt(IDC_EDIT_ADDSILENCE, (m_nEditOption == addsilence_resize) ? m_nLength : m_nSamples);

	return TRUE;
}


void CAddSilenceDlg::OnOK()
//-------------------------
{
	m_nSamples = GetDlgItemInt(IDC_EDIT_ADDSILENCE);
	m_nEditOption = GetEditMode();
	CDialog::OnOK();
}


void CAddSilenceDlg::OnEditModeChanged()
//------------------------------------------------
{
	enmAddSilenceOptions cNewEditOption = GetEditMode();
	if(cNewEditOption != addsilence_resize && m_nEditOption == addsilence_resize)
	{
		// switch to "add silenece"
		m_nLength = GetDlgItemInt(IDC_EDIT_ADDSILENCE);
		SetDlgItemInt(IDC_EDIT_ADDSILENCE, m_nSamples);
	} else if(cNewEditOption == addsilence_resize && m_nEditOption != addsilence_resize)
	{
		// switch to "resize"
		m_nSamples = GetDlgItemInt(IDC_EDIT_ADDSILENCE);
		SetDlgItemInt(IDC_EDIT_ADDSILENCE, m_nLength);
	}
	m_nEditOption = cNewEditOption;
}


enmAddSilenceOptions CAddSilenceDlg::GetEditMode()
{
	if(IsDlgButtonChecked(IDC_RADIO_ADDSILENCE_BEGIN)) return addsilence_at_beginning;
	else if(IsDlgButtonChecked(IDC_RADIO_ADDSILENCE_END)) return addsilence_at_end;
	else if(IsDlgButtonChecked(IDC_RADIO_RESIZETO)) return addsilence_resize;
	return addsilence_at_end;
}


/////////////////////////////////////////////////////////////////////////
// Sample grid dialog

void CSampleGridDlg::DoDataExchange(CDataExchange* pDX)
//-----------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSampleGridDlg)
	DDX_Control(pDX, IDC_EDIT1,			m_EditSegments);
	DDX_Control(pDX, IDC_SPIN1,			m_SpinSegments);
	//}}AFX_DATA_MAP
}


BOOL CSampleGridDlg::OnInitDialog()
//---------------------------------
{
	CDialog::OnInitDialog();
	m_SpinSegments.SetRange32(0, m_nMaxSegments);
	m_SpinSegments.SetPos(m_nSegments);
	SetDlgItemInt(IDC_EDIT1, m_nSegments, FALSE);
	GetDlgItem(IDC_EDIT1)->SetFocus();
	return TRUE;
}


void CSampleGridDlg::OnOK()
//-------------------------
{
	m_nSegments = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////
// Sample cross-fade dialog

void CSampleXFadeDlg::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSampleGridDlg)
	DDX_Control(pDX, IDC_EDIT1,			m_EditSamples);
	DDX_Control(pDX, IDC_SPIN1,			m_SpinSamples);
	//}}AFX_DATA_MAP
}


BOOL CSampleXFadeDlg::OnInitDialog()
//----------------------------------
{
	CDialog::OnInitDialog();
	m_SpinSamples.SetRange32(0, m_nMaxSamples);
	m_SpinSamples.SetPos(m_nSamples);
	SetDlgItemInt(IDC_EDIT1, m_nSamples, FALSE);
	GetDlgItem(IDC_EDIT1)->SetFocus();
	return TRUE;
}


void CSampleXFadeDlg::OnOK()
//--------------------------
{
	m_nSamples = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
	CDialog::OnOK();
}

