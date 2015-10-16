/*
 * ProgressDialog.h
 * ----------------
 * Purpose: An abortable, progress-indicating dialog, e.g. for showing conversion progress.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

//====================================
class CProgressDialog : public CDialog
//====================================
{
public:
	bool m_abort;

	CProgressDialog(CWnd *parent = nullptr);

	// Set the window title
	void SetTitle(const TCHAR *title);
	// Set the text to be displayed along the progress bar.
	void SetText(const TCHAR *text);
	// Set the minimum and maximum value of the progress bar.
	void SetRange(uint32 min, uint32 max);
	// Set the current progress.
	void SetProgress(uint32 progress);
	// Set the progress bar to marquee mode, i.e. only show activity, but no specific progress percentage.
	void SetUnknownProgress();
	// Process all queued Windows messages
	void ProcessMessages();
	// Run method for this dialog that must implement the action to be carried out.
	virtual void Run() = 0;

protected:
	BOOL OnInitDialog();
	void OnCancel() { m_abort = true; }

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END