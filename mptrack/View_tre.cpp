/*
 * view_tre.cpp
 * ------------
 * Purpose: Tree view for managing open songs, sound files, file browser, ...
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "ImageLists.h"
#include "View_tre.h"
#include "Mptrack.h"
#include "Moddoc.h"
#include "Dlsbank.h"
#include "dlg_misc.h"
#include "../common/mptFileIO.h"
#include "../common/FileReader.h"
#include "FileDialog.h"
#include "Globals.h"
#include "ExternalSamples.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/plugins/PlugInterface.h"


OPENMPT_NAMESPACE_BEGIN


CSoundFile *CModTree::m_SongFile = nullptr;

/////////////////////////////////////////////////////////////////////////////
// CModTreeDropTarget


BOOL CModTreeDropTarget::Register(CModTree *pWnd)
//-----------------------------------------------
{
	m_pModTree = pWnd;
	return COleDropTarget::Register(pWnd);
}


DROPEFFECT CModTreeDropTarget::OnDragEnter(CWnd *pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
//-----------------------------------------------------------------------------------------------------------------
{
	if ((m_pModTree) && (m_pModTree == pWnd)) return m_pModTree->OnDragEnter(pDataObject, dwKeyState, point);
	return DROPEFFECT_NONE;
}


DROPEFFECT CModTreeDropTarget::OnDragOver(CWnd *pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
//----------------------------------------------------------------------------------------------------------------
{
	if ((m_pModTree) && (m_pModTree == pWnd)) return m_pModTree->OnDragOver(pDataObject, dwKeyState, point);
	return DROPEFFECT_NONE;
}


BOOL CModTreeDropTarget::OnDrop(CWnd *pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
//-----------------------------------------------------------------------------------------------------------
{
	if ((m_pModTree) && (m_pModTree == pWnd)) return m_pModTree->OnDrop(pDataObject, dropEffect, point);
	return FALSE;
}


ModTreeDocInfo::ModTreeDocInfo(CModDoc &modDoc) : modDoc(modDoc)
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();
	nSeqSel = SEQUENCEINDEX_INVALID;
	nOrdSel = ORDERINDEX_INVALID;
	hSong = hPatterns = hSamples = hInstruments = hComments = hOrders = hEffects = nullptr;
	tiPatterns.resize(sndFile.Patterns.Size(), nullptr);
	tiOrders.resize(sndFile.Order.GetNumSequences());
	tiSequences.resize(sndFile.Order.GetNumSequences(), nullptr);
	samplesPlaying.reset();
	instrumentsPlaying.reset();
}


/////////////////////////////////////////////////////////////////////////////
// CModTree

BEGIN_MESSAGE_MAP(CModTree, CTreeCtrl)
	//{{AFX_MSG_MAP(CViewModTree)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_DROPFILES()
	ON_NOTIFY_REFLECT(NM_DBLCLK,		OnItemDblClk)
	ON_NOTIFY_REFLECT(NM_RETURN,		OnItemReturn)
	ON_NOTIFY_REFLECT(NM_RCLICK,		OnItemRightClick)
	ON_NOTIFY_REFLECT(NM_CLICK,			OnItemLeftClick)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED,	OnItemExpanded)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG,	OnBeginLDrag)
	ON_NOTIFY_REFLECT(TVN_BEGINRDRAG,	OnBeginRDrag)
	ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT,OnBeginLabelEdit)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT,	OnEndLabelEdit)
	ON_COMMAND(ID_MODTREE_REFRESH,		OnRefreshTree)
	ON_COMMAND(ID_MODTREE_EXECUTE,		OnExecuteItem)
	ON_COMMAND(ID_MODTREE_REMOVE,		OnDeleteTreeItem)
	ON_COMMAND(ID_MODTREE_PLAY,			OnPlayTreeItem)
	ON_COMMAND(ID_MODTREE_REFRESHINSTRLIB, OnRefreshInstrLib)
	ON_COMMAND(ID_MODTREE_OPENITEM,		OnOpenTreeItem)
	ON_COMMAND(ID_MODTREE_MUTE,			OnMuteTreeItem)
	ON_COMMAND(ID_MODTREE_SOLO,			OnSoloTreeItem)
	ON_COMMAND(ID_MODTREE_UNMUTEALL,	OnUnmuteAllTreeItem)
	ON_COMMAND(ID_MODTREE_DUPLICATE,	OnDuplicateTreeItem)
	ON_COMMAND(ID_MODTREE_INSERT,		OnInsertTreeItem)
	ON_COMMAND(ID_MODTREE_SWITCHTO,		OnSwitchToTreeItem)
	ON_COMMAND(ID_MODTREE_CLOSE,		OnCloseItem)
	ON_COMMAND(ID_MODTREE_SETPATH,		OnSetItemPath)
	ON_COMMAND(ID_MODTREE_SAVEITEM,		OnSaveItem)
	ON_COMMAND(ID_MODTREE_SAVEALL,		OnSaveAll)
	ON_COMMAND(ID_MODTREE_RELOADITEM,	OnReloadItem)
	ON_COMMAND(ID_MODTREE_RELOADALL,	OnReloadAll)
	ON_COMMAND(ID_MODTREE_FINDMISSING,	OnFindMissing)
	ON_COMMAND(ID_ADD_SOUNDBANK,		OnAddDlsBank)
	ON_COMMAND(ID_IMPORT_MIDILIB,		OnImportMidiLib)
	ON_COMMAND(ID_EXPORT_MIDILIB,		OnExportMidiLib)
	ON_COMMAND(ID_SOUNDBANK_PROPERTIES,	OnSoundBankProperties)
	ON_COMMAND(ID_MODTREE_SHOWDIRS,		OnShowDirectories)
	ON_COMMAND(ID_MODTREE_SHOWALLFILES,	OnShowAllFiles)
	ON_COMMAND(ID_MODTREE_SOUNDFILESONLY,OnShowSoundFiles)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,		OnCustomKeyMsg)	//rewbs.customKeys
	//}}AFX_MSG_MAP
	ON_WM_KILLFOCUS()		//rewbs.customKeys
	ON_WM_SETFOCUS()		//rewbs.customKeys
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CViewModTree construction/destruction

CModTree::CModTree(CModTree *pDataTree) :
	m_pDataTree(pDataTree),
	m_hDropWnd(nullptr),
	m_WatchDir(),
	m_dwStatus(0),
	m_nDocNdx(0), m_nDragDocNdx(0),
	m_hItemDrag(nullptr), m_hItemDrop(nullptr),
	m_hInsLib(nullptr), m_hMidiLib(nullptr),
	m_bShowAllFiles(false), doLabelEdit(false)
//--------------------------------------------
{
	if(m_pDataTree != nullptr)
	{
		// Set up instrument library monitoring thread
		m_hWatchDirKillThread = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hSwitchWatchDir = CreateEvent(NULL,  FALSE, FALSE, NULL);
		m_WatchDirThread = mpt::thread([&](){MonitorInstrumentLibrary();});
	}
	MemsetZero(m_tiMidi);
	MemsetZero(m_tiPerc);
}


CModTree::~CModTree()
//-------------------
{
	for(auto iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
	{
		delete (*iter);
	}
	DocInfo.clear();

	delete m_SongFile;
	m_SongFile = nullptr;

	if(m_pDataTree != nullptr)
	{
		SetEvent(m_hWatchDirKillThread);
		m_WatchDirThread.join();
		CloseHandle(m_hSwitchWatchDir);
		CloseHandle(m_hWatchDirKillThread);
	}
}


void CModTree::Init()
//-------------------
{
	m_MediaFoundationExtensions = FileType(CSoundFile::GetMediaFoundationFileTypes()).GetExtensions();

	DWORD dwRemove = TVS_SINGLEEXPAND;
	DWORD dwAdd = TVS_EDITLABELS | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;

	if (IsSampleBrowser())
	{
		dwRemove |= (TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS);
		dwAdd &= ~(TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS);
	}
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SINGLEEXPAND)
	{
		dwRemove &= ~TVS_SINGLEEXPAND;
		dwAdd |= TVS_SINGLEEXPAND;
		m_dwStatus |= TREESTATUS_SINGLEEXPAND;
	}
	ModifyStyle(dwRemove, dwAdd);
	ModifyStyleEx(0, WS_EX_ACCEPTFILES);

	if(!IsSampleBrowser())
	{
		std::vector<WCHAR> curDir;
		DWORD size = GetCurrentDirectoryW(0, nullptr);
		if(size)
		{
			curDir.resize(size + 1);
			GetCurrentDirectoryW(size + 1, curDir.data());
		}
		const mpt::PathString dirs[] =
		{
			TrackerSettings::Instance().PathSamples.GetDefaultDir(),
			TrackerSettings::Instance().PathInstruments.GetDefaultDir(),
			TrackerSettings::Instance().PathSongs.GetDefaultDir(),
			mpt::PathString::FromNative(curDir.data())
		};
		for(int i = 0; i < CountOf(dirs); i++)
		{
			m_InstrLibPath = dirs[i];
			if(!m_InstrLibPath.empty()) break;
		}
		m_InstrLibPath.EnsureTrailingSlash();
		m_pDataTree->InsLibSetFullPath(m_InstrLibPath, mpt::PathString());
	}

	SetImageList(&CMainFrame::GetMainFrame()->m_MiscIcons, TVSIL_NORMAL);
	if (!IsSampleBrowser())
	{
		// Create Midi Library
		m_hMidiLib = InsertItem("MIDI Library", IMAGE_FOLDER, IMAGE_FOLDER, TVI_ROOT, TVI_LAST);
		for (UINT iMidGrp=0; iMidGrp<17; iMidGrp++)
		{
			InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, szMidiGroupNames[iMidGrp], IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, (MODITEM_HDR_MIDIGROUP << MIDILIB_SHIFT) | iMidGrp, m_hMidiLib, TVI_LAST);
		}
	}
	m_hInsLib = InsertItem("Instrument Library", IMAGE_FOLDER, IMAGE_FOLDER, TVI_ROOT, TVI_LAST);
	RefreshMidiLibrary();
	RefreshDlsBanks();
	RefreshInstrumentLibrary();
	m_DropTarget.Register(this);
}


BOOL CModTree::PreTranslateMessage(MSG *pMsg)
//-------------------------------------------
{
	if (!pMsg) return TRUE;

	if(doLabelEdit)
	{
		if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		{
			// End editing by making edit box lose focus.
			SetFocus();
			return TRUE;
		}
		return CTreeCtrl::PreTranslateMessage(pMsg);
	}

	if (pMsg->message == WM_KEYDOWN)
	{
		switch(pMsg->wParam)
		{
		case VK_SPACE:
			if(!(pMsg->lParam & 0x40000000)) OnPlayTreeItem();
			return TRUE;

		case VK_RETURN:
			if(!(pMsg->lParam & 0x40000000))
			{
				HTREEITEM hItem = GetSelectedItem();
				if(hItem)
				{
					if(CMainFrame::GetInputHandler()->CtrlPressed())
					{
						if(IsSampleBrowser())
						{
							// Ctrl+Enter: Load sample into currently selected sample or instrument slot
							CModScrollView *view = static_cast<CModScrollView *>(CMainFrame::GetMainFrame()->GetActiveView());
							const ModItem modItem = GetModItem(hItem);
							if(view && (modItem.type == MODITEM_INSLIB_SAMPLE || modItem.type == MODITEM_INSLIB_INSTRUMENT))
							{
								const mpt::PathString file = InsLibGetFullPath(hItem);
								const char *className = view->GetRuntimeClass()->m_lpszClassName;
								if(!strcmp("CViewSample", className))
								{
									view->SendCtrlMessage(CTRLMSG_SMP_OPENFILE, (LPARAM)&file);
								} else if(!strcmp("CViewInstrument", className))
								{
									view->SendCtrlMessage(CTRLMSG_INS_OPENFILE, (LPARAM)&file);
								}
							}
						} else
						{
							// Ctrl+Enter: Edit item
							EditLabel(hItem);
						}
					} else
					{
						if(!ExecuteItem(hItem))
						{
							if(ItemHasChildren(hItem))
							{
								Expand(hItem, TVE_TOGGLE);
							}
						}
					}
				}
			}
			return TRUE;

		case VK_TAB:
			// Tab: Switch between folder and file view.
			if(this == CMainFrame::GetMainFrame()->GetUpperTreeview())
				CMainFrame::GetMainFrame()->GetLowerTreeview()->SetFocus();
			else
				CMainFrame::GetMainFrame()->GetUpperTreeview()->SetFocus();
			return TRUE;
		
		case VK_BACK:
			// Backspace: Go up one directory
			if(GetParentRootItem(GetSelectedItem()) == m_hInsLib || IsSampleBrowser())
			{
				CMainFrame::GetMainFrame()->GetUpperTreeview()->InstrumentLibraryChDir(MPT_PATHSTRING(".."), false);
				return TRUE;
			}
		}
	} else if(pMsg->message == WM_CHAR)
	{
		ModItem item = GetModItem(GetSelectedItem());
		switch(item.type)
		{
		case MODITEM_MIDIINSTRUMENT:
		case MODITEM_MIDIPERCUSSION:
		case MODITEM_INSLIB_SAMPLE:
		case MODITEM_INSLIB_INSTRUMENT:
		case MODITEM_DLSBANK_INSTRUMENT:
			// Avoid cycling through tree-view elements on key hold
			return true;
		}
	}

	//We handle keypresses before Windows has a chance to handle them (for alt etc..)
	if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
		(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
	{
		CInputHandler* ih = CMainFrame::GetInputHandler();

		//Translate message manually
		UINT nChar = (UINT)pMsg->wParam;
		UINT nRepCnt = LOWORD(pMsg->lParam);
		UINT nFlags = HIWORD(pMsg->lParam);
		KeyEventType kT = ih->GetKeyEventType(nFlags);
		InputTargetContext ctx = (InputTargetContext)(kCtxViewTree);

		if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull)
			return true; // Mapped to a command, no need to pass message on.
	}
	return CTreeCtrl::PreTranslateMessage(pMsg);
}


mpt::PathString CModTree::InsLibGetFullPath(HTREEITEM hItem) const
//----------------------------------------------------------------
{
	mpt::PathString fullPath = m_InstrLibPath;
	fullPath.EnsureTrailingSlash();
	return fullPath + mpt::PathString::FromCStringW(GetItemTextW(hItem));
}


bool CModTree::InsLibSetFullPath(const mpt::PathString &libPath, const mpt::PathString &songName)
//-----------------------------------------------------------------------------------------------
{
	if(!songName.empty() && mpt::PathString::CompareNoCase(m_SongFileName, songName))
	{
		// Load module for previewing its instruments
		InputFile f(libPath + songName);
		if(f.IsValid())
		{
			FileReader file = GetFileReader(f);
			if(file.IsValid())
			{
				if(m_SongFile != nullptr)
				{
					m_SongFile->Destroy();
				} else
				{
					m_SongFile = new (std::nothrow) CSoundFile;
				}
				if(m_SongFile != nullptr)
				{
					if(!m_SongFile->Create(file, CSoundFile::loadNoPatternOrPluginData, nullptr))
					{
						return false;
					}
					// Destroy some stuff that we're not going to use anyway.
					m_SongFile->Patterns.DestroyPatterns();
					m_SongFile->m_songMessage.clear();
				}
			}
		} else
		{
			return false;
		}
	}
	m_InstrLibPath = libPath;
	m_SongFileName = songName;
	return true;
}


bool CModTree::SetSoundFile(FileReader &file)
//-------------------------------------------
{
	CSoundFile *sndFile = new (std::nothrow) CSoundFile;
	if(sndFile == nullptr || !sndFile->Create(file, CSoundFile::loadNoPatternOrPluginData))
	{
		delete sndFile;
		return false;
	}

	if(m_SongFile != nullptr)
	{
		m_SongFile->Destroy();
		delete m_SongFile;
	}
	m_SongFile = sndFile;
	m_SongFile->Patterns.DestroyPatterns();
	m_SongFile->m_songMessage.clear();
	const mpt::PathString fileName = file.GetFileName();
	m_InstrLibPath = fileName.GetPath();
	m_SongFileName = fileName.GetFullFileName();
	RefreshInstrumentLibrary();
	return true;
}


void CModTree::OnOptionsChanged()
//-------------------------------
{
	DWORD dwRemove = TVS_SINGLEEXPAND, dwAdd = 0;
	m_dwStatus &= ~TREESTATUS_SINGLEEXPAND;
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SINGLEEXPAND)
	{
		dwRemove = 0;
		dwAdd = TVS_SINGLEEXPAND;
		m_dwStatus |= TREESTATUS_SINGLEEXPAND;
	}
	ModifyStyle(dwRemove, dwAdd);
}


void CModTree::AddDocument(CModDoc &modDoc)
//-----------------------------------------
{
	// Check if document is already in the list
	for(auto iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
	{
		if(&(*iter)->modDoc == &modDoc)
		{
			return;
		}
	}

	ModTreeDocInfo *pInfo = new (std::nothrow) ModTreeDocInfo(modDoc);
	if(!pInfo)
	{
		return;
	}
	DocInfo.push_back(pInfo);

	UpdateView(*pInfo, UpdateHint().ModType());
	if (pInfo->hSong)
	{
		Expand(pInfo->hSong, TVE_EXPAND);
		EnsureVisible(pInfo->hSong);
		SelectItem(pInfo->hSong);
	}
}


void CModTree::RemoveDocument(CModDoc &modDoc)
//--------------------------------------------
{
	for(auto iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
	{
		if(&(*iter)->modDoc == &modDoc)
		{
			DeleteItem((*iter)->hSong);
			delete (*iter);
			DocInfo.erase(iter);
			break;
		}
	}
	// Refresh all item IDs
	for(size_t i = 0; i < DocInfo.size(); i++)
	{
		SetItemData(DocInfo[i]->hSong, i);
	}
}


// Get CModDoc that is associated with a tree item
ModTreeDocInfo *CModTree::GetDocumentInfoFromItem(HTREEITEM hItem)
//----------------------------------------------------------------
{
	hItem = GetParentRootItem(hItem);
	if(hItem != nullptr)
	{
		// Root item has moddoc pointer
		const size_t doc = GetItemData(hItem);
		if(doc < DocInfo.size() && hItem == DocInfo[doc]->hSong)
		{
			return DocInfo[doc];
		}
	}
	return nullptr;
}


// Get modtree doc information for a given CModDoc
ModTreeDocInfo *CModTree::GetDocumentInfoFromModDoc(CModDoc &modDoc)
//------------------------------------------------------------------
{
	for(auto iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
	{
		if(&(*iter)->modDoc == &modDoc)
		{
			return (*iter);
		}
	}
	return nullptr;
}


/////////////////////////////////////////////////////////////////////////////
// CViewModTree drawing

void CModTree::RefreshMidiLibrary()
//---------------------------------
{
	std::wstring s;
	WCHAR stmp[256];
	TV_ITEMW tvi;
	const MIDILIBSTRUCT &midiLib = CTrackApp::GetMidiLibrary();

	if (IsSampleBrowser()) return;
	// Midi Programs
	HTREEITEM parent = GetChildItem(m_hMidiLib);
	for(UINT iMidi = 0; iMidi < 128; iMidi++)
	{
		DWORD dwImage = IMAGE_INSTRMUTE;
		s = mpt::ToWString(iMidi) + L": " + mpt::ToWide(mpt::CharsetASCII, szMidiProgramNames[iMidi]);
		const LPARAM param = (MODITEM_MIDIINSTRUMENT << MIDILIB_SHIFT) | iMidi;
		if(!midiLib.MidiMap[iMidi].empty())
		{
			s += L": " + midiLib.MidiMap[iMidi].GetFullFileName().ToWide();
			dwImage = IMAGE_INSTRUMENTS;
		}
		if (!m_tiMidi[iMidi])
		{
			m_tiMidi[iMidi] = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
							s.c_str(), dwImage, dwImage, 0, 0, param, parent, TVI_LAST);
		} else
		{
			tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
			tvi.hItem = m_tiMidi[iMidi];
			tvi.pszText = stmp;
			tvi.cchTextMax = sizeof(stmp);
			tvi.iImage = tvi.iSelectedImage = dwImage;
			GetItem(&tvi);
			if(s != stmp || tvi.iImage != (int)dwImage)
			{
				SetItem(m_tiMidi[iMidi], TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
					s.c_str(), dwImage, dwImage, 0, 0, param);
			}
		}
		if((iMidi % 8u) == 7u)
		{
			parent = GetNextSiblingItem(parent);
		}
	}
	// Midi Percussions
	for (UINT iPerc=24; iPerc<=84; iPerc++)
	{
		DWORD dwImage = IMAGE_NOSAMPLE;
		s = mpt::ToWide(mpt::CharsetASCII, CSoundFile::GetNoteName((ModCommand::NOTE)(iPerc + NOTE_MIN))) + L": " + mpt::ToWide(mpt::CharsetASCII, szMidiPercussionNames[iPerc - 24]);
		const LPARAM param = (MODITEM_MIDIPERCUSSION << MIDILIB_SHIFT) | iPerc;
		if(!midiLib.MidiMap[iPerc | 0x80].empty())
		{
			s += L": " + midiLib.MidiMap[iPerc | 0x80].GetFullFileName().ToWide();
			dwImage = IMAGE_SAMPLES;
		}
		if (!m_tiPerc[iPerc])
		{
			m_tiPerc[iPerc] = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
							s.c_str(), dwImage, dwImage, 0, 0, param, parent, TVI_LAST);
		} else
		{
			tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
			tvi.hItem = m_tiPerc[iPerc];
			tvi.pszText = stmp;
			tvi.cchTextMax = sizeof(stmp);
			tvi.iImage = tvi.iSelectedImage = dwImage;
			GetItem(&tvi);
			if(s != stmp || tvi.iImage != (int)dwImage)
			{
				SetItem(m_tiPerc[iPerc], TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE,
							s.c_str(), dwImage, dwImage, 0, 0, param);
			}
		}
	}
}


void CModTree::RefreshDlsBanks()
//------------------------------
{
	CHAR s[256];
	HTREEITEM hDlsRoot = m_hMidiLib;

	if (IsSampleBrowser()) return;

	if(m_tiDLS.size() < CTrackApp::gpDLSBanks.size())
	{
		m_tiDLS.resize(CTrackApp::gpDLSBanks.size(), nullptr);
	}

	for(size_t iDls=0; iDls < CTrackApp::gpDLSBanks.size(); iDls++)
	{
		if(CTrackApp::gpDLSBanks[iDls])
		{
			if(!m_tiDLS[iDls])
			{
				TV_SORTCB tvs;
				CDLSBank *pDlsBank = CTrackApp::gpDLSBanks[iDls];
				// Add DLS file folder
				std::wstring name = pDlsBank->GetFileName().GetFullFileName().ToWide();
				m_tiDLS[iDls] = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
								name.c_str(), IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, iDls, TVI_ROOT, hDlsRoot);
				// Memorize Banks
				WORD wBanks[16];
				HTREEITEM hBanks[16];
				MemsetZero(wBanks);
				MemsetZero(hBanks);
				UINT nBanks = 0;
				// Add Drum Kits folder
				HTREEITEM hDrums = InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE,
						"Drum Kits", IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, 0, m_tiDLS[iDls], TVI_LAST);
				// Add Instruments
				UINT nInstr = pDlsBank->GetNumInstruments();
				for (UINT iIns=0; iIns<nInstr; iIns++)
				{
					DLSINSTRUMENT *pDlsIns = pDlsBank->GetInstrument(iIns);
					if (pDlsIns)
					{
						CHAR szName[256];
						wsprintf(szName, "%u: %s", pDlsIns->ulInstrument & 0x7F, pDlsIns->szName);
						// Drum Kit
						if (pDlsIns->ulBank & F_INSTRUMENT_DRUMS)
						{
							HTREEITEM hKit = InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM,
								szName, IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, pDlsIns->ulInstrument & 0x7F, hDrums, TVI_LAST);
							for (UINT iRgn=0; iRgn<pDlsIns->nRegions; iRgn++)
							{
								UINT keymin = pDlsIns->Regions[iRgn].uKeyMin;
								UINT keymax = pDlsIns->Regions[iRgn].uKeyMax;

								const CHAR *regionName = pDlsBank->GetRegionName(iIns, iRgn);
								if(regionName == nullptr && (keymin >= 24) && (keymin <= 84))
								{
									regionName = szMidiPercussionNames[keymin - 24];
								} else
								{
									regionName = "";
								}

								if (keymin >= keymax)
								{
									wsprintf(szName, "%s%u: %s", CSoundFile::m_NoteNames[keymin % 12], keymin / 12, regionName);
								} else
								{
									wsprintf(szName, "%s%u-%s%u: %s",
										CSoundFile::m_NoteNames[keymin % 12], keymin / 12,
										CSoundFile::m_NoteNames[keymax % 12], keymax / 12,
										regionName);
								}
								LPARAM lParam = DlsItem::EncodeValuePerc((uint8)(iRgn), (uint16)iIns);
								InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM,
										szName, IMAGE_INSTRUMENTS, IMAGE_INSTRUMENTS, 0, 0, lParam, hKit, TVI_LAST);
							}
							tvs.hParent = hKit;
							tvs.lpfnCompare = ModTreeDrumCompareProc;
							tvs.lParam = reinterpret_cast<LPARAM>(CTrackApp::gpDLSBanks[iDls]);
							SortChildrenCB(&tvs);
						} else
						// Melodic
						{
							HTREEITEM hbank = hBanks[0];
							UINT mbank = (pDlsIns->ulBank & 0x7F7F);
							UINT i;
							for (i=0; i<nBanks; i++)
							{
								if (wBanks[i] == mbank) break;
							}
							if (i < nBanks)
							{
								hbank = hBanks[i];
							} else
							if (nBanks < 16)
							{
								wsprintf(s, (mbank) ? "Melodic Bank %02X.%02X" : "Melodic", mbank >> 8, mbank & 0x7F);
								UINT j=0;
								while ((j<nBanks) && (mbank > wBanks[j])) j++;
								for (UINT k=nBanks; k>j; k--)
								{
									wBanks[k] = wBanks[k-1];
									hBanks[k] = hBanks[k-1];
								}
								hbank = InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE,
									s, IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, 0,
									m_tiDLS[iDls], (j > 0) ? hBanks[j-1] : TVI_FIRST);
								wBanks[j] = (WORD)mbank;
								hBanks[j] = hbank;
								nBanks++;
							}
							LPARAM lParam = DlsItem::EncodeValueInstr((pDlsIns->ulInstrument & 0x7F), (uint16)iIns);
							InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM,
								szName, IMAGE_INSTRUMENTS, IMAGE_INSTRUMENTS, 0, 0, lParam, hbank, TVI_LAST);
						}
					}
				}
				// Sort items
				for (UINT iBnk=0; iBnk<nBanks; iBnk++) if (hBanks[iBnk])
				{
					tvs.hParent = hBanks[iBnk];
					tvs.lpfnCompare = ModTreeInsLibCompareProc;
					tvs.lParam = (LPARAM)this;
					SortChildrenCB(&tvs);
				}
				if (hDrums != NULL)
				{
					tvs.hParent = hDrums;
					tvs.lpfnCompare = ModTreeInsLibCompareProc;
					tvs.lParam = (LPARAM)this;
					SortChildrenCB(&tvs);
				}
			}
			hDlsRoot = m_tiDLS[iDls];
		} else
		{
			if (m_tiDLS[iDls])
			{
				DeleteItem(m_tiDLS[iDls]);
				m_tiDLS[iDls] = nullptr;
			}
		}
	}
}


void CModTree::RefreshInstrumentLibrary()
//---------------------------------------
{
	SetRedraw(FALSE);
	EmptyInstrumentLibrary();
	FillInstrumentLibrary();
	SetRedraw(TRUE);
	if (m_pDataTree)
	{
		m_pDataTree->InsLibSetFullPath(m_InstrLibPath, m_SongFileName);
		m_pDataTree->RefreshInstrumentLibrary();
	}
}


void CModTree::UpdateView(ModTreeDocInfo &info, UpdateHint hint)
//--------------------------------------------------------------
{
	TCHAR s[256], stmp[256];
	TV_ITEM tvi;
	MemsetZero(tvi);
	const FlagSet<HintType> hintType = hint.GetType();
	if (IsSampleBrowser() || hintType == HINT_NONE) return;

	const CModDoc &modDoc = info.modDoc;
	const CSoundFile &sndFile = modDoc.GetrSoundFile();

	// Create headers
	s[0] = 0;
	const GeneralHint generalHint = hint.ToType<GeneralHint>();
	if(generalHint.GetType()[HINT_MODTYPE | HINT_MODGENERAL] || (!info.hSong))
	{
		// Module folder + sub folders
		std::wstring name = modDoc.GetPathNameMpt().GetFullFileName().ToWide();
		if(name.empty()) name = mpt::PathString::FromCStringSilent(modDoc.GetTitle()).SanitizeComponent().ToWide();

		if(!info.hSong)
		{
			info.hSong = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, name.c_str(), IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, (LPARAM)(DocInfo.size() - 1), TVI_ROOT, TVI_FIRST);
			info.hOrders = InsertItem(_T("Sequence"), IMAGE_FOLDER, IMAGE_FOLDER, info.hSong, TVI_LAST);
			info.hPatterns = InsertItem(_T("Patterns"), IMAGE_FOLDER, IMAGE_FOLDER, info.hSong, TVI_LAST);
			info.hSamples = InsertItem(_T("Samples"), IMAGE_FOLDER, IMAGE_FOLDER, info.hSong, TVI_LAST);
		} else if(generalHint.GetType()[HINT_MODGENERAL | HINT_MODTYPE])
		{
			if(name.c_str() != GetItemTextW(info.hSong))
			{
				SetItemText(info.hSong, name.c_str());
			}
		}
	}

	if (sndFile.GetModSpecifications().instrumentsMax > 0)
	{
		if(!info.hInstruments) info.hInstruments = InsertItem(_T("Instruments"), IMAGE_FOLDER, IMAGE_FOLDER, info.hSong, info.hSamples);
	} else
	{
		if(info.hInstruments)
		{
			DeleteItem(info.hInstruments);
			info.hInstruments = NULL;
		}
	}
	if (!info.hComments) info.hComments = InsertItem(_T("Comments"), IMAGE_COMMENTS, IMAGE_COMMENTS, info.hSong, TVI_LAST);
	// Add effects
	const PluginHint pluginHint = hint.ToType<PluginHint>();
	if (pluginHint.GetType()[HINT_MODTYPE | HINT_PLUGINNAMES])
	{
		HTREEITEM hItem = info.hEffects ? GetChildItem(info.hEffects) : nullptr;
		PLUGINDEX firstPlug = 0, lastPlug = MAX_MIXPLUGINS - 1;
		if(pluginHint.GetPlugin() && hItem)
		{
			// Only update one specific plugin name
			firstPlug = lastPlug = pluginHint.GetPlugin() - 1;
			while(hItem && GetItemData(hItem) != firstPlug)
			{
				hItem = GetNextSiblingItem(hItem);
			}
		}
		bool hasPlugs = false;
		for(PLUGINDEX i = firstPlug; i <= lastPlug; i++)
		{
			const SNDMIXPLUGIN &plugin = sndFile.m_MixPlugins[i];
			if(plugin.IsValidPlugin())
			{
				// Now we can be sure that we want to create this folder.
				if(!info.hEffects)
				{
					info.hEffects = InsertItem(_T("Plugins"), IMAGE_FOLDER, IMAGE_FOLDER, info.hSong, info.hInstruments ? info.hInstruments : info.hSamples);
				}

				wsprintf(s, "FX%u: %s", i + 1, plugin.GetName());
				int nImage = IMAGE_NOPLUGIN;
				if(plugin.pMixPlugin != nullptr) nImage = (plugin.pMixPlugin->IsInstrument()) ? IMAGE_PLUGININSTRUMENT : IMAGE_EFFECTPLUGIN;
				
				if(hItem)
				{
					// Replace existing item
					tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
					tvi.hItem = hItem;
					tvi.pszText = stmp;
					tvi.cchTextMax = CountOf(stmp);
					GetItem(&tvi);
					if(tvi.iImage != nImage || tvi.lParam != i || strcmp(s, tvi.pszText))
					{
						SetItem(hItem, TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, s, nImage, nImage, 0, 0, i);
					}
					hItem = GetNextSiblingItem(hItem);
				} else
				{
					InsertItem(TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, s, nImage, nImage, 0, 0, i, info.hEffects, TVI_LAST);
				}
				hasPlugs = true;
			}
		}
		if(!hasPlugs && firstPlug == lastPlug)
		{
			// If we only updated one plugin, we still need to check all the other slots if there is any plugin in them.
			for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
			{
				if(sndFile.m_MixPlugins[i].IsValidPlugin())
				{
					hasPlugs = true;
					break;
				}
			}
		}
		if(!hasPlugs && info.hEffects)
		{
			DeleteItem(info.hEffects);
			info.hEffects = nullptr;
		} else if(!pluginHint.GetPlugin())
		{
			// Delete superfluous tree items
			while(hItem)
			{
				HTREEITEM nextItem = GetNextSiblingItem(hItem);
				DeleteItem(hItem);
				hItem = nextItem;
			}
		}
	}
	// Add Orders
	const PatternHint patternHint = hint.ToType<PatternHint>();
	const SequenceHint seqHint = hint.ToType<SequenceHint>();
	if (info.hOrders && (seqHint.GetType()[HINT_MODTYPE | HINT_MODSEQUENCE | HINT_SEQNAMES] || patternHint.GetType()[HINT_PATNAMES]))
	{
		const PATTERNINDEX nPat = patternHint.GetPattern();
		bool adjustParentNode = false;	// adjust sequence name of "Sequence" node?

		// (only one seq remaining || previously only one sequence): update parent item
		if((info.tiSequences.size() > 1 && sndFile.Order.GetNumSequences() == 1) || (info.tiSequences.size() == 1 && sndFile.Order.GetNumSequences() > 1))
		{
			for(size_t nSeq = 0; nSeq < info.tiOrders.size(); nSeq++)
			{
				for(size_t nOrd = 0; nOrd < info.tiOrders[nSeq].size(); nOrd++) if (info.tiOrders[nSeq][nOrd])
				{
					if(info.tiOrders[nSeq][nOrd]) DeleteItem(info.tiOrders[nSeq][nOrd]); info.tiOrders[nSeq][nOrd] = NULL;
				}
				if(info.tiSequences[nSeq]) DeleteItem(info.tiSequences[nSeq]); info.tiSequences[nSeq] = NULL;
			}
			info.tiOrders.resize(sndFile.Order.GetNumSequences());
			info.tiSequences.resize(sndFile.Order.GetNumSequences(), NULL);
			adjustParentNode = true;
		}

		// If there are too many sequences, delete them.
		for(size_t nSeq = sndFile.Order.GetNumSequences(); nSeq < info.tiSequences.size(); nSeq++) if (info.tiSequences[nSeq])
		{
			for(size_t nOrd = 0; nOrd < info.tiOrders[nSeq].size(); nOrd++) if (info.tiOrders[nSeq][nOrd])
			{
				DeleteItem(info.tiOrders[nSeq][nOrd]); info.tiOrders[nSeq][nOrd] = NULL;
			}
			DeleteItem(info.tiSequences[nSeq]); info.tiSequences[nSeq] = NULL;
		}
		if (info.tiSequences.size() < sndFile.Order.GetNumSequences()) // Resize tiSequences if needed.
		{
			info.tiSequences.resize(sndFile.Order.GetNumSequences(), NULL);
			info.tiOrders.resize(sndFile.Order.GetNumSequences());
		}

		HTREEITEM hAncestorNode = info.hOrders;

		SEQUENCEINDEX nSeqMin = 0, nSeqMax = sndFile.Order.GetNumSequences() - 1;
		SEQUENCEINDEX nHintParam = seqHint.GetSequence();
		if (seqHint.GetType()[HINT_SEQNAMES] && (nHintParam <= nSeqMax)) nSeqMin = nSeqMax = nHintParam;

		// Adjust caption of the "Sequence" node (if only one sequence exists, it should be labeled with the sequence name)
		if((seqHint.GetType()[HINT_SEQNAMES] && sndFile.Order.GetNumSequences() == 1) || adjustParentNode)
		{
			CString seqName = sndFile.Order.GetSequence(0).GetName().c_str();
			if(seqName.IsEmpty() || sndFile.Order.GetNumSequences() > 1)
				seqName = _T("Sequence");
			else
				seqName = _T("Sequence: ") + seqName;
			SetItem(info.hOrders, TVIF_TEXT, seqName, 0, 0, 0, 0, 0);
		}

		// go through all sequences
		for(SEQUENCEINDEX nSeq = nSeqMin; nSeq <= nSeqMax; nSeq++)
		{
			if(sndFile.Order.GetNumSequences() > 1)
			{
				// more than one sequence -> add folder
				CString seqName;
				if(sndFile.Order.GetSequence(nSeq).GetName().empty())
					seqName.Format(_T("Sequence %u"), nSeq);
				else
					seqName.Format("%u: %s", nSeq, sndFile.Order.GetSequence(nSeq).GetName().c_str());

				UINT state = (nSeq == sndFile.Order.GetCurrentSequenceIndex()) ? TVIS_BOLD : 0;

				if(info.tiSequences[nSeq] == NULL)
				{
					info.tiSequences[nSeq] = InsertItem(seqName, IMAGE_FOLDER, IMAGE_FOLDER, info.hOrders, TVI_LAST);
				}
				// Update bold item
				strcpy(stmp, seqName);
				tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_STATE | TVIF_PARAM;
				tvi.state = 0;
				tvi.stateMask = TVIS_BOLD;
				tvi.hItem = info.tiSequences[nSeq];
				tvi.pszText = stmp;
				tvi.cchTextMax = CountOf(stmp);
				LPARAM param = (nSeq << SEQU_SHIFT) | ORDERINDEX_INVALID;
				GetItem(&tvi);
				if(tvi.state != state || tvi.pszText != seqName || tvi.lParam != param)
					SetItem(info.tiSequences[nSeq], TVIF_TEXT | TVIF_STATE | TVIF_PARAM, seqName, 0, 0, state, TVIS_BOLD, param);

				hAncestorNode = info.tiSequences[nSeq];
			}

			const ORDERINDEX ordLength = sndFile.Order.GetSequence(nSeq).GetLengthTailTrimmed();
			// If there are items past the new sequence length, delete them.
			for(size_t nOrd = ordLength; nOrd < info.tiOrders[nSeq].size(); nOrd++) if (info.tiOrders[nSeq][nOrd])
			{
				DeleteItem(info.tiOrders[nSeq][nOrd]); info.tiOrders[nSeq][nOrd] = NULL;
			}
			if (info.tiOrders[nSeq].size() < ordLength) // Resize tiOrders if needed.
				info.tiOrders[nSeq].resize(ordLength, nullptr);
			const bool patNamesOnly = patternHint.GetType()[HINT_PATNAMES];

			//if (hintFlagPart == HINT_PATNAMES) && (dwHintParam < sndFile.Order.size())) imin = imax = dwHintParam;
			for (ORDERINDEX iOrd = 0; iOrd < ordLength; iOrd++)
			{
				if(patNamesOnly && sndFile.Order.GetSequence(nSeq)[iOrd] != nPat)
					continue;
				UINT state = (iOrd == info.nOrdSel && nSeq == info.nSeqSel) ? TVIS_BOLD : 0;
				if (sndFile.Order.GetSequence(nSeq)[iOrd] < sndFile.Patterns.Size())
				{
					stmp[0] = 0;
					sndFile.Patterns[sndFile.Order.GetSequence(nSeq)[iOrd]].GetName(stmp);
					if (stmp[0])
					{
						wsprintf(s, (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_HEXDISPLAY) ? "[%02Xh] %u: %s" : "[%02u] %u: %s",
							iOrd, sndFile.Order.GetSequence(nSeq)[iOrd], stmp);
					} else
					{
						wsprintf(s, (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_HEXDISPLAY) ? _T("[%02Xh] Pattern %u") : _T("[%02u] Pattern %u"),
							iOrd, sndFile.Order.GetSequence(nSeq)[iOrd]);
					}
				} else
				{
					if(sndFile.Order.GetSequence(nSeq)[iOrd] == sndFile.Order.GetIgnoreIndex())
					{
						// +++ Item
						wsprintf(s, _T("[%02u] Skip"), iOrd);
					} else
					{
						// --- Item
						wsprintf(s, _T("[%02u] Stop"), iOrd);
					}
				}

				LPARAM param = (nSeq << SEQU_SHIFT) | iOrd;
				if (info.tiOrders[nSeq][iOrd])
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_STATE;
					tvi.state = 0;
					tvi.stateMask = TVIS_BOLD;
					tvi.hItem = info.tiOrders[nSeq][iOrd];
					tvi.pszText = stmp;
					tvi.cchTextMax = CountOf(stmp);
					GetItem(&tvi);
					if(tvi.state != state || strcmp(s, stmp))
						SetItem(info.tiOrders[nSeq][iOrd], TVIF_TEXT | TVIF_STATE | TVIF_PARAM, s, 0, 0, state, TVIS_BOLD, param);
				} else
				{
					info.tiOrders[nSeq][iOrd] = InsertItem(TVIF_HANDLE | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, s, IMAGE_PARTITION, IMAGE_PARTITION, 0, 0, param, hAncestorNode, TVI_LAST);
				}
			}
		}
	}
	// Add Patterns
	if (info.hPatterns && patternHint.GetType()[HINT_MODTYPE | HINT_PATNAMES] && sndFile.Patterns.Size() > 0)
	{
		const PATTERNINDEX nPat = patternHint.GetPattern();
		info.tiPatterns.resize(sndFile.Patterns.Size(), NULL);
		PATTERNINDEX imin = 0, imax = sndFile.Patterns.Size()-1;
		if (patternHint.GetType()[HINT_PATNAMES] && (nPat < sndFile.Patterns.Size())) imin = imax = nPat;
		bool bDelPat = false;

		ASSERT(info.tiPatterns.size() == sndFile.Patterns.Size());
		for(PATTERNINDEX iPat = imin; iPat <= imax; iPat++)
		{
			if ((bDelPat) && (info.tiPatterns[iPat]))
			{
				DeleteItem(info.tiPatterns[iPat]);
				info.tiPatterns[iPat] = NULL;
			}
			if (sndFile.Patterns[iPat])
			{
				stmp[0] = 0;
				sndFile.Patterns[iPat].GetName(stmp);
				if (stmp[0])
				{
					wsprintf(s, _T("%u: %s"), iPat, stmp);
				} else
				{
					wsprintf(s, _T("%u"), iPat);
				}
				if (info.tiPatterns[iPat])
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE;
					tvi.hItem = info.tiPatterns[iPat];
					tvi.pszText = stmp;
					tvi.cchTextMax = CountOf(stmp);
					GetItem(&tvi);
					if (strcmp(s, stmp)) SetItem(info.tiPatterns[iPat], TVIF_TEXT, s, 0, 0, 0, 0, 0);
				} else
				{
					info.tiPatterns[iPat] = InsertItem(s, IMAGE_PATTERNS, IMAGE_PATTERNS, info.hPatterns, TVI_LAST);
				}
				SetItemData(info.tiPatterns[iPat], iPat);
			} else
			{
				if (info.tiPatterns[iPat])
				{
					DeleteItem(info.tiPatterns[iPat]);
					info.tiPatterns[iPat] = NULL;
					bDelPat = true;
				}
			}
		}
	}
	// Add Samples
	const SampleHint sampleHint = hint.ToType<SampleHint>();
	if (info.hSamples && sampleHint.GetType()[HINT_MODTYPE | HINT_SMPNAMES | HINT_SAMPLEINFO | HINT_SAMPLEDATA])
	{
		const SAMPLEINDEX nSmp = sampleHint.GetSample();
		SAMPLEINDEX smin = 1, smax = MAX_SAMPLES - 1;
		if (sampleHint.GetType()[HINT_SMPNAMES] && (nSmp) && (nSmp < MAX_SAMPLES))
		{
			smin = smax = nSmp;
		}
		HTREEITEM hChild = GetNthChildItem(info.hSamples, smin - 1);
		for(SAMPLEINDEX nSmp = smin; nSmp <= smax; nSmp++)
		{
			HTREEITEM hNextChild = GetNextSiblingItem(hChild);
			if (nSmp <= sndFile.GetNumSamples())
			{
				const ModSample &sample = sndFile.GetSample(nSmp);
				const bool sampleExists = (sample.pSample != nullptr);
				int nImage = (sampleExists) ? IMAGE_SAMPLES : IMAGE_NOSAMPLE;
				if(sampleExists && info.samplesPlaying[nSmp]) nImage = IMAGE_SAMPLEACTIVE;
				if(info.modDoc.IsSampleMuted(nSmp)) nImage = IMAGE_SAMPLEMUTE;

				if(sndFile.GetType() == MOD_TYPE_MPT)
				{
					const TCHAR *status = _T("");
					if(sample.uFlags[SMP_KEEPONDISK])
					{
						status = sampleExists ? _T(" [external]") : _T(" [MISSING]");
					}
					wsprintf(s, _T("%3d: %s%s%s"), nSmp, sample.uFlags.test_all(SMP_MODIFIED | SMP_KEEPONDISK) ? _T("* ") : _T(""), sndFile.m_szNames[nSmp], status);
				} else
				{
					wsprintf(s, _T("%3d: %s"), nSmp, sndFile.m_szNames[nSmp]);
				}

				if (!hChild)
				{
					hChild = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, s, nImage, nImage, 0, 0, nSmp, info.hSamples, TVI_LAST);
				} else
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE;
					tvi.hItem = hChild;
					tvi.pszText = stmp;
					tvi.cchTextMax = CountOf(stmp);
					tvi.iImage = tvi.iSelectedImage = nImage;
					GetItem(&tvi);
					if(tvi.iImage != nImage || strcmp(s, stmp) || GetItemData(hChild) != nSmp)
					{
						SetItem(hChild, TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, s, nImage, nImage, 0, 0, nSmp);
					}
				}
			} else if(hChild != nullptr)
			{
				DeleteItem(hChild);
			} else
			{
				break;
			}
			hChild = hNextChild;
		}
	}
	// Add Instruments
	const InstrumentHint instrHint = hint.ToType<InstrumentHint>();
	if (info.hInstruments && instrHint.GetType()[HINT_MODTYPE | HINT_INSNAMES | HINT_INSTRUMENT])
	{
		INSTRUMENTINDEX smin = 1, smax = MAX_INSTRUMENTS - 1;
		const INSTRUMENTINDEX nIns = instrHint.GetInstrument();
		if (instrHint.GetType()[HINT_INSNAMES] && (nIns) && (nIns < MAX_INSTRUMENTS))
		{
			smin = smax = nIns;
		}
		HTREEITEM hChild = GetNthChildItem(info.hInstruments, smin - 1);
		for (INSTRUMENTINDEX nIns = smin; nIns <= smax; nIns++)
		{
			HTREEITEM hNextChild = GetNextSiblingItem(hChild);
			if (nIns <= sndFile.GetNumInstruments())
			{
				wsprintf(s, _T("%3u: %s"), nIns, sndFile.GetInstrumentName(nIns));

				int nImage = IMAGE_INSTRUMENTS;
				if(info.instrumentsPlaying[nIns]) nImage = IMAGE_INSTRACTIVE;
				if(!sndFile.Instruments[nIns] || info.modDoc.IsInstrumentMuted(nIns)) nImage = IMAGE_INSTRMUTE;

				if (!hChild)
				{
					hChild = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, s, nImage, nImage, 0, 0, nIns, info.hInstruments, TVI_LAST);
				} else
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE;
					tvi.hItem = hChild;
					tvi.pszText = stmp;
					tvi.cchTextMax = CountOf(stmp);
					tvi.iImage = tvi.iSelectedImage = nImage;
					GetItem(&tvi);
					if(tvi.iImage != nImage || strcmp(s, stmp) || GetItemData(hChild) != nIns)
					{
						SetItem(hChild, TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, s, nImage, nImage, 0, 0, nIns);
					}
				}
			} else if(hChild != nullptr)
			{
				DeleteItem(hChild);
			} else
			{
				break;
			}
			hChild = hNextChild;
		}
	}
}


CModTree::ModItem CModTree::GetModItem(HTREEITEM hItem)
//-----------------------------------------------------
{
	if (!hItem) return ModItem(MODITEM_NULL);
	// First, test root items
	if (hItem == m_hInsLib) return ModItem(MODITEM_HDR_INSTRUMENTLIB);
	if (hItem == m_hMidiLib) return ModItem(MODITEM_HDR_MIDILIB);

	// The immediate parent of the item (NULL if this item is on the root level of the tree)
	HTREEITEM hItemParent = GetParentItem(hItem);
	// Parent of the parent.
	HTREEITEM hItemParentParent = GetParentItem(hItemParent);
	// Get the root parent of the selected item, which can be the item itself.
	HTREEITEM hRootParent = hItem;
	if(!IsSampleBrowser())
	{
		hRootParent = GetParentRootItem(hItem);
	}

	uint32 itemData = static_cast<uint32>(GetItemData(hItem));
	uint32 rootItemData = static_cast<uint32>(GetItemData(hRootParent));

	// Midi Library
	if(hRootParent == m_hMidiLib && hRootParent != hItem && !IsSampleBrowser())
	{
		return ModItem(static_cast<ModItemType>(itemData >> MIDILIB_SHIFT), itemData & MIDILIB_MASK);
	}
	// Instrument Library
	if(hRootParent == m_hInsLib || (IsSampleBrowser() && hItem != m_hInsLib))
	{
		TV_ITEM tvi;
		tvi.mask = TVIF_IMAGE|TVIF_HANDLE;
		tvi.hItem = hItem;
		tvi.iImage = 0;
		if (GetItem(&tvi))
		{
			// Sample ?
			if (tvi.iImage == IMAGE_SAMPLES) return ModItem(MODITEM_INSLIB_SAMPLE);
			// Instrument ?
			if (tvi.iImage == IMAGE_INSTRUMENTS) return ModItem(MODITEM_INSLIB_INSTRUMENT);
			// Song ?
			if (tvi.iImage == IMAGE_FOLDERSONG) return ModItem(MODITEM_INSLIB_SONG);
			return ModItem(MODITEM_INSLIB_FOLDER);
		}
		return ModItem(MODITEM_NULL);
	}
	if (IsSampleBrowser()) return ModItem(MODITEM_NULL);
	// Songs
	if(rootItemData < DocInfo.size())
	{
		m_nDocNdx = rootItemData;
		ModTreeDocInfo *pInfo = DocInfo[rootItemData];

		if(hItem == pInfo->hSong) return ModItem(MODITEM_HDR_SONG);
		if(hRootParent == pInfo->hSong)
		{
			if (hItem == pInfo->hPatterns) return ModItem(MODITEM_HDR_PATTERNS);
			if (hItem == pInfo->hOrders) return ModItem(MODITEM_HDR_ORDERS);
			if (hItem == pInfo->hSamples) return ModItem(MODITEM_HDR_SAMPLES);
			if (hItem == pInfo->hInstruments) return ModItem(MODITEM_HDR_INSTRUMENTS);
			if (hItem == pInfo->hComments) return ModItem(MODITEM_COMMENTS);
			// Order List or Sequence item?
			if ((hItemParent == pInfo->hOrders) || (hItemParentParent == pInfo->hOrders))
			{
				const ORDERINDEX ord = (ORDERINDEX)(itemData & SEQU_MASK);
				const SEQUENCEINDEX seq = (SEQUENCEINDEX)(itemData >> SEQU_SHIFT);
				if(ord == ORDERINDEX_INVALID)
				{
					return ModItem(MODITEM_SEQUENCE, seq);
				} else
				{
					return ModItem(MODITEM_ORDER, ord, seq);
				}
			}

			ModItem modItem(MODITEM_NULL, itemData);
			if (hItemParent == pInfo->hPatterns)
			{
				// Pattern
				modItem.type = MODITEM_PATTERN;
			} else if (hItemParent == pInfo->hSamples)
			{
				// Sample
				modItem.type = MODITEM_SAMPLE;
			} else if (hItemParent == pInfo->hInstruments)
			{
				// Instrument
				modItem.type = MODITEM_INSTRUMENT;
			} else if (hItemParent == pInfo->hEffects)
			{
				// Effect
				modItem.type = MODITEM_EFFECT;
			}
			return modItem;
		}
	}

	// DLS banks
	if(itemData < m_tiDLS.size() && hItem == m_tiDLS[itemData])
		return ModItem(MODITEM_DLSBANK_FOLDER, itemData);

	// DLS Instruments
	if(hRootParent != nullptr)
	{
		if(rootItemData < m_tiDLS.size() && m_tiDLS[rootItemData] == hRootParent)
		{
			if (hItem == m_tiDLS[rootItemData])
				return ModItem(MODITEM_DLSBANK_FOLDER, (uint32)rootItemData);

			if ((itemData & DLS_TYPEMASK) == DLS_TYPEPERC
				|| (itemData & DLS_TYPEMASK) == DLS_TYPEINST)
			{
				return DlsItem(rootItemData, itemData);
			}
		}
	}
	return ModItem(MODITEM_NULL);
}


BOOL CModTree::ExecuteItem(HTREEITEM hItem)
//-----------------------------------------
{
	if (hItem)
	{
		const ModItem modItem = GetModItem(hItem);
		uint32 modItemID = modItem.val1;
		CModDoc *modDoc = m_nDocNdx < DocInfo.size() ? &(DocInfo[m_nDocNdx]->modDoc) : nullptr;

		switch(modItem.type)
		{
		case MODITEM_COMMENTS:
			if (modDoc) modDoc->ActivateView(IDD_CONTROL_COMMENTS, 0);
			return TRUE;

		/*case MODITEM_SEQUENCE:
			if (modDoc) modDoc->ActivateView(IDD_CONTROL_PATTERNS, (dwItem << SEQU_SHIFT) | SEQU_INDICATOR);
			return TRUE;*/

		case MODITEM_ORDER:
			if (modDoc) modDoc->ActivateView(IDD_CONTROL_PATTERNS, modItemID | (uint32(modItem.val2) << SEQU_SHIFT) | SEQU_INDICATOR);
			return TRUE;

		case MODITEM_PATTERN:
			if (modDoc) modDoc->ActivateView(IDD_CONTROL_PATTERNS, modItemID);
			return TRUE;

		case MODITEM_SAMPLE:
			if (modDoc) modDoc->ActivateView(IDD_CONTROL_SAMPLES, modItemID);
			return TRUE;

		case MODITEM_INSTRUMENT:
			if (modDoc) modDoc->ActivateView(IDD_CONTROL_INSTRUMENTS, modItemID);
			return TRUE;

		case MODITEM_MIDIPERCUSSION:
			modItemID |= 0x80;
		case MODITEM_MIDIINSTRUMENT:
			OpenMidiInstrument(modItemID);
			return TRUE;

		case MODITEM_EFFECT:
		case MODITEM_INSLIB_SAMPLE:
		case MODITEM_INSLIB_INSTRUMENT:
			PlayItem(hItem, NOTE_MIDDLEC);
			return TRUE;

		case MODITEM_INSLIB_SONG:
		case MODITEM_INSLIB_FOLDER:
			InstrumentLibraryChDir(mpt::PathString::FromCStringW(GetItemTextW(hItem)), modItem.type == MODITEM_INSLIB_SONG);
			return TRUE;

		case MODITEM_HDR_SONG:
			if (modDoc) modDoc->ActivateWindow();
			return TRUE;

		case MODITEM_DLSBANK_INSTRUMENT:
			PlayItem(hItem, NOTE_MIDDLEC);
			return TRUE;

		case MODITEM_HDR_INSTRUMENTLIB:
			if(IsSampleBrowser())
			{
				BrowseForFolder dlg(m_InstrLibPath, _T("Select a new instrument library folder..."));
				if(dlg.Show())
				{
					mpt::PathString dir = dlg.GetDirectory();
					dir.EnsureTrailingSlash();
					CMainFrame::GetMainFrame()->GetUpperTreeview()->InstrumentLibraryChDir(dir, false);
				}
			}
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CModTree::PlayItem(HTREEITEM hItem, ModCommand::NOTE nParam)
//---------------------------------------------------------------
{
	if (hItem)
	{
		const ModItem modItem = GetModItem(hItem);
		uint32 modItemID = modItem.val1;
		CModDoc *modDoc = m_nDocNdx < DocInfo.size() ? &(DocInfo[m_nDocNdx]->modDoc) : nullptr;

		switch(modItem.type)
		{
		case MODITEM_SAMPLE:
			if (modDoc)
			{
				if (nParam == NOTE_NOTECUT)
				{
					modDoc->NoteOff(0, true); // cut previous playing samples
				} else if (nParam & 0x80)
				{
					modDoc->NoteOff(nParam & 0x7F, true);
				} else
				{
					modDoc->NoteOff(0, true); // cut previous playing samples
					modDoc->PlayNote(nParam & 0x7F, 0, static_cast<SAMPLEINDEX>(modItemID), false);
				}
			}
			return TRUE;

		case MODITEM_INSTRUMENT:
			if (modDoc)
			{
				if (nParam == NOTE_NOTECUT)
				{
					modDoc->NoteOff(0, true);
				} else if (nParam & 0x80)
				{
					modDoc->NoteOff(nParam & 0x7F, true);
				} else
				{
					modDoc->NoteOff(0, true);
					modDoc->PlayNote(nParam & 0x7F, static_cast<INSTRUMENTINDEX>(modItemID), 0, false);
				}
			}
			return TRUE;

		case MODITEM_EFFECT:
			if ((modDoc) && (modItemID < MAX_MIXPLUGINS))
			{/*
				CSoundFile *pSndFile = pModDoc->GetSoundFile();
				PSNDMIXPLUGIN pPlugin = &pSndFile->m_MixPlugins[dwItem];
				if (pPlugin->pMixPlugin)
				{
					CVstPlugin *pVstPlugin = (CVstPlugin *)pPlugin->pMixPlugin;
					pVstPlugin->ToggleEditor();
				}*/
				modDoc->TogglePluginEditor(modItemID);
			}
			return TRUE;

		case MODITEM_INSLIB_SAMPLE:
		case MODITEM_INSLIB_INSTRUMENT:
			if(nParam != NOTE_NOTECUT)
			{
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				if(!m_SongFileName.empty())
				{
					// Preview sample / instrument in module
					char szName[16];
					mpt::String::CopyN(szName, GetItemText(hItem));
					const size_t n = ConvertStrTo<size_t>(szName);
					if (pMainFrm && m_SongFile)
					{
						if (modItem.type == MODITEM_INSLIB_INSTRUMENT)
						{
							pMainFrm->PlaySoundFile(*m_SongFile, static_cast<INSTRUMENTINDEX>(n), SAMPLEINDEX_INVALID, nParam);
						} else
						{
							pMainFrm->PlaySoundFile(*m_SongFile, INSTRUMENTINDEX_INVALID, static_cast<SAMPLEINDEX>(n), nParam);
						}
					}
				} else
				{
					// Preview sample / instrument file
					if (pMainFrm) pMainFrm->PlaySoundFile(InsLibGetFullPath(hItem), nParam);
				}
			} else
			{
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				if (pMainFrm) pMainFrm->StopPreview();
			}

			return TRUE;

		case MODITEM_MIDIPERCUSSION:
			modItemID |= 0x80;
		case MODITEM_MIDIINSTRUMENT:
			{
				MIDILIBSTRUCT &midiLib = CTrackApp::GetMidiLibrary();
				if(modItemID < CountOf(midiLib.MidiMap) && !midiLib.MidiMap[modItemID].empty())
				{
					CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
					if (pMainFrm) pMainFrm->PlaySoundFile(midiLib.MidiMap[modItemID], static_cast<ModCommand::NOTE>(nParam));
				}
			}
			return TRUE;

		case MODITEM_DLSBANK_INSTRUMENT:
			{
				const DlsItem &item = *static_cast<const DlsItem *>(&modItem);
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				uint32 bank = item.GetBankIndex();
				if ((bank < CTrackApp::gpDLSBanks.size()) && (CTrackApp::gpDLSBanks[bank]) && (pMainFrm))
				{
					CDLSBank *pDLSBank = CTrackApp::gpDLSBanks[bank];
					UINT rgn = 0, instr = item.GetInstr();
					// Drum
					if (item.IsPercussion())
					{
						rgn = item.GetRegion();
					} else
					// Melodic
					if (item.IsInstr())
					{
						rgn = pDLSBank->GetRegionFromKey(instr, nParam - NOTE_MIN);
					}
					pMainFrm->PlayDLSInstrument(bank, instr, rgn, static_cast<ModCommand::NOTE>(nParam));
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}


BOOL CModTree::SetMidiInstrument(UINT nIns, const mpt::PathString &fileName)
//--------------------------------------------------------------------------
{
	MIDILIBSTRUCT &midiLib = CTrackApp::GetMidiLibrary();
	if(nIns < 128)
	{
		midiLib.MidiMap[nIns] = fileName;
		RefreshMidiLibrary();
		return TRUE;
	}
	return FALSE;
}


BOOL CModTree::SetMidiPercussion(UINT nPerc, const mpt::PathString &fileName)
//---------------------------------------------------------------------------
{
	MIDILIBSTRUCT &midiLib = CTrackApp::GetMidiLibrary();
	if(nPerc < 128)
	{
		UINT nIns = nPerc | 0x80;
		midiLib.MidiMap[nIns] = fileName;
		RefreshMidiLibrary();
		return TRUE;
	}
	return FALSE;
}


void CModTree::DeleteTreeItem(HTREEITEM hItem)
//--------------------------------------------
{
	const ModItem modItem = GetModItem(hItem);
	uint32 modItemID = modItem.val1;
	TCHAR s[64];

	CModDoc *modDoc = m_nDocNdx < DocInfo.size() ? &(DocInfo[m_nDocNdx]->modDoc) : nullptr;
	if(modItem.IsSongItem() && modDoc == nullptr)
	{
		return;
	}

	switch(modItem.type)
	{
	case MODITEM_SEQUENCE:
		wsprintf(s, _T("Remove sequence %u?"), modItemID);
		if(Reporting::Confirm(s, false, true) == cnfNo) break;
		modDoc->GetrSoundFile().Order.RemoveSequence((SEQUENCEINDEX)(modItemID));
		modDoc->UpdateAllViews(nullptr, SequenceHint().Data());
		break;

	case MODITEM_ORDER:
		// might be slightly annoying to ask for confirmation here, and it's rather easy to restore the orderlist anyway.
		if(modDoc->RemoveOrder((SEQUENCEINDEX)(modItem.val2), (ORDERINDEX)(modItem.val1)))
		{
			modDoc->UpdateAllViews(nullptr, SequenceHint().Data());
		}
		break;

	case MODITEM_PATTERN:
		{
			CSoundFile &sndFile = modDoc->GetrSoundFile();
			bool isUsed = false;
			// First, find all used patterns in all sequences.
			for(SEQUENCEINDEX seq = 0; seq < sndFile.Order.GetNumSequences(); seq++)
			{
				const auto sequence = sndFile.Order.GetSequence(seq);
				const ORDERINDEX ordLength = sequence.GetLengthTailTrimmed();
				for(ORDERINDEX ord = 0; ord < ordLength; ord++)
				{
					if(sequence[ord] == modItemID)
					{
						isUsed = true;
						break;
					}
				}
			}
			wsprintf(s, _T("Remove pattern %u?\nThis pattern is currently%s used."), modItemID, isUsed ? _T("") : _T(" not"));
			if(Reporting::Confirm(s, false, isUsed) == cnfYes && modDoc->RemovePattern((PATTERNINDEX)modItemID))
			{
				modDoc->UpdateAllViews(nullptr, PatternHint((PATTERNINDEX)modItemID).Data().Names());
			}
		}
		break;

	case MODITEM_SAMPLE:
		wsprintf(s, _T("Remove sample %u?"), modItemID);
		if(!modDoc->GetrSoundFile().GetSample(static_cast<SAMPLEINDEX>(modItemID)).HasSampleData() || Reporting::Confirm(s, false, true) == cnfYes)
		{
			modDoc->GetSampleUndo().PrepareUndo((SAMPLEINDEX)modItemID, sundo_replace, "Delete");
			const SAMPLEINDEX oldNumSamples = modDoc->GetNumSamples();
			if (modDoc->RemoveSample((SAMPLEINDEX)modItemID))
			{
				modDoc->UpdateAllViews(nullptr, SampleHint(modDoc->GetNumSamples() != oldNumSamples ? 0 : (SAMPLEINDEX)modItemID).Info().Data().Names());
			}
		}
		break;

	case MODITEM_INSTRUMENT:
		wsprintf(s, _T("Remove instrument %u?"), modItemID);
		if(Reporting::Confirm(s, false, true) == cnfYes)
		{
			const INSTRUMENTINDEX oldNumInstrs = modDoc->GetNumInstruments();
			if(modDoc->RemoveInstrument((INSTRUMENTINDEX)modItemID))
			{
				modDoc->UpdateAllViews(nullptr, InstrumentHint(modDoc->GetNumInstruments() != oldNumInstrs ? 0 : INSTRUMENTINDEX(modItemID)).Info().Envelope().ModType());
			}
		}
		break;

	case MODITEM_MIDIINSTRUMENT:
		SetMidiInstrument(modItemID, MPT_PATHSTRING(""));
		RefreshMidiLibrary();
		break;
	case MODITEM_MIDIPERCUSSION:
		SetMidiPercussion(modItemID, MPT_PATHSTRING(""));
		RefreshMidiLibrary();
		break;

	case MODITEM_DLSBANK_FOLDER:
		CTrackApp::RemoveDLSBank(modItemID);
		RefreshDlsBanks();
		break;

	case MODITEM_INSLIB_SONG:
	case MODITEM_INSLIB_SAMPLE:
	case MODITEM_INSLIB_INSTRUMENT:
		{
			// Create double-null-terminated path
			const std::wstring fullPath = InsLibGetFullPath(hItem).ToWide() + L'\0';
			SHFILEOPSTRUCTW fos;
			MemsetZero(fos);
			fos.hwnd = m_hWnd;
			fos.wFunc = FO_DELETE;
			fos.pFrom = fullPath.c_str();
			fos.fFlags = CMainFrame::GetInputHandler()->ShiftPressed() ? 0 : FOF_ALLOWUNDO;
			if ((0 == SHFileOperationW(&fos)) && (!fos.fAnyOperationsAborted)) RefreshInstrumentLibrary();
		}
		break;
	}
}


BOOL CModTree::OpenTreeItem(HTREEITEM hItem)
//------------------------------------------
{
	const ModItem modItem = GetModItem(hItem);

	switch(modItem.type)
	{
	case MODITEM_INSLIB_SONG:
		{
			theApp.OpenDocumentFile(InsLibGetFullPath(hItem));
		}
		break;
	}
	return TRUE;
}


BOOL CModTree::OpenMidiInstrument(DWORD dwItem)
//---------------------------------------------
{
	std::vector<FileType> mediaFoundationTypes = CSoundFile::GetMediaFoundationFileTypes();
	FileDialog dlg = OpenFileDialog()
		.EnableAudioPreview()
		.ExtensionFilter(
			"All Instruments and Banks|*.xi;*.pat;*.iti;*.wav;*.aif;*.aiff;*.sf2;*.sbk;*.dls;*.flac;*.opus;*.ogg;*.oga;*.mp1;*.mp2;*.mp3" + ToFilterOnlyString(mediaFoundationTypes, true).ToLocale() + "|"
			"FastTracker II Instruments (*.xi)|*.xi|"
			"GF1 Patches (*.pat)|*.pat|"
			"Wave Files (*.wav)|*.wav|"
	#ifdef MPT_WITH_FLAC
			"FLAC Files (*.flac,*.oga)|*.flac;*.oga|"
	#endif // MPT_WITH_FLAC
	#if defined(MPT_WITH_OPUSFILE)
			"Opus Files (*.opus,*.oga)|*.opus;*.oga|"
	#endif // MPT_WITH_OPUSFILE
	#if defined(MPT_WITH_VORBISFILE) || defined(MPT_WITH_STBVORBIS)
			"Ogg Vorbis Files (*.ogg,*.oga)|*.ogg;*.oga|"
	#endif // VORBIS
	#if defined(MPT_ENABLE_MP3_SAMPLES)
			"MPEG Files (*.mp1,*.mp2,*.mp3)|*.mp1;*.mp2;*.mp3|"
	#endif // MPT_ENABLE_MP3_SAMPLES
	#if defined(MPT_WITH_MEDIAFOUNDATION)
			+ ToFilterString(mediaFoundationTypes, FileTypeFormatShowExtensions).ToLocale() +
	#endif
			"Impulse Tracker Instruments (*.iti)|*.iti;*.its|"
			"SoundFont 2.0 Banks (*.sf2)|*.sf2;*.sbk|"
			"DLS Sound Banks (*.dls)|*.dls|"
			"All Files (*.*)|*.*||");
	if(!dlg.Show()) return FALSE;

	if (dwItem & 0x80)
		return SetMidiPercussion(dwItem & 0x7F, dlg.GetFirstFile());
	else
		return SetMidiInstrument(dwItem, dlg.GetFirstFile());
}


// Empty Instrument Library
void CModTree::EmptyInstrumentLibrary()
//-------------------------------------
{
	HTREEITEM h;
	if (!m_hInsLib) return;
	if (!IsSampleBrowser())
	{
		DeleteChildren(m_hInsLib);
	} else
	{
		while ((h = GetNextItem(m_hInsLib, TVGN_NEXT)) != NULL)
		{
			DeleteItem(h);
		}
	}
}


bool CModTree::IsMediaFoundationExtension(const std::string &ext) const
//---------------------------------------------------------------------
{
	for(std::size_t i = 0; i < m_MediaFoundationExtensions.size(); ++i)
	{
		if(m_MediaFoundationExtensions[i].empty())
		{
			continue;
		}
		if(m_MediaFoundationExtensions[i].ToLocale() == ext)
		{
			return true;
		}
	}
	return false;
}

// Refresh Instrument Library
void CModTree::FillInstrumentLibrary()
//------------------------------------
{
	if (!m_hInsLib) return;

	SetRedraw(FALSE);
	if(!m_SongFileName.empty() && IsSampleBrowser() && m_SongFile)
	{
		// Fill browser with samples / instruments of module file
		SetItemText(m_hInsLib, m_SongFileName.AsNative().c_str());
		SetItemImage(m_hInsLib, IMAGE_FOLDERSONG, IMAGE_FOLDERSONG);
		for(INSTRUMENTINDEX ins = 1; ins <= m_SongFile->GetNumInstruments(); ins++)
		{
			ModInstrument *pIns = m_SongFile->Instruments[ins];
			if(pIns)
			{
				WCHAR s[MAX_INSTRUMENTNAME + 10];
				swprintf(s, CountOf(s), L"%3d: %s", ins, mpt::ToWide(mpt::CharsetLocale, pIns->name).c_str());
				ModTreeInsert(s, IMAGE_INSTRUMENTS);
			}
		}
		for(SAMPLEINDEX smp = 1; smp <= m_SongFile->GetNumSamples(); smp++)
		{
			const ModSample &sample = m_SongFile->GetSample(smp);
			if(sample.pSample)
			{
				WCHAR s[MAX_SAMPLENAME + 10];
				swprintf(s, CountOf(s), L"%3d: %s", smp, mpt::ToWide(mpt::CharsetLocale, m_SongFile->m_szNames[smp]).c_str());
				ModTreeInsert(s, IMAGE_SAMPLES);
			}
		}
	} else
	{
		std::wstring text;
		if(!IsSampleBrowser())
		{
			text = L"Instrument Library (" + m_InstrLibPath.ToWide() + L")";
			SetItemText(m_hInsLib, text.c_str());
		} else
		{
			SetItemText(m_hInsLib, m_InstrLibPath.ToWide().c_str());
			SetItemImage(m_hInsLib, IMAGE_FOLDER, IMAGE_FOLDER);
		}

		// Enumerating Drives...
		if(!IsSampleBrowser())
		{
			CImageList &images = CMainFrame::GetMainFrame()->m_MiscIcons;
			// Avoid adding the same images again and again...
			images.SetImageCount(IMGLIST_NUMIMAGES);

			WCHAR s[16];
			wcscpy(s, L"?:\\");
			for(UINT iDrive = 'A'; iDrive <= 'Z'; iDrive++)
			{
				s[0] = (WCHAR)iDrive;
				UINT nDriveType = GetDriveTypeW(s);
				if(nDriveType != DRIVE_UNKNOWN && nDriveType != DRIVE_NO_ROOT_DIR)
				{
					SHFILEINFOW fileInfo;
					SHGetFileInfoW(s, 0, &fileInfo, sizeof(fileInfo), SHGFI_ICON | SHGFI_SMALLICON);
					ModTreeInsert(s, images.Add(fileInfo.hIcon));
					DestroyIcon(fileInfo.hIcon);
				}
			}
		}

		std::vector<const char *> modExts = CSoundFile::GetSupportedExtensions(false);

		// Enumerating Directories and samples/instruments
		const mpt::PathString path = m_InstrLibPath + MPT_PATHSTRING("*.*");
		const bool showDirs = !IsSampleBrowser() || TrackerSettings::Instance().showDirsInSampleBrowser, showInstrs = IsSampleBrowser();

		HANDLE hFind;
		WIN32_FIND_DATAW wfd;
		MemsetZero(wfd);
		if((hFind = FindFirstFileW(path.AsNative().c_str(), &wfd)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				// Up Directory
				if(!wcscmp(wfd.cFileName, L".."))
				{
					if(showDirs)
					{
						ModTreeInsert(wfd.cFileName, IMAGE_FOLDERPARENT);
					}
				} else if (wfd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_SYSTEM))
				{
					// Ignore these files
					continue;
				} else if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// Directory
					if(wcscmp(wfd.cFileName, L".") && showDirs)
					{
						ModTreeInsert(wfd.cFileName, IMAGE_FOLDER);
					}
				} else if(wfd.nFileSizeHigh > 0 || wfd.nFileSizeLow >= 16)
				{
					// Get lower-case file extension without dot.
					const std::string ext = mpt::ToCharset(mpt::CharsetUTF8, mpt::PathString::FromNative(wfd.cFileName).GetFileExt().ToWide());
					char s[16];
					mpt::String::Copy(s, ext);

					if(s[0])
					{
						const size_t len = strlen(s);
						for(size_t i = 0; i < len; i++)
						{
							s[i] = (char)tolower(s[i + 1]);
						}
					}

					// Instruments
					if ((!strcmp(s, "xi"))
						|| (!strcmp(s, "iti"))
						|| (!strcmp(s, "sf2"))
						|| (!strcmp(s, "sbk"))
						|| (!strcmp(s, "dls"))
						|| (!strcmp(s, "pat")) )
					{
						if (showInstrs)
						{
							ModTreeInsert(wfd.cFileName, IMAGE_INSTRUMENTS);
						}
					} else if(std::find_if(modExts.begin(), modExts.end(),
						[&s] (const char *str) -> bool { return !strcmp(s, str); }) != modExts.end())
					{
						// Songs
						if (showDirs)
						{
							ModTreeInsert(wfd.cFileName, IMAGE_FOLDERSONG);
						}
					} else
					// Samples
					if(!strcmp(s, "wav")
						|| !strcmp(s, "flac")
						|| !strcmp(s, "mp1")
						|| !strcmp(s, "mp2")
						|| !strcmp(s, "mp3")
						|| !strcmp(s, "smp")
						|| !strcmp(s, "raw")
						|| !strcmp(s, "s3i")
						|| !strcmp(s, "its")
						|| !strcmp(s, "aif")
						|| !strcmp(s, "aiff")
						|| !strcmp(s, "snd")
						|| !strcmp(s, "svx")
						|| !strcmp(s, "voc")
						|| !strcmp(s, "8sv")
						|| !strcmp(s, "8svx")
						|| !strcmp(s, "16sv")
						|| !strcmp(s, "16svx")
						|| IsMediaFoundationExtension(s)
						|| (m_bShowAllFiles // Exclude the extensions below
							&& strcmp(s, "txt")
							&& strcmp(s, "diz")
							&& strcmp(s, "nfo")
							&& strcmp(s, "doc")
							&& strcmp(s, "ini")
							&& strcmp(s, "pdf")
							&& strcmp(s, "zip")
							&& strcmp(s, "rar")
							&& strcmp(s, "lha")
							&& strcmp(s, "exe")
							&& strcmp(s, "dll")
							&& strcmp(s, "mol"))
						)
					{
						if (showInstrs)
						{
							ModTreeInsert(wfd.cFileName, IMAGE_SAMPLES);
						}
					}
				}
			} while (FindNextFileW(hFind, &wfd));
			FindClose(hFind);
		}
	}
	
	// Sort items
	TV_SORTCB tvs;
	tvs.hParent = (!IsSampleBrowser()) ? m_hInsLib : TVI_ROOT;
	tvs.lpfnCompare = ModTreeInsLibCompareProc;
	tvs.lParam = (LPARAM)this;
	SortChildren(tvs.hParent);
	SortChildrenCB(&tvs);
	SetRedraw(TRUE);

	{
		MPT_LOCK_GUARD<mpt::mutex> l(m_WatchDirMutex);
		if(m_InstrLibPath != m_WatchDir)
		{
			m_WatchDir = m_InstrLibPath;
			SetEvent(m_hSwitchWatchDir);
		}
	}
}


// Monitor changes in the instrument library folder.
void CModTree::MonitorInstrumentLibrary()
//---------------------------------------
{
	mpt::SetCurrentThreadPriority(mpt::ThreadPriorityLowest);
	DWORD result;
	mpt::PathString lastWatchDir;
	HANDLE hWatchDir = INVALID_HANDLE_VALUE;
	DWORD lastRefresh = GetTickCount();
	DWORD timeout = INFINITE;
	DWORD interval = TrackerSettings::Instance().GUIUpdateInterval;
	do
	{
		{
			MPT_LOCK_GUARD<mpt::mutex> l(m_WatchDirMutex);
			if(m_WatchDir != lastWatchDir)
			{
				if(hWatchDir != INVALID_HANDLE_VALUE)
				{
					FindCloseChangeNotification(hWatchDir);
					hWatchDir = INVALID_HANDLE_VALUE;
					lastWatchDir = mpt::PathString();
				}
				if(!m_WatchDir.empty())
				{
					hWatchDir = FindFirstChangeNotificationW(m_WatchDir.AsNative().c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
					lastWatchDir = m_WatchDir;
				}
			}
		}
		const HANDLE waitHandles[3] = { m_hWatchDirKillThread, m_hSwitchWatchDir, hWatchDir };
		result = WaitForMultipleObjects(hWatchDir != INVALID_HANDLE_VALUE ? 3 : 2, waitHandles, FALSE, timeout);
		DWORD now = GetTickCount();
		if(result == WAIT_TIMEOUT)
		{
			PostMessage(WM_COMMAND, ID_MODTREE_REFRESHINSTRLIB);
			lastRefresh = now;
			timeout = INFINITE;
		} else if(result == WAIT_OBJECT_0 + 1)
		{
			// nothing
			// will switch to new dir in next loop iteration
		} else if(result == WAIT_OBJECT_0 + 2)
		{
			FindNextChangeNotification(hWatchDir);
			timeout = 0; // update timeout later	
		}
		if(timeout != INFINITE)
		{
			// Update timeout. Can happen either because we just got a change event,
			// or because we got a directory switch event and still have a change
			// notification pending.
			if(now - lastRefresh >= interval)
			{
				PostMessage(WM_COMMAND, ID_MODTREE_REFRESHINSTRLIB);
				lastRefresh = now;
				timeout = INFINITE;
			} else
			{
				timeout = lastRefresh + interval - now;
			}
		}
	} while(result != WAIT_OBJECT_0);
	if(hWatchDir != INVALID_HANDLE_VALUE)
	{
		FindCloseChangeNotification(hWatchDir);
		hWatchDir = INVALID_HANDLE_VALUE;
		lastWatchDir = mpt::PathString();
	}
}


// Insert sample browser item.
void CModTree::ModTreeInsert(const WCHAR *name, int image)
//--------------------------------------------------------
{
	DWORD dwId = 0;
	switch(image)
	{
	case IMAGE_FOLDERPARENT:
		dwId = 1;
		break;
	case IMAGE_FOLDER:
		dwId = 2;
		break;
	case IMAGE_FOLDERSONG:
		dwId = 3;
		break;
	case IMAGE_SAMPLES:
		if(!m_SongFileName.empty()) { dwId = 5; break; }
	case IMAGE_INSTRUMENTS:
		dwId = 4;
		break;
	}
	InsertItem(TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT,
		name,
		image, image,
		0, 0,
		(LPARAM)dwId,
		(!IsSampleBrowser()) ? m_hInsLib : TVI_ROOT,
		TVI_LAST);
}


int CALLBACK CModTree::ModTreeInsLibCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM)
//-------------------------------------------------------------------------------------
{
	lParam1 &= 0x7FFFFFFF;
	lParam2 &= 0x7FFFFFFF;
	return static_cast<int>(lParam1 - lParam2);
}


int CALLBACK CModTree::ModTreeDrumCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM pDLSBank)
//--------------------------------------------------------------------------------------------
{
	lParam1 &= 0x7FFFFFFF;
	lParam2 &= 0x7FFFFFFF;
	if ((lParam1 & 0xFF00FFFF) == (lParam2 & 0xFF00FFFF))
	{
		if(pDLSBank)
		{
			const DLSINSTRUMENT *pDlsIns = reinterpret_cast<CDLSBank *>(pDLSBank)->GetInstrument(lParam1 & 0xFFFF);
			lParam1 = (lParam1 >> 16) & 0xFF;
			lParam2 = (lParam2 >> 16) & 0xFF;
			if ((pDlsIns) && (lParam1 < (LONG)pDlsIns->nRegions) && (lParam2 < (LONG)pDlsIns->nRegions))
			{
				lParam1 = pDlsIns->Regions[lParam1].uKeyMin;
				lParam2 = pDlsIns->Regions[lParam2].uKeyMin;
			}
		}
	}
	return static_cast<int>(lParam1 - lParam2);
}


void CModTree::InstrumentLibraryChDir(mpt::PathString dir, bool isSong)
//---------------------------------------------------------------------
{
	if(dir.empty()) return;
	if(IsSampleBrowser())
	{
		CMainFrame::GetMainFrame()->GetUpperTreeview()->InstrumentLibraryChDir(dir, isSong);
		return;
	}

	BeginWaitCursor();

	bool ok = false;
	if(isSong)
	{
		ok = m_pDataTree->InsLibSetFullPath(m_InstrLibPath, dir);
		if(ok)
		{
			m_pDataTree->RefreshInstrumentLibrary();
			m_InstrLibHighlightPath = dir;
		}
	} else
	{
		if(dir == MPT_PATHSTRING(".."))
		{
			// Go one dir up.
			std::wstring prevDir = m_InstrLibPath.GetPath().ToWide();
			std::wstring::size_type pos = prevDir.find_last_of(L"\\/", prevDir.length() - 2);
			if(pos != std::wstring::npos)
			{
				m_InstrLibHighlightPath = mpt::PathString::FromWide(prevDir.substr(pos + 1, prevDir.length() - pos - 2));	// Highlight previously accessed directory
				prevDir = prevDir.substr(0, pos + 1);
			}
			dir = mpt::PathString::FromWide(prevDir);
		} else
		{
			// Drives are formatted like "E:\", folders are just folder name without slash.
			if(!dir.HasTrailingSlash())
			{
				dir = m_InstrLibPath + dir;
				dir.EnsureTrailingSlash();
			}
			m_InstrLibHighlightPath = MPT_PATHSTRING("..");	// Highlight first entry
		}

		if(dir.IsDirectory())
		{
			m_SongFileName = MPT_PATHSTRING("");
			delete m_SongFile;
			m_SongFile = nullptr;
			m_InstrLibPath = dir;
			PostMessage(WM_COMMAND, ID_MODTREE_REFRESHINSTRLIB);
			ok = true;
		}
	}
	EndWaitCursor();

	if(ok)
	{
		MPT_LOCK_GUARD<mpt::mutex> l(m_WatchDirMutex);
		m_WatchDir = mpt::PathString();
	} else
	{
		std::wstring s = L"Unable to browse to \"" + dir.ToWide() + L"\"";
		Reporting::Error(s, L"Instrument Library");
	}
}


bool CModTree::GetDropInfo(DRAGONDROP &dropInfo, mpt::PathString &fullPath)
//-------------------------------------------------------------------------
{
	dropInfo.pModDoc = (m_nDragDocNdx < DocInfo.size() ? &(DocInfo[m_nDragDocNdx]->modDoc) : nullptr);
	dropInfo.dwDropType = DRAGONDROP_NOTHING;
	dropInfo.dwDropItem = m_itemDrag.val1;
	dropInfo.lDropParam = 0;
	switch(m_itemDrag.type)
	{
	case MODITEM_ORDER:
		dropInfo.dwDropType = DRAGONDROP_ORDER;
		break;

	case MODITEM_PATTERN:
		dropInfo.dwDropType = DRAGONDROP_PATTERN;
		break;

	case MODITEM_SAMPLE:
		dropInfo.dwDropType = DRAGONDROP_SAMPLE;
		break;

	case MODITEM_INSTRUMENT:
		dropInfo.dwDropType = DRAGONDROP_INSTRUMENT;
		break;

	case MODITEM_SEQUENCE:
	case MODITEM_HDR_ORDERS:
		dropInfo.dwDropType = DRAGONDROP_SEQUENCE;
		break;

	case MODITEM_INSLIB_SAMPLE:
	case MODITEM_INSLIB_INSTRUMENT:
		if(!m_SongFileName.empty())
		{
			TCHAR s[32];
			mpt::String::CopyN(s, GetItemText(m_hItemDrag));
			const uint32 n = ConvertStrTo<uint32>(s);
			dropInfo.dwDropType = (m_itemDrag.type == MODITEM_INSLIB_SAMPLE) ? DRAGONDROP_SAMPLE : DRAGONDROP_INSTRUMENT;
			dropInfo.dwDropItem = n;
			dropInfo.pModDoc = nullptr;
			dropInfo.lDropParam = (LPARAM)m_SongFile;
		} else
		{
			fullPath = InsLibGetFullPath(m_hItemDrag);
			dropInfo.dwDropType = DRAGONDROP_SOUNDFILE;
			dropInfo.lDropParam = (LPARAM)&fullPath;
		}
		break;

	case MODITEM_MIDIPERCUSSION:
		dropInfo.dwDropItem |= 0x80;
	case MODITEM_MIDIINSTRUMENT:
		{
			MIDILIBSTRUCT &midiLib = CTrackApp::GetMidiLibrary();
			if(!midiLib.MidiMap[dropInfo.dwDropItem & 0xFF].empty())
			{
				fullPath = midiLib.MidiMap[dropInfo.dwDropItem & 0xFF];
				dropInfo.dwDropType = DRAGONDROP_MIDIINSTR;
				dropInfo.lDropParam = (LPARAM)&fullPath;
			}
		}
		break;

	case MODITEM_INSLIB_SONG:
		fullPath = InsLibGetFullPath(m_hItemDrag);
		dropInfo.pModDoc = NULL;
		dropInfo.dwDropType = DRAGONDROP_SONG;
		dropInfo.dwDropItem = 0;
		dropInfo.lDropParam = (LPARAM)&fullPath;
		break;

	case MODITEM_DLSBANK_INSTRUMENT:
		{
			const DlsItem &item = *static_cast<const DlsItem *>(&m_itemDrag);
			ASSERT(item.IsInstr() || item.IsPercussion());
			dropInfo.dwDropType = DRAGONDROP_DLS;
			// dwDropItem = DLS Bank #
			dropInfo.dwDropItem = item.GetBankIndex();	// bank #
			// Melodic: (Instrument)
			// Drums:	(0x80000000) | (Region << 16) | (Instrument)
			dropInfo.lDropParam = (LPARAM)((m_itemDrag.val1 & (DLS_TYPEPERC | DLS_REGIONMASK | DLS_INSTRMASK)));
		}
		break;
	}
	return (dropInfo.dwDropType != DRAGONDROP_NOTHING);
}


bool CModTree::CanDrop(HTREEITEM hItem, bool bDoDrop)
//---------------------------------------------------
{
	const ModItem modItemDrop = GetModItem(hItem);
	const uint32 modItemDropID = modItemDrop.val1;
	const uint32 modItemDragID = m_itemDrag.val1;

	const ModTreeDocInfo *pInfoDrag = (m_nDragDocNdx < DocInfo.size() ? DocInfo[m_nDragDocNdx] : nullptr);
	const ModTreeDocInfo *pInfoDrop = (m_nDocNdx < DocInfo.size() ? DocInfo[m_nDocNdx] : nullptr);
	CModDoc *pModDoc = (pInfoDrop) ? &pInfoDrop->modDoc : nullptr;
	CSoundFile *pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : nullptr;
	const bool sameModDoc = pInfoDrag && (pModDoc == &pInfoDrag->modDoc);

	switch(modItemDrop.type)
	{
	case MODITEM_ORDER:
	case MODITEM_SEQUENCE:
		if ((m_itemDrag.type == MODITEM_ORDER) && (pModDoc) && sameModDoc)
		{
			// drop an order somewhere
			if (bDoDrop)
			{
				SEQUENCEINDEX nSeqFrom = (SEQUENCEINDEX)m_itemDrag.val2, nSeqTo = (SEQUENCEINDEX)modItemDrop.val2;
				ORDERINDEX nOrdFrom = (ORDERINDEX)m_itemDrag.val1, nOrdTo = (ORDERINDEX)modItemDrop.val1;
				if(modItemDrop.type == MODITEM_SEQUENCE)
				{
					// drop on sequence -> attach
					nSeqTo = (SEQUENCEINDEX)modItemDrop.val1;
					nOrdTo = pSndFile->Order.GetSequence(nSeqTo).GetLengthTailTrimmed();
				}

				if (nSeqFrom != nSeqTo || nOrdFrom != nOrdTo)
				{
					if(pModDoc->MoveOrder(nOrdFrom, nOrdTo, true, false, nSeqFrom, nSeqTo) == true)
					{
						pModDoc->SetModified();
					}
				}
			}
			return true;
		}
		break;

	case MODITEM_HDR_ORDERS:
		// Drop your sequences here.
		// At the moment, only dropping sequences into another module is possible.
		if((m_itemDrag.type == MODITEM_SEQUENCE || m_itemDrag.type == MODITEM_HDR_ORDERS) && pSndFile && pInfoDrag && !sameModDoc)
		{
			if(bDoDrop && pInfoDrag != nullptr)
			{
				// copy mod sequence over.
				CSoundFile &dragSndFile = pInfoDrag->modDoc.GetrSoundFile();
				const SEQUENCEINDEX nOrigSeq = (SEQUENCEINDEX)modItemDragID;
				const ModSequence &origSeq = dragSndFile.Order.GetSequence(nOrigSeq);

				if(pSndFile->GetModSpecifications().sequencesMax > 1)
				{
					pSndFile->Order.AddSequence(false);
				}
				else
				{
					if(Reporting::Confirm(_T("Replace the current orderlist?"), _T("Sequence import")) == cnfNo)
						return false;
				}
				pSndFile->Order.resize(std::min(pSndFile->GetModSpecifications().ordersMax, origSeq.GetLength()), pSndFile->Order.GetInvalidPatIndex());
				for(ORDERINDEX nOrd = 0; nOrd < std::min(pSndFile->GetModSpecifications().ordersMax, origSeq.GetLengthTailTrimmed()); nOrd++)
				{
					PATTERNINDEX nOrigPat = dragSndFile.Order.GetSequence(nOrigSeq)[nOrd];
					// translate pattern index
					if(nOrigPat == dragSndFile.Order.GetInvalidPatIndex())
						pSndFile->Order[nOrd] = pSndFile->Order.GetInvalidPatIndex();
					else if(nOrigPat == dragSndFile.Order.GetIgnoreIndex() && pSndFile->GetModSpecifications().hasIgnoreIndex)
						pSndFile->Order[nOrd] = pSndFile->Order.GetIgnoreIndex();
					else if(nOrigPat == dragSndFile.Order.GetIgnoreIndex() && !pSndFile->GetModSpecifications().hasIgnoreIndex)
						pSndFile->Order[nOrd] = pSndFile->Order.GetInvalidPatIndex();
					else if(nOrigPat >= pSndFile->GetModSpecifications().patternsMax)
						pSndFile->Order[nOrd] = pSndFile->Order.GetInvalidPatIndex();
					else
						pSndFile->Order[nOrd] = nOrigPat;
				}
				pModDoc->UpdateAllViews(nullptr, SequenceHint().Data());
				pModDoc->SetModified();
			}
			return true;
		}
		break;

	case MODITEM_SAMPLE:
		if(m_itemDrag.type == MODITEM_SAMPLE && pInfoDrag != nullptr)
		{
			if(bDoDrop)
			{
				if(sameModDoc)
				{
					// Reorder samples in a module
					const SAMPLEINDEX from = static_cast<SAMPLEINDEX>(modItemDragID - 1), to = static_cast<SAMPLEINDEX>(modItemDropID - 1);

					std::vector<SAMPLEINDEX> newOrder(pModDoc->GetNumSamples());
					for(SAMPLEINDEX smp = 0; smp < pModDoc->GetNumSamples(); smp++)
					{
						newOrder[smp] = smp + 1;
					}

					newOrder.erase(newOrder.begin() + from);
					newOrder.insert(newOrder.begin() + to, from + 1);

					pModDoc->ReArrangeSamples(newOrder);
				} else
				{
					// Load sample into other module
					pSndFile->ReadSampleFromSong(static_cast<SAMPLEINDEX>(modItemDropID), pInfoDrag->modDoc.GetrSoundFile(), static_cast<SAMPLEINDEX>(modItemDragID));
				}
				pModDoc->UpdateAllViews(nullptr, SampleHint().Info().Data().Names());
				pModDoc->UpdateAllViews(nullptr, PatternHint().Data());
				pModDoc->UpdateAllViews(nullptr, InstrumentHint().Info());
				pModDoc->SetModified();
				SelectItem(hItem);
			}
			return true;
		}
		break;

	case MODITEM_INSTRUMENT:
		if(m_itemDrag.type == MODITEM_INSTRUMENT && pInfoDrag != nullptr)
		{
			if(bDoDrop)
			{
				if(sameModDoc)
				{
					// Reorder instruments in a module
					const INSTRUMENTINDEX from = static_cast<INSTRUMENTINDEX>(modItemDragID - 1), to = static_cast<INSTRUMENTINDEX>(modItemDropID - 1);

					std::vector<INSTRUMENTINDEX> newOrder(pModDoc->GetNumInstruments());
					for(INSTRUMENTINDEX ins = 0; ins < pModDoc->GetNumInstruments(); ins++)
					{
						newOrder[ins] = ins + 1;
					}

					newOrder.erase(newOrder.begin() + from);
					newOrder.insert(newOrder.begin() + to, from + 1);

					pModDoc->ReArrangeInstruments(newOrder);
				} else
				{
					// Load instrument into other module
					pSndFile->ReadInstrumentFromSong(static_cast<INSTRUMENTINDEX>(modItemDropID), pInfoDrag->modDoc.GetrSoundFile(), static_cast<INSTRUMENTINDEX>(modItemDragID));
				}
				pModDoc->UpdateAllViews(nullptr, InstrumentHint().Info().Envelope().Names());
				pModDoc->UpdateAllViews(nullptr, PatternHint().Data());
				pModDoc->SetModified();
				SelectItem(hItem);
			}
			return true;
		}
		break;

	case MODITEM_MIDIINSTRUMENT:
	case MODITEM_MIDIPERCUSSION:
		if ((m_itemDrag.type == MODITEM_INSLIB_SAMPLE) || (m_itemDrag.type == MODITEM_INSLIB_INSTRUMENT))
		{
			if (bDoDrop)
			{
				mpt::PathString fullPath = InsLibGetFullPath(m_hItemDrag);
				if (modItemDrop.type == MODITEM_MIDIINSTRUMENT)
					SetMidiInstrument(modItemDropID, fullPath);
				else
					SetMidiPercussion(modItemDropID, fullPath);
			}
			return true;
		}
		break;
	}
	return false;
}


void CModTree::UpdatePlayPos(CModDoc &modDoc, Notification *pNotify)
//------------------------------------------------------------------
{
	ModTreeDocInfo *pInfo = GetDocumentInfoFromModDoc(modDoc);
	if(pInfo == nullptr) return;

	const CSoundFile &sndFile = modDoc.GetrSoundFile();
	ORDERINDEX nNewOrd = (pNotify) ? pNotify->order : ORDERINDEX_INVALID;
	SEQUENCEINDEX nNewSeq = sndFile.Order.GetCurrentSequenceIndex();
	if (nNewOrd != pInfo->nOrdSel || nNewSeq != pInfo->nSeqSel)
	{
		pInfo->nOrdSel = nNewOrd;
		pInfo->nSeqSel = nNewSeq;
		UpdateView(*pInfo, SequenceHint().Data());
	}

	// Update sample / instrument playing status icons (will only detect instruments with samples, though)

	if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_LIVEUPDATETREE) == 0) return;
	// TODO: Is there a way to find out if the treeview is actually visible?
	/*static int nUpdateCount = 0;
	nUpdateCount++;
	if(nUpdateCount < 5) return; // don't update too often
	nUpdateCount = 0;*/

	// check whether the lists are actually visible (don't waste resources)
	const bool updateSamples = IsItemExpanded(pInfo->hSamples), updateInstruments = IsItemExpanded(pInfo->hInstruments);

	pInfo->samplesPlaying.reset();
	pInfo->instrumentsPlaying.reset();

	if(!updateSamples && !updateInstruments) return;

	for(const ModChannel *chn = sndFile.m_PlayState.Chn; chn != sndFile.m_PlayState.Chn + CountOf(sndFile.m_PlayState.Chn); chn++)
	{
		if(chn->pCurrentSample != nullptr && chn->nLength != 0 && !chn->increment.IsZero())
		{
			if(updateSamples)
			{
				for(SAMPLEINDEX nSmp = sndFile.GetNumSamples(); nSmp >= 1; nSmp--)
				{
					if(chn->pModSample == &sndFile.GetSample(nSmp))
					{
						pInfo->samplesPlaying.set(nSmp);
						break;
					}
				}
			}
			if(updateInstruments)
			{
				for(INSTRUMENTINDEX nIns = sndFile.GetNumInstruments(); nIns >= 1; nIns--)
				{
					if(chn->pModInstrument == sndFile.Instruments[nIns])
					{
						pInfo->instrumentsPlaying.set(nIns);
						break;
					}
				}
			}
		}
	}
	// what should be updated?
	if(updateSamples) UpdateView(*pInfo, SampleHint().Info());
	if(updateInstruments) UpdateView(*pInfo, InstrumentHint().Info());
}



/////////////////////////////////////////////////////////////////////////////
// CViewModTree message handlers


void CModTree::OnUpdate(CModDoc *pModDoc, UpdateHint hint, CObject *pHint)
//------------------------------------------------------------------------
{
	if (pHint != this)
	{
		for (auto iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
		{
			if (&(*iter)->modDoc == pModDoc || !pModDoc)
			{
				UpdateView((**iter), hint);
				break;
			}
		}
	}
}

void CModTree::OnItemExpanded(LPNMHDR pnmhdr, LRESULT *pResult)
//-------------------------------------------------------------
{
	LPNMTREEVIEW pnm = (LPNMTREEVIEW)pnmhdr;
	if ((pnm->itemNew.iImage == IMAGE_FOLDER) || (pnm->itemNew.iImage == IMAGE_OPENFOLDER))
	{
		int iNewImage = (pnm->itemNew.state & TVIS_EXPANDED) ? IMAGE_OPENFOLDER : IMAGE_FOLDER;
		SetItemImage(pnm->itemNew.hItem, iNewImage, iNewImage);
	}
	if (pResult) *pResult = TRUE;
}


void CModTree::OnBeginDrag(HTREEITEM hItem, bool bLeft, LRESULT *pResult)
//-----------------------------------------------------------------------
{
	if (!(m_dwStatus & TREESTATUS_DRAGGING))
	{
		bool bDrag = false;

		m_hDropWnd = NULL;
		m_hItemDrag = hItem;
		if (m_hItemDrag != NULL)
		{
			if (!ItemHasChildren(m_hItemDrag)) SelectItem(m_hItemDrag);
		}
		m_itemDrag = GetModItem(m_hItemDrag);
		m_nDragDocNdx = m_nDocNdx;
		switch(m_itemDrag.type)
		{
		case MODITEM_ORDER:
		case MODITEM_PATTERN:
		case MODITEM_SAMPLE:
		case MODITEM_INSTRUMENT:
		case MODITEM_SEQUENCE:
		case MODITEM_MIDIINSTRUMENT:
		case MODITEM_MIDIPERCUSSION:
		case MODITEM_INSLIB_SAMPLE:
		case MODITEM_INSLIB_INSTRUMENT:
		case MODITEM_INSLIB_SONG:
			bDrag = true;
			break;
		case MODITEM_HDR_ORDERS:
			// can we drag an order header? (only in MPTM format and if there's only one sequence)
			{
				const CModDoc *pModDoc = (m_nDragDocNdx < DocInfo.size() ? &(DocInfo[m_nDragDocNdx]->modDoc) : nullptr);
				if(pModDoc && pModDoc->GetrSoundFile().Order.GetNumSequences() == 1)
					bDrag = true;
			}
			break;
		default:
			if (m_itemDrag.type == MODITEM_DLSBANK_INSTRUMENT) bDrag = true;
		}
		if (bDrag)
		{
			m_dwStatus |= (bLeft) ? TREESTATUS_LDRAG : TREESTATUS_RDRAG;
			m_hItemDrop = NULL;
			SetCapture();
		}
	}
	if (pResult) *pResult = TRUE;
}


void CModTree::OnBeginRDrag(LPNMHDR pnmhdr, LRESULT *pResult)
//-----------------------------------------------------------
{
	if (pnmhdr)
	{
		LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmhdr;
		OnBeginDrag(pnmtv->itemNew.hItem, false, pResult);
	}
}


void CModTree::OnBeginLDrag(LPNMHDR pnmhdr, LRESULT *pResult)
//-----------------------------------------------------------
{
	if (pnmhdr)
	{
		LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmhdr;
		OnBeginDrag(pnmtv->itemNew.hItem, true, pResult);
	}
}


void CModTree::OnItemDblClk(LPNMHDR, LRESULT *pResult)
//----------------------------------------------------
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	HTREEITEM hItem = GetSelectedItem();
	if ((hItem) && (hItem == HitTest(pt)))
	{
		ExecuteItem(hItem);
	}
	if (pResult) *pResult = 0;
}


void CModTree::OnItemReturn(LPNMHDR, LRESULT *pResult)
//----------------------------------------------------
{
	HTREEITEM hItem = GetSelectedItem();
	if (hItem) ExecuteItem(hItem);
	if (pResult) *pResult = 0;
}


void CModTree::OnItemRightClick(LPNMHDR, LRESULT *pResult)
//--------------------------------------------------------
{
	HTREEITEM hItem;
	HMENU hMenu;
	POINT pt, ptClient;
	UINT flags;

	GetCursorPos(&pt);
	ptClient = pt;
	ScreenToClient(&ptClient);
	hItem = HitTest(ptClient, &flags);
	if (m_dwStatus & TREESTATUS_LDRAG)
	{
		if (ItemHasChildren(hItem))
		{
			Expand(hItem, TVE_TOGGLE);
		} else
		{
			m_hItemDrop = NULL;
			m_hDropWnd = NULL;
			OnEndDrag(TREESTATUS_DRAGGING);
		}
	} else
	{
		if (m_dwStatus & TREESTATUS_DRAGGING)
		{
			m_hItemDrop = NULL;
			m_hDropWnd = NULL;
			OnEndDrag(TREESTATUS_DRAGGING);
		}
		hMenu = ::CreatePopupMenu();
		if (hMenu)
		{
			const CModDoc *modDoc = GetDocumentFromItem(hItem);
			const CSoundFile *sndFile = modDoc != nullptr ? modDoc->GetSoundFile() : nullptr;

			UINT nDefault = 0;
			BOOL bSep = FALSE;

			const ModItem modItem = GetModItem(hItem);
			const uint32 modItemID = modItem.val1;

			SelectItem(hItem);
			switch(modItem.type)
			{
			case MODITEM_HDR_SONG:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, _T("&View"));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_CLOSE, _T("&Close"));
				break;

			case MODITEM_COMMENTS:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, _T("&View Comments"));
				break;

			case MODITEM_ORDER:
			case MODITEM_PATTERN:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, _T("&Edit Pattern"));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE,
								(modItem.type == MODITEM_ORDER) ? _T("&Delete from list") : _T("&Delete Pattern"));
				break;

			case MODITEM_SEQUENCE:
				{
					bool isCurSeq = false;
					if(sndFile && sndFile->GetModSpecifications().sequencesMax > 1)
					{
						if(sndFile->Order.GetSequence((SEQUENCEINDEX)modItemID).GetLength() == 0)
						{
							nDefault = ID_MODTREE_SWITCHTO;
						}
						isCurSeq = (sndFile->Order.GetCurrentSequenceIndex() == (SEQUENCEINDEX)modItemID);
					}

					if(!isCurSeq)
					{
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_SWITCHTO, _T("&Switch to Seqeuence"));
					}
					AppendMenu(hMenu, MF_STRING | (sndFile->Order.GetNumSequences() < MAX_SEQUENCES ? 0 : MF_GRAYED), ID_MODTREE_INSERT, _T("&Insert Sequence"));
					AppendMenu(hMenu, MF_STRING | (sndFile->Order.GetNumSequences() < MAX_SEQUENCES ? 0 : MF_GRAYED), ID_MODTREE_DUPLICATE , _T("D&uplicate Sequence"));
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, _T("&Delete Sequence"));
				}
				break;


			case MODITEM_HDR_ORDERS:
				if(sndFile && sndFile->GetModSpecifications().sequencesMax > 1)
				{
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_INSERT, _T("&Insert Sequence"));
					if(sndFile->Order.GetNumSequences() == 1) // this is a sequence
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_DUPLICATE, _T("D&uplicate Sequence"));
				}
				break;

			case MODITEM_SAMPLE:
				{
					nDefault = ID_MODTREE_EXECUTE;
					AppendMenu(hMenu, MF_STRING, nDefault, _T("&View Sample"));
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, _T("&Play Sample"));
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_INSERT, _T("&Insert Sample"));
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_DUPLICATE, _T("D&uplicate Sample"));
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, _T("&Delete Sample"));
					if ((modDoc) && (!modDoc->GetNumInstruments()))
					{
						AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
						AppendMenu(hMenu, (modDoc->IsSampleMuted((SAMPLEINDEX)modItemID) ? MF_CHECKED : 0) | MF_STRING, ID_MODTREE_MUTE, _T("&Mute Sample"));
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_SOLO, _T("S&olo Sample"));
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_UNMUTEALL, _T("&Unmute all"));
					}
					if(sndFile != nullptr)
					{
						SAMPLEINDEX smpID = static_cast<SAMPLEINDEX>(modItem.val1);
						const ModSample &sample = sndFile->GetSample(smpID);
						const bool hasPath = sndFile->SampleHasPath(smpID);
						const bool menuForThisSample = (sample.HasSampleData() && sndFile->GetType() == MOD_TYPE_MPT) || hasPath;

						bool anyPath = false, anyModified = false, anyMissing = false;
						for(SAMPLEINDEX smp = 1; smp <= sndFile->GetNumSamples(); smp++)
						{
							const ModSample &sample = sndFile->GetSample(smp);
							if(sndFile->SampleHasPath(smp) && smp != smpID)
							{
								anyPath = true;
								if(sample.HasSampleData() && sample.uFlags[SMP_MODIFIED])
								{
									anyModified = true;
								}
							}
							if(sndFile->IsExternalSampleMissing(smp))
							{
								anyMissing = true;
							}
							if(anyPath && anyModified && anyMissing) break;
						}

						if(menuForThisSample || anyPath || anyModified)
						{
							AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
							if(menuForThisSample) AppendMenu(hMenu, MF_STRING | ((sndFile->GetType() == MOD_TYPE_MPT || hasPath) ? 0 : MF_GRAYED), ID_MODTREE_SETPATH, _T("Set P&ath"));
							if(menuForThisSample) AppendMenu(hMenu, MF_STRING | ((hasPath && sample.HasSampleData() && sample.uFlags[SMP_MODIFIED]) ? 0 : MF_GRAYED), ID_MODTREE_SAVEITEM, _T("&Save"));
							if(anyModified) AppendMenu(hMenu, MF_STRING, ID_MODTREE_SAVEALL, _T("&Save All"));
							if(menuForThisSample) AppendMenu(hMenu, MF_STRING | (hasPath ? 0 : MF_GRAYED), ID_MODTREE_RELOADITEM, _T("&Reload"));
							if(anyPath) AppendMenu(hMenu, MF_STRING, ID_MODTREE_RELOADALL, _T("&Reload All"));
							if(anyMissing) AppendMenu(hMenu, MF_STRING, ID_MODTREE_FINDMISSING, _T("&Find Missing Samples"));
						}
					}
				}
				break;

			case MODITEM_INSTRUMENT:
				{
					nDefault = ID_MODTREE_EXECUTE;
					AppendMenu(hMenu, MF_STRING, nDefault, _T("&View Instrument"));
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, _T("&Play Instrument"));
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_INSERT, _T("&Insert Instrument"));
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_DUPLICATE, _T("D&uplicate Instrument"));
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, _T("&Delete Instrument"));
					if (modDoc)
					{
						AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
						AppendMenu(hMenu, (modDoc->IsInstrumentMuted((INSTRUMENTINDEX)modItemID) ? MF_CHECKED : 0) | MF_STRING, ID_MODTREE_MUTE, _T("&Mute Instrument"));
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_SOLO, _T("S&olo Instrument"));
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_UNMUTEALL, _T("&Unmute all"));
					}
				}
				break;

			case MODITEM_EFFECT:
				{
					nDefault = ID_MODTREE_EXECUTE;
					AppendMenu(hMenu, MF_STRING, nDefault, _T("&Edit"));

					if(modDoc != nullptr)
					{
						AppendMenu(hMenu, (modDoc->GetrSoundFile().m_MixPlugins[modItemID].IsBypassed() ? MF_CHECKED : 0) | MF_STRING, ID_MODTREE_MUTE, _T("&Bypass"));
					}
				}
				break;

			case MODITEM_MIDIINSTRUMENT:
			case MODITEM_MIDIPERCUSSION:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, _T("&Map Instrument"));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, _T("&Play Instrument"));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, _T("&Unmap Instrument"));
				AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
			case MODITEM_HDR_MIDILIB:
			case MODITEM_HDR_MIDIGROUP:
				AppendMenu(hMenu, MF_STRING, ID_IMPORT_MIDILIB, _T("&Import MIDI Library"));
				AppendMenu(hMenu, MF_STRING, ID_EXPORT_MIDILIB, _T("E&xport MIDI Library"));
				bSep = TRUE;
				break;

			case MODITEM_INSLIB_FOLDER:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, _T("&Browse..."));
				break;

			case MODITEM_INSLIB_SONG:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, _T("&Browse Song..."));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_OPENITEM, _T("&Edit Song"));
				AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, _T("&Delete"));
				break;

			case MODITEM_INSLIB_SAMPLE:
			case MODITEM_INSLIB_INSTRUMENT:
				nDefault = ID_MODTREE_PLAY;
				if(!m_SongFileName.empty())
				{
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, _T("&Play"));
				} else
				{
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, _T("&Play File"));
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, _T("&Delete"));
				}
				break;

			case MODITEM_DLSBANK_FOLDER:
				nDefault = ID_SOUNDBANK_PROPERTIES;
				AppendMenu(hMenu, MF_STRING, nDefault, _T("&Properties"));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, _T("Re&move this bank"));
			case MODITEM_NULL:
				AppendMenu(hMenu, MF_STRING, ID_ADD_SOUNDBANK, _T("Add Sound &Bank..."));
				bSep = TRUE;
				break;

			case MODITEM_DLSBANK_INSTRUMENT:
				nDefault = ID_MODTREE_PLAY;
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, _T("&Play Instrument"));
				break;

			case MODITEM_HDR_INSTRUMENTLIB:
				if(IsSampleBrowser())
				{
					nDefault = ID_MODTREE_EXECUTE;
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_EXECUTE, _T("&Browse..."));
				}
				break;
			}
			if (nDefault) SetMenuDefaultItem(hMenu, nDefault, FALSE);
			if ((modItem.type == MODITEM_INSLIB_FOLDER)
			 || (modItem.type == MODITEM_INSLIB_SONG)
			 || (modItem.type == MODITEM_HDR_INSTRUMENTLIB))
			{
				if ((bSep) || (nDefault)) AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
				AppendMenu(hMenu, TrackerSettings::Instance().showDirsInSampleBrowser ? (MF_STRING|MF_CHECKED) : MF_STRING, ID_MODTREE_SHOWDIRS, _T("Show &Directories in Sample Browser"));
				AppendMenu(hMenu, (m_bShowAllFiles) ? (MF_STRING|MF_CHECKED) : MF_STRING, ID_MODTREE_SHOWALLFILES, _T("Show &All Files"));
				AppendMenu(hMenu, (m_bShowAllFiles) ? MF_STRING : (MF_STRING|MF_CHECKED), ID_MODTREE_SOUNDFILESONLY, _T("Show &Sound Files"));
				bSep = TRUE;
			}
			if ((bSep) || (nDefault)) AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_REFRESH, _T("&Refresh"));
			TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x + 4, pt.y, 0, m_hWnd, NULL);
			DestroyMenu(hMenu);
		}
	}
	if (pResult) *pResult = 0;
}


void CModTree::OnItemLeftClick(LPNMHDR, LRESULT *pResult)
//-------------------------------------------------------
{
	if (!(m_dwStatus & TREESTATUS_RDRAG))
	{
		POINT pt;
		UINT flags;
		GetCursorPos(&pt);
		ScreenToClient(&pt);
		HTREEITEM hItem = HitTest(pt, &flags);
		if (hItem != NULL)
		{
			const ModItem modItem = GetModItem(hItem);
			const uint32 modItemID = modItem.val1;

			switch(modItem.type)
			{
			case MODITEM_INSLIB_FOLDER:
			case MODITEM_INSLIB_SONG:
				if (m_dwStatus & TREESTATUS_SINGLEEXPAND) ExecuteItem(hItem);
				break;

			case MODITEM_SAMPLE:
			case MODITEM_INSTRUMENT:
				{
					CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
					CFrameWnd *pFrame = pMainFrm->GetActiveFrame();
					if (pFrame)
					{
						pFrame->SendMessage(WM_MOD_INSTRSELECTED,
							(modItem.type == MODITEM_INSTRUMENT) ? TRUE : FALSE,
							(LPARAM)modItemID);
					}
				}
				break;

			case MODITEM_HDR_SONG:
				ExecuteItem(hItem);
				break;
			}
		}
	}
	if (pResult) *pResult = 0;
}


void CModTree::OnEndDrag(DWORD dwMask)
//------------------------------------
{
	if (m_dwStatus & dwMask)
	{
		m_dwStatus &= ~dwMask;
		if (!(m_dwStatus & TREESTATUS_DRAGGING))
		{
			ReleaseCapture();
			SetCursor(CMainFrame::curArrow);
			SelectDropTarget(NULL);
			if (m_hItemDrop != NULL)
			{
				CanDrop(m_hItemDrop, TRUE);
			} else
			if (m_hDropWnd)
			{
				DRAGONDROP dropinfo;
				mpt::PathString fullPath;
				if(GetDropInfo(dropinfo, fullPath))
				{
					if (dropinfo.dwDropType == DRAGONDROP_SONG)
					{
						theApp.OpenDocumentFile(fullPath);
					} else
					{
						::SendMessage(m_hDropWnd, WM_MOD_DRAGONDROPPING, TRUE, (LPARAM)&dropinfo);
					}
				}
			}
		}
	}
}


void CModTree::OnLButtonUp(UINT nFlags, CPoint point)
//---------------------------------------------------
{
	OnEndDrag(TREESTATUS_LDRAG);
	CTreeCtrl::OnLButtonUp(nFlags, point);
}


void CModTree::OnRButtonUp(UINT nFlags, CPoint point)
//---------------------------------------------------
{
	OnEndDrag(TREESTATUS_RDRAG);
	CTreeCtrl::OnRButtonUp(nFlags, point);
}


void CModTree::OnMouseMove(UINT nFlags, CPoint point)
//---------------------------------------------------
{
	if (m_dwStatus & TREESTATUS_DRAGGING)
	{
		HTREEITEM hItem;
		UINT flags;

		// Bug?
		if (!(nFlags & (MK_LBUTTON|MK_RBUTTON)))
		{
			m_itemDrag = ModItem(MODITEM_NULL);
			m_hItemDrag = NULL;
			OnEndDrag(TREESTATUS_DRAGGING);
			return;
		}
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm)
		{
			CRect rect;
			GetClientRect(&rect);
			if (rect.PtInRect(point))
			{
				m_hDropWnd = m_hWnd;
				bool bCanDrop = CanDrop( HitTest(point, &flags), false);
				SetCursor((bCanDrop) ? CMainFrame::curDragging : CMainFrame::curNoDrop2);
			} else
			{
				CPoint pt = point;
				ClientToScreen(&pt);
				CWnd *pWnd = WindowFromPoint(pt);
				HWND hwnd = (pWnd) ? pWnd->m_hWnd : NULL;
				if (hwnd != m_hDropWnd)
				{
					BOOL bCanDrop = FALSE;
					m_hDropWnd = hwnd;
					if (hwnd == m_hWnd)
					{
						bCanDrop = TRUE;
					} else
					if (hwnd != NULL)
					{
						DRAGONDROP dropinfo;
						mpt::PathString fullPath;
						if(GetDropInfo(dropinfo, fullPath))
						{
							if (dropinfo.dwDropType == DRAGONDROP_SONG)
							{
								bCanDrop = TRUE;
							} else
							if (::SendMessage(hwnd, WM_MOD_DRAGONDROPPING, FALSE, (LPARAM)&dropinfo))
							{
								bCanDrop = TRUE;
							}
						}
					}
					SetCursor((bCanDrop) ? CMainFrame::curDragging : CMainFrame::curNoDrop);
					if (bCanDrop)
					{
						if (GetDropHilightItem() != m_hItemDrag)
						{
							SelectDropTarget(m_hItemDrag);
						}
						m_hItemDrop = NULL;
						return;
					}
				}
			}
			if ((point.x >= -1) && (point.x <= rect.right + GetSystemMetrics(SM_CXVSCROLL)))
			{
				if (point.y <= 0)
				{
					HTREEITEM hfirst = GetFirstVisibleItem();
					if (hfirst != NULL)
					{
						HTREEITEM hprev = GetPrevVisibleItem(hfirst);
						if (hprev != NULL) SelectSetFirstVisible(hprev);
					}
				} else
				if (point.y >= rect.bottom-1)
				{
					hItem = HitTest(point, &flags);
					HTREEITEM hNext = GetNextItem(hItem, TVGN_NEXTVISIBLE);
					if (hNext != NULL)
					{
						EnsureVisible(hNext);
					}
				}
			}
			if ((hItem = HitTest(point, &flags)) != NULL)
			{
				SelectDropTarget(hItem);
				EnsureVisible(hItem);
				m_hItemDrop = hItem;
			}
		}
	}
	CTreeCtrl::OnMouseMove(nFlags, point);
}


void CModTree::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
//-------------------------------------------------------------
{
	switch(nChar)
	{
	case VK_DELETE:
		DeleteTreeItem(GetSelectedItem());
		break;
	}
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CModTree::OnRefreshTree()
//----------------------------
{
	BeginWaitCursor();
	for (auto iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
	{
		UpdateView((**iter), UpdateHint().ModType());
	}
	RefreshMidiLibrary();
	RefreshDlsBanks();
	RefreshInstrumentLibrary();
	EndWaitCursor();
}


void CModTree::OnExecuteItem()
//----------------------------
{
	ExecuteItem(GetSelectedItem());
}


void CModTree::OnDeleteTreeItem()
//-------------------------------
{
	DeleteTreeItem(GetSelectedItem());
}


void CModTree::OnPlayTreeItem()
//-----------------------------
{
	PlayItem(GetSelectedItem(), NOTE_MIDDLEC);
}


void CModTree::OnOpenTreeItem()
//-----------------------------
{
	OpenTreeItem(GetSelectedItem());
}


void CModTree::OnMuteTreeItem()
//-----------------------------
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	const uint32 modItemID = modItem.val1;

	ModTreeDocInfo *info = GetDocumentInfoFromItem(hItem);
	if (info)
	{
		CModDoc &modDoc = info->modDoc;
		if ((modItem.type == MODITEM_SAMPLE) && !modDoc.GetNumInstruments())
		{
			modDoc.MuteSample((SAMPLEINDEX)modItemID, !modDoc.IsSampleMuted((SAMPLEINDEX)modItemID));
			UpdateView(*info, SampleHint((SAMPLEINDEX)modItemID).Info().Names());
		} else
		if ((modItem.type == MODITEM_INSTRUMENT) && modDoc.GetNumInstruments())
		{
			modDoc.MuteInstrument((INSTRUMENTINDEX)modItemID, !modDoc.IsInstrumentMuted((INSTRUMENTINDEX)modItemID));
			UpdateView(*info, InstrumentHint((INSTRUMENTINDEX)modItemID).Info().Names());
		} else if ((modItem.type == MODITEM_EFFECT))
		{
			IMixPlugin *pPlugin = modDoc.GetrSoundFile().m_MixPlugins[modItemID].pMixPlugin;
			if(pPlugin == nullptr)
				return;
			pPlugin->ToggleBypass();
			modDoc.SetModified();
			//UpdateView(GetDocumentIDFromModDoc(pModDoc), HINT_MIXPLUGINS);
		}
	}
}


void CModTree::OnSoloTreeItem()
//-----------------------------
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	const uint32 modItemID = modItem.val1;

	ModTreeDocInfo *info = GetDocumentInfoFromItem(hItem);
	if (info)
	{
		CModDoc &modDoc = info->modDoc;
		INSTRUMENTINDEX nInstruments = modDoc.GetNumInstruments();
		if ((modItem.type == MODITEM_SAMPLE) && (!nInstruments))
		{
			for (SAMPLEINDEX nSmp = 1; nSmp <= modDoc.GetNumSamples(); nSmp++)
			{
				modDoc.MuteSample(nSmp, nSmp != modItemID);
			}
			UpdateView(*info, SampleHint().Info().Names());
		} else if ((modItem.type == MODITEM_INSTRUMENT) && (nInstruments))
		{
			for (INSTRUMENTINDEX nIns = 1; nIns <= nInstruments; nIns++)
			{
				modDoc.MuteInstrument(nIns, nIns != modItemID);
			}
			UpdateView(*info, InstrumentHint().Info().Names());
		}
	}
}


void CModTree::OnUnmuteAllTreeItem()
//----------------------------------
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);

	ModTreeDocInfo *info = GetDocumentInfoFromItem(hItem);
	if (info)
	{
		CModDoc &modDoc = info->modDoc;
		if ((modItem.type == MODITEM_SAMPLE) || (modItem.type == MODITEM_INSTRUMENT))
		{
			for (SAMPLEINDEX nSmp = 1; nSmp <= modDoc.GetNumSamples(); nSmp++)
			{
				modDoc.MuteSample(nSmp, false);
			}
			UpdateView(*info, SampleHint().Info().Names());
			for (INSTRUMENTINDEX nIns = 1; nIns <= modDoc.GetNumInstruments(); nIns++)
			{
				modDoc.MuteInstrument(nIns, false);
			}
			UpdateView(*info, InstrumentHint().Info().Names());
		}
	}
}


// Helper function for generating an insert vector for samples/instruments
template<typename T>
std::vector<T> GenerateInsertVector(size_t howMany, size_t insertPos, T insertId)
//-------------------------------------------------------------------------------
{
	std::vector<T> newOrder(howMany);
	for(T i = 0; i < howMany; i++)
	{
		newOrder[i] = i + 1;
	}
	newOrder.insert(newOrder.begin() + insertPos, insertId);
	return newOrder;
}


void CModTree::InsertOrDupItem(bool insert)
//-----------------------------------------
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	const uint32 modItemID = modItem.val1;

	ModTreeDocInfo *info = GetDocumentInfoFromItem(hItem);
	if (info)
	{
		CModDoc &modDoc = info->modDoc;
		CSoundFile &sndFile = modDoc.GetrSoundFile();
		if(modItem.type == MODITEM_SEQUENCE || modItem.type == MODITEM_HDR_ORDERS)
		{
			// Duplicate / insert sequence
			if(insert)
			{
				sndFile.Order.AddSequence(false);
			} else
			{
				sndFile.Order.SetSequence((SEQUENCEINDEX)modItemID);
				sndFile.Order.AddSequence(true);
			}
			modDoc.SetModified();
			UpdateView(*info, SequenceHint(MAX_SEQUENCES).Data().Names());
			modDoc.UpdateAllViews(nullptr, SequenceHint().Data().Names());
		} else if(modItem.type == MODITEM_SAMPLE)
		{
			// Duplicate / insert sample
			std::vector<SAMPLEINDEX> newOrder = GenerateInsertVector<SAMPLEINDEX>(sndFile.GetNumSamples(), modItemID, static_cast<SAMPLEINDEX>(insert ? 0 : modItemID));
			if(modDoc.ReArrangeSamples(newOrder) != SAMPLEINDEX_INVALID)
			{
				modDoc.SetModified();
				modDoc.UpdateAllViews(nullptr, SampleHint().Info().Data().Names());
				modDoc.UpdateAllViews(nullptr, PatternHint());
			} else
			{
				Reporting::Error("Maximum number of samples reached.");
			}
		} else if(modItem.type == MODITEM_INSTRUMENT)
		{
			// Duplicate / insert instrument
			std::vector<INSTRUMENTINDEX> newOrder = GenerateInsertVector<INSTRUMENTINDEX>(sndFile.GetNumInstruments(), modItemID, static_cast<INSTRUMENTINDEX>(insert ? 0 : modItemID));
			if(modDoc.ReArrangeInstruments(newOrder) != INSTRUMENTINDEX_INVALID)
			{
				modDoc.UpdateAllViews(NULL, InstrumentHint().Info().Envelope().Names());
				modDoc.UpdateAllViews(nullptr, PatternHint());
				modDoc.SetModified();
			} else
			{
				Reporting::Error("Maximum number of instruments reached.");
			}
		}
	}
}


void CModTree::OnSwitchToTreeItem()
//---------------------------------
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);

	CModDoc *pModDoc = GetDocumentFromItem(hItem);
	if (pModDoc && (modItem.type == MODITEM_SEQUENCE))
	{
		pModDoc->ActivateView(IDD_CONTROL_PATTERNS, uint32(modItem.val1 << SEQU_SHIFT) | SEQU_INDICATOR);
	}
}


void CModTree::OnSetItemPath()
//----------------------------
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	CModDoc *pModDoc = GetDocumentFromItem(hItem);

	if(pModDoc && modItem.val1)
	{
		SAMPLEINDEX smpID = static_cast<SAMPLEINDEX>(modItem.val1);
		const mpt::PathString path = pModDoc->GetrSoundFile().GetSamplePath(smpID);
		FileDialog dlg = OpenFileDialog()
			.ExtensionFilter("All Samples|*.wav;*.flac|All files(*.*)|*.*||");	// Only show samples that we actually can save as well.
		if(path.empty())
			dlg.WorkingDirectory(TrackerSettings::Instance().PathSamples.GetWorkingDir());
		else
			dlg.DefaultFilename(path);
		if(!dlg.Show()) return;
		TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetWorkingDirectory());

		if(dlg.GetFirstFile() != pModDoc->GetrSoundFile().GetSamplePath(smpID))
		{
			pModDoc->GetrSoundFile().SetSamplePath(smpID, dlg.GetFirstFile());
			pModDoc->SetModified();
		}
		OnReloadItem();
	}
}


void CModTree::OnSaveItem()
//-------------------------
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	CModDoc *pModDoc = GetDocumentFromItem(hItem);

	if(pModDoc && modItem.val1)
	{
		SAMPLEINDEX smpID = static_cast<SAMPLEINDEX>(modItem.val1);
		pModDoc->SaveSample(smpID);
		if(pModDoc) pModDoc->UpdateAllViews(NULL, SampleHint(smpID).Info());
		OnRefreshTree();
	}
}


void CModTree::OnSaveAll()
//-------------------------
{
	CModDoc *pModDoc = GetDocumentFromItem(GetSelectedItem());
	if(pModDoc != nullptr)
	{
		pModDoc->SaveAllSamples();
		if(pModDoc) pModDoc->UpdateAllViews(nullptr, SampleHint().Info());
		OnRefreshTree();
	}
}


void CModTree::OnReloadItem()
//---------------------------
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	CModDoc *pModDoc = GetDocumentFromItem(hItem);

	if(pModDoc && modItem.val1)
	{
		SAMPLEINDEX smpID = static_cast<SAMPLEINDEX>(modItem.val1);
		CSoundFile &sndFile = pModDoc->GetrSoundFile();
		pModDoc->GetSampleUndo().PrepareUndo(smpID, sundo_replace, "Replace");
		if(!sndFile.LoadExternalSample(smpID, sndFile.GetSamplePath(smpID)))
		{
			pModDoc->GetSampleUndo().RemoveLastUndoStep(smpID);
			Reporting::Error(L"Unable to load sample:\n" + sndFile.GetSamplePath(smpID).ToWide());
		} else
		{
			if(!sndFile.GetSample(smpID).uFlags[SMP_KEEPONDISK])
			{
				pModDoc->SetModified();
			}
			pModDoc->UpdateAllViews(NULL, SampleHint(smpID).Info().Data().Names());
		}

		OnRefreshTree();
	}
}


void CModTree::OnReloadAll()
//--------------------------
{
	CModDoc *pModDoc = GetDocumentFromItem(GetSelectedItem());
	if(pModDoc != nullptr)
	{
		CSoundFile &sndFile = pModDoc->GetrSoundFile();
		bool anyMissing = false;
		for(SAMPLEINDEX smp = 1; smp <= sndFile.GetNumSamples(); smp++)
		{
			const mpt::PathString &path = sndFile.GetSamplePath(smp);
			if(path.empty()) continue;

			pModDoc->GetSampleUndo().PrepareUndo(smp, sundo_replace, "Replace");
			if(!sndFile.LoadExternalSample(smp, path))
			{
				pModDoc->GetSampleUndo().RemoveLastUndoStep(smp);
				anyMissing = true;
			} else
			{
				if(!sndFile.GetSample(smp).uFlags[SMP_KEEPONDISK])
				{
					pModDoc->SetModified();
				}
			}
		}
		pModDoc->UpdateAllViews(nullptr, SampleHint().Info().Data().Names());
		OnRefreshTree();
		if(anyMissing)
		{
			OnFindMissing();
		}
	}
}


// Find missing external samples
void CModTree::OnFindMissing()
//----------------------------
{
	CModDoc *pModDoc = GetDocumentFromItem(GetSelectedItem());
	if(pModDoc == nullptr)
	{
		return;
	}
	ExternalSamplesDlg dlg(*pModDoc, CMainFrame::GetMainFrame());
	dlg.DoModal();
}


void CModTree::OnAddDlsBank()
//---------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->OnAddDlsBank();
}


void CModTree::OnImportMidiLib()
//------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->OnImportMidiLib();
}


void CModTree::OnExportMidiLib()
//------------------------------
{
	FileDialog dlg = SaveFileDialog()
		.DefaultExtension("ini")
		.DefaultFilename("mptrack.ini")
		.ExtensionFilter("Text and INI files (*.txt,*.ini)|*.txt;*.ini|"
			"All Files (*.*)|*.*||");
	if(!dlg.Show()) return;

	CTrackApp::ExportMidiConfig(dlg.GetFirstFile());
}


///////////////////////////////////////////////////////////////////////
// Drop support

DROPEFFECT CModTree::OnDragEnter(COleDataObject*, DWORD, CPoint)
//--------------------------------------------------------------
{
	return DROPEFFECT_LINK;
}


DROPEFFECT CModTree::OnDragOver(COleDataObject*, DWORD, CPoint point)
//-------------------------------------------------------------------
{
	UINT flags;
	HTREEITEM hItem = HitTest(point, &flags);

	const ModItem modItem = GetModItem(hItem);

	switch(modItem.type)
	{
	case MODITEM_MIDIINSTRUMENT:
	case MODITEM_MIDIPERCUSSION:
		if (hItem != GetDropHilightItem())
		{
			SelectDropTarget(hItem);
			EnsureVisible(hItem);
		}
		m_hItemDrag = hItem;
		m_itemDrag = modItem;
		return DROPEFFECT_LINK;
	// Folders:
	case MODITEM_HDR_MIDILIB:
	case MODITEM_HDR_MIDIGROUP:
		EnsureVisible(GetChildItem(hItem));
		break;
	}
	return DROPEFFECT_NONE;
}


BOOL CModTree::OnDrop(COleDataObject* pDataObject, DROPEFFECT, CPoint)
//--------------------------------------------------------------------
{
	STGMEDIUM stgm;
	HDROP hDropInfo;
	UINT nFiles;
	BOOL bOk = FALSE;

	if (!pDataObject) return FALSE;
	if (!pDataObject->GetData(CF_HDROP, &stgm)) return FALSE;
	if (stgm.tymed != TYMED_HGLOBAL) return FALSE;
	if (stgm.hGlobal == NULL) return FALSE;
	hDropInfo = (HDROP)stgm.hGlobal;
	nFiles = DragQueryFileW(hDropInfo, (UINT)-1, NULL, 0);
	if (nFiles)
	{
		UINT size = ::DragQueryFileW(hDropInfo, 0, nullptr, 0) + 1;
		std::vector<WCHAR> fileName(size, L'\0');
		if(DragQueryFileW(hDropInfo, 0, fileName.data(), size))
		{
			switch(m_itemDrag.type)
			{
			case MODITEM_MIDIINSTRUMENT:
				bOk = SetMidiInstrument(m_itemDrag.val1, mpt::PathString::FromNative(fileName.data()));
				break;
			case MODITEM_MIDIPERCUSSION:
				bOk = SetMidiPercussion(m_itemDrag.val1, mpt::PathString::FromNative(fileName.data()));
				break;
			}
		}
	}
	// After dropping some file on the MIDI library, we will need to do this or you
	// won't be able to select a new item until you started a new (internal) dragondrop.
	if(m_hItemDrag)
	{
		OnBeginDrag(m_hItemDrag, true, nullptr);
		OnEndDrag(TREESTATUS_DRAGGING);
	}
	m_itemDrag = ModItem(MODITEM_NULL);
	m_hItemDrag = NULL;
	::DragFinish(hDropInfo);
	return bOk;
}


void CModTree::OnRefreshInstrLib()
//--------------------------------
{
	HTREEITEM hActive;

	BeginWaitCursor();
	RefreshInstrumentLibrary();
	if (!IsSampleBrowser())
	{
		hActive = NULL;
		if(!m_InstrLibHighlightPath.empty() || !m_SongFileName.empty())
		{
			HTREEITEM hItem = GetChildItem(m_hInsLib);
			while (hItem != NULL)
			{
				const mpt::PathString str = mpt::PathString::FromCStringW(GetItemTextW(hItem));
				if(!mpt::PathString::CompareNoCase(str, m_InstrLibHighlightPath))
				{
					hActive = hItem;
					break;
				}
				hItem = GetNextItem(hItem, TVGN_NEXT);
			}
			if(m_SongFileName.empty()) m_InstrLibHighlightPath = MPT_PATHSTRING("");
		}
		SelectSetFirstVisible(m_hInsLib);
		if (hActive != NULL) SelectItem(hActive);
	}
	EndWaitCursor();
}


void CModTree::OnShowDirectories()
//--------------------------------
{
	TrackerSettings::Instance().showDirsInSampleBrowser = !TrackerSettings::Instance().showDirsInSampleBrowser;
	OnRefreshInstrLib();
}


void CModTree::OnShowAllFiles()
//-----------------------------
{
	if (!m_bShowAllFiles)
	{
		m_bShowAllFiles = true;
		OnRefreshInstrLib();
	}
}


void CModTree::OnShowSoundFiles()
//-------------------------------
{
	if (m_bShowAllFiles)
	{
		m_bShowAllFiles = false;
		OnRefreshInstrLib();
	}
}


void CModTree::OnSoundBankProperties()
//------------------------------------
{
	const ModItem modItem = GetModItem(GetSelectedItem());
	if(modItem.type == MODITEM_DLSBANK_FOLDER
		&& modItem.val1 < CTrackApp::gpDLSBanks.size() && CTrackApp::gpDLSBanks[modItem.val1])
	{
		CSoundBankProperties dlg(*CTrackApp::gpDLSBanks[modItem.val1], this);
		dlg.DoModal();
	}
}


LRESULT CModTree::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//----------------------------------------------------------------
{
	if(wParam == kcNull)
		return NULL;

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	ModCommand::NOTE note = NOTE_NONE;
	const bool start = wParam >= kcTreeViewStartNotes && wParam <= kcTreeViewEndNotes,
		stop = (wParam >= kcTreeViewStartNoteStops && wParam <= kcTreeViewEndNoteStops) && !IsSampleBrowser();
	if(start || stop)
	{
		note = static_cast<ModCommand::NOTE>(wParam - (start ? kcTreeViewStartNotes : kcTreeViewStartNoteStops) + 1 + pMainFrm->GetBaseOctave() * 12);
	} else if(wParam == kcTreeViewStopPreview)
	{
		note = NOTE_NOTECUT;
	}
	if(note != NOTE_NONE)
	{
		if(stop) note |= 0x80;

		if(PlayItem(GetSelectedItem(), note))
			return wParam;
		else
			return NULL;
	}

	return NULL;
}


void CModTree::OnKillFocus(CWnd* pNewWnd)
//--------------------------------------
{
	CTreeCtrl::OnKillFocus(pNewWnd);
	CMainFrame::GetMainFrame()->m_bModTreeHasFocus = false;

}


void CModTree::OnSetFocus(CWnd* pOldWnd)
//--------------------------------------
{
	CTreeCtrl::OnSetFocus(pOldWnd);
	CMainFrame::GetMainFrame()->m_bModTreeHasFocus = true;
}


bool CModTree::IsItemExpanded(HTREEITEM hItem)
//--------------------------------------------
{
	// checks if a treeview item is expanded.
	if(hItem == NULL) return false;
	TV_ITEM tvi;
	tvi.mask = TVIF_HANDLE | TVIF_STATE;
	tvi.state = 0;
	tvi.stateMask = TVIS_EXPANDED;
	tvi.hItem = hItem;
	GetItem(&tvi);
	return (tvi.state & TVIS_EXPANDED) != 0;
}


void CModTree::OnCloseItem()
//--------------------------
{
	HTREEITEM hItem = GetSelectedItem();
	CModDoc *pModDoc = GetDocumentFromItem(hItem);
	if(pModDoc == nullptr) return;
	// Spam our message to the first available view
	POSITION pos = pModDoc->GetFirstViewPosition();
	if(pos == NULL) return;
	CView* pView = pModDoc->GetNextView(pos);
	if (pView) pView->PostMessage(WM_COMMAND, ID_FILE_CLOSE);
}


// Delete all children of a tree item
void CModTree::DeleteChildren(HTREEITEM hItem)
//--------------------------------------------
{
	if(hItem != nullptr)
	{
		HTREEITEM hChildItem;
		while((hChildItem = GetChildItem(hItem)) != nullptr)
		{
			DeleteItem(hChildItem);
		}
	}
}


// Get the n-th child of a tree node
HTREEITEM CModTree::GetNthChildItem(HTREEITEM hItem, int index)
//-------------------------------------------------------------
{
	HTREEITEM hChildItem = nullptr;
	if(hItem != nullptr && ItemHasChildren(hItem))
	{
		hChildItem = GetChildItem(hItem);
		while(index-- > 0)
		{
			hChildItem = GetNextSiblingItem(hChildItem);
		}
	}
	return hChildItem;
}


// Gets the root parent of an item, i.e. if C is a child of B and B is a child of A, GetParentRootItem(C) returns A.
// A root item is considered to be its own parent, i.e. the returned value is only ever NULL if the input value was NULL.
HTREEITEM CModTree::GetParentRootItem(HTREEITEM hItem)
//----------------------------------------------------
{
	if(hItem != nullptr)
	{
		for(;;)
		{
			const HTREEITEM h = GetParentItem(hItem);
			if(h == nullptr || h == hItem) break;
			hItem = h;
		}
	}
	return hItem;
}


// Editing sample, instrument, order, pattern, etc. labels
void CModTree::OnBeginLabelEdit(NMHDR *nmhdr, LRESULT *result)
//------------------------------------------------------------
{
	NMTVDISPINFO *info = reinterpret_cast<NMTVDISPINFO *>(nmhdr);
	CEdit *editCtrl = GetEditControl();
	const ModItem modItem = GetModItem(info->item.hItem);
	const CModDoc *modDoc = modItem.IsSongItem() ? GetDocumentFromItem(info->item.hItem) : nullptr;

	if(editCtrl != nullptr && modDoc != nullptr)
	{
		const CSoundFile &sndFile = modDoc->GetrSoundFile();
		const CModSpecifications &modSpecs = sndFile.GetModSpecifications();
		std::string text;
		doLabelEdit = false;

		switch(modItem.type)
		{
		case MODITEM_ORDER:
			{
				PATTERNINDEX pat = sndFile.Order.GetSequence(static_cast<SEQUENCEINDEX>(modItem.val2)).At(static_cast<ORDERINDEX>(modItem.val1));
				if(pat == sndFile.Order.GetInvalidPatIndex())
					text = "---";
				else if(pat == sndFile.Order.GetIgnoreIndex())
					text = "+++";
				else
					text = mpt::ToString(pat);
				doLabelEdit = true;
			}
			break;

		case MODITEM_HDR_ORDERS:
			if(sndFile.Order.GetNumSequences() != 1 || sndFile.GetModSpecifications().sequencesMax <= 1)
			{
				break;
			}
			MPT_FALLTHROUGH;
		case MODITEM_SEQUENCE:
			if(modItem.val1 < sndFile.Order.GetNumSequences())
			{
				text = sndFile.Order.GetSequence(static_cast<SEQUENCEINDEX>(modItem.val1)).GetName();
				doLabelEdit = true;
			}
			break;

		case MODITEM_PATTERN:
			if(modItem.val1 < sndFile.Patterns.GetNumPatterns() && modSpecs.hasPatternNames)
			{
				text = sndFile.Patterns[modItem.val1].GetName();
				editCtrl->SetLimitText(MAX_PATTERNNAME - 1);
				doLabelEdit = true;
			}
			break;

		case MODITEM_SAMPLE:
			if(modItem.val1 <= sndFile.GetNumSamples())
			{
				text = sndFile.m_szNames[modItem.val1];
				editCtrl->SetLimitText(modSpecs.sampleNameLengthMax);
				doLabelEdit = true;
			}
			break;

		case MODITEM_INSTRUMENT:
			if(modItem.val1 <= sndFile.GetNumInstruments() && sndFile.Instruments[modItem.val1] != nullptr)
			{
				text = sndFile.Instruments[modItem.val1]->name;
				editCtrl->SetLimitText(modSpecs.instrNameLengthMax);
				doLabelEdit = true;
			}
			break;
		}

		if(doLabelEdit)
		{
			CMainFrame::GetInputHandler()->Bypass(true);
			editCtrl->SetWindowText(text.c_str());
			*result = FALSE;
			return;
		}
	}
	*result = TRUE;
}


// End editing sample, instrument, order, pattern, etc. labels
void CModTree::OnEndLabelEdit(NMHDR *nmhdr, LRESULT *result)
//----------------------------------------------------------
{
	CMainFrame::GetInputHandler()->Bypass(false);
	doLabelEdit = false;

	NMTVDISPINFO *info = reinterpret_cast<NMTVDISPINFO *>(nmhdr);
	const ModItem modItem = GetModItem(info->item.hItem);
	CModDoc *modDoc = modItem.IsSongItem() ? GetDocumentFromItem(info->item.hItem) : nullptr;

	if(info->item.pszText != nullptr && modDoc != nullptr)
	{
		CSoundFile &sndFile = modDoc->GetrSoundFile();
		const CModSpecifications &modSpecs = sndFile.GetModSpecifications();

		switch(modItem.type)
		{
		case MODITEM_ORDER:
			if(info->item.pszText[0])
			{
				PATTERNINDEX pat = ConvertStrTo<PATTERNINDEX>(info->item.pszText);
				bool valid = true;
				if(info->item.pszText[0] == '-')
				{
					pat = sndFile.Order.GetInvalidPatIndex();
				} else if(info->item.pszText[0] == '+')
				{
					if(modSpecs.hasIgnoreIndex)
						pat = sndFile.Order.GetIgnoreIndex();
					else
						valid = false;
				} else
				{
					valid = (pat < sndFile.Patterns.GetNumPatterns());
				}
				PATTERNINDEX &target = sndFile.Order.GetSequence(static_cast<SEQUENCEINDEX>(modItem.val2)).At(static_cast<ORDERINDEX>(modItem.val1));
				if(valid && pat != target)
				{
					target = pat;
					modDoc->SetModified();
					modDoc->UpdateAllViews(nullptr, SequenceHint().Data());
				}
			}
			break;

		case MODITEM_HDR_ORDERS:
		case MODITEM_SEQUENCE:
			if(modItem.val1 < sndFile.Order.GetNumSequences() && sndFile.Order.GetSequence(static_cast<SEQUENCEINDEX>(modItem.val1)).GetName() != info->item.pszText)
			{
				sndFile.Order.GetSequence(static_cast<SEQUENCEINDEX>(modItem.val1)).SetName(info->item.pszText);
				modDoc->SetModified();
				modDoc->UpdateAllViews(NULL, SequenceHint((SEQUENCEINDEX)modItem.val1).Data().Names());
			}
			break;

		case MODITEM_PATTERN:
			if(modItem.val1 < sndFile.Patterns.GetNumPatterns() && modSpecs.hasPatternNames && sndFile.Patterns[modItem.val1].GetName() != info->item.pszText)
			{
				sndFile.Patterns[modItem.val1].SetName(info->item.pszText);
				modDoc->SetModified();
				modDoc->UpdateAllViews(NULL, PatternHint((PATTERNINDEX)modItem.val1).Data().Names());
			}
			break;

		case MODITEM_SAMPLE:
			if(modItem.val1 <= sndFile.GetNumSamples() && strcmp(sndFile.m_szNames[modItem.val1], info->item.pszText))
			{
				mpt::String::CopyN(sndFile.m_szNames[modItem.val1], info->item.pszText, modSpecs.sampleNameLengthMax);
				modDoc->SetModified();
				modDoc->UpdateAllViews(NULL, SampleHint((SAMPLEINDEX)modItem.val1).Info().Names());
			}
			break;

		case MODITEM_INSTRUMENT:
			if(modItem.val1 <= sndFile.GetNumInstruments() && sndFile.Instruments[modItem.val1] != nullptr && strcmp(sndFile.Instruments[modItem.val1]->name, info->item.pszText))
			{
				mpt::String::CopyN(sndFile.Instruments[modItem.val1]->name, info->item.pszText, modSpecs.instrNameLengthMax);
				modDoc->SetModified();
				modDoc->UpdateAllViews(NULL, InstrumentHint((INSTRUMENTINDEX)modItem.val1).Info().Names());
			}
			break;
		}
	}
	*result = FALSE;
}


// Drop files from Windows
void CModTree::OnDropFiles(HDROP hDropInfo)
//-----------------------------------------
{
	bool refreshDLS = false;
	const UINT nFiles = ::DragQueryFileW(hDropInfo, (UINT)-1, NULL, 0);
	CMainFrame::GetMainFrame()->SetForegroundWindow();
	for(UINT f = 0; f < nFiles; f++)
	{
		UINT size = ::DragQueryFileW(hDropInfo, f, nullptr, 0) + 1;
		std::vector<WCHAR> fileName(size, L'\0');
		if(::DragQueryFileW(hDropInfo, f, fileName.data(), size))
		{
			mpt::PathString file(mpt::PathString::FromNative(fileName.data()));
			if(IsSampleBrowser())
			{
				// Set sample browser location to this directory or file
				const bool isSong = !file.IsDirectory();
				CModTree *dirBrowser = CMainFrame::GetMainFrame()->GetUpperTreeview();
				dirBrowser->m_InstrLibPath = file.GetPath();
				if(isSong) dirBrowser->RefreshInstrumentLibrary();
				dirBrowser->InstrumentLibraryChDir(file.GetFullFileName(), isSong);
				break;
			} else
			{
				if(CTrackApp::AddDLSBank(file))
				{
					refreshDLS = true;
				} else
				{
					// Pass message on
					theApp.OpenDocumentFile(file);
				}
			}
		}
	}
	if(refreshDLS)
	{
		RefreshDlsBanks();
	}
	::DragFinish(hDropInfo);
}


OPENMPT_NAMESPACE_END
