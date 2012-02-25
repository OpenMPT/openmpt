/*
 * SelectPluginDialog.cpp
 * ----------------------
 * Purpose: Dialog for adding plugins to a song.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "SelectPluginDialog.h"


#ifndef NO_VST

/////////////////////////////////////////////////////////////////////////////////
// Plugin selection dialog


BEGIN_MESSAGE_MAP(CSelectPluginDlg, CDialog)
	ON_NOTIFY(TVN_SELCHANGED,	 IDC_TREE1, OnSelChanged)
	ON_NOTIFY(NM_DBLCLK,		 IDC_TREE1, OnSelDblClk)
	ON_COMMAND(IDC_BUTTON1,		 OnAddPlugin)
	ON_COMMAND(IDC_BUTTON2,		 OnRemovePlugin)
	ON_EN_CHANGE(IDC_NAMEFILTER, OnNameFilterChanged)
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()


void CSelectPluginDlg::DoDataExchange(CDataExchange* pDX)
//-------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE1, m_treePlugins);
	DDX_Text(pDX, IDC_NAMEFILTER, m_sNameFilter);
}


CSelectPluginDlg::CSelectPluginDlg(CModDoc *pModDoc, int nPlugSlot, CWnd *parent) : CDialog(IDD_SELECTMIXPLUGIN, parent)
//----------------------------------------------------------------------------------------------------------------------
{
	m_pPlugin = NULL;
	m_pModDoc = pModDoc;
	m_nPlugSlot = nPlugSlot;

	if (m_pModDoc)
	{
		CSoundFile* pSndFile = pModDoc->GetSoundFile();
		if (pSndFile && (0 <= m_nPlugSlot && m_nPlugSlot < MAX_MIXPLUGINS))
		{
			m_pPlugin = &pSndFile->m_MixPlugins[m_nPlugSlot];
		}
	}

	CMainFrame::GetMainFrame()->GetInputHandler()->Bypass(true);
}


CSelectPluginDlg::~CSelectPluginDlg()
//-----------------------------------
{
	CMainFrame::GetMainFrame()->GetInputHandler()->Bypass(false);
}


BOOL CSelectPluginDlg::OnInitDialog()
//-----------------------------------
{
	DWORD dwRemove = TVS_EDITLABELS|TVS_SINGLEEXPAND;
	DWORD dwAdd = TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;

	CDialog::OnInitDialog();
	m_treePlugins.ModifyStyle(dwRemove, dwAdd);
	m_treePlugins.SetImageList(CMainFrame::GetMainFrame()->GetImageList(), TVSIL_NORMAL);

	if (m_pPlugin)
	{
		CString targetSlot;
		targetSlot.Format("&Put in FX%02d", m_nPlugSlot + 1);
		SetDlgItemText(IDOK, targetSlot);
		::EnableWindow(::GetDlgItem(m_hWnd, IDOK), TRUE);
	} else
	{
		::EnableWindow(::GetDlgItem(m_hWnd, IDOK), FALSE);
	}

	MoveWindow(CMainFrame::GetSettings().gnPlugWindowX,
		CMainFrame::GetSettings().gnPlugWindowY,
		CMainFrame::GetSettings().gnPlugWindowWidth,
		CMainFrame::GetSettings().gnPlugWindowHeight);

	UpdatePluginsList();
	OnSelChanged(NULL, NULL);
	return TRUE;
}


void CSelectPluginDlg::OnOK()
//---------------------------
{
	// -> CODE#0002
	// -> DESC="list box to choose VST plugin presets (programs)"
	if(m_pPlugin==NULL) { CDialog::OnOK(); return; }
	// -! NEW_FEATURE#0002

	BOOL bChanged = FALSE;
	CVstPluginManager *pManager = theApp.GetPluginManager();
	PVSTPLUGINLIB pNewPlug = (PVSTPLUGINLIB)m_treePlugins.GetItemData(m_treePlugins.GetSelectedItem());
	PVSTPLUGINLIB pFactory = NULL;
	CVstPlugin *pCurrentPlugin = NULL;
	if (m_pPlugin) pCurrentPlugin = (CVstPlugin *)m_pPlugin->pMixPlugin;
	if ((pManager) && (pManager->IsValidPlugin(pNewPlug))) pFactory = pNewPlug;
	// Plugin selected
	if (pFactory)
	{
		if ((!pCurrentPlugin) || (pCurrentPlugin->GetPluginFactory() != pFactory))
		{
			CriticalSection cs;
			if (pCurrentPlugin) pCurrentPlugin->Release();
			// Just in case...
			m_pPlugin->pMixPlugin = NULL;
			m_pPlugin->pMixState = NULL;
			// Remove old state
			m_pPlugin->nPluginDataSize = 0;
			if (m_pPlugin->pPluginData) delete[] m_pPlugin->pPluginData;
			m_pPlugin->pPluginData = NULL;
			// Initialize plugin info
			MemsetZero(m_pPlugin->Info);
			m_pPlugin->Info.dwPluginId1 = pFactory->dwPluginId1;
			m_pPlugin->Info.dwPluginId2 = pFactory->dwPluginId2;

			switch(m_pPlugin->Info.dwPluginId2)
			{
				// Enable drymix by default for these known plugins
			case CCONST('S', 'c', 'o', 'p'):
				m_pPlugin->Info.dwInputRouting |= MIXPLUG_INPUTF_WETMIX;
				break;
			}

			lstrcpyn(m_pPlugin->Info.szName, pFactory->szLibraryName, 32);
			lstrcpyn(m_pPlugin->Info.szLibraryName, pFactory->szLibraryName, 64);

			cs.Leave();

			// Now, create the new plugin
			if (pManager)
			{
				pManager->CreateMixPlugin(m_pPlugin, (m_pModDoc) ? m_pModDoc->GetSoundFile() : 0);
				if (m_pPlugin->pMixPlugin)
				{
					CHAR s[128];
					CVstPlugin *p = (CVstPlugin *)m_pPlugin->pMixPlugin;
					s[0] = 0;
					if ((p->GetDefaultEffectName(s)) && (s[0]))
					{
						s[31] = 0;
						lstrcpyn(m_pPlugin->Info.szName, s, 32);
					}
				}
			}
			bChanged = TRUE;
		}
	} else
		// No effect
	{
		CriticalSection cs;
		if (pCurrentPlugin)
		{
			pCurrentPlugin->Release();
			bChanged = TRUE;
		}
		// Just in case...
		m_pPlugin->pMixPlugin = NULL;
		m_pPlugin->pMixState = NULL;
		// Remove old state
		m_pPlugin->nPluginDataSize = 0;
		if (m_pPlugin->pPluginData) delete[] m_pPlugin->pPluginData;
		m_pPlugin->pPluginData = NULL;
		// Clear plugin info
		MemsetZero(m_pPlugin->Info);
	}

	//remember window size:
	RECT rect;
	GetWindowRect(&rect);
	CMainFrame::GetSettings().gnPlugWindowX = rect.left;
	CMainFrame::GetSettings().gnPlugWindowY = rect.top;
	CMainFrame::GetSettings().gnPlugWindowWidth  = rect.right - rect.left;
	CMainFrame::GetSettings().gnPlugWindowHeight = rect.bottom - rect.top;

	if (bChanged)
	{
		CMainFrame::GetSettings().gnPlugWindowLast = m_pPlugin->Info.dwPluginId2;
		CDialog::OnOK();
	}
	else
	{
		CDialog::OnCancel();
	}
}


void CSelectPluginDlg::OnCancel()
//-------------------------------
{
	//remember window size:
	RECT rect;
	GetWindowRect(&rect);
	CMainFrame::GetSettings().gnPlugWindowX = rect.left;
	CMainFrame::GetSettings().gnPlugWindowY = rect.top;
	CMainFrame::GetSettings().gnPlugWindowWidth  = rect.right - rect.left;
	CMainFrame::GetSettings().gnPlugWindowHeight = rect.bottom - rect.top;

	CDialog::OnCancel();
}


void CSelectPluginDlg::OnNameFilterChanged() 
//------------------------------------------
{
	GetDlgItem(IDC_NAMEFILTER)->GetWindowText(m_sNameFilter);
	m_sNameFilter = m_sNameFilter.MakeLower();
	UpdatePluginsList();
}


void CSelectPluginDlg::UpdatePluginsList(DWORD forceSelect /* = 0*/)
//------------------------------------------------------------------
{
	CVstPluginManager *pManager = theApp.GetPluginManager();
	HTREEITEM cursel, hDmo, hVst, hSynth;

	m_treePlugins.SetRedraw(FALSE);
	m_treePlugins.DeleteAllItems();

	hSynth = AddTreeItem("VST Instruments", IMAGE_FOLDER, false);
	hDmo = AddTreeItem("DirectX Media Audio Effects", IMAGE_FOLDER, false);
	hVst = AddTreeItem("VST Audio Effects", IMAGE_FOLDER, false);
	cursel = AddTreeItem("No plugin (empty slot)", IMAGE_NOPLUGIN, false);

	if (pManager)
	{
		PVSTPLUGINLIB pCurrent = NULL;
		PVSTPLUGINLIB p = pManager->GetFirstPlugin();
		while (p)
		{
			// Apply name filter
			if (m_sNameFilter != "")
			{
				CString displayName = p->szLibraryName;
				if (displayName.MakeLower().Find(m_sNameFilter) == -1)
				{
					p = p->pNext;
					continue;
				}
			}

			HTREEITEM hParent;
			if (p->dwPluginId1 == kDmoMagic)
			{
				hParent = hDmo;
			} else
			{
				hParent = (p->bIsInstrument) ? hSynth : hVst;
			}

			HTREEITEM h = AddTreeItem(p->szLibraryName, p->bIsInstrument ? IMAGE_PLUGININSTRUMENT : IMAGE_EFFECTPLUGIN, true, hParent, (LPARAM)p);

			//If filter is active, expand nodes.
			if (m_sNameFilter != "") m_treePlugins.EnsureVisible(h);

			//Which plugin should be selected?
			if (m_pPlugin)
			{

				//forced selection (e.g. just after add plugin)
				if (forceSelect != 0)
				{
					if (p->dwPluginId2 == forceSelect)
					{
						pCurrent = p;
					}
				}

				//Current slot's plugin
				else if (m_pPlugin->pMixPlugin)
				{
					CVstPlugin *pVstPlug = (CVstPlugin *)m_pPlugin->pMixPlugin;
					if (pVstPlug->GetPluginFactory() == p) pCurrent = p;
				} 

				//Plugin with matching ID to current slot's plug
				else if (/* (!pCurrent) && */ m_pPlugin->Info.dwPluginId1 !=0 || m_pPlugin->Info.dwPluginId2 != 0)
				{
					if ((p->dwPluginId1 == m_pPlugin->Info.dwPluginId1)
						&& (p->dwPluginId2 == m_pPlugin->Info.dwPluginId2))
					{
							pCurrent = p;
					}
				}

				//Last selected plugin
				else
				{
					if (p->dwPluginId2 == CMainFrame::GetSettings().gnPlugWindowLast)
					{
						pCurrent = p;
					}
				}
			}
			if (pCurrent == p) cursel = h;
			p = p->pNext;
		}
	}
	m_treePlugins.SetRedraw(TRUE);
	if (cursel)
	{
		m_treePlugins.SelectItem(cursel);
		m_treePlugins.SetItemState(cursel, TVIS_BOLD, TVIS_BOLD);
		m_treePlugins.EnsureVisible(cursel);
	}
}

HTREEITEM CSelectPluginDlg::AddTreeItem(LPSTR szTitle, int iImage, bool bSort, HTREEITEM hParent, LPARAM lParam)
//--------------------------------------------------------------------------------------------------------------
{
	TVINSERTSTRUCT tvis;
	MemsetZero(tvis);

	tvis.hParent = hParent;
	tvis.hInsertAfter = (bSort) ? TVI_SORT : TVI_FIRST;
	tvis.item.mask = TVIF_IMAGE | TVIF_PARAM | TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	tvis.item.pszText = szTitle;
	tvis.item.iImage = tvis.item.iSelectedImage = iImage;
	tvis.item.lParam = lParam;
	return m_treePlugins.InsertItem(&tvis);
}


void CSelectPluginDlg::OnSelDblClk(NMHDR *, LRESULT *result)
//----------------------------------------------------------
{
	// -> CODE#0002
	// -> DESC="list box to choose VST plugin presets (programs)"
	if(m_pPlugin == NULL) return;
	// -! NEW_FEATURE#0002

	HTREEITEM hSel = m_treePlugins.GetSelectedItem();
	int nImage, nSelectedImage;
	m_treePlugins.GetItemImage(hSel, nImage, nSelectedImage);

	if ((hSel) && (nImage != IMAGE_FOLDER)) OnOK();
	if (result) *result = 0;
}


void CSelectPluginDlg::OnSelChanged(NMHDR *, LRESULT *result)
//-----------------------------------------------------------
{
	CVstPluginManager *pManager = theApp.GetPluginManager();
	PVSTPLUGINLIB pPlug = (PVSTPLUGINLIB)m_treePlugins.GetItemData(m_treePlugins.GetSelectedItem());
	if ((pManager) && (pManager->IsValidPlugin(pPlug)))
	{
		SetDlgItemText(IDC_TEXT_CURRENT_VSTPLUG, pPlug->szDllPath);
	} else
	{
		SetDlgItemText(IDC_TEXT_CURRENT_VSTPLUG, "");
	}
	if (result) *result = 0;
}


struct PROBLEMATIC_PLUG
{
	DWORD id1;
	DWORD id2;
	DWORD version;
	LPCSTR name;
	LPCSTR problem;
};

//TODO: Check whether the list is still valid.
static const PROBLEMATIC_PLUG gProblemPlugs[] =
{
	{ kEffectMagic, CCONST('N', 'i', '4', 'S'), 1, "Native Instruments B4", "*  v1.1.1 hangs on playback. Do not proceed unless you have v1.1.5 or newer.  *" },
	{ kEffectMagic, CCONST('m', 'd', 'a', 'C'), 1, "MDA Degrade", "*  Old versions of this plugin can crash OpenMPT.\nEnsure that you have the latest version of this plugin.  *" },
	{ kEffectMagic, CCONST('f', 'V', '2', 's'), 1, "Farbrausch V2", "*  This plugin can cause OpenMPT to freeze if being used in a combination with various other plugins.\nIt is recommended to not use V2 in combination with any other plugins.  *" },
	{ kEffectMagic, CCONST('f', 'r', 'V', '2'), 1, "Farbrausch V2", "*  This plugin can cause OpenMPT to freeze if being used in a combination with various other plugins.\nIt is recommended to not use V2 in combination with any other plugins.  *" },
};

bool CSelectPluginDlg::VerifyPlug(PVSTPLUGINLIB plug) 
//---------------------------------------------------
{
	CString s;
	for (size_t p = 0; p < CountOf(gProblemPlugs); p++)
	{
		if ( (gProblemPlugs[p].id2 == plug->dwPluginId2)
			/*&& (gProblemPlugs[p].id1 == plug->dwPluginId1)*/)
		{
			s.Format("WARNING: This plugin has been identified as %s,\nwhich is known to have the following problem with OpenMPT:\n\n%s\n\nWould you still like to add this plugin to the library?", gProblemPlugs[p].name, gProblemPlugs[p].problem);
			return (Reporting::Confirm(s) == cnfYes);
		}
	}

	return true;
}


void CSelectPluginDlg::OnAddPlugin()
//----------------------------------
{
	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "dll", "",
		"VST Plugins (*.dll)|*.dll||",
		CMainFrame::GetSettings().GetWorkingDirectory(DIR_PLUGINS),
		true);
	if(files.abort) return;

	CMainFrame::GetSettings().SetWorkingDirectory(files.workingDirectory.c_str(), DIR_PLUGINS, true);

	CVstPluginManager *pManager = theApp.GetPluginManager();
	bool bOk = false;

	PVSTPLUGINLIB plugLib = nullptr;
	for(size_t counter = 0; counter < files.filenames.size(); counter++)
	{

		CString sFilename = files.filenames[counter].c_str();

		if (pManager)
		{
			plugLib = pManager->AddPlugin(sFilename, FALSE);
			if (plugLib)
			{
				bOk = true;
				if(!VerifyPlug(plugLib))
				{
					pManager->RemovePlugin(plugLib);
				}
			}
		}
	}
	if (bOk)
	{
		// Force selection to last added plug.
		UpdatePluginsList(plugLib ? plugLib->dwPluginId2 : 0);
	} else
	{
		Reporting::Error("At least one selected file was not a valid VST Plugin.");
	}
}


void CSelectPluginDlg::OnRemovePlugin()
//-------------------------------------
{
	const HTREEITEM pluginToDelete = m_treePlugins.GetSelectedItem();
	PVSTPLUGINLIB pPlug = (PVSTPLUGINLIB)m_treePlugins.GetItemData(pluginToDelete);
	CVstPluginManager *pManager = theApp.GetPluginManager();

	if ((pManager) && (pPlug))
	{
		if(pManager->RemovePlugin(pPlug))
		{
			m_treePlugins.DeleteItem(pluginToDelete);
		}
	}
}


void CSelectPluginDlg::OnSize(UINT nType, int cx, int cy)
//-------------------------------------------------------
{
	CDialog::OnSize(nType, cx, cy);

	if (m_treePlugins)
	{
		m_treePlugins.MoveWindow(8, 36, cx - 104, cy - 63, FALSE);

		::MoveWindow(GetDlgItem(IDC_STATIC_VSTNAMEFILTER)->m_hWnd, 8, 11, 40, 21, FALSE);
		::MoveWindow(GetDlgItem(IDC_NAMEFILTER)->m_hWnd, 40, 8, cx - 136, 21, FALSE);

		::MoveWindow(GetDlgItem(IDC_TEXT_CURRENT_VSTPLUG)->m_hWnd, 8, cy - 20, cx - 22, 25, FALSE);   
		::MoveWindow(GetDlgItem(IDOK)->m_hWnd,			cx-85,	8,    75, 23, FALSE);
		::MoveWindow(GetDlgItem(IDCANCEL)->m_hWnd,		cx-85,	39,    75, 23, FALSE);
		::MoveWindow(GetDlgItem(IDC_BUTTON1)->m_hWnd ,	cx-85,	cy-80, 75, 23, FALSE);	
		::MoveWindow(GetDlgItem(IDC_BUTTON2)->m_hWnd,	cx-85,	cy-52, 75, 23, FALSE);
		Invalidate();
	}
}

void CSelectPluginDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
//-------------------------------------------------------
{
	lpMMI->ptMinTrackSize.x = 300;
	lpMMI->ptMinTrackSize.y = 270;
	CDialog::OnGetMinMaxInfo(lpMMI);
}

#endif // NO_VST
