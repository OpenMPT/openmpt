/*
 * MPTrackUtilWine.cpp
 * -------------------
 * Purpose: Wine utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "MPTrackUtilWine.h"
#include "Mptrack.h"
#include "../common/misc_util.h"
#include "../misc/mptWine.h"


OPENMPT_NAMESPACE_BEGIN


namespace Util
{


namespace Wine
{


class CExecutePosixShellScriptProgressDialog
	: public CDialog
{

protected:

	mpt::Wine::Context & wine;
	std::string m_Title;
	std::string m_Status;
	bool m_bAbort;
	std::string m_script;
	FlagSet<mpt::Wine::ExecFlags> m_Flags;
	std::map<std::string, std::vector<char> > m_Filetree;
	mpt::Wine::ExecResult m_ExecResult;
	std::string m_ExceptionString;

public:

	CExecutePosixShellScriptProgressDialog(mpt::Wine::Context & wine, std::string script, FlagSet<mpt::Wine::ExecFlags> flags, std::map<std::string, std::vector<char> > filetree, std::string title, std::string status, CWnd *parent = NULL);

	BOOL OnInitDialog();

	void OnCancel();

	afx_msg void OnButton1();

	static mpt::Wine::ExecuteProgressResult ProgressCancelCallback(void *userdata);

	static void ProgressCallback(void *userdata);

	mpt::Wine::ExecuteProgressResult Progress(bool allowCancel);
	
	void MessageLoop();

	mpt::Wine::ExecResult GetExecResult() const;
	std::string GetExceptionString() const;

private:

	DECLARE_MESSAGE_MAP()

};


BEGIN_MESSAGE_MAP(CExecutePosixShellScriptProgressDialog, CDialog)
	ON_COMMAND(IDC_BUTTON1,	&CExecutePosixShellScriptProgressDialog::OnButton1)
END_MESSAGE_MAP()


CExecutePosixShellScriptProgressDialog::CExecutePosixShellScriptProgressDialog(mpt::Wine::Context & wine, std::string script, FlagSet<mpt::Wine::ExecFlags> flags, std::map<std::string, std::vector<char> > filetree, std::string title, std::string status, CWnd *parent)
	: CDialog(IDD_PROGRESS, parent)
	, wine(wine)
	, m_Title(title)
	, m_Status(status)
	, m_bAbort(false)
	, m_script(script)
	, m_Flags(flags)
	, m_Filetree(filetree)
	, m_ExecResult(mpt::Wine::ExecResult::Error())
{
	return;
}


void CExecutePosixShellScriptProgressDialog::OnCancel()
{
	m_bAbort = true;
}


mpt::Wine::ExecuteProgressResult CExecutePosixShellScriptProgressDialog::ProgressCancelCallback(void *userdata)
{
	return mpt::void_ptr<CExecutePosixShellScriptProgressDialog>(userdata)->Progress(true);
}


void CExecutePosixShellScriptProgressDialog::ProgressCallback(void *userdata)
{
	mpt::void_ptr<CExecutePosixShellScriptProgressDialog>(userdata)->Progress(false);
}


void CExecutePosixShellScriptProgressDialog::MessageLoop()
{
	MSG msg;
	while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}


mpt::Wine::ExecResult CExecutePosixShellScriptProgressDialog::GetExecResult() const
{
	return m_ExecResult;
}


std::string CExecutePosixShellScriptProgressDialog::GetExceptionString() const
{
	return m_ExceptionString;
}


BOOL CExecutePosixShellScriptProgressDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetWindowText(mpt::ToCString(mpt::Charset::UTF8, m_Title));
	SetDlgItemText(IDCANCEL, _T("Cancel"));
	SetWindowLong(::GetDlgItem(m_hWnd, IDC_PROGRESS1), GWL_STYLE, GetWindowLong(::GetDlgItem(m_hWnd, IDC_PROGRESS1), GWL_STYLE) | PBS_MARQUEE);
	::SendMessage(::GetDlgItem(m_hWnd, IDC_PROGRESS1), PBM_SETMARQUEE, 1, 30); // 30 is Windows default, but Wine < 1.7.15 defaults to 0
	PostMessage(WM_COMMAND, IDC_BUTTON1);
	return TRUE;
}


mpt::Wine::ExecuteProgressResult CExecutePosixShellScriptProgressDialog::Progress(bool allowCancel)
{
	if(m_bAbort)
	{
		return mpt::Wine::ExecuteProgressAsyncCancel;
	}
	::ShowWindow(::GetDlgItem(m_hWnd, IDCANCEL), allowCancel ? SW_SHOW : SW_HIDE);
	MessageLoop();
	if(m_bAbort)
	{
		return mpt::Wine::ExecuteProgressAsyncCancel;
	}
	::Sleep(10);
	return mpt::Wine::ExecuteProgressContinueWaiting;
}


void CExecutePosixShellScriptProgressDialog::OnButton1()
{
	if(m_script.empty())
	{
		EndDialog(IDCANCEL);
		return;
	}

	SetDlgItemText(IDC_TEXT1, mpt::ToCString(mpt::Charset::UTF8, m_Status));
	MessageLoop();
	if(m_bAbort)
	{
		EndDialog(IDCANCEL);
		return;
	}
	::Sleep(10);

	try
	{
		m_ExecResult = wine.ExecutePosixShellScript(m_script, m_Flags, m_Filetree, m_Title, &ProgressCallback, &ProgressCancelCallback, this);
	} catch(const mpt::Wine::Exception &e)
	{
		m_ExceptionString = mpt::get_exception_text<std::string>(e);
		EndDialog(IDCANCEL);
		return;
	}

	MessageLoop();
	if(m_bAbort)
	{
		EndDialog(IDCANCEL);
		return;
	}

	SetDlgItemText(IDC_TEXT1, _T("Done."));
	::ShowWindow(::GetDlgItem(m_hWnd, IDCANCEL), SW_HIDE);
	for(int i = 0; i < 10; ++i)
	{
		MessageLoop();
		::Sleep(10);
	}

	MessageLoop();

	EndDialog(IDOK);
}


static void ProgressCallback(void * /*userdata*/ )
{
	MSG msg;
	while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	::Sleep(10);
}


static mpt::Wine::ExecuteProgressResult ProgressCancelCallback(void *userdata)
{
	ProgressCallback(userdata);
	return mpt::Wine::ExecuteProgressContinueWaiting;
}


mpt::Wine::ExecResult ExecutePosixShellScript(mpt::Wine::Context & wine, std::string script, FlagSet<mpt::Wine::ExecFlags> flags, std::map<std::string, std::vector<char> > filetree, std::string title, std::string status)
{
	if(flags[mpt::Wine::ExecFlagProgressWindow])
	{
		CExecutePosixShellScriptProgressDialog dlg(wine, script, flags, filetree, title, status, theApp.GetMainWnd());
		if(dlg.DoModal() != IDOK)
		{
			if(!dlg.GetExceptionString().empty())
			{
				throw mpt::Wine::Exception(dlg.GetExceptionString());
			}
			throw mpt::Wine::Exception("Canceled.");
		}
		return dlg.GetExecResult();
	} else
	{
		return wine.ExecutePosixShellScript(script, flags, filetree, title, &ProgressCallback, &ProgressCancelCallback, nullptr);
	}
}


Dialog::Dialog(std::string title, bool terminal)
	: m_Terminal(terminal)
	, m_Title(title)
{
	return;
}

std::string Dialog::Detect() const
{
	std::string script;
	script += std::string() + "chmod u+x ./build/wine/dialog.sh" + "\n";
	return script;
}

std::string Dialog::DialogVar() const
{
	if(m_Terminal)
	{
		return "./build/wine/dialog.sh tui";
	} else
	{
		return "./build/wine/dialog.sh gui";
	}
}

std::string Dialog::Title() const
{
	return m_Title;
}

std::string Dialog::Status(std::string text) const
{
	return std::string() + DialogVar() + " --infobox \"" + Title() + "\" \"" + text + "\"";
}

std::string Dialog::MessageBox(std::string text) const
{
	return std::string() + DialogVar() + " --msgbox \"" + Title() + "\" \"" + text + "\"";
}

std::string Dialog::YesNo(std::string text) const
{
	return std::string() + DialogVar() + " --yesno \"" + Title() + "\" \"" + text + "\"";
}

std::string Dialog::TextBox(std::string text) const
{
	return std::string() + DialogVar() + " --textbox \"" + Title() + "\" \"" + text + "\"";
}

std::string Dialog::Progress(std::string text) const
{
	return std::string() + DialogVar() + " --gauge \"" + Title() + "\" \"" + text + "\"";
}


} // namespace Wine


} // namespace Util


OPENMPT_NAMESPACE_END
