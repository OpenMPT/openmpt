/*
 * ScaleEnvPointsDlg.h
 * -------------------
 * Purpose: Dialog for scaling instrument envelope points on x and y axis.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "CDecimalSupport.h"

OPENMPT_NAMESPACE_BEGIN

struct InstrumentEnvelope;

class CScaleEnvPointsDlg : public CDialog
{
protected:
	CNumberEdit m_EditX, m_EditY, m_EditOffset;
	InstrumentEnvelope &m_Env;
	static double m_factorX, m_factorY, m_offsetY;
	int m_nCenter;

public:
	CScaleEnvPointsDlg(CWnd* pParent, InstrumentEnvelope &env, int nCenter)
		: CDialog(IDD_SCALE_ENV_POINTS, pParent)
		, m_Env(env)
		, m_nCenter(nCenter)
	{ }

	void Apply();

protected:
	void OnOK() override;
	BOOL OnInitDialog() override;
};

OPENMPT_NAMESPACE_END
