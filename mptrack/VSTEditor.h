/*
 * VSTEditor.h
 * -----------
 * Purpose: Implementation of the custom plugin editor window that is used if a plugin provides an own editor GUI.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "AbstractVstEditor.h"

OPENMPT_NAMESPACE_BEGIN

#ifndef NO_VST

class COwnerVstEditor : public CAbstractVstEditor
{
protected:
	CStatic m_plugWindow;
	int m_width = 0, m_height = 0;

public:
	COwnerVstEditor(CVstPlugin &plugin) : CAbstractVstEditor(plugin) { }
	~COwnerVstEditor() override { }

	// Plugins may request to change the GUI size.
	bool IsResizable() const override { return true; }
	bool SetSize(int contentWidth, int contentHeight) override;

	void UpdateParamDisplays() override;

	bool OpenEditor(CWnd *parent) override;
	void DoClose() override;

protected:
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnPaint();

	LRESULT OnPreTranslateKeyDown(WPARAM wParam, LPARAM lParam) { return HandlePreTranslateMessage(WM_KEYDOWN, wParam, lParam); }
	LRESULT OnPreTranslateKeyUp(WPARAM wParam, LPARAM lParam) { return HandlePreTranslateMessage(WM_KEYUP, wParam, lParam); }
	LRESULT OnPreTranslateSysKeyDown(WPARAM wParam, LPARAM lParam) { return HandlePreTranslateMessage(WM_SYSKEYDOWN, wParam, lParam); }
	LRESULT OnPreTranslateSysKeyUp(WPARAM wParam, LPARAM lParam) { return HandlePreTranslateMessage(WM_SYSKEYUP, wParam, lParam); }
	LRESULT HandlePreTranslateMessage(UINT message, WPARAM wParam, LPARAM lParam)
	{
		MSG msg = {m_plugWindow, message, wParam, lParam, 0, {}};
		return HandleKeyMessage(msg);
	}

	DECLARE_MESSAGE_MAP()
};

#endif // NO_VST

OPENMPT_NAMESPACE_END
