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
#include "Mainfrm.h"
#include "InputHandler.h"

OPENMPT_NAMESPACE_BEGIN

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


OPENMPT_NAMESPACE_END
