/*
 * ResizableDialog.cpp
 * -------------------
 * Purpose: A wrapper for resizable MFC dialogs that fixes the dialog's minimum size,
 *          as MFC does not scale controls properly if the user makes the dialog smaller than it originally was.
 *          It also does not handle DPI changes properly. Because CMFCDynamicLayout is not inheritable in any practical way,
 *          we do a partial re-implementation.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "ResizableDialog.h"
#include "HighDPISupport.h"
#include "MPTrackUtil.h"
#include "resource.h"

OPENMPT_NAMESPACE_BEGIN


class CMFCDynamicLayoutDataEx : public CMFCDynamicLayoutData
{
public:
	// This only exists because m_listCtrls is protected... use the chance to turn it into a slightly more sensible container
	std::vector<ResizableDialog::DynamicItem> GetItems(CWnd &parentWnd) const
	{
		std::vector<ResizableDialog::DynamicItem> items;
		CWnd *child = parentWnd.GetWindow(GW_CHILD);
		POSITION pos = m_listCtrls.GetHeadPosition();
		while(pos != nullptr)
		{
			const Item &item = m_listCtrls.GetNext(pos);
			if(!item.m_moveSettings.IsNone() || !item.m_sizeSettings.IsNone())
			{
				CRect rect;
				child->GetWindowRect(rect);
				parentWnd.ScreenToClient(rect);
				ResizableDialog::DynamicItem &dynamicItem = items.emplace_back();
				dynamicItem.hwnd = *child;
				dynamicItem.initialPoint = rect.TopLeft();
				dynamicItem.initialSize = rect.Size();
				dynamicItem.moveSettings = item.m_moveSettings;
				dynamicItem.sizeSettings = item.m_sizeSettings;
			}
			child = child->GetNextWindow();
		}
		return items;
	}
};


BEGIN_MESSAGE_MAP(ResizableDialog, DialogBase)
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()


ResizableDialog::ResizableDialog(UINT nIDTemplate, CWnd *pParentWnd)
	: DialogBase{nIDTemplate, pParentWnd}
{ }


BOOL ResizableDialog::OnInitDialog()
{
	// Take our original measurements before MFC tries to load the dynamic layout in OnInitDialog(),
	// as it may modify the window dimensions (especially in mixed-DPI contexts) and cause some controls to be misplaced.
	CRect rect;
	GetWindowRect(rect);
	m_minSize = rect.Size();

	GetClientRect(rect);
	m_originalClientSize = rect.Size();

	UpdateDPI();
	m_originalDPI = GetDPI();

	DialogBase::OnInitDialog();
	LoadDynamicLayout(m_lpszTemplateName);

	HICON icon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MODULETYPE));
	SetIcon(icon, FALSE);
	SetIcon(icon, TRUE);

	return TRUE;
}


void ResizableDialog::PostNcDestroy()
{
	// In case the dialog object gets reused (e.g. pattern clipboard dialog)
	m_dynamicItems.clear();
	m_minSize = {};
	DialogBase::PostNcDestroy();
}


void ResizableDialog::LoadDynamicLayout(const TCHAR *lpszResourceName)
{
	if(!m_hWnd || !lpszResourceName)
		return;

	const auto resource = GetResource(lpszResourceName, RT_DIALOG_LAYOUT);
	if(resource.empty())
		return;
	
	CMFCDynamicLayoutDataEx layoutData;
	layoutData.ReadResource(const_cast<std::byte *>(resource.data()), static_cast<UINT>(resource.size()));
	m_dynamicItems = layoutData.GetItems(*this);
	EnableDynamicLayout(FALSE);  // Free MFC's own dynamic layout resources
}


CRect ResizableDialog::AdjustItemRect(const DynamicItem &item, const CSize windowSize) const
{
	const double scale = static_cast<double>(m_originalDPI) / GetDPI();
	const double ratioX = 0.01 * (windowSize.cx * scale - m_originalClientSize.cx);
	const double ratioY = 0.01 * (windowSize.cy * scale - m_originalClientSize.cy);

	CPoint move;
	if(item.moveSettings.IsHorizontal())
		move.x = mpt::saturate_trunc<int>(ratioX * item.moveSettings.m_nXRatio);
	if(item.moveSettings.IsVertical())
		move.y = mpt::saturate_trunc<int>(ratioY * item.moveSettings.m_nYRatio);

	CSize size;
	if(item.sizeSettings.IsHorizontal())
		size.cx = mpt::saturate_trunc<int>(ratioX * item.sizeSettings.m_nXRatio);
	if(item.sizeSettings.IsVertical())
		size.cy = mpt::saturate_trunc<int>(ratioY * item.sizeSettings.m_nYRatio);

	auto itemPoint = item.initialPoint + move;
	auto itemSize = item.initialSize + size;
	if(GetDPI() != m_originalDPI)
	{
		itemPoint.x = MulDiv(itemPoint.x, GetDPI(), m_originalDPI);
		itemPoint.y = MulDiv(itemPoint.y, GetDPI(), m_originalDPI);
		itemSize.cx = MulDiv(itemSize.cx, GetDPI(), m_originalDPI);
		itemSize.cy = MulDiv(itemSize.cy, GetDPI(), m_originalDPI);
	}

	return CRect{itemPoint, itemSize};
}


void ResizableDialog::ResizeDynamicLayout()
{
	if(m_dynamicItems.empty() || IsIconic() || !m_enableAutoLayout)
		return;

	CRect rectWindow;
	GetClientRect(rectWindow);
	const CSize windowSize = rectWindow.Size();
	HDWP dwp = ::BeginDeferWindowPos(static_cast<int>(m_dynamicItems.size()));
	for(const auto &item : m_dynamicItems)
	{
		if(::IsWindow(item.hwnd))
		{
			CRect rect = AdjustItemRect(item, windowSize);
			::DeferWindowPos(dwp, item.hwnd, nullptr, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER | SWP_NOREPOSITION | SWP_NOACTIVATE | SWP_NOCOPYBITS);
		}
	}
	::EndDeferWindowPos(dwp);
}


void ResizableDialog::OnGetMinMaxInfo(MINMAXINFO *mmi)
{
	if(m_enableAutoLayout)
	{
		auto size = m_minSize;
		if(GetDPI() != m_originalDPI)
		{
			size.x = MulDiv(size.x, GetDPI(), m_originalDPI);
			size.y = MulDiv(size.y, GetDPI(), m_originalDPI);
		}
		mmi->ptMinTrackSize = size;
	}
	DialogBase::OnGetMinMaxInfo(mmi);
}

OPENMPT_NAMESPACE_END
