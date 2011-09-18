#include "stdafx.h"
#include "mptrack.h"
#include "moddoc.h"
#include "mainfrm.h"
#include "view_tre.h"
#include "dlsbank.h"
#include "dlg_misc.h"
#include "vstplug.h"

#ifndef TVS_SINGLEEXPAND
#define TVS_SINGLEEXPAND	0x400
#endif


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


/////////////////////////////////////////////////////////////////////////////
// CModTree

BEGIN_MESSAGE_MAP(CModTree, CTreeCtrl)
	//{{AFX_MSG_MAP(CViewModTree)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_KEYDOWN()
	ON_NOTIFY_REFLECT(NM_DBLCLK,		OnItemDblClk)
	ON_NOTIFY_REFLECT(NM_RETURN,		OnItemReturn)
	ON_NOTIFY_REFLECT(NM_RCLICK,		OnItemRightClick)
	ON_NOTIFY_REFLECT(NM_CLICK,			OnItemLeftClick)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED,	OnItemExpanded)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG,	OnBeginLDrag)
	ON_NOTIFY_REFLECT(TVN_BEGINRDRAG,	OnBeginRDrag)
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

	// -> CODE#0023
// -> DESC="IT project files (.itp)"
	ON_COMMAND(ID_MODTREE_SETPATH,		OnSetItemPath)
	ON_COMMAND(ID_MODTREE_SAVEITEM,		OnSaveItem)
// -! NEW_FEATURE#0023

	ON_COMMAND(ID_ADD_SOUNDBANK,		OnAddDlsBank)
	ON_COMMAND(ID_IMPORT_MIDILIB,		OnImportMidiLib)
	ON_COMMAND(ID_EXPORT_MIDILIB,		OnExportMidiLib)
	ON_COMMAND(ID_SOUNDBANK_PROPERTIES,	OnSoundBankProperties)
	ON_COMMAND(ID_MODTREE_SHOWALLFILES, OnShowAllFiles)
	ON_COMMAND(ID_MODTREE_SOUNDFILESONLY,OnShowSoundFiles)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,		OnCustomKeyMsg)	//rewbs.customKeys
	//}}AFX_MSG_MAP
	ON_WM_KILLFOCUS()		//rewbs.customKeys
	ON_WM_SETFOCUS()		//rewbs.customKeys
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CViewModTree construction/destruction

CModTree::CModTree(CModTree *pDataTree)
//-------------------------------------
{
	m_pDataTree = pDataTree;
	m_dwStatus = 0;
	m_bShowAllFiles = false;
	m_hItemDrag = m_hItemDrop = NULL;
	m_szInstrLibPath[0] = 0;
	m_szOldPath[0] = 0;
	m_szSongName[0] = 0;
	m_hDropWnd = NULL;
	m_hInsLib = m_hMidiLib = NULL;
	m_nDocNdx = m_nDragDocNdx = 0;
	MemsetZero(m_tiMidiGrp);
	MemsetZero(m_tiMidi);
	MemsetZero(m_tiPerc);
	MemsetZero(m_tiDLS);
	DocInfo.clear();
}


CModTree::~CModTree()
//-------------------
{
	vector<ModTreeDocInfo *>::iterator iter;
	for (iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
	{
		delete (*iter);
	}
	DocInfo.clear();
}


void CModTree::Init()
//-------------------
{
	DWORD dwRemove = TVS_EDITLABELS|TVS_SINGLEEXPAND;
	DWORD dwAdd = TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;

	if (!m_pDataTree)
	{
		dwRemove |= (TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS);
		dwAdd &= ~(TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS);
	}
	if (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_SINGLEEXPAND)
	{
		dwRemove &= ~TVS_SINGLEEXPAND;
		dwAdd |= TVS_SINGLEEXPAND;
		m_dwStatus |= TREESTATUS_SINGLEEXPAND;
	}
	ModifyStyle(dwRemove, dwAdd);
	strcpy(m_szInstrLibPath, CMainFrame::GetSettings().GetDefaultDirectory(DIR_SAMPLES));
	SetImageList(CMainFrame::GetMainFrame()->GetImageList(), TVSIL_NORMAL);
	if (m_pDataTree)
	{
		// Create Midi Library
		m_hMidiLib = InsertItem("MIDI Library", IMAGE_FOLDER, IMAGE_FOLDER, TVI_ROOT, TVI_LAST);
		for (UINT iMidGrp=0; iMidGrp<17; iMidGrp++)
		{
			m_tiMidiGrp[iMidGrp] = InsertItem(szMidiGroupNames[iMidGrp], IMAGE_FOLDER, IMAGE_FOLDER, m_hMidiLib, TVI_LAST);
		}
	}
	m_hInsLib = InsertItem("Instrument Library", IMAGE_FOLDER, IMAGE_FOLDER, TVI_ROOT, TVI_LAST);
	RefreshMidiLibrary();
	RefreshDlsBanks();
	RefreshInstrumentLibrary();
	m_DropTarget.Register(this);
}


BOOL CModTree::PreTranslateMessage(MSG* pMsg)
//-------------------------------------------
{
	if (!pMsg) return TRUE;
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_SPACE)
		{
			if (!(pMsg->lParam & 0x40000000)) OnPlayTreeItem();
			return TRUE;
		} else
		if (pMsg->wParam == VK_RETURN)
		{
			if (!(pMsg->lParam & 0x40000000))
			{
				HTREEITEM hItem = GetSelectedItem();
				if (hItem)
				{
					if (!ExecuteItem(hItem))
					{
						if (ItemHasChildren(hItem))
						{
							Expand(hItem, TVE_TOGGLE);
						}
					}
				}
			}
			return TRUE;
		} 
	} 
	//rewbs.customKeys
	//We handle keypresses before Windows has a chance to handle them (for alt etc..)
	if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) || 
		(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
	{
		CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();
		
		//Translate message manually
		UINT nChar = pMsg->wParam;
		UINT nRepCnt = LOWORD(pMsg->lParam);
		UINT nFlags = HIWORD(pMsg->lParam);
		KeyEventType kT = ih->GetKeyEventType(nFlags);
		InputTargetContext ctx = (InputTargetContext)(kCtxViewTree);
		
		if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull)
			return true; // Mapped to a command, no need to pass message on.
	}
	//end rewbs.customKeys
	return CTreeCtrl::PreTranslateMessage(pMsg);
}


void CModTree::InsLibGetFullPath(HTREEITEM hItem, LPSTR pszFullPath) const
//------------------------------------------------------------------------
{
	strcpy(pszFullPath, m_szInstrLibPath);
	if ((pszFullPath[0]) && (pszFullPath[strlen(pszFullPath)-1] != '\\')) strcat(pszFullPath, "\\");
	strcat(pszFullPath, GetItemText(hItem));
}


void CModTree::InsLibSetFullPath(LPCSTR pszLibPath, LPCSTR pszSongName)
//---------------------------------------------------------------------
{
	strcpy(m_szInstrLibPath, pszLibPath);
	if ((pszSongName[0]) && (_stricmp(m_szSongName, pszSongName)))
	{
		CMappedFile f;

		SetCurrentDirectory(m_szInstrLibPath);
		if (f.Open(pszSongName))
		{
			DWORD dwLen = f.GetLength();
			if (dwLen)
			{
				LPBYTE lpStream = f.Lock();
				if (lpStream)
				{
					//m_SongFile.Create(lpStream, CMainFrame::GetMainFrame()->GetActiveDoc(), dwLen);
					m_SongFile.Destroy();
					m_SongFile.Create(lpStream, NULL, dwLen);
					f.Unlock();
				}
			}
			f.Close();
		}
	}
	strcpy(m_szSongName, pszSongName);
}


void CModTree::OnOptionsChanged()
//-------------------------------
{
	DWORD dwRemove = TVS_SINGLEEXPAND, dwAdd = 0;
	m_dwStatus &= ~TREESTATUS_SINGLEEXPAND;
	if (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_SINGLEEXPAND)
	{
		dwRemove = 0;
		dwAdd = TVS_SINGLEEXPAND;
		m_dwStatus |= TREESTATUS_SINGLEEXPAND;
	}
	ModifyStyle(dwRemove, dwAdd);
}


void CModTree::AddDocument(CModDoc *pModDoc)
//------------------------------------------
{
	// Check if document is already in the list
	vector<ModTreeDocInfo *>::iterator iter;
	for (iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
	{
		if ((*iter)->pModDoc == pModDoc)
		{
			return;
		}
	}

	ModTreeDocInfo *pInfo = new ModTreeDocInfo(pModDoc->GetSoundFile());
	pInfo->pModDoc = pModDoc;
	pInfo->nSeqSel = SEQUENCEINDEX_INVALID;
	pInfo->nOrdSel = ORDERINDEX_INVALID;
	DocInfo.push_back(pInfo);

	UpdateView(pInfo, HINT_MODTYPE);
	if (pInfo->hSong)
	{
		Expand(pInfo->hSong, TVE_EXPAND);
		EnsureVisible(pInfo->hSong);
		SelectItem(pInfo->hSong);
	}
}


void CModTree::RemoveDocument(CModDoc *pModDoc)
//---------------------------------------------
{
	vector<ModTreeDocInfo *>::iterator iter;
	for (iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
	{
		if((*iter)->pModDoc == pModDoc)
		{
			DeleteItem((*iter)->hSong);
			delete (*iter);
			DocInfo.erase(iter);
			return;
		}
	}
}


// Get CModDoc that is associated with a tree item
CModDoc *CModTree::GetDocumentFromItem(HTREEITEM hItem)
//-----------------------------------------------------
{
	for (int ilimit=0; ilimit<10; ilimit++)
	{
		HTREEITEM h = GetParentItem(hItem);
		if (h == NULL) break;
		hItem = h;
	}
	if (hItem != NULL)
	{
		vector<ModTreeDocInfo *>::iterator iter;
		for (iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
		{
			if (hItem == (*iter)->hSong) return (*iter)->pModDoc;
		}
	}
	return NULL;
}


// Get modtree doc information for a given CModDoc
ModTreeDocInfo *CModTree::GetDocumentInfoFromModDoc(CModDoc *pModDoc)
//-------------------------------------------------------------------
{
	vector<ModTreeDocInfo *>::iterator iter;
	for (iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
	{
		if ((*iter)->pModDoc == pModDoc)
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
	CHAR s[256], stmp[256];
	TV_ITEM tvi;
	CHAR szName[_MAX_FNAME], szExt[_MAX_EXT];
	LPMIDILIBSTRUCT lpMidiLib = CTrackApp::GetMidiLibrary();

	if (!m_pDataTree) return;
	// Midi Programs
	for (UINT iMidi=0; iMidi<128; iMidi++)
	{
		DWORD dwImage = IMAGE_NOINSTRUMENT;
		wsprintf(s, "%d: %s", iMidi, szMidiProgramNames[iMidi]);
		if ((lpMidiLib) && (lpMidiLib->MidiMap[iMidi]) && (lpMidiLib->MidiMap[iMidi][0]))
		{
			_splitpath(lpMidiLib->MidiMap[iMidi], NULL, NULL, szName, szExt);
			strncat(s, ": ", sizeof(s));
			s[sizeof(s)-1] = 0;
			strncat(s, szName, sizeof(s));
			s[sizeof(s)-1] = 0;
			strncat(s, szExt, sizeof(s));
			s[sizeof(s)-1] = 0;
			if (szName[0]) dwImage = IMAGE_INSTRUMENTS;
		}
		if (!m_tiMidi[iMidi])
		{
			m_tiMidi[iMidi] = InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE,
							s, dwImage, dwImage, 0, 0, 0, m_tiMidiGrp[iMidi/8], TVI_LAST);
		} else
		{
			tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE;
			tvi.hItem = m_tiMidi[iMidi];
			tvi.pszText = stmp;
			tvi.cchTextMax = sizeof(stmp);
			tvi.iImage = tvi.iSelectedImage = dwImage;
			GetItem(&tvi);
			if ((strcmp(s, stmp)) || (tvi.iImage != (int)dwImage))
			{
				SetItem(m_tiMidi[iMidi], TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE,
								s, dwImage, dwImage, 0, 0, 0);
			}
		}
	}
	// Midi Percussions
	for (UINT iPerc=24; iPerc<=84; iPerc++)
	{
		DWORD dwImage = IMAGE_NOSAMPLE;
		wsprintf(s, "%s: %s", szDefaultNoteNames[iPerc], szMidiPercussionNames[iPerc-24]);
		if ((lpMidiLib) && (lpMidiLib->MidiMap[iPerc|0x80]) && (lpMidiLib->MidiMap[iPerc|0x80][0]))
		{
			_splitpath(lpMidiLib->MidiMap[iPerc|0x80], NULL, NULL, szName, szExt);
			strncat(s, ": ", sizeof(s));
			s[sizeof(s)-1] = 0;
			strncat(s, szName, sizeof(s));
			s[sizeof(s)-1] = 0;
			strncat(s, szExt, sizeof(s));
			s[sizeof(s)-1] = 0;
			if (szName[0]) dwImage = IMAGE_SAMPLES;
		}
		if (!m_tiPerc[iPerc])
		{
			m_tiPerc[iPerc] = InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE,
							s, dwImage, dwImage, 0, 0, 0, m_tiMidiGrp[16], TVI_LAST);
		} else
		{
			tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE;
			tvi.hItem = m_tiPerc[iPerc];
			tvi.pszText = stmp;
			tvi.cchTextMax = sizeof(stmp);
			tvi.iImage = tvi.iSelectedImage = dwImage;
			GetItem(&tvi);
			if ((strcmp(s, stmp)) || (tvi.iImage != (int)dwImage))
			{
				SetItem(m_tiPerc[iPerc], TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE,
							s, dwImage, dwImage, 0, 0, 0);
			}
		}
	}
}


void CModTree::RefreshDlsBanks()
//------------------------------
{
	CHAR s[256];
	HTREEITEM hDlsRoot = m_hMidiLib;

	if (!m_pDataTree) return;
	for (UINT iDls=0; iDls<MAX_DLS_BANKS; iDls++)
	{
		if (CTrackApp::gpDLSBanks[iDls])
		{
			if (!m_tiDLS[iDls])
			{
				CHAR szName[_MAX_PATH] = "", szExt[_MAX_EXT] = ".dls";
				TV_SORTCB tvs;
				CDLSBank *pDlsBank = CTrackApp::gpDLSBanks[iDls];
				// Add DLS file folder
				_splitpath(pDlsBank->GetFileName(), NULL, NULL, szName, szExt);
				strcat(szName, szExt);
				m_tiDLS[iDls] = InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE,
								szName, IMAGE_FOLDER, IMAGE_FOLDER, 0, 0, 0, TVI_ROOT, hDlsRoot);
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
						wsprintf(szName, "%d: %s", pDlsIns->ulInstrument & 0x7F, pDlsIns->szName);
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
									wsprintf(szName, "%s%d: %s", szNoteNames[keymin % 12], keymin / 12, regionName);
								} else
								{
									wsprintf(szName, "%s%d-%s%d: %s",
										szNoteNames[keymin % 12], keymin / 12,
										szNoteNames[keymax % 12], keymax / 12,
										regionName);
								}
								LPARAM lParam = 0x80000000|(iDls<<24)|(iRgn<<16)|iIns;
								InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM,
										szName, IMAGE_INSTRUMENTS, IMAGE_INSTRUMENTS, 0, 0, lParam, hKit, TVI_LAST);
							}
							tvs.hParent = hKit;
							tvs.lpfnCompare = ModTreeDrumCompareProc;
							tvs.lParam = (LPARAM)this;
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
							LPARAM lParam = 0x40000000|(iDls<<24)|((pDlsIns->ulInstrument & 0x7F)<<16)|(iIns);
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
				HTREEITEM h;
				while ((h = GetChildItem(m_tiDLS[iDls])) != NULL)
				{
					DeleteItem(h);
				}
				DeleteItem(m_tiDLS[iDls]);
				m_tiDLS[iDls] = NULL;
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
		m_pDataTree->InsLibSetFullPath(m_szInstrLibPath, m_szSongName);
		m_pDataTree->RefreshInstrumentLibrary();
	}
}


void CModTree::UpdateView(ModTreeDocInfo *pInfo, DWORD lHint)
//----------------------------------------------------------
{
	CModDoc *pDoc;
	CSoundFile *pSndFile;
	CHAR s[256], stmp[256];
	TV_ITEM tvi;
	const DWORD hintFlagPart = HintFlagPart(lHint);
	if ((pInfo == nullptr) || (pInfo->pModDoc == nullptr) || (!m_pDataTree)) return;
	if (!hintFlagPart) return;
	pDoc = pInfo->pModDoc;
	pSndFile = pDoc->GetSoundFile();
	// Create headers
	s[0] = 0;
	if ((hintFlagPart & (HINT_MODGENERAL|HINT_MODTYPE)) || (!pInfo->hSong))
	{
		_splitpath(pDoc->GetPathName(), NULL, NULL, s, NULL);
		if (!s[0]) strcpy(s, "untitled");
		MemsetZero(tvi);
	}
	if (!pInfo->hSong)
	{
		pInfo->hSong = InsertItem(s, TVI_ROOT, TVI_FIRST);
		pInfo->hOrders = InsertItem("Sequence", IMAGE_FOLDER, IMAGE_FOLDER, pInfo->hSong, TVI_LAST);
		pInfo->hPatterns = InsertItem("Patterns", IMAGE_FOLDER, IMAGE_FOLDER, pInfo->hSong, TVI_LAST);
		pInfo->hSamples = InsertItem("Samples", IMAGE_FOLDER, IMAGE_FOLDER, pInfo->hSong, TVI_LAST);
	}
	if (hintFlagPart & (HINT_MODGENERAL|HINT_MODTYPE))
	{
		tvi.mask |= TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.hItem = pInfo->hSong;
		tvi.pszText = stmp;
		tvi.cchTextMax = sizeof(stmp);
		tvi.iImage = tvi.iSelectedImage = IMAGE_FOLDER;
		GetItem(&tvi);
		if ((strcmp(s, stmp)) || (tvi.iImage != IMAGE_FOLDER))
		{
			tvi.mask |= TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvi.hItem = pInfo->hSong;
			tvi.pszText = s;
			tvi.iImage = tvi.iSelectedImage = IMAGE_FOLDER;
			SetItem(&tvi);
		}
	}
	if (pSndFile->GetType() & (MOD_TYPE_IT|MOD_TYPE_XM|MOD_TYPE_MPT))
	{
		if (!pInfo->hInstruments) pInfo->hInstruments = InsertItem("Instruments", IMAGE_FOLDER, IMAGE_FOLDER, pInfo->hSong, TVI_LAST);
	} else
	{
		if (pInfo->hInstruments)
		{
			for (UINT iRem=MAX_INSTRUMENTS-1; iRem; iRem--) if (pInfo->tiInstruments[iRem])
			{
				DeleteItem(pInfo->tiInstruments[iRem]);
				pInfo->tiInstruments[iRem] = NULL;
			}
			DeleteItem(pInfo->hInstruments);
			pInfo->hInstruments = NULL;
		}
	}
	if (!pInfo->hComments) pInfo->hComments = InsertItem("Comments", IMAGE_COMMENTS, IMAGE_COMMENTS, pInfo->hSong, TVI_LAST);
	// Add effects
	if (hintFlagPart & (HINT_MODGENERAL|HINT_MODTYPE|HINT_MODCHANNELS|HINT_MIXPLUGINS))
	{
		UINT nFx = 0;
		for (UINT iRem=0; iRem<MAX_MIXPLUGINS; iRem++)
		{
			UINT n = MAX_MIXPLUGINS-1-iRem;
			if (pInfo->tiEffects[n])
			{
				DeleteItem(pInfo->tiEffects[n]);
				pInfo->tiEffects[n] = NULL;
			}
		}
		for (UINT iFx=0; iFx<MAX_MIXPLUGINS; iFx++)
		{
			PSNDMIXPLUGIN pPlugin = &pSndFile->m_MixPlugins[iFx];
			if (pPlugin->Info.dwPluginId1)
			{
				if (!pInfo->hEffects)
				{
					pInfo->hEffects = InsertItem("Plugins", IMAGE_FOLDER, IMAGE_FOLDER, pInfo->hSong, TVI_LAST);
				}
				wsprintf(s, "FX%d: %s", iFx+1, pPlugin->Info.szName);
				int nImage = IMAGE_NOPLUGIN;
				if(pPlugin->pMixPlugin != nullptr) nImage = (pPlugin->pMixPlugin->isInstrument()) ? IMAGE_PLUGININSTRUMENT : IMAGE_EFFECTPLUGIN;
				pInfo->tiEffects[iFx] = InsertItem(s, nImage, nImage, pInfo->hEffects, TVI_LAST);
				nFx++;
			}
		}
		if (!nFx)
		{
			if (pInfo->hEffects)
			{
				DeleteItem(pInfo->hEffects);
				pInfo->hEffects = NULL;
			}
		}
	}
	// Add Orders
	if ((pInfo->hOrders) && (hintFlagPart != HINT_INSNAMES) && (hintFlagPart != HINT_SMPNAMES))
	{
		const DWORD nPat = (lHint >> HINT_SHIFT_PAT);
		bool adjustParentNode = false;	// adjust sequence name of "Sequence" node?

		// (only one seq remaining || previously only one sequence): update parent item
		if((pInfo->tiSequences.size() > 1 && pSndFile->Order.GetNumSequences() == 1) || (pInfo->tiSequences.size() == 1 && pSndFile->Order.GetNumSequences() > 1))
		{
			for(size_t nSeq = 0; nSeq < pInfo->tiOrders.size(); nSeq++)
			{
				for(size_t nOrd = 0; nOrd < pInfo->tiOrders[nSeq].size(); nOrd++) if (pInfo->tiOrders[nSeq][nOrd])
				{
					if(pInfo->tiOrders[nSeq][nOrd]) DeleteItem(pInfo->tiOrders[nSeq][nOrd]); pInfo->tiOrders[nSeq][nOrd] = NULL;
				}
				if(pInfo->tiSequences[nSeq]) DeleteItem(pInfo->tiSequences[nSeq]); pInfo->tiSequences[nSeq] = NULL;
			}
			pInfo->tiOrders.resize(pSndFile->Order.GetNumSequences());
			pInfo->tiSequences.resize(pSndFile->Order.GetNumSequences(), NULL);
			adjustParentNode = true;
		}

		// If there are too many sequences, delete them.
		for(size_t nSeq = pSndFile->Order.GetNumSequences(); nSeq < pInfo->tiSequences.size(); nSeq++) if (pInfo->tiSequences[nSeq])
		{
			for(size_t nOrd = 0; nOrd < pInfo->tiOrders[nSeq].size(); nOrd++) if (pInfo->tiOrders[nSeq][nOrd])
			{
				DeleteItem(pInfo->tiOrders[nSeq][nOrd]); pInfo->tiOrders[nSeq][nOrd] = NULL;
			}
			DeleteItem(pInfo->tiSequences[nSeq]); pInfo->tiSequences[nSeq] = NULL;
		}
		if (pInfo->tiSequences.size() < pSndFile->Order.GetNumSequences()) // Resize tiSequences if needed.
		{
			pInfo->tiSequences.resize(pSndFile->Order.GetNumSequences(), NULL);
			pInfo->tiOrders.resize(pSndFile->Order.GetNumSequences());
		}

		HTREEITEM hAncestorNode = pInfo->hOrders;

		SEQUENCEINDEX nSeqMin = 0, nSeqMax = pSndFile->Order.GetNumSequences() - 1;
		SEQUENCEINDEX nHintParam = lHint >> HINT_SHIFT_SEQUENCE;
		if ((hintFlagPart == HINT_SEQNAMES) && (nHintParam <= nSeqMax)) nSeqMin = nSeqMax = nHintParam;

		// Adjust caption of the "Sequence" node (if only one sequence exists, it should be labeled with the sequence name)
		if(((hintFlagPart == HINT_SEQNAMES) && pSndFile->Order.GetNumSequences() == 1) || adjustParentNode)
		{
			CString sSeqName = pSndFile->Order.GetSequence(0).m_sName;
			if(sSeqName.IsEmpty() || pSndFile->Order.GetNumSequences() > 1)
				sSeqName = "Sequence";
			else
				sSeqName = "Sequence: " + sSeqName;
			SetItem(pInfo->hOrders, TVIF_TEXT, sSeqName, 0, 0, 0, 0, 0);
		}

		// go through all sequences
		for(SEQUENCEINDEX nSeq = nSeqMin; nSeq <= nSeqMax; nSeq++)
		{
			if(pSndFile->Order.GetNumSequences() > 1)
			{
				// more than one sequence -> add folder
				CString sSeqName;
				if(pSndFile->Order.GetSequence(nSeq).m_sName.IsEmpty())
					sSeqName.Format("Sequence %d", nSeq);
				else
					sSeqName.Format("%d: %s", nSeq, (LPCTSTR)pSndFile->Order.GetSequence(nSeq).m_sName);

				UINT state = (nSeq == pSndFile->Order.GetCurrentSequenceIndex()) ? TVIS_BOLD : 0;

				if(pInfo->tiSequences[nSeq] == NULL)
				{
					pInfo->tiSequences[nSeq] = InsertItem(sSeqName, IMAGE_FOLDER, IMAGE_FOLDER, pInfo->hOrders, TVI_LAST);
				}
				// Update bold item
				strcpy(stmp, sSeqName);
				tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_STATE;
				tvi.state = 0;
				tvi.stateMask = TVIS_BOLD;
				tvi.hItem = pInfo->tiSequences[nSeq];
				tvi.pszText = stmp;
				tvi.cchTextMax = sizeof(stmp);
				GetItem(&tvi);
				if(tvi.state != state || tvi.pszText != sSeqName)
					SetItem(pInfo->tiSequences[nSeq], TVIF_TEXT | TVIF_STATE, sSeqName, 0, 0, state, TVIS_BOLD, 0);

				hAncestorNode = pInfo->tiSequences[nSeq];
			}

			// If there are items past the new sequence length, delete them.
			for(size_t nOrd = pSndFile->Order.GetSequence(nSeq).GetLengthTailTrimmed(); nOrd < pInfo->tiOrders[nSeq].size(); nOrd++) if (pInfo->tiOrders[nSeq][nOrd])
			{
				DeleteItem(pInfo->tiOrders[nSeq][nOrd]); pInfo->tiOrders[nSeq][nOrd] = NULL;
			}
			if (pInfo->tiOrders[nSeq].size() < pSndFile->Order.GetSequence(nSeq).GetLengthTailTrimmed()) // Resize tiOrders if needed.
				pInfo->tiOrders[nSeq].resize(pSndFile->Order.GetSequence(nSeq).GetLengthTailTrimmed(), NULL);
			const bool patNamesOnly = (hintFlagPart == HINT_PATNAMES);

			//if (hintFlagPart == HINT_PATNAMES) && (dwHintParam < pSndFile->Order.size())) imin = imax = dwHintParam;
			for (ORDERINDEX iOrd = 0; iOrd < pSndFile->Order.GetSequence(nSeq).GetLengthTailTrimmed(); iOrd++)
			{
				if(patNamesOnly && pSndFile->Order.GetSequence(nSeq)[iOrd] != nPat)
					continue;
				UINT state = (iOrd == pInfo->nOrdSel && nSeq == pInfo->nSeqSel) ? TVIS_BOLD : 0;
				if (pSndFile->Order.GetSequence(nSeq)[iOrd] < pSndFile->Patterns.Size())
				{
					stmp[0] = 0;
					pSndFile->Patterns[pSndFile->Order.GetSequence(nSeq)[iOrd]].GetName(stmp, CountOf(stmp));
					if (stmp[0])
					{
						wsprintf(s, (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_HEXDISPLAY) ? "[%02Xh] %d: %s" : "[%02d] %d: %s",
							iOrd, pSndFile->Order.GetSequence(nSeq)[iOrd], stmp);
					} else
					{
						wsprintf(s, (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_HEXDISPLAY) ? "[%02Xh] Pattern %d" : "[%02d] Pattern %d",
							iOrd, pSndFile->Order.GetSequence(nSeq)[iOrd]);
					}
				} else
				{
					if(pSndFile->Order.GetSequence(nSeq)[iOrd] == pSndFile->Order.GetIgnoreIndex())
					{
						// +++ Item
						wsprintf(s, "[%02d] Skip", iOrd);
					} else
					{
						// --- Item
						wsprintf(s, "[%02d] Stop", iOrd);
					}
				}

				if (pInfo->tiOrders[nSeq][iOrd])
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_STATE;
					tvi.state = 0;
					tvi.stateMask = TVIS_BOLD;
					tvi.hItem = pInfo->tiOrders[nSeq][iOrd];
					tvi.pszText = stmp;
					tvi.cchTextMax = sizeof(stmp);
					GetItem(&tvi);
					if ((strcmp(s, stmp)) || (tvi.state != state))
						SetItem(pInfo->tiOrders[nSeq][iOrd], TVIF_TEXT | TVIF_STATE, s, 0, 0, state, TVIS_BOLD, 0);
				} else
				{
					pInfo->tiOrders[nSeq][iOrd] = InsertItem(s, IMAGE_PARTITION, IMAGE_PARTITION, hAncestorNode, TVI_LAST);
				}
			}
		}
	}
	// Add Patterns
	if ((pInfo->hPatterns) && (hintFlagPart != HINT_INSNAMES) && (hintFlagPart != HINT_SMPNAMES))
	{
		const PATTERNINDEX nPat = (PATTERNINDEX)(lHint >> HINT_SHIFT_PAT);
		pInfo->tiPatterns.resize(pSndFile->Patterns.Size(), NULL);
		PATTERNINDEX imin = 0, imax = pSndFile->Patterns.Size()-1;
		if ((hintFlagPart == HINT_PATNAMES) && (nPat < pSndFile->Patterns.Size())) imin = imax = nPat;
		BOOL bDelPat = FALSE;

		ASSERT(pInfo->tiPatterns.size() == pSndFile->Patterns.Size());
		for(PATTERNINDEX iPat = imin; iPat <= imax; iPat++)
		{
			if ((bDelPat) && (pInfo->tiPatterns[iPat]))
			{
				DeleteItem(pInfo->tiPatterns[iPat]);
				pInfo->tiPatterns[iPat] = NULL;
			}
			if (pSndFile->Patterns[iPat])
			{
				stmp[0] = 0;
				pSndFile->Patterns[iPat].GetName(stmp, CountOf(stmp));
				if (stmp[0])
				{
					wsprintf(s, "%d: %s", iPat, stmp);
				} else
				{
					wsprintf(s, "%d", iPat);
				}
				if (pInfo->tiPatterns[iPat])
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE;
					tvi.hItem = pInfo->tiPatterns[iPat];
					tvi.pszText = stmp;
					tvi.cchTextMax = sizeof(stmp);
					GetItem(&tvi);
					if (strcmp(s, stmp)) SetItem(pInfo->tiPatterns[iPat], TVIF_TEXT, s, 0, 0, 0, 0, 0);
				} else
				{
					pInfo->tiPatterns[iPat] = InsertItem(s, IMAGE_PATTERNS, IMAGE_PATTERNS, pInfo->hPatterns, TVI_LAST);
				}
			} else
			{
				if (pInfo->tiPatterns[iPat])
				{
					DeleteItem(pInfo->tiPatterns[iPat]);
					pInfo->tiPatterns[iPat] = NULL;
					bDelPat = TRUE;
				}
			}
		}
	}
	// Add Samples
	if ((pInfo->hSamples) && (hintFlagPart != HINT_INSNAMES) && (hintFlagPart != HINT_PATNAMES))
	{
		const SAMPLEINDEX nSmp = (SAMPLEINDEX)(lHint >> HINT_SHIFT_SMP);
		SAMPLEINDEX smin = 1, smax = MAX_SAMPLES - 1;
		if ((hintFlagPart == HINT_SMPNAMES) && (nSmp) && (nSmp < MAX_SAMPLES)) { smin = smax = nSmp; }
		for(SAMPLEINDEX nSmp = smin; nSmp <= smax; nSmp++)
		{
			if (nSmp <= pSndFile->m_nSamples)
			{
				const bool sampleExists = (pSndFile->GetSample(nSmp).pSample != nullptr);
				int nImage = (sampleExists) ? IMAGE_SAMPLES : IMAGE_NOSAMPLE;
				if(sampleExists && pInfo->samplesPlaying[nSmp]) nImage = IMAGE_SAMPLEACTIVE;
				if(pInfo->pModDoc->IsSampleMuted(nSmp)) nImage = IMAGE_SAMPLEMUTE;

				wsprintf(s, "%3d: %s", nSmp, pSndFile->m_szNames[nSmp]);
				if (!pInfo->tiSamples[nSmp])
				{
					pInfo->tiSamples[nSmp] = InsertItem(s, nImage, nImage, pInfo->hSamples, TVI_LAST);
				} else
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE;
					tvi.hItem = pInfo->tiSamples[nSmp];
					tvi.pszText = stmp;
					tvi.cchTextMax = sizeof(stmp);
					tvi.iImage = tvi.iSelectedImage = nImage;
					GetItem(&tvi);
					if ((strcmp(s, stmp)) || (tvi.iImage != nImage))
					{
						SetItem(pInfo->tiSamples[nSmp], TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE, s, nImage, nImage, 0, 0, 0);
					}
				}
			} else
			{
				if (pInfo->tiSamples[nSmp])
				{
					DeleteItem(pInfo->tiSamples[nSmp]);
					pInfo->tiSamples[nSmp] = NULL;
				}
			}
		}
	}
	// Add Instruments
	if ((pInfo->hInstruments) && (hintFlagPart != HINT_SMPNAMES) && (hintFlagPart != HINT_PATNAMES))
	{
		INSTRUMENTINDEX smin = 1, smax = MAX_INSTRUMENTS - 1;
		const INSTRUMENTINDEX nIns = (INSTRUMENTINDEX)(lHint >> HINT_SHIFT_INS);
		if ((hintFlagPart == HINT_INSNAMES) && (nIns) && (nIns < MAX_INSTRUMENTS))
		{
			smin = smax = nIns;
			if (((pSndFile->Instruments[smin]) && (pInfo->tiInstruments[smin] == NULL))
			 || ((!pSndFile->Instruments[smin]) && (pInfo->tiInstruments[smin] != NULL)))
			{
				smax = MAX_INSTRUMENTS-1;
				for (UINT iRem=smin; iRem<smax; iRem++)
				{
					if (pInfo->tiInstruments[iRem])
					{
						DeleteItem(pInfo->tiInstruments[iRem]);
						pInfo->tiInstruments[iRem] = NULL;
					}
				}
			}
		}
		for (INSTRUMENTINDEX nIns = smin; nIns <= smax; nIns++)
		{
			if ((nIns <= pSndFile->GetNumInstruments()) && (pSndFile->Instruments[nIns]))
			{
				if((pSndFile->m_dwSongFlags & SONG_ITPROJECT) != 0)
				{
					// path info for ITP instruments
					const bool pathOk = pSndFile->m_szInstrumentPath[nIns - 1][0] != '\0';
					const bool instMod = pDoc->m_bsInstrumentModified.test(nIns - 1);
					wsprintf(s, pathOk ? (instMod ? "%3d: * %s" : "%3d: %s") : "%3d: ? %s", nIns, (LPCTSTR)pSndFile->GetInstrumentName(nIns));
				} else
				{
					wsprintf(s, "%3d: %s", nIns, (LPCTSTR)pSndFile->GetInstrumentName(nIns));
				}

				int nImage = IMAGE_INSTRUMENTS;
				if(pInfo->instrumentsPlaying[nIns]) nImage = IMAGE_INSTRACTIVE;
				if(pInfo->pModDoc->IsInstrumentMuted(nIns)) nImage = IMAGE_INSTRMUTE;

				if (!pInfo->tiInstruments[nIns])
				{
					pInfo->tiInstruments[nIns] = InsertItem(s, nImage, nImage, pInfo->hInstruments, TVI_LAST);
				} else
				{
					tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE;
					tvi.hItem = pInfo->tiInstruments[nIns];
					tvi.pszText = stmp;
					tvi.cchTextMax = sizeof(stmp);
					tvi.iImage = tvi.iSelectedImage = nImage;
					GetItem(&tvi);
					if ((strcmp(s, stmp)) || (tvi.iImage != nImage))
						SetItem(pInfo->tiInstruments[nIns], TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE, s, nImage, nImage, 0, 0, 0);
				}
			} else
			{
				if (pInfo->tiInstruments[nIns])
				{
					DeleteItem(pInfo->tiInstruments[nIns]);
					pInfo->tiInstruments[nIns] = NULL;
				}
			}
		}
	}
}


uint64 CModTree::GetModItem(HTREEITEM hItem)
//-----------------------------------------
{
	LPARAM lParam;
	HTREEITEM hItemParent, hItemParentParent, hRootParent;

	if (!hItem) return 0;
	// First, test root items
	if (hItem == m_hInsLib) return MODITEM_HDR_INSTRUMENTLIB;
	if (hItem == m_hMidiLib) return MODITEM_HDR_MIDILIB;
	// Test DLS Banks
	lParam = GetItemData(hItem);
	hItemParent = GetParentItem(hItem);
	hItemParentParent = GetParentItem(hItemParent);
	hRootParent = hItemParent;
	CModDoc *pModDoc = GetDocumentFromItem(hItem);
	CSoundFile *pSndFile = pModDoc ? pModDoc->GetSoundFile() : nullptr;
	if ((hRootParent != NULL) && (m_pDataTree))
	{
		HTREEITEM h;
		for (;;)
		{
			h = GetParentItem(hRootParent);
			if ((!h) || (h == hRootParent)) break;
			hRootParent = h;
		}
	}
	// Midi Library
	if ((hRootParent == m_hMidiLib) && (m_pDataTree))
	{
		for (UINT iGrp=0; iGrp<17; iGrp++)
		{
			if (hItem == m_tiMidiGrp[iGrp]) return (MODITEM_HDR_MIDIGROUP | (iGrp << 16));
		}
		for (UINT iMidi=0; iMidi<128; iMidi++)
		{
			if (hItem == m_tiMidi[iMidi]) return (MODITEM_MIDIINSTRUMENT | (iMidi << 16));
			if (hItem == m_tiPerc[iMidi]) return (MODITEM_MIDIPERCUSSION | (iMidi << 16));
		}
	}
	// Instrument Library
	if ((hRootParent == m_hInsLib) || ((!m_pDataTree) && (hItem != m_hInsLib)))
	{
		CHAR s[256];
		TV_ITEM tvi;

		tvi.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_HANDLE;
		tvi.hItem = hItem;
		tvi.pszText = s;
		tvi.cchTextMax = sizeof(s);
		tvi.iImage = 0;
		if (GetItem(&tvi))
		{
			// Sample ?
			if (tvi.iImage == IMAGE_SAMPLES) return MODITEM_INSLIB_SAMPLE;
			// Instrument ?
			if (tvi.iImage == IMAGE_INSTRUMENTS) return MODITEM_INSLIB_INSTRUMENT;
			// Song ?
			if (tvi.iImage == IMAGE_FOLDERSONG) return MODITEM_INSLIB_SONG;
			return MODITEM_INSLIB_FOLDER;
		}
		return 0;
	}
	if (!m_pDataTree) return 0;
	// Songs
	for (size_t i = 0; i < DocInfo.size(); i++)
	{
		m_nDocNdx = i;
		ModTreeDocInfo *pInfo = DocInfo[i];
		if (hItem == pInfo->hSong) return MODITEM_HDR_SONG;
		if (hRootParent == pInfo->hSong)
		{
			if (hItem == pInfo->hPatterns) return MODITEM_HDR_PATTERNS;
			if (hItem == pInfo->hOrders) return MODITEM_HDR_ORDERS;
			if (hItem == pInfo->hSamples) return MODITEM_HDR_SAMPLES;
			if (hItem == pInfo->hInstruments) return MODITEM_HDR_INSTRUMENTS;
			if (hItem == pInfo->hComments) return MODITEM_COMMENTS;
			// Order List or Sequence item?
			if ((hItemParent == pInfo->hOrders) || (hItemParentParent == pInfo->hOrders))
			{
				// find sequence this item belongs to
				for(SEQUENCEINDEX nSeq = 0; nSeq < pInfo->tiOrders.size(); nSeq++)
				{
					if(hItem == pInfo->tiSequences[nSeq]) return (MODITEM_SEQUENCE | (nSeq << 16));
					for(ORDERINDEX nOrd = 0; nOrd < pInfo->tiOrders[nSeq].size(); nOrd++)
					{
						if (hItem == pInfo->tiOrders[nSeq][nOrd]) return (MODITEM_ORDER | (nOrd << 16) | (((uint64)nSeq) << 32));
					}
				}
			}
			// Pattern ?
			if (hItemParent == pInfo->hPatterns && pSndFile)
			{
				ASSERT(pInfo->tiPatterns.size() == pSndFile->Patterns.Size());
				for (UINT i=0; i<pInfo->tiPatterns.size(); i++)
				{
					if (hItem == pInfo->tiPatterns[i]) return (MODITEM_PATTERN | (i << 16));
				}
			}
			// Sample ?
			if (hItemParent == pInfo->hSamples)
			{
				for (UINT i=0; i<MAX_SAMPLES; i++)
				{
					if (hItem == pInfo->tiSamples[i]) return (MODITEM_SAMPLE | (i << 16));
				}
			}
			// Instrument ?
			if (hItemParent == pInfo->hInstruments)
			{
				for (UINT i=0; i<MAX_INSTRUMENTS; i++)
				{
					if (hItem == pInfo->tiInstruments[i]) return (MODITEM_INSTRUMENT | (i << 16));
				}
			}
			// Effect ?
			if (hItemParent == pInfo->hEffects)
			{
				for (UINT i=0; i<MAX_MIXPLUGINS; i++)
				{
					if (hItem == pInfo->tiEffects[i]) return (MODITEM_EFFECT | (i<<16));
				}
			}
			return 0;
		}
	}
	// Dls Instruments
	for (UINT iDls=0; iDls<MAX_DLS_BANKS; iDls++) if (m_tiDLS[iDls])
	{
		if (hItem == m_tiDLS[iDls])	return (MODITEM_DLSBANK_FOLDER | (iDls << 16));
		if (m_tiDLS[iDls] == hRootParent)
		{
			if ((lParam & 0x3F000000) == (LONG)(iDls << 24))
			{
				if (((lParam & 0xC0000000) == 0x80000000)
				 || ((lParam & 0xC0000000) == 0x40000000))
				{
					return (lParam | 0x8000);
				}
			}
		}
	}
	return MODITEM_NULL;
}


BOOL CModTree::ExecuteItem(HTREEITEM hItem)
//-----------------------------------------
{
	if (hItem)
	{
		const uint64 modItem = GetModItem(hItem);
		const uint32 modItemType = GetModItemType(modItem);
		uint32 modItemID = GetModItemID(modItem);
		ModTreeDocInfo *pInfo = (m_nDocNdx < DocInfo.size() ? DocInfo[m_nDocNdx] : nullptr);
		CModDoc *pModDoc = (pInfo) ? pInfo->pModDoc : NULL;
		switch(modItemType)
		{
		case MODITEM_COMMENTS:
			if (pModDoc) pModDoc->ActivateView(IDD_CONTROL_COMMENTS, 0);
			return TRUE;
		
		/*case MODITEM_SEQUENCE:
			if (pModDoc) pModDoc->ActivateView(IDD_CONTROL_PATTERNS, (dwItem << 16) | 0x8000);
			return TRUE;*/

		case MODITEM_ORDER:
			if (pModDoc) pModDoc->ActivateView(IDD_CONTROL_PATTERNS, modItemID | 0x8000);
			return TRUE;

		case MODITEM_PATTERN:
			if (pModDoc) pModDoc->ActivateView(IDD_CONTROL_PATTERNS, modItemID);
			return TRUE;

		case MODITEM_SAMPLE:
			if (pModDoc) pModDoc->ActivateView(IDD_CONTROL_SAMPLES, modItemID);
			return TRUE;

		case MODITEM_INSTRUMENT:
			if (pModDoc) pModDoc->ActivateView(IDD_CONTROL_INSTRUMENTS, modItemID);
			return TRUE;

		case MODITEM_MIDIPERCUSSION:
			modItemID |= 0x80;
		case MODITEM_MIDIINSTRUMENT:
			OpenMidiInstrument(modItemID);
			return TRUE;

		case MODITEM_EFFECT:
		case MODITEM_INSLIB_SAMPLE:
		case MODITEM_INSLIB_INSTRUMENT:
			PlayItem(hItem);
			return TRUE;

		case MODITEM_INSLIB_SONG:
		case MODITEM_INSLIB_FOLDER:
			InstrumentLibraryChDir(GetItemText(hItem));
			return TRUE;

		case MODITEM_HDR_SONG:
			if (pModDoc) pModDoc->ActivateWindow();
			return TRUE;

		default:
			if (modItemType & 0x8000)
			{
				PlayItem(hItem);
				return TRUE;
			}
		}
	}
	return FALSE;
}


BOOL CModTree::PlayItem(HTREEITEM hItem, UINT nParam)
//---------------------------------------------------
{
	if (hItem)
	{
		const uint64 modItem = GetModItem(hItem);
		const uint32 modItemType = GetModItemType(modItem);
		uint32 modItemID = GetModItemID(modItem);
		ModTreeDocInfo *pInfo = (m_nDocNdx < DocInfo.size() ? DocInfo[m_nDocNdx] : nullptr);
		CModDoc *pModDoc = (pInfo) ? pInfo->pModDoc : NULL;
		switch(modItemType)
		{
		case MODITEM_SAMPLE:
			if (pModDoc)
			{
				if (!nParam) nParam = NOTE_MIDDLEC;
				if (nParam & 0x80)
				{
					pModDoc->NoteOff(nParam & 0x7F, TRUE);
				} else
				{
					pModDoc->NoteOff(0, TRUE); // cut previous playing samples
					pModDoc->PlayNote(nParam & 0x7F, 0, modItemID, FALSE);
				}
			}
			return TRUE;

		case MODITEM_INSTRUMENT:
			if (pModDoc)
			{
				if (!nParam) nParam = NOTE_MIDDLEC;
				if (nParam & 0x80)
				{
					pModDoc->NoteOff(nParam, TRUE);
				} else
				{
					pModDoc->NoteOff(0, TRUE);
					pModDoc->PlayNote(nParam, modItemID, 0, FALSE);
				}
			}
			return TRUE;

		case MODITEM_EFFECT:
			if ((pModDoc) && (modItemID < MAX_MIXPLUGINS))
			{/*
				CSoundFile *pSndFile = pModDoc->GetSoundFile();
				PSNDMIXPLUGIN pPlugin = &pSndFile->m_MixPlugins[dwItem];
				if (pPlugin->pMixPlugin)
				{
					CVstPlugin *pVstPlugin = (CVstPlugin *)pPlugin->pMixPlugin;
					pVstPlugin->ToggleEditor();
				}*/
				pModDoc->TogglePluginEditor(modItemID);
			}
			return TRUE;

		case MODITEM_INSLIB_SAMPLE:
		case MODITEM_INSLIB_INSTRUMENT:
			if (m_szSongName[0])
			{
				CHAR szName[64];
				lstrcpyn(szName, GetItemText(hItem), sizeof(szName));
				UINT n = 0;
				if (szName[0] >= '0') n += (szName[0] - '0');
				if ((szName[1] >= '0') && (szName[1] <= '9')) n = n*10 + (szName[1] - '0');
				if ((szName[2] >= '0') && (szName[2] <= '9')) n = n*10 + (szName[2] - '0');
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				if (pMainFrm)
				{
					if (modItemType == MODITEM_INSLIB_INSTRUMENT)
					{
						pMainFrm->PlaySoundFile(&m_SongFile, n, 0, nParam);
					} else
					{
						pMainFrm->PlaySoundFile(&m_SongFile, 0, n, nParam);
					}
				}
			} else
			{
				CHAR szFullPath[_MAX_PATH] = "";
				InsLibGetFullPath(hItem, szFullPath);
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				if (pMainFrm) pMainFrm->PlaySoundFile(szFullPath, nParam);
			}
			break;

		case MODITEM_MIDIPERCUSSION:
			modItemID |= 0x80;
		case MODITEM_MIDIINSTRUMENT:
			{
				LPMIDILIBSTRUCT lpMidiLib = CTrackApp::GetMidiLibrary();
				if ((lpMidiLib) && (modItemID < 256) && (lpMidiLib->MidiMap[modItemID]))
				{
					CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
					if (pMainFrm) pMainFrm->PlaySoundFile(lpMidiLib->MidiMap[modItemID], nParam);
				}
			}
			break;

		default:
			if (modItemType & 0x8000)
			{
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				UINT bank = (modItem & 0x3F000000) >> 24;
				if ((bank < MAX_DLS_BANKS) && (CTrackApp::gpDLSBanks[bank]) && (pMainFrm))
				{
					CDLSBank *pDLSBank = CTrackApp::gpDLSBanks[bank];
					UINT rgn = 0, instr = (modItem & 0x00007FFF);
					// Drum 
					if (modItem & 0x80000000)
					{
						rgn = (modItem & 0x007F0000) >> 16;
					} else
					// Melodic
					if (modItem & 0x40000000)
					{
						if ((!nParam) || (nParam > NOTE_MAX)) nParam = NOTE_MIDDLEC;
						rgn = pDLSBank->GetRegionFromKey(instr, nParam-1);
					}
					pMainFrm->PlayDLSInstrument(bank, instr, rgn);
				}
			}
		}
	}
	return FALSE;
}


BOOL CModTree::SetMidiInstrument(UINT nIns, LPCTSTR lpszFileName)
//--------------------------------------------------------------
{
	LPMIDILIBSTRUCT lpMidiLib = CTrackApp::GetMidiLibrary();
	if ((lpMidiLib) && (nIns < 128))
	{
		if (!lpMidiLib->MidiMap[nIns])
		{
			if ((lpMidiLib->MidiMap[nIns] = new TCHAR[_MAX_PATH]) == NULL) return FALSE;
		}
		strcpy(lpMidiLib->MidiMap[nIns], lpszFileName);
		RefreshMidiLibrary();
		return TRUE;
	}
	return FALSE;
}


BOOL CModTree::SetMidiPercussion(UINT nPerc, LPCTSTR lpszFileName)
//---------------------------------------------------------------
{
	LPMIDILIBSTRUCT lpMidiLib = CTrackApp::GetMidiLibrary();
	if ((lpMidiLib) && (nPerc < 128))
	{
		UINT nIns = nPerc | 0x80;
		if (!lpMidiLib->MidiMap[nIns])
		{
			if ((lpMidiLib->MidiMap[nIns] = new TCHAR[_MAX_PATH]) == NULL) return FALSE;
		}
		strcpy(lpMidiLib->MidiMap[nIns], lpszFileName);
		RefreshMidiLibrary();
		return TRUE;
	}
	return FALSE;
}


BOOL CModTree::DeleteTreeItem(HTREEITEM hItem)
//--------------------------------------------
{
	const uint64 modItem = GetModItem(hItem);
	const uint32 modItemType = GetModItemType(modItem);
	const uint32 modItemID = GetModItemID(modItem);
	TCHAR s[64];

	ModTreeDocInfo *pInfo = (m_nDocNdx < DocInfo.size() ? DocInfo[m_nDocNdx] : nullptr);
	CModDoc *pModDoc = (pInfo) ? pInfo->pModDoc : nullptr;
	CSoundFile *pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : nullptr;
	switch(modItemType)
	{
	case MODITEM_SEQUENCE:
		if (pModDoc && pSndFile)
		{
			wsprintf(s, _T("Remove sequence %d?"), modItemID);
			if(Reporting::Confirm(s, false, true) == cnfNo) break;
			pSndFile->Order.RemoveSequence((SEQUENCEINDEX)(modItemID));
			pModDoc->UpdateAllViews(NULL, HINT_MODSEQUENCE, NULL);
		}
		break;

	case MODITEM_ORDER:
		// might be slightly annoying to ask for confirmation here, and it's rather easy to restore the orderlist anyway.
		if ((pModDoc) && (pModDoc->RemoveOrder((SEQUENCEINDEX)(modItemID >> 16), (ORDERINDEX)(modItemID & 0xFFFF))))
		{
			pModDoc->UpdateAllViews(NULL, HINT_MODSEQUENCE, NULL);
		}
		break;

	case MODITEM_PATTERN:
		wsprintf(s, _T("Remove pattern %d?"), modItemID);
		if (pModDoc == nullptr || Reporting::Confirm(s, false, true) == cnfNo) break;
		if (pModDoc->RemovePattern((PATTERNINDEX)modItemID))
		{
			pModDoc->UpdateAllViews(NULL, (UINT(modItemID) << HINT_SHIFT_PAT) | HINT_PATTERNDATA|HINT_PATNAMES);
		}
		break;

	case MODITEM_SAMPLE:
		wsprintf(s, _T("Remove sample %d?"), modItemID);
		if (pModDoc == nullptr || Reporting::Confirm(s, false, true) == cnfNo) break;
		pModDoc->GetSampleUndo()->PrepareUndo((SAMPLEINDEX)modItemID, sundo_replace);
		if (pModDoc->RemoveSample((SAMPLEINDEX)modItemID))
		{
			pModDoc->UpdateAllViews(NULL, (UINT(modItemID) << HINT_SHIFT_SMP) | HINT_SMPNAMES|HINT_SAMPLEDATA|HINT_SAMPLEINFO);
		}
		break;

	case MODITEM_INSTRUMENT:
		wsprintf(s, _T("Remove instrument %d?"), modItemID);
		if (pModDoc == nullptr || Reporting::Confirm(s, false, true) == cnfNo) break;
		if (pModDoc->RemoveInstrument((INSTRUMENTINDEX)modItemID))
		{
			pModDoc->UpdateAllViews(NULL, (UINT(modItemID) << HINT_SHIFT_INS) | HINT_MODTYPE|HINT_ENVELOPE|HINT_INSTRUMENT);
		}
		break;

	case MODITEM_MIDIINSTRUMENT:
		SetMidiInstrument(modItemID, "");
		RefreshMidiLibrary();
		break;
	case MODITEM_MIDIPERCUSSION:
		SetMidiPercussion(modItemID, "");
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
			CHAR szFullPath[_MAX_PATH] = "";
			SHFILEOPSTRUCT fos;

			InsLibGetFullPath(hItem, szFullPath);
			memset(&fos, 0, sizeof(fos));
			fos.hwnd = m_hWnd;
			fos.wFunc = FO_DELETE;
			fos.pFrom = szFullPath;
			fos.fFlags = FOF_ALLOWUNDO;
			if ((0 == SHFileOperation(&fos)) && (!fos.fAnyOperationsAborted)) RefreshInstrumentLibrary();
		}
		break;
	}
	return TRUE;
}


BOOL CModTree::OpenTreeItem(HTREEITEM hItem)
//------------------------------------------
{
	const uint64 modItem = GetModItem(hItem);
	const uint32 modItemType = GetModItemType(modItem);
	//const uint32 modItemID = GetModItemID(modItem);

	switch(modItemType)
	{
	case MODITEM_INSLIB_SONG:
		{
			CHAR szFullPath[_MAX_PATH] = "";
			InsLibGetFullPath(hItem, szFullPath);
			theApp.OpenDocumentFile(szFullPath);
		}
		break;
	}
	return TRUE;
}


BOOL CModTree::OpenMidiInstrument(DWORD dwItem)
//---------------------------------------------
{
	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "", "",
		"All Instruments and Banks|*.xi;*.pat;*.iti;*.wav;*.aif;*.aiff;*.sf2;*.sbk;*.dls|"
		"FastTracker II Instruments (*.xi)|*.xi|"
		"GF1 Patches (*.pat)|*.pat|"
		"Wave Files (*.wav)|*.wav|"
		"Impulse Tracker Instruments (*.iti)|*.iti;*.its|"
		"SoundFont 2.0 Banks (*.sf2)|*.sf2;*.sbk|"
		"DLS Sound Banks (*.dls)|*.dls|"
		"All Files (*.*)|*.*||");
	if(files.abort) return FALSE;

	if (dwItem & 0x80)
		return SetMidiPercussion(dwItem & 0x7F, files.first_file.c_str());
	else
		return SetMidiInstrument(dwItem, files.first_file.c_str());
}


// Empty Instrument Library
void CModTree::EmptyInstrumentLibrary()
//-------------------------------------
{
	HTREEITEM h;
	if (!m_hInsLib) return;
	if (m_pDataTree)
	{
		while ((h = GetChildItem(m_hInsLib)) != NULL)
		{
			DeleteItem(h);
		}
	} else
	{
		while ((h = GetNextItem(m_hInsLib, TVGN_NEXT)) != NULL)
		{
			DeleteItem(h);
		}
	}
}


// Refresh Instrument Library
void CModTree::FillInstrumentLibrary()
//------------------------------------
{
	TV_INSERTSTRUCT tvis;
	CHAR s[_MAX_PATH+32], szPath[_MAX_PATH] = "";
	
	if (!m_hInsLib) return;
	SetRedraw(FALSE);
	if ((m_szSongName[0]) && (!m_pDataTree))
	{
		wsprintf(s, "%s", m_szSongName);
		SetItemText(m_hInsLib, s);
		SetItemImage(m_hInsLib, IMAGE_FOLDERSONG, IMAGE_FOLDERSONG);
		for (UINT iIns=1; iIns<=m_SongFile.m_nInstruments; iIns++)
		{
			MODINSTRUMENT *pIns = m_SongFile.Instruments[iIns];
			if (pIns)
			{
				lstrcpyn(szPath, pIns->name, 32);
				wsprintf(s, "%3d: %s", iIns, szPath);
				ModTreeBuildTVIParam(tvis, s, IMAGE_INSTRUMENTS);
				InsertItem(&tvis);
			}
		}
		for (UINT iSmp=1; iSmp<=m_SongFile.m_nSamples; iSmp++)
		{
			const MODSAMPLE &sample = m_SongFile.GetSample(iSmp);
			lstrcpyn(szPath, m_SongFile.m_szNames[iSmp], 32);
			if (sample.pSample)
			{
				wsprintf(s, "%3d: %s", iSmp, szPath);
				ModTreeBuildTVIParam(tvis, s, IMAGE_SAMPLES);
				InsertItem(&tvis);
			}
		}
	} else
	{
		CHAR szFileName[_MAX_PATH];
		WIN32_FIND_DATA wfd;
		HANDLE hFind;

		if (m_szInstrLibPath[0])
			strcpy(szPath, m_szInstrLibPath);
		else
			GetCurrentDirectory(sizeof(szPath), szPath);
		if (m_pDataTree)
		{
			wsprintf(s, "Instrument Library (%s)", szPath);
			strcpy(&s[80], "..."); // Limit text
			SetItemText(m_hInsLib, s);
		} else
		{
			wsprintf(s, "%s", szPath);
			strcpy(&s[80], "..."); // Limit text
			SetItemText(m_hInsLib, s);
			SetItemImage(m_hInsLib, IMAGE_FOLDER, IMAGE_FOLDER);
		}
		// Enumerating Drives...
		if (m_pDataTree)
		{
			strcpy(s, "?:\\");
			for (UINT iDrive='A'; iDrive<='Z'; iDrive++)  //rewbs.fix3112: < became <= 
			{
				s[0] = (CHAR)iDrive;
				UINT nDriveType = GetDriveType(s);
				int nImage = 0;
				switch(nDriveType)
				{
				case DRIVE_REMOVABLE:	nImage = (iDrive < 'C') ? IMAGE_FLOPPYDRIVE : IMAGE_REMOVABLEDRIVE; break;
				case DRIVE_FIXED:		nImage = IMAGE_FIXEDDRIVE; break;
				case DRIVE_REMOTE:		nImage = IMAGE_NETWORKDRIVE; break;
				case DRIVE_CDROM:		nImage = IMAGE_CDROMDRIVE; break;
				case DRIVE_RAMDISK:		nImage = IMAGE_RAMDRIVE; break;
				}
				if (nImage)
				{
					ModTreeBuildTVIParam(tvis, s, nImage);
					InsertItem(&tvis);
				}
			}
		}

		// The path is too long - we can't continue. This can actually only happen with an invalid path
		if(strlen(szPath) >= ARRAYELEMCOUNT(szPath) - 1)
			return;

		// Enumerating Directories and samples/instruments
		if (szPath[strlen(szPath) - 1] != '\\')
			strcat(szPath, "\\");
		strcpy(m_szInstrLibPath, szPath);
		strcat(szPath, "*.*");
		memset(&wfd, 0, sizeof(wfd));
		if ((hFind = FindFirstFile(szPath, &wfd)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				// Up Directory
				if (!strcmp(wfd.cFileName, ".."))
				{
					if (m_pDataTree)
					{
						ModTreeBuildTVIParam(tvis, wfd.cFileName, IMAGE_FOLDERPARENT);
						InsertItem(&tvis);
					}
				} else
				// Ignore these files
				if (wfd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|
											FILE_ATTRIBUTE_OFFLINE|
											FILE_ATTRIBUTE_TEMPORARY|
											FILE_ATTRIBUTE_SYSTEM
					))
				{
					// skip it
				} else
				// Directory
				if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if ((strcmp(wfd.cFileName, ".")) && (m_pDataTree))
					{
						ModTreeBuildTVIParam(tvis, wfd.cFileName, IMAGE_FOLDER);
						InsertItem(&tvis);
					}
				} else
				if ((!wfd.nFileSizeHigh) && (wfd.nFileSizeLow >= 16))
				{
					strcpy(szFileName, m_szInstrLibPath);
					strncat(szFileName, wfd.cFileName, sizeof(szFileName));
					_splitpath(szFileName, NULL, NULL, NULL, s);
					// Instruments
					if ((!lstrcmpi(s, ".xi"))
					 || (!lstrcmpi(s, ".iti"))
					 || (!lstrcmpi(s, ".pat")) )
					{
						if (!m_pDataTree)
						{
							ModTreeBuildTVIParam(tvis, wfd.cFileName, IMAGE_INSTRUMENTS);
							InsertItem(&tvis);
						}
					} else
					// Songs
					if ((!lstrcmpi(s, ".mod"))
					 || (!lstrcmpi(s, ".s3m"))
					 || (!lstrcmpi(s, ".xm"))
					 || (!lstrcmpi(s, ".it"))
					 || (!lstrcmpi(s, ".mptm"))
// -> CODE#0023
// -> DESC="IT project files (.itp)"
//					 || (!lstrcmpi(s, ".itp"))	ericus 03/03/2005 : temporarily deactivated 03/03/2005
// -! NEW_FEATURE#0023
					 || ((m_bShowAllFiles)
					  && ((!lstrcmpi(s, ".mdz"))
					  || (!lstrcmpi(s, ".s3z"))
					  || (!lstrcmpi(s, ".xmz"))
					  || (!lstrcmpi(s, ".itz"))
					  || (!lstrcmpi(s, ".669"))
					  || (!lstrcmpi(s, ".ams"))
					  || (!lstrcmpi(s, ".amf"))
					  || (!lstrcmpi(s, ".mdz"))
					  || (!lstrcmpi(s, ".dsm"))
					  || (!lstrcmpi(s, ".far"))
					  || (!lstrcmpi(s, ".mdl"))
					  || (!lstrcmpi(s, ".mtm"))
					  || (!lstrcmpi(s, ".nst"))
					  || (!lstrcmpi(s, ".okt"))
					  || (!lstrcmpi(s, ".stm"))
					  || (!lstrcmpi(s, ".ult"))
					  || (!lstrcmpi(s, ".psm"))
					  || (!lstrcmpi(s, ".dmf"))
					  || (!lstrcmpi(s, ".mt2"))
					  || (!lstrcmpi(s, ".med"))
					  || (!lstrcmpi(s, ".wow"))
					  || (!lstrcmpi(s, ".gdm"))
					  || (!lstrcmpi(s, ".imf"))
					  || (!lstrcmpi(s, ".j2b"))
#ifndef NO_MO3_SUPPORT
					  || (!lstrcmpi(s, ".mo3"))
#endif
					  )))
					{
						if (m_pDataTree)
						{
							ModTreeBuildTVIParam(tvis, wfd.cFileName, IMAGE_FOLDERSONG);
							InsertItem(&tvis);
						}
					} else
					// Samples
					if ((!lstrcmpi(s, ".wav"))
					 || (!lstrcmpi(s, ".smp"))
					 || (!lstrcmpi(s, ".raw"))
					 || (!lstrcmpi(s, ".s3i"))
					 || (!lstrcmpi(s, ".its"))
					 || (!lstrcmpi(s, ".aif"))
					 || (!lstrcmpi(s, ".aiff"))
					 || (!lstrcmpi(s, ".snd"))
					 || (!lstrcmpi(s, ".svx"))
					 || (!lstrcmpi(s, ".voc"))
					 || (!lstrcmpi(s, ".8sv"))
					 || (!lstrcmpi(s, ".8svx"))
					 || (!lstrcmpi(s, ".16sv"))
					 || (!lstrcmpi(s, ".16svx"))
					 || ((m_bShowAllFiles) // Exclude the extensions below
					  && (lstrcmpi(s, ".txt"))
					  && (lstrcmpi(s, ".diz"))
					  && (lstrcmpi(s, ".nfo"))
					  && (lstrcmpi(s, ".doc"))
					  && (lstrcmpi(s, ".ini"))
					  && (lstrcmpi(s, ".pdf"))
					  && (lstrcmpi(s, ".zip"))
					  && (lstrcmpi(s, ".rar"))
					  && (lstrcmpi(s, ".lha"))
					  && (lstrcmpi(s, ".exe"))
					  && (lstrcmpi(s, ".dll"))
					  && (lstrcmpi(s, ".mol")))
					)
					{
						if (!m_pDataTree)
						{
							ModTreeBuildTVIParam(tvis, wfd.cFileName, IMAGE_SAMPLES);
							InsertItem(&tvis);
						}
					}
				}
			} while (FindNextFile(hFind, &wfd));
			FindClose(hFind);
		}
	}
	// Sort items
	TV_SORTCB tvs;
	tvs.hParent = (m_pDataTree) ? m_hInsLib : TVI_ROOT;
    tvs.lpfnCompare = ModTreeInsLibCompareProc;
	tvs.lParam = (LPARAM)this;
	SortChildren(tvs.hParent);
	SortChildrenCB(&tvs);
	SetRedraw(TRUE);
}


void CModTree::ModTreeBuildTVIParam(TV_INSERTSTRUCT &tvis, LPCSTR lpszName, int iImage)
//-------------------------------------------------------------------------------------
{
	DWORD dwId = 0;
	switch(iImage)
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
		if (m_szSongName[0]) { dwId = 5; break; }
	case IMAGE_INSTRUMENTS:
		dwId = 4;
		break;
	}
	tvis.hParent = (m_pDataTree) ? m_hInsLib : TVI_ROOT;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	tvis.item.hItem = 0;
	tvis.item.state = 0;
	tvis.item.stateMask = 0;
	tvis.item.pszText = (LPSTR)lpszName;
	tvis.item.cchTextMax = 0;
	tvis.item.iImage = iImage;
	tvis.item.iSelectedImage = iImage;
	tvis.item.cChildren = 0;
	tvis.item.lParam = (LPARAM)dwId;
}


int CALLBACK CModTree::ModTreeInsLibCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM)
//-------------------------------------------------------------------------------------
{
	lParam1 &= 0x7FFFFFFF;
	lParam2 &= 0x7FFFFFFF;
	return lParam1 - lParam2;
}


int CALLBACK CModTree::ModTreeDrumCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM)
//-----------------------------------------------------------------------------------
{
	lParam1 &= 0x7FFFFFFF;
	lParam2 &= 0x7FFFFFFF;
	if ((lParam1 & 0xFF00FFFF) == (lParam2 & 0xFF00FFFF))
	{
		UINT iDls = (lParam1 >> 24) & 0xFF;
		if ((iDls < MAX_DLS_BANKS) && (CTrackApp::gpDLSBanks[iDls]))
		{
			CDLSBank *pDLSBank = CTrackApp::gpDLSBanks[iDls];
			DLSINSTRUMENT *pDlsIns = pDLSBank->GetInstrument(lParam1 & 0xFFFF);
			lParam1 = (lParam1 >> 16) & 0xFF;
			lParam2 = (lParam2 >> 16) & 0xFF;
			if ((pDlsIns) && (lParam1 < (LONG)pDlsIns->nRegions) && (lParam2 < (LONG)pDlsIns->nRegions))
			{
				lParam1 = pDlsIns->Regions[lParam1].uKeyMin;
				lParam2 = pDlsIns->Regions[lParam2].uKeyMin;
			}
		}
	}
	return lParam1 - lParam2;
}


BOOL CModTree::InstrumentLibraryChDir(LPCSTR lpszDir)
//---------------------------------------------------
{
	CHAR s[_MAX_PATH+80], sdrive[_MAX_DRIVE];
	BOOL bOk = FALSE, bSong = FALSE;
	
	if ((!lpszDir) || (!lpszDir[0])) return FALSE;
	BeginWaitCursor();
	if (!GetCurrentDirectory(sizeof(s), s)) s[0] = 0;
	if (!strcmp(lpszDir+1, ":\\"))
	{
		sdrive[0] = lpszDir[0];
		sdrive[1] = lpszDir[1];
		sdrive[2] = 0;
		lpszDir = sdrive;
	}
	SetCurrentDirectory(m_szInstrLibPath);
	if (!lstrcmpi(lpszDir, ".."))
	{
		UINT k = strlen(m_szInstrLibPath);
		if ((k > 0) && (m_szInstrLibPath[k-1] == '\\')) k--;
		while ((k > 0) && (m_szInstrLibPath[k-1] != '\\')) k--;
		m_szOldPath[0] = 0;
		if (k > 0)
		{
			strcpy(m_szOldPath, &m_szInstrLibPath[k]);
			k = strlen(m_szOldPath);
			if ((k > 0) && (m_szOldPath[k-1] == '\\')) m_szOldPath[k-1] = 0;
		}
	} else
	{
		CFileStatus status;
		if ((CFile::GetStatus(lpszDir, status)) && (!(status.m_attribute & CFile::directory)))
		{
			bSong = TRUE;
		}
		strcpy(m_szOldPath, "..");
	}
	if (bSong)
	{
		GetCurrentDirectory(sizeof(m_szInstrLibPath), m_szInstrLibPath);
		strcpy(m_szSongName, lpszDir);
		if (m_pDataTree)
		{
			m_pDataTree->InsLibSetFullPath(m_szInstrLibPath, m_szSongName);
			m_pDataTree->RefreshInstrumentLibrary();
		} else
		{
			PostMessage(WM_COMMAND, ID_MODTREE_REFRESHINSTRLIB);
		}
		bOk = TRUE;
	} else
	{
		if (SetCurrentDirectory(lpszDir))
		{
			m_szSongName[0] = 0;
			if (m_SongFile.m_nType) m_SongFile.Destroy();
			GetCurrentDirectory(sizeof(m_szInstrLibPath), m_szInstrLibPath);
			PostMessage(WM_COMMAND, ID_MODTREE_REFRESHINSTRLIB);
			bOk = TRUE;
		}
	}
	if (s[0]) SetCurrentDirectory(s);
	EndWaitCursor();
	if (!bOk)
	{
		wsprintf(s, "Unable to browse to \"%s\"", lpszDir);
		Reporting::Error(s);
	}
	return TRUE;
}


BOOL CModTree::GetDropInfo(LPDRAGONDROP pdropinfo, LPSTR pszFullPath)
//-------------------------------------------------------------------
{
	ModTreeDocInfo *pInfo = (m_nDragDocNdx < DocInfo.size() ? DocInfo[m_nDragDocNdx] : nullptr);
	pdropinfo->pModDoc = (pInfo) ? pInfo->pModDoc : nullptr;
	pdropinfo->dwDropType = DRAGONDROP_NOTHING;
	pdropinfo->dwDropItem = GetModItemID(m_qwItemDrag);
	pdropinfo->lDropParam = 0;
	switch(GetModItemType(m_qwItemDrag))
	{
	case MODITEM_ORDER:
		pdropinfo->dwDropType = DRAGONDROP_ORDER;
		break;

	case MODITEM_PATTERN:
		pdropinfo->dwDropType = DRAGONDROP_PATTERN;
		break;
	
	case MODITEM_SAMPLE:
		pdropinfo->dwDropType = DRAGONDROP_SAMPLE;
		break;

	case MODITEM_INSTRUMENT:
		pdropinfo->dwDropType = DRAGONDROP_INSTRUMENT;
		break;

	case MODITEM_SEQUENCE:
	case MODITEM_HDR_ORDERS:
		pdropinfo->dwDropType = DRAGONDROP_SEQUENCE;
		break;

	case MODITEM_INSLIB_SAMPLE:
	case MODITEM_INSLIB_INSTRUMENT:
		if (m_szSongName[0])
		{
			CHAR s[32];
			lstrcpyn(s, GetItemText(m_hItemDrag), CountOf(s));
			UINT n = 0;
			if (s[0] >= '0') n += (s[0] - '0');
			if ((s[1] >= '0') && (s[1] <= '9')) n = n*10 + (s[1] - '0');
			if ((s[2] >= '0') && (s[2] <= '9'))  n = n*10 + (s[2] - '0');
			pdropinfo->dwDropType = ((m_qwItemDrag & 0xFFFF) == MODITEM_INSLIB_SAMPLE) ? DRAGONDROP_SAMPLE : DRAGONDROP_INSTRUMENT;
			pdropinfo->dwDropItem = n;
			pdropinfo->pModDoc = nullptr;
			pdropinfo->lDropParam = (LPARAM)&m_SongFile;
		} else
		{
			InsLibGetFullPath(m_hItemDrag, pszFullPath);
			pdropinfo->dwDropType = DRAGONDROP_SOUNDFILE;
			pdropinfo->lDropParam = (LPARAM)pszFullPath;
		}
		break;

	case MODITEM_MIDIPERCUSSION:
		pdropinfo->dwDropItem |= 0x80;
	case MODITEM_MIDIINSTRUMENT:
		{
			LPMIDILIBSTRUCT lpMidiLib = CTrackApp::GetMidiLibrary();
			if ((lpMidiLib) && (lpMidiLib->MidiMap[pdropinfo->dwDropItem&0xFF]))
			{
				strcpy(pszFullPath, lpMidiLib->MidiMap[pdropinfo->dwDropItem&0xFF]);
				pdropinfo->dwDropType = DRAGONDROP_MIDIINSTR;
				pdropinfo->lDropParam = (LPARAM)pszFullPath;
			}
		}
		break;

	case MODITEM_INSLIB_SONG:
		InsLibGetFullPath(m_hItemDrag, pszFullPath);
		pdropinfo->pModDoc = NULL;
		pdropinfo->dwDropType = DRAGONDROP_SONG;
		pdropinfo->dwDropItem = 0;
		pdropinfo->lDropParam = (LPARAM)pszFullPath;
		break;

	default:
		if (m_qwItemDrag & 0xC0000000)
		{
			pdropinfo->dwDropType = DRAGONDROP_DLS;
			// dwDropItem = DLS Bank #
			pdropinfo->dwDropItem = (DWORD)((m_qwItemDrag & 0x3F000000) >> 24);	// bank #
			// Melodic: (Instrument)
			// Drums:	(0x80000000) | (Region << 16) | (Instrument)
			pdropinfo->lDropParam = (LPARAM)((m_qwItemDrag & 0x80FF7FFF)); // 
			break;
		}
	}
	return (pdropinfo->dwDropType != DRAGONDROP_NOTHING);
}


bool CModTree::CanDrop(HTREEITEM hItem, bool bDoDrop)
//---------------------------------------------------
{
	const uint64 modItemDrop = GetModItem(hItem);
	const uint32 modItemDropType = GetModItemType(modItemDrop);
	const uint32 modItemDropID = GetModItemID(modItemDrop);

	const uint32 modItemDragType = GetModItemType(m_qwItemDrag);
	const uint32 modItemDragID = GetModItemID(m_qwItemDrag);

	const ModTreeDocInfo *pInfoDrag = (m_nDragDocNdx < DocInfo.size() ? DocInfo[m_nDragDocNdx] : nullptr);
	const ModTreeDocInfo *pInfoDrop = (m_nDocNdx < DocInfo.size() ? DocInfo[m_nDocNdx] : nullptr);
	CModDoc *pModDoc = (pInfoDrop) ? pInfoDrop->pModDoc : nullptr;
	CSoundFile *pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : nullptr;

	switch(modItemDropType)
	{
	case MODITEM_ORDER:
	case MODITEM_SEQUENCE:
		if ((modItemDragType == MODITEM_ORDER) && (pModDoc) && (pSndFile) && (m_nDocNdx == m_nDragDocNdx))
		{
			// drop an order somewhere
			if (bDoDrop)
			{
				SEQUENCEINDEX nSeqFrom = (SEQUENCEINDEX)(modItemDragID >> 16), nSeqTo = (SEQUENCEINDEX)(modItemDropID >> 16);
				ORDERINDEX nOrdFrom = (ORDERINDEX)(modItemDragID & 0xFFFF), nOrdTo = (ORDERINDEX)(modItemDropID & 0xFFFF);
				if(modItemDropType == MODITEM_SEQUENCE)
				{
					// drop on sequence -> attach
					nSeqTo = (SEQUENCEINDEX)(modItemDropID & 0xFFFF);
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
		if((modItemDragType == MODITEM_SEQUENCE || modItemDragType == MODITEM_HDR_ORDERS) && pSndFile && pInfoDrag && pModDoc != pInfoDrag->pModDoc)
		{
			if(bDoDrop && pInfoDrag != nullptr)
			{
				// copy mod sequence over.
				CModDoc *pDragDoc = pInfoDrag->pModDoc;
				if(pDragDoc == nullptr) return false;
				CSoundFile *pDragSndFile = pDragDoc->GetSoundFile();
				if(pDragSndFile == nullptr) return false;
				const SEQUENCEINDEX nOrigSeq = (SEQUENCEINDEX)modItemDragID;
				const ModSequence *pOrigSeq = &(pDragSndFile->Order.GetSequence(nOrigSeq));
				if(pOrigSeq == nullptr) return false;

				if(pSndFile->GetType() == MOD_TYPE_MPT)
				{
					pSndFile->Order.AddSequence(false);
				}
				else
				{
					if(Reporting::Confirm(_T("Replace the current orderlist?"), _T("Sequence import")) == cnfNo)
						return false;
				}
				pSndFile->Order.resize(min(pSndFile->GetModSpecifications().ordersMax, pOrigSeq->GetLength()), pSndFile->Order.GetInvalidPatIndex());
				for(ORDERINDEX nOrd = 0; nOrd < min(pSndFile->GetModSpecifications().ordersMax,pOrigSeq->GetLengthTailTrimmed()); nOrd++)
				{
					PATTERNINDEX nOrigPat = pDragSndFile->Order.GetSequence(nOrigSeq)[nOrd];
					// translate pattern index
					if(nOrigPat == pDragSndFile->Order.GetInvalidPatIndex())
						pSndFile->Order[nOrd] = pSndFile->Order.GetInvalidPatIndex();
					else if(nOrigPat == pDragSndFile->Order.GetIgnoreIndex() && pSndFile->GetModSpecifications().hasIgnoreIndex)
						pSndFile->Order[nOrd] = pSndFile->Order.GetIgnoreIndex();
					else if(nOrigPat == pDragSndFile->Order.GetIgnoreIndex() && !pSndFile->GetModSpecifications().hasIgnoreIndex)
						pSndFile->Order[nOrd] = pSndFile->Order.GetInvalidPatIndex();
					else if(nOrigPat >= pSndFile->GetModSpecifications().patternsMax)
						pSndFile->Order[nOrd] = pSndFile->Order.GetInvalidPatIndex();
					else
						pSndFile->Order[nOrd] = nOrigPat;
				}
				pModDoc->UpdateAllViews(NULL, HINT_MODSEQUENCE, NULL);
				pModDoc->SetModified();
			}
			return true;
		}
		break;

	case MODITEM_SAMPLE:
		break;

	case MODITEM_INSTRUMENT:
		break;

	case MODITEM_MIDIINSTRUMENT:
	case MODITEM_MIDIPERCUSSION:
		if ((modItemDragType == MODITEM_INSLIB_SAMPLE) || (modItemDragType == MODITEM_INSLIB_INSTRUMENT))
		{
			if (bDoDrop)
			{
				CHAR szFullPath[_MAX_PATH] = "";
				InsLibGetFullPath(m_hItemDrag, szFullPath);
				if (modItemDropType == MODITEM_MIDIINSTRUMENT)
					SetMidiInstrument((DWORD)modItemDropID, szFullPath);
				else
					SetMidiPercussion((DWORD)modItemDropID, szFullPath);
			}
			return true;
		}
		break;
	}
	return false;
}


void CModTree::UpdatePlayPos(CModDoc *pModDoc, PMPTNOTIFICATION pNotify)
//----------------------------------------------------------------------
{
	ModTreeDocInfo *pInfo = GetDocumentInfoFromModDoc(pModDoc);
	if(pInfo == nullptr) return;

	ORDERINDEX nNewOrd = (pNotify) ? pNotify->nOrder : ORDERINDEX_INVALID;
	SEQUENCEINDEX nNewSeq = (pModDoc->GetSoundFile() != nullptr) ? pModDoc->GetSoundFile()->Order.GetCurrentSequenceIndex() : SEQUENCEINDEX_INVALID;
	if (nNewOrd != pInfo->nOrdSel || nNewSeq != pInfo->nSeqSel)
	{
		pInfo->nOrdSel = nNewOrd;
		pInfo->nSeqSel = nNewSeq;
		UpdateView(pInfo, HINT_MODSEQUENCE);
	}

	// Update sample / instrument playing status icons (will only detect instruments with samples, though)

	if((CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_LIVEUPDATETREE) == 0) return;
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

	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return;

	for(CHANNELINDEX nChn = 0; nChn < MAX_CHANNELS; nChn++)
	{
		if(pSndFile->Chn[nChn].pCurrentSample != nullptr)
		{
			if(updateSamples)
			{
				for(SAMPLEINDEX nSmp = pSndFile->GetNumSamples(); nSmp >= 1; nSmp--)
				{
					if(pSndFile->Chn[nChn].pModSample == &pSndFile->GetSample(nSmp))
					{
						pInfo->samplesPlaying.set(nSmp);
						break;
					}
				}
			}
			if(updateInstruments)
			{
				for(INSTRUMENTINDEX nIns = pSndFile->GetNumInstruments(); nIns >= 1; nIns--)
				{
					if(pSndFile->Chn[nChn].pModInstrument == pSndFile->Instruments[nIns])
					{
						pInfo->instrumentsPlaying.set(nIns);
						break;
					}
				}
			}
		}
	}
	// what should be updated?
	DWORD dwHintFlags = (updateSamples ? HINT_SAMPLEINFO : 0) | (updateInstruments ? HINT_INSTRUMENT : 0);
	if(dwHintFlags != 0) UpdateView(pInfo, dwHintFlags);
}



/////////////////////////////////////////////////////////////////////////////
// CViewModTree message handlers


void CModTree::OnUpdate(CModDoc *pModDoc, DWORD dwHint, CObject *pHint)
//---------------------------------------------------------------------
{
	dwHint &= (HINT_PATNAMES|HINT_SMPNAMES|HINT_INSNAMES|HINT_MODTYPE|HINT_MODGENERAL|HINT_MODSEQUENCE|HINT_MIXPLUGINS|HINT_MPTOPTIONS|HINT_MASK_ITEM|HINT_SEQNAMES);
	if ((pHint != this) && (dwHint & HINT_MASK_FLAGS))
	{
		vector<ModTreeDocInfo *>::iterator iter;
		for (iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
		{
			if (((*iter)->pModDoc == pModDoc) || (!pModDoc))
			{
				UpdateView((*iter), dwHint);
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
		UINT iNewImage = (pnm->itemNew.state & TVIS_EXPANDED) ? IMAGE_OPENFOLDER : IMAGE_FOLDER;
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
		m_qwItemDrag = GetModItem(m_hItemDrag);
		m_nDragDocNdx = m_nDocNdx;
		switch(GetModItemType(m_qwItemDrag))
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
				CModDoc *pModDoc = (m_nDragDocNdx < DocInfo.size() ? DocInfo[m_nDragDocNdx]->pModDoc : nullptr);
				CSoundFile *pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : nullptr;
				if(pSndFile && pSndFile->Order.GetNumSequences() == 1)
					bDrag = true;
			}
			break;
		default:
			if (m_qwItemDrag & 0x8000) bDrag = true;
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
	UINT flags;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	HTREEITEM hItem = GetSelectedItem();
	if ((hItem) && (hItem == HitTest(pt, &flags)))
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
			UINT nDefault = 0;
			BOOL bSep = FALSE;

			const uint64 modItem = GetModItem(hItem);
			const uint32 modItemType = GetModItemType(modItem);
			const uint32 modItemID = GetModItemID(modItem);

			SelectItem(hItem);
			switch(modItemType)
			{
			case MODITEM_HDR_SONG:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, "&View");
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_CLOSE, "&Close");
				break;

			case MODITEM_COMMENTS:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, "&View Comments");
				break;
			
			case MODITEM_ORDER:
			case MODITEM_PATTERN:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, "&Edit Pattern");
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE,
								(modItemType == MODITEM_ORDER) ? "&Delete from list" : "&Delete Pattern");
				break;

			case MODITEM_SEQUENCE:
				{
					bool isCurSeq = false;
					CModDoc *pModDoc = GetDocumentFromItem(hItem);
					CSoundFile *pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : nullptr;
					if(pModDoc && pSndFile && (pModDoc->GetModType() == MOD_TYPE_MPT))
					{
						if(pSndFile->Order.GetSequence((SEQUENCEINDEX)modItemID).GetLength() == 0)
						{
							nDefault = ID_MODTREE_SWITCHTO;
						}
						isCurSeq = (pSndFile->Order.GetCurrentSequenceIndex() == (SEQUENCEINDEX)modItemID);
					}

					if(!isCurSeq)
					{
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_SWITCHTO, "&Switch to Seqeuence");
					}
					AppendMenu(hMenu, MF_STRING | (pSndFile->Order.GetNumSequences() < MAX_SEQUENCES ? 0 : MF_GRAYED), ID_MODTREE_INSERT, "&Insert Sequence");
					AppendMenu(hMenu, MF_STRING | (pSndFile->Order.GetNumSequences() < MAX_SEQUENCES ? 0 : MF_GRAYED), ID_MODTREE_DUPLICATE , "D&uplicate Sequence");
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, "&Delete Sequence");
				}
				break;


			case MODITEM_HDR_ORDERS:
				{
					CModDoc *pModDoc = GetDocumentFromItem(hItem);
					CSoundFile *pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : nullptr;
					if(pModDoc && pSndFile && (pModDoc->GetModType() == MOD_TYPE_MPT))
					{
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_INSERT, "&Insert Sequence");
						if(pSndFile->Order.GetNumSequences() == 1) // this is a sequence
							AppendMenu(hMenu, MF_STRING, ID_MODTREE_DUPLICATE, "D&uplicate Sequence");
					}
				}
				break;

			case MODITEM_SAMPLE:
				{
					CModDoc *pModDoc = GetDocumentFromItem(hItem);
					nDefault = ID_MODTREE_EXECUTE;
					AppendMenu(hMenu, MF_STRING, nDefault, "&View Sample");
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, "&Play Sample");
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, "&Delete Sample");
					if ((pModDoc) && (!pModDoc->GetNumInstruments()))
					{
						AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
						AppendMenu(hMenu, (pModDoc->IsSampleMuted((SAMPLEINDEX)modItemID) ? MF_CHECKED:0)|MF_STRING, ID_MODTREE_MUTE, "&Mute Sample");
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_SOLO, "&Solo Sample");
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_UNMUTEALL, "&Unmute all");
					}
				}
				break;

			case MODITEM_INSTRUMENT:
				{
					CModDoc *pModDoc = GetDocumentFromItem(hItem);
					nDefault = ID_MODTREE_EXECUTE;
					AppendMenu(hMenu, MF_STRING, nDefault, "&View Instrument");
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, "&Play Instrument");
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, "&Delete Instrument");
					if ((pModDoc) && (pModDoc->GetNumInstruments()))
					{
						AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
						AppendMenu(hMenu, (pModDoc->IsInstrumentMuted((INSTRUMENTINDEX)modItemID) ? MF_CHECKED:0)|MF_STRING, ID_MODTREE_MUTE, "&Mute Instrument");
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_SOLO, "&Solo Instrument");
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_UNMUTEALL, "&Unmute all");
// -> CODE#0023
// -> DESC="IT project files (.itp)"
						AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_SETPATH, "&Set path");
						AppendMenu(hMenu, MF_STRING, ID_MODTREE_SAVEITEM, "&Save");
// -! NEW_FEATURE#0023
					}
				}
				break;

			case MODITEM_EFFECT:
				{
					nDefault = ID_MODTREE_EXECUTE;
					AppendMenu(hMenu, MF_STRING, nDefault, "&Edit");

					CModDoc *pModDoc = GetDocumentFromItem(hItem);
					CSoundFile *pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;
					if (pSndFile) {
						PSNDMIXPLUGIN pPlugin = &pSndFile->m_MixPlugins[modItemID];
						if (pPlugin) {
							bool bypassed = ((pPlugin->Info.dwInputRouting&MIXPLUG_INPUTF_BYPASS) != 0);
							AppendMenu(hMenu, (bypassed?MF_CHECKED:0)|MF_STRING, ID_MODTREE_MUTE, "&Bypass");
						}
					}
				}
				break;

			case MODITEM_MIDIINSTRUMENT:
			case MODITEM_MIDIPERCUSSION:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, "&Map Instrument");
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, "&Play Instrument");
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, "&Unmap Instrument");
				AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
			case MODITEM_HDR_MIDILIB:
			case MODITEM_HDR_MIDIGROUP:
				AppendMenu(hMenu, MF_STRING, ID_IMPORT_MIDILIB, "&Import MIDI Library");
				AppendMenu(hMenu, MF_STRING, ID_EXPORT_MIDILIB, "E&xport MIDI Library");
				bSep = TRUE;
				break;

			case MODITEM_INSLIB_FOLDER:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, "&Browse...");
				break;

			case MODITEM_INSLIB_SONG:
				nDefault = ID_MODTREE_EXECUTE;
				AppendMenu(hMenu, MF_STRING, nDefault, "&Browse Song...");
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_OPENITEM, "&Edit Song");
				AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, "&Delete");
				break;

			case MODITEM_INSLIB_SAMPLE:
			case MODITEM_INSLIB_INSTRUMENT:
				nDefault = ID_MODTREE_PLAY;
				if (m_szSongName[0])
				{
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, "&Play");
				} else
				{
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, "&Play File");
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, "&Delete");
				}
				break;

			case MODITEM_DLSBANK_FOLDER:
				nDefault = ID_SOUNDBANK_PROPERTIES;
				AppendMenu(hMenu, MF_STRING, nDefault, "&Properties");
				AppendMenu(hMenu, MF_STRING, ID_MODTREE_REMOVE, "Re&move this bank");
			case MODITEM_NULL:
				AppendMenu(hMenu, MF_STRING, ID_ADD_SOUNDBANK, "Add Sound &Bank...");
				bSep = TRUE;
				break;

			default:
				if (modItemType & 0x8000)
				{
					nDefault = ID_MODTREE_PLAY;
					AppendMenu(hMenu, MF_STRING, ID_MODTREE_PLAY, "&Play Instrument");
				}
				break;
			}
			if (nDefault) SetMenuDefaultItem(hMenu, nDefault, FALSE);
			if ((modItemType == MODITEM_INSLIB_FOLDER)
			 || (modItemType == MODITEM_INSLIB_SONG)
			 || (modItemType == MODITEM_HDR_INSTRUMENTLIB))
			{
				if ((bSep) || (nDefault)) AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
				AppendMenu(hMenu, (m_bShowAllFiles) ? (MF_STRING|MF_CHECKED) : MF_STRING, ID_MODTREE_SHOWALLFILES, "Show All Files");
				AppendMenu(hMenu, (m_bShowAllFiles) ? MF_STRING : (MF_STRING|MF_CHECKED), ID_MODTREE_SOUNDFILESONLY, "Show Sound Files");
				bSep = TRUE;
			}
			if ((bSep) || (nDefault)) AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
			AppendMenu(hMenu, MF_STRING, ID_MODTREE_REFRESH, "&Refresh");
			TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x+4,pt.y,0,m_hWnd,NULL);
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
			const uint64 modItem = GetModItem(hItem);
			const uint32 modItemType = GetModItemType(modItem);
			const uint32 modItemID = GetModItemID(modItem);

			switch(modItemType)
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
							(modItemType == MODITEM_INSTRUMENT) ? TRUE : FALSE,
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
				CHAR szFullPath[_MAX_PATH] = "";
				if (GetDropInfo(&dropinfo, szFullPath))
				{
					if (dropinfo.dwDropType == DRAGONDROP_SONG)
					{
						theApp.OpenDocumentFile((LPCTSTR)dropinfo.lDropParam);
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
			m_qwItemDrag = 0;
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
						CHAR szFullPath[_MAX_PATH] = "";
						if (GetDropInfo(&dropinfo, szFullPath))
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
	vector<ModTreeDocInfo *>::iterator iter;
	for (iter = DocInfo.begin(); iter != DocInfo.end(); iter++)
	{
		UpdateView((*iter), HINT_MODTYPE);
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
	PlayItem(GetSelectedItem());
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
	CModDoc *pModDoc;

	const uint64 modItem = GetModItem(hItem);
	const uint32 modItemType = GetModItemType(modItem);
	const uint32 modItemID = GetModItemID(modItem);

	pModDoc = GetDocumentFromItem(hItem);

	if (pModDoc)
	{
		if ((modItemType == MODITEM_SAMPLE) && (!pModDoc->GetNumInstruments()))
		{
			pModDoc->MuteSample((SAMPLEINDEX)modItemID, (pModDoc->IsSampleMuted((SAMPLEINDEX)modItemID)) ? false : true);
			UpdateView(GetDocumentInfoFromModDoc(pModDoc), HINT_SMPNAMES | HINT_SAMPLEINFO);
		} else
		if ((modItemType == MODITEM_INSTRUMENT) && (pModDoc->GetNumInstruments()))
		{
			pModDoc->MuteInstrument((INSTRUMENTINDEX)modItemID, (pModDoc->IsInstrumentMuted((INSTRUMENTINDEX)modItemID)) ? false : true);
			UpdateView(GetDocumentInfoFromModDoc(pModDoc), HINT_INSNAMES | HINT_INSTRUMENT);
		} else
		if ((modItemType == MODITEM_EFFECT))
		{
			CSoundFile *pSndFile = pModDoc ? pModDoc->GetSoundFile() : nullptr;
			if (pSndFile == nullptr)
				return;
			PSNDMIXPLUGIN pPlugin = &pSndFile->m_MixPlugins[modItemID];
			if(pPlugin == nullptr)
				return;
			CVstPlugin *pVstPlugin = (CVstPlugin *)pPlugin->pMixPlugin;
			if(pVstPlugin == nullptr)
				return;
			pVstPlugin->Bypass();
			pModDoc->SetModified();
			//UpdateView(GetDocumentIDFromModDoc(pModDoc), HINT_MIXPLUGINS);
		}
	}
}


void CModTree::OnSoloTreeItem()
//-----------------------------
{
	HTREEITEM hItem = GetSelectedItem();
	CModDoc *pModDoc;

	const uint64 modItem = GetModItem(hItem);
	const uint32 modItemType = GetModItemType(modItem);
	const uint32 modItemID = GetModItemID(modItem);

	pModDoc = GetDocumentFromItem(hItem);
	if (pModDoc)
	{
		INSTRUMENTINDEX nInstruments = pModDoc->GetNumInstruments();
		if ((modItemType == MODITEM_SAMPLE) && (!nInstruments))
		{
			for (SAMPLEINDEX nSmp = 1; nSmp <= pModDoc->GetNumSamples(); nSmp++)
			{
				pModDoc->MuteSample(nSmp, (nSmp == modItemID) ? false : true);
				UpdateView(GetDocumentInfoFromModDoc(pModDoc), HINT_SMPNAMES | HINT_SAMPLEINFO);
			}
		} else
		if ((modItemType == MODITEM_INSTRUMENT) && (nInstruments))
		{
			for (INSTRUMENTINDEX nIns = 1; nIns <= nInstruments; nIns++)
			{
				pModDoc->MuteInstrument(nIns, (nIns == modItemID) ? false : true);
				UpdateView(GetDocumentInfoFromModDoc(pModDoc), HINT_INSNAMES | HINT_INSTRUMENT);
			}
		}
	}
}


void CModTree::OnUnmuteAllTreeItem()
//----------------------------------
{
	HTREEITEM hItem = GetSelectedItem();
	CModDoc *pModDoc;

	const uint64 modItem = GetModItem(hItem);
	const uint32 modItemType = GetModItemType(modItem);
	//const uint32 modItemID = GetModItemID(modItem);

	pModDoc = GetDocumentFromItem(hItem);
	if (pModDoc)
	{
		if ((modItemType == MODITEM_SAMPLE) || (modItemType == MODITEM_INSTRUMENT))
		{
			for (SAMPLEINDEX nSmp = 1; nSmp <= pModDoc->GetNumSamples(); nSmp++)
			{
				pModDoc->MuteSample(nSmp, false);
				UpdateView(GetDocumentInfoFromModDoc(pModDoc), HINT_SMPNAMES | HINT_SAMPLEINFO);
			}
			for (INSTRUMENTINDEX nIns = 1; nIns <= pModDoc->GetNumInstruments(); nIns++)
			{
				pModDoc->MuteInstrument(nIns, false);
				UpdateView(GetDocumentInfoFromModDoc(pModDoc), HINT_INSNAMES | HINT_INSTRUMENT);
			}
		}
	}
}


void CModTree::OnDuplicateTreeItem()
//----------------------------------
{
	HTREEITEM hItem = GetSelectedItem();
	CModDoc *pModDoc;

	const uint64 modItem = GetModItem(hItem);
	const uint32 modItemType = GetModItemType(modItem);
	const uint32 modItemID = GetModItemID(modItem);

	pModDoc = GetDocumentFromItem(hItem);
	CSoundFile *pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : nullptr;

	if (pModDoc && pSndFile && ((modItemType == MODITEM_SEQUENCE) || (modItemType == MODITEM_HDR_ORDERS)))
	{
		pSndFile->Order.SetSequence((SEQUENCEINDEX)modItemID);
		pSndFile->Order.AddSequence(true);
		pModDoc->SetModified();
		UpdateView(GetDocumentInfoFromModDoc(pModDoc), HINT_SEQNAMES|HINT_MODSEQUENCE);
		pModDoc->UpdateAllViews(NULL, HINT_SEQNAMES|HINT_MODSEQUENCE);
	}
}


void CModTree::OnInsertTreeItem()
//-------------------------------
{
	HTREEITEM hItem = GetSelectedItem();
	CModDoc *pModDoc;

	const uint64 modItem = GetModItem(hItem);
	const uint32 modItemType = GetModItemType(modItem);
	//const uint32 modItemID = GetModItemID(modItem);

	pModDoc = GetDocumentFromItem(hItem);
	CSoundFile *pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : nullptr;

	if (pModDoc && pSndFile && ((modItemType == MODITEM_SEQUENCE) || (modItemType == MODITEM_HDR_ORDERS)))
	{
		pSndFile->Order.AddSequence(false);
		pModDoc->SetModified();
		UpdateView(GetDocumentInfoFromModDoc(pModDoc), HINT_SEQNAMES|HINT_MODSEQUENCE);
		pModDoc->UpdateAllViews(NULL, HINT_SEQNAMES|HINT_MODSEQUENCE);
	}
}

void CModTree::OnSwitchToTreeItem()
//---------------------------------
{
	HTREEITEM hItem = GetSelectedItem();
	CModDoc *pModDoc;

	const uint64 modItem = GetModItem(hItem);
	const uint32 modItemType = GetModItemType(modItem);
	const uint32 modItemID = GetModItemID(modItem);

	pModDoc = GetDocumentFromItem(hItem);

	if (pModDoc && (modItemType == MODITEM_SEQUENCE))
	{
		pModDoc->ActivateView(IDD_CONTROL_PATTERNS, (modItemID << 16) | 0x8000);
	}
}

// -> CODE#0023
// -> DESC="IT project files (.itp)"
void CModTree::OnSetItemPath()
//----------------------------
{
	HTREEITEM hItem = GetSelectedItem();
	CModDoc *pModDoc = GetDocumentFromItem(hItem);
	CSoundFile *pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	const uint64 modItem = GetModItem(hItem);
	//const uint32 modItemType = GetModItemType(modItem);
	const uint32 modItemID = GetModItemID(modItem);

	if(pSndFile && modItemID){

		FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "", "",
			"All files(*.*)|*.*||");
		if(files.abort) return;

		strcpy(pSndFile->m_szInstrumentPath[modItemID - 1], files.first_file.c_str());
		OnRefreshTree();
	}
}

void CModTree::OnSaveItem()
//-------------------------
{
	HTREEITEM hItem = GetSelectedItem();
	CModDoc *pModDoc = GetDocumentFromItem(hItem);
	CSoundFile *pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	const uint64 modItem = GetModItem(hItem);
	//const uint32 modItemType = GetModItemType(modItem);
	const uint32 modItemID = GetModItemID(modItem);

	if(pSndFile && modItemID){

		if(pSndFile->m_szInstrumentPath[modItemID - 1][0] == '\0')
		{
			FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, (pSndFile->GetType() == MOD_TYPE_XM) ? "xi" : "iti", "",
				(pSndFile->GetType() == MOD_TYPE_XM) ?
				"FastTracker II Instruments (*.xi)|*.xi|"
				"Impulse Tracker Instruments (*.iti)|*.iti||" :
				"Impulse Tracker Instruments (*.iti)|*.iti|"
				"FastTracker II Instruments (*.xi)|*.xi||");
			if(files.abort) return;

			strcpy(pSndFile->m_szInstrumentPath[modItemID - 1], files.first_file.c_str());
		}

		if(pSndFile->m_szInstrumentPath[modItemID - 1][0] != '\0')
		{
			int size = strlen(pSndFile->m_szInstrumentPath[modItemID - 1]);
			BOOL iti = _stricmp(&pSndFile->m_szInstrumentPath[modItemID - 1][size-3],"iti") == 0;
			BOOL xi  = _stricmp(&pSndFile->m_szInstrumentPath[modItemID - 1][size-2],"xi") == 0;

			if(iti || (!iti && !xi  && pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)))
				pSndFile->SaveITIInstrument((INSTRUMENTINDEX)modItemID, pSndFile->m_szInstrumentPath[modItemID - 1]);
			if(xi  || (!xi  && !iti && pSndFile->m_nType == MOD_TYPE_XM))
				pSndFile->SaveXIInstrument((INSTRUMENTINDEX)modItemID, pSndFile->m_szInstrumentPath[modItemID - 1]);

			pModDoc->m_bsInstrumentModified.reset(modItemID - 1);
		}

		if(pModDoc) pModDoc->UpdateAllViews(NULL, HINT_MODTYPE);
		OnRefreshTree();
	}
}
// -! NEW_FEATURE#0023


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
	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, "ini", "mptrack.ini",
		"Text and INI files (*.txt,*.ini)|*.txt;*.ini|"
		"All Files (*.*)|*.*||");
	if(files.abort) return;

	CTrackApp::ExportMidiConfig(files.first_file.c_str());
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

	const uint64 modItem = GetModItem(hItem);
	const uint32 modItemType = GetModItemType(modItem);
	//const uint32 modItemID = GetModItemID(modItem);

	switch(modItemType)
	{
	case MODITEM_MIDIINSTRUMENT:
	case MODITEM_MIDIPERCUSSION:
		if (hItem != GetDropHilightItem())
		{
			SelectDropTarget(hItem);
			EnsureVisible(hItem);
		}
		m_hItemDrag = hItem;
		m_qwItemDrag = modItem;
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
	TCHAR szFileName[_MAX_PATH] = "";
	STGMEDIUM stgm;
	HDROP hDropInfo;
	UINT nFiles;
	BOOL bOk = FALSE;
	
	if (!pDataObject) return FALSE;
	if (!pDataObject->GetData(CF_HDROP, &stgm)) return FALSE;
	if (stgm.tymed != TYMED_HGLOBAL) return FALSE;
	if (stgm.hGlobal == NULL) return FALSE;
	hDropInfo = (HDROP)stgm.hGlobal;
	nFiles = DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	if (nFiles)
	{
		DragQueryFile(hDropInfo, 0, szFileName, _MAX_PATH);
		if (szFileName[0])
		{
			DWORD dwItem = (DWORD)(m_qwItemDrag >> 16);
			switch(m_qwItemDrag & 0xFFFF)
			{
			case MODITEM_MIDIINSTRUMENT:
				bOk = SetMidiInstrument(dwItem, szFileName);
				break;
			case MODITEM_MIDIPERCUSSION:
				bOk = SetMidiPercussion(dwItem, szFileName);
				break;
			}
		}
	}
	::DragFinish(hDropInfo);
	return bOk;
}


void CModTree::OnRefreshInstrLib()
//--------------------------------
{
	HTREEITEM hActive;
	
	BeginWaitCursor();
	RefreshInstrumentLibrary();
	if (m_pDataTree)
	{
		hActive = NULL;
		if ((m_szOldPath[0]) || (m_szSongName[0]))
		{
			HTREEITEM hItem = GetChildItem(m_hInsLib);
			while (hItem != NULL)
			{
				CString str = GetItemText(hItem);
				if (((m_szSongName[0]) && (!lstrcmpi(str, m_szSongName)))
				 || ((!m_szSongName[0]) && (!lstrcmpi(str, m_szOldPath))))
				{
					hActive = hItem;
					break;
				}
				hItem = GetNextItem(hItem, TVGN_NEXT);
			}
			if (!m_szSongName[0]) m_szOldPath[0] = 0;
		}
		SelectSetFirstVisible(m_hInsLib);
		if (hActive != NULL) SelectItem(hActive);
	}
	EndWaitCursor();
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
	HTREEITEM hItem = GetSelectedItem();

	const uint64 modItem = GetModItem(hItem);
	const uint32 modItemType = GetModItemType(modItem);
	const uint32 modItemID = GetModItemID(modItem);

	if ((modItemType & 0xFFFF) == MODITEM_DLSBANK_FOLDER)
	{
		UINT nBank = modItemID;
		if ((nBank < MAX_DLS_BANKS) && (CTrackApp::gpDLSBanks[nBank]))
		{
			CSoundBankProperties dlg(CTrackApp::gpDLSBanks[nBank], this);
			dlg.DoModal();
		}
	}
}


LRESULT CModTree::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//----------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;
	
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if (wParam>=kcTreeViewStartNotes && wParam<=kcTreeViewEndNotes)
	{
		PlayItem(GetSelectedItem(), wParam - kcTreeViewStartNotes + 1 + pMainFrm->GetBaseOctave() * 12);
		return wParam;
	}
	if (wParam>=kcTreeViewStartNoteStops && wParam<=kcTreeViewEndNoteStops)
	{	
		return wParam;
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
	return (tvi.state & TVIS_EXPANDED) != 0 ? true : false;
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
