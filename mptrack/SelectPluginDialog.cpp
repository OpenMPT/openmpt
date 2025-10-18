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

#include "SelectPluginDialog.h"
#include "FileDialog.h"
#include "FolderScanner.h"
#include "HighDPISupport.h"
#include "ImageLists.h"
#include "InputHandler.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Mptrack.h"
#include "Reporting.h"
#include "resource.h"
#include "TrackerSettings.h"
#include "../common/mptStringBuffer.h"
#include "../pluginBridge/BridgeWrapper.h"
#include "../soundlib/plugins/PluginManager.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "mpt/string/utility.hpp"


OPENMPT_NAMESPACE_BEGIN

/////////////////////////////////////////////////////////////////////////////////
// Plugin selection dialog


BEGIN_MESSAGE_MAP(CSelectPluginDlg, ResizableDialog)
	ON_NOTIFY(TVN_SELCHANGED,		IDC_TREE1, &CSelectPluginDlg::OnSelChanged)
	ON_NOTIFY(NM_DBLCLK,			IDC_TREE1, &CSelectPluginDlg::OnSelDblClk)
	ON_COMMAND(IDC_BUTTON1,			&CSelectPluginDlg::OnAddPlugin)
	ON_COMMAND(IDC_BUTTON3,			&CSelectPluginDlg::OnScanFolder)
	ON_COMMAND(IDC_BUTTON2,			&CSelectPluginDlg::OnRemovePlugin)
	ON_COMMAND(IDC_CHECK1,			&CSelectPluginDlg::OnSetBridge)
	ON_COMMAND(IDC_CHECK2,			&CSelectPluginDlg::OnSetBridge)
	ON_COMMAND(IDC_CHECK3,			&CSelectPluginDlg::OnSetBridge)
	ON_EN_CHANGE(IDC_NAMEFILTER,	&CSelectPluginDlg::OnNameFilterChanged)
	ON_EN_CHANGE(IDC_PLUGINTAGS,	&CSelectPluginDlg::OnPluginTagsChanged)
END_MESSAGE_MAP()


void CSelectPluginDlg::DoDataExchange(CDataExchange* pDX)
{
	ResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE1, m_treePlugins);
	DDX_Control(pDX, IDC_CHECK1, m_chkBridge);
	DDX_Control(pDX, IDC_CHECK2, m_chkShare);
	DDX_Control(pDX, IDC_CHECK3, m_chkLegacyBridge);
}


CSelectPluginDlg::CSelectPluginDlg(CModDoc *pModDoc, PLUGINDEX pluginSlot, CWnd *parent)
    : ResizableDialog(IDD_SELECTMIXPLUGIN, parent)
    , m_pModDoc(pModDoc)
    , m_nPlugSlot(pluginSlot)
{
	if(m_pModDoc && 0 <= m_nPlugSlot && m_nPlugSlot < MAX_MIXPLUGINS)
	{
		m_pPlugin = &(pModDoc->GetSoundFile().m_MixPlugins[m_nPlugSlot]);
	}

	CMainFrame::GetInputHandler()->Bypass(true);
}


CSelectPluginDlg::~CSelectPluginDlg()
{
	CMainFrame::GetInputHandler()->Bypass(false);
}


BOOL CSelectPluginDlg::OnInitDialog()
{
	DWORD dwRemove = TVS_EDITLABELS|TVS_SINGLEEXPAND;
	DWORD dwAdd = TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;

	ResizableDialog::OnInitDialog();
	m_treePlugins.ModifyStyle(dwRemove, dwAdd);
	m_treePlugins.SetImageList(&CMainFrame::GetMainFrame()->m_MiscIcons, TVSIL_NORMAL);

	if (m_pPlugin)
	{
		CString targetSlot = MPT_CFORMAT("&Put in FX{}")(mpt::cfmt::dec0<2>(m_nPlugSlot + 1));
		SetDlgItemText(IDOK, targetSlot);
		::EnableWindow(::GetDlgItem(m_hWnd, IDOK), TRUE);
	} else
	{
		::EnableWindow(::GetDlgItem(m_hWnd, IDOK), FALSE);
	}

	const int dpi = HighDPISupport::GetDpiForWindow(m_hWnd);
	CRect rect
	(
		CPoint(MulDiv(TrackerSettings::Instance().gnPlugWindowX, dpi, 96), MulDiv(TrackerSettings::Instance().gnPlugWindowY, dpi, 96)),
		CSize(MulDiv(TrackerSettings::Instance().gnPlugWindowWidth, dpi, 96), MulDiv(TrackerSettings::Instance().gnPlugWindowHeight, dpi, 96))
	);
	::MapWindowPoints(GetParent()->m_hWnd, HWND_DESKTOP, (CPoint *)&rect, 2);
	WINDOWPLACEMENT wnd;
	wnd.length = sizeof(wnd);
	GetWindowPlacement(&wnd);
	wnd.showCmd = SW_SHOW;
	wnd.rcNormalPosition = rect;
	SetWindowPlacement(&wnd);

	UpdatePluginsList();
	OnSelChanged(NULL, NULL);
	return TRUE;
}


void CSelectPluginDlg::OnDPIChanged()
{
	DialogBase::OnDPIChanged();
	// TODO: Icon scaling will be wrong if main window is on a screen with different DPI
	m_treePlugins.SetImageList(&CMainFrame::GetMainFrame()->m_MiscIcons, TVSIL_NORMAL);
	m_treePlugins.SetIndent(0);
}


void CSelectPluginDlg::OnOK()
{
	if(m_pPlugin == nullptr)
	{
		ResizableDialog::OnOK();
		return;
	}

	bool changed = false;
	CVstPluginManager *pManager = theApp.GetPluginManager();
	VSTPluginLib *pNewPlug = GetSelectedPlugin();
	VSTPluginLib *pFactory = nullptr;
	IMixPlugin *pCurrentPlugin = nullptr;
	if(m_pPlugin)
		pCurrentPlugin = m_pPlugin->pMixPlugin;
	if((pManager) && (pManager->IsValidPlugin(pNewPlug)))
		pFactory = pNewPlug;

	if (pFactory)
	{
		// Plugin selected
		if ((!pCurrentPlugin) || &pCurrentPlugin->GetPluginFactory() != pFactory)
		{
			CriticalSection cs;

			// Destroy old plugin, if there was one.
			const auto oldOutput = m_pPlugin->GetOutputPlugin();
			m_pPlugin->Destroy();

			// Initialize plugin info
			MemsetZero(m_pPlugin->Info);
			if(oldOutput != PLUGINDEX_INVALID)
				m_pPlugin->SetOutputPlugin(oldOutput);
			m_pPlugin->Info.dwPluginId1 = pFactory->pluginId1;
			m_pPlugin->Info.dwPluginId2 = pFactory->pluginId2;
			m_pPlugin->Info.shellPluginID = pFactory->shellPluginID;
			m_pPlugin->editorX = m_pPlugin->editorY = int32_min;
			m_pPlugin->SetAutoSuspend(TrackerSettings::Instance().enableAutoSuspend);

#ifdef MPT_WITH_VST
			if(m_pPlugin->Info.dwPluginId1 == Vst::kEffectMagic)
			{
				switch(m_pPlugin->Info.dwPluginId2)
				{
					// Enable drymix by default for these known plugins
				case Vst::FourCC("Scop"):
					m_pPlugin->SetDryMix();
					break;
				}
			}
#endif // MPT_WITH_VST

			m_pPlugin->Info.szName = pFactory->libraryName.ToLocale();
			m_pPlugin->Info.szLibraryName = pFactory->libraryName.ToUTF8();

			cs.Leave();

			// Now, create the new plugin
			if(pManager && m_pModDoc)
			{
				pManager->CreateMixPlugin(*m_pPlugin, m_pModDoc->GetSoundFile());
				if (m_pPlugin->pMixPlugin)
				{
					IMixPlugin *p = m_pPlugin->pMixPlugin;
					const CString name = p->GetDefaultEffectName();
					if(!name.IsEmpty())
					{
						m_pPlugin->Info.szName = mpt::ToCharset(mpt::Charset::Locale, name);
					}
					m_pPlugin->SetDryMix(p->IsInstrument());

					// Check if plugin slot is already assigned to any instrument, and if not, create one.
					if(p->IsInstrument() && m_pModDoc->HasInstrumentForPlugin(m_nPlugSlot) == INSTRUMENTINDEX_INVALID)
					{
						m_pModDoc->InsertInstrumentForPlugin(m_nPlugSlot);
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
		if(m_pModDoc)
			changed = m_pModDoc->RemovePlugin(m_nPlugSlot);
	}

	//remember window size:
	SaveWindowPos();

	if(changed)
	{
		if(m_pPlugin->Info.dwPluginId2)
			TrackerSettings::Instance().lastSelectedPlugin = {static_cast<uint32>(m_pPlugin->Info.dwPluginId1), static_cast<uint32>(m_pPlugin->Info.dwPluginId2), m_pPlugin->Info.shellPluginID};
		if(m_pModDoc)
		{
			m_pModDoc->UpdateAllViews(nullptr, PluginHint(static_cast<PLUGINDEX>(m_nPlugSlot + 1)).Info().Names());
		}
		ResizableDialog::OnOK();
	} else
	{
		ResizableDialog::OnCancel();
	}
}


void CSelectPluginDlg::OnCancel()
{
	//remember window size:
	SaveWindowPos();
	ResizableDialog::OnCancel();
}


VSTPluginLib* CSelectPluginDlg::GetSelectedPlugin()
{
	HTREEITEM item = m_treePlugins.GetSelectedItem();
	if(item)
		return reinterpret_cast<VSTPluginLib *>(m_treePlugins.GetItemData(item));
	else
		return nullptr;
}


void CSelectPluginDlg::SaveWindowPos() const
{
	WINDOWPLACEMENT wnd;
	wnd.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(&wnd);
	CRect rect = wnd.rcNormalPosition;
	::MapWindowPoints(HWND_DESKTOP, GetParent()->m_hWnd, (CPoint *)&rect, 2);
	TrackerSettings::Instance().gnPlugWindowX = HighDPISupport::ScalePixelsInv(rect.left, m_hWnd);
	TrackerSettings::Instance().gnPlugWindowY = HighDPISupport::ScalePixelsInv(rect.top, m_hWnd);
	TrackerSettings::Instance().gnPlugWindowWidth = HighDPISupport::ScalePixelsInv(rect.Width(), m_hWnd);
	TrackerSettings::Instance().gnPlugWindowHeight = HighDPISupport::ScalePixelsInv(rect.Height(), m_hWnd);
}


BOOL CSelectPluginDlg::PreTranslateMessage(MSG *pMsg)
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

	return ResizableDialog::PreTranslateMessage(pMsg);
}


void CSelectPluginDlg::OnNameFilterChanged()
{
	// Update name filter text
	m_nameFilter = mpt::ToLowerCase(GetWindowTextUnicode(*GetDlgItem(IDC_NAMEFILTER)));

	UpdatePluginsList();
}


void CSelectPluginDlg::UpdatePluginsList(const VSTPluginLib *forceSelect)
{
	CVstPluginManager *pManager = theApp.GetPluginManager();

	m_treePlugins.SetRedraw(FALSE);
	m_treePlugins.DeleteAllItems();

	static constexpr struct
	{
		PluginCategory category;
		const TCHAR *description;
	} categories[] =
	{
		{ PluginCategory::Effect,         _T("Audio Effects") },
		{ PluginCategory::Generator,      _T("Tone Generators") },
		{ PluginCategory::Restoration,    _T("Audio Restauration") },
		{ PluginCategory::SurroundFx,     _T("Surround Effects") },
		{ PluginCategory::RoomFx,         _T("Room Effects") },
		{ PluginCategory::Spacializer,    _T("Spacializers") },
		{ PluginCategory::Mastering,      _T("Mastering Plugins") },
		{ PluginCategory::Analysis,       _T("Analysis Plugins") },
		{ PluginCategory::OfflineProcess, _T("Offline Processing") },
		{ PluginCategory::Shell,          _T("Shell Plugins") },
		{ PluginCategory::Unknown,        _T("Unsorted") },
		{ PluginCategory::DMO,            _T("DirectX Media Audio Effects") },
		{ PluginCategory::Synth,          _T("Instrument Plugins") },
		{ PluginCategory::Hidden,         _T("Legacy Plugins") },
	};

	const HTREEITEM noPlug = AddTreeItem(_T("No plugin (empty slot)"), IMAGE_NOPLUGIN, false);
	HTREEITEM currentPlug = noPlug;

	std::bitset<uint8(PluginCategory::NumCategories)> categoryUsed;
	HTREEITEM categoryFolders[uint8(PluginCategory::NumCategories)];
	for(const auto &cat : categories)
	{
		categoryFolders[static_cast<uint32>(cat.category)] = AddTreeItem(cat.description, IMAGE_FOLDER, false);
	}

	enum PlugMatchQuality
	{
		kNoMatch,
		kSameIdAsLast,
		kSameIdAsLastWithPlatformMatch,
		kSameIdAsCurrent,
		kFoundCurrentPlugin,
	};
	PlugMatchQuality foundPlugin = kNoMatch;

	const auto lastPluginID = TrackerSettings::Instance().lastSelectedPlugin.Get();
	const bool nameFilterActive = !m_nameFilter.empty();
	const auto currentTags = mpt::split(m_nameFilter, U_(" "));

	if(pManager)
	{
		bool first = true;

		for(auto &p : *pManager)
		{
			MPT_ASSERT(p);
			const VSTPluginLib &plug = *p;
			if(plug.category == PluginCategory::Hidden && (m_pPlugin == nullptr || m_pPlugin->pMixPlugin == nullptr || &m_pPlugin->pMixPlugin->GetPluginFactory() != p.get()))
				continue;

			if(nameFilterActive)
			{
				// Apply name filter
				bool matches = false;
				// Search in plugin names
				{
					mpt::ustring displayName = mpt::ToLowerCase(plug.libraryName.ToUnicode());
					if(displayName.find(m_nameFilter, 0) != displayName.npos)
					{
						matches = true;
					}
				}
				// Search in plugin tags
				if(!matches)
				{
					mpt::ustring tags = mpt::ToLowerCase(plug.tags);
					for(const auto &tag : currentTags)
					{
						if(!tag.empty() && tags.find(tag, 0) != tags.npos)
						{
							matches = true;
							break;
						}
					}
				}
				// Search in plugin vendors
				if(!matches)
				{
					mpt::ustring vendor = mpt::ToLowerCase(mpt::ToUnicode(plug.vendor));
					if(vendor.find(m_nameFilter, 0) != vendor.npos)
					{
						matches = true;
					}
				}
				if(!matches) continue;
			}

			CString title = plug.libraryName.ToCString();
#ifdef MPT_WITH_VST
			if(!plug.IsNativeFromCache())
			{
				title += MPT_CFORMAT(" ({})")(plug.GetDllArchNameUser());
			}
#endif // MPT_WITH_VST
			HTREEITEM h = AddTreeItem(title, plug.isInstrument ? IMAGE_PLUGININSTRUMENT : IMAGE_EFFECTPLUGIN, true, categoryFolders[static_cast<uint32>(plug.category)], reinterpret_cast<LPARAM>(&plug));
			categoryUsed[static_cast<uint32>(plug.category)] = true;

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

			if(forceSelect != nullptr && &plug == forceSelect)
			{
				// Forced selection (e.g. just after add plugin)
				currentPlug = h;
				foundPlugin = kFoundCurrentPlugin;
			}

			if(m_pPlugin && foundPlugin < kFoundCurrentPlugin)
			{
				//Which plugin should be selected?
				if(m_pPlugin->pMixPlugin)
				{
					// Current slot's plugin
					IMixPlugin *pPlugin = m_pPlugin->pMixPlugin;
					if (&pPlugin->GetPluginFactory() == &plug)
					{
						currentPlug = h;
						foundPlugin = kFoundCurrentPlugin;
					}
				} else if(m_pPlugin->Info.dwPluginId1 != 0 || m_pPlugin->Info.dwPluginId2 != 0)
				{
					// Plugin with matching ID to current slot's plug
					if(plug.pluginId1 == m_pPlugin->Info.dwPluginId1
						&& plug.pluginId2 == m_pPlugin->Info.dwPluginId2
						&& plug.shellPluginID == m_pPlugin->Info.shellPluginID)
					{
						currentPlug = h;
						foundPlugin = kSameIdAsCurrent;
					}
				} else if(static_cast<uint32>(plug.pluginId1) == lastPluginID.pluginID1
					&& static_cast<uint32>(plug.pluginId2) == lastPluginID.pluginID2
					&& plug.shellPluginID == lastPluginID.shellPluginID
					&& foundPlugin < kSameIdAsLastWithPlatformMatch)
				{
					// Previously selected plugin
#ifdef MPT_WITH_VST
					foundPlugin = plug.IsNativeFromCache() ? kSameIdAsLastWithPlatformMatch : kSameIdAsLast;
#else // !MPT_WITH_VST
					foundPlugin = kSameIdAsLastWithPlatformMatch;
#endif // MPT_WITH_VST
					currentPlug = h;
				}
			}
		}
	}

	// Remove empty categories
	for(size_t i = 0; i < std::size(categoryFolders); i++)
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


HTREEITEM CSelectPluginDlg::AddTreeItem(const TCHAR *title, int image, bool sort, HTREEITEM hParent, LPARAM lParam)
{
	return m_treePlugins.InsertItem(
		TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT,
		title,
		image, image,
		0, 0,
		lParam,
		hParent,
		(sort ? TVI_SORT : TVI_LAST));
}


void CSelectPluginDlg::OnSelDblClk(NMHDR *, LRESULT *result)
{
	if(m_pPlugin == nullptr) return;

	HTREEITEM hSel = m_treePlugins.GetSelectedItem();
	int nImage, nSelectedImage;
	m_treePlugins.GetItemImage(hSel, nImage, nSelectedImage);

	if ((hSel) && (nImage != IMAGE_FOLDER)) OnOK();
	if (result) *result = 0;
}


void CSelectPluginDlg::OnSelChanged(NMHDR *, LRESULT *result)
{
	CVstPluginManager *pManager = theApp.GetPluginManager();
	VSTPluginLib *pPlug = GetSelectedPlugin();
	bool  showBoxes = false;
	BOOL enableTagsTextBox = FALSE;
	BOOL enableRemoveButton = FALSE;
	if (pManager != nullptr && pManager->IsValidPlugin(pPlug))
	{
		if(pPlug->vendor.IsEmpty())
			SetDlgItemText(IDC_VENDOR, _T(""));
		else
			SetDlgItemText(IDC_VENDOR, _T("Vendor: ") + pPlug->vendor);
		if(pPlug->dllPath.empty())
			SetDlgItemText(IDC_TEXT_CURRENT_VSTPLUG, _T("Built-in plugin"));
		else
			SetDlgItemText(IDC_TEXT_CURRENT_VSTPLUG, pPlug->dllPath.ToCString());
		SetDlgItemText(IDC_PLUGINTAGS, mpt::ToCString(pPlug->tags));
		enableRemoveButton = pPlug->isBuiltIn ? FALSE : TRUE;
#ifdef MPT_WITH_VST
		if(pPlug->pluginId1 == Vst::kEffectMagic && !pPlug->isBuiltIn)
		{
			bool isBridgeAvailable =
					((pPlug->GetDllArch() == PluginArch_x86) && IsComponentAvailable(pluginBridge_x86))
				||
					((pPlug->GetDllArch() == PluginArch_x86) && IsComponentAvailable(pluginBridgeLegacy_x86))
				||
					((pPlug->GetDllArch() == PluginArch_amd64) && IsComponentAvailable(pluginBridge_amd64))
				||
					((pPlug->GetDllArch() == PluginArch_amd64) && IsComponentAvailable(pluginBridgeLegacy_amd64))
#if defined(MPT_WITH_WINDOWS10)
				||
					((pPlug->GetDllArch() == PluginArch_arm) && IsComponentAvailable(pluginBridge_arm))
				||
					((pPlug->GetDllArch() == PluginArch_arm) && IsComponentAvailable(pluginBridgeLegacy_arm))
				||
					((pPlug->GetDllArch() == PluginArch_arm64) && IsComponentAvailable(pluginBridge_arm64))
				||
					((pPlug->GetDllArch() == PluginArch_arm64) && IsComponentAvailable(pluginBridgeLegacy_arm64))
#endif // MPT_WITH_WINDOWS10
				;
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

			m_chkLegacyBridge.SetCheck((!pPlug->modernBridge) ? BST_CHECKED : BST_UNCHECKED);
			m_chkLegacyBridge.EnableWindow(m_chkBridge.GetCheck() != BST_UNCHECKED);

			showBoxes = true;
		}
		enableTagsTextBox = TRUE;
#endif // MPT_WITH_VST
	} else
	{
		SetDlgItemText(IDC_VENDOR, _T(""));
		SetDlgItemText(IDC_TEXT_CURRENT_VSTPLUG, _T(""));
		SetDlgItemText(IDC_PLUGINTAGS, _T(""));
	}
	GetDlgItem(IDC_PLUGINTAGS)->EnableWindow(enableTagsTextBox);
	GetDlgItem(IDC_BUTTON2)->EnableWindow(enableRemoveButton);
	if(!showBoxes)
	{
		m_chkBridge.EnableWindow(FALSE);
		m_chkShare.EnableWindow(FALSE);
		m_chkLegacyBridge.EnableWindow(FALSE);
		m_chkBridge.SetCheck(BST_UNCHECKED);
		m_chkShare.SetCheck(BST_UNCHECKED);
		m_chkLegacyBridge.SetCheck(BST_UNCHECKED);
	}
	if (result) *result = 0;
}


#ifdef MPT_WITH_VST
namespace
{
// TODO: Keep these lists up-to-date.
constexpr struct
{
	int32 id1;
	int32 id2;
	const char *name;
	const char *problem;
} ProblematicPlugins[] =
{
	{Vst::kEffectMagic, Vst::FourCC("mdaC"), "MDA Degrade", "* Old versions of this plugin can crash OpenMPT.\nEnsure that you have the latest version of this plugin."},
	{Vst::kEffectMagic, Vst::FourCC("fV2s"), "Farbrausch V2", "* This plugin can cause OpenMPT to freeze if being used in a combination with various other plugins.\nIt is recommended to use V2 only through the Plugin Bridge."},
	{Vst::kEffectMagic, Vst::FourCC("frV2"), "Farbrausch V2", "* This plugin can cause OpenMPT to freeze if being used in a combination with various other plugins.\nIt is recommended to use V2 only through the Plugin Bridge."},
	{Vst::kEffectMagic, Vst::FourCC("MMID"), "MIDI Input Output", "* The MIDI Input / Output plugin is now built right into OpenMPT and should not be loaded from an external file."},
};

// Plugins that should always be bridged or require a specific bridge mode.
constexpr struct
{
	int32 id1;
	int32 id2;
	bool useBridge;
	bool shareInstance;
	bool modernBridge;
} ForceBridgePlugins[] =
{
	{Vst::kEffectMagic, Vst::FourCC("fV2s"), true, false, false},  // V2 freezes on shutdown if there's more than one instance per process
	{Vst::kEffectMagic, Vst::FourCC("frV2"), true, false, false},  // ditto
	{Vst::kEffectMagic, Vst::FourCC("SKV3"), false, true, false},  // SideKick v3 always has to run in a shared instance
	{Vst::kEffectMagic, Vst::FourCC("YWS!"), false, true, false},  // You Wa Shock ! always has to run in a shared instance
	{Vst::kEffectMagic, Vst::FourCC("rsfz"), true, true, false},   // rgc:audio sfz has issues with /LARGEADDRESSAWARE, so must run through the legacy bridge
	{Vst::kEffectMagic, Vst::FourCC("S1Vs"), mpt::arch_bits == 64, true, false},  // Synth1 64-bit has an issue with pointers using the high 32 bits, hence must use the legacy bridge without high-entropy heap
};
}  // namespace
#endif // MPT_WITH_VST


bool CSelectPluginDlg::VerifyPlugin(VSTPluginLib *plug, CWnd *parent)
{
#ifdef MPT_WITH_VST
	for(const auto &p : ProblematicPlugins)
	{
		if(p.id2 == plug->pluginId2 && p.id1 == plug->pluginId1)
		{
			std::string s = MPT_AFORMAT("WARNING: This plugin has been identified as {},\nwhich is known to have the following problem with OpenMPT:\n\n{}\n\nWould you still like to add this plugin to the library?")(p.name, p.problem);
			if(Reporting::Confirm(s, false, false, parent) == cnfNo)
			{
				return false;
			}
			break;
		}
	}

	for(const auto &p : ForceBridgePlugins)
	{
		if(p.id2 == plug->pluginId2 && p.id1 == plug->pluginId1)
		{
			plug->useBridge = p.useBridge;
			plug->shareBridgeInstance = p.shareInstance;
			if(!p.modernBridge)
				plug->modernBridge = false;
			plug->WriteToCache();
			break;
		}
	}
#else // !MPT_WITH_VST
	MPT_UNREFERENCED_PARAMETER(plug);
	MPT_UNREFERENCED_PARAMETER(parent);
#endif // MPT_WITH_VST
	return true;
}


void CSelectPluginDlg::OnAddPlugin()
{
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.DefaultExtension(U_("dll"))
		.ExtensionFilter(U_("VST Plugins|*.dll;*.vst3||"))
		.WorkingDirectory(TrackerSettings::Instance().PathPlugins.GetWorkingDir());
	if(!dlg.Show(this)) return;

	TrackerSettings::Instance().PathPlugins.SetWorkingDir(dlg.GetWorkingDirectory());

	CVstPluginManager *plugManager = theApp.GetPluginManager();
	if(!plugManager)
		return;

	VSTPluginLib *plugLib = nullptr;
	bool update = false;

	for(const auto &file : dlg.GetFilenames())
	{
		for(VSTPluginLib *lib : plugManager->AddPlugin(file, TrackerSettings::Instance().BrokenPluginsWorkaroundVSTMaskAllCrashes, false))
		{
			update = true;
			if(!VerifyPlugin(lib, this))
			{
				plugManager->RemovePlugin(lib);
			} else
			{
				plugLib = lib;

				// If this plugin was missing anywhere, try loading it
				ReloadMissingPlugins(*lib);
			}
		}
	}
	if(update)
	{
		// Force selection to last added plug.
		UpdatePluginsList(plugLib);
	} else
	{
		Reporting::Error("No valid VST Plugin was selected.");
	}
}


void CSelectPluginDlg::OnScanFolder()
{
	BrowseForFolder dlg(TrackerSettings::Instance().PathPlugins.GetWorkingDir(), _T("Select a folder that should be scanned for VST plugins (including sub-folders)"));
	if(!dlg.Show(this)) return;

	TrackerSettings::Instance().PathPlugins.SetWorkingDir(dlg.GetDirectory());
	VSTPluginLib *plugLib = ScanPlugins(dlg.GetDirectory(), this);
	UpdatePluginsList(plugLib);

	// If any of the plugins was missing anywhere, try loading it
	for(auto &p : *theApp.GetPluginManager())
	{
		ReloadMissingPlugins(*p);
	}
}


VSTPluginLib *CSelectPluginDlg::ScanPlugins(const mpt::PathString &path, CWnd *parent)
{
	CVstPluginManager *pManager = theApp.GetPluginManager();
	VSTPluginLib *plugLib = nullptr;
	bool update = false;

	DialogBase pluginScanDlg;
	pluginScanDlg.Create(IDD_SCANPLUGINS, parent);
	pluginScanDlg.CenterWindow(parent);
	pluginScanDlg.ModifyStyle(0, WS_SYSMENU, WS_SYSMENU);
	pluginScanDlg.ShowWindow(SW_SHOW);

	FolderScanner scan(path, FolderScanner::kOnlyFiles | FolderScanner::kFindInSubDirectories);
	bool maskCrashes = TrackerSettings::Instance().BrokenPluginsWorkaroundVSTMaskAllCrashes;
	mpt::PathString fileName;
	int files = 0;
	while(scan.Next(fileName) && pluginScanDlg.IsWindowVisible())
	{
		if(!mpt::PathCompareNoCase(fileName.GetFilenameExtension(), P_(".dll")))
		{
			CWnd *text = pluginScanDlg.GetDlgItem(IDC_SCANTEXT);
			CString scanStr = _T("Scanning Plugin...\n") + fileName.ToCString();
			text->SetWindowText(scanStr);
			MSG msg;
			while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}

			for(VSTPluginLib *lib : pManager->AddPlugin(fileName, maskCrashes, false))
			{
				update = true;
				if(!VerifyPlugin(lib, parent))
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
		Reporting::Information(MPT_AFORMAT("Found {} plugin{}.")(files, files == 1 ? "" : "s").c_str(), parent);
		return plugLib;
	} else
	{
		Reporting::Error("Could not find any valid VST plugins.");
		return nullptr;
	}
}


// After adding new plugins, check if they were missing in any open songs.
void CSelectPluginDlg::ReloadMissingPlugins(const VSTPluginLib &lib) const
{
	CVstPluginManager *plugManager = theApp.GetPluginManager();
	auto docs = theApp.GetOpenDocuments();
	for(auto &modDoc : docs)
	{
		CSoundFile &sndFile = modDoc->GetSoundFile();
		bool updateDoc = false;
		for(auto &plugin : sndFile.m_MixPlugins)
		{
			if(plugin.pMixPlugin == nullptr
				&& plugin.Info.dwPluginId1 == lib.pluginId1
				&& plugin.Info.dwPluginId2 == lib.pluginId2
				&& plugin.Info.shellPluginID == lib.shellPluginID)
			{
				updateDoc = true;
				plugManager->CreateMixPlugin(plugin, sndFile);
				if(plugin.pMixPlugin)
				{
					plugin.pMixPlugin->RestoreAllParameters(plugin.defaultProgram);
				}
			}
		}
		if(updateDoc)
		{
			modDoc->UpdateAllViews(nullptr, PluginHint().Info().Names());
			CMainFrame::GetMainFrame()->UpdateTree(modDoc, PluginHint().Info().Names());
		}
	}
}


void CSelectPluginDlg::OnRemovePlugin()
{
	const HTREEITEM pluginToDelete = m_treePlugins.GetSelectedItem();
	VSTPluginLib *plugin = GetSelectedPlugin();
	CVstPluginManager *plugManager = theApp.GetPluginManager();

	if(plugManager && plugin)
	{
		if(plugManager->RemovePlugin(plugin))
		{
			m_treePlugins.DeleteItem(pluginToDelete);
		}
	}
}


void CSelectPluginDlg::OnSetBridge()
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
		m_chkLegacyBridge.EnableWindow(m_chkBridge.GetCheck() != BST_UNCHECKED);
		plug->shareBridgeInstance = m_chkShare.GetCheck() != BST_UNCHECKED;
		plug->modernBridge = m_chkLegacyBridge.GetCheck() == BST_UNCHECKED;
		plug->WriteToCache();
	}
}


void CSelectPluginDlg::OnPluginTagsChanged()
{
	VSTPluginLib *plug = GetSelectedPlugin();
	if (plug)
	{
		plug->tags = GetWindowTextUnicode(*GetDlgItem(IDC_PLUGINTAGS));
	}
}


OPENMPT_NAMESPACE_END
