/*
 * CloseMainDialog.h
 * -----------------
 * Purpose: Class for displaying a dialog with a list of unsaved documents, and the ability to choose which documents should be saved or not.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

//===================================
class CloseMainDialog: public CDialog
//===================================
{
protected:
	
	CListBox m_List;

	CString FormatTitle(const CModDoc *pModDoc, bool fullPath);

public:
	CloseMainDialog();
	~CloseMainDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	afx_msg void OnSaveAll();
	afx_msg void OnSaveNone();

	afx_msg void OnSwitchFullPaths();

	DECLARE_MESSAGE_MAP()

};

OPENMPT_NAMESPACE_END
