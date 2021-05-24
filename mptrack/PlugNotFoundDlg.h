/*
 * PlugNotFoundDlg.h
 * -----------------
 * Purpose: Dialog for handling missing plugins
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "ResizableDialog.h"
#include "CListCtrl.h"

OPENMPT_NAMESPACE_BEGIN

struct VSTPluginLib;

class PlugNotFoundDialog : public ResizableDialog
{
	std::vector<VSTPluginLib *> &m_plugins;
	CListCtrlEx m_List;

public:
	PlugNotFoundDialog(std::vector<VSTPluginLib *> &plugins, CWnd *parent);

	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;

	afx_msg void OnRemove();

	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END
