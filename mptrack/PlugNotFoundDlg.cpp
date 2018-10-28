/*
 * PlugNotFoundDlg.cpp
 * -------------------
 * Purpose: Dialog for handling missing plugins
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "PlugNotFoundDlg.h"
#include "../soundlib/plugins/PluginManager.h"
#include "Mptrack.h"
#include "resource.h"

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(PlugNotFoundDialog, ResizableDialog)
	ON_COMMAND(IDC_BUTTON_REMOVE, &PlugNotFoundDialog::OnRemove)
END_MESSAGE_MAP()


void PlugNotFoundDialog::DoDataExchange(CDataExchange *pDX)
{
	ResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_List);
}


PlugNotFoundDialog::PlugNotFoundDialog(std::vector<VSTPluginLib *> &plugins, CWnd *parent)
	: ResizableDialog(IDD_MISSINGPLUGS, parent)
	, m_plugins(plugins)
{
}


BOOL PlugNotFoundDialog::OnInitDialog()
{
	ResizableDialog::OnInitDialog();

	// Initialize table
	const CListCtrlEx::Header headers[] =
	{
		{ _T("Plugin"),		128, LVCFMT_LEFT },
		{ _T("File Path"),	308, LVCFMT_LEFT },
	};
	m_List.SetHeaders(headers);
	m_List.SetExtendedStyle(m_List.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

	for(const auto &plug : m_plugins)
	{
		const int insertAt = m_List.InsertItem(m_List.GetItemCount(), plug->libraryName.AsNative().c_str());
		if(insertAt == -1)
			continue;
		m_List.SetItemText(insertAt, 1, plug->dllPath.AsNative().c_str());
		m_List.SetItemDataPtr(insertAt, plug);
		m_List.SetCheck(insertAt);
	}
	m_List.SetFocus();

	return TRUE;
}


void PlugNotFoundDialog::OnRemove()
{
	const int plugs = m_List.GetItemCount();
	for(int i = 0; i < plugs; i++)
	{
		if(m_List.GetCheck(i))
		{
			theApp.GetPluginManager()->RemovePlugin(static_cast<VSTPluginLib *>(m_List.GetItemDataPtr(i)));
		}
	}
	OnOK();
}

OPENMPT_NAMESPACE_END
