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
	ON_NOTIFY(NM_DBLCLK,	IDC_LIST1,	OnOptionDblClick)
	ON_EN_CHANGE(IDC_EDIT1,				OnFindStringChanged)
END_MESSAGE_MAP()

void COptionsAdvanced::DoDataExchange(CDataExchange* pDX)
//-------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModTypeDlg)
	DDX_Control(pDX, IDC_LIST1,			m_List);
	//}}AFX_DATA_MAP
}


BOOL COptionsAdvanced::PreTranslateMessage(MSG *msg)
//--------------------------------------------------
{
	if(msg->message == WM_KEYDOWN && msg->wParam == VK_RETURN)
	{
		OnOptionDblClick(nullptr, nullptr);
		return TRUE;
	}
	return FALSE;
}


BOOL COptionsAdvanced::OnInitDialog()
//-----------------------------------
{
	CPropertyPage::OnInitDialog();

	const CListCtrlEx::Header headers[] =
	{
		{ _T("Setting"),	100, LVCFMT_LEFT },
		{ _T("Type"),		40,  LVCFMT_LEFT },
		{ _T("Value"),		190, LVCFMT_LEFT },
		{ _T("Default"),	62,  LVCFMT_LEFT },
	};
	m_List.SetHeaders(headers);
	m_List.SetExtendedStyle(m_List.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	ReInit();
	return TRUE;
}


void COptionsAdvanced::ReInit()
//-----------------------------
{
	m_List.SetRedraw(FALSE);
	m_List.DeleteAllItems();
	bool useGroups = ((int)ListView_EnableGroupView(m_List.m_hWnd, TRUE)) != -1;
	m_List.SetItemCount(theApp.GetSettings().size());

	m_IndexToPath.clear();
	m_IndexToPath.reserve(theApp.GetSettings().size());
	m_Groups.clear();
	int numGroups = 0;
	CString findStr;
	GetDlgItemText(IDC_EDIT1, findStr);
	findStr.MakeLower();
	int i = 0;
	for(SettingsContainer::SettingsMap::const_iterator it = theApp.GetSettings().begin(); it != theApp.GetSettings().end(); ++it)
	{
		const SettingPath &path = it->first;
		const SettingValue &value = it->second;

		int groupID = 0;
		const mpt::ustring &section = path.GetSection();
		UNORDERED_MAP<mpt::ustring, int>::const_iterator gi = m_Groups.find(section);
		if(gi == m_Groups.end() && useGroups)
		{
			LVGROUP group;
#if _WIN32_WINNT >= 0x0600
			group.cbSize = LVGROUP_V5_SIZE;
#else
			group.cbSize = sizeof(group);
#endif
			group.mask = LVGF_HEADER | LVGF_GROUPID;
			group.pszHeader = const_cast<wchar_t *>(section.c_str());
			group.cchHeader = 0;
			group.pszFooter = nullptr;
			group.cchFooter = 0;
			group.iGroupId = groupID = numGroups++;
			group.stateMask = LVGS_COLLAPSIBLE;
			group.state = LVGS_COLLAPSIBLE;
			group.uAlign = LVGA_HEADER_LEFT;
			ListView_InsertGroup(m_List.m_hWnd, -1, &group);
			m_Groups.insert(std::make_pair(section, groupID));
		} else
		{
			groupID = gi->second;
		}

		bool addString = true;
		if(!findStr.IsEmpty())
		{
			CString str = mpt::ToCString(path.FormatAsString() + MPT_USTRING(" = ") + value.FormatValueAsString());
			addString = str.MakeLower().Find(findStr) >= 0;
		}
		if(addString)
		{
			const mpt::ustring str = useGroups ? it->first.GetKey() : it->first.FormatAsString();
			LVITEMW lvi;
			lvi.mask = LVIF_TEXT | LVIF_GROUPID | LVIF_PARAM;
			lvi.iItem = i++;
			lvi.iSubItem = 0;
			lvi.state = 0;
			lvi.stateMask = 0;
			lvi.pszText = const_cast<wchar_t *>(str.c_str());
			lvi.cchTextMax = 0;
			lvi.iImage = 0;
			lvi.lParam = m_IndexToPath.size();
			lvi.iIndent = 0;
			lvi.iGroupId = groupID;

			int index = m_List.SendMessage(LVM_INSERTITEMW, 0, (LPARAM)(&lvi));
			m_List.SetItemText(index, 1, value.FormatTypeAsString().c_str());
			m_List.SetItemText(index, 2, value.FormatValueAsString().c_str());
			m_List.SetItemText(index, 3, it->second.GetDefault().FormatValueAsString().c_str());
			m_IndexToPath.push_back(it->first);
		}
	}

	m_List.SetItemCount(i);
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


void COptionsAdvanced::OnOptionDblClick(NMHDR *, LRESULT *)
//---------------------------------------------------------
{
	const int index = m_List.GetSelectionMark();
	const SettingPath path = m_IndexToPath[m_List.GetItemData(index)];
	SettingValue val = theApp.GetSettings().GetMap().find(path)->second;
	if(val.GetType() == SettingTypeBool)
	{
		val = !val.as<bool>();
	} else
	{
		CInputDlg inputDlg(this, _T("Enter new value for ") + mpt::ToCString(path.FormatAsString()), mpt::ToCString(val.FormatValueAsString()));
		if(inputDlg.DoModal() != IDOK)
		{
			return;
		}
		val.SetFromString(inputDlg.resultString);
	}
	theApp.GetSettings().Write(path, val);
	m_List.SetItemText(index, 2, val.FormatValueAsString().c_str());
	m_List.SetSelectionMark(index);
	OnSettingsChanged();
}

OPENMPT_NAMESPACE_END

#endif // MPT_SETTINGS_CACHE
