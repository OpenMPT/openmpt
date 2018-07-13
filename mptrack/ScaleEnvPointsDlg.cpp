/*
 * ScaleEnvPointsDlg.cpp
 * ---------------------
 * Purpose: Dialog for scaling instrument envelope points on x and y axis.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "resource.h"
#include "ModInstrument.h"
#include "ScaleEnvPointsDlg.h"


OPENMPT_NAMESPACE_BEGIN

double CScaleEnvPointsDlg::m_factorX = 1.0;
double CScaleEnvPointsDlg::m_factorY = 1.0;
double CScaleEnvPointsDlg::m_offsetY = 0.0;

BOOL CScaleEnvPointsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_EditX.SubclassDlgItem(IDC_EDIT_FACTORX, this);
	m_EditY.SubclassDlgItem(IDC_EDIT_FACTORY, this);
	m_EditOffset.SubclassDlgItem(IDC_EDIT3, this);
	m_EditX.AllowNegative(false);
	m_EditX.SetDecimalValue(m_factorX);
	m_EditY.SetDecimalValue(m_factorY);
	m_EditOffset.SetDecimalValue(m_offsetY);

	return TRUE;	// return TRUE unless you set the focus to a control
}


void CScaleEnvPointsDlg::OnOK()
{
	m_EditX.GetDecimalValue(m_factorX);
	m_EditY.GetDecimalValue(m_factorY);
	m_EditOffset.GetDecimalValue(m_offsetY);
	CDialog::OnOK();
}


void CScaleEnvPointsDlg::Apply()
{
	if(m_factorX > 0 && m_factorX != 1)
	{
		for(uint32 i = 0; i < m_Env.size(); i++)
		{
			m_Env[i].tick = static_cast<EnvelopeNode::tick_t>(m_factorX * m_Env[i].tick);

			// Checking that the order of points is preserved.
			if(i > 0 && m_Env[i].tick <= m_Env[i - 1].tick)
				m_Env[i].tick = m_Env[i - 1].tick + 1;
		}
	}

	if(m_factorY != 1 || m_offsetY != 0)
	{
		double factor = m_factorY;
		bool invert = false;
		if(m_factorY < 0)
		{
			invert = true;
			factor = -factor;
		}
		for(auto &pt : m_Env)
		{
			if(invert) pt.value = ENVELOPE_MAX - pt.value;
			pt.value = mpt::saturate_round<EnvelopeNode::value_t>(Clamp((factor * (pt.value - m_nCenter)) + m_nCenter + m_offsetY, double(ENVELOPE_MIN), double(ENVELOPE_MAX)));
		}
	}
}


OPENMPT_NAMESPACE_END
