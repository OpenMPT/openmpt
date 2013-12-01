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
#include "../common/StringFixer.h"
#include "FileDialog.h"


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
}


CSelectPluginDlg::CSelectPluginDlg(CModDoc *pModDoc, int nPlugSlot, CWnd *parent) : CDialog(IDD_SELECTMIXPLUGIN, parent)
//----------------------------------------------------------------------------------------------------------------------
{
	m_pPlugin = NULL;
	m_pModDoc = pModDoc;
	m_nPlugSlot = nPlugSlot;

	if(m_pModDoc)
	{
		if(0 <= m_nPlugSlot && m_nPlugSlot < MAX_MIXPLUGINS)
		{
			m_pPlugin = &(pModDoc->GetrSoundFile().m_MixPlugins[m_nPlugSlot]);
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

	MoveWindow(TrackerSettings::Instance().gnPlugWindowX,
		TrackerSettings::Instance().gnPlugWindowY,
		TrackerSettings::Instance().gnPlugWindowWidth,
		TrackerSettings::Instance().gnPlugWindowHeight);

	UpdatePluginsList();
	OnSelChanged(NULL, NULL);
	return TRUE;
}


void CSelectPluginDlg::OnOK()
//---------------------------
{
	if(m_pPlugin==nullptr) { CDialog::OnOK(); return; }

	bool changed = false;
	CVstPluginManager *pManager = theApp.GetPluginManager();
	VSTPluginLib *pNewPlug = (VSTPluginLib *)m_treePlugins.GetItemData(m_treePlugins.GetSelectedItem());
	VSTPluginLib *pFactory = nullptr;
	CVstPlugin *pCurrentPlugin = nullptr;
	if (m_pPlugin) pCurrentPlugin = dynamic_cast<CVstPlugin *>(m_pPlugin->pMixPlugin);
	if ((pManager) && (pManager->IsValidPlugin(pNewPlug))) pFactory = pNewPlug;

	if (pFactory)
	{
		// Plugin selected
		if ((!pCurrentPlugin) || &pCurrentPlugin->GetPluginFactory() != pFactory)
		{
			CriticalSection cs;

			if (pCurrentPlugin != nullptr)
			{
				pCurrentPlugin->Release();
			}

			// Just in case...
			m_pPlugin->pMixPlugin = nullptr;
			m_pPlugin->pMixState = nullptr;

			// Remove old state
			m_pPlugin->nPluginDataSize = 0;
			if (m_pPlugin->pPluginData) delete[] m_pPlugin->pPluginData;
			m_pPlugin->pPluginData = nullptr;

			// Initialize plugin info
			MemsetZero(m_pPlugin->Info);
			m_pPlugin->Info.dwPluginId1 = pFactory->pluginId1;
			m_pPlugin->Info.dwPluginId2 = pFactory->pluginId2;

			switch(m_pPlugin->Info.dwPluginId2)
			{
				// Enable drymix by default for these known plugins
			case CCONST('S', 'c', 'o', 'p'):
				m_pPlugin->SetWetMix();
				break;
			}

			mpt::String::Copy(m_pPlugin->Info.szName, pFactory->libraryName.ToLocale().c_str());
			mpt::String::Copy(m_pPlugin->Info.szLibraryName, pFactory->libraryName.ToUTF8().c_str());

			cs.Leave();

			// Now, create the new plugin
			if(pManager && m_pModDoc)
			{
				pManager->CreateMixPlugin(*m_pPlugin, m_pModDoc->GetrSoundFile());
				if (m_pPlugin->pMixPlugin)
				{
					CHAR s[128];
					CVstPlugin *p = (CVstPlugin *)m_pPlugin->pMixPlugin;
					s[0] = 0;
					if ((p->GetDefaultEffectName(s)) && (s[0]))
					{
						mpt::String::Copy(m_pPlugin->Info.szName, s);
					}
				}
			}
			changed = true;
		}
	} else
	{
		// No effect
		CriticalSection cs;
		if (pCurrentPlugin)
		{
			pCurrentPlugin->Release();
			changed = true;
		}

		// Just in case...
		m_pPlugin->pMixPlugin = nullptr;
		m_pPlugin->pMixState = nullptr;

		// Remove old state
		m_pPlugin->nPluginDataSize = 0;
		if (m_pPlugin->pPluginData) delete[] m_pPlugin->pPluginData;
		m_pPlugin->pPluginData = nullptr;

		// Clear plugin info
		MemsetZero(m_pPlugin->Info);
	}

	//remember window size:
	RECT rect;
	GetWindowRect(&rect);
	TrackerSettings::Instance().gnPlugWindowX = rect.left;
	TrackerSettings::Instance().gnPlugWindowY = rect.top;
	TrackerSettings::Instance().gnPlugWindowWidth  = rect.right - rect.left;
	TrackerSettings::Instance().gnPlugWindowHeight = rect.bottom - rect.top;

	if (changed)
	{
		if(m_pPlugin->Info.dwPluginId2)
			TrackerSettings::Instance().gnPlugWindowLast = m_pPlugin->Info.dwPluginId2;
		CDialog::OnOK();
	} else
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
	TrackerSettings::Instance().gnPlugWindowX = rect.left;
	TrackerSettings::Instance().gnPlugWindowY = rect.top;
	TrackerSettings::Instance().gnPlugWindowWidth  = rect.right - rect.left;
	TrackerSettings::Instance().gnPlugWindowHeight = rect.bottom - rect.top;

	CDialog::OnCancel();
}


BOOL CSelectPluginDlg::PreTranslateMessage(MSG *pMsg)
//---------------------------------------------------
{
	// Use up/down keys to navigate in tree view, even if search field is focussed.
	if(pMsg != nullptr && pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN) && GetFocus() != &m_treePlugins)
	{
		HTREEITEM selItem = m_treePlugins.GetSelectedItem();
		if(selItem == nullptr)
		{
			selItem = m_treePlugins.GetRootItem();
		}
		while((selItem = m_treePlugins.GetNextItem(selItem, pMsg->wParam == VK_UP ? TVGN_PREVIOUSVISIBLE : TVGN_NEXTVISIBLE)) != nullptr)
		{
			int nImage, nSelectedImage;
			m_treePlugins.GetItemImage(selItem, nImage, nSelectedImage);
			if(nImage != IMAGE_FOLDER)
			{
				m_treePlugins.SelectItem(selItem);
				m_treePlugins.EnsureVisible(selItem);
				return TRUE;
			}
		}
		return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}


void CSelectPluginDlg::OnNameFilterChanged()
//------------------------------------------
{
	// Update name filter text
	HWND hwnd = GetDlgItem(IDC_NAMEFILTER)->m_hWnd;
	int len = GetWindowTextLengthW(hwnd);
	m_nameFilter.resize(len);
	GetWindowTextW(hwnd, &m_nameFilter[0], len + 1);
	for(int i = 0; i < len; i++) m_nameFilter[i] = ::towlower(m_nameFilter[i]);

	UpdatePluginsList();
}


void CSelectPluginDlg::UpdatePluginsList(VstInt32 forceSelect /* = 0*/)
//---------------------------------------------------------------------
{
	CVstPluginManager *pManager = theApp.GetPluginManager();

	m_treePlugins.SetRedraw(FALSE);
	m_treePlugins.DeleteAllItems();

	static const struct
	{
		VSTPluginLib::PluginCategory category;
		const WCHAR *description;
	} categories[] =
	{
		{ VSTPluginLib::catEffect,			L"Audio Effects" },
		{ VSTPluginLib::catGenerator,		L"Tone Generators" },
		{ VSTPluginLib::catRestoration,		L"Audio Restauration" },
		{ VSTPluginLib::catSurroundFx,		L"Surround Effects" },
		{ VSTPluginLib::catRoomFx,			L"Room Effects" },
		{ VSTPluginLib::catSpacializer,		L"Spacializers" },
		{ VSTPluginLib::catMastering,		L"Mastering Plugins" },
		{ VSTPluginLib::catAnalysis,		L"Analysis Plugins" },
		{ VSTPluginLib::catOfflineProcess,	L"Offline Processing" },
		{ VSTPluginLib::catShell,			L"Shell Plugins" },
		{ VSTPluginLib::catUnknown,			L"Unsorted" },
		{ VSTPluginLib::catDMO,				L"DirectX Media Audio Effects" },
		{ VSTPluginLib::catSynth,			L"Instrument Plugins" },
	};

	std::bitset<VSTPluginLib::numCategories> categoryUsed;
	HTREEITEM categoryFolders[VSTPluginLib::numCategories];
	for(size_t i = CountOf(categories); i != 0; )
	{
		i--;
		categoryFolders[categories[i].category] = AddTreeItem(categories[i].description, IMAGE_FOLDER, false);
	}

	HTREEITEM noPlug = AddTreeItem(L"No plugin (empty slot)", IMAGE_NOPLUGIN, false);
	HTREEITEM currentPlug = noPlug;
	bool foundCurrentPlug = false;

	const bool nameFilterActive = !m_nameFilter.empty();
	if(pManager)
	{
		bool first = true;

		for(CVstPluginManager::const_iterator p = pManager->begin(); p != pManager->end(); p++)
		{
			ASSERT(*p);
			const VSTPluginLib &plug = **p;
			if(nameFilterActive)
			{
				// Apply name filter
				std::wstring displayName = plug.libraryName.ToWide();
				for(size_t i = 0; i < displayName.length(); i++) displayName[i] = ::towlower(displayName[i]);
				if(displayName.find(m_nameFilter, 0) == displayName.npos)
				{
					continue;
				}
			}

			HTREEITEM h = AddTreeItem(plug.libraryName.AsNative().c_str(), plug.isInstrument ? IMAGE_PLUGININSTRUMENT : IMAGE_EFFECTPLUGIN, true, categoryFolders[plug.category], reinterpret_cast<LPARAM>(&plug));
			categoryUsed[plug.category] = true;

			if(nameFilterActive)
			{
				// If filter is active, expand nodes.
				m_treePlugins.EnsureVisible(h);
				if(first)
				{
					first = false;
					m_treePlugins.SelectItem(h);
				}
			}

			if(m_pPlugin && !foundCurrentPlug)
			{
				//Which plugin should be selected?

				if(forceSelect != 0 && plug.pluginId2 == forceSelect)
				{
					//forced selection (e.g. just after add plugin)
					currentPlug = h;
				} else if(m_pPlugin->pMixPlugin)
				{
					//Current slot's plugin
					CVstPlugin *pVstPlug = (CVstPlugin *)m_pPlugin->pMixPlugin;
					if (&pVstPlug->GetPluginFactory() == &plug)
					{
						currentPlug = h;
					}
				} else if(m_pPlugin->Info.dwPluginId1 != 0 || m_pPlugin->Info.dwPluginId2 != 0)
				{
					//Plugin with matching ID to current slot's plug
					if(plug.pluginId1 == m_pPlugin->Info.dwPluginId1
						&& plug.pluginId2 == m_pPlugin->Info.dwPluginId2)
					{
						currentPlug = h;
					}
				} else if(plug.pluginId2 == TrackerSettings::Instance().gnPlugWindowLast)
				{
					// Previously selected plugin
					currentPlug = h;
				}

				if(currentPlug == h)
				{
					foundCurrentPlug = true;
				}
			}
		}
	}

	// Remove empty categories
	for(size_t i = 0; i < CountOf(categoryFolders); i++)
	{
		if(!categoryUsed[i])
		{
			m_treePlugins.DeleteItem(categoryFolders[i]);
		}
	}

	m_treePlugins.SetRedraw(TRUE);

	if(!nameFilterActive || currentPlug != noPlug)
	{
		m_treePlugins.SelectItem(currentPlug);
	}
	m_treePlugins.SetItemState(currentPlug, TVIS_BOLD, TVIS_BOLD);
	m_treePlugins.EnsureVisible(currentPlug);
}


HTREEITEM CSelectPluginDlg::AddTreeItem(const WCHAR *title, int image, bool sort, HTREEITEM hParent, LPARAM lParam)
//-----------------------------------------------------------------------------------------------------------------
{
	return m_treePlugins.InsertItem(
		TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT,
		title,
		image, image,
		0, 0,
		lParam,
		hParent,
		(sort ? TVI_SORT : TVI_FIRST));
}


void CSelectPluginDlg::OnSelDblClk(NMHDR *, LRESULT *result)
//----------------------------------------------------------
{
	if(m_pPlugin == nullptr) return;

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
	VSTPluginLib *pPlug = (VSTPluginLib *)m_treePlugins.GetItemData(m_treePlugins.GetSelectedItem());
	if ((pManager) && (pManager->IsValidPlugin(pPlug)))
	{
		SetDlgItemTextW(m_hWnd, IDC_TEXT_CURRENT_VSTPLUG, pPlug->dllPath.ToWide().c_str());
	} else
	{
		SetDlgItemText(IDC_TEXT_CURRENT_VSTPLUG, "");
	}
	if (result) *result = 0;
}


bool CSelectPluginDlg::VerifyPlug(const VSTPluginLib *plug)
//---------------------------------------------------------
{
	// TODO: Keep this list up-to-date.
	static const struct
	{
		VstInt32 id1;
		VstInt32 id2;
		char *name;
		char *problem;
	} problemPlugs[] =
	{
		{ kEffectMagic, CCONST('N', 'i', '4', 'S'), "Native Instruments B4", "*  v1.1.1 hangs on playback. Do not proceed unless you have v1.1.5 or newer.  *" },
		{ kEffectMagic, CCONST('m', 'd', 'a', 'C'), "MDA Degrade", "*  Old versions of this plugin can crash OpenMPT.\nEnsure that you have the latest version of this plugin.  *" },
		{ kEffectMagic, CCONST('f', 'V', '2', 's'), "Farbrausch V2", "*  This plugin can cause OpenMPT to freeze if being used in a combination with various other plugins.\nIt is recommended to not use V2 in combination with any other plugins.  *" },
		{ kEffectMagic, CCONST('f', 'r', 'V', '2'), "Farbrausch V2", "*  This plugin can cause OpenMPT to freeze if being used in a combination with various other plugins.\nIt is recommended to not use V2 in combination with any other plugins.  *" },
	};

	for(size_t p = 0; p < CountOf(problemPlugs); p++)
	{
		if(problemPlugs[p].id2 == plug->pluginId2 /*&& gProblemPlugs[p].id1 == plug->dwPluginId1*/)
		{
			CString s;
			s.Format("WARNING: This plugin has been identified as %s,\nwhich is known to have the following problem with OpenMPT:\n\n%s\n\nWould you still like to add this plugin to the library?", problemPlugs[p].name, problemPlugs[p].problem);
			return (Reporting::Confirm(s) == cnfYes);
		}
	}

	return true;
}


void CSelectPluginDlg::OnAddPlugin()
//----------------------------------
{
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.DefaultExtension("dll")
		.ExtensionFilter("VST Plugins (*.dll)|*.dll||")
		.WorkingDirectory(TrackerDirectories::Instance().GetWorkingDirectory(DIR_PLUGINS));
	if(!dlg.Show(this)) return;

	TrackerDirectories::Instance().SetWorkingDirectory(dlg.GetWorkingDirectory(), DIR_PLUGINS, true);

	CVstPluginManager *pManager = theApp.GetPluginManager();
	bool bOk = false;

	VSTPluginLib *plugLib = nullptr;
	const FileDialog::PathList &files = dlg.GetFilenames();
	for(size_t counter = 0; counter < files.size(); counter++)
	{
		if (pManager)
		{
			plugLib = pManager->AddPlugin(files[counter], false);
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
		UpdatePluginsList(plugLib ? plugLib->pluginId2 : 0);
	} else
	{
		Reporting::Error("At least one selected file was not a valid VST Plugin.");
	}
}


void CSelectPluginDlg::OnRemovePlugin()
//-------------------------------------
{
	const HTREEITEM pluginToDelete = m_treePlugins.GetSelectedItem();
	VSTPluginLib *pPlug = (VSTPluginLib *)m_treePlugins.GetItemData(pluginToDelete);
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
