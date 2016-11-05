/*
 * ScaleEnvPointsDlg.h
 * -------------------
 * Purpose: Dialog for scaling instrument envelope points on x and y axis.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "CDecimalSupport.h"

OPENMPT_NAMESPACE_BEGIN

struct InstrumentEnvelope;

//=======================================
class CScaleEnvPointsDlg : public CDialog
//=======================================
{
protected:
	CNumberEdit m_EditX, m_EditY;
	InstrumentEnvelope &m_Env; //To tell which envelope to process.
	static double m_fFactorX, m_fFactorY;
	int m_nCenter;

public:
	CScaleEnvPointsDlg(CWnd* pParent, InstrumentEnvelope &env, int nCenter)
		: CDialog(IDD_SCALE_ENV_POINTS, pParent)
		, m_Env(env)
		, m_nCenter(nCenter)
	{ }

	void Apply();

protected:
	virtual void OnOK();
	virtual BOOL OnInitDialog();
};

OPENMPT_NAMESPACE_END
