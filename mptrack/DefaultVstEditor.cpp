#include "stdafx.h"
//#include "vstplug.h"
#include "moddoc.h"
#include "defaultvsteditor.h"
#include ".\defaultvsteditor.h"

CDefaultVstEditor::CDefaultVstEditor(CVstPlugin *pPlugin) : CAbstractVstEditor(pPlugin)
{
	m_nCurrentParam = 0;
	m_nControlLock=0;
}

CDefaultVstEditor::~CDefaultVstEditor(void)
{
}
BEGIN_MESSAGE_MAP(CDefaultVstEditor, CAbstractVstEditor)
	//{{AFX_MSG_MAP(CDefaultVstEditor)
	ON_EN_CHANGE(IDC_PARAMVALUETEXT,OnParamValChangedText)
	ON_LBN_SELCHANGE(IDC_PARAMLIST,	OnParamChanged)
/*
	ON_WM_CLOSE()
	ON_COMMAND(ID_PRESET_LOAD,			OnLoadPreset)
	ON_COMMAND(ID_PRESET_SAVE,			OnSavePreset)
	ON_COMMAND(ID_PRESET_RANDOM,		OnRandomizePreset)
*/

	//}}AFX_MSG_MAP
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

void CDefaultVstEditor::DoDataExchange(CDataExchange* pDX)
//----------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDefaultVstEditor)
	DDX_Control(pDX, IDC_PARAMLIST,			m_lbParameters);
	DDX_Control(pDX, IDC_PARAMLABEL,		m_statParamLabel);
	DDX_Control(pDX, IDC_PARAMVALUESLIDE,	m_slParam);
	DDX_Control(pDX, IDC_PARAMVALUETEXT,	m_editParam);


	//}}AFX_DATA_MAP
}

BOOL CDefaultVstEditor::OpenEditor(CWnd *parent)
//---------------------------------------
{
	Create(IDD_DEFAULTPLUGINEDITOR, parent);
	SetupMenu();

	ShowWindow(SW_SHOW);

	m_slParam.SetRange(0, 100);


	if (m_pVstPlugin)
	{
		char s[128];
		long nParams = m_pVstPlugin->GetNumParameters();
		m_lbParameters.SetRedraw(FALSE);
		m_lbParameters.ResetContent();
		for (long i=0; i<nParams; i++)
		{
			CHAR sname[64];
			m_pVstPlugin->GetParamName(i, sname, sizeof(sname));
			wsprintf(s, "%02X: %s", i|0x80, sname);
			m_lbParameters.SetItemData(m_lbParameters.AddString(s), i);
		}
		m_lbParameters.SetRedraw(TRUE);
		if (m_nCurrentParam >= nParams) m_nCurrentParam = 0;
		m_lbParameters.SetCurSel(m_nCurrentParam);
		UpdateAll();
	} 
	return TRUE;
}


VOID CDefaultVstEditor::OnClose()
//------------------------
{
	DoClose();
}


VOID CDefaultVstEditor::OnOK()
//---------------------
{
	OnClose();
}


VOID CDefaultVstEditor::OnCancel()
//-------------------------
{
	OnClose();
}




VOID CDefaultVstEditor::DoClose()
//------------------------
{
	DestroyWindow();
	if (m_pVstPlugin)
		m_pVstPlugin->Dispatch(effEditClose, 0, 0, NULL, 0);
}


void CDefaultVstEditor::OnParamChanged()
{
	m_nCurrentParam = m_lbParameters.GetItemData(m_lbParameters.GetCurSel());
	UpdateAll();
}

void CDefaultVstEditor::OnParamValChangedText()
{
	char s[64];
	m_editParam.GetWindowText(s, 64);
	int val = atoi(s);
	if (val > 100) 
		val=100;
	m_pVstPlugin->SetParameter(m_nCurrentParam, val/100.0f);
	
	if (!m_nControlLock) {
		UpdateAll();
		m_pVstPlugin->GetModDoc()->SetModified();
	}
}

void CDefaultVstEditor::OnParamValChangedSlide()
{
	char s[64];
	int val = 100-m_slParam.GetPos();
	m_pVstPlugin->SetParameter(m_nCurrentParam, val/100.0f);
	
	wsprintf(s, "%000d", val);
	
	if (!m_nControlLock) {
		UpdateAll();
		m_pVstPlugin->GetModDoc()->SetModified();
	}
}
void CDefaultVstEditor::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if ((nSBCode != SB_ENDSCROLL))
		OnParamValChangedSlide();

	// if ((nCode == SB_ENDSCROLL) || (nCode == SB_THUMBPOSITION))

	CAbstractVstEditor::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CDefaultVstEditor::UpdateAll()
{
	if (m_nControlLock)
		return;

	char s[256];
	int val = static_cast<int>(m_pVstPlugin->GetParameter(m_nCurrentParam)*100.0f+0.5f);
	wsprintf(s, "%000d", val);

	CHAR sunits[64], sdisplay[64], label[128];
	m_pVstPlugin->GetParamLabel(m_nCurrentParam, sunits);
	m_pVstPlugin->GetParamDisplay(m_nCurrentParam, sdisplay);
	wsprintf(label, "%s %s", sdisplay, sunits);

	m_nControlLock++;
	//Don't update when textbox has focus, so user can change content
	if (&m_editParam !=	m_editParam.GetFocus())	
		m_editParam.SetWindowText(s);
	m_statParamLabel.SetWindowText(label);
	m_slParam.SetPos(100-val);
	m_nControlLock--;
	
}

