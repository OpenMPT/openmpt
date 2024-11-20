/*
 * DialogBase.cpp
 * --------------
 * Purpose: Base class for dialogs that adds functionality on top of CDialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "DialogBase.h"
#include "HighDPISupport.h"
#include "InputHandler.h"
#include "Mainfrm.h"

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(DialogBase, CDialog)
	//{{AFX_MSG_MAP(DialogBase)
	ON_MESSAGE(WM_DPICHANGED,             &DialogBase::OnDPIChanged)
	ON_MESSAGE(WM_DPICHANGED_AFTERPARENT, &DialogBase::OnDPIChanged)  // Dialog inside dialog
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL DialogBase::OnInitDialog()
{
	UpdateDPI();
	return CDialog::OnInitDialog();
}


BOOL DialogBase::PreTranslateMessage(MSG *pMsg)
{
	if(pMsg && HandleGlobalKeyMessage(*pMsg))
		return TRUE;

	return CDialog::PreTranslateMessage(pMsg);
}


bool DialogBase::HandleGlobalKeyMessage(const MSG &msg)
{
	// We handle keypresses before Windows has a chance to handle them (for alt etc..)
	if(msg.message == WM_KEYDOWN || msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP || msg.message == WM_SYSKEYDOWN)
	{
		if(CInputHandler *ih = CMainFrame::GetInputHandler())
		{
			const auto event = ih->Translate(msg);
			if(ih->KeyEvent(kCtxAllContexts, event) != kcNull)
			{
				// Special case: ESC is typically bound to stopping playback, but we also want to allow ESC to close dialogs
				if(msg.message != WM_KEYDOWN || msg.wParam != VK_ESCAPE)
					return true;  // Mapped to a command, no need to pass message on.
			}
		}
	}

	return false;
}


LRESULT DialogBase::OnDPIChanged(WPARAM, LPARAM)
{
	// We may receive a first WM_DPICHANGED before OnInitDialog() even runs. We don't want to tell our dialogs about a DPI "change" that early.
	const bool updateDPI = (m_dpi != 0);
	UpdateDPI();
	auto result = Default();  // Re-scale all dialog items
	if(updateDPI)
		OnDPIChanged();
	return result;
}


void DialogBase::UpdateDPI()
{
	m_dpi = HighDPISupport::GetDpiForWindow(m_hWnd);
}


INT_PTR DialogBase::OnToolHitTest(CPoint point, TOOLINFO *pTI) const
{
	INT_PTR nHit = CDialog::OnToolHitTest(point, pTI);
	if(nHit >= 0 && pTI && (pTI->uFlags & TTF_IDISHWND))
	{
		// Workaround to get tooltips even for disabled controls inside group boxes that are positioned in the "correct" tab order position.
		// For some reason doesn't work for enabled controls (probably because pTI->hwnd then doesn't point at the active control under the cursor),
		// so we use the default code path there, which works just fine.
		HWND child = reinterpret_cast<HWND>(pTI->uId);
		if(!::IsWindowEnabled(child))
		{
			pTI->uId = nHit;
			pTI->uFlags &= ~TTF_IDISHWND;
			::GetWindowRect(child, &pTI->rect);
			ScreenToClient(&pTI->rect);
		}
	}
	return nHit;
}


OPENMPT_NAMESPACE_END
