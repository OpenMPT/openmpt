/*
 * ScaleEnvPointsDlg.h
 * -------------------
 * Purpose: Dialog for scaling instrument envelope points on x and y axis.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

//=======================================
class CScaleEnvPointsDlg : public CDialog
//=======================================
{
public:

	CScaleEnvPointsDlg(CWnd* pParent, InstrumentEnvelope *pEnv, int nCenter) : CDialog(IDD_SCALE_ENV_POINTS, pParent)
	{
		m_pEnv = pEnv;
		m_nCenter = nCenter;
	}

private:
	InstrumentEnvelope *m_pEnv; //To tell which envelope to process.
	static float m_fFactorX, m_fFactorY;
	int m_nCenter;

protected:
	virtual void OnOK();
	virtual BOOL OnInitDialog();
};
