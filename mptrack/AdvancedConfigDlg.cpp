/*
 * AdvancedConfigDlg.cpp
 * ---------------------
 * Purpose: Implementation of the advanced settings dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mainfrm.h"
#include "AdvancedConfigDlg.h"
#include "Settings.h"
#include "dlg_misc.h"

#if defined(MPT_SETTINGS_CACHE)

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(COptionsAdvanced, CPropertyPage)
	ON_LBN_DBLCLK(IDC_LIST4,	OnOptionDblClick)
	ON_EN_CHANGE(IDC_EDIT1,		OnFindStringChanged)
END_MESSAGE_MAP()

void COptionsAdvanced::DoDataExchange(CDataExchange* pDX)
//-------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModTypeDlg)
	DDX_Control(pDX, IDC_LIST4,			m_List);
	//}}AFX_DATA_MAP
}


static CString FormatSetting(const SettingPath &path, const SettingValue &val)
//----------------------------------------------------------------------------
{
	return mpt::ToCString(path.FormatAsString() + MPT_USTRING(" = ") + val.FormatAsString());
}


BOOL COptionsAdvanced::PreTranslateMessage(MSG *msg)
//--------------------------------------------------
{
	if(msg->message == WM_KEYDOWN && msg->wParam == VK_RETURN)
	{
		OnOptionDblClick();
		return TRUE;
	}
	return FALSE;
}


BOOL COptionsAdvanced::OnInitDialog()
//-----------------------------------
{
	CPropertyPage::OnInitDialog();
	ReInit();
	return TRUE;
}


void COptionsAdvanced::ReInit()
//-----------------------------
{
	m_List.SetRedraw(FALSE);
	m_List.ResetContent();
	m_IndexToPath.clear();
	CString findStr;
	GetDlgItemText(IDC_EDIT1, findStr);
	findStr.MakeLower();
	for(SettingsContainer::SettingsMap::const_iterator it = theApp.GetSettings().begin(); it != theApp.GetSettings().end(); ++it)
	{
		CString str = FormatSetting(it->first, it->second);
		bool addString = true;
		if(!findStr.IsEmpty())
		{
			CString strLower = str;
			addString = strLower.MakeLower().Find(findStr) >= 0;
		}
		if(addString)
		{
			int index = m_List.AddString(str);
			m_IndexToPath[index] = it->first;
		}
	}
	m_List.SetRedraw(TRUE);
	m_List.Invalidate(FALSE);
}


void COptionsAdvanced::OnOK()
//---------------------------
{
	CPropertyPage::OnOK();
}


BOOL COptionsAdvanced::OnSetActive()
//----------------------------------
{
	ReInit();
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_ADVANCED;
	return CPropertyPage::OnSetActive();
}


void COptionsAdvanced::OnOptionDblClick()
//---------------------------------------
{
	const int index = m_List.GetCurSel();
	if(m_IndexToPath.find(index) == m_IndexToPath.end())
	{
		return;
	}
	const SettingPath path = m_IndexToPath[index];
	SettingValue val = theApp.GetSettings().GetMap().find(path)->second;
	if(val.GetType() == SettingTypeBool)
	{
		val = !val.as<bool>();
	} else
	{
		CInputDlg inputDlg(this, mpt::ToCString(path.FormatAsString()), mpt::ToCString(val.FormatValueAsString()));
		if(inputDlg.DoModal() != IDOK)
		{
			return;
		}
		val.SetFromString(inputDlg.resultString);
	}
	theApp.GetSettings().Write(path, val);
	m_List.DeleteString(index);
	m_List.InsertString(index, FormatSetting(path, val));
	m_List.SetCurSel(index);
	OnSettingsChanged();
}

OPENMPT_NAMESPACE_END

#endif // MPT_SETTINGS_CACHE
