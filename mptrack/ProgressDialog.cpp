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
#include "Mptrack.h"

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(CProgressDialog, CDialog)
	ON_COMMAND(IDC_BUTTON1,	&CProgressDialog::Run)
END_MESSAGE_MAP()

CProgressDialog::CProgressDialog(CWnd *parent)
	: CDialog(IDD_PROGRESS, parent)
{ }

CProgressDialog::~CProgressDialog()
{
	if(m_taskBarList)
	{
		m_taskBarList->SetProgressState(*theApp.m_pMainWnd, TBPF_NOPROGRESS);
		m_taskBarList->Release();
	}

	if(IsWindow(m_hWnd))
	{
		// This should only happen if this dialog gets destroyed as part of stack unwinding
		EndDialog(IDCANCEL);
		DestroyWindow();
	}
}

BOOL CProgressDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	PostMessage(WM_COMMAND, IDC_BUTTON1);
	return TRUE;
}


void CProgressDialog::SetTitle(const TCHAR *title)
{
	SetWindowText(title);
}



void CProgressDialog::SetAbortText(const TCHAR *abort)
{
	SetDlgItemText(IDCANCEL, abort);
}


void CProgressDialog::SetText(const TCHAR *text)
{
	SetDlgItemText(IDC_TEXT1, text);
}


void CProgressDialog::SetRange(uint64 min, uint64 max)
{
	MPT_ASSERT(min <= max);
	m_min = min;
	m_max = max;
	m_shift = 0;
	// Is the range too big for 32-bit values?
	while(max > int32_max)
	{
		m_shift++;
		max >>= 1;
	}
	::SendMessage(::GetDlgItem(m_hWnd, IDC_PROGRESS1), PBM_SETRANGE32, static_cast<uint32>(m_min >> m_shift), static_cast<uint32>(m_max >> m_shift));
}


void CProgressDialog::SetProgress(uint64 progress)
{
	::SendMessage(::GetDlgItem(m_hWnd, IDC_PROGRESS1), PBM_SETPOS, static_cast<uint32>(progress >> m_shift), 0);
	if(m_taskBarList != nullptr)
	{
		m_taskBarList->SetProgressValue(*theApp.m_pMainWnd, progress - m_min, m_max - m_min);
	}
}


void CProgressDialog::EnableTaskbarProgress()
{
	if(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_ALL, IID_ITaskbarList3, (void**)&m_taskBarList) != S_OK)
	{
		return;
	}
	if(m_taskBarList != nullptr)
	{
		m_taskBarList->SetProgressState(*theApp.m_pMainWnd, TBPF_NORMAL);
	}
}


void CProgressDialog::ProcessMessages()
{
	MSG msg;
	while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}

OPENMPT_NAMESPACE_END
