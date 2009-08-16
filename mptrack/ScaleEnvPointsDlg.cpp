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
		WORD (*array)[MAX_ENVPOINTS] = NULL;
		UINT* arraySize = NULL;
		switch(m_Env)
		{
			case ENV_VOLUME:
				array = &m_pInstrument->VolPoints;
				arraySize = &m_pInstrument->nVolEnv;
			break;

			case ENV_PANNING:
				array = &m_pInstrument->PanPoints;
				arraySize = &m_pInstrument->nPanEnv;
			break;

			case ENV_PITCH:
				array = &m_pInstrument->PitchPoints;
				arraySize = &m_pInstrument->nPitchEnv;
			break;
		}

		if(array && arraySize)
		{
			for(UINT i = 0; i<*arraySize; i++)
			{
				(*array)[i] = static_cast<WORD>(factor * (*array)[i]);

				//Checking that the order of points is preserved.
				if(i > 0 && (*array)[i] <= (*array)[i-1])
					(*array)[i] = (*array)[i-1]+1;
			}
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
