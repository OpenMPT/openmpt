/*
 * UpdateToolTip.h
 * ---------------
 * Purpose: Implementation of the update tooltip in the main toolbar.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

class UpdateToolTip : public CToolTipCtrl
{
public:
	enum class PopAction
	{
		Undetermined,
		CloseButton,
		ClickLink,
		ClickBubble,
	};
protected:
	CString m_infoURL;
	PopAction m_popAction = PopAction::Undetermined;

public:
	bool ShowUpdate(CWnd &parent, const CString &newVersion, const CString &infoURL, const CRect rectClient, const CPoint ptScreen, const int buttonID);

protected:
	void SetResult(PopAction action);

	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnPop(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnShow(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLinkClick(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
