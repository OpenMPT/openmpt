/*
 *
 * Reporting.cpp
 * -------------
 * Purpose: A class for showing notifications, prompts, etc...
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */


#include "Stdafx.h"
#include "Reporting.h"
#include "../mptrack/Mainfrm.h"


UINT Reporting::Notification(CString text, UINT flags, CWnd *parent)
//------------------------------------------------------------------
{
	return Notification(text, MAINFRAME_TITLE, flags, parent);
}


UINT Reporting::Notification(CString text, CString caption, UINT flags, CWnd *parent)
//-----------------------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if(pMainFrm != nullptr && pMainFrm->GetInputHandler() != nullptr)
	{
		pMainFrm->GetInputHandler()->Bypass(true);
	}

	UINT result = ::MessageBox((parent ? parent->m_hWnd : NULL), text, caption, flags);

	if(pMainFrm != nullptr && pMainFrm->GetInputHandler() != nullptr)
	{
		pMainFrm->GetInputHandler()->Bypass(false);
	}

	return result;
}
