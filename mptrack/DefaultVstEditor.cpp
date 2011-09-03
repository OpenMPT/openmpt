#include "stdafx.h"
//#include "vstplug.h"
#include "moddoc.h"
#include "defaultvsteditor.h"
#include ".\defaultvsteditor.h"

#ifndef NO_VST

CDefaultVstEditor::CDefaultVstEditor(CVstPlugin *pPlugin) : CAbstractVstEditor(pPlugin)
//-------------------------------------------------------------------------------------
{
	m_nCurrentParam = 0;
	m_nControlLock=0;
}

CDefaultVstEditor::~CDefaultVstEditor(void)
//-----------------------------------------
{
}

BEGIN_MESSAGE_MAP(CDefaultVstEditor, CAbstractVstEditor)
	//{{AFX_MSG_MAP(CDefaultVstEditor)
	ON_EN_CHANGE(IDC_PARAMVALUETEXT,OnParamTextboxChanged)
	ON_LBN_SELCHANGE(IDC_PARAMLIST,	OnParamChanged)
	//}}AFX_MSG_MAP
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

void CDefaultVstEditor::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDefaultVstEditor)
	DDX_Control(pDX, IDC_PARAMLIST,			m_lbParameters);
	DDX_Control(pDX, IDC_PARAMLABEL,		m_statParamLabel);
	DDX_Control(pDX, IDC_PARAMVALUESLIDE,	m_slParam);
	DDX_Control(pDX, IDC_PARAMVALUETEXT,	m_editParam);
	//}}AFX_DATA_MAP
}

void CDefaultVstEditor::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//--------------------------------------------------------------------------------
{
	CSliderCtrl* pScrolledSlider = reinterpret_cast<CSliderCtrl*>(pScrollBar);
	if ((pScrolledSlider == &m_slParam) && (nSBCode != SB_ENDSCROLL))
	{
		OnParamSliderChanged();
	}
	CAbstractVstEditor::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CDefaultVstEditor::OpenEditor(CWnd *parent)
//----------------------------------------------
{
	Create(IDD_DEFAULTPLUGINEDITOR, parent);
	SetTitle();
	SetupMenu();

	m_slParam.SetRange(0, PARAM_RESOLUTION);
	ShowWindow(SW_SHOW);
	
	// Fill param listbox
	if (m_pVstPlugin)
	{
		char s[128];
		long nParams = m_pVstPlugin->GetNumParameters();
		m_lbParameters.SetRedraw(FALSE);	//disable lisbox refreshes during fill to avoid flicker
		m_lbParameters.ResetContent();
		for (long i=0; i<nParams; i++)
		{
			CHAR sname[64];
			m_pVstPlugin->GetParamName(i, sname, sizeof(sname));
			wsprintf(s, "%02X: %s", i|0x80, sname);
			m_lbParameters.SetItemData(m_lbParameters.AddString(s), i);
		}
		m_lbParameters.SetRedraw(TRUE);		//re-enable lisbox refreshes
		if (m_nCurrentParam >= nParams) m_nCurrentParam = 0;
		m_lbParameters.SetCurSel(m_nCurrentParam);
		UpdateParamDisplays();
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
	if (m_pVstPlugin) {
		m_pVstPlugin->Dispatch(effEditClose, 0, 0, NULL, 0);
	}
}

void CDefaultVstEditor::OnParamChanged()
//--------------------------------------
{
	m_nCurrentParam = m_lbParameters.GetItemData(m_lbParameters.GetCurSel());
	UpdateParamDisplays();
}

// Called when a change occurs to the parameter textbox
// If the change is triggered by the user, we'll need to notify the plugin and update 
// the other GUI controls 
void CDefaultVstEditor::OnParamTextboxChanged()
//---------------------------------------------
{
	if (m_nControlLock) {	// Lock will be set if the GUI change was triggered internally (in UpdateParamDisplays).
		return;				// We're only interested in handling changes triggered by the user.
	}

	//Extract value and notify plug
	char s[64];
	m_editParam.GetWindowText(s, 64);
	int val = atoi(s);
	if (val > PARAM_RESOLUTION) 
		val=PARAM_RESOLUTION;
	m_pVstPlugin->SetParameter(m_nCurrentParam, val/static_cast<float>(PARAM_RESOLUTION));
	
	UpdateParamDisplays();	// update other GUI controls
	m_pVstPlugin->GetModDoc()->SetModified();
}

// Called when a change occurs to the parameter slider
// If the change is triggered by the user, we'll need to notify the plugin and update 
// the other GUI controls 
void CDefaultVstEditor::OnParamSliderChanged()
//----------------------------------------------
{
	if (m_nControlLock) {	// Lock will be set if the GUI change was triggered internally (in UpdateParamDisplays).
		return;				// We're only interested in handling changes triggered by the user.
	}

	//Extract value and notify plug
	int val = PARAM_RESOLUTION-m_slParam.GetPos();
	m_pVstPlugin->SetParameter(m_nCurrentParam, val/static_cast<float>(PARAM_RESOLUTION));
	
	UpdateParamDisplays();	// update other GUI controls
	m_pVstPlugin->GetModDoc()->SetModified();

}


//Update all GUI controls with the new param value
void CDefaultVstEditor::UpdateParamDisplays()
//-------------------------------------------
{
	if (m_nControlLock) {	//Just to make sure we're not here as a consequence of an internal GUI change.
		return;
	}

	//Get the param value fromt the plug and massage it into the formats we need
	char s[256];
	int val = static_cast<int>(m_pVstPlugin->GetParameter(m_nCurrentParam)*static_cast<float>(PARAM_RESOLUTION)+0.5f);
	wsprintf(s, "%0000d", val);

	CHAR sunits[64], sdisplay[64], label[128];
	m_pVstPlugin->GetParamLabel(m_nCurrentParam, sunits);
	m_pVstPlugin->GetParamDisplay(m_nCurrentParam, sdisplay);
	wsprintf(label, "%s %s", sdisplay, sunits);

	//Update the GUI controls
	m_nControlLock++;	// Set lock to indicate that the changes to the GUI are internal - no need to notify the plug and re-update GUI.
	m_statParamLabel.SetWindowText(label);
	m_slParam.SetPos(PARAM_RESOLUTION-val);
	if (&m_editParam !=	m_editParam.GetFocus()) {	//Don't update textbox when it has focus, else this will prevent user from changing the content
		m_editParam.SetWindowText(s);
	}
	m_nControlLock--;	// Unset lock - done with internal GUI updates.
	
}

#endif // NO_VST

