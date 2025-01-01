/*
 * DialogBase.h
 * ------------
 * Purpose: Base class for dialogs that adds functionality on top of CDialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

class DialogBase : public CDialog
{
public:
	using CDialog::CDialog;

	BOOL OnInitDialog() override;
	BOOL PreTranslateMessage(MSG *pMsg) override;
	INT_PTR OnToolHitTest(CPoint point, TOOLINFO *pTI) const override;

	static bool HandleGlobalKeyMessage(const MSG &msg);

	int GetDPI() const { return m_dpi; }

protected:
	virtual void OnDPIChanged() {}
	void UpdateDPI();

	virtual CString GetToolTipText(UINT /*id*/, HWND /*hwnd*/) const { return {}; }

	afx_msg LRESULT OnDPIChanged(WPARAM, LPARAM);
	afx_msg BOOL OnToolTipText(UINT, NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

private:
	CString m_tooltipText;
	int m_dpi = 0;
};

OPENMPT_NAMESPACE_END
