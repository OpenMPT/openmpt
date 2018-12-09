/*
 * WelcomeDialog.cpp
 * -----------------
 * Purpose: "First run" OpenMPT welcome dialog
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "../common/mptPathString.h"

OPENMPT_NAMESPACE_BEGIN

class WelcomeDlg : public CDialog
{
protected:
	mpt::PathString m_vstPath;

public:
	WelcomeDlg(CWnd *parent);

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	void OnCancel() override;
	void PostNcDestroy() override { CDialog::PostNcDestroy(); delete this; }

	afx_msg void OnOptions();
	afx_msg void OnScanPlugins();

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
