/*
 * View_tre.cpp
 * ------------
 * Purpose: Tree view for managing open songs, sound files, file browser, ...
 * Notes  : There are two instances of this class, one for the upper half and one for the lower half of the tree view.
 *          The lower half is referred to the sample browser in the code, i.e. IsSampleBrowser() returns true for code
 *          running in the lower half.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "View_tre.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "ImageLists.h"
#include "Moddoc.h"
#include "Dlsbank.h"
#include "dlg_misc.h"
#include "mpt/io_file/inputfile.hpp"
#include "mpt/io_file_read/inputfile_filecursor.hpp"
#include "../common/mptFileIO.h"
#include "mpt/fs/fs.hpp"
#include "../common/FileReader.h"
#include "FileDialog.h"
#include "Globals.h"
#include "ExternalSamples.h"
#include "FolderScanner.h"
#include "LinkResolver.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "../soundlib/MIDIEvents.h"

#include <winioctl.h>


OPENMPT_NAMESPACE_BEGIN


CSoundFile *CModTree::m_SongFile = nullptr;
CModTree::LibrarySortOrder CModTree::m_librarySort = LibrarySortOrder::Name;

/////////////////////////////////////////////////////////////////////////////
// CModTreeDropTarget


BOOL CModTreeDropTarget::Register(CModTree *pWnd)
{
	m_pModTree = pWnd;
	return COleDropTarget::Register(pWnd);
}


DROPEFFECT CModTreeDropTarget::OnDragEnter(CWnd *pWnd, COleDataObject *pDataObject, DWORD dwKeyState, CPoint point)
{
	if((m_pModTree) && (m_pModTree == pWnd))
		return m_pModTree->OnDragEnter(pDataObject, dwKeyState, point);
	return DROPEFFECT_NONE;
}


DROPEFFECT CModTreeDropTarget::OnDragOver(CWnd *pWnd, COleDataObject *pDataObject, DWORD dwKeyState, CPoint point)
{
	if((m_pModTree) && (m_pModTree == pWnd))
		return m_pModTree->OnDragOver(pDataObject, dwKeyState, point);
	return DROPEFFECT_NONE;
}


BOOL CModTreeDropTarget::OnDrop(CWnd *pWnd, COleDataObject *pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	if((m_pModTree) && (m_pModTree == pWnd))
		return m_pModTree->OnDrop(pDataObject, dropEffect, point);
	return FALSE;
}


ModTreeDocInfo::ModTreeDocInfo(CModDoc &modDoc)
    : modDoc(modDoc)
{
	const CSoundFile &sndFile = modDoc.GetSoundFile();
	tiPatterns.resize(sndFile.Patterns.Size(), nullptr);
	tiOrders.resize(sndFile.Order.GetNumSequences());
	tiSequences.resize(sndFile.Order.GetNumSequences(), nullptr);
}


/////////////////////////////////////////////////////////////////////////////
// CModTree

BEGIN_MESSAGE_MAP(CModTree, CTreeCtrl)
	//{{AFX_MSG_MAP(CViewModTree)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_XBUTTONUP()
	ON_WM_DROPFILES()

	ON_NOTIFY_REFLECT(NM_DBLCLK,          &CModTree::OnItemDblClk)
	ON_NOTIFY_REFLECT(NM_RETURN,          &CModTree::OnItemReturn)
	ON_NOTIFY_REFLECT(NM_RCLICK,          &CModTree::OnItemRightClick)
	ON_NOTIFY_REFLECT(NM_CLICK,           &CModTree::OnItemLeftClick)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED,   &CModTree::OnItemExpanded)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG,      &CModTree::OnBeginLDrag)
	ON_NOTIFY_REFLECT(TVN_BEGINRDRAG,     &CModTree::OnBeginRDrag)
	ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, &CModTree::OnBeginLabelEdit)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT,   &CModTree::OnEndLabelEdit)
	ON_NOTIFY_REFLECT(TVN_GETDISPINFO,    &CModTree::OnGetDispInfo)

	ON_COMMAND(ID_MODTREE_REFRESH,           &CModTree::OnRefreshTree)
	ON_COMMAND(ID_MODTREE_EXECUTE,           &CModTree::OnExecuteItem)
	ON_COMMAND(ID_MODTREE_REMOVE,            &CModTree::OnDeleteTreeItem)
	ON_COMMAND(ID_MODTREE_PLAY,              &CModTree::OnPlayTreeItem)
	ON_COMMAND(ID_MODTREE_REFRESHINSTRLIB,   &CModTree::OnRefreshInstrLib)
	ON_COMMAND(ID_MODTREE_OPENITEM,          &CModTree::OnOpenTreeItem)
	ON_COMMAND(ID_MODTREE_MUTE,              &CModTree::OnMuteTreeItem)
	ON_COMMAND(ID_MODTREE_MUTE_ONLY_EFFECTS, &CModTree::OnMuteOnlyEffects)
	ON_COMMAND(ID_MODTREE_SOLO,              &CModTree::OnSoloTreeItem)
	ON_COMMAND(ID_MODTREE_UNMUTEALL,         &CModTree::OnUnmuteAllTreeItem)
	ON_COMMAND(ID_MODTREE_DUPLICATE,         &CModTree::OnDuplicateTreeItem)
	ON_COMMAND(ID_MODTREE_INSERT,            &CModTree::OnInsertTreeItem)
	ON_COMMAND(ID_MODTREE_SWITCHTO,          &CModTree::OnSwitchToTreeItem)
	ON_COMMAND(ID_MODTREE_CLOSE,             &CModTree::OnCloseItem)
	ON_COMMAND(ID_MODTREE_SETPATH,           &CModTree::OnSetItemPath)
	ON_COMMAND(ID_MODTREE_SAVEITEM,          &CModTree::OnSaveItem)
	ON_COMMAND(ID_MODTREE_SAVEALL,           &CModTree::OnSaveAll)
	ON_COMMAND(ID_MODTREE_RELOADITEM,        &CModTree::OnReloadItem)
	ON_COMMAND(ID_MODTREE_RELOADALL,         &CModTree::OnReloadAll)
	ON_COMMAND(ID_MODTREE_FINDMISSING,       &CModTree::OnFindMissing)
	ON_COMMAND(ID_MODTREE_RENAME,            &CModTree::OnRenameItem)
	ON_COMMAND(ID_ADD_SOUNDBANK,             &CModTree::OnAddDlsBank)
	ON_COMMAND(ID_IMPORT_MIDILIB,            &CModTree::OnImportMidiLib)
	ON_COMMAND(ID_EXPORT_MIDILIB,            &CModTree::OnExportMidiLib)
	ON_COMMAND(ID_SOUNDBANK_PROPERTIES,      &CModTree::OnSoundBankProperties)
	ON_COMMAND(ID_MODTREE_SHOWDIRS,          &CModTree::OnShowDirectories)
	ON_COMMAND(ID_MODTREE_SHOWALLFILES,      &CModTree::OnShowAllFiles)
	ON_COMMAND(ID_MODTREE_SOUNDFILESONLY,    &CModTree::OnShowSoundFiles)
	ON_COMMAND(ID_MODTREE_GOTO_INSDIR,       &CModTree::OnGotoInstrumentDir)
	ON_COMMAND(ID_MODTREE_GOTO_SMPDIR,       &CModTree::OnGotoSampleDir)
	ON_COMMAND(ID_OPEN_LIBRARY_FILTER,       &CModTree::OnOpenInstrumentLibraryFilter)
	ON_COMMAND(ID_MODTREE_SORT_BY_NAME,      &CModTree::OnSortByName)
	ON_COMMAND(ID_MODTREE_SORT_BY_DATE,      &CModTree::OnSortByDate)
	ON_COMMAND(ID_MODTREE_SORT_BY_SIZE,      &CModTree::OnSortBySize)
	
	ON_MESSAGE(WM_MOD_KEYCOMMAND, &CModTree::OnCustomKeyMsg)
	ON_MESSAGE(WM_MOD_MIDIMSG,    &CModTree::OnMidiMsg)
	//}}AFX_MSG_MAP
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CViewModTree construction/destruction

CModTree::CModTree(CModTree *pDataTree)
	: m_pDataTree(pDataTree)
{
	if(m_pDataTree != nullptr)
	{
		// Set up instrument library monitoring thread
		m_hWatchDirKillThread = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		m_hSwitchWatchDir = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		m_WatchDirThread = std::thread([this]() { MonitorInstrumentLibrary(); });
	}
	MemsetZero(m_tiMidi);
	MemsetZero(m_tiPerc);

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
	// Wine does not support natural sorting with SORT_DIGITSASNUMBERS, fall back to normal sorting
	if(!::CompareString(LOCALE_USER_DEFAULT, m_stringCompareFlags, _T(""), -1, _T(""), -1))
		m_stringCompareFlags &= ~SORT_DIGITSASNUMBERS;
#endif
}


CModTree::~CModTree()
{
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
{
	m_modExtensions = CSoundFile::GetSupportedExtensions(false);
	m_MediaFoundationExtensions = FileType(CSoundFile::GetMediaFoundationFileTypes()).GetExtensions();

	DWORD dwRemove = TVS_SINGLEEXPAND;
	DWORD dwAdd = TVS_EDITLABELS | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;

	if(IsSampleBrowser())
	{
		dwRemove |= (TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS);
		dwAdd &= ~(TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS);
	}
	if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SINGLEEXPAND)
	{
		dwRemove &= ~TVS_SINGLEEXPAND;
		dwAdd |= TVS_SINGLEEXPAND;
		m_dwStatus |= TREESTATUS_SINGLEEXPAND;
	}
	ModifyStyle(dwRemove, dwAdd);
	ModifyStyleEx(0, WS_EX_ACCEPTFILES);

	if(!IsSampleBrowser())
	{
		std::vector<TCHAR> curDir(::GetCurrentDirectory(0, nullptr), '\0');
		::GetCurrentDirectory(static_cast<DWORD>(curDir.size()), curDir.data());
		mpt::PathString dirs[] =
		{
			TrackerSettings::Instance().PathSamples.GetDefaultDir(),
			TrackerSettings::Instance().PathInstruments.GetDefaultDir(),
			TrackerSettings::Instance().PathSongs.GetDefaultDir(),
			mpt::PathString::FromNative(curDir.data())
		};
		for(auto &path : dirs)
		{
			m_InstrLibPath = std::move(path);
			if(!m_InstrLibPath.empty())
				break;
		}
		m_InstrLibPath = m_InstrLibPath.WithTrailingSlash();
		m_pDataTree->InsLibSetFullPath(m_InstrLibPath, mpt::PathString());
	}

	SetImageList(&CMainFrame::GetMainFrame()->m_MiscIcons, TVSIL_NORMAL);
	if(!IsSampleBrowser())
	{
		// Create Midi Library
		m_hMidiLib = InsertItem(_T("MIDI Library"), IMAGE_FOLDER, IMAGE_FOLDER, TVI_ROOT, TVI_LAST);
		for(UINT iMidGrp = 0; iMidGrp < 17; iMidGrp++)
		{
			InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, mpt::ToCString(mpt::Charset::ASCII, szMidiGroupNames[iMidGrp]), IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, (MODITEM_HDR_MIDIGROUP << MIDILIB_SHIFT) | iMidGrp, m_hMidiLib, TVI_LAST);
		}
	}
	m_hInsLib = InsertItem(_T("Instrument Library"), IMAGE_FOLDER, IMAGE_FOLDER, TVI_ROOT, TVI_LAST);
	RefreshMidiLibrary();
	RefreshDlsBanks();
	RefreshInstrumentLibrary();
	m_DropTarget.Register(this);
}


BOOL CModTree::PreTranslateMessage(MSG *pMsg)
{
	if(!pMsg)
		return TRUE;

	if(m_doLabelEdit)
	{
		if(pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE))
		{
			EndEditLabelNow((pMsg->wParam == VK_ESCAPE) ? TRUE : FALSE);
			return TRUE;
		}
		return CTreeCtrl::PreTranslateMessage(pMsg);
	}

	if(pMsg->message == WM_KEYDOWN)
	{
		switch(pMsg->wParam)
		{
		case VK_TAB:
			// Tab: Switch between folder and file view.
			GetOtherView()->SetFocus();
			return TRUE;

		case VK_BACK:
			// Backspace: Go up one directory
			if(GetParentRootItem(GetSelectedItem()) == m_hInsLib || IsSampleBrowser())
			{
				InstrumentLibraryChDir(P_(".."), !m_SongFileName.empty());
				return TRUE;
			}
			break;

		case VK_APPS:
			// Handle Application (menu) key
			if(HTREEITEM item = GetSelectedItem())
			{
				CRect rect;
				GetItemRect(item, &rect, FALSE);
				ClientToScreen(rect);
				OnItemRightClick(item, rect.TopLeft() + CPoint{rect.Height() / 2, rect.Height() / 2});
			}
			return TRUE;

		case VK_ESCAPE:
			GetParent()->PostMessage(WM_COMMAND, ID_CLOSE_LIBRARY_FILTER);
			break;
		}
	} else if(pMsg->message == WM_CHAR)
	{
		ModItem item = GetModItem(GetSelectedItem());
		switch(item.type)
		{
		case MODITEM_SAMPLE:
		case MODITEM_INSTRUMENT:
		case MODITEM_MIDIINSTRUMENT:
		case MODITEM_MIDIPERCUSSION:
		case MODITEM_INSLIB_SAMPLE:
		case MODITEM_INSLIB_INSTRUMENT:
		case MODITEM_DLSBANK_INSTRUMENT:
			// Avoid cycling through tree-view elements on key hold
			return TRUE;
		}
	}

	// We handle keypresses before Windows has a chance to handle them (for alt etc..)
	if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
		(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
	{
		CInputHandler *ih = CMainFrame::GetInputHandler();
		if(ih->KeyEvent(kCtxViewTree, ih->Translate(*pMsg)) != kcNull)
			return true;  // Mapped to a command, no need to pass message on.
	}
	return CTreeCtrl::PreTranslateMessage(pMsg);
}


mpt::PathString CModTree::InsLibGetFullPath(HTREEITEM hItem) const
{
	mpt::PathString fullPath = m_InstrLibPath;
	fullPath = fullPath.WithTrailingSlash();
	return fullPath + mpt::PathString::FromCString(GetItemText(hItem));
}


bool CModTree::InsLibSetFullPath(const mpt::PathString &libPath, const mpt::PathString &songName)
{
	if(!songName.empty() && mpt::PathCompareNoCase(m_SongFileName, songName))
	{
		// Load module for previewing its instruments
		mpt::IO::InputFile f(libPath + songName, TrackerSettings::Instance().MiscCacheCompleteFileBeforeLoading);
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
					m_SongFile = new(std::nothrow) CSoundFile;
				}
				if(m_SongFile != nullptr)
				{
					try
					{
						if(!m_SongFile->Create(file, CSoundFile::loadNoPatternOrPluginData, nullptr))
						{
							return false;
						}
					} catch(mpt::out_of_memory e)
					{
						mpt::delete_out_of_memory(e);
						return false;
					} catch(const std::exception &)
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
{
	std::unique_ptr<CSoundFile> sndFile;
	try
	{
		sndFile = std::make_unique<CSoundFile>();
		if(!sndFile->Create(file, CSoundFile::loadNoPatternOrPluginData))
		{
			return false;
		}
	} catch(mpt::out_of_memory e)
	{
		mpt::delete_out_of_memory(e);
		return false;
	} catch(const std::exception &)
	{
		return false;
	}

	if(m_SongFile != nullptr)
	{
		m_SongFile->Destroy();
		delete m_SongFile;
	}
	m_SongFile = sndFile.release();
	m_SongFile->Patterns.DestroyPatterns();
	m_SongFile->m_songMessage.clear();
	const mpt::PathString fileName = file.GetOptionalFileName().value_or(P_(""));
	m_InstrLibPath = fileName.GetDirectoryWithDrive();
	m_SongFileName = fileName.GetFilename();
	RefreshInstrumentLibrary();
	return true;
}


CModTree* CModTree::GetOtherView()
{
	if(this == CMainFrame::GetMainFrame()->GetUpperTreeview())
		return CMainFrame::GetMainFrame()->GetLowerTreeview();
	else
		return CMainFrame::GetMainFrame()->GetUpperTreeview();
}

void CModTree::OnOptionsChanged()
{
	DWORD dwRemove = TVS_SINGLEEXPAND, dwAdd = 0;
	m_dwStatus &= ~TREESTATUS_SINGLEEXPAND;
	if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SINGLEEXPAND)
	{
		dwRemove = 0;
		dwAdd = TVS_SINGLEEXPAND;
		m_dwStatus |= TREESTATUS_SINGLEEXPAND;
	}
	ModifyStyle(dwRemove, dwAdd);
}


void CModTree::AddDocument(CModDoc &modDoc)
{
	const auto [it, inserted] = m_docInfo.try_emplace(&modDoc, modDoc);
	if(!inserted)
		return;

	auto &info = it->second;
	UpdateView(info, UpdateHint().ModType());
	if(info.hSong)
	{
		Expand(info.hSong, TVE_EXPAND);
		EnsureVisible(info.hSong);
		SelectItem(info.hSong);
	}
}


void CModTree::RemoveDocument(const CModDoc &modDoc)
{
	auto doc = m_docInfo.find(&modDoc);
	if(doc == m_docInfo.end())
		return;

	DeleteItem(doc->second.hSong);
	m_docInfo.erase(doc);
}


// Get CModDoc that is associated with a tree item
ModTreeDocInfo *CModTree::GetDocumentInfoFromItem(HTREEITEM hItem)
{
	hItem = GetParentRootItem(hItem);
	if(hItem != nullptr)
	{
		// Root item has moddoc pointer
		const auto doc = m_docInfo.find(reinterpret_cast<const CModDoc *>(GetItemData(hItem)));
		if(doc != m_docInfo.end() && hItem == doc->second.hSong)
		{
			return &doc->second;
		}
	}
	return nullptr;
}


// Get modtree doc information for a given CModDoc
ModTreeDocInfo *CModTree::GetDocumentInfoFromModDoc(CModDoc &modDoc)
{
	auto doc = m_docInfo.find(&modDoc);
	if(doc != m_docInfo.end())
		return &doc->second;
	else
		return nullptr;
}


size_t CModTree::GetDLSBankIndexFromItem(HTREEITEM hItem) const
{
	return static_cast<size_t>(std::distance(m_tiDLS.begin(), std::find(m_tiDLS.begin(), m_tiDLS.end(), GetParentRootItem(hItem))));
}


CDLSBank *CModTree::GetDLSBankFromItem(HTREEITEM hItem) const
{
	const auto bank = GetDLSBankIndexFromItem(hItem);
	if(bank < CTrackApp::gpDLSBanks.size())
		return CTrackApp::gpDLSBanks[bank].get();
	else
		return nullptr;
}


/////////////////////////////////////////////////////////////////////////////
// CViewModTree drawing

void CModTree::RefreshMidiLibrary()
{
	CString s;
	CString stmp;
	TVITEM tvi;
	const MidiLibrary &midiLib = CTrackApp::GetMidiLibrary();

	if(IsSampleBrowser())
		return;
	// Midi Programs
	HTREEITEM parent = GetChildItem(m_hMidiLib);
	for(UINT iMidi = 0; iMidi < 128; iMidi++)
	{
		DWORD dwImage = IMAGE_INSTRMUTE;
		s = mpt::cfmt::val(iMidi) + _T(": ") + mpt::ToCString(mpt::Charset::ASCII, szMidiProgramNames[iMidi]);
		const LPARAM param = (MODITEM_MIDIINSTRUMENT << MIDILIB_SHIFT) | iMidi;
		if(!midiLib[iMidi].empty())
		{
			s += _T(": ") + midiLib[iMidi].GetFilename().ToCString();
			dwImage = IMAGE_INSTRUMENTS;
		}
		if(!m_tiMidi[iMidi])
		{
			m_tiMidi[iMidi] = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
							s, dwImage, dwImage, 0, 0, param, parent, TVI_LAST);
		} else
		{
			tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
			tvi.hItem = m_tiMidi[iMidi];
			tvi.pszText = stmp.GetBuffer(s.GetLength() + 1);
			tvi.cchTextMax = stmp.GetAllocLength();
			tvi.iImage = tvi.iSelectedImage = dwImage;
			GetItem(&tvi);
			s.ReleaseBuffer();
			if(s != stmp || tvi.iImage != (int)dwImage)
			{
				SetItem(m_tiMidi[iMidi], TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
					s, dwImage, dwImage, 0, 0, param);
			}
		}
		if((iMidi % 8u) == 7u)
		{
			parent = GetNextSiblingItem(parent);
		}
	}
	// Midi Percussions
	for(UINT iPerc = 24; iPerc <= 84; iPerc++)
	{
		DWORD dwImage = IMAGE_NOSAMPLE;
		s = mpt::ToCString(CSoundFile::GetNoteName((ModCommand::NOTE)(iPerc + NOTE_MIN), CSoundFile::GetDefaultNoteNames()))
		    + _T(": ") + mpt::ToCString(mpt::Charset::ASCII, szMidiPercussionNames[iPerc - 24]);
		const LPARAM param = (MODITEM_MIDIPERCUSSION << MIDILIB_SHIFT) | iPerc;
		if(!midiLib[iPerc | 0x80].empty())
		{
			s += _T(": ") + midiLib[iPerc | 0x80].GetFilename().ToCString();
			dwImage = IMAGE_SAMPLES;
		}
		if(!m_tiPerc[iPerc])
		{
			m_tiPerc[iPerc] = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
							s, dwImage, dwImage, 0, 0, param, parent, TVI_LAST);
		} else
		{
			tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
			tvi.hItem = m_tiPerc[iPerc];
			tvi.pszText = stmp.GetBuffer(s.GetLength() + 1);
			tvi.cchTextMax = stmp.GetAllocLength();
			tvi.iImage = tvi.iSelectedImage = dwImage;
			GetItem(&tvi);
			s.ReleaseBuffer();
			if(s != stmp || tvi.iImage != (int)dwImage)
			{
				SetItem(m_tiPerc[iPerc], TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
							s, dwImage, dwImage, 0, 0, param);
			}
		}
	}
}


void CModTree::RefreshDlsBanks(const bool forceRefresh)
{
	const mpt::Charset charset = mpt::Charset::Locale;

	if(IsSampleBrowser())
		return;

	if(m_tiDLS.size() < CTrackApp::gpDLSBanks.size())
	{
		m_tiDLS.resize(CTrackApp::gpDLSBanks.size(), nullptr);
	}

	auto filter = mpt::ToWin(m_filterString);
	const bool applyFilter = !filter.empty();
	if(applyFilter)
		filter = _T("*") + filter + _T("*");

	LockRedraw();
	HTREEITEM hInsertAfter = m_hMidiLib;
	for(size_t iDls = 0; iDls < CTrackApp::gpDLSBanks.size(); iDls++)
	{
		if(!CTrackApp::gpDLSBanks[iDls])
		{
			// Was this bank removed?
			if(m_tiDLS[iDls])
			{
				DeleteItem(m_tiDLS[iDls]);
				m_tiDLS[iDls] = nullptr;
			}
			continue;
		}

		if(m_tiDLS[iDls] != nullptr && !forceRefresh)
			continue;

		// Add DLS file folder
		const CDLSBank &dlsBank = *CTrackApp::gpDLSBanks[iDls];
		if(m_tiDLS[iDls])
			DeleteChildren(m_tiDLS[iDls]);
		else
			m_tiDLS[iDls] = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
				dlsBank.GetFileName().GetFilename().AsNative().c_str(), IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, iDls, TVI_ROOT, hInsertAfter);

		std::map<uint16, HTREEITEM> banks;
		HTREEITEM hDrums = nullptr;

		// Add Instruments (done backwards to improve performance as per https://devblogs.microsoft.com/oldnewthing/20111125-00/?p=9033)
		MPT_ASSERT(dlsBank.GetNumInstruments() <= 0x10000);
		for(auto iIns = dlsBank.GetNumInstruments(); iIns > 0;)
		{
			iIns--;
			const DLSINSTRUMENT *pDlsIns = dlsBank.GetInstrument(iIns);
			if(!pDlsIns)
				continue;

			TCHAR szName[256];
			const LPARAM lParamInstr = DlsItem::ToLPARAM(static_cast<uint16>(iIns), (pDlsIns->ulInstrument & 0x7F), false);
			if(pDlsIns->ulBank & F_INSTRUMENT_DRUMS)
			{
				// Drum Kit
				HTREEITEM hKit = nullptr;
				MPT_ASSERT(pDlsIns->Regions.size() <= 0x8000);
				for(auto region = static_cast<uint32>(pDlsIns->Regions.size()); region > 0;)
				{
					region--;
					if(pDlsIns->Regions[region].IsDummy())
						continue;

					UINT keymin = pDlsIns->Regions[region].uKeyMin;
					UINT keymax = pDlsIns->Regions[region].uKeyMax;

					const char *regionName = dlsBank.GetRegionName(iIns, region);
					if(regionName == nullptr || !regionName[0])
					{
						if(keymin >= 24 && keymin <= 84)
							regionName = szMidiPercussionNames[keymin - 24];
						else
							regionName = "";
					}

					const auto regionNameStr = mpt::ToWin(charset, regionName);
					if(applyFilter && !PathMatchSpec(regionNameStr.c_str(), filter.c_str()))
						continue;

					if(!hKit)
					{
						if(!hDrums)
							hDrums = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, _T("Drum Kits"), IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, DLS_DRUM_FOLDER_LPARAM, m_tiDLS[iDls], TVI_LAST);

						wsprintf(szName, _T("%u: %s"), pDlsIns->ulInstrument & 0x7F, mpt::ToWin(charset, pDlsIns->szName).c_str());
						hKit = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, szName, IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, lParamInstr, hDrums, TVI_FIRST);
					}

					if(keymin >= keymax)
					{
						wsprintf(szName, _T("%s%u: %s"),
							mpt::ToWin(CSoundFile::GetDefaultNoteName(keymin % 12)).c_str(),
							keymin / 12,
							regionNameStr.c_str());
					} else
					{
						wsprintf(szName, _T("%s%u-%s%u: %s"),
							mpt::ToWin(CSoundFile::GetDefaultNoteName(keymin % 12)).c_str(),
							keymin / 12,
							mpt::ToWin(CSoundFile::GetDefaultNoteName(keymax % 12)).c_str(),
							keymax / 12,
							regionNameStr.c_str());
					}

					LPARAM lParam = DlsItem::ToLPARAM(static_cast<uint16>(iIns), static_cast<uint16>(region), true);
					InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
							szName, IMAGE_INSTRUMENTS, IMAGE_INSTRUMENTS, 0, 0, lParam, hKit, TVI_FIRST);
				}
				if(applyFilter && hKit)
					Expand(hKit, TVE_EXPAND);
			} else
			{
				// Melodic
				const auto instrName = mpt::ToWin(charset, pDlsIns->szName);
				if(applyFilter && !PathMatchSpec(instrName.c_str(), filter.c_str()))
					continue;

				uint16 mbank = (pDlsIns->ulBank & 0x7F7F);
				auto hbank = banks.find(mbank);
				if(hbank == banks.end())
				{
					wsprintf(szName, mbank ? _T("Melodic Bank %02d.%02d") : _T("Melodic"), mbank >> 8, mbank & 0x7F);
					// Find out where to insert this bank in the tree
					hbank = banks.insert(std::make_pair(mbank, nullptr)).first;
					HTREEITEM insertAfter = (hbank == banks.begin()) ? TVI_FIRST : std::prev(hbank)->second;
					hbank->second = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
						szName, IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, 0,
						m_tiDLS[iDls], insertAfter);
				}

				wsprintf(szName, _T("%u: %s"), pDlsIns->ulInstrument & 0x7F, instrName.c_str());
				InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
					szName, IMAGE_INSTRUMENTS, IMAGE_INSTRUMENTS, 0, 0, lParamInstr, hbank->second, TVI_FIRST);
			}
		}
		if(applyFilter)
		{
			for(const auto &bank : banks)
			{
				Expand(bank.second, TVE_EXPAND);
			}
		}
		hInsertAfter = m_tiDLS[iDls];
	}
	UnlockRedraw();
}


void CModTree::RefreshInstrumentLibrary()
{
	LockRedraw();
	// Check if the currently selected item should be selected after refreshing
	CString selectedName;
	if((IsSampleBrowser() || GetParentRootItem(GetSelectedItem()) == m_hInsLib)
	   && GetItemText(GetSampleBrowser()->m_hInsLib) == (m_SongFileName.empty() ? m_InstrLibPath : m_SongFileName).ToCString())
	{
		selectedName = GetItemText(GetSelectedItem());
	}
	if(!m_InstrLibHighlightPath.empty())
	{
		selectedName = m_InstrLibHighlightPath.ToCString();
	}
	m_InstrLibHighlightPath = {};
	FillInstrumentLibrary(selectedName);
	auto selectedItem = GetSelectedItem();
	if(selectedItem)
		EnsureVisible(selectedItem);
	UnlockRedraw();
	if(!IsSampleBrowser())
	{
		m_pDataTree->InsLibSetFullPath(m_InstrLibPath, m_SongFileName);
		m_pDataTree->RefreshInstrumentLibrary();
	}
}


void CModTree::UpdateView(ModTreeDocInfo &info, UpdateHint hint)
{
	TCHAR stmp[256];
	TVITEM tvi;
	MemsetZero(tvi);
	const FlagSet<HintType> hintType = hint.GetType();
	if(IsSampleBrowser() || hintType == HINT_NONE)
		return;

	const CModDoc &modDoc = info.modDoc;
	const CSoundFile &sndFile = modDoc.GetSoundFile();

	// Create headers
	const GeneralHint generalHint = hint.ToType<GeneralHint>();
	if(generalHint.GetType()[HINT_MODTYPE | HINT_MODGENERAL] || (!info.hSong))
	{
		// Module folder + sub folders
		CString name = modDoc.GetPathNameMpt().GetFilename().ToCString();
		if(name.IsEmpty())
			name = SanitizePathComponent(modDoc.GetTitle());

		if(!info.hSong)
		{
			info.hSong = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, name, IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, reinterpret_cast<LPARAM>(&info.modDoc), TVI_ROOT, TVI_FIRST);
			info.hOrders = InsertItem(_T("Sequence"), IMAGE_FOLDER, IMAGE_FOLDER, info.hSong, TVI_LAST);
			info.hPatterns = InsertItem(_T("Patterns"), IMAGE_FOLDER, IMAGE_FOLDER, info.hSong, TVI_LAST);
			info.hSamples = InsertItem(_T("Samples"), IMAGE_FOLDER, IMAGE_FOLDER, info.hSong, TVI_LAST);
		} else if(generalHint.GetType()[HINT_MODGENERAL | HINT_MODTYPE])
		{
			if(name != GetItemText(info.hSong))
			{
				SetItemText(info.hSong, name);
			}
		}
	}

	if(sndFile.GetModSpecifications().instrumentsMax > 0)
	{
		if(!info.hInstruments)
			info.hInstruments = InsertItem(_T("Instruments"), IMAGE_FOLDER, IMAGE_FOLDER, info.hSong, info.hSamples);
	} else
	{
		if(info.hInstruments)
		{
			DeleteItem(info.hInstruments);
			info.hInstruments = NULL;
		}
	}
	if(!info.hComments)
		info.hComments = InsertItem(_T("Comments"), IMAGE_COMMENTS, IMAGE_COMMENTS, info.hSong, TVI_LAST);
	// Add effects
	const PluginHint pluginHint = hint.ToType<PluginHint>();
	if(pluginHint.GetType()[HINT_MODTYPE | HINT_PLUGINNAMES])
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

				CString s = MPT_CFORMAT("FX{}: {}")(i + 1, mpt::ToCString(plugin.GetName()));
				int nImage = IMAGE_NOPLUGIN;
				if(plugin.pMixPlugin != nullptr)
					nImage = (plugin.pMixPlugin->IsInstrument()) ? IMAGE_PLUGININSTRUMENT : IMAGE_EFFECTPLUGIN;

				if(hItem)
				{
					// Replace existing item
					tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
					tvi.hItem = hItem;
					tvi.pszText = stmp;
					tvi.cchTextMax = mpt::saturate_cast<int>(std::size(stmp));
					GetItem(&tvi);
					if(tvi.iImage != nImage || tvi.lParam != i || s != CString(tvi.pszText))
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
			for(const auto &plug : sndFile.m_MixPlugins)
			{
				if(plug.IsValidPlugin())
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
	if(info.hOrders && (seqHint.GetType()[HINT_MODTYPE | HINT_MODSEQUENCE | HINT_SEQNAMES] || patternHint.GetType()[HINT_PATNAMES]))
	{
		const PATTERNINDEX nPat = patternHint.GetPattern();
		bool adjustParentNode = false;  // adjust sequence name of "Sequence" node?

		// (only one seq remaining || previously only one sequence): update parent item
		if((info.tiSequences.size() > 1 && sndFile.Order.GetNumSequences() == 1) || (info.tiSequences.size() == 1 && sndFile.Order.GetNumSequences() > 1))
		{
			for(auto &seq : info.tiOrders)
			{
				for(auto &ord : seq)
				{
					if(ord)
						DeleteItem(ord);
					ord = nullptr;
				}
			}
			for(auto &seq : info.tiSequences)
			{
				if(seq)
					DeleteItem(seq);
				seq = nullptr;
			}
			info.tiOrders.resize(sndFile.Order.GetNumSequences());
			info.tiSequences.resize(sndFile.Order.GetNumSequences(), nullptr);
			adjustParentNode = true;
		}

		// If there are too many sequences, delete them.
		for(size_t seq = sndFile.Order.GetNumSequences(); seq < info.tiSequences.size(); seq++) if (info.tiSequences[seq])
		{
			for(auto &ord : info.tiOrders[seq]) if (ord)
			{
				DeleteItem(ord); ord = nullptr;
			}
			DeleteItem(info.tiSequences[seq]); info.tiSequences[seq] = nullptr;
		}
		if (info.tiSequences.size() < sndFile.Order.GetNumSequences()) // Resize tiSequences if needed.
		{
			info.tiSequences.resize(sndFile.Order.GetNumSequences(), nullptr);
			info.tiOrders.resize(sndFile.Order.GetNumSequences());
		}

		HTREEITEM hAncestorNode = info.hOrders;

		SEQUENCEINDEX nSeqMin = 0, nSeqMax = sndFile.Order.GetNumSequences() - 1;
		SEQUENCEINDEX nHintParam = seqHint.GetSequence();
		if(seqHint.GetType()[HINT_SEQNAMES] && (nHintParam <= nSeqMax))
			nSeqMin = nSeqMax = nHintParam;

		// Adjust caption of the "Sequence" node (if only one sequence exists, it should be labeled with the sequence name)
		if((seqHint.GetType()[HINT_SEQNAMES] && sndFile.Order.GetNumSequences() == 1) || adjustParentNode)
		{
			CString seqName = mpt::ToCString(sndFile.Order(0).GetName());
			if(seqName.IsEmpty() || sndFile.Order.GetNumSequences() > 1)
				seqName = _T("Sequence");
			else
				seqName = _T("Sequence: ") + seqName;
			SetItem(info.hOrders, TVIF_TEXT, seqName, 0, 0, 0, 0, 0);
		}

		// go through all sequences
		CString seqName;
		for(SEQUENCEINDEX seq = nSeqMin; seq <= nSeqMax; seq++)
		{
			if(sndFile.Order.GetNumSequences() > 1)
			{
				// more than one sequence -> add folder
				if(sndFile.Order(seq).GetName().empty())
				{
					seqName = MPT_CFORMAT("Sequence {}")(seq + 1);
				} else
				{
					seqName = MPT_CFORMAT("{}: ")(seq + 1);
					seqName += mpt::ToCString(sndFile.Order(seq).GetName());
				}

				UINT state = (seq == sndFile.Order.GetCurrentSequenceIndex()) ? TVIS_BOLD : 0;

				if(info.tiSequences[seq] == NULL)
				{
					info.tiSequences[seq] = InsertItem(seqName, IMAGE_FOLDER, IMAGE_FOLDER, info.hOrders, TVI_LAST);
				}
				// Update bold item
				_tcscpy(stmp, seqName);
				tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_STATE | TVIF_PARAM;
				tvi.state = 0;
				tvi.stateMask = TVIS_BOLD;
				tvi.hItem = info.tiSequences[seq];
				tvi.pszText = stmp;
				tvi.cchTextMax = mpt::saturate_cast<int>(std::size(stmp));
				LPARAM param = (seq << SEQU_SHIFT) | ORDERINDEX_INVALID;
				GetItem(&tvi);
				if(tvi.state != state || tvi.pszText != seqName || tvi.lParam != param)
					SetItem(info.tiSequences[seq], TVIF_TEXT | TVIF_STATE | TVIF_PARAM, seqName, 0, 0, state, TVIS_BOLD, param);

				hAncestorNode = info.tiSequences[seq];
			}

			const ORDERINDEX ordLength = sndFile.Order(seq).GetLengthTailTrimmed();
			// If there are items past the new sequence length, delete them.
			for(size_t nOrd = ordLength; nOrd < info.tiOrders[seq].size(); nOrd++) if (info.tiOrders[seq][nOrd])
			{
				DeleteItem(info.tiOrders[seq][nOrd]); info.tiOrders[seq][nOrd] = NULL;
			}
			if (info.tiOrders[seq].size() < ordLength) // Resize tiOrders if needed.
				info.tiOrders[seq].resize(ordLength, nullptr);
			const bool patNamesOnly = patternHint.GetType()[HINT_PATNAMES];

			//if (hintFlagPart == HINT_PATNAMES) && (dwHintParam < sndFile.Order().GetLength())) imin = imax = dwHintParam;
			CString patName;
			for(ORDERINDEX iOrd = 0; iOrd < ordLength; iOrd++)
			{
				TCHAR s[256];
				s[0] = 0;
				if(patNamesOnly && sndFile.Order(seq)[iOrd] != nPat)
					continue;
				UINT state = (iOrd == info.ordSel && seq == info.seqSel) ? TVIS_BOLD : 0;
				if(sndFile.Order(seq)[iOrd] < sndFile.Patterns.Size())
				{
					patName = mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.Patterns[sndFile.Order(seq)[iOrd]].GetName());
					if(!patName.IsEmpty())
					{
						wsprintf(s, (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_HEXDISPLAY) ? _T("[%02Xh] %u: ") : _T("[%02u] %u: "),
							iOrd, sndFile.Order(seq)[iOrd]);
						_tcscat(s, patName.GetString());
					} else
					{
						wsprintf(s, (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_HEXDISPLAY) ? _T("[%02Xh] Pattern %u") : _T("[%02u] Pattern %u"),
							iOrd, sndFile.Order(seq)[iOrd]);
					}
				} else
				{
					if(sndFile.Order(seq)[iOrd] == sndFile.Order.GetIgnoreIndex())
					{
						// +++ Item
						wsprintf(s, _T("[%02u] Skip"), iOrd);
					} else
					{
						// --- Item
						wsprintf(s, _T("[%02u] Stop"), iOrd);
					}
				}

				LPARAM param = (seq << SEQU_SHIFT) | iOrd;
				if(info.tiOrders[seq][iOrd])
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_STATE;
					tvi.state = 0;
					tvi.stateMask = TVIS_BOLD;
					tvi.hItem = info.tiOrders[seq][iOrd];
					tvi.pszText = stmp;
					tvi.cchTextMax = mpt::saturate_cast<int>(std::size(stmp));
					GetItem(&tvi);
					if(tvi.state != state || _tcscmp(s, stmp))
						SetItem(info.tiOrders[seq][iOrd], TVIF_TEXT | TVIF_STATE | TVIF_PARAM, s, 0, 0, state, TVIS_BOLD, param);
				} else
				{
					info.tiOrders[seq][iOrd] = InsertItem(TVIF_HANDLE | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, s, IMAGE_PARTITION, IMAGE_PARTITION, 0, 0, param, hAncestorNode, TVI_LAST);
				}
			}
		}
	}
	// Add Patterns
	if(info.hPatterns && patternHint.GetType()[HINT_MODTYPE | HINT_PATNAMES])
	{
		const PATTERNINDEX nPat = patternHint.GetPattern();
		PATTERNINDEX minPat = 0, maxPat = sndFile.Patterns.Size();
		if(patternHint.GetType()[HINT_PATNAMES] && nPat < sndFile.Patterns.Size())
		{
			minPat = nPat;
			maxPat = nPat + 1;
		}

		for(size_t pat = sndFile.Patterns.Size(); pat < info.tiPatterns.size(); pat++)
		{
			if(info.tiPatterns[pat])
				DeleteItem(info.tiPatterns[pat]);
		}
		info.tiPatterns.resize(sndFile.Patterns.Size(), nullptr);

		mpt::winstring patName;
		for(PATTERNINDEX pat = minPat; pat < maxPat; pat++)
		{
			TCHAR s[256];
			s[0] = 0;
			if(sndFile.Patterns.IsValidPat(pat))
			{
				patName = mpt::ToWin(sndFile.GetCharsetInternal(), sndFile.Patterns[pat].GetName());
				wsprintf(s, _T("%u"), pat);
				if(!patName.empty())
				{
					_tcscat(s, _T(": "));
					_tcscat(s, patName.c_str());
				}
				if(info.tiPatterns[pat])
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE;
					tvi.hItem = info.tiPatterns[pat];
					tvi.pszText = stmp;
					tvi.cchTextMax = mpt::saturate_cast<int>(std::size(stmp));
					GetItem(&tvi);
					if(_tcscmp(s, stmp))
						SetItem(info.tiPatterns[pat], TVIF_TEXT, s, 0, 0, 0, 0, 0);
				} else
				{
					info.tiPatterns[pat] = InsertItem(s, IMAGE_PATTERNS, IMAGE_PATTERNS, info.hPatterns, TVI_LAST);
				}
				SetItemData(info.tiPatterns[pat], pat);
			} else if(pat < info.tiPatterns.size() && info.tiPatterns[pat])
			{
				DeleteItem(info.tiPatterns[pat]);
				info.tiPatterns[pat] = nullptr;
			}
		}
	}
	// Add Samples
	const SampleHint sampleHint = hint.ToType<SampleHint>();
	if(info.hSamples && sampleHint.GetType()[HINT_MODTYPE | HINT_SMPNAMES | HINT_SAMPLEINFO | HINT_SAMPLEDATA])
	{
		const SAMPLEINDEX hintSmp = sampleHint.GetSample();
		SAMPLEINDEX smin = 1, smax = MAX_SAMPLES - 1;
		if(sampleHint.GetType()[HINT_SMPNAMES | HINT_SAMPLEINFO | HINT_SAMPLEDATA] && hintSmp > 0 && hintSmp < MAX_SAMPLES)
		{
			smin = smax = hintSmp;
		}
		HTREEITEM hChild = GetNthChildItem(info.hSamples, smin - 1);
		for(SAMPLEINDEX nSmp = smin; nSmp <= smax; nSmp++)
		{
			TCHAR s[256];
			s[0] = 0;
			HTREEITEM hNextChild = GetNextSiblingItem(hChild);
			if(nSmp <= sndFile.GetNumSamples())
			{
				const ModSample &sample = sndFile.GetSample(nSmp);
				const bool sampleExists = (sample.HasSampleData());

				static constexpr int Images[] =
				{
					IMAGE_NOSAMPLE,  IMAGE_NOSAMPLE,        IMAGE_NOSAMPLE,
					IMAGE_SAMPLES,   IMAGE_SAMPLEACTIVE,    IMAGE_SAMPLEMUTE,
					IMAGE_EXTSAMPLE, IMAGE_EXTSAMPLEACTIVE, IMAGE_EXTSAMPLEMUTE,
					IMAGE_OPLINSTR,  IMAGE_OPLINSTRACTIVE,  IMAGE_OPLINSTRMUTE,
				};

				int image = 0;
				if(sampleExists)
					image = 3;
				if(sample.uFlags[SMP_KEEPONDISK])
					image = 6;
				if(sample.uFlags[CHN_ADLIB])
					image = 9;

				if(info.modDoc.IsSampleMuted(nSmp))
					image += 2;
				else if(info.samplesPlaying[nSmp])
					image++;

				if(sample.uFlags[SMP_KEEPONDISK] && !sampleExists)
					image = IMAGE_EXTSAMPLEMISSING;
				else
					image = Images[image];

				if(sndFile.GetType() == MOD_TYPE_MPT)
				{
					const TCHAR *status = _T("");
					if(sample.uFlags[SMP_KEEPONDISK])
					{
						status = sampleExists ? _T(" [external]") : _T(" [MISSING]");
					}
					wsprintf(s, _T("%3d: %s%s%s"), nSmp, sample.uFlags.test_all(SMP_MODIFIED | SMP_KEEPONDISK) ? _T("* ") : _T(""), mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.m_szNames[nSmp]).GetString(), status);
				} else
				{
					wsprintf(s, _T("%3d: %s"), nSmp, mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.m_szNames[nSmp]).GetString());
				}

				if(!hChild)
				{
					hChild = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, s, image, image, 0, 0, nSmp, info.hSamples, TVI_LAST);
				} else
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE;
					tvi.hItem = hChild;
					tvi.pszText = stmp;
					tvi.cchTextMax = mpt::saturate_cast<int>(std::size(stmp));
					tvi.iImage = tvi.iSelectedImage = image;
					GetItem(&tvi);
					if(tvi.iImage != image || _tcsncmp(s, stmp, std::size(stmp)) || GetItemData(hChild) != nSmp)
					{
						SetItem(hChild, TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, s, image, image, 0, 0, nSmp);
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
	if(info.hInstruments && instrHint.GetType()[HINT_MODTYPE | HINT_INSNAMES | HINT_INSTRUMENT])
	{
		INSTRUMENTINDEX smin = 1, smax = MAX_INSTRUMENTS - 1;
		const INSTRUMENTINDEX hintIns = instrHint.GetInstrument();
		if(instrHint.GetType()[HINT_INSNAMES | HINT_INSTRUMENT] && hintIns > 0 && hintIns < MAX_INSTRUMENTS)
		{
			smin = smax = hintIns;
		}
		HTREEITEM hChild = GetNthChildItem(info.hInstruments, smin - 1);
		for(INSTRUMENTINDEX nIns = smin; nIns <= smax; nIns++)
		{
			TCHAR s[256];
			s[0] = 0;
			HTREEITEM hNextChild = GetNextSiblingItem(hChild);
			if(nIns <= sndFile.GetNumInstruments())
			{
				wsprintf(s, _T("%3u: %s"), nIns, mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.GetInstrumentName(nIns)).GetString());

				int nImage = IMAGE_INSTRUMENTS;
				if(info.instrumentsPlaying[nIns])
					nImage = IMAGE_INSTRACTIVE;
				if(!sndFile.Instruments[nIns] || info.modDoc.IsInstrumentMuted(nIns))
					nImage = IMAGE_INSTRMUTE;

				if(!hChild)
				{
					hChild = InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, s, nImage, nImage, 0, 0, nIns, info.hInstruments, TVI_LAST);
				} else
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE;
					tvi.hItem = hChild;
					tvi.pszText = stmp;
					tvi.cchTextMax = mpt::saturate_cast<int>(std::size(stmp));
					tvi.iImage = tvi.iSelectedImage = nImage;
					GetItem(&tvi);
					if(tvi.iImage != nImage || _tcscmp(s, stmp) || GetItemData(hChild) != nIns)
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
{
	if(!hItem)
		return ModItem(MODITEM_NULL);
	// First, test root items
	if(hItem == m_hInsLib)
		return ModItem(MODITEM_HDR_INSTRUMENTLIB);
	if(hItem == m_hMidiLib)
		return ModItem(MODITEM_HDR_MIDILIB);

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
		TVITEM tvi;
		tvi.mask = TVIF_IMAGE | TVIF_HANDLE;
		tvi.hItem = hItem;
		tvi.iImage = 0;
		if(GetItem(&tvi))
		{
			switch(tvi.iImage)
			{
			case IMAGE_SAMPLES:
			case IMAGE_OPLINSTR:
				// Sample
				return ModItem(MODITEM_INSLIB_SAMPLE);
			case IMAGE_INSTRUMENTS:
				// Instrument
				return ModItem(MODITEM_INSLIB_INSTRUMENT);
			case IMAGE_FOLDERSONG:
				// Song
				return ModItem(MODITEM_INSLIB_SONG);
			default:
				return ModItem(MODITEM_INSLIB_FOLDER);
			}
		}
		return ModItem(MODITEM_NULL);
	}
	if(IsSampleBrowser())
		return ModItem(MODITEM_NULL);
	// Songs
	if(auto info = GetDocumentInfoFromItem(hRootParent); info != nullptr)
	{
		m_selectedDoc = &info->modDoc;

		if(hItem == info->hSong)
			return ModItem(MODITEM_HDR_SONG);
		if(hRootParent == info->hSong)
		{
			if(hItem == info->hPatterns)
				return ModItem(MODITEM_HDR_PATTERNS);
			if(hItem == info->hOrders)
				return ModItem(MODITEM_HDR_ORDERS);
			if(hItem == info->hSamples)
				return ModItem(MODITEM_HDR_SAMPLES);
			if(hItem == info->hInstruments)
				return ModItem(MODITEM_HDR_INSTRUMENTS);
			if(hItem == info->hEffects)
				return ModItem(MODITEM_HDR_EFFECTS);
			if(hItem == info->hComments)
				return ModItem(MODITEM_COMMENTS);
			// Order List or Sequence item?
			if((hItemParent == info->hOrders) || (hItemParentParent == info->hOrders))
			{
				const auto ord = static_cast<ORDERINDEX>(itemData & SEQU_MASK);
				const auto seq = static_cast<SEQUENCEINDEX>(itemData >> SEQU_SHIFT);
				if(ord == ORDERINDEX_INVALID)
					return ModItem(MODITEM_SEQUENCE, seq);
				else
					return ModItem(MODITEM_ORDER, ord, seq);
			}

			ModItem modItem(MODITEM_NULL, itemData);
			if(hItemParent == info->hPatterns)
			{
				// Pattern
				modItem.type = MODITEM_PATTERN;
			} else if(hItemParent == info->hSamples)
			{
				// Sample
				modItem.type = MODITEM_SAMPLE;
			} else if(hItemParent == info->hInstruments)
			{
				// Instrument
				modItem.type = MODITEM_INSTRUMENT;
			} else if(hItemParent == info->hEffects)
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
			int image = 0, selImage = 0;
			const bool isFolder = GetItemImage(hItem, image, selImage) && (image == IMAGE_FOLDER || image == IMAGE_OPENFOLDER);
			if(!isFolder || GetItemData(hItemParent) == DLS_DRUM_FOLDER_LPARAM)
				return DlsItem::FromLPARAM(itemData);
		}
	}
	return ModItem(MODITEM_NULL);
}


bool CModTree::ExecuteItem(HTREEITEM hItem)
{
	if(hItem)
	{
		const ModItem modItem = GetModItem(hItem);
		uint32 modItemID = modItem.val1;
		CModDoc *modDoc = m_docInfo.count(m_selectedDoc) ? m_selectedDoc : nullptr;

		switch(modItem.type)
		{
		case MODITEM_COMMENTS:
			if(modDoc)
				modDoc->ActivateView(IDD_CONTROL_COMMENTS, 0);
			return true;

		case MODITEM_SEQUENCE:
			if(modDoc
				&& modItemID < modDoc->GetSoundFile().Order.GetNumSequences()
				&& !modDoc->GetSoundFile().Order(static_cast<SEQUENCEINDEX>(modItemID)).GetLengthTailTrimmed())
				modDoc->ActivateView(IDD_CONTROL_PATTERNS, (modItemID << SEQU_SHIFT) | SEQU_INDICATOR);
			return true;

		case MODITEM_ORDER:
			if(modDoc)
				modDoc->ActivateView(IDD_CONTROL_PATTERNS, modItemID | (uint32(modItem.val2) << SEQU_SHIFT) | SEQU_INDICATOR);
			return true;

		case MODITEM_PATTERN:
			if(modDoc)
				modDoc->ActivateView(IDD_CONTROL_PATTERNS, modItemID);
			return true;

		case MODITEM_SAMPLE:
			if(modDoc)
				modDoc->ActivateView(IDD_CONTROL_SAMPLES, modItemID);
			return true;

		case MODITEM_INSTRUMENT:
			if(modDoc)
				modDoc->ActivateView(IDD_CONTROL_INSTRUMENTS, modItemID);
			return true;

		case MODITEM_MIDIPERCUSSION:
			modItemID |= 0x80;
			[[fallthrough]];
		case MODITEM_MIDIINSTRUMENT:
			OpenMidiInstrument(modItemID);
			return true;

		case MODITEM_EFFECT:
		case MODITEM_INSLIB_SAMPLE:
		case MODITEM_INSLIB_INSTRUMENT:
			PlayItem(hItem, NOTE_MIDDLEC);
			return true;

		case MODITEM_INSLIB_SONG:
		case MODITEM_INSLIB_FOLDER:
			// If it's a shell link, resolve the link target
			if(auto file = LinkResolver().Resolve(m_InstrLibPath.ToCString() + GetItemText(hItem)); !file.empty())
			{
				SetFullInstrumentLibraryPath(file);
			} else
			{
				InstrumentLibraryChDir(mpt::PathString::FromCString(GetItemText(hItem)), modItem.type == MODITEM_INSLIB_SONG);
			}
			return true;

		case MODITEM_HDR_SONG:
			if(modDoc)
				modDoc->ActivateWindow();
			return true;

		case MODITEM_DLSBANK_INSTRUMENT:
			if(GetItemData(GetParentItem(hItem)) != DLS_DRUM_FOLDER_LPARAM)
				PlayItem(hItem, NOTE_MIDDLEC);
			return true;

		case MODITEM_HDR_INSTRUMENTLIB:
			if(IsSampleBrowser())
			{
				BrowseForFolder dlg(m_InstrLibPath, _T("Select a new instrument library folder..."));
				if(dlg.Show())
				{
					SetFullInstrumentLibraryPath(dlg.GetDirectory());
				}
				return true;
			}
			break;
		}
	}
	return false;
}


void CModTree::PlayDLSItem(const CDLSBank &dlsBank, const DlsItem &item, ModCommand::NOTE note)
{
	UINT rgn, instr = item.GetInstr();
	if(item.IsPercussion())
		rgn = item.GetRegion();
	else
		rgn = dlsBank.GetRegionFromKey(instr, note - NOTE_MIN);
	CMainFrame::GetMainFrame()->PlayDLSInstrument(dlsBank, instr, rgn, note);
}


BOOL CModTree::PlayItem(HTREEITEM hItem, ModCommand::NOTE note, int volume)
{
	if(hItem)
	{
		const ModItem modItem = GetModItem(hItem);
		uint32 modItemID = modItem.val1;
		CModDoc *modDoc = m_docInfo.count(m_selectedDoc) ? m_selectedDoc : nullptr;

		switch(modItem.type)
		{
		case MODITEM_SAMPLE:
			if(modDoc)
			{
				if(note == NOTE_NOTECUT)
				{
					modDoc->NoteOff(0, true);  // cut previous playing samples
				} else if(note & 0x80)
				{
					modDoc->NoteOff(note & 0x7F, true);
				} else
				{
					modDoc->NoteOff(0, true);  // cut previous playing samples
					modDoc->PlayNote(PlayNoteParam(note & 0x7F).Sample(static_cast<SAMPLEINDEX>(modItemID)).Volume(volume));
				}
			}
			return TRUE;

		case MODITEM_INSTRUMENT:
			if(modDoc)
			{
				if(note == NOTE_NOTECUT)
				{
					modDoc->NoteOff(0, true);
				} else if(note & 0x80)
				{
					modDoc->NoteOff(note & 0x7F, true);
				} else
				{
					modDoc->NoteOff(0, true);
					modDoc->PlayNote(PlayNoteParam(note & 0x7F).Instrument(static_cast<INSTRUMENTINDEX>(modItemID)).Volume(volume));
				}
			}
			return TRUE;

		case MODITEM_EFFECT:
			if((modDoc) && (modItemID < MAX_MIXPLUGINS))
			{
				modDoc->TogglePluginEditor(modItemID);
			}
			return TRUE;

		case MODITEM_INSLIB_SAMPLE:
		case MODITEM_INSLIB_INSTRUMENT:
			if(note != NOTE_NOTECUT)
			{
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				if(!m_SongFileName.empty())
				{
					// Preview sample / instrument in module
					const size_t n = ConvertStrTo<size_t>(GetItemText(hItem));
					if(pMainFrm && m_SongFile)
					{
						if(modItem.type == MODITEM_INSLIB_INSTRUMENT)
						{
							pMainFrm->PlaySoundFile(*m_SongFile, static_cast<INSTRUMENTINDEX>(n), SAMPLEINDEX_INVALID, note, volume);
						} else
						{
							pMainFrm->PlaySoundFile(*m_SongFile, INSTRUMENTINDEX_INVALID, static_cast<SAMPLEINDEX>(n), note, volume);
						}
					}
				} else
				{
					// Preview sample / instrument file
					auto file = InsLibGetFullPath(hItem);
					// If it's a shell link, resolve the link target
					if(auto resolvedName = LinkResolver().Resolve(file.AsNative().c_str()); !resolvedName.empty())
						file = std::move(resolvedName);

					if(pMainFrm)
						pMainFrm->PlaySoundFile(file, note, volume);
				}
			} else
			{
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				if(pMainFrm)
					pMainFrm->StopPreview();
			}

			return TRUE;

		case MODITEM_MIDIPERCUSSION:
			modItemID |= 0x80;
			[[fallthrough]];
		case MODITEM_MIDIINSTRUMENT:
			{
				const MidiLibrary &midiLib = CTrackApp::GetMidiLibrary();
				if(modItemID < midiLib.size() && !midiLib[modItemID].empty())
				{
					CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
					CDLSBank *dlsBank = nullptr;
					if(!mpt::PathCompareNoCase(m_cachedBankName, midiLib[modItemID]))
					{
						dlsBank = m_cachedBank.get();
					}
					if(dlsBank == nullptr && CDLSBank::IsDLSBank(midiLib[modItemID]))
					{
						m_cachedBank = std::make_unique<CDLSBank>();
						if(m_cachedBank->Open(midiLib[modItemID]))
						{
							m_cachedBankName = midiLib[modItemID];
							dlsBank = m_cachedBank.get();
						}
					}
					if(dlsBank != nullptr)
					{
						uint32 item = 0;
						if(modItemID < 0x80)
						{
							if(dlsBank->FindInstrument(false, 0xFFFF, modItemID, note - NOTE_MIN, &item))
								PlayDLSItem(*dlsBank, DlsItem(static_cast<uint16>(item)), note);
						} else
						{
							if(dlsBank->FindInstrument(true, 0xFFFF, 0xFF, modItemID & 0x7F, &item))
								PlayDLSItem(*dlsBank, DlsItem(static_cast<uint16>(item), static_cast<uint16>(dlsBank->GetRegionFromKey(item, modItemID & 0x7F))), note);
						}
					} else
					{
						pMainFrm->PlaySoundFile(midiLib[modItemID], note, volume);
					}
				}
			}
			return TRUE;

		case MODITEM_DLSBANK_INSTRUMENT:
			{
				const DlsItem &item = static_cast<const DlsItem &>(modItem);
				CDLSBank *dlsBank = GetDLSBankFromItem(hItem);
				if(dlsBank != nullptr)
				{
					PlayDLSItem(*dlsBank, item, note);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}


BOOL CModTree::SetMidiInstrument(UINT nIns, const mpt::PathString &fileName)
{
	MidiLibrary &midiLib = CTrackApp::GetMidiLibrary();
	if(nIns < 128)
	{
		midiLib[nIns] = fileName;
		RefreshMidiLibrary();
		return TRUE;
	}
	return FALSE;
}


BOOL CModTree::SetMidiPercussion(UINT nPerc, const mpt::PathString &fileName)
{
	MidiLibrary &midiLib = CTrackApp::GetMidiLibrary();
	if(nPerc < 128)
	{
		UINT nIns = nPerc | 0x80;
		midiLib[nIns] = fileName;
		RefreshMidiLibrary();
		return TRUE;
	}
	return FALSE;
}


static mpt::ustring TreeDeletionString(const mpt::uchar *type, uint32 id, const mpt::ustring &name)
{
	mpt::ustring s = MPT_UFORMAT("Remove {} {}")(mpt::ustring(type), id);
	if(!name.empty())
		s += U_(": ") + name;
	s.append(1, UC_('?'));
	return s;
}


void CModTree::DeleteTreeItem(HTREEITEM hItem, const bool permanently)
{
	const ModItem modItem = GetModItem(hItem);
	uint32 modItemID = modItem.val1;

	CModDoc *modDoc = m_docInfo.count(m_selectedDoc) ? m_selectedDoc : nullptr;
	CSoundFile *sndFile = modDoc ? &modDoc->GetSoundFile() : nullptr;
	if(modItem.IsSongItem() && modDoc == nullptr)
	{
		return;
	}

	switch(modItem.type)
	{
	case MODITEM_SEQUENCE:
		if(sndFile)
		{
			const SEQUENCEINDEX seq = static_cast<SEQUENCEINDEX>(modItemID);
			if(Reporting::Confirm(TreeDeletionString(UL_("sequence"), seq + 1, sndFile->Order(seq).GetName()), false, true) == cnfNo) break;
			sndFile->Order.RemoveSequence(seq);
			modDoc->UpdateAllViews(nullptr, SequenceHint().Data());
		}
		break;

	case MODITEM_ORDER:
		// might be slightly annoying to ask for confirmation here, and it's rather easy to restore the orderlist anyway.
		if(modDoc && modDoc->RemoveOrder(static_cast<SEQUENCEINDEX>(modItem.val2), static_cast<ORDERINDEX>(modItem.val1)))
		{
			modDoc->UpdateAllViews(nullptr, SequenceHint().Data());
		}
		break;

	case MODITEM_PATTERN:
		if(modDoc && sndFile)
		{
			const PATTERNINDEX pat = static_cast<PATTERNINDEX>(modItemID);
			bool isUsed = false;
			// First, find all used patterns in all sequences.
			for(const auto &sequence : sndFile->Order)
			{
				if(sequence.FindOrder(pat) != ORDERINDEX_INVALID)
				{
					isUsed = true;
					break;
				}
			}
			mpt::ustring s = TreeDeletionString(UL_("pattern"), modItemID, mpt::ToUnicode(sndFile->GetCharsetInternal(), sndFile->Patterns[pat].GetName()));
			s += MPT_UFORMAT("\nThis pattern is currently {}used.")(isUsed ? U_("") : U_("un"));
			if(Reporting::Confirm(s, false, isUsed) == cnfYes && modDoc->RemovePattern(pat))
			{
				modDoc->UpdateAllViews(nullptr, PatternHint(pat).Data().Names());
				if(isUsed)
					modDoc->UpdateAllViews(nullptr, SequenceHint().Data());  // Pattern color will change in sequence
			}
		}
		break;

	case MODITEM_SAMPLE:
		if(modDoc && sndFile)
		{
			if(!sndFile->GetSample(static_cast<SAMPLEINDEX>(modItemID)).HasSampleData()
				 || Reporting::Confirm(TreeDeletionString(UL_("sample"), modItemID, mpt::ToUnicode(sndFile->GetCharsetInternal(), sndFile->m_szNames[modItemID])), false, true) == cnfYes)
			{
				const SAMPLEINDEX smp = static_cast<SAMPLEINDEX>(modItemID);
				modDoc->GetSampleUndo().PrepareUndo(smp, sundo_replace, "Delete");
				const SAMPLEINDEX oldNumSamples = modDoc->GetNumSamples();
				if(modDoc->RemoveSample(smp))
				{
					modDoc->UpdateAllViews(nullptr, SampleHint(modDoc->GetNumSamples() != oldNumSamples ? 0 : smp).Info().Data().Names());
				}
			}
		}
		break;

	case MODITEM_INSTRUMENT:
		if(modDoc && sndFile)
		{
			if(sndFile->Instruments[modItemID] == nullptr
				 || Reporting::Confirm(TreeDeletionString(UL_("instrument"), modItemID, mpt::ToUnicode(sndFile->GetCharsetInternal(), sndFile->Instruments[modItemID]->name)), false, true) == cnfYes)
			{
				const INSTRUMENTINDEX ins = static_cast<INSTRUMENTINDEX>(modItemID);
				modDoc->GetInstrumentUndo().PrepareUndo(ins, "Delete");
				const INSTRUMENTINDEX oldNumInstrs = modDoc->GetNumInstruments();
				if(modDoc->RemoveInstrument(ins))
				{
					modDoc->UpdateAllViews(nullptr, InstrumentHint(modDoc->GetNumInstruments() != oldNumInstrs ? 0 : ins).Info().Envelope().ModType());
				}
			}
		}
		break;

	case MODITEM_EFFECT:
		if(modDoc && Reporting::Confirm(TreeDeletionString(UL_("plugin FX"), modItemID + 1, sndFile->m_MixPlugins[modItemID].GetName()), false, true) == cnfYes)
		{
			modDoc->RemovePlugin(static_cast<PLUGINDEX>(modItemID));
		}
		break;

	case MODITEM_MIDIINSTRUMENT:
		SetMidiInstrument(modItemID, P_(""));
		RefreshMidiLibrary();
		break;
	case MODITEM_MIDIPERCUSSION:
		SetMidiPercussion(modItemID, P_(""));
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
			const mpt::winstring fullPath = InsLibGetFullPath(hItem).AsNative() + _T('\0');
			SHFILEOPSTRUCT fos;
			MemsetZero(fos);
			fos.hwnd = m_hWnd;
			fos.wFunc = FO_DELETE;
			fos.pFrom = fullPath.c_str();
			fos.fFlags = permanently ? 0 : FOF_ALLOWUNDO;
			if(!SHFileOperation(&fos) && !fos.fAnyOperationsAborted)
			{
				HTREEITEM newSel = GetNextSiblingItem(hItem);
				if(!newSel) newSel = GetPrevSiblingItem(hItem);
				SelectItem(newSel);
				RefreshInstrumentLibrary();
				SetFocus();
			}
		}
		break;
	}
}


BOOL CModTree::OpenTreeItem(HTREEITEM hItem)
{
	const ModItem modItem = GetModItem(hItem);

	switch(modItem.type)
	{
	case MODITEM_INSLIB_SONG:
		theApp.OpenDocumentFile(InsLibGetFullPath(hItem).ToCString());
		break;
	case MODITEM_HDR_INSTRUMENTLIB:
		CTrackApp::OpenDirectory(m_InstrLibPath);
		break;
	case MODITEM_INSLIB_FOLDER:
		// Open path in Explorer
		CTrackApp::OpenDirectory(InsLibGetFullPath(hItem));
		break;
	}
	return TRUE;
}


BOOL CModTree::OpenMidiInstrument(DWORD dwItem)
{
	std::vector<FileType> mediaFoundationTypes = CSoundFile::GetMediaFoundationFileTypes();
	FileDialog dlg = OpenFileDialog()
		.EnableAudioPreview()
		.ExtensionFilter(
			"All Instruments and Banks (*.xi,*.pat,*.iti,*.sfz,*.dls,*.sf2,...)|*.xi;*.pat;*.iti;*.sfz;*.wav;*.w64;*.caf;*.aif;*.aiff;*.sbk;*.sf2;*.sf3;*.sf4;*.dls;*.mss;*.flac;*.opus;*.ogg;*.oga;*.mp1;*.mp2;*.mp3" + ToFilterOnlyString(mediaFoundationTypes, true).ToLocale() + "|"
			"FastTracker II Instruments (*.xi)|*.xi|"
			"GF1 Patches (*.pat)|*.pat|"
			"Wave Files (*.wav)|*.wav|"
			"Wave64 Files (*.w64)|*.w64|"
			"CAF Files (*.caf)|*.caf|"
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
			"SFZ Instruments (*.sfz)|*.sfz|"
			"SoundFont 2.0 Banks (*.sf2)|*.sbk;*.sf2;*.sf3;*.sf4|"
			"DLS Sound Banks (*.dls;*.mss)|*.dls;*.mss|"
			"All Files (*.*)|*.*||");
	if(!dlg.Show()) return FALSE;

	if(dwItem & 0x80)
		return SetMidiPercussion(dwItem & 0x7F, dlg.GetFirstFile());
	else
		return SetMidiInstrument(dwItem, dlg.GetFirstFile());
}


// Refresh Instrument Library
void CModTree::FillInstrumentLibrary(const TCHAR *selectedItem)
{
	if(!m_hInsLib && !IsSampleBrowser())
		return;

	LockRedraw();

	if(!IsSampleBrowser())
	{
		DeleteChildren(m_hInsLib);
	} else
	{
		DeleteItem(TVI_ROOT);
		m_hInsLib = nullptr;
	}

	m_fileBrowserEntries.clear();

	if(!m_SongFileName.empty() && IsSampleBrowser() && m_SongFile)
	{
		// Fill browser with samples / instruments of module file
		for(INSTRUMENTINDEX ins = 1; ins <= m_SongFile->GetNumInstruments(); ins++)
		{
			ModInstrument *pIns = m_SongFile->Instruments[ins];
			if(pIns)
			{
				TCHAR s[MAX_INSTRUMENTNAME + 10];
				_sntprintf(s, std::size(s), _T("%3d: %s"), ins, mpt::ToWin(m_SongFile->GetCharsetInternal(), pIns->name).c_str());
				m_fileBrowserEntries.push_back({s, 0, 0, IMAGE_INSTRUMENTS, false});
			}
		}
		for(SAMPLEINDEX smp = 1; smp <= m_SongFile->GetNumSamples(); smp++)
		{
			const ModSample &sample = m_SongFile->GetSample(smp);
			if(sample.HasSampleData() || sample.uFlags[CHN_ADLIB])
			{
				TCHAR s[MAX_SAMPLENAME + 10];
				_sntprintf(s, std::size(s), _T("%3d: %s"), smp, mpt::ToWin(m_SongFile->GetCharsetInternal(), m_SongFile->m_szNames[smp]).c_str());
				m_fileBrowserEntries.push_back({s, sample.GetSampleSizeInBytes(), 0, static_cast<uint32>(sample.uFlags[CHN_ADLIB] ? IMAGE_OPLINSTR : IMAGE_SAMPLES), false});
			}
		}
	} else if(!m_InstrLibPath.empty())
	{
		if(!IsSampleBrowser())
		{
			SetItemText(m_hInsLib, _T("Instrument Library (") + m_InstrLibPath.ToCString() + _T(")"));
		}

		// Enumerating Drives...
		if(!IsSampleBrowser())
		{
			CImageList &images = CMainFrame::GetMainFrame()->m_MiscIcons;
			// Avoid adding the same images again and again...
			images.SetImageCount(IMGLIST_NUMIMAGES);

			TCHAR s[] = _T("?:\\");
			for(int drive = 'A'; drive <= 'Z'; drive++)
			{
				s[0] = static_cast<TCHAR>(drive);
				auto driveType = GetDriveType(s);
				if(driveType != DRIVE_UNKNOWN && driveType != DRIVE_NO_ROOT_DIR)
				{
					if(driveType == DRIVE_REMOVABLE)
					{
						TCHAR sDevice[] = _T("\\\\.\\?:");
						sDevice[4] = s[0];
						auto hDevice = CreateFile(sDevice, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
						if(hDevice != INVALID_HANDLE_VALUE)
						{
							// Check if removable media is inserted
							DWORD bytesReturned = 0;
							auto success = DeviceIoControl(hDevice, IOCTL_STORAGE_CHECK_VERIFY2, nullptr, 0, nullptr, 0, &bytesReturned, nullptr);
							CloseHandle(hDevice);
							if(!success && GetLastError() == ERROR_NOT_READY)
								continue;
						}
					}
					SHFILEINFO fileInfo;
					SHGetFileInfo(s, 0, &fileInfo, sizeof(fileInfo), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);  // SHGFI_USEFILEATTRIBUTES speeds up retrieval for offline network shares
					const int imageIndex = fileInfo.hIcon ? images.Add(fileInfo.hIcon) : IMAGE_FOLDER;
					m_fileBrowserEntries.push_back({s, 0, 0, static_cast<uint32>(imageIndex < 0 ? IMAGE_FOLDER : imageIndex), false});
					DestroyIcon(fileInfo.hIcon);
				}
			}
		}

		// Enumerating Directories and samples/instruments
		const mpt::PathString path = m_InstrLibPath + P_("*.*");
		const bool showDirs = !IsSampleBrowser() || TrackerSettings::Instance().showDirsInSampleBrowser;
		const bool showInstrs = IsSampleBrowser();

		enum
		{
			FILTER_FIRST_VALID = 0,
			FILTER_REJECT_FILE = -1,
			FILTER_UNKNOWN_FILE = -2,
		};

		const auto FilterFile = [this, showInstrs, showDirs](const mpt::PathString &fileName) -> int
		{

			static constexpr auto instrExts = {"xi", "iti", "sfz", "sf2", "sf3", "sf4", "sbk", "dls", "mss", "pat"};
			static constexpr auto sampleExts = {"wav", "flac", "ogg", "opus", "mp1", "mp2", "mp3", "smp", "raw", "s3i", "its", "aif", "aiff", "au", "snd", "svx", "voc", "8sv", "8svx", "16sv", "16svx", "w64", "caf", "sb0", "sb2", "sbi", "brr"};
			static constexpr auto allExtsBlacklist = {"txt", "diz", "nfo", "doc", "ini", "pdf", "zip", "rar", "lha", "exe", "dll", "lnk", "url"};

			// Get lower-case file extension without dot.
			mpt::PathString extPS = fileName.GetFilenameExtension();
			std::string ext = extPS.ToUTF8();
			if(!ext.empty())
			{
				ext.erase(0, 1);
				ext = mpt::ToLowerCaseAscii(ext);
				extPS = mpt::PathString::FromUTF8(ext);
			}

			bool knownExtension = true;
			if(mpt::contains(instrExts, ext))
			{
				if(showInstrs)
					return IMAGE_INSTRUMENTS;
			} else if(mpt::contains(sampleExts, ext))
			{
				if(showInstrs)
					return IMAGE_SAMPLES;
			} else if(mpt::contains(m_modExtensions, ext))
			{
				if(showDirs || m_showAllFiles)
					return IMAGE_FOLDERSONG;
			} else if(!extPS.empty() && mpt::contains(m_MediaFoundationExtensions, extPS))
			{
				if(showInstrs)
					return IMAGE_SAMPLES;
			} else
			{
				if(showDirs)
				{
					// Amiga-style prefix (i.e. mod.songname)
					std::string prefixExt = fileName.ToUTF8();
					const auto dotPos = prefixExt.find('.');
					if(dotPos != std::string::npos && mpt::contains(m_modExtensions, prefixExt.erase(dotPos)))
						return IMAGE_FOLDERSONG;
				}
				knownExtension = false;
			}

			if(m_showAllFiles && !mpt::contains(allExtsBlacklist, ext))
				return IMAGE_SAMPLES;
				
			return knownExtension ? FILTER_REJECT_FILE : FILTER_UNKNOWN_FILE;
		};

		HKEY hkey = nullptr;
		bool showHidden = false;
		if(auto cr = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), 0, KEY_READ, &hkey); cr == ERROR_SUCCESS)
		{
			DWORD dataType = REG_NONE;
			DWORD data = 0, datasize = sizeof(data);
			if(RegQueryValueEx(hkey, _T("Hidden"), 0, &dataType, reinterpret_cast<BYTE *>(&data), &datasize) == ERROR_SUCCESS && dataType == REG_DWORD)
				showHidden = (data == 1);
			RegCloseKey(hkey);
		}

		LinkResolver linkResolver;
		HANDLE hFind;
		WIN32_FIND_DATA wfd;
		MemsetZero(wfd);
		if((hFind = FindFirstFile(path.AsNative().c_str(), &wfd)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				// Up Directory
				int type = FILTER_REJECT_FILE;
				if(!_tcscmp(wfd.cFileName, _T("..")))
				{
					if(showDirs)
						type = IMAGE_FOLDERPARENT;
				} else if(wfd.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE)
				{
					// Ignore unavailable files
					continue;
				} else if((wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
				          && ((wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) || !showHidden))
				{
					// Only show hidden files if Explorer is configured to do so (and never show hidden system files)
					continue;
				} else if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// Directory
					if(_tcscmp(wfd.cFileName, _T(".")) && showDirs)
						type = IMAGE_FOLDER;
				} else if(wfd.nFileSizeHigh > 0 || wfd.nFileSizeLow >= 9)
				{
					type = FilterFile(mpt::PathString::FromNative(wfd.cFileName));
					if(type == FILTER_UNKNOWN_FILE)
					{
						// Try resolving file as link if it wasn't a module or instrument
						const auto nativePath = (m_InstrLibPath.AsNative() + wfd.cFileName);
						if(const auto resolvedName = linkResolver.Resolve(nativePath.c_str()); !resolvedName.empty())
						{
							if(mpt::native_fs{}.is_directory(resolvedName) && showDirs)
								type = IMAGE_FOLDER;
							else if(mpt::native_fs{}.is_file(resolvedName))
								type = FilterFile(resolvedName);
						}
					}
				}
				if(type >= FILTER_FIRST_VALID)
				{
					m_fileBrowserEntries.push_back({wfd.cFileName,
						wfd.nFileSizeLow | (static_cast<uint64>(wfd.nFileSizeHigh) << 32),
						wfd.ftLastWriteTime.dwLowDateTime | (static_cast<uint64>(wfd.ftLastWriteTime.dwHighDateTime) << 32),
						static_cast<uint32>(type),
						(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0});
				}
			} while(FindNextFile(hFind, &wfd));
			FindClose(hFind);
		}
	}

	SortInstrumentLibrary();
	FilterInstrumentLibrary({}, selectedItem);

	UnlockRedraw();

	{
		mpt::lock_guard<mpt::mutex> l(m_WatchDirMutex);
		if(m_InstrLibPath != m_WatchDir)
		{
			m_WatchDir = m_InstrLibPath;
			SetEvent(m_hSwitchWatchDir);
		}
	}
}


void CModTree::SortInstrumentLibrary()
{
	std::sort(m_fileBrowserEntries.begin(), m_fileBrowserEntries.end(), [this](const FileBrowserEntry &left, const FileBrowserEntry &right)
	{
		const int sortL = ImageToSortOrder(left.image), sortR = ImageToSortOrder(right.image);
		if (sortL != sortR)
			return sortL < sortR;

		if (m_librarySort == LibrarySortOrder::Date && left.modtime != right.modtime)
			return left.modtime > right.modtime;
		else if (m_librarySort == LibrarySortOrder::Size && left.size != right.size)
			return left.size > right.size;

		return ::CompareString(LOCALE_USER_DEFAULT, m_stringCompareFlags, left.name.c_str(), -1, right.name.c_str(), -1) == CSTR_LESS_THAN;
	});
}


void CModTree::FilterInstrumentLibrary(mpt::winstring filter, const TCHAR *selectedItem)
{
	LockRedraw();

	if(!filter.empty())
		filter = _T("*") + filter + _T("*");

	HTREEITEM item = GetNextItem(m_hInsLib, IsSampleBrowser() ? TVGN_NEXT : TVGN_CHILD);
	while(item != nullptr)
	{
		HTREEITEM nextItem = GetNextSiblingItem(item);
		if(nextItem == nullptr)
			break;
		item = nextItem;
	}

	// TODO: Maybe first delete front-to-back, then insert back-to-front?

	// Insert items in reverse, as insertion via TVI_FIRST is faster than TVI_LAST as per https://devblogs.microsoft.com/oldnewthing/20111125-00/?p=9033
	DWORD_PTR entryID = m_fileBrowserEntries.size();
	HTREEITEM selectedTreeItem = nullptr;
	for(auto entry = m_fileBrowserEntries.crbegin(); entry != m_fileBrowserEntries.crend(); entry++, entryID--)
	{
		const bool add = filter.empty() || PathMatchSpec(entry->name.c_str(), filter.c_str()) != FALSE;

		if(add)
		{
			while(item && GetItemData(item) > entryID)
			{
				item = GetPrevSiblingItem(item);
			}

			if(!item || GetItemData(item) != entryID)
			{
				int state = entry->hidden ? TVIS_CUT : 0;
				item = InsertItem(TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_STATE | TVIF_PARAM, LPSTR_TEXTCALLBACK, entry->image, entry->image, state, state, entryID, IsSampleBrowser() ? TVI_ROOT : m_hInsLib, item ? item : TVI_FIRST);
			}

			if(selectedItem != nullptr && !_tcscmp(entry->name.c_str(), selectedItem))
			{
				selectedTreeItem = item;
			}

			item = GetPrevSiblingItem(item);
		} else
		{
			while(item && GetItemData(item) >= entryID)
			{
				HTREEITEM prevItem = GetPrevSiblingItem(item);
				DeleteItem(item);
				if(item == m_hInsLib)
					m_hInsLib = nullptr;
				item = prevItem;
			}
		}
	}

	if(IsSampleBrowser())
	{
		if(m_hInsLib)
			DeleteItem(m_hInsLib);
		if(!m_SongFileName.empty() && m_SongFile)
			m_hInsLib = InsertItem(m_SongFileName.ToCString(), IMAGE_FOLDERSONG, IMAGE_FOLDERSONG, TVI_ROOT, TVI_FIRST);
		else
			m_hInsLib = InsertItem(m_InstrLibPath.ToCString(), IMAGE_FOLDER, IMAGE_FOLDER, TVI_ROOT, TVI_FIRST);
	}

	if(!filter.empty() && !selectedTreeItem)
	{
		if(IsSampleBrowser())
		{
			selectedTreeItem = GetFirstVisibleItem();
			if(selectedTreeItem == m_hInsLib)
				selectedTreeItem = GetNextItem(m_hInsLib, TVGN_NEXT);
		} else
		{
			selectedTreeItem = GetChildItem(m_hInsLib);
		}
	}

	UnlockRedraw();

	if(selectedTreeItem)
	{
		SelectItem(selectedTreeItem);
		EnsureVisible(selectedTreeItem);
	}
}


void CModTree::SetInstrumentLibraryFilter(const mpt::winstring &filter)
{
	if(m_filterString == filter)
		return;

	m_filterString = filter;
	LockRedraw();
	FilterInstrumentLibrary(m_filterString, GetItemText(GetSelectedItem()));
	if(!IsSampleBrowser())
		RefreshDlsBanks(true);
	UnlockRedraw();
}


void CModTree::SetInstrumentLibraryFilterSortOrder(LibrarySortOrder sortType)
{
	if(m_librarySort == sortType)
		return;

	m_librarySort = sortType;
	SortInstrumentLibrary();
	FilterInstrumentLibrary(m_filterString, GetItemText(GetSelectedItem()));
	GetOtherView()->SortInstrumentLibrary();
	GetOtherView()->FilterInstrumentLibrary(GetOtherView()->m_filterString, GetItemText(GetSelectedItem()));
}


void CModTree::OnGetDispInfo(LPNMHDR pnmhdr, LRESULT *)
{
	NMTVDISPINFO *nmtv = reinterpret_cast<NMTVDISPINFO *>(pnmhdr);
	size_t index = nmtv->item.lParam;
	if(index > 0 && index <= m_fileBrowserEntries.size())
		nmtv->item.pszText = const_cast<TCHAR *>(m_fileBrowserEntries[index - 1].name.c_str());
	else
		nmtv->item.pszText = const_cast<TCHAR*>(_T("???"));
}


int CModTree::ImageToSortOrder(int image) const
{
	// Item image indicates sort order
	switch(image)
	{
		case IMAGE_FOLDERPARENT:
			return 1;
		case IMAGE_FOLDER:
			return 2;
		case IMAGE_FOLDERSONG:
			return 3;
		case IMAGE_SAMPLES:
		case IMAGE_OPLINSTR:
			// Only group instruments and samples separately if we're browsing inside a module file
			if(!m_SongFileName.empty())
				return 5;
			[[fallthrough]];
		case IMAGE_INSTRUMENTS:
		default:
			return 4;
	}
}


// Monitor changes in the instrument library folder.
void CModTree::MonitorInstrumentLibrary()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
	mpt::log::Trace::SetThreadId(mpt::log::Trace::ThreadKindWatchdir, GetCurrentThreadId());
	DWORD result;
	mpt::PathString lastWatchDir;
	HANDLE hWatchDir = INVALID_HANDLE_VALUE;
	DWORD64 lastRefresh = Util::GetTickCount64();
	DWORD timeout = INFINITE;
	DWORD interval = TrackerSettings::Instance().FSUpdateInterval;
	do
	{
		{
			mpt::lock_guard<mpt::mutex> l(m_WatchDirMutex);
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
					hWatchDir = FindFirstChangeNotification(m_WatchDir.AsNative().c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
					lastWatchDir = m_WatchDir;
				}
			}
		}
		const HANDLE waitHandles[3] = {m_hWatchDirKillThread, m_hSwitchWatchDir, hWatchDir};
		result = WaitForMultipleObjects(hWatchDir != INVALID_HANDLE_VALUE ? 3 : 2, waitHandles, FALSE, timeout);
		DWORD64 now = Util::GetTickCount64();
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
			timeout = 0;  // update timeout later
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
				timeout = interval - static_cast<DWORD>(now - lastRefresh);
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


void CModTree::SetFullInstrumentLibraryPath(mpt::PathString path)
{
	if(mpt::native_fs{}.is_directory(path))
	{
		path = path.WithTrailingSlash();
		InstrumentLibraryChDir(path, false);
	} else if(mpt::native_fs{}.is_file(path))
	{
		// Browse module contents
		CModTree *dirBrowser = CMainFrame::GetMainFrame()->GetUpperTreeview();
		dirBrowser->m_InstrLibPath = path.GetDirectoryWithDrive();
		dirBrowser->RefreshInstrumentLibrary();
		dirBrowser->InstrumentLibraryChDir(path.GetFilename(), true);
	}
}


void CModTree::InstrumentLibraryChDir(mpt::PathString dir, bool isSong)
{
	if(dir.empty())
		return;

	if(IsSampleBrowser())
	{
		CMainFrame::GetMainFrame()->GetUpperTreeview()->InstrumentLibraryChDir(dir, isSong);
		return;
	}

	GetParent()->PostMessage(WM_COMMAND, ID_CLOSE_LIBRARY_FILTER);
	const bool updateDLSBanks = !m_filterString.empty();
	m_filterString.clear();
	m_pDataTree->m_filterString.clear();

	BeginWaitCursor();

	bool ok = false;
	const bool goUp = (dir == P_(".."));
	m_previousPath = {};
	if(isSong && !goUp)
	{
		ok = m_pDataTree->InsLibSetFullPath(m_InstrLibPath, dir);
		if(ok)
		{
			m_pDataTree->RefreshInstrumentLibrary();
			m_InstrLibHighlightPath = dir;
		}
	} else
	{
		if(goUp)
		{
			if(isSong)
			{
				// Leave song
				m_InstrLibHighlightPath = std::move(m_pDataTree->m_SongFileName);
				dir = m_InstrLibPath;
			} else
			{
				// Go one dir up.
				mpt::winstring prevDir = m_InstrLibPath.GetDirectoryWithDrive().AsNative();
				mpt::winstring::size_type pos = prevDir.find_last_of(_T("\\/"), prevDir.length() - 2);
				if(pos != mpt::winstring::npos)
				{
					m_InstrLibHighlightPath = mpt::PathString::FromNative(prevDir.substr(pos + 1, prevDir.length() - pos - 2));  // Highlight previously accessed directory
					prevDir = prevDir.substr(0, pos + 1);
				}
				m_previousPath = m_InstrLibHighlightPath;
				dir = mpt::PathString::FromNative(prevDir);
			}
		} else
		{
			// Drives are formatted like "E:\", folders are just folder name without slash.
			do
			{
				if(!dir.HasTrailingSlash())
				{
					dir = m_InstrLibPath + dir;
					dir = dir.WithTrailingSlash();
				}
				m_InstrLibHighlightPath = P_("..");  // Highlight first entry

				FolderScanner scan(dir, FolderScanner::kFilesAndDirectories);
				mpt::PathString name;
				if(scan.Next(name) && !scan.Next(name) && mpt::native_fs{}.is_directory(name))
				{
					// There is only one directory and nothing else in the path,
					// so skip this directory and automatically descend further down into the tree.
					dir = name;
					dir = dir.WithTrailingSlash();
					continue;
				}
			} while(false);
		}

		if(mpt::native_fs{}.is_directory(dir))
		{
			m_SongFileName = P_("");
			delete m_SongFile;
			m_SongFile = nullptr;
			m_InstrLibPath = dir;
			GetSampleBrowser()->m_InstrLibHighlightPath = m_InstrLibHighlightPath;
			PostMessage(WM_COMMAND, ID_MODTREE_REFRESHINSTRLIB);
			ok = true;
		}
	}
	if(updateDLSBanks)
		RefreshDlsBanks(true);

	EndWaitCursor();

	if(ok)
	{
		mpt::lock_guard<mpt::mutex> l(m_WatchDirMutex);
		m_WatchDir = mpt::PathString();
	} else
	{
		Reporting::Error(MPT_CFORMAT("Unable to browse to \"{}\"")(dir), _T("Instrument Library"));
	}
}


bool CModTree::GetDropInfo(DRAGONDROP &dropInfo, mpt::PathString &fullPath)
{
	const auto dragDoc = m_docInfo.find(m_dragDoc);
	dropInfo.sndFile = dragDoc != m_docInfo.end() ? &dragDoc->second.modDoc.GetSoundFile() : nullptr;
	dropInfo.dropType = DRAGONDROP_NOTHING;
	dropInfo.dropItem = m_itemDrag.val1;
	dropInfo.dropParam = 0;
	switch(m_itemDrag.type)
	{
	case MODITEM_ORDER:
		dropInfo.dropType = DRAGONDROP_ORDER;
		break;

	case MODITEM_PATTERN:
		dropInfo.dropType = DRAGONDROP_PATTERN;
		break;

	case MODITEM_SAMPLE:
		dropInfo.dropType = DRAGONDROP_SAMPLE;
		break;

	case MODITEM_INSTRUMENT:
		dropInfo.dropType = DRAGONDROP_INSTRUMENT;
		break;

	case MODITEM_SEQUENCE:
	case MODITEM_HDR_ORDERS:
		dropInfo.dropType = DRAGONDROP_SEQUENCE;
		break;

	case MODITEM_INSLIB_SAMPLE:
	case MODITEM_INSLIB_INSTRUMENT:
		if(!m_SongFileName.empty())
		{
			const uint32 n = ConvertStrTo<uint32>(GetItemText(m_hItemDrag));
			dropInfo.dropType = (m_itemDrag.type == MODITEM_INSLIB_SAMPLE) ? DRAGONDROP_SAMPLE : DRAGONDROP_INSTRUMENT;
			dropInfo.dropItem = n;
			dropInfo.sndFile = m_SongFile;
			dropInfo.dropParam = 0;
		} else
		{
			fullPath = InsLibGetFullPath(m_hItemDrag);
			dropInfo.dropType = DRAGONDROP_SOUNDFILE;
			dropInfo.dropParam = reinterpret_cast<uintptr_t>(&fullPath);
		}
		break;

	case MODITEM_MIDIPERCUSSION:
		dropInfo.dropItem |= 0x80;
		[[fallthrough]];
	case MODITEM_MIDIINSTRUMENT:
		{
			MidiLibrary &midiLib = CTrackApp::GetMidiLibrary();
			if(!midiLib[dropInfo.dropItem & 0xFF].empty())
			{
				fullPath = midiLib[dropInfo.dropItem & 0xFF];
				dropInfo.dropType = DRAGONDROP_MIDIINSTR;
				dropInfo.dropParam = reinterpret_cast<uintptr_t>(&fullPath);
			}
		}
		break;

	case MODITEM_INSLIB_SONG:
		fullPath = InsLibGetFullPath(m_hItemDrag);
		dropInfo.sndFile = nullptr;
		dropInfo.dropType = DRAGONDROP_SONG;
		dropInfo.dropItem = 0;
		dropInfo.dropParam = reinterpret_cast<uintptr_t>(&fullPath);
		break;

	case MODITEM_DLSBANK_INSTRUMENT:
		{
			dropInfo.dropType = DRAGONDROP_DLS;
			dropInfo.dropItem = static_cast<uint32>(GetDLSBankIndexFromItem(m_hItemDrag));  // bank #
			// Melodic: (Instrument)
			// Drums:   (0x80000000) | (Region << 16) | (Instrument)
			dropInfo.dropParam = m_itemDrag.val1;
		}
		break;
	}
	return (dropInfo.dropType != DRAGONDROP_NOTHING);
}


bool CModTree::CanDrop(HTREEITEM hItem, bool doDrop)
{
	const ModItem modItemDrop = GetModItem(hItem);
	const uint32 modItemDropID = modItemDrop.val1;
	const uint32 modItemDragID = m_itemDrag.val1;

	const auto dragIter = m_docInfo.find(m_dragDoc);
	const auto selIter = m_docInfo.find(m_selectedDoc);
	const ModTreeDocInfo *infoDrag = dragIter != m_docInfo.end() ? &dragIter->second : nullptr;
	const ModTreeDocInfo *infoDrop = selIter != m_docInfo.end() ? &selIter->second : nullptr;
	CModDoc *modDoc = infoDrop ? &infoDrop->modDoc : nullptr;
	CSoundFile *sndFile = modDoc ? &modDoc->GetSoundFile() : nullptr;
	const bool sameModDoc = infoDrag && (modDoc == &infoDrag->modDoc);
	const bool sameItem = modItemDrop == m_itemDrag && sameModDoc;

	switch(modItemDrop.type)
	{
	case MODITEM_ORDER:
	case MODITEM_SEQUENCE:
		if(m_itemDrag.type == MODITEM_ORDER && modDoc && sameModDoc)
		{
			// Drop an order somewhere
			if(doDrop)
			{
				SEQUENCEINDEX seqFrom = static_cast<SEQUENCEINDEX>(m_itemDrag.val2), seqTo = static_cast<SEQUENCEINDEX>(modItemDrop.val2);
				ORDERINDEX ordFrom = static_cast<ORDERINDEX>(m_itemDrag.val1), ordTo = static_cast<ORDERINDEX>(modItemDrop.val1);
				if(modItemDrop.type == MODITEM_SEQUENCE)
				{
					// Drop on sequence -> attach
					seqTo = static_cast<SEQUENCEINDEX>(modItemDrop.val1);
					ordTo = sndFile->Order(seqTo).GetLengthTailTrimmed();
				}

				if(seqFrom != seqTo || ordFrom != ordTo)
				{
					if(modDoc->MoveOrder(ordFrom, ordTo, true, false, seqFrom, seqTo) == true)
					{
						modDoc->SetModified();
					}
				}
			}
			return true;
		} else if(modItemDrop.type == MODITEM_SEQUENCE && m_itemDrag.type == MODITEM_SEQUENCE && modDoc && sameModDoc)
		{
			if(doDrop && !sameItem)
			{
				// Rearrange sequences
				const SEQUENCEINDEX from = static_cast<SEQUENCEINDEX>(modItemDragID), to = static_cast<SEQUENCEINDEX>(modItemDropID);

				std::vector<SEQUENCEINDEX> newOrder(sndFile->Order.GetNumSequences());
				std::iota(newOrder.begin(), newOrder.end(), SEQUENCEINDEX(0));
				newOrder.erase(newOrder.begin() + from);
				newOrder.insert(newOrder.begin() + to, from);

				modDoc->ReArrangeSequences(newOrder);

				auto curSeq = sndFile->Order.GetCurrentSequenceIndex();
				if(curSeq == from)
					curSeq = to;
				else if(from > curSeq && to <= curSeq)
					curSeq++;
				else if(from < curSeq && to >= curSeq)
					curSeq--;
				sndFile->Order.SetSequence(curSeq);

				modDoc->UpdateAllViews(nullptr, SequenceHint(SEQUENCEINDEX_INVALID).Names().Data());
				modDoc->SetModified();

				SelectItem(hItem);
			}
			return true;
		}
		break;

	case MODITEM_HDR_ORDERS:
		// Drop your sequences here.
		// At the moment, only dropping sequences into another module is possible and it doesn't copy the patterns themselves.
		if((m_itemDrag.type == MODITEM_SEQUENCE || m_itemDrag.type == MODITEM_HDR_ORDERS) && modDoc && sndFile && infoDrag && !sameModDoc)
		{
			if(doDrop && infoDrag != nullptr)
			{
				// copy mod sequence over.
				CSoundFile &dragSndFile = infoDrag->modDoc.GetSoundFile();
				const SEQUENCEINDEX origSeqId = static_cast<SEQUENCEINDEX>(modItemDragID);
				const ModSequence &origSeq = dragSndFile.Order(origSeqId);
				SEQUENCEINDEX sequenceHint = SEQUENCEINDEX_INVALID;

				if(sndFile->GetModSpecifications().sequencesMax > 1)
				{
					sequenceHint = sndFile->Order.AddSequence();
				} else
				{
					if(Reporting::Confirm(_T("Replace the current orderlist?"), _T("Sequence import")) == cnfNo)
						return false;
					sequenceHint = 0;
				}
				sndFile->Order().resize(std::min(sndFile->GetModSpecifications().ordersMax, origSeq.GetLength()), sndFile->Order.GetInvalidPatIndex());
				for(ORDERINDEX nOrd = 0; nOrd < std::min(sndFile->GetModSpecifications().ordersMax, origSeq.GetLengthTailTrimmed()); nOrd++)
				{
					PATTERNINDEX pat = dragSndFile.Order(origSeqId)[nOrd];
					// translate pattern index
					if(pat == dragSndFile.Order.GetInvalidPatIndex())
						pat = sndFile->Order.GetInvalidPatIndex();
					else if(pat == dragSndFile.Order.GetIgnoreIndex() && sndFile->GetModSpecifications().hasIgnoreIndex)
						pat = sndFile->Order.GetIgnoreIndex();
					else if(pat == dragSndFile.Order.GetIgnoreIndex() && !sndFile->GetModSpecifications().hasIgnoreIndex)
						pat = sndFile->Order.GetInvalidPatIndex();
					else if(pat >= sndFile->GetModSpecifications().patternsMax)
						pat = sndFile->Order.GetInvalidPatIndex();
					
					sndFile->Order()[nOrd] = pat;
				}
				modDoc->UpdateAllViews(nullptr, SequenceHint(sequenceHint).Data());
				modDoc->SetModified();
			}
			return true;
		}
		break;

	case MODITEM_SAMPLE:
		if(m_itemDrag.type == MODITEM_SAMPLE && modDoc && infoDrag != nullptr)
		{
			if(doDrop)
			{
				if(sameModDoc)
				{
					// Reorder samples in a module
					if(sameItem)
						return true;
					const SAMPLEINDEX from = static_cast<SAMPLEINDEX>(modItemDragID - 1), to = static_cast<SAMPLEINDEX>(modItemDropID - 1);

					std::vector<SAMPLEINDEX> newOrder(modDoc->GetNumSamples());
					std::iota(newOrder.begin(), newOrder.end(), SAMPLEINDEX(1));
					newOrder.erase(newOrder.begin() + from);
					newOrder.insert(newOrder.begin() + to, from + 1);

					modDoc->ReArrangeSamples(newOrder);
				} else
				{
					// Load sample into other module
					sndFile->ReadSampleFromSong(static_cast<SAMPLEINDEX>(modItemDropID), infoDrag->modDoc.GetSoundFile(), static_cast<SAMPLEINDEX>(modItemDragID));
				}
				modDoc->UpdateAllViews(nullptr, SampleHint().Info().Data().Names());
				modDoc->UpdateAllViews(nullptr, PatternHint().Data());
				modDoc->UpdateAllViews(nullptr, InstrumentHint().Info());
				modDoc->SetModified();
				SelectItem(hItem);
			}
			return true;
		}
		break;

	case MODITEM_INSTRUMENT:
		if(m_itemDrag.type == MODITEM_INSTRUMENT && modDoc && infoDrag != nullptr)
		{
			if(doDrop)
			{
				if(sameModDoc)
				{
					// Reorder instruments in a module
					if(sameItem)
						return true;
					const INSTRUMENTINDEX from = static_cast<INSTRUMENTINDEX>(modItemDragID - 1), to = static_cast<INSTRUMENTINDEX>(modItemDropID - 1);

					std::vector<INSTRUMENTINDEX> newOrder(modDoc->GetNumInstruments());
					std::iota(newOrder.begin(), newOrder.end(), INSTRUMENTINDEX(1));
					newOrder.erase(newOrder.begin() + from);
					newOrder.insert(newOrder.begin() + to, from + 1);

					modDoc->ReArrangeInstruments(newOrder);
				} else
				{
					// Load instrument into other module
					sndFile->ReadInstrumentFromSong(static_cast<INSTRUMENTINDEX>(modItemDropID), infoDrag->modDoc.GetSoundFile(), static_cast<INSTRUMENTINDEX>(modItemDragID));
				}
				modDoc->UpdateAllViews(nullptr, InstrumentHint().Info().Envelope().Names());
				modDoc->UpdateAllViews(nullptr, PatternHint().Data());
				modDoc->SetModified();
				SelectItem(hItem);
			}
			return true;
		}
		break;

	case MODITEM_MIDIINSTRUMENT:
	case MODITEM_MIDIPERCUSSION:
		if((m_itemDrag.type == MODITEM_INSLIB_SAMPLE) || (m_itemDrag.type == MODITEM_INSLIB_INSTRUMENT))
		{
			if(doDrop)
			{
				mpt::PathString fullPath = InsLibGetFullPath(m_hItemDrag);
				if(modItemDrop.type == MODITEM_MIDIINSTRUMENT)
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
{
	ModTreeDocInfo *info = GetDocumentInfoFromModDoc(modDoc);
	if(info == nullptr)
		return;

	const CSoundFile &sndFile = modDoc.GetSoundFile();
	ORDERINDEX nNewOrd = (pNotify) ? pNotify->order : ORDERINDEX_INVALID;
	SEQUENCEINDEX nNewSeq = sndFile.Order.GetCurrentSequenceIndex();
	if(nNewOrd != info->ordSel || nNewSeq != info->seqSel)
	{
		// Remove bold state from old item
		if(info->seqSel < info->tiOrders.size() && info->ordSel < info->tiOrders[info->seqSel].size())
			SetItemState(info->tiOrders[info->seqSel][info->ordSel], 0, TVIS_BOLD);

		info->ordSel = nNewOrd;
		info->seqSel = nNewSeq;
		if(info->seqSel < info->tiOrders.size() && info->ordSel < info->tiOrders[info->seqSel].size())
			SetItemState(info->tiOrders[info->seqSel][info->ordSel], TVIS_BOLD, TVIS_BOLD);
		else
			UpdateView(*info, SequenceHint().Data());
	}

	// Update sample / instrument playing status icons (will only detect instruments with samples, though)

	if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_LIVEUPDATETREE) == 0)
		return;
	// TODO: Is there a way to find out if the treeview is actually visible?
	/*static int nUpdateCount = 0;
	nUpdateCount++;
	if(nUpdateCount < 5) return; // don't update too often
	nUpdateCount = 0;*/

	// check whether the lists are actually visible (don't waste resources)
	const bool updateSamples = IsItemExpanded(info->hSamples), updateInstruments = IsItemExpanded(info->hInstruments);

	info->samplesPlaying.reset();
	info->instrumentsPlaying.reset();

	if(!updateSamples && !updateInstruments)
		return;

	for(const auto &chn : sndFile.m_PlayState.Chn)
	{
		if(chn.pCurrentSample != nullptr && chn.nLength != 0 && chn.IsSamplePlaying())
		{
			if(updateSamples)
			{
				for(SAMPLEINDEX nSmp = sndFile.GetNumSamples(); nSmp >= 1; nSmp--)
				{
					if(chn.pModSample == &sndFile.GetSample(nSmp))
					{
						info->samplesPlaying.set(nSmp);
						break;
					}
				}
			}
			if(updateInstruments)
			{
				for(INSTRUMENTINDEX nIns = sndFile.GetNumInstruments(); nIns >= 1; nIns--)
				{
					if(chn.pModInstrument == sndFile.Instruments[nIns])
					{
						info->instrumentsPlaying.set(nIns);
						break;
					}
				}
			}
		}
	}
	// what should be updated?
	if(updateSamples)
		UpdateView(*info, SampleHint().Info());
	if(updateInstruments)
		UpdateView(*info, InstrumentHint().Info());
}



/////////////////////////////////////////////////////////////////////////////
// CViewModTree message handlers


void CModTree::OnUpdate(CModDoc *pModDoc, UpdateHint hint, CObject *pHint)
{
	if(pHint == this)
		return;

	for(auto &[doc, docInfo] : m_docInfo)
	{
		if(doc == pModDoc || !pModDoc)
		{
			UpdateView(docInfo, hint);
			if(pModDoc)
				break;
		}
	}
}

void CModTree::OnItemExpanded(LPNMHDR pnmhdr, LRESULT *pResult)
{
	LPNMTREEVIEW pnm = (LPNMTREEVIEW)pnmhdr;
	if((pnm->itemNew.iImage == IMAGE_FOLDER) || (pnm->itemNew.iImage == IMAGE_OPENFOLDER))
	{
		int iNewImage = (pnm->itemNew.state & TVIS_EXPANDED) ? IMAGE_OPENFOLDER : IMAGE_FOLDER;
		SetItemImage(pnm->itemNew.hItem, iNewImage, iNewImage);
	}
	if(pResult)
		*pResult = TRUE;
}


void CModTree::OnBeginDrag(HTREEITEM hItem, bool bLeft, LRESULT *pResult)
{
	if(!(m_dwStatus & TREESTATUS_DRAGGING))
	{
		bool bDrag = false;

		m_hDropWnd = NULL;
		m_hItemDrag = hItem;
		if(m_hItemDrag != NULL)
		{
			if(!ItemHasChildren(m_hItemDrag))
				SelectItem(m_hItemDrag);
		}
		m_itemDrag = GetModItem(m_hItemDrag);
		m_dragDoc = m_selectedDoc;
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
				const CModDoc *pModDoc = m_docInfo.count(m_dragDoc) ? m_dragDoc : nullptr;
				if(pModDoc && pModDoc->GetSoundFile().Order.GetNumSequences() == 1)
					bDrag = true;
			}
			break;
		default:
			if(m_itemDrag.type == MODITEM_DLSBANK_INSTRUMENT)
				bDrag = true;
		}
		if(bDrag)
		{
			m_dwStatus |= (bLeft) ? TREESTATUS_LDRAG : TREESTATUS_RDRAG;
			m_hItemDrop = NULL;
			SetCapture();
		}
	}
	if(pResult)
		*pResult = TRUE;
}


void CModTree::OnBeginRDrag(LPNMHDR pnmhdr, LRESULT *pResult)
{
	if(pnmhdr)
	{
		LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmhdr;
		OnBeginDrag(pnmtv->itemNew.hItem, false, pResult);
	}
}


void CModTree::OnBeginLDrag(LPNMHDR pnmhdr, LRESULT *pResult)
{
	if(pnmhdr)
	{
		LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmhdr;
		OnBeginDrag(pnmtv->itemNew.hItem, true, pResult);
	}
}


void CModTree::OnItemDblClk(LPNMHDR, LRESULT *pResult)
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	HTREEITEM hItem = GetSelectedItem();
	if((hItem) && (hItem == HitTest(pt)))
	{
		ExecuteItem(hItem);
	}
	if(pResult)
		*pResult = 0;
}


void CModTree::OnItemReturn(LPNMHDR, LRESULT *pResult)
{
	HTREEITEM hItem = GetSelectedItem();
	if(hItem)
		ExecuteItem(hItem);
	if(pResult)
		*pResult = 0;
}


void CModTree::OnItemRightClick(LPNMHDR, LRESULT *pResult)
{
	CPoint pt, ptClient;
	UINT flags = 0;

	GetCursorPos(&pt);
	ptClient = pt;
	ScreenToClient(&ptClient);
	OnItemRightClick(HitTest(ptClient, &flags), pt);
	if(pResult)
		*pResult = 0;
}


void CModTree::OnItemRightClick(HTREEITEM hItem, CPoint pt)
{
	if(m_dwStatus & TREESTATUS_LDRAG)
	{
		if(ItemHasChildren(hItem))
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
		if(m_dwStatus & TREESTATUS_DRAGGING)
		{
			m_hItemDrop = NULL;
			m_hDropWnd = NULL;
			OnEndDrag(TREESTATUS_DRAGGING);
		}
		HMENU hMenu = ::CreatePopupMenu(), hSubMenu = nullptr;
		if(!hMenu)
			return;

		const CModDoc *modDoc = GetDocumentFromItem(hItem);
		const CSoundFile *sndFile = modDoc != nullptr ? &modDoc->GetSoundFile() : nullptr;
		const CInputHandler *ih = CMainFrame::GetInputHandler();

		UINT defaultID = 0;
		bool addSeparator = false;

		const ModItem modItem = GetModItem(hItem);
		const uint32 modItemID = modItem.val1;

		SelectItem(hItem);
		switch(modItem.type)
		{
		case MODITEM_HDR_SONG:
			defaultID = ID_MODTREE_EXECUTE;
			AppendMenu(hMenu, MF_STRING, defaultID, ih->GetKeyTextFromCommand(kcTreeViewOpen, _T("&View")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_CLOSE, _T("&Close"));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_RENAME, ih->GetKeyTextFromCommand(kcTreeViewRename, _T("Re&name")));
			break;

		case MODITEM_COMMENTS:
			defaultID = ID_MODTREE_EXECUTE;
			AppendMenu(hMenu, MF_STRING, defaultID, ih->GetKeyTextFromCommand(kcTreeViewOpen, _T("&View Comments")));
			break;

		case MODITEM_ORDER:
		case MODITEM_PATTERN:
			defaultID = ID_MODTREE_EXECUTE;
			AppendMenu(hMenu, MF_STRING, defaultID, ih->GetKeyTextFromCommand(kcTreeViewOpen, _T("&Edit Pattern")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE,
							ih->GetKeyTextFromCommand(kcTreeViewDelete, (modItem.type == MODITEM_ORDER) ? _T("&Delete from list") : _T("&Delete Pattern")));
			if(modItem.type == MODITEM_PATTERN && sndFile && sndFile->GetModSpecifications().hasPatternNames)
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_RENAME, ih->GetKeyTextFromCommand(kcTreeViewRename, _T("Re&name Pattern")));
			else if(modItem.type == MODITEM_ORDER)
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_RENAME, ih->GetKeyTextFromCommand(kcTreeViewRename, _T("&Set Pattern")));
			break;

		case MODITEM_SEQUENCE:
			if(sndFile)
			{
				bool isCurSeq = false;
				if(sndFile->GetModSpecifications().sequencesMax > 1)
				{
					if(sndFile->Order((SEQUENCEINDEX)modItemID).GetLength() == 0)
					{
						defaultID = ID_MODTREE_SWITCHTO;
					}
					isCurSeq = (sndFile->Order.GetCurrentSequenceIndex() == (SEQUENCEINDEX)modItemID);
				}

				if(!isCurSeq)
				{
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_SWITCHTO, _T("&Switch to Seqeuence"));
				}
				AppendMenu(hMenu, MF_STRING | (sndFile->Order.GetNumSequences() < MAX_SEQUENCES ? 0 : MF_GRAYED), ID_MODTREE_INSERT, ih->GetKeyTextFromCommand(kcTreeViewInsert, _T("&Insert Sequence")));
				AppendMenu(hMenu, MF_STRING | (sndFile->Order.GetNumSequences() < MAX_SEQUENCES ? 0 : MF_GRAYED), ID_MODTREE_DUPLICATE , ih->GetKeyTextFromCommand(kcTreeViewDuplicate,  _T("D&uplicate Sequence")));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, ih->GetKeyTextFromCommand(kcTreeViewDelete, _T("&Delete Sequence")));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_RENAME, ih->GetKeyTextFromCommand(kcTreeViewRename, _T("Re&name Sequence")));
			}
			break;


		case MODITEM_HDR_ORDERS:
			if(sndFile && sndFile->GetModSpecifications().sequencesMax > 1)
			{
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_INSERT, ih->GetKeyTextFromCommand(kcTreeViewInsert, _T("&Insert Sequence")));
				if(sndFile->Order.GetNumSequences() == 1)
				{
					// This is a sequence
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_DUPLICATE, ih->GetKeyTextFromCommand(kcTreeViewDuplicate, _T("D&uplicate Sequence")));
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_RENAME, ih->GetKeyTextFromCommand(kcTreeViewRename, _T("Re&name Sequence")));
				}
			}
			break;

		case MODITEM_SAMPLE:
			defaultID = ID_MODTREE_EXECUTE;
			AppendMenu(hMenu, MF_STRING, defaultID, ih->GetKeyTextFromCommand(kcTreeViewOpen, _T("&View Sample")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, ih->GetKeyTextFromCommand(kcTreeViewPlay, _T("&Play Sample")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_INSERT, ih->GetKeyTextFromCommand(kcTreeViewInsert, _T("&Insert Sample")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_DUPLICATE, ih->GetKeyTextFromCommand(kcTreeViewDuplicate, _T("D&uplicate Sample")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, ih->GetKeyTextFromCommand(kcTreeViewDelete, _T("&Delete Sample")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_RENAME, ih->GetKeyTextFromCommand(kcTreeViewRename, _T("Re&name Sample")));
			if(modDoc && !modDoc->GetNumInstruments())
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
					if(sndFile->SampleHasPath(smp) && smp != smpID)
					{
						anyPath = true;
						if(sndFile->GetSample(smp).HasSampleData() && sndFile->GetSample(smp).uFlags[SMP_MODIFIED])
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
			break;

		case MODITEM_INSTRUMENT:
			defaultID = ID_MODTREE_EXECUTE;
			AppendMenu(hMenu, MF_STRING, defaultID, ih->GetKeyTextFromCommand(kcTreeViewOpen, _T("&View Instrument")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, ih->GetKeyTextFromCommand(kcTreeViewPlay, _T("&Play Instrument")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_INSERT, ih->GetKeyTextFromCommand(kcTreeViewInsert, _T("&Insert Instrument")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_DUPLICATE, ih->GetKeyTextFromCommand(kcTreeViewDuplicate, _T("D&uplicate Instrument")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, ih->GetKeyTextFromCommand(kcTreeViewDelete, _T("&Delete Instrument")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_RENAME, ih->GetKeyTextFromCommand(kcTreeViewRename, _T("Re&name Instrument")));
			if(modDoc)
			{
				AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
				AppendMenu(hMenu, (modDoc->IsInstrumentMuted((INSTRUMENTINDEX)modItemID) ? MF_CHECKED : 0) | MF_STRING, ID_MODTREE_MUTE, _T("&Mute Instrument"));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_SOLO, _T("S&olo Instrument"));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_UNMUTEALL, _T("&Unmute all"));
			}
			break;

		case MODITEM_HDR_EFFECTS:
			if(sndFile && sndFile->m_loadedPlugins)
			{
				AppendMenu(hMenu, MF_STRING | (AllPluginsBypassed(*sndFile, false) ? MF_CHECKED : 0), ID_MODTREE_MUTE, _T("B&ypass All Plugins"));
				if(HasEffectPlugins(*sndFile))
					AppendMenu(hMenu, MF_STRING | (AllPluginsBypassed(*sndFile, true) ? MF_CHECKED : 0), ID_MODTREE_MUTE_ONLY_EFFECTS, _T("Bypass All &Effects"));
			}
			break;

		case MODITEM_EFFECT:
			defaultID = ID_MODTREE_EXECUTE;
			AppendMenu(hMenu, MF_STRING, defaultID, ih->GetKeyTextFromCommand(kcTreeViewOpen, _T("&Edit")));

			if(modDoc != nullptr)
			{
				AppendMenu(hMenu, (modDoc->GetSoundFile().m_MixPlugins[modItemID].IsBypassed() ? MF_CHECKED : 0) | MF_STRING, ID_MODTREE_MUTE, _T("&Bypass"));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, ih->GetKeyTextFromCommand(kcTreeViewDelete, _T("&Delete Plugin")));
			}
			break;

		case MODITEM_MIDIINSTRUMENT:
		case MODITEM_MIDIPERCUSSION:
			defaultID = ID_MODTREE_EXECUTE;
			AppendMenu(hMenu, MF_STRING, defaultID, ih->GetKeyTextFromCommand(kcTreeViewOpen, _T("&Map Instrument")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, ih->GetKeyTextFromCommand(kcTreeViewPlay, _T("&Play Instrument")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, _T("&Unmap Instrument"));
			AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
			[[fallthrough]];
		case MODITEM_HDR_MIDILIB:
		case MODITEM_HDR_MIDIGROUP:
			AppendMenu(hMenu, MF_STRING, ID_IMPORT_MIDILIB, _T("&Import MIDI Library"));
			AppendMenu(hMenu, MF_STRING, ID_EXPORT_MIDILIB, _T("E&xport MIDI Library"));
			addSeparator = true;
			break;

		case MODITEM_HDR_INSTRUMENTLIB:
			if(!IsSampleBrowser())
			{
				hSubMenu = AddLibraryFindAndSortMenus(hMenu);
				break;
			}
			if(!m_SongFileName.empty())
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_CLOSE, _T("&Close Song"));
			[[fallthrough]];
		case MODITEM_INSLIB_FOLDER:
			defaultID = ID_MODTREE_EXECUTE;
			AppendMenu(hMenu, MF_STRING, defaultID, ih->GetKeyTextFromCommand(kcTreeViewOpen, _T("&Browse...")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_RENAME, ih->GetKeyTextFromCommand(kcTreeViewRename, _T("Set &Path")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_OPENITEM, _T("&Open in Explorer"));
			hSubMenu = AddLibraryFindAndSortMenus(hMenu);

			{
				auto insDir = TrackerSettings::Instance().PathInstruments.GetDefaultDir();
				auto smpDir = TrackerSettings::Instance().PathSamples.GetDefaultDir();
				if(!insDir.empty() && insDir != m_InstrLibPath)
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_GOTO_INSDIR, _T("Go to &Instrument directory"));
				if(!smpDir.empty() && smpDir != insDir && smpDir != m_InstrLibPath)
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_GOTO_SMPDIR, _T("Go to Sa&mple directory"));
			}
			break;

		case MODITEM_INSLIB_SONG:
			defaultID = ID_MODTREE_EXECUTE;
			AppendMenu(hMenu, MF_STRING, defaultID, ih->GetKeyTextFromCommand(kcTreeViewOpen, _T("&Browse Song...")));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_OPENITEM, _T("&Edit Song"));
			hSubMenu = AddLibraryFindAndSortMenus(hMenu);
			AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, ih->GetKeyTextFromCommand(kcTreeViewDelete, _T("&Delete")));
			break;

		case MODITEM_INSLIB_SAMPLE:
		case MODITEM_INSLIB_INSTRUMENT:
			defaultID = ID_MODTREE_PLAY;
			if(!m_SongFileName.empty())
			{
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, ih->GetKeyTextFromCommand(kcTreeViewPlay, _T("&Play")));
			} else
			{
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, ih->GetKeyTextFromCommand(kcTreeViewPlay, _T("&Play File")));
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, ih->GetKeyTextFromCommand(kcTreeViewDelete, _T("&Delete")));
			}
			hSubMenu = AddLibraryFindAndSortMenus(hMenu);
			break;

		case MODITEM_DLSBANK_FOLDER:
			defaultID = ID_SOUNDBANK_PROPERTIES;
			AppendMenu(hMenu, MF_STRING, defaultID, _T("&Properties"));
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, _T("Re&move this bank"));
			[[fallthrough]];
		case MODITEM_NULL:
			AppendMenu(hMenu, MF_STRING, ID_ADD_SOUNDBANK, _T("Add Sound &Bank..."));
			addSeparator = true;
			break;

		case MODITEM_DLSBANK_INSTRUMENT:
			defaultID = ID_MODTREE_PLAY;
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, ih->GetKeyTextFromCommand(kcTreeViewPlay, _T("&Play Instrument")));
			break;
		}

		if(defaultID)
			SetMenuDefaultItem(hMenu, defaultID, FALSE);
		
		if((modItem.type == MODITEM_INSLIB_FOLDER)
			|| (modItem.type == MODITEM_INSLIB_SONG)
			|| (modItem.type == MODITEM_HDR_INSTRUMENTLIB))
		{
			if(addSeparator || defaultID)
				AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
			AppendMenu(hMenu, TrackerSettings::Instance().showDirsInSampleBrowser ? (MF_STRING|MF_CHECKED) : MF_STRING, ID_MODTREE_SHOWDIRS, _T("Show &Directories in Sample Browser"));
			AppendMenu(hMenu, (m_showAllFiles) ? (MF_STRING|MF_CHECKED) : MF_STRING, ID_MODTREE_SHOWALLFILES, _T("Show &All Files"));
			AppendMenu(hMenu, (m_showAllFiles) ? MF_STRING : (MF_STRING|MF_CHECKED), ID_MODTREE_SOUNDFILESONLY, _T("Show &Sound Files"));
			addSeparator = true;
		}

		if(addSeparator || defaultID)
			AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
		AppendMenu(hMenu, MF_STRING, ID_MODTREE_REFRESH, _T("&Refresh"));

		TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x + 4, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hMenu);
		if(hSubMenu)
			DestroyMenu(hSubMenu);
	}
}


HMENU CModTree::AddLibraryFindAndSortMenus(HMENU hMenu) const
{
	const CInputHandler *ih = CMainFrame::GetInputHandler();
	AppendMenu(hMenu, MF_STRING, ID_OPEN_LIBRARY_FILTER, ih->GetKeyTextFromCommand(kcTreeViewFind, _T("&Find...")));

	HMENU hSubMenu = ::CreatePopupMenu();
	AppendMenu(hSubMenu, MF_STRING | (m_librarySort == LibrarySortOrder::Name ? MF_CHECKED : 0), ID_MODTREE_SORT_BY_NAME, ih->GetKeyTextFromCommand(kcTreeViewSortByName, _T("&Name")));
	AppendMenu(hSubMenu, MF_STRING | (m_librarySort == LibrarySortOrder::Date ? MF_CHECKED : 0), ID_MODTREE_SORT_BY_DATE, ih->GetKeyTextFromCommand(kcTreeViewSortByDate, _T("&Date")));
	AppendMenu(hSubMenu, MF_STRING | (m_librarySort == LibrarySortOrder::Size ? MF_CHECKED : 0), ID_MODTREE_SORT_BY_SIZE, ih->GetKeyTextFromCommand(kcTreeViewSortBySize, _T("&Size")));
	AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), _T("Sort B&y"));
	return hSubMenu;
}


void CModTree::OnItemLeftClick(LPNMHDR, LRESULT *pResult)
{
	if(!(m_dwStatus & TREESTATUS_RDRAG))
	{
		POINT pt;
		UINT flags = 0;
		GetCursorPos(&pt);
		ScreenToClient(&pt);
		HTREEITEM hItem = HitTest(pt, &flags);
		if(hItem != NULL)
		{
			const ModItem modItem = GetModItem(hItem);
			const uint32 modItemID = modItem.val1;

			switch(modItem.type)
			{
			case MODITEM_INSLIB_FOLDER:
			case MODITEM_INSLIB_SONG:
				if(m_dwStatus & TREESTATUS_SINGLEEXPAND)
					ExecuteItem(hItem);
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
	if(pResult)
		*pResult = 0;
}


void CModTree::OnEndDrag(DWORD dwMask)
{
	if(m_dwStatus & dwMask)
	{
		m_dwStatus &= ~dwMask;
		if(!(m_dwStatus & TREESTATUS_DRAGGING))
		{
			ReleaseCapture();
			SetCursor(CMainFrame::curArrow);
			SelectDropTarget(nullptr);
			if(m_hItemDrop != nullptr)
			{
				CanDrop(m_hItemDrop, true);
			} else if(m_hDropWnd)
			{
				DRAGONDROP dropinfo;
				mpt::PathString fullPath;
				if(GetDropInfo(dropinfo, fullPath))
				{
					if(dropinfo.dropType == DRAGONDROP_SONG)
					{
						theApp.OpenDocumentFile(fullPath.ToCString());
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
{
	OnEndDrag(TREESTATUS_LDRAG);
	CTreeCtrl::OnLButtonUp(nFlags, point);
}


void CModTree::OnRButtonUp(UINT nFlags, CPoint point)
{
	OnEndDrag(TREESTATUS_RDRAG);
	CTreeCtrl::OnRButtonUp(nFlags, point);
}


void CModTree::OnXButtonUp(UINT nFlags, UINT nButton, CPoint point)
{
	bool isSampleBrowser = IsSampleBrowser();
	if(!isSampleBrowser)
	{
		// In the upper panel, only do folder navigation if the mouse cursor is somewhere below the "Instrument Library" item
		CRect rect;
		GetItemRect(m_hInsLib, rect, FALSE);
		if(point.y > rect.top)
			isSampleBrowser = true;
	}
	if(isSampleBrowser)
	{
		if(nButton == XBUTTON1)
		{
			InstrumentLibraryChDir(P_(".."), !m_SongFileName.empty());
		} else if(nButton == XBUTTON2)
		{
			const auto &previousPath = CMainFrame::GetMainFrame()->GetUpperTreeview()->m_previousPath;
			InstrumentLibraryChDir(previousPath, mpt::native_fs{}.is_file(m_InstrLibPath + previousPath));
		}
	}
	CTreeCtrl::OnXButtonUp(nFlags, nButton, point);
}


void CModTree::OnMouseMove(UINT nFlags, CPoint point)
{
	if(m_dwStatus & TREESTATUS_DRAGGING)
	{
		HTREEITEM hItem;
		UINT flags = 0;

		// Bug?
		if(!(nFlags & (MK_LBUTTON | MK_RBUTTON)))
		{
			m_itemDrag = ModItem(MODITEM_NULL);
			m_hItemDrag = NULL;
			OnEndDrag(TREESTATUS_DRAGGING);
			return;
		}
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if(pMainFrm)
		{
			CRect rect;
			GetClientRect(&rect);
			if(rect.PtInRect(point))
			{
				m_hDropWnd = m_hWnd;
				bool bCanDrop = CanDrop(HitTest(point, &flags), false);
				SetCursor((bCanDrop) ? CMainFrame::curDragging : CMainFrame::curNoDrop2);
			} else
			{
				CPoint screenPt = point;
				ClientToScreen(&screenPt);
				HWND hwnd = ::WindowFromPoint(screenPt);
				if(hwnd != m_hDropWnd)
				{
					bool canDrop = false;
					m_hDropWnd = hwnd;
					if(hwnd == m_hWnd)
					{
						canDrop = true;
					} else if(hwnd != NULL)
					{
						DRAGONDROP dropinfo;
						mpt::PathString fullPath;
						if(GetDropInfo(dropinfo, fullPath))
						{
							if(dropinfo.dropType == DRAGONDROP_SONG)
							{
								canDrop = true;
							} else if(::SendMessage(hwnd, WM_MOD_DRAGONDROPPING, FALSE, (LPARAM)&dropinfo))
							{
								canDrop = true;
							}
						}
					}
					SetCursor(canDrop ? CMainFrame::curDragging : CMainFrame::curNoDrop);
					if(canDrop)
					{
						if(GetDropHilightItem() != m_hItemDrag)
						{
							SelectDropTarget(m_hItemDrag);
						}
						m_hItemDrop = NULL;
						return;
					}
				}
			}
			if((point.x >= -1) && (point.x <= rect.right + GetSystemMetrics(SM_CXVSCROLL)))
			{
				if(point.y <= 0)
				{
					HTREEITEM hfirst = GetFirstVisibleItem();
					if(hfirst != NULL)
					{
						HTREEITEM hprev = GetPrevVisibleItem(hfirst);
						if(hprev != NULL)
							SelectSetFirstVisible(hprev);
					}
				} else if(point.y >= rect.bottom - 1)
				{
					hItem = HitTest(point, &flags);
					HTREEITEM hNext = GetNextItem(hItem, TVGN_NEXTVISIBLE);
					if(hNext != NULL)
					{
						EnsureVisible(hNext);
					}
				}
			}
			if((hItem = HitTest(point, &flags)) != NULL)
			{
				SelectDropTarget(hItem);
				m_hItemDrop = hItem;
			}
		}
	}
	CTreeCtrl::OnMouseMove(nFlags, point);
}


void CModTree::OnRefreshTree()
{
	BeginWaitCursor();
	for(auto &doc : m_docInfo)
	{
		UpdateView(doc.second, UpdateHint().ModType());
	}
	RefreshMidiLibrary();
	RefreshDlsBanks();
	RefreshInstrumentLibrary();
	EndWaitCursor();
}


void CModTree::OnExecuteItem()
{
	ExecuteItem(GetSelectedItem());
}


void CModTree::OnDeleteTreeItem()
{
	DeleteTreeItem(GetSelectedItem(), CMainFrame::GetInputHandler()->ShiftPressed());
}


void CModTree::OnPlayTreeItem()
{
	PlayItem(GetSelectedItem(), NOTE_MIDDLEC);
}


void CModTree::OnOpenTreeItem()
{
	OpenTreeItem(GetSelectedItem());
}


void CModTree::OnMuteTreeItem()
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	const uint32 modItemID = modItem.val1;

	ModTreeDocInfo *info = GetDocumentInfoFromItem(hItem);
	if(info)
	{
		CModDoc &modDoc = info->modDoc;
		if((modItem.type == MODITEM_SAMPLE) && !modDoc.GetNumInstruments())
		{
			modDoc.MuteSample((SAMPLEINDEX)modItemID, !modDoc.IsSampleMuted((SAMPLEINDEX)modItemID));
			UpdateView(*info, SampleHint((SAMPLEINDEX)modItemID).Info().Names());
		} else if((modItem.type == MODITEM_INSTRUMENT) && modDoc.GetNumInstruments())
		{
			modDoc.MuteInstrument((INSTRUMENTINDEX)modItemID, !modDoc.IsInstrumentMuted((INSTRUMENTINDEX)modItemID));
			UpdateView(*info, InstrumentHint((INSTRUMENTINDEX)modItemID).Info().Names());
		} else if(modItem.type == MODITEM_EFFECT)
		{
			IMixPlugin *pPlugin = modDoc.GetSoundFile().m_MixPlugins[modItemID].pMixPlugin;
			if(pPlugin == nullptr)
				return;
			pPlugin->ToggleBypass();
			if(modDoc.GetSoundFile().GetModSpecifications().supportsPlugins)
				modDoc.SetModified();
			//UpdateView(*info, PluginHint(static_cast<PLUGINDEX>(modItemID + 1)));
		} else if(modItem.type == MODITEM_HDR_EFFECTS)
		{
			auto &sndFile = modDoc.GetSoundFile();
			BypassAllPlugins(sndFile, !AllPluginsBypassed(sndFile, false), false);
		}
	}
}


void CModTree::OnMuteOnlyEffects()
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);

	ModTreeDocInfo *info = GetDocumentInfoFromItem(hItem);
	if(info)
	{
		CModDoc &modDoc = info->modDoc;
		if(modItem.type == MODITEM_HDR_EFFECTS)
		{
			auto &sndFile = modDoc.GetSoundFile();
			BypassAllPlugins(sndFile, !AllPluginsBypassed(sndFile, true), true);
		}
	}
}


void CModTree::OnSoloTreeItem()
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	const uint32 modItemID = modItem.val1;

	ModTreeDocInfo *info = GetDocumentInfoFromItem(hItem);
	if(info)
	{
		CModDoc &modDoc = info->modDoc;
		INSTRUMENTINDEX nInstruments = modDoc.GetNumInstruments();
		if((modItem.type == MODITEM_SAMPLE) && (!nInstruments))
		{
			for(SAMPLEINDEX nSmp = 1; nSmp <= modDoc.GetNumSamples(); nSmp++)
			{
				modDoc.MuteSample(nSmp, nSmp != modItemID);
			}
			UpdateView(*info, SampleHint().Info().Names());
		} else if((modItem.type == MODITEM_INSTRUMENT) && (nInstruments))
		{
			for(INSTRUMENTINDEX nIns = 1; nIns <= nInstruments; nIns++)
			{
				modDoc.MuteInstrument(nIns, nIns != modItemID);
			}
			UpdateView(*info, InstrumentHint().Info().Names());
		}
	}
}


void CModTree::OnUnmuteAllTreeItem()
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);

	ModTreeDocInfo *info = GetDocumentInfoFromItem(hItem);
	if(info)
	{
		CModDoc &modDoc = info->modDoc;
		if((modItem.type == MODITEM_SAMPLE) || (modItem.type == MODITEM_INSTRUMENT))
		{
			for(SAMPLEINDEX nSmp = 1; nSmp <= modDoc.GetNumSamples(); nSmp++)
			{
				modDoc.MuteSample(nSmp, false);
			}
			UpdateView(*info, SampleHint().Info().Names());
			for(INSTRUMENTINDEX nIns = 1; nIns <= modDoc.GetNumInstruments(); nIns++)
			{
				modDoc.MuteInstrument(nIns, false);
			}
			UpdateView(*info, InstrumentHint().Info().Names());
		}
	}
}


bool CModTree::HasEffectPlugins(const CSoundFile &sndFile)
{
	for(const auto &plugin : sndFile.m_MixPlugins)
	{
		if(!plugin.pMixPlugin)
			continue;
		if(!plugin.pMixPlugin->IsInstrument())
			return true;
	}
	return false;

}


bool CModTree::AllPluginsBypassed(const CSoundFile &sndFile, bool onlyEffects)
{
	for(const auto &plugin : sndFile.m_MixPlugins)
	{
		if(!plugin.pMixPlugin)
			continue;
		if(onlyEffects && plugin.pMixPlugin->IsInstrument())
			continue;
		if(!plugin.IsBypassed())
			return false;
	}
	return true;
}


void CModTree::BypassAllPlugins(CSoundFile &sndFile, bool bypass, bool onlyEffects)
{
	bool modified = false;
	for(auto &plugin : sndFile.m_MixPlugins)
	{
		if(!plugin.pMixPlugin)
			continue;
		if(onlyEffects && plugin.pMixPlugin->IsInstrument())
			continue;
		if(plugin.IsBypassed() != bypass)
		{
			plugin.pMixPlugin->Bypass(bypass);
			modified = true;
		}
	}
	if(modified && sndFile.GetModSpecifications().supportsPlugins && sndFile.GetpModDoc())
		sndFile.GetpModDoc()->SetModified();
}


// Helper function for generating an insert vector for samples/instruments/sequences
template <typename T>
static std::vector<T> GenerateInsertVector(size_t howMany, size_t insertPos, T insertId, T startId)
{
	std::vector<T> newOrder(howMany);
	std::iota(newOrder.begin(), newOrder.end(), startId);
	newOrder.insert(newOrder.begin() + insertPos, insertId);
	return newOrder;
}


void CModTree::InsertOrDupItem(bool insert)
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	const uint32 modItemID = modItem.val1;

	ModTreeDocInfo *info = GetDocumentInfoFromItem(hItem);
	if(info)
	{
		CModDoc &modDoc = info->modDoc;
		CSoundFile &sndFile = modDoc.GetSoundFile();
		if(modItem.type == MODITEM_SEQUENCE || modItem.type == MODITEM_HDR_ORDERS)
		{
			// Duplicate / insert sequence
			const SEQUENCEINDEX newIndex = (modItem.type == MODITEM_HDR_ORDERS) ? sndFile.Order.GetNumSequences() : static_cast<SEQUENCEINDEX>(modItemID + 1);
			std::vector<SEQUENCEINDEX> newOrder = GenerateInsertVector<SEQUENCEINDEX>(sndFile.Order.GetNumSequences(), newIndex, static_cast<SEQUENCEINDEX>(insert ? SEQUENCEINDEX_INVALID : modItemID), 0);
			if(modDoc.ReArrangeSequences(newOrder) != SEQUENCEINDEX_INVALID)
			{
				sndFile.Order.SetSequence(newIndex);
				if(const auto name = sndFile.Order().GetName(); !insert && !name.empty())
					sndFile.Order().SetName(name + U_(" (Copy)"));
				modDoc.UpdateAllViews(nullptr, SequenceHint(SEQUENCEINDEX_INVALID).Names().Data());
				modDoc.SetModified();
			} else
			{
				Reporting::Error("Maximum number of sequences reached.");
			}
		} else if(modItem.type == MODITEM_SAMPLE)
		{
			// Duplicate / insert sample
			std::vector<SAMPLEINDEX> newOrder = GenerateInsertVector<SAMPLEINDEX>(sndFile.GetNumSamples(), modItemID, static_cast<SAMPLEINDEX>(insert ? 0 : modItemID), 1);
			if(modDoc.ReArrangeSamples(newOrder) != SAMPLEINDEX_INVALID)
			{
				modDoc.SetModified();
				modDoc.UpdateAllViews(nullptr, SampleHint().Info().Data().Names());
				modDoc.UpdateAllViews(nullptr, PatternHint().Data());
			} else
			{
				Reporting::Error("Maximum number of samples reached.");
			}
		} else if(modItem.type == MODITEM_INSTRUMENT)
		{
			// Duplicate / insert instrument
			std::vector<INSTRUMENTINDEX> newOrder = GenerateInsertVector<INSTRUMENTINDEX>(sndFile.GetNumInstruments(), modItemID, static_cast<INSTRUMENTINDEX>(insert ? 0 : modItemID), 1);
			if(modDoc.ReArrangeInstruments(newOrder) != INSTRUMENTINDEX_INVALID)
			{
				modDoc.UpdateAllViews(NULL, InstrumentHint().Info().Envelope().Names());
				modDoc.UpdateAllViews(nullptr, PatternHint().Data());
				modDoc.SetModified();
			} else
			{
				Reporting::Error("Maximum number of instruments reached.");
			}
		}
	}
}


void CModTree::OnSwitchToTreeItem()
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);

	CModDoc *pModDoc = GetDocumentFromItem(hItem);
	if(pModDoc && (modItem.type == MODITEM_SEQUENCE))
	{
		pModDoc->ActivateView(IDD_CONTROL_PATTERNS, uint32(modItem.val1 << SEQU_SHIFT) | SEQU_INDICATOR);
	}
}


void CModTree::OnSetItemPath()
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	CModDoc *pModDoc = GetDocumentFromItem(hItem);

	if(pModDoc && modItem.val1)
	{
		SAMPLEINDEX smpID = static_cast<SAMPLEINDEX>(modItem.val1);
		const mpt::PathString path = pModDoc->GetSoundFile().GetSamplePath(smpID);
		FileDialog dlg = OpenFileDialog()
			.ExtensionFilter(U_("All Samples|*.wav;*.flac|All files(*.*)|*.*||"));	// Only show samples that we actually can save as well.
		if(path.empty())
			dlg.WorkingDirectory(TrackerSettings::Instance().PathSamples.GetWorkingDir());
		else
			dlg.DefaultFilename(path);
		if(!dlg.Show())
			return;
		TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetWorkingDirectory());

		if(dlg.GetFirstFile() != pModDoc->GetSoundFile().GetSamplePath(smpID))
		{
			pModDoc->GetSoundFile().SetSamplePath(smpID, dlg.GetFirstFile());
			pModDoc->SetModified();
		}
		OnReloadItem();
	}
}


void CModTree::OnSaveItem()
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	CModDoc *pModDoc = GetDocumentFromItem(hItem);

	if(pModDoc && modItem.val1)
	{
		SAMPLEINDEX smpID = static_cast<SAMPLEINDEX>(modItem.val1);
		pModDoc->SaveSample(smpID);
		if(pModDoc)
			pModDoc->UpdateAllViews(NULL, SampleHint(smpID).Info());
		OnRefreshTree();
	}
}


void CModTree::OnSaveAll()
{
	CModDoc *pModDoc = GetDocumentFromItem(GetSelectedItem());
	if(pModDoc != nullptr)
	{
		pModDoc->SaveAllSamples(false);
		if(pModDoc)
			pModDoc->UpdateAllViews(nullptr, SampleHint().Info());
		OnRefreshTree();
	}
}


void CModTree::OnReloadItem()
{
	HTREEITEM hItem = GetSelectedItem();

	const ModItem modItem = GetModItem(hItem);
	CModDoc *pModDoc = GetDocumentFromItem(hItem);

	if(pModDoc && modItem.val1)
	{
		SAMPLEINDEX smpID = static_cast<SAMPLEINDEX>(modItem.val1);
		CSoundFile &sndFile = pModDoc->GetSoundFile();
		pModDoc->GetSampleUndo().PrepareUndo(smpID, sundo_replace, "Replace");
		if(!sndFile.LoadExternalSample(smpID, sndFile.GetSamplePath(smpID)))
		{
			pModDoc->GetSampleUndo().RemoveLastUndoStep(smpID);
			Reporting::Error(_T("Unable to load sample:\n") + sndFile.GetSamplePath(smpID).AsNative());
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
{
	CModDoc *pModDoc = GetDocumentFromItem(GetSelectedItem());
	if(pModDoc != nullptr)
	{
		CSoundFile &sndFile = pModDoc->GetSoundFile();
		bool anyMissing = false;
		for(SAMPLEINDEX smp = 1; smp <= sndFile.GetNumSamples(); smp++)
		{
			const mpt::PathString &path = sndFile.GetSamplePath(smp);
			if(path.empty())
				continue;

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
{
	CModDoc *pModDoc = GetDocumentFromItem(GetSelectedItem());
	if(pModDoc == nullptr)
	{
		return;
	}
	MissingExternalSamplesDlg dlg(*pModDoc, CMainFrame::GetMainFrame());
	dlg.DoModal();
}


void CModTree::OnAddDlsBank()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm)
		pMainFrm->OnAddDlsBank();
}


void CModTree::OnImportMidiLib()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm)
		pMainFrm->OnImportMidiLib();
}


void CModTree::OnExportMidiLib()
{
	FileDialog dlg = SaveFileDialog()
		.DefaultExtension(U_("ini"))
		.DefaultFilename(P_("mptrack.ini"))
		.ExtensionFilter(U_("Text and INI files (*.txt,*.ini)|*.txt;*.ini|All Files (*.*)|*.*||"));
	if(!dlg.Show()) return;

	CTrackApp::ExportMidiConfig(dlg.GetFirstFile());
}


///////////////////////////////////////////////////////////////////////
// Drop support

DROPEFFECT CModTree::OnDragEnter(COleDataObject *, DWORD, CPoint)
{
	return DROPEFFECT_LINK;
}


DROPEFFECT CModTree::OnDragOver(COleDataObject *, DWORD, CPoint point)
{
	UINT flags = 0;
	HTREEITEM hItem = HitTest(point, &flags);

	const ModItem modItem = GetModItem(hItem);

	switch(modItem.type)
	{
	case MODITEM_MIDIINSTRUMENT:
	case MODITEM_MIDIPERCUSSION:
		if(hItem != GetDropHilightItem())
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


BOOL CModTree::OnDrop(COleDataObject *pDataObject, DROPEFFECT, CPoint)
{
	STGMEDIUM stgm;
	HDROP hDropInfo;
	UINT nFiles;
	BOOL bOk = FALSE;

	if(!pDataObject)
		return FALSE;
	if(!pDataObject->GetData(CF_HDROP, &stgm))
		return FALSE;
	if(stgm.tymed != TYMED_HGLOBAL)
		return FALSE;
	if(stgm.hGlobal == NULL)
		return FALSE;
	hDropInfo = (HDROP)stgm.hGlobal;
	nFiles = DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	if(nFiles)
	{
		UINT size = ::DragQueryFile(hDropInfo, 0, nullptr, 0) + 1;
		std::vector<TCHAR> fileName(size, _T('\0'));
		if(DragQueryFile(hDropInfo, 0, fileName.data(), size))
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
{
	BeginWaitCursor();
	RefreshInstrumentLibrary();
	EndWaitCursor();
}


void CModTree::OnOpenInstrumentLibraryFilter()
{
	static_cast<CModTreeBar*>(GetParent())->StartTreeFilter(*this);
}


void CModTree::OnShowDirectories()
{
	TrackerSettings::Instance().showDirsInSampleBrowser = !TrackerSettings::Instance().showDirsInSampleBrowser;
	OnRefreshInstrLib();
}


void CModTree::OnShowAllFiles()
{
	if(!m_showAllFiles)
	{
		m_showAllFiles = true;
		OnRefreshInstrLib();
	}
}


void CModTree::OnShowSoundFiles()
{
	if(m_showAllFiles)
	{
		m_showAllFiles = false;
		OnRefreshInstrLib();
	}
}


void CModTree::OnGotoInstrumentDir()
{
	SetFullInstrumentLibraryPath(TrackerSettings::Instance().PathInstruments.GetDefaultDir());
}


void CModTree::OnGotoSampleDir()
{
	SetFullInstrumentLibraryPath(TrackerSettings::Instance().PathSamples.GetDefaultDir());
}


void CModTree::OnSoundBankProperties()
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
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	ModCommand::NOTE note = NOTE_NONE;
	const bool start = wParam >= kcTreeViewStartNotes && wParam <= kcTreeViewEndNotes;
	const bool stop = wParam >= kcTreeViewStartNoteStops && wParam <= kcTreeViewEndNoteStops && !IsSampleBrowser();

	switch(wParam)
	{
	case kcTreeViewStopPreview:
		note = NOTE_NOTECUT;
		break;

	case kcTreeViewOpen:
		if(HTREEITEM hItem = GetSelectedItem(); hItem)
		{
			if(!ExecuteItem(hItem))
			{
				if(ItemHasChildren(hItem))
				{
					Expand(hItem, TVE_TOGGLE);
				}
			}
		}
		return wParam;

	case kcTreeViewPlay:
		OnPlayTreeItem();
		return wParam;

	case kcTreeViewInsert:
	case kcTreeViewDuplicate:
		InsertOrDupItem(wParam == kcTreeViewInsert);
		return wParam;

	case kcTreeViewDelete:
	case kcTreeViewDeletePermanently:
		DeleteTreeItem(GetSelectedItem(), wParam == kcTreeViewDeletePermanently);
		return wParam;

	case kcTreeViewFind:
		OnOpenInstrumentLibraryFilter();
		return wParam;

	case kcTreeViewRename:
	case kcTreeViewSendToEditorInsertNew:
		if(HTREEITEM hItem = GetSelectedItem(); hItem)
		{
			const ModItem modItem = GetModItem(hItem);
			static constexpr ModItemType instrumentTypes[] = {MODITEM_INSLIB_SAMPLE, MODITEM_INSLIB_INSTRUMENT, MODITEM_MIDIINSTRUMENT, MODITEM_MIDIPERCUSSION, MODITEM_DLSBANK_INSTRUMENT};
			if(mpt::contains(instrumentTypes, modItem.type))
			{
				// Load sample into currently selected (or new) sample or instrument slot
				CModScrollView *view = static_cast<CModScrollView *>(CMainFrame::GetMainFrame()->GetActiveView());
				if(view)
				{
					const char *className = view->GetRuntimeClass()->m_lpszClassName;
					const bool isSampleView = !strcmp("CViewSample", className);
					const bool isInstrumentView = !strcmp("CViewInstrument", className);
					mpt::PathString fullPath = InsLibGetFullPath(hItem);
					DRAGONDROP dropInfo;
					m_hItemDrag = hItem;
					m_itemDrag = modItem;
					if((isSampleView || isInstrumentView) && GetDropInfo(dropInfo, fullPath))
					{
						dropInfo.insertType = (wParam == kcTreeViewSendToEditorInsertNew) ? DRAGONDROP::InsertType::InsertNew : DRAGONDROP::InsertType::Replace;
						view->SendMessage(WM_MOD_DRAGONDROPPING, TRUE, reinterpret_cast<LPARAM>(&dropInfo));
						// In case a message box like "create instrument for sample?" showed up
						SetFocus();
					}
				}
			} else if(!IsSampleBrowser() && wParam != kcTreeViewSendToEditorInsertNew)
			{
				EditLabel(hItem);
			}
		}
		return wParam;

	case kcTreeViewSortByName:
		OnSortByName();
		return wParam;
	case kcTreeViewSortByDate:
		OnSortByDate();
		return wParam;
	case kcTreeViewSortBySize:
		OnSortBySize();
		return wParam;

	default:
		if(start || stop)
		{
			const ModItem modItem = GetModItem(GetSelectedItem());
			CModDoc *modDoc = m_docInfo.count(m_selectedDoc) ? m_selectedDoc : nullptr;
			const int noteOffset = static_cast<int>(wParam - (start ? kcTreeViewStartNotes : kcTreeViewStartNoteStops));
			note = static_cast<ModCommand::NOTE>(Clamp(NOTE_MIN + pMainFrm->GetBaseOctave() * 12 + noteOffset, NOTE_MIN, NOTE_MAX));
			if(modDoc && modItem.type == MODITEM_INSTRUMENT)
				note = modDoc->GetNoteWithBaseOctave(noteOffset, static_cast<INSTRUMENTINDEX>(modItem.val1));
		}
		break;
	}

	if(note != NOTE_NONE)
	{
		if(stop)
			note |= 0x80;

		if(PlayItem(GetSelectedItem(), note))
			return wParam;
		else
			return kcNull;
	}

	return kcNull;
}


LRESULT CModTree::OnMidiMsg(WPARAM midiData_, LPARAM)
{
	uint32 midiData = static_cast<uint32>(midiData_);
	// Handle MIDI messages assigned to shortcuts
	CInputHandler *ih = CMainFrame::GetInputHandler();
	ih->HandleMIDIMessage(kCtxViewTree, midiData) != kcNull
	    || ih->HandleMIDIMessage(kCtxAllContexts, midiData) != kcNull;

	uint8 midiByte1 = MIDIEvents::GetDataByte1FromEvent(midiData);
	int volume;
	switch(MIDIEvents::GetTypeFromEvent(midiData))
	{
	case MIDIEvents::evNoteOn:
		volume = MIDIEvents::GetDataByte2FromEvent(midiData);
		if(volume > 0)
		{
			PlayItem(GetSelectedItem(), midiByte1 + NOTE_MIN, Util::muldivr(volume, 256, 127));
			return 1;
		}
		[[fallthrough]];
	case MIDIEvents::evNoteOff:
		PlayItem(GetSelectedItem(), NOTE_NOTECUT);
		return 1;
	}
	return 0;
}


void CModTree::OnKillFocus(CWnd *pNewWnd)
{
	CTreeCtrl::OnKillFocus(pNewWnd);
	CMainFrame::GetMainFrame()->m_bModTreeHasFocus = false;
	if(pNewWnd != nullptr)
		CMainFrame::GetMainFrame()->SetMidiRecordWnd(pNewWnd->m_hWnd);
}


void CModTree::OnSetFocus(CWnd *pOldWnd)
{
	CTreeCtrl::OnSetFocus(pOldWnd);
	CMainFrame::GetMainFrame()->m_bModTreeHasFocus = true;
	CMainFrame::GetMainFrame()->SetMidiRecordWnd(m_hWnd);
}


bool CModTree::IsItemExpanded(HTREEITEM hItem)
{
	// checks if a treeview item is expanded.
	if(hItem == NULL)
		return false;
	TVITEM tvi;
	tvi.mask = TVIF_HANDLE | TVIF_STATE;
	tvi.state = 0;
	tvi.stateMask = TVIS_EXPANDED;
	tvi.hItem = hItem;
	GetItem(&tvi);
	return (tvi.state & TVIS_EXPANDED) != 0;
}


void CModTree::OnCloseItem()
{
	HTREEITEM hItem = GetSelectedItem();
	if(hItem == m_hInsLib && !m_SongFileName.empty())
	{
		InstrumentLibraryChDir(P_(".."), true);
		return;
	}
	CModDoc *pModDoc = GetDocumentFromItem(hItem);
	if(pModDoc == nullptr)
		return;
	// Spam our message to the first available view
	POSITION pos = pModDoc->GetFirstViewPosition();
	if(pos == nullptr)
		return;
	CView *pView = pModDoc->GetNextView(pos);
	if(pView)
		pView->PostMessage(WM_COMMAND, ID_FILE_CLOSE);
}


// Delete all children of a tree item
void CModTree::DeleteChildren(HTREEITEM hItem)
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
HTREEITEM CModTree::GetNthChildItem(HTREEITEM hItem, int index) const
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
HTREEITEM CModTree::GetParentRootItem(HTREEITEM hItem) const
{
	while(hItem != nullptr)
	{
		const HTREEITEM h = GetParentItem(hItem);
		if(h == nullptr || h == hItem)
			break;
		hItem = h;
	}
	return hItem;
}


void CModTree::OnRenameItem()
{
	EditLabel(GetSelectedItem());
}


// Editing sample, instrument, order, pattern, etc. labels
void CModTree::OnBeginLabelEdit(NMHDR *nmhdr, LRESULT *result)
{
	NMTVDISPINFO *info = reinterpret_cast<NMTVDISPINFO *>(nmhdr);
	CEdit *editCtrl = GetEditControl();
	if(editCtrl == nullptr)
		return;

	const ModItem modItem = GetModItem(info->item.hItem);
	const CModDoc *modDoc = modItem.IsSongItem() ? GetDocumentFromItem(info->item.hItem) : nullptr;

	mpt::ustring text;
	m_doLabelEdit = false;

	if(modDoc != nullptr)
	{
		const CSoundFile &sndFile = modDoc->GetSoundFile();
		const CModSpecifications &modSpecs = sndFile.GetModSpecifications();

		switch(modItem.type)
		{
		case MODITEM_HDR_SONG:
			text = mpt::ToUnicode(sndFile.GetCharsetInternal(), sndFile.m_songName);
			editCtrl->SetLimitText(modSpecs.modNameLengthMax);
			m_doLabelEdit = true;
			break;

		case MODITEM_ORDER:
			{
				PATTERNINDEX pat = sndFile.Order(static_cast<SEQUENCEINDEX>(modItem.val2)).at(static_cast<ORDERINDEX>(modItem.val1));
				if(pat == sndFile.Order.GetInvalidPatIndex())
					text = UL_("---");
				else if(pat == sndFile.Order.GetIgnoreIndex())
					text = UL_("+++");
				else
					text = mpt::ufmt::val(pat);
				m_doLabelEdit = true;
			}
			break;

		case MODITEM_HDR_ORDERS:
			if(sndFile.Order.GetNumSequences() != 1 || sndFile.GetModSpecifications().sequencesMax <= 1)
			{
				break;
			}
			[[fallthrough]];
		case MODITEM_SEQUENCE:
			if(modItem.val1 < sndFile.Order.GetNumSequences())
			{
				text = sndFile.Order(static_cast<SEQUENCEINDEX>(modItem.val1)).GetName();
				m_doLabelEdit = true;
			}
			break;

		case MODITEM_PATTERN:
			if(modItem.val1 < sndFile.Patterns.GetNumPatterns() && modSpecs.hasPatternNames)
			{
				text = mpt::ToUnicode(sndFile.GetCharsetInternal(), sndFile.Patterns[modItem.val1].GetName());
				editCtrl->SetLimitText(MAX_PATTERNNAME - 1);
				m_doLabelEdit = true;
			}
			break;

		case MODITEM_SAMPLE:
			if(modItem.val1 <= sndFile.GetNumSamples())
			{
				text = mpt::ToUnicode(sndFile.GetCharsetInternal(), sndFile.m_szNames[modItem.val1]);
				editCtrl->SetLimitText(modSpecs.sampleNameLengthMax);
				m_doLabelEdit = true;
			}
			break;

		case MODITEM_INSTRUMENT:
			if(modItem.val1 <= sndFile.GetNumInstruments() && sndFile.Instruments[modItem.val1] != nullptr)
			{
				text = mpt::ToUnicode(sndFile.GetCharsetInternal(), sndFile.Instruments[modItem.val1]->name);
				editCtrl->SetLimitText(modSpecs.instrNameLengthMax);
				m_doLabelEdit = true;
			}
			break;
		}
	} else if(modItem.type == MODITEM_HDR_INSTRUMENTLIB)
	{
		text = (m_InstrLibPath + m_SongFileName).ToUnicode();
		m_doLabelEdit = true;
	}

	if(m_doLabelEdit)
	{
		CMainFrame::GetInputHandler()->Bypass(true);
		editCtrl->SetWindowText(mpt::ToCString(text));
	}
	*result = m_doLabelEdit ? FALSE : TRUE;
}


// End editing sample, instrument, order, pattern, etc. labels
void CModTree::OnEndLabelEdit(NMHDR *nmhdr, LRESULT *result)
{
	CMainFrame::GetInputHandler()->Bypass(false);
	m_doLabelEdit = false;

	NMTVDISPINFO *info = reinterpret_cast<NMTVDISPINFO *>(nmhdr);
	const ModItem modItem = GetModItem(info->item.hItem);
	CModDoc *modDoc = modItem.IsSongItem() ? GetDocumentFromItem(info->item.hItem) : nullptr;

	*result = FALSE;
	if(info->item.pszText == nullptr)
		return;

	if(modDoc != nullptr)
	{
		CSoundFile &sndFile = modDoc->GetSoundFile();
		const CModSpecifications &modSpecs = sndFile.GetModSpecifications();

		const mpt::ustring itemText = mpt::ToUnicode(CString(info->item.pszText));
		switch(modItem.type)
		{
		case MODITEM_HDR_SONG:
			if(mpt::ToUnicode(sndFile.GetCharsetInternal(), sndFile.m_songName) != itemText)
			{
				sndFile.m_songName = mpt::truncate(mpt::ToCharset(sndFile.GetCharsetInternal(), itemText), modSpecs.modNameLengthMax);
				modDoc->SetModified();
				modDoc->UpdateAllViews(nullptr, GeneralHint().General());
			}
			break;

		case MODITEM_ORDER:
			if(!itemText.empty())
			{
				PATTERNINDEX pat = ConvertStrTo<PATTERNINDEX>(itemText);
				bool valid = true;
				if(itemText[0] == UC_('-'))
				{
					pat = sndFile.Order.GetInvalidPatIndex();
				} else if(itemText[0] == UC_('+'))
				{
					if(modSpecs.hasIgnoreIndex)
						pat = sndFile.Order.GetIgnoreIndex();
					else
						valid = false;
				} else
				{
					valid = (pat < sndFile.Patterns.GetNumPatterns());
				}
				PATTERNINDEX &target = sndFile.Order(static_cast<SEQUENCEINDEX>(modItem.val2))[static_cast<ORDERINDEX>(modItem.val1)];
				if(valid && pat != target)
				{
					target = pat;
					modDoc->SetModified();
					modDoc->UpdateAllViews(nullptr, SequenceHint().Data());
				}
			} else
			{
				MessageBeep(MB_ICONWARNING);
			}
			break;

		case MODITEM_HDR_ORDERS:
		case MODITEM_SEQUENCE:
			if(modItem.val1 < sndFile.Order.GetNumSequences() && sndFile.Order(static_cast<SEQUENCEINDEX>(modItem.val1)).GetName() != itemText)
			{
				sndFile.Order(static_cast<SEQUENCEINDEX>(modItem.val1)).SetName(itemText);
				modDoc->SetModified();
				modDoc->UpdateAllViews(nullptr, SequenceHint(static_cast<SEQUENCEINDEX>(modItem.val1)).Names());
			}
			break;

		case MODITEM_PATTERN:
			if(modItem.val1 < sndFile.Patterns.GetNumPatterns() && modSpecs.hasPatternNames && mpt::ToUnicode(sndFile.GetCharsetInternal(), sndFile.Patterns[modItem.val1].GetName()) != itemText)
			{
				sndFile.Patterns[modItem.val1].SetName(mpt::ToCharset(sndFile.GetCharsetInternal(), itemText));
				modDoc->SetModified();
				modDoc->UpdateAllViews(nullptr, PatternHint(static_cast<PATTERNINDEX>(modItem.val1)).Data().Names());
			}
			break;

		case MODITEM_SAMPLE:
			if(modItem.val1 <= sndFile.GetNumSamples() && mpt::ToUnicode(sndFile.GetCharsetInternal(), sndFile.m_szNames[modItem.val1]) != itemText)
			{
				sndFile.m_szNames[modItem.val1] = mpt::truncate(mpt::ToCharset(sndFile.GetCharsetInternal(), itemText), modSpecs.sampleNameLengthMax);
				modDoc->SetModified();
				modDoc->UpdateAllViews(nullptr, SampleHint(static_cast<SAMPLEINDEX>(modItem.val1)).Info().Names());
			}
			break;

		case MODITEM_INSTRUMENT:
			if(modItem.val1 <= sndFile.GetNumInstruments() && sndFile.Instruments[modItem.val1] != nullptr && mpt::ToUnicode(sndFile.GetCharsetInternal(), sndFile.Instruments[modItem.val1]->name) != itemText)
			{
				sndFile.Instruments[modItem.val1]->name = mpt::truncate(mpt::ToCharset(sndFile.GetCharsetInternal(), itemText), modSpecs.instrNameLengthMax);
				modDoc->SetModified();
				modDoc->UpdateAllViews(nullptr, InstrumentHint(static_cast<INSTRUMENTINDEX>(modItem.val1)).Info().Names());
			}
			break;
		}
	} else if(modItem.type == MODITEM_HDR_INSTRUMENTLIB)
	{
		const auto newPath = mpt::PathString::FromNative(info->item.pszText);
		if(mpt::PathCompareNoCase(newPath, m_InstrLibPath + m_SongFileName))
			SetFullInstrumentLibraryPath(newPath);
	}
}


// Drop files from Windows
void CModTree::OnDropFiles(HDROP hDropInfo)
{
	bool refreshDLS = false;
	const UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	CMainFrame::GetMainFrame()->SetForegroundWindow();
	for(UINT f = 0; f < nFiles; f++)
	{
		UINT size = ::DragQueryFile(hDropInfo, f, nullptr, 0) + 1;
		std::vector<TCHAR> fileName(size, _T('\0'));
		if(::DragQueryFile(hDropInfo, f, fileName.data(), size))
		{
			mpt::PathString file(mpt::PathString::FromNative(fileName.data()));
			if(IsSampleBrowser())
			{
				// Set sample browser location to this directory or file
				SetFullInstrumentLibraryPath(file);
				break;
			} else
			{
				if(CTrackApp::AddDLSBank(file))
				{
					refreshDLS = true;
				} else
				{
					// Pass message on
					theApp.OpenDocumentFile(file.ToCString());
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
