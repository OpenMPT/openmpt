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


double CScaleEnvPointsDlg::m_fFactorX = 1.0;
double CScaleEnvPointsDlg::m_fFactorY = 1.0;


BOOL CScaleEnvPointsDlg::OnInitDialog()
//-------------------------------------
{
	CDialog::OnInitDialog();
	m_EditX.SubclassDlgItem(IDC_EDIT_FACTORX, this);
	m_EditY.SubclassDlgItem(IDC_EDIT_FACTORY, this);
	m_EditX.AllowNegative(false);
	m_EditX.SetDecimalValue(m_fFactorX);
	m_EditY.SetDecimalValue(m_fFactorY);

	return TRUE;	// return TRUE unless you set the focus to a control
}


void CScaleEnvPointsDlg::OnOK()
//-----------------------------
{
	m_EditX.GetDecimalValue(m_fFactorX);
	m_EditY.GetDecimalValue(m_fFactorY);
	CDialog::OnOK();
}


void CScaleEnvPointsDlg::Apply()
//------------------------------
{
	if(m_fFactorX > 0 && m_fFactorX != 1)
	{
		for(uint32 i = 0; i < m_Env.size(); i++)
		{
			m_Env[i].tick = static_cast<EnvelopeNode::tick_t>(m_fFactorX * m_Env[i].tick);

			// Checking that the order of points is preserved.
			if(i > 0 && m_Env[i].tick <= m_Env[i - 1].tick)
				m_Env[i].tick = m_Env[i - 1].tick + 1;
		}
	}

	if(m_fFactorY != 1)
	{
		double factor = m_fFactorY;
		bool invert = false;
		if(m_fFactorY < 0)
		{
			invert = true;
			factor = -factor;
		}
		for(uint32 i = 0; i < m_Env.size(); i++)
		{
			if(invert) m_Env[i].value = ENVELOPE_MAX - m_Env[i].value;
			m_Env[i].value = Clamp(static_cast<EnvelopeNode::value_t>((factor * ((int)m_Env[i].value - m_nCenter)) + m_nCenter), EnvelopeNode::value_t(ENVELOPE_MIN), EnvelopeNode::value_t(ENVELOPE_MAX));
		}
	}
}


OPENMPT_NAMESPACE_END
