/*
 * VSTEditor.h
 * -----------
 * Purpose: Implementation of the custom plugin editor window that is used if a plugin provides an own editor GUI.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "AbstractVstEditor.h"

OPENMPT_NAMESPACE_BEGIN

#ifndef NO_VST

class COwnerVstEditor: public CAbstractVstEditor
{
protected:
	CStatic m_plugWindow;
	int m_width, m_height;

public:
	COwnerVstEditor(CVstPlugin &plugin);
	virtual ~COwnerVstEditor() { };

	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnPaint();

	// Plugins may request to change the GUI size.
	bool IsResizable() const override { return true; };
	bool SetSize(int contentWidth, int contentHeight) override;

	void UpdateParamDisplays() override;

	bool OpenEditor(CWnd *parent) override;
	void DoClose() override;
};

#endif // NO_VST

OPENMPT_NAMESPACE_END
