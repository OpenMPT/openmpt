/*
 * CloseMainDialog.h
 * -----------------
 * Purpose: Class for displaying a dialog with a list of unsaved documents, and the ability to choose which documents should be saved or not.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"
#include "ResizableDialog.h"

OPENMPT_NAMESPACE_BEGIN

class CloseMainDialog: public ResizableDialog
{
protected:
	
	CListBox m_List;
	CPoint m_minSize;

	static CString FormatTitle(const CModDoc *modDoc, bool fullPath);

public:
	CloseMainDialog();
	~CloseMainDialog();

protected:
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;

	afx_msg void OnSaveAll();
	afx_msg void OnSaveNone();

	afx_msg void OnSwitchFullPaths();

	DECLARE_MESSAGE_MAP()

};

OPENMPT_NAMESPACE_END
