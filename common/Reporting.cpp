#include "Stdafx.h"
#include "Reporting.h"
#include "../mptrack/Mainfrm.h"

UINT Reporting::Notification(CString text, UINT flags /* = MB_OK*/, HWND parent /* = NULL*/)
//------------------------------------------------------------------------------------------
{
	return ::MessageBox(parent, text, MAINFRAME_TITLE, flags);
};


UINT Reporting::Notification(CString text, CString caption, UINT flags /* = MB_OK*/, HWND parent /* = NULL*/)
//-----------------------------------------------------------------------------------------------------------
{
	if(parent == NULL && CMainFrame::GetMainFrame() != nullptr)
	{
		parent = CMainFrame::GetMainFrame()->m_hWnd;
	}

	return ::MessageBox(parent, text, caption, flags);
};
