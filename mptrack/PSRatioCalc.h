#pragma once
#include "afxwin.h"
// CPSRatioCalc dialog

class CPSRatioCalc : public CDialog
{
	DECLARE_DYNAMIC(CPSRatioCalc)

public:
	enum { IDD = IDD_PITCHSHIFT };
	CPSRatioCalc(const CSoundFile &sndFile, SAMPLEINDEX sample, double ratio, CWnd* pParent = NULL);   // standard constructor
	virtual ~CPSRatioCalc();
	double m_dRatio;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	const CSoundFile &sndFile;
	SAMPLEINDEX sampleIndex;

	ULONGLONG m_lSamplesNew;
	ULONGLONG m_lMsNew, m_lMsOrig;
	double m_dRowsOrig, m_dRowsNew;
	UINT m_nTempo, m_nSpeed;

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
