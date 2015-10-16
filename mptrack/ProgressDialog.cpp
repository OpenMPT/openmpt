/*
 * ProgressDialog.cpp
 * ------------------
 * Purpose: An abortable, progress-indicating dialog, e.g. for showing conversion progress.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "resource.h"
#include "ProgressDialog.h"

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(CProgressDialog, CDialog)
	ON_COMMAND(IDC_BUTTON1,	Run)
END_MESSAGE_MAP()

CProgressDialog::CProgressDialog(CWnd *parent)
//--------------------------------------------
	: CDialog(IDD_PROGRESS, parent)
	, m_abort(false)
{ }

BOOL CProgressDialog::OnInitDialog()
//----------------------------------
{
	CDialog::OnInitDialog();
	PostMessage(WM_COMMAND, IDC_BUTTON1);
	return TRUE;
}


void CProgressDialog::SetTitle(const TCHAR *title)
//------------------------------------------------
{
	SetWindowText(title);
}


void CProgressDialog::SetText(const TCHAR *text)
//----------------------------------------------
{
	SetDlgItemText(IDC_TEXT1, text);
}


void CProgressDialog::SetRange(uint32 min, uint32 max)
//----------------------------------------------------
{
	::SendMessage(::GetDlgItem(m_hWnd, IDC_PROGRESS1), PBM_SETRANGE32, min, max);
}


void CProgressDialog::SetProgress(uint32 progress)
//------------------------------------------------
{
	::SendMessage(::GetDlgItem(m_hWnd, IDC_PROGRESS1), PBM_SETPOS, progress, 0);
}


void CProgressDialog::SetUnknownProgress()
//----------------------------------------
{
	HWND wnd = ::GetDlgItem(m_hWnd, IDC_PROGRESS1);
	SetWindowLong(wnd, GWL_STYLE, GetWindowLong(wnd, GWL_STYLE) | PBS_MARQUEE);
}


void CProgressDialog::ProcessMessages()
//-------------------------------------
{
	MSG msg;
	while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}

OPENMPT_NAMESPACE_END
