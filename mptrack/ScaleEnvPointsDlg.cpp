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

	if(m_fFactorX > 0 && m_fFactorX != 1)
	{
		for(uint32 i = 0; i < m_Env.nNodes; i++)
		{
			m_Env.Ticks[i] = static_cast<uint16>(m_fFactorX * m_Env.Ticks[i]);

			// Checking that the order of points is preserved.
			if(i > 0 && m_Env.Ticks[i] <= m_Env.Ticks[i - 1])
				m_Env.Ticks[i] = m_Env.Ticks[i - 1] + 1;
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
		for(uint32 i = 0; i < m_Env.nNodes; i++)
		{
			if(invert) m_Env.Values[i] = ENVELOPE_MAX - m_Env.Values[i];
			m_Env.Values[i] = Clamp(static_cast<uint8>((factor * ((int)m_Env.Values[i] - m_nCenter)) + m_nCenter), uint8(ENVELOPE_MIN), uint8(ENVELOPE_MAX));
		}
	}

	CDialog::OnOK();
}


OPENMPT_NAMESPACE_END
