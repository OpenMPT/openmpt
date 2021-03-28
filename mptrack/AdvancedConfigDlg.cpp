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

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(COptionsAdvanced, CPropertyPage)
	ON_NOTIFY(NM_DBLCLK,	IDC_LIST1,	&COptionsAdvanced::OnOptionDblClick)
#ifndef MPT_MFC_FULL
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST1, &COptionsAdvanced::OnCustomDrawList)
#endif
	ON_EN_CHANGE(IDC_EDIT1,				&COptionsAdvanced::OnFindStringChanged)
	ON_COMMAND(IDC_BUTTON1, &COptionsAdvanced::OnSaveNow)
END_MESSAGE_MAP()

void COptionsAdvanced::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModTypeDlg)
	DDX_Control(pDX, IDC_LIST1,			m_List);
	//}}AFX_DATA_MAP
}


BOOL COptionsAdvanced::PreTranslateMessage(MSG *msg)
{
	if(msg->message == WM_KEYDOWN && msg->wParam == VK_RETURN)
	{
		OnOptionDblClick(nullptr, nullptr);
		return TRUE;
	}
	return FALSE;
}


BOOL COptionsAdvanced::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_List.SetExtendedStyle(m_List.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	ListView_EnableGroupView(m_List.m_hWnd, FALSE); // try to set known state
	int enableGroupsResult1 = static_cast<int>(ListView_EnableGroupView(m_List.m_hWnd, TRUE));
	int enableGroupsResult2 = static_cast<int>(ListView_EnableGroupView(m_List.m_hWnd, TRUE));
	// Looks like we have to check enabling and check that a second enabling does
	// not change anything.
	// Just checking if enabling fails with -1 does not work for older control
	// versions because they just do not know the window message at all and return
	// 0, always. At least Wine does behave this way.
	if(enableGroupsResult1 == 1 && enableGroupsResult2 == 0)
	{
		m_listGrouped = true;
	} else
	{
		// Did not behave as documented or expected, the actual state of the
		// control is unknown by now.
		// Play safe and set and assume the traditional ungrouped mode again.
		ListView_EnableGroupView(m_List.m_hWnd, FALSE);
		m_listGrouped = false;
	}

	if(m_listGrouped)
	{
		static constexpr ListCtrl::Header headers[] =
		{
			{ _T("Setting"), 150, LVCFMT_LEFT },
			{ _T("Type"),    40,  LVCFMT_LEFT },
			{ _T("Value"),   140, LVCFMT_LEFT },
			{ _T("Default"), 62,  LVCFMT_LEFT },
		};
		m_List.SetHeaders(headers);
	} else
	{
		static constexpr ListCtrl::Header headers[] =
		{
			{ _T("Setting"), 200, LVCFMT_LEFT },
			{ _T("Type"),    40,  LVCFMT_LEFT },
			{ _T("Value"),   100, LVCFMT_LEFT },
			{ _T("Default"), 52,  LVCFMT_LEFT },
		};
		m_List.SetHeaders(headers);
	}

	ReInit();
	return TRUE;
}


void COptionsAdvanced::ReInit()
{
	m_List.SetRedraw(FALSE);
	m_List.DeleteAllItems();
	if(m_listGrouped)
	{
		ListView_RemoveAllGroups(m_List.m_hWnd);
	}
	m_List.SetItemCount(static_cast<int>(theApp.GetSettings().size()));

	m_indexToPath.clear();
	m_indexToPath.reserve(theApp.GetSettings().size());
	m_groups.clear();
	int numGroups = 0;

	mpt::ustring findStr = mpt::ToLowerCase(GetWindowTextUnicode(*GetDlgItem(IDC_EDIT1)));

	LVITEMW lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.mask |= (m_listGrouped ? LVIF_GROUPID : 0);
	lvi.iSubItem = 0;
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.cchTextMax = 0;
	lvi.iImage = 0;
	lvi.iIndent = 0;
	lvi.iGroupId = 0;

	int i = 0;
	for(const auto &[path, state] : theApp.GetSettings())
	{
		// In MPT_USTRING_MODE_WIDE mode,
		// this loop is heavily optimized to avoid as much string copies as possible
		// in order to perform ok-ish in debug builds.
		// MPT_USTRING_MODE_UTF8 is not optimized as we (currently) do not build in
		// this mode by default.
		const mpt::ustring &section = path.GetRefSection();
		const mpt::ustring &key = path.GetRefKey();
		const SettingValue &value = state.GetRefValue();
		const SettingValue &defaultValue = state.GetRefDefault();

		if(!findStr.empty())
		{
			mpt::ustring str = path.FormatAsString() + U_("=") + value.FormatValueAsString();
			str = mpt::ToLowerCase(str);
			if(str.find(findStr) == mpt::ustring::npos)
			{
				continue;
			}
		}

		int index;
		lvi.iItem = i++;
		lvi.lParam = m_indexToPath.size();

		if(m_listGrouped)
		{

			auto gi = m_groups.find(section);
			if(gi == m_groups.end())
			{
				LVGROUP group;
	#if _WIN32_WINNT >= 0x0600
				group.cbSize = LVGROUP_V5_SIZE;
	#else
				group.cbSize = sizeof(group);
	#endif
				group.mask = LVGF_HEADER | LVGF_GROUPID;
#if MPT_USTRING_MODE_WIDE
				group.pszHeader = const_cast<wchar_t *>(section.c_str());
#else
				const std::wstring wsection = mpt::ToWide(section);
				group.pszHeader = const_cast<wchar_t *>(wsection.c_str());
#endif
				group.cchHeader = 0;
				group.pszFooter = nullptr;
				group.cchFooter = 0;
				group.iGroupId = lvi.iGroupId = numGroups++;
				group.stateMask = LVGS_COLLAPSIBLE;
				group.state = LVGS_COLLAPSIBLE;
				group.uAlign = LVGA_HEADER_LEFT;
				ListView_InsertGroup(m_List.m_hWnd, -1, &group);
				m_groups.insert(std::make_pair(section, lvi.iGroupId));
			} else
			{
				lvi.iGroupId = gi->second;
			}

#if MPT_USTRING_MODE_WIDE
			lvi.pszText = const_cast<wchar_t *>(key.c_str());
#else
			const std::wstring wkey = mpt::ToWide(key);
			lvi.pszText = const_cast<wchar_t *>(wkey.c_str());
#endif
			index = static_cast<int>(m_List.SendMessage(LVM_INSERTITEMW, 0, (LPARAM)(&lvi)));

		} else
		{

			const mpt::ustring sectionAndKey = path.FormatAsString();
#if MPT_USTRING_MODE_WIDE
			lvi.pszText = const_cast<wchar_t *>(sectionAndKey.c_str());
#else
			const std::wstring wsectionAndKey = mpt::ToWide(sectionAndKey);
			lvi.pszText = const_cast<wchar_t *>(wsectionAndKey.c_str());
#endif
			index = static_cast<int>(m_List.SendMessage(LVM_INSERTITEMW, 0, (LPARAM)(&lvi)));

		}

#if MPT_USTRING_MODE_WIDE
		m_List.SetItemText(index, 1, value.FormatTypeAsString().c_str());
		m_List.SetItemText(index, 2, value.FormatValueAsString().c_str());
		m_List.SetItemText(index, 3, defaultValue.FormatValueAsString().c_str());
#else
		m_List.SetItemText(index, 1, mpt::ToCString(value.FormatTypeAsString()));
		m_List.SetItemText(index, 2, mpt::ToCString(value.FormatValueAsString()));
		m_List.SetItemText(index, 3, mpt::ToCString(defaultValue.FormatValueAsString()));
#endif
		m_indexToPath.push_back(path);
	}

	m_List.SetItemCount(i);
	m_List.SetRedraw(TRUE);
	m_List.Invalidate(FALSE);
}


void COptionsAdvanced::OnOK()
{
	CSoundFile::SetDefaultNoteNames();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->PostMessage(WM_MOD_INVALIDATEPATTERNS, HINT_MPTOPTIONS);
	CPropertyPage::OnOK();
}


BOOL COptionsAdvanced::OnSetActive()
{
	ReInit();
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_ADVANCED;
	return CPropertyPage::OnSetActive();
}


#ifdef MPT_MFC_FULL


COLORREF CAdvancedSettingsList::OnGetCellBkColor(int nRow, int /* nColumn */ )
{
	const bool isDefault = theApp.GetSettings().GetMap().find(m_indexToPath[GetItemData(nRow)])->second.IsDefault();
	COLORREF defColor = GetBkColor();
	COLORREF txtColor = GetTextColor();
	COLORREF modColor = RGB(GetRValue(defColor) * 0.9 + GetRValue(txtColor) * 0.1, GetGValue(defColor) * 0.9 + GetGValue(txtColor) * 0.1, GetBValue(defColor) * 0.9 + GetBValue(txtColor) * 0.1);
	return isDefault ? defColor : modColor;
}


COLORREF CAdvancedSettingsList::OnGetCellTextColor(int nRow, int nColumn)
{
	return CMFCListCtrlEx::OnGetCellTextColor(nRow, nColumn);
}


#else // !MPT_MFC_FULL


void COptionsAdvanced::OnCustomDrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW *pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
	*pResult = CDRF_DODEFAULT;
	switch(pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
		{
			const bool isDefault = theApp.GetSettings().GetMap().find(m_indexToPath[pLVCD->nmcd.lItemlParam])->second.IsDefault();
			COLORREF defColor = m_List.GetBkColor();
			COLORREF txtColor = m_List.GetTextColor();
			COLORREF modColor = RGB(GetRValue(defColor) * 0.9 + GetRValue(txtColor) * 0.1, GetGValue(defColor) * 0.9 + GetGValue(txtColor) * 0.1, GetBValue(defColor) * 0.9 + GetBValue(txtColor) * 0.1);
			pLVCD->clrTextBk = isDefault ? defColor : modColor;
		}
		break;
	}	
}


#endif // MPT_MFC_FULL


void COptionsAdvanced::OnOptionDblClick(NMHDR *, LRESULT *)
{
	const int index = m_List.GetSelectionMark();
	if(index < 0)
		return;
	const SettingPath path = m_indexToPath[m_List.GetItemData(index)];
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
		val.SetFromString(inputDlg.resultAsString);
	}
	theApp.GetSettings().Write(path, val);
#if MPT_USTRING_MODE_WIDE
	m_List.SetItemText(index, 2, val.FormatValueAsString().c_str());
#else
	m_List.SetItemText(index, 2, mpt::ToCString(val.FormatValueAsString()));
#endif
	m_List.SetSelectionMark(index);
	OnSettingsChanged();
}


void COptionsAdvanced::OnSaveNow()
{
	TrackerSettings::Instance().SaveSettings();
	theApp.GetSettings().WriteSettings();
}


OPENMPT_NAMESPACE_END
