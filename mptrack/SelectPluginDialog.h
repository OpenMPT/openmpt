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

#include "CTreeCtrl.h"
#include "../common/ComponentManager.h"

OPENMPT_NAMESPACE_BEGIN

class CModDoc;
struct SNDMIXPLUGIN;
struct VSTPluginLib;
class ComponentPluginBridge32;
class ComponentPluginBridge64;

//====================================
class CSelectPluginDlg: public CDialog
//====================================
{
protected:
	SNDMIXPLUGIN *m_pPlugin;
	CModDoc *m_pModDoc;
	CTreeCtrlW m_treePlugins;
	CButton m_chkBridge, m_chkShare;
	mpt::ustring m_nameFilter;
#ifndef NO_VST
	ComponentHandle<ComponentPluginBridge32> pluginBridge32;
	ComponentHandle<ComponentPluginBridge64> pluginBridge64;
#endif
	PLUGINDEX m_nPlugSlot;

	HTREEITEM AddTreeItem(const WCHAR *title, int image, bool sort, HTREEITEM hParent = TVI_ROOT, LPARAM lParam = NULL);

public:
	CSelectPluginDlg(CModDoc *pModDoc, PLUGINDEX nPlugSlot, CWnd *parent);
	~CSelectPluginDlg();

	static VSTPluginLib *ScanPlugins(const mpt::PathString &path, CWnd *parent);

protected:
	VSTPluginLib *GetSelectedPlugin();
	void SaveWindowPos() const;

	void ReloadMissingPlugins(const VSTPluginLib *lib) const;

	void DoClose();
	void UpdatePluginsList(int32 forceSelect = 0);
	static bool VerifyPlug(VSTPluginLib *plug, CWnd *parent);
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG *pMsg);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnAddPlugin();
	afx_msg void OnScanFolder();
	afx_msg void OnRemovePlugin();
	afx_msg void OnNameFilterChanged();
	afx_msg void OnSetBridge();
	afx_msg void OnSelChanged(NMHDR *pNotifyStruct, LRESULT *result);
	afx_msg void OnSelDblClk(NMHDR *pNotifyStruct, LRESULT *result);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnPluginTagsChanged();
};

OPENMPT_NAMESPACE_END
