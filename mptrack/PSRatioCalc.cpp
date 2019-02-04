/*
 * PSRatioCalc.cpp
 * ---------------
 * Purpose: Dialog for calculating sample pitch shift ratios in the sample editor.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "PSRatioCalc.h"


OPENMPT_NAMESPACE_BEGIN


// CPSRatioCalc dialog

IMPLEMENT_DYNAMIC(CPSRatioCalc, CDialog)
CPSRatioCalc::CPSRatioCalc(const CSoundFile &sndFile, SAMPLEINDEX sample, double ratio, CWnd* pParent /*=NULL*/)
	: CDialog(IDD_PITCHSHIFT, pParent)
	, m_dRatio(ratio)
	, sndFile(sndFile)
	, sampleIndex(sample)
{
	// Calculate/verify samplerate at C5.
	const ModSample &smp = sndFile.GetSample(sampleIndex);
	uint32 sampleRate = smp.GetSampleRate(sndFile.GetType());
	if(sampleRate <= 0)
		sampleRate = 8363;

	m_nSpeed = sndFile.m_PlayState.m_nMusicSpeed;
	m_nTempo = sndFile.m_PlayState.m_nMusicTempo;

	// Sample rate will not change. We can calculate original duration once and disgard sampleRate.
	m_lMsOrig = static_cast<ULONGLONG>(1000.0 * ((double)smp.nLength / sampleRate));
	CalcSamples();
	CalcMs();
	CalcRows();
}


void CPSRatioCalc::DoDataExchange(CDataExchange* pDX)
{
	CWnd* hasFocus = GetFocus();

	CDialog::DoDataExchange(pDX);
	SmpLength origLength = sndFile.GetSample(sampleIndex).nLength;
	DDX_Text(pDX, IDC_SAMPLE_LENGTH_ORIGINAL, origLength);
	DDX_Text(pDX, IDC_SAMPLE_LENGTH_NEW, m_lSamplesNew);
	DDX_Text(pDX, IDC_MS_LENGTH_ORIGINAL2, m_lMsOrig);
	DDX_Text(pDX, IDC_MS_LENGTH_NEW, m_lMsNew);
	DDX_Text(pDX, IDC_SPEED, m_nSpeed);
	DDX_Text(pDX, IDC_ROW_LENGTH_ORIGINAL, m_dRowsOrig);

	//These 2 CEdits must only be updated if they don't have focus (to preserve trailing . and 0s etc..)
	if (pDX->m_bSaveAndValidate || hasFocus != GetDlgItem(IDC_ROW_LENGTH_NEW2))
		DDX_Text(pDX, IDC_ROW_LENGTH_NEW2, m_dRowsNew);
	if (pDX->m_bSaveAndValidate || hasFocus != GetDlgItem(IDC_PSRATIO))
		DDX_Text(pDX, IDC_PSRATIO, m_dRatio);
}


BEGIN_MESSAGE_MAP(CPSRatioCalc, CDialog)
	ON_EN_UPDATE(IDC_SAMPLE_LENGTH_NEW, &CPSRatioCalc::OnEnChangeSamples)
	ON_EN_UPDATE(IDC_MS_LENGTH_NEW, &CPSRatioCalc::OnEnChangeMs)
	ON_EN_UPDATE(IDC_SPEED, &CPSRatioCalc::OnEnChangeSpeed)
	ON_EN_UPDATE(IDC_TEMPO, &CPSRatioCalc::OnEnChangeSpeed)
	ON_EN_UPDATE(IDC_ROW_LENGTH_NEW2, &CPSRatioCalc::OnEnChangeRows)
	ON_EN_UPDATE(IDC_PSRATIO, &CPSRatioCalc::OnEnChangeratio)
	ON_BN_CLICKED(IDOK, &CPSRatioCalc::OnBnClickedOk)
END_MESSAGE_MAP()


BOOL CPSRatioCalc::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_EditTempo.SubclassDlgItem(IDC_TEMPO, this);
	m_EditTempo.AllowNegative(false);
	m_EditTempo.SetTempoValue(m_nTempo);
	return TRUE;
}

// CPSRatioCalc message handlers
void CPSRatioCalc::OnEnChangeSamples()
{
	UpdateData();
	if (m_lSamplesNew && sndFile.GetSample(sampleIndex).nLength)
	{
		m_dRatio = (double)m_lSamplesNew / (double)sndFile.GetSample(sampleIndex).nLength * 100;
		CalcMs();
		CalcRows();
		UpdateData(FALSE);
	}
}

void CPSRatioCalc::OnEnChangeMs()
{
	UpdateData();
	if (m_lMsOrig && m_lMsNew)
	{
		m_dRatio = (double)m_lMsNew / (double)m_lMsOrig * 100;
		CalcSamples();
		CalcRows();
		UpdateData(FALSE);
	}
	
}

void CPSRatioCalc::OnEnChangeRows()
{
	UpdateData();
	if (m_dRowsOrig && m_dRowsNew)
	{
		m_dRatio = m_dRowsNew / m_dRowsOrig * 100.0;
		CalcSamples();
		CalcMs();	
		UpdateData(FALSE);
	}

}

void CPSRatioCalc::OnEnChangeSpeed()
{
	UpdateData();
	m_nTempo = m_EditTempo.GetTempoValue();
	if (m_nTempo < TEMPO(1, 0)) m_nTempo = TEMPO(1, 0);
	if (m_nSpeed < 1) m_nSpeed = 1;
	CalcRows();
	UpdateData(FALSE);
}

void CPSRatioCalc::OnEnChangeratio()
{
	UpdateData();
	if (m_dRatio)
	{
		CalcSamples();
		CalcMs();
		CalcRows();
		UpdateData(FALSE);
	}
}

// CPSRatioCalc Internal Calculations:

void CPSRatioCalc::CalcSamples()
{
	m_lSamplesNew = static_cast<ULONGLONG>(sndFile.GetSample(sampleIndex).nLength * (m_dRatio / 100.0));
	return;
}

void CPSRatioCalc::CalcMs()
{
	m_lMsNew = static_cast<ULONGLONG>(m_lMsOrig * (m_dRatio / 100.0));
	return;
}

void CPSRatioCalc::CalcRows()
{
	double rowTime = sndFile.GetRowDuration(m_nTempo, m_nSpeed);

	m_dRowsOrig = (double)m_lMsOrig / rowTime;
	m_dRowsNew = m_dRowsOrig * (m_dRatio / 100);

	return;
}

void CPSRatioCalc::OnBnClickedOk()
{
	if (m_dRatio<50.0 || m_dRatio>200.0)
	{
		Reporting::Error("Error: ratio must be between 50% and 200%.");
		return;
	}
	OnOK();
}


OPENMPT_NAMESPACE_END
