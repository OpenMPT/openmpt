/*
 * ResizableDialog.h
 * -----------------
 * Purpose: A wrapper for resizable MFC dialogs that fixes the dialog's minimum size
 *          (as MFC does not scale controls properly if the user makes the dialog smaller than it originally was)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

class ResizableDialog : public CDialog
{
private:
	CPoint m_minSize;

public:
	ResizableDialog() = default;
	explicit ResizableDialog(UINT nIDTemplate, CWnd *pParentWnd = nullptr);

protected:
	BOOL OnInitDialog() override;
	afx_msg void OnGetMinMaxInfo(MINMAXINFO *mmi);
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
