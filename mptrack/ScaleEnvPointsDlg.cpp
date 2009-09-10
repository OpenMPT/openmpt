// ScaleEnvPointsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mptrack.h"
#include "ScaleEnvPointsDlg.h"


// CScaleEnvPointsDlg dialog

IMPLEMENT_DYNAMIC(CScaleEnvPointsDlg, CDialog)
CScaleEnvPointsDlg::CScaleEnvPointsDlg(CWnd* pParent, MODINSTRUMENT* pInst, UINT env)
	: CDialog(CScaleEnvPointsDlg::IDD, pParent),
	  m_pInstrument(pInst),
	  m_Env(env)
//----------------------------------------------------------------------
{
}

CScaleEnvPointsDlg::~CScaleEnvPointsDlg()
//---------------------------------------
{
}

void CScaleEnvPointsDlg::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_FACTOR, m_EditFactor);
}


BEGIN_MESSAGE_MAP(CScaleEnvPointsDlg, CDialog)

END_MESSAGE_MAP()


// CScaleEnvPointsDlg message handlers

void CScaleEnvPointsDlg::OnOK()
//------------------------------
{
	char buffer[10];
	GetDlgItemText(IDC_EDIT_FACTOR, buffer, 9);
	float factor = ConvertStrTo<float>(buffer);
	if(factor > 0)
	{
		INSTRUMENTENVELOPE *pEnv = nullptr;
		switch(m_Env)
		{
			case ENV_PANNING:	pEnv = &m_pInstrument->PanEnv;		break;
			case ENV_PITCH:		pEnv = &m_pInstrument->PitchEnv;	break;
			default:			pEnv = &m_pInstrument->VolEnv;		break;
		}
		for(UINT i = 0; i< pEnv->nNodes; i++)
		{
			pEnv->Ticks[i] = static_cast<WORD>(factor * pEnv->Ticks[i]);

			//Checking that the order of points is preserved.
			if(i > 0 && pEnv->Ticks[i] <= pEnv->Ticks[i - 1])
				pEnv->Ticks[i] = pEnv->Ticks[i - 1] + 1;
		}
	}

	CDialog::OnOK();
}

BOOL CScaleEnvPointsDlg::OnInitDialog()
//-------------------------------------
{
	CDialog::OnInitDialog();

    SetDlgItemText(IDC_EDIT_FACTOR, "");
	m_EditFactor.SetFocus();

	
	return FALSE;  // return TRUE unless you set the focus to a control
}
