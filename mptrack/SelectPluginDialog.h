/*
 * SelectPluginDialog.h
 * --------------------
 * Purpose: Dialog for adding plugins to a song.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"
#include "ResizableDialog.h"
#include "../common/ComponentManager.h"

OPENMPT_NAMESPACE_BEGIN

class CModDoc;
struct SNDMIXPLUGIN;
struct VSTPluginLib;
class ComponentPluginBridge_x86;
class ComponentPluginBridgeLegacy_x86;
class ComponentPluginBridge_amd64;
class ComponentPluginBridgeLegacy_amd64;
#if defined(MPT_WITH_WINDOWS10)
class ComponentPluginBridge_arm;
class ComponentPluginBridgeLegacy_arm;
class ComponentPluginBridge_arm64;
class ComponentPluginBridgeLegacy_arm64;
#endif  // MPT_WITH_WINDOWS10

class CSelectPluginDlg : public ResizableDialog
{
protected:
	SNDMIXPLUGIN *m_pPlugin = nullptr;
	CModDoc *m_pModDoc = nullptr;
	CTreeCtrl m_treePlugins;
	CButton m_chkBridge;
	CButton m_chkShare;
	CButton m_chkLegacyBridge;
	mpt::ustring m_nameFilter;
#ifndef NO_VST
	ComponentHandle<ComponentPluginBridge_x86> pluginBridge_x86;
	ComponentHandle<ComponentPluginBridgeLegacy_x86> pluginBridgeLegacy_x86;
	ComponentHandle<ComponentPluginBridge_amd64> pluginBridge_amd64;
	ComponentHandle<ComponentPluginBridgeLegacy_amd64> pluginBridgeLegacy_amd64;
#if defined(MPT_WITH_WINDOWS10)
	ComponentHandle<ComponentPluginBridge_arm> pluginBridge_arm;
	ComponentHandle<ComponentPluginBridgeLegacy_arm> pluginBridgeLegacy_arm;
	ComponentHandle<ComponentPluginBridge_arm64> pluginBridge_arm64;
	ComponentHandle<ComponentPluginBridgeLegacy_arm64> pluginBridgeLegacy_arm64;
#endif  // MPT_WITH_WINDOWS10
#endif
	PLUGINDEX m_nPlugSlot = 0;

public:
	CSelectPluginDlg(CModDoc *pModDoc, PLUGINDEX pluginSlot, CWnd *parent);
	~CSelectPluginDlg();

	static VSTPluginLib *ScanPlugins(const mpt::PathString &path, CWnd *parent);
	static bool VerifyPlugin(VSTPluginLib *plug, CWnd *parent);

protected:
	HTREEITEM AddTreeItem(const TCHAR *title, int image, bool sort, HTREEITEM hParent = TVI_ROOT, LPARAM lParam = NULL);

	VSTPluginLib *GetSelectedPlugin();
	void SaveWindowPos() const;

	void ReloadMissingPlugins(const VSTPluginLib *lib) const;

	void UpdatePluginsList(const VSTPluginLib *forceSelect = nullptr);

	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;
	void OnCancel() override;
	BOOL PreTranslateMessage(MSG *pMsg) override;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnAddPlugin();
	afx_msg void OnScanFolder();
	afx_msg void OnRemovePlugin();
	afx_msg void OnNameFilterChanged();
	afx_msg void OnSetBridge();
	afx_msg void OnSelChanged(NMHDR *pNotifyStruct, LRESULT *result);
	afx_msg void OnSelDblClk(NMHDR *pNotifyStruct, LRESULT *result);
	afx_msg void OnPluginTagsChanged();
};

OPENMPT_NAMESPACE_END
