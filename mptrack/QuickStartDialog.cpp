/*
 * QuickStartDialog.cpp
 * --------------------
 * Purpose: Dialog to show inside the MDI client area when no modules are loaded.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "QuickStartDialog.h"
#include "Image.h"
#include "ImageLists.h"
#include "Mainfrm.h"
#include "Mptrack.h"
#include "MPTrackUtil.h"
#include "resource.h"
#include "TrackerSettings.h"


OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(QuickStartDlg, ResizableDialog)
	ON_COMMAND(IDC_BUTTON1,   &QuickStartDlg::OnNew)
	ON_COMMAND(IDC_BUTTON2,   &QuickStartDlg::OnOpen)
	ON_COMMAND(ID_REMOVE,     &QuickStartDlg::OnRemoveMRUItem)
	ON_COMMAND(ID_REMOVE_ALL, &QuickStartDlg::OnRemoveAllMRUItems)

	ON_EN_CHANGE(IDC_EDIT1, &QuickStartDlg::OnUpdateFilter)
	
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, &QuickStartDlg::OnOpenFile)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &QuickStartDlg::OnRightClickFile)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &QuickStartDlg::OnItemChanged)
END_MESSAGE_MAP()


void QuickStartDlg::DoDataExchange(CDataExchange *pDX)
{
	ResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON1, m_newButton);
	DDX_Control(pDX, IDC_BUTTON2, m_openButton);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Control(pDX, IDC_EDIT1, m_find);
}


QuickStartDlg::QuickStartDlg(const std::vector<mpt::PathString> &templates, const std::vector<mpt::PathString> &examples, CWnd *parent)
{
	m_newButton.SetAccessibleText(_T("New Module"));
	m_openButton.SetAccessibleText(_T("Open Module"));
	Create(IDD_QUICKSTART, parent);

	m_groupsEnabled = m_list.EnableGroupView();
	m_list.SetRedraw(FALSE);
	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT);
	m_list.InsertColumn(0, _T("File"), LVCFMT_LEFT);
	m_list.InsertColumn(1, _T("Location"), LVCFMT_LEFT);
	
	const std::pair<const std::vector<mpt::PathString> &, const TCHAR *> PathGroups[] =
	{
		{TrackerSettings::Instance().mruFiles, _T("Recent Files")},
		{templates, _T("Templates")},
		{examples, _T("Example Modules")},
	};
	static_assert(mpt::array_size<decltype(PathGroups)>::size == mpt::array_size<decltype(m_paths)>::size);
	for(size_t groupId = 0; groupId < std::size(PathGroups); groupId++)
	{
		if(m_groupsEnabled)
		{
			LVGROUP group{};
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
			group.cbSize = LVGROUP_V5_SIZE;
#else
			group.cbSize = sizeof(group);
#endif
			group.mask = LVGF_HEADER | LVGF_GROUPID;
#if defined(UNICODE)
			group.pszHeader = const_cast<TCHAR *>(PathGroups[groupId].second);
#else
			std::wstring titlew = mpt::ToWide(mpt::winstring(PathGroups[groupId].second));
			group.pszHeader = const_cast<WCHAR *>(titlew.c_str());
#endif
			group.cchHeader = 0;
			group.pszFooter = nullptr;
			group.cchFooter = 0;
			group.iGroupId = static_cast<int>(groupId);
			group.stateMask = 0;
			group.state = 0;
			group.uAlign = LVGA_HEADER_LEFT;
			ListView_InsertGroup(m_list.m_hWnd, -1, &group);
			ListView_SetGroupState(m_list.m_hWnd, group.iGroupId, LVGS_COLLAPSIBLE, LVGS_COLLAPSIBLE);
		}

		m_paths[groupId] = PathGroups[groupId].first;
	}
	UpdateFileList();
	m_list.SetColumnWidth(0, LVSCW_AUTOSIZE);
	m_list.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	m_list.SetRedraw(TRUE);
	m_list.Invalidate(FALSE);
	OnItemChanged(nullptr, nullptr);
	UpdateHeight();
}


BOOL QuickStartDlg::OnInitDialog()
{
	ResizableDialog::OnInitDialog();
	OnDPIChanged();
	return TRUE;
}


void QuickStartDlg::OnDPIChanged()
{
	const double scaling = GetDPI() / 96.0;

	if(m_prevDPI)
	{
		// We don't use this as a stand-alone dialog but rather embed it in the MDI child area, and it is not resized automatically.
		MPT_ASSERT(GetStyle() & WS_CHILD);
		CRect windowRect;
		GetClientRect(windowRect);
		windowRect.right = Util::muldiv(windowRect.right, GetDPI(), m_prevDPI);
		windowRect.bottom = Util::muldiv(windowRect.bottom, GetDPI(), m_prevDPI);
		HighDPISupport::AdjustWindowRectEx(windowRect, GetStyle(), FALSE, GetExStyle(), GetDPI());
		SetWindowPos(nullptr, 0, 0, windowRect.Width(), windowRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
	}
	m_prevDPI = GetDPI();

	m_list.SetImageList(&CMainFrame::GetMainFrame()->m_PatternIcons, LVSIL_SMALL);
	m_list.SetColumnWidth(0, LVSCW_AUTOSIZE);
	m_list.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);

	m_bmpNew.DeleteObject();
	m_bmpOpen.DeleteObject();

	std::unique_ptr<RawGDIDIB> bitmapNew, bitmapOpen;
	try
	{
		bitmapNew = LoadPixelImage(GetResource(MAKEINTRESOURCE(IDB_NEW_BIG), _T("PNG")), scaling);
		bitmapOpen = LoadPixelImage(GetResource(MAKEINTRESOURCE(IDB_OPEN_BIG), _T("PNG")), scaling);
	} catch(...)
	{
		return;
	}
	auto *dc = GetDC();
	CopyToCompatibleBitmap(m_bmpNew, *dc, *bitmapNew);
	CopyToCompatibleBitmap(m_bmpOpen, *dc, *bitmapOpen);
	ReleaseDC(dc);
	m_newButton.SetBitmap(m_bmpNew);
	m_openButton.SetBitmap(m_bmpOpen);
}


BOOL QuickStartDlg::PreTranslateMessage(MSG *pMsg)
{
	// Use up/down keys to navigate in list, even if search field is focussed. This also skips the group headers during navigation
	if(pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN) && GetFocus() == &m_find)
	{
		const int curSel = m_list.GetSelectionMark();
		const int selItem = std::clamp(curSel + (pMsg->wParam == VK_UP ? -1 : 1), 0, m_list.GetItemCount() - 1);
		if(curSel != selItem)
		{
			if(curSel != -1)
				m_list.SetItemState(curSel, 0, LVIS_SELECTED);
			m_list.SetItemState(selItem, LVIS_SELECTED, LVIS_SELECTED);
			m_list.SetSelectionMark(selItem);
		}
		return TRUE;
	}

	return ResizableDialog::PreTranslateMessage(pMsg);
}


void QuickStartDlg::UpdateHeight()
{
	// Try to make the view tall enough to view the entire list contents, but only up to 90% of the MDI client area
	CRect listRect, viewRect;
	m_list.GetClientRect(listRect);
	m_list.GetViewRect(viewRect);

	CRect windowRect, parentRect, itemRect;
	GetClientRect(windowRect);
	GetParent()->GetClientRect(parentRect);
	m_list.GetItemRect(0, itemRect, LVIR_BOUNDS);
	viewRect.bottom += itemRect.Height() * 2;  // View height calculation seems to be a bit off and would still cause a vertical scrollbar to appear without this adjustment
	if(viewRect.bottom > listRect.bottom)
		windowRect.bottom += viewRect.bottom - listRect.bottom;
	const int maxHeight = std::max(HighDPISupport::ScalePixels(154, m_hWnd), Util::muldiv(parentRect.bottom, 9, 10));
	LimitMax(windowRect.bottom, maxHeight);
	HighDPISupport::AdjustWindowRectEx(windowRect, GetStyle(), FALSE, GetExStyle(), GetDPI());
	SetWindowPos(nullptr, 0, 0, windowRect.Width(), windowRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
}


void QuickStartDlg::OnNew()
{
	theApp.NewDocument();
}


void QuickStartDlg::OnOpen()
{
	CMainFrame::GetMainFrame()->SendMessage(WM_COMMAND, ID_FILE_OPEN);
}


void QuickStartDlg::OnOK()
{
	OnOpenFile(nullptr, nullptr);
}


void QuickStartDlg::OnRemoveMRUItem()
{
	auto &mruFiles = TrackerSettings::Instance().mruFiles;
	int i = -1;
	while((i = m_list.GetNextItem(i, LVNI_SELECTED | LVNI_ALL)) != -1)
	{
		if(GetItemGroup(i) != 0)
			continue;
		auto &path = m_paths[0][GetItemIndex(i)];
		if(auto it = std::find(mruFiles.begin(), mruFiles.end(), path); it != mruFiles.end())
			mruFiles.erase(it);
		path = {};
		m_list.DeleteItem(i);
		i--;
	}
	CMainFrame::GetMainFrame()->UpdateMRUList();
}


void QuickStartDlg::OnRemoveAllMRUItems()
{
	TrackerSettings::Instance().mruFiles.clear();
	m_paths[0].clear();
	for(int i = m_list.GetItemCount() - 1; i >= 0; i--)
	{
		if(GetItemGroup(i) == 0)
			m_list.DeleteItem(i);
	}
	CMainFrame::GetMainFrame()->UpdateMRUList();
}


void QuickStartDlg::OnUpdateFilter()
{
	m_list.SetRedraw(FALSE);
	LPARAM highlight = LPARAM(-1);
	if(int sel = m_list.GetSelectionMark(); sel != -1)
		highlight = m_list.GetItemData(sel);
	m_list.DeleteAllItems();
	CString filter;
	m_find.GetWindowText(filter);
	UpdateFileList(highlight, filter);
	m_list.SetRedraw(TRUE);
}


void QuickStartDlg::UpdateFileList(LPARAM highlight, CString filter)
{
	const bool applyFilter = !filter.IsEmpty();
	if(applyFilter)
		filter = _T("*") + filter + _T("*");

	LVITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
	if(m_groupsEnabled)
		lvi.mask |= LVIF_GROUPID;
	lvi.iSubItem = 0;
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.cchTextMax = 0;
	lvi.iImage = TIMAGE_MODULE_FILE;
	lvi.iIndent = 0;

	bool highlightFound = false;
	int itemId = -1;
	for(size_t groupId = 0; groupId < m_paths.size(); groupId++)
	{
		lvi.iGroupId = static_cast<int>(groupId);
		for(size_t i = 0; i < m_paths[groupId].size(); i++)
		{
			if(m_paths[groupId][i].empty() || (applyFilter && !PathMatchSpec(m_paths[groupId][i].AsNative().c_str(), filter)))
				continue;
			const auto filename = m_paths[groupId][i].GetFilename().AsNative();
			lvi.iItem = ++itemId;
			lvi.lParam = static_cast<LPARAM>(i | (groupId << 24));
			lvi.pszText = const_cast<TCHAR *>(filename.c_str());
			m_list.InsertItem(&lvi);
			m_list.SetItemText(itemId, 1, m_paths[groupId][i].GetDirectoryWithDrive().AsNative().c_str());
			if(lvi.lParam == highlight)
			{
				m_list.SetItemState(itemId, LVIS_SELECTED, LVIS_SELECTED);
				m_list.SetSelectionMark(itemId);
				highlightFound = true;
			}
		}
	}
	if(!highlightFound)
	{
		m_list.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
		m_list.SetSelectionMark(0);
	}
}


void QuickStartDlg::OnOpenFile(NMHDR *, LRESULT *)
{
	struct OpenItem
	{
		mpt::PathString path;  // Must create copy because dialog will get destroyed after successfully loading the first file
		int item = 0;
		int group = 0;
	};
	std::vector<OpenItem> files;
	int i = -1;
	while((i = m_list.GetNextItem(i, LVNI_SELECTED | LVNI_ALL)) != -1)
	{
		const int group = GetItemGroup(i);
		const size_t index = GetItemIndex(i);
		files.push_back({std::move(m_paths[group][index]), i, group});
	}
	bool success = false;
	for(const auto &item : files)
	{
		if(item.group != 1)
			success |= (theApp.OpenDocumentFile(item.path.ToCString()) != nullptr);
		else
			success |= (theApp.OpenTemplateFile(item.path) != nullptr);
	}

	if(!success)
	{
		// If at least one item managed to load, the dialog will now be destroyed, and there are no items to delete from the list
		for(auto it = files.rbegin(); it != files.rend(); it++)
		{
			m_list.DeleteItem(it->item);
		}
	}
}


void QuickStartDlg::OnRightClickFile(NMHDR *nmhdr, LRESULT *)
{
	const NMITEMACTIVATE *item = reinterpret_cast<NMITEMACTIVATE *>(nmhdr);
	if(item->iItem < 0)
		return;
	CPoint point = item->ptAction;
	m_list.ClientToScreen(&point);
	// Can only remove MRU items
	if(GetItemGroup(item->iItem) != 0)
		return;
	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, ID_REMOVE, _T("&Remove from Recent File List"));
	menu.AppendMenu(MF_STRING, ID_REMOVE_ALL, _T("&Clear Recent File List"));
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	menu.DestroyMenu();
}


void QuickStartDlg::OnItemChanged(NMHDR *, LRESULT *)
{
	GetDlgItem(IDOK)->EnableWindow(m_list.GetSelectedCount() != 0 ? TRUE : FALSE);
}

OPENMPT_NAMESPACE_END
