/*
 * Ctrl_com.h
 * ----------
 * Purpose: Song comments tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

class CCtrlComments final : public CModControlDlg
{
protected:
	CEdit m_EditComments;
	int charWidth = 0;
	bool m_Reformatting = false;

public:
	CCtrlComments(CModControlView &parent, CModDoc &document);

	//{{AFX_VIRTUAL(CCtrlComments)
	Setting<LONG> &GetSplitPosRef() override { return TrackerSettings::Instance().glCommentsWindowHeight; }
	BOOL OnInitDialog() override;
	void DoDataExchange(CDataExchange *pDX) override;  // DDX/DDV support
	void RecalcLayout() override;
	void UpdateView(UpdateHint hint, CObject *pObj = nullptr) override;
	CRuntimeClass *GetAssociatedViewClass() override;
	void OnActivatePage(LPARAM) override;
	void OnDeactivatePage() override;
	BOOL PreTranslateMessage(MSG *pMsg) override;
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CCtrlComments)
	afx_msg void OnCommentsUpdated();
	afx_msg void OnCommentsChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
