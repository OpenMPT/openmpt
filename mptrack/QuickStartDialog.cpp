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
	ON_WM_SIZE()

	ON_COMMAND(IDC_BUTTON1,   &QuickStartDlg::OnNew)
	ON_COMMAND(IDC_BUTTON2,   &QuickStartDlg::OnOpen)
	ON_COMMAND(ID_REMOVE,     &QuickStartDlg::OnRemoveMRUItem)
	ON_COMMAND(ID_REMOVE_ALL, &QuickStartDlg::OnRemoveAllMRUItems)

	ON_EN_CHANGE(IDC_EDIT1, &QuickStartDlg::OnUpdateFilter)
	
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, &QuickStartDlg::OnOpenFile)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &QuickStartDlg::OnRightClickFile)
END_MESSAGE_MAP()


void QuickStartDlg::DoDataExchange(CDataExchange *pDX)
{
	ResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON1, m_newButton);
	DDX_Control(pDX, IDC_BUTTON2, m_openButton);
	DDX_Control(pDX, IDCANCEL, m_closeButton);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Control(pDX, IDC_EDIT1, m_find);
}


QuickStartDlg::QuickStartDlg(const std::vector<mpt::PathString> &templates, const std::vector<mpt::PathString> &examples, CWnd *parent)
{
	m_newButton.SetAccessibleText(_T("New Module"));
	m_openButton.SetAccessibleText(_T("Open Module"));
	m_closeButton.SetAccessibleText(_T("Close"));
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
	m_list.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
	m_list.SetColumnWidth(0, LVSCW_AUTOSIZE);
	m_list.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	m_list.SetRedraw(TRUE);
	m_list.Invalidate(FALSE);
	UpdateHeight();
}


BOOL QuickStartDlg::OnInitDialog()
{
	ResizableDialog::OnInitDialog();
	EnableToolTips();
	OnDPIChanged();
	return TRUE;
}


void QuickStartDlg::OnDPIChanged()
{
	const double scaling = GetDPI() / 96.0;

	if(m_prevDPI)
	{
		CRect windowRect{CPoint{}, m_prevSize};
		windowRect.right = Util::muldiv(windowRect.right, GetDPI(), m_prevDPI);
		windowRect.bottom = Util::muldiv(windowRect.bottom, GetDPI(), m_prevDPI);
		AdjustWindowRectEx(windowRect, GetStyle(), FALSE, GetExStyle());
		SetWindowPos(nullptr, 0, 0, windowRect.Width(), windowRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
	}

	m_list.SetImageList(&CMainFrame::GetMainFrame()->m_PatternIcons, LVSIL_SMALL);
	m_list.SetColumnWidth(0, LVSCW_AUTOSIZE);
	m_list.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);

	m_buttonFont.DeleteObject();
	m_buttonFont.CreateFont(mpt::saturate_round<int>(14 * scaling), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, _T("Marlett"));
	m_closeButton.SetFont(&m_buttonFont);

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


INT_PTR QuickStartDlg::OnToolHitTest(CPoint point, TOOLINFO *pTI) const
{
	auto result = ResizableDialog::OnToolHitTest(point, pTI);
	if(result == IDCANCEL)
	{
		// MFC will free() the text
		mpt::tstring text = _T("Close");
		TCHAR *textP = static_cast<TCHAR *>(calloc(text.size() + 1, sizeof(TCHAR)));
		std::copy(text.begin(), text.end(), textP);
		pTI->lpszText = textP;
	}
	return result;
}


BOOL QuickStartDlg::PreTranslateMessage(MSG *pMsg)
{
	// Use up/down keys to navigate in list, even if search field is focussed. This also skips the group headers during navigation
	if(pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN) && GetFocus() == &m_find)
	{
		m_list.ModifyStyle(0, LVS_SHOWSELALWAYS);
		int selItem = std::clamp(m_list.GetSelectionMark() + (pMsg->wParam == VK_UP ? -1 : 1), 0, m_list.GetItemCount() - 1);
		m_list.SetItemState(selItem, LVIS_SELECTED, LVIS_SELECTED);
		m_list.SetSelectionMark(selItem);
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
	const int maxHeight = Util::muldiv(parentRect.bottom, 9, 10);
	LimitMax(windowRect.bottom, maxHeight);
	AdjustWindowRectEx(windowRect, GetStyle(), FALSE, GetExStyle());
	SetWindowPos(nullptr, 0, 0, windowRect.Width(), windowRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
}


void QuickStartDlg::OnSize(UINT nType, int cx, int cy)
{
	ResizableDialog::OnSize(nType, cx, cy);
	m_prevSize.SetSize(cx, cy);
	m_prevDPI = GetDPI();
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
	if(const CWnd *focus = GetFocus(); focus == &m_list || focus == &m_find)
		OnOpenFile(nullptr, nullptr);
}


void QuickStartDlg::OnRemoveMRUItem()
{
	const int index = m_list.GetSelectionMark();
	if(index < 0)
		return;
	auto &mruFiles= TrackerSettings::Instance().mruFiles;
	if(static_cast<size_t>(index) >= mruFiles.size())
		return;
	mruFiles.erase(mruFiles.begin() + index);
	m_list.DeleteItem(index);
	m_paths[GetItemGroup(index)][GetItemIndex(index)] = {};
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
	m_list.DeleteAllItems();
	CString filter;
	m_find.GetWindowText(filter);
	UpdateFileList(filter);
	m_list.SetRedraw(TRUE);
}


void QuickStartDlg::UpdateFileList(CString filter)
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
		}
	}
}


void QuickStartDlg::OnOpenFile(NMHDR *, LRESULT *)
{
	const int i = m_list.GetSelectionMark();
	if(i < 0)
		return;
	const size_t index = GetItemIndex(i);
	const int group = GetItemGroup(i);
	const auto &path = m_paths[group][index];
	CDocument *doc = nullptr;
	if(group != 1)
		doc = theApp.OpenDocumentFile(path.ToCString());
	else
		doc = theApp.OpenTemplateFile(path);

	if(!doc)
	{
		m_list.DeleteItem(i);
		m_paths[group][i] = {};
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

OPENMPT_NAMESPACE_END
