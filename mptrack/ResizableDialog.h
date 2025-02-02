/*
 * ResizableDialog.h
 * -----------------
 * Purpose: A wrapper for resizable MFC dialogs that fixes various issues.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "DialogBase.h"

OPENMPT_NAMESPACE_BEGIN

class ResizableDialog : public DialogBase
{
public:
	struct DynamicItem
	{
		HWND hwnd;
		CPoint initialPoint;
		CSize initialSize;
		CMFCDynamicLayout::MoveSettings moveSettings;
		CMFCDynamicLayout::SizeSettings sizeSettings;
	};

	ResizableDialog() = default;
	explicit ResizableDialog(UINT nIDTemplate, CWnd *pParentWnd = nullptr);

protected:
	BOOL OnInitDialog() override;
	void PostNcDestroy() override;
	void ResizeDynamicLayout() override;

	afx_msg void OnGetMinMaxInfo(MINMAXINFO *mmi);

	void EnableAutoLayout(bool enable) { m_enableAutoLayout = enable; }

	DECLARE_MESSAGE_MAP()

private:
	void LoadDynamicLayout(const TCHAR *lpszResourceName);
	CRect AdjustItemRect(const DynamicItem &item, const CSize windowSize) const;

	std::vector<DynamicItem> m_dynamicItems;
	CPoint m_minSize;
	CSize m_originalClientSize;
	int m_originalDPI = 0;
	bool m_enableAutoLayout = true;
};

OPENMPT_NAMESPACE_END
