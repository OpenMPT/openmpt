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
#include "InputHandler.h"
#include "Moddoc.h"
#include "../common/StringFixer.h"
#include "FileDialog.h"
#include "Vstplug.h"
#include "SelectPluginDialog.h"
#include "../pluginBridge/BridgeWrapper.h"
#include "FolderScanner.h"


OPENMPT_NAMESPACE_BEGIN


#ifndef NO_VST

/////////////////////////////////////////////////////////////////////////////////
// Plugin selection dialog


BEGIN_MESSAGE_MAP(CSelectPluginDlg, CDialog)
	ON_NOTIFY(TVN_SELCHANGED,		IDC_TREE1, OnSelChanged)
	ON_NOTIFY(NM_DBLCLK,			IDC_TREE1, OnSelDblClk)
	ON_COMMAND(IDC_BUTTON1,			OnAddPlugin)
	ON_COMMAND(IDC_BUTTON3,			OnScanFolder)
	ON_COMMAND(IDC_BUTTON2,			OnRemovePlugin)
	ON_COMMAND(IDC_CHECK1,			OnSetBridge)
	ON_COMMAND(IDC_CHECK2,			OnSetBridge)
	ON_EN_CHANGE(IDC_NAMEFILTER,	OnNameFilterChanged)
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()


void CSelectPluginDlg::DoDataExchange(CDataExchange* pDX)
//-------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE1, m_treePlugins);
	DDX_Control(pDX, IDC_CHECK1, m_chkBridge);
	DDX_Control(pDX, IDC_CHECK2, m_chkShare);
}


CSelectPluginDlg::CSelectPluginDlg(CModDoc *pModDoc, int nPlugSlot, CWnd *parent) : CDialog(IDD_SELECTMIXPLUGIN, parent)
//----------------------------------------------------------------------------------------------------------------------
{
	m_pPlugin = NULL;
	m_pModDoc = pModDoc;
	m_nPlugSlot = nPlugSlot;

	hasBridge32 = Util::sdOs::IsPathFileAvailable(theApp.GetAppDirPath() + MPT_PATHSTRING("PluginBridge32.exe"), Util::sdOs::FileModeExists);
	hasBridge64 = Util::sdOs::IsPathFileAvailable(theApp.GetAppDirPath() + MPT_PATHSTRING("PluginBridge64.exe"), Util::sdOs::FileModeExists);

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
	m_treePlugins.SetImageList(&CMainFrame::GetMainFrame()->m_MiscIcons, TVSIL_NORMAL);

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
	VSTPluginLib *pNewPlug = GetSelectedPlugin();
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

			// Destroy old plugin, if there was one.
			m_pPlugin->Destroy();

			// Initialize plugin info
			MemsetZero(m_pPlugin->Info);
			m_pPlugin->Info.dwPluginId1 = pFactory->pluginId1;
			m_pPlugin->Info.dwPluginId2 = pFactory->pluginId2;
			m_pPlugin->editorX = m_pPlugin->editorY = int32_min;

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
				} else
				{
					MemsetZero(m_pPlugin->Info);
				}
			}
			changed = true;
		}
	} else if(m_pPlugin->IsValidPlugin())
	{
		// No effect
		CriticalSection cs;
		m_pPlugin->Destroy();
		// Clear plugin info
		MemsetZero(m_pPlugin->Info);
		changed = true;
	}

	//remember window size:
	RECT rect;
	GetWindowRect(&rect);
	TrackerSettings::Instance().gnPlugWindowX = rect.left;
	TrackerSettings::Instance().gnPlugWindowY = rect.top;
	TrackerSettings::Instance().gnPlugWindowWidth  = rect.right - rect.left;
	TrackerSettings::Instance().gnPlugWindowHeight = rect.bottom - rect.top;

	if(changed)
	{
		if(m_pPlugin->Info.dwPluginId2)
			TrackerSettings::Instance().gnPlugWindowLast = m_pPlugin->Info.dwPluginId2;
		if(m_pModDoc)
		{
			m_pModDoc->UpdateAllViews(nullptr, PluginHint(static_cast<PLUGINDEX>(m_nPlugSlot + 1)).Info().Names());
		}
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
	if(len)
	{
		GetWindowTextW(hwnd, &m_nameFilter[0], len + 1);
		for(int i = 0; i < len; i++) m_nameFilter[i] = ::towlower(m_nameFilter[i]);
	}

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

			if(forceSelect != 0 && plug.pluginId2 == forceSelect)
			{
				// Forced selection (e.g. just after add plugin)
				currentPlug = h;
				foundCurrentPlug = true;
			}

			if(m_pPlugin && !foundCurrentPlug)
			{
				//Which plugin should be selected?
				if(m_pPlugin->pMixPlugin)
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
	VSTPluginLib *pPlug = GetSelectedPlugin();
	int showBoxes = SW_HIDE;
	if ((pManager) && (pManager->IsValidPlugin(pPlug)))
	{
		SetDlgItemTextW(m_hWnd, IDC_TEXT_CURRENT_VSTPLUG, pPlug->dllPath.ToWide().c_str());
		if(pPlug->pluginId1 == kEffectMagic)
		{
			bool isBridgeAvailable = (hasBridge32 && pPlug->GetDllBits() == 32) || (hasBridge64 && pPlug->GetDllBits() == 64);
			if(TrackerSettings::Instance().bridgeAllPlugins || !isBridgeAvailable)
			{
				m_chkBridge.EnableWindow(FALSE);
				m_chkBridge.SetCheck(isBridgeAvailable ? BST_CHECKED : BST_UNCHECKED);
			} else
			{
				bool native = pPlug->IsNative();

				m_chkBridge.EnableWindow(native ? TRUE : FALSE);
				m_chkBridge.SetCheck((pPlug->useBridge || !native) ? BST_CHECKED : BST_UNCHECKED);
			}

			m_chkShare.SetCheck(pPlug->shareBridgeInstance ? BST_CHECKED : BST_UNCHECKED);
			m_chkShare.EnableWindow(m_chkBridge.GetCheck() != BST_UNCHECKED);

			showBoxes = SW_SHOW;
		}
	} else
	{
		SetDlgItemText(IDC_TEXT_CURRENT_VSTPLUG, "");
	}
	m_chkBridge.ShowWindow(showBoxes);
	m_chkShare.ShowWindow(showBoxes);
	if (result) *result = 0;
}


bool CSelectPluginDlg::VerifyPlug(VSTPluginLib *plug)
//---------------------------------------------------
{
	// TODO: Keep these lists up-to-date.
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
		{ kEffectMagic, CCONST('f', 'V', '2', 's'), "Farbrausch V2", "*  This plugin can cause OpenMPT to freeze if being used in a combination with various other plugins.\nIt is recommended to not use V2 in combination with any other plugins or use it brigded mode only.  *" },
		{ kEffectMagic, CCONST('f', 'r', 'V', '2'), "Farbrausch V2", "*  This plugin can cause OpenMPT to freeze if being used in a combination with various other plugins.\nIt is recommended to not use V2 in combination with any other plugins or use it brigded mode only.  *" },
	};

	// Plugins that should always be bridged.
	static const struct
	{
		VstInt32 id1;
		VstInt32 id2;
		bool useBridge;
		bool shareInstance;
	} bridgedPlugs[] =
	{
		{ kEffectMagic, CCONST('f', 'V', '2', 's'), true, true },	// Single instances of V2 can communicate (I think)
		{ kEffectMagic, CCONST('f', 'r', 'V', '2'), true, false },
		{ kEffectMagic, CCONST('S', 'K', 'V', '3'), false, true },	// SideKick v3 always has to run in a shared instance
		{ kEffectMagic, CCONST('Y', 'W', 'S', '!'), false, true },	// You Wa Shock ! always has to run in a shared instance
	};

	for(size_t p = 0; p < CountOf(problemPlugs); p++)
	{
		if(problemPlugs[p].id2 == plug->pluginId2 && problemPlugs[p].id1 == plug->pluginId1)
		{
			CString s;
			s.Format("WARNING: This plugin has been identified as %s,\nwhich is known to have the following problem with OpenMPT:\n\n%s\n\nWould you still like to add this plugin to the library?", problemPlugs[p].name, problemPlugs[p].problem);
			if(Reporting::Confirm(s, false, false, this) == cnfNo)
			{
				return false;
			}
			break;
		}
	}

	for(size_t p = 0; p < CountOf(bridgedPlugs); p++)
	{
		if(bridgedPlugs[p].id2 == plug->pluginId2 && bridgedPlugs[p].id1 == plug->pluginId1)
		{
			plug->useBridge = bridgedPlugs[p].useBridge;
			plug->shareBridgeInstance = bridgedPlugs[p].shareInstance;
			plug->WriteToCache();
			break;
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

	TrackerDirectories::Instance().SetWorkingDirectory(dlg.GetWorkingDirectory(), DIR_PLUGINS);

	CVstPluginManager *pManager = theApp.GetPluginManager();

	VSTPluginLib *plugLib = nullptr;
	bool update = false;

	const FileDialog::PathList &files = dlg.GetFilenames();
	for(size_t counter = 0; counter < files.size(); counter++)
	{
		if (pManager)
		{
			VSTPluginLib *lib = pManager->AddPlugin(files[counter], false);
			if(lib != nullptr)
			{
				update = true;
				if(!VerifyPlug(lib))
				{
					pManager->RemovePlugin(lib);
				} else
				{
					plugLib = lib;

					// If this plugin was missing anywhere, try loading it
					std::vector<CModDoc *> docs(theApp.GetOpenDocuments());
					for(size_t i = 0; i < docs.size(); i++)
					{
						CModDoc &doc = *docs[i];
						CSoundFile &sndFile = doc.GetrSoundFile();
						bool updateDoc = false;
						for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
						{
							SNDMIXPLUGIN &plugin = sndFile.m_MixPlugins[plug];
							if(plugin.pMixPlugin == nullptr
								&& plugin.Info.dwPluginId1 == lib->pluginId1
								&& plugin.Info.dwPluginId2 == lib->pluginId2)
							{
								CSoundFile::gpMixPluginCreateProc(plugin, sndFile);
								if(plugin.pMixPlugin)
								{
									plugin.pMixPlugin->RestoreAllParameters(plugin.defaultProgram);
								}
							}
						}
						if(updateDoc)
						{
							doc.UpdateAllViews(nullptr, PluginHint().Info().Names());
						}
					}
				}
			}
		}
	}
	if(update)
	{
		// Force selection to last added plug.
		UpdatePluginsList(plugLib ? plugLib->pluginId2 : 0);
	} else
	{
		Reporting::Error("At least one selected file was not a valid VST Plugin.");
	}
}


void CSelectPluginDlg::OnScanFolder()
//-----------------------------------
{
	BrowseForFolder dlg(TrackerDirectories::Instance().GetWorkingDirectory(DIR_PLUGINS), "Select a folder that should be scanned for VST plugins (including sub-folders)");
	if(!dlg.Show(this)) return;

	TrackerDirectories::Instance().SetWorkingDirectory(dlg.GetDirectory(), DIR_PLUGINS);

	CVstPluginManager *pManager = theApp.GetPluginManager();
	VSTPluginLib *plugLib = nullptr;
	bool update = false;

	CDialog pluginScanDlg;
	pluginScanDlg.Create(IDD_SCANPLUGINS, this);
	pluginScanDlg.CenterWindow(this);
	pluginScanDlg.ShowWindow(SW_SHOW);

	FolderScanner scan(dlg.GetDirectory(), true);
	mpt::PathString fileName;
	int files = 0;
	while(scan.NextFile(fileName))
	{
		if(!mpt::PathString::CompareNoCase(fileName.GetFileExt(), MPT_PATHSTRING(".dll")))
		{
			CWnd *text = pluginScanDlg.GetDlgItem(IDC_SCANTEXT);
			std::wstring scanStr = L"Scanning Plugin...\n" + fileName.ToWide();
			SetWindowTextW(text->m_hWnd, scanStr.c_str());
			MSG msg;
			while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}

			VSTPluginLib *lib = pManager->AddPlugin(fileName, false);
			if(lib)
			{
				update = true;
				if(!VerifyPlug(lib))
				{
					pManager->RemovePlugin(lib);
				} else
				{
					plugLib = lib;
					files++;
				}
			}
		}
	}

	if(update)
	{
		// Force selection to last added plug.
		Reporting::Information(mpt::String::Print("Found %1 plugins.", files).c_str(), this);
		UpdatePluginsList(plugLib ? plugLib->pluginId2 : 0);
	} else
	{
		Reporting::Error("Could not find any valid VST plugins.");
	}
}


void CSelectPluginDlg::OnRemovePlugin()
//-------------------------------------
{
	const HTREEITEM pluginToDelete = m_treePlugins.GetSelectedItem();
	VSTPluginLib *pPlug = GetSelectedPlugin();
	CVstPluginManager *pManager = theApp.GetPluginManager();

	if ((pManager) && (pPlug))
	{
		if(pManager->RemovePlugin(pPlug))
		{
			m_treePlugins.DeleteItem(pluginToDelete);
		}
	}
}


void CSelectPluginDlg::OnSetBridge()
//----------------------------------
{
	VSTPluginLib *plug = GetSelectedPlugin();
	if(plug)
	{
		if(m_chkBridge.IsWindowEnabled())
		{
			// Only update this setting if the current setting isn't an enforced setting (e.g. because plugin isn't native).
			// This has the advantage that plugins don't get force-checked when switching between 32-bit and 64-bit versions of OpenMPT.
			plug->useBridge = m_chkBridge.GetCheck() != BST_UNCHECKED;
		}
		m_chkShare.EnableWindow(m_chkBridge.GetCheck() != BST_UNCHECKED);
		plug->shareBridgeInstance = m_chkShare.GetCheck() != BST_UNCHECKED;
		plug->WriteToCache();
	}
}


void CSelectPluginDlg::OnSize(UINT nType, int cx, int cy)
//-------------------------------------------------------
{
	CDialog::OnSize(nType, cx, cy);

	if (m_treePlugins)
	{
		m_treePlugins.MoveWindow(8, 36, cx - 109, cy - 88, FALSE);

		GetDlgItem(IDC_STATIC_VSTNAMEFILTER)->MoveWindow(8, 11, 40, 21, FALSE);
		GetDlgItem(IDC_NAMEFILTER)->MoveWindow(40, 8, cx - 141, 21, FALSE);
		GetDlgItem(IDC_TEXT_CURRENT_VSTPLUG)->MoveWindow(8, cy - 45, cx - 22, 20, FALSE);
		m_chkBridge.MoveWindow(8, cy - 25, 110, 20, FALSE);
		m_chkShare.MoveWindow(120, cy - 25, cx - 128, 20, FALSE);
		const int rightOff = cx - 90;	// Offset of right button column
		GetDlgItem(IDOK)->MoveWindow(		rightOff,	8,			80, 23, FALSE);
		GetDlgItem(IDCANCEL)->MoveWindow(	rightOff,	39,			80, 23, FALSE);
		GetDlgItem(IDC_BUTTON1)->MoveWindow(rightOff,	cy - 133,	80, 23, FALSE);
		GetDlgItem(IDC_BUTTON3)->MoveWindow(rightOff,	cy - 105,	80, 23, FALSE);
		GetDlgItem(IDC_BUTTON2)->MoveWindow(rightOff,	cy - 77,	80, 23, FALSE);
		Invalidate();
	}
}

void CSelectPluginDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
//-------------------------------------------------------
{
	lpMMI->ptMinTrackSize.x = 350;
	lpMMI->ptMinTrackSize.y = 270;
	CDialog::OnGetMinMaxInfo(lpMMI);
}

#endif // NO_VST


OPENMPT_NAMESPACE_END
