/*
 * ProgressDialog.h
 * ----------------
 * Purpose: An abortable, progress-indicating dialog, e.g. for showing conversion progress.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

OPENMPT_NAMESPACE_BEGIN

class CProgressDialog : public CDialog
{
private:
	ITaskbarList3 *m_taskBarList = nullptr;
	uint64 m_min = 0, m_max = 0, m_shift = 0;

public:
	bool m_abort = false;

	CProgressDialog(CWnd *parent = nullptr);
	~CProgressDialog();

	// Set the window title
	void SetTitle(const TCHAR *title);
	// Set the text on abort button
	void SetAbortText(const TCHAR *abort);
	// Set the text to be displayed along the progress bar.
	void SetText(const TCHAR *text);
	// Set the minimum and maximum value of the progress bar.
	void SetRange(uint64 min, uint64 max);
	// Set the current progress.
	void SetProgress(uint64 progress);
	// Show the progress in the task bar as well
	void EnableTaskbarProgress();
	// Process all queued Windows messages
	void ProcessMessages();
	// Run method for this dialog that must implement the action to be carried out.
	virtual void Run() = 0;

protected:
	BOOL OnInitDialog() override;
	void OnCancel() override { m_abort = true; }

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END