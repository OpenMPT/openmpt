#pragma once
#include "afxwin.h"
// CPSRatioCalc dialog

class CPSRatioCalc : public CDialog
{
	DECLARE_DYNAMIC(CPSRatioCalc)

public:
	enum { IDD = IDD_PITCHSHIFT };
	CPSRatioCalc(ULONGLONG samples, ULONGLONG sampleRate, UINT speed, UINT tempo, UINT rowsPerBeat, BYTE tempoMode, double ratio,  CWnd* pParent = NULL);   // standard constructor
	virtual ~CPSRatioCalc();
	double m_dRatio;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	ULONGLONG m_lSamplesNew, m_lSamplesOrig;
	ULONGLONG m_lMsNew, m_lMsOrig;
	UINT m_nTempo, m_nSpeed, m_nRowsPerBeat;
	BYTE m_nTempoMode;
	double m_dRowsOrig, m_dRowsNew; 
	
	afx_msg void OnEnChangeSamples();
	afx_msg void OnEnChangeMs();
	afx_msg void OnEnChangeSpeed();
	afx_msg void OnEnChangeRows();
	afx_msg void OnEnChangeratio();

	void CalcSamples();
	void CalcMs();
	void CalcRows();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};
