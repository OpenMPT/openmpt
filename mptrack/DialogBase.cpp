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
	// We handle keypresses before Windows has a chance to handle them (for alt etc..)
	if(pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP || pMsg->message == WM_SYSKEYUP || pMsg->message == WM_SYSKEYDOWN)
	{
		if(CInputHandler *ih = CMainFrame::GetInputHandler())
		{
			const auto event = ih->Translate(*pMsg);
			if(ih->KeyEvent(kCtxAllContexts, event) != kcNull)
			{
				// Special case: ESC is typically bound to stopping playback, but we also want to allow ESC to close dialogs
				if(pMsg->message != WM_KEYDOWN || pMsg->wParam != VK_ESCAPE)
					return TRUE;  // Mapped to a command, no need to pass message on.
			}
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

OPENMPT_NAMESPACE_END
