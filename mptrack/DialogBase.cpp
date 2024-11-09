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


OPENMPT_NAMESPACE_END
