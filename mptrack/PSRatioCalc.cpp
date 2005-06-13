// PSRatioCalc.cpp : implementation file
//

#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "PSRatioCalc.h"
#include ".\psratiocalc.h"
#include "sndfile.h"		//for tempo mode enum

// CPSRatioCalc dialog

IMPLEMENT_DYNAMIC(CPSRatioCalc, CDialog)
CPSRatioCalc::CPSRatioCalc(ULONGLONG samples, ULONGLONG sampleRate, UINT speed, UINT tempo, UINT rowsPerBeat, BYTE tempoMode, double ratio, CWnd* pParent /*=NULL*/)
	: CDialog(CPSRatioCalc::IDD, pParent)
	, m_lSamplesOrig(samples), m_nSpeed(speed), m_nTempo(tempo), m_dRatio(ratio), m_nRowsPerBeat(rowsPerBeat), m_nTempoMode(tempoMode)
{
	//Sample rate will not change. We can calculate original duration once and disgard sampleRate.
	m_lMsOrig=1000.0*((double)m_lSamplesOrig / sampleRate);
	CalcSamples();
	CalcMs();
	CalcRows();
}

CPSRatioCalc::~CPSRatioCalc()
{
}

void CPSRatioCalc::DoDataExchange(CDataExchange* pDX)
{
	CWnd* hasFocus = GetFocus();

	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SAMPLE_LENGTH_ORIGINAL, m_lSamplesOrig);
	DDX_Text(pDX, IDC_SAMPLE_LENGTH_NEW, m_lSamplesNew);
	DDX_Text(pDX, IDC_MS_LENGTH_ORIGINAL2, m_lMsOrig);
	DDX_Text(pDX, IDC_MS_LENGTH_NEW, m_lMsNew);
	DDX_Text(pDX, IDC_SPEED, m_nSpeed);
	DDX_Text(pDX, IDC_TEMPO, m_nTempo);
	DDX_Text(pDX, IDC_ROW_LENGTH_ORIGINAL, m_dRowsOrig);

	//These 2 CEdits must only be updated if they don't have focus (to preserve trailing . and 0s etc..)
	if (pDX->m_bSaveAndValidate || hasFocus != GetDlgItem(IDC_ROW_LENGTH_NEW2))
		DDX_Text(pDX, IDC_ROW_LENGTH_NEW2, m_dRowsNew);
	if (pDX->m_bSaveAndValidate || hasFocus != GetDlgItem(IDC_PSRATIO))
		DDX_Text(pDX, IDC_PSRATIO, m_dRatio);
}


BEGIN_MESSAGE_MAP(CPSRatioCalc, CDialog)
	ON_EN_UPDATE(IDC_SAMPLE_LENGTH_NEW, OnEnChangeSamples)
	ON_EN_UPDATE(IDC_MS_LENGTH_NEW, OnEnChangeMs)
	ON_EN_UPDATE(IDC_SPEED, OnEnChangeSpeed)
	ON_EN_UPDATE(IDC_TEMPO, OnEnChangeSpeed)
	ON_EN_UPDATE(IDC_ROW_LENGTH_NEW2, OnEnChangeRows)
	ON_EN_UPDATE(IDC_PSRATIO, OnEnChangeratio)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CPSRatioCalc message handlers
void CPSRatioCalc::OnEnChangeSamples()
{
	UpdateData();
	if (m_lSamplesOrig && m_lSamplesOrig)
	{
		m_dRatio = (double)m_lSamplesNew/(double)m_lSamplesOrig*100;
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
		m_dRatio = (double)m_lMsNew/(double)m_lMsOrig*100;
		CalcSamples();
		CalcRows();
		UpdateData(FALSE);
	}
	
}

void CPSRatioCalc::OnEnChangeRows()
{
	UpdateData();
	if (m_dRowsOrig && m_dRowsNew && m_nTempo && m_nSpeed)
	{
		m_dRatio = m_dRowsNew/m_dRowsOrig*100.0;
		CalcSamples();
		CalcMs();	
		UpdateData(FALSE);
	}

}

void CPSRatioCalc::OnEnChangeSpeed()
{
	UpdateData();
	if (m_nTempo == 0) m_nTempo=1;
	if (m_nSpeed == 0) m_nSpeed=1;
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
	m_lSamplesNew=m_lSamplesOrig*(m_dRatio/100.0);
	return;
}

void CPSRatioCalc::CalcMs()
{
	m_lMsNew=m_lMsOrig*(m_dRatio/100.0);
	return;
}

void CPSRatioCalc::CalcRows()
{
	double rowTime;

	switch(m_nTempoMode) {

		case tempo_mode_alternative: 
			rowTime = 60000.0 / (1.65625 * (double)(m_nSpeed * m_nTempo));
			break;

		case tempo_mode_modern: 
			rowTime = 60000.0/(double)m_nTempo / (double)m_nRowsPerBeat;
			break;

		case tempo_mode_classic: 
		default:
			rowTime = 2500.0 * (double)m_nSpeed/(double)m_nTempo;
			break;
	}
/*
	if (CMainFrame::m_dwPatternSetup & PATTERN_MODERNSPEED) {
		rowTime = 60000.0/(double)m_nTempo / (double)m_nRowsPerBeat;
	} 
	else if (CMainFrame::m_dwPatternSetup & PATTERN_ALTERNTIVEBPMSPEED) {
		rowTime = 60000.0 / (1.65625 * (double)(m_nSpeed * m_nTempo));
	} 
	else {
		rowTime = 2500.0 * (double)m_nSpeed/(double)m_nTempo;
	}
*/	
	m_dRowsOrig = (double)m_lMsOrig/rowTime;
	m_dRowsNew = m_dRowsOrig*(m_dRatio/100);

	return;
}

void CPSRatioCalc::OnBnClickedOk()
{
	if (m_dRatio<50.0 || m_dRatio>200.0)
	{
		::MessageBox(NULL, "Error: ratio must be between 50% and 200%.", 
					       "Error", MB_ICONERROR | MB_OK);
		return;
	}
	OnOK();
}
