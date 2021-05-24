/*
 * PSRatioCalc.h
 * -------------
 * Purpose: Dialog for calculating sample pitch shift ratios in the sample editor.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "CDecimalSupport.h"

OPENMPT_NAMESPACE_BEGIN

class CPSRatioCalc : public CDialog
{
	DECLARE_DYNAMIC(CPSRatioCalc)

public:
	CPSRatioCalc(const CSoundFile &sndFile, SAMPLEINDEX sample, double ratio, CWnd* pParent = NULL);   // standard constructor
	double m_dRatio;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	CNumberEdit m_EditTempo;
	const CSoundFile &sndFile;
	SAMPLEINDEX sampleIndex;

	ULONGLONG m_lSamplesNew;
	ULONGLONG m_lMsNew, m_lMsOrig;
	double m_dRowsOrig, m_dRowsNew;
	uint32 m_nSpeed;
	TEMPO m_nTempo;

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

OPENMPT_NAMESPACE_END
