/*
 * View_tre.h
 * ----------
 * Purpose: Tree view for managing open songs, sound files, file browser, ...
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "../soundlib/modcommand.h"
#include "../soundlib/Snd_defs.h"

#include "Mptrack.h"
#include "Notification.h"
#include "UpdateHints.h"

#include <vector>
#include <bitset>

OPENMPT_NAMESPACE_BEGIN

class CModDoc;
class CModTree;
class CSoundFile;
class CDLSBank;

struct ModTreeDocInfo
{
	// Tree state variables
	std::vector<std::vector<HTREEITEM>> tiOrders;
	std::vector<HTREEITEM> tiSequences, tiPatterns;
	CModDoc &modDoc;
	HTREEITEM hSong = nullptr, hPatterns = nullptr, hSamples = nullptr, hInstruments = nullptr, hComments = nullptr, hOrders = nullptr, hEffects = nullptr;

	// Module information
	ORDERINDEX ordSel = ORDERINDEX_INVALID;
	SEQUENCEINDEX seqSel = SEQUENCEINDEX_INVALID;

	std::bitset<MAX_SAMPLES> samplesPlaying;
	std::bitset<MAX_INSTRUMENTS> instrumentsPlaying;

	ModTreeDocInfo(CModDoc &modDoc);
};


class CModTreeDropTarget: public COleDropTarget
{
protected:
	CModTree *m_pModTree = nullptr;

public:
	CModTreeDropTarget() = default;
	BOOL Register(CModTree *pWnd);

public:
	DROPEFFECT OnDragEnter(CWnd *pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) override;
	DROPEFFECT OnDragOver(CWnd *pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) override;
	BOOL OnDrop(CWnd *pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) override;
};


class CModTree: public CTreeCtrl
{
public:
	enum ModItemType : uint8
	{
		MODITEM_NULL = 0,

		MODITEM_BEGIN_SONGITEMS,
		MODITEM_ORDER = MODITEM_BEGIN_SONGITEMS,
		MODITEM_PATTERN,
		MODITEM_SAMPLE,
		MODITEM_INSTRUMENT,
		MODITEM_COMMENTS,
		MODITEM_EFFECT,
		MODITEM_SEQUENCE,
		MODITEM_HDR_SONG,
		MODITEM_HDR_ORDERS,
		MODITEM_HDR_PATTERNS,
		MODITEM_HDR_SAMPLES,
		MODITEM_HDR_INSTRUMENTS,
		MODITEM_HDR_EFFECTS,
		MODITEM_END_SONGITEMS = MODITEM_HDR_EFFECTS,

		MODITEM_HDR_INSTRUMENTLIB,
		MODITEM_HDR_MIDILIB,
		MODITEM_HDR_MIDIGROUP,
		MODITEM_MIDIINSTRUMENT,
		MODITEM_MIDIPERCUSSION,
		MODITEM_INSLIB_FOLDER,
		MODITEM_INSLIB_SAMPLE,
		MODITEM_INSLIB_INSTRUMENT,
		MODITEM_INSLIB_SONG,
		MODITEM_DLSBANK_FOLDER,
		MODITEM_DLSBANK_INSTRUMENT,
	};

protected:
	enum TreeStatus
	{
		TREESTATUS_RDRAG = 0x01,
		TREESTATUS_LDRAG = 0x02,
		TREESTATUS_SINGLEEXPAND = 0x04,
		TREESTATUS_DRAGGING = (TREESTATUS_RDRAG | TREESTATUS_LDRAG)
	};

	// Bit mask magic
	enum
	{
		MIDILIB_SHIFT = 16,
		MIDILIB_MASK  = (1 << MIDILIB_SHIFT) - 1,

		// Must be consistent with CCtrlPatterns::OnActivatePage
		SEQU_SHIFT     = 16,
		SEQU_MASK      = (1 << SEQU_SHIFT) - 1,
		SEQU_INDICATOR = 0x80000000,

		// Soundbank instrument identification (must be consistent with CViewInstrument::OnDragonDrop / CViewSample::OnDragonDrop)
		DLS_TYPEPERC    = 0x80000000,
		DLS_INSTRMASK   = 0x0000FFFF,
		DLS_REGIONMASK  = 0x7FFF0000,  // Drum region
		DLS_REGIONSHIFT = 16,

		DLS_DRUM_FOLDER_LPARAM = 0x12345678,
	};
	static_assert((ORDERINDEX_INVALID & SEQU_MASK) == ORDERINDEX_INVALID, "ORDERINDEX doesn't fit in GetItemData() parameter");
	static_assert((ORDERINDEX_MAX & SEQU_MASK) == ORDERINDEX_MAX, "ORDERINDEX doesn't fit in GetItemData() parameter");
	static_assert((((SEQUENCEINDEX_INVALID << SEQU_SHIFT) & ~SEQU_INDICATOR) >> SEQU_SHIFT) == SEQUENCEINDEX_INVALID, "SEQUENCEINDEX doesn't fit in GetItemData() parameter");

	struct ModItem
	{
		uint32 val1;
		uint16 val2;
		ModItemType type;

		ModItem(ModItemType t = MODITEM_NULL, uint32 v1 = 0, uint16 v2 = 0) : val1(v1), val2(v2), type(t) { }
		bool IsSongItem() const noexcept { return type >= MODITEM_BEGIN_SONGITEMS && type <= MODITEM_END_SONGITEMS; }
		bool operator==(const ModItem &other) const noexcept { return val1 == other.val1 && val2 == other.val2 && type == other.type; }
		bool operator!=(const ModItem &other) const noexcept { return !(*this == other); }
	};

	struct DlsItem : public ModItem
	{
		explicit DlsItem(uint16 instr) : ModItem(MODITEM_DLSBANK_INSTRUMENT, instr, 0) { }
		DlsItem(uint16 instr, uint16 region) : ModItem(MODITEM_DLSBANK_INSTRUMENT, (instr & DLS_INSTRMASK) | ((region << DLS_REGIONSHIFT) & DLS_REGIONMASK) | DLS_TYPEPERC, 0) { }

		uint32 GetRegion() const noexcept { return (val1 & DLS_REGIONMASK) >> DLS_REGIONSHIFT; }
		uint32 GetInstr() const noexcept { return (val1 & DLS_INSTRMASK); }
		bool IsPercussion() const noexcept { return ((val1 & DLS_TYPEPERC) == DLS_TYPEPERC); }
		bool IsMelodic() const noexcept { return !IsPercussion(); }

		static ModItem FromLPARAM(uint32 lparam) { return ModItem{MODITEM_DLSBANK_INSTRUMENT, lparam, 0}; }
		static LPARAM ToLPARAM(uint16 instr, uint16 region, bool isPerc) { return (instr & DLS_INSTRMASK) | ((region << DLS_REGIONSHIFT) & DLS_REGIONMASK) | (isPerc ? DLS_TYPEPERC : 0); }
	};

	static CSoundFile *m_SongFile;  // For browsing samples and instruments inside modules on disk
	CModTreeDropTarget m_DropTarget;
	CModTree *m_pDataTree = nullptr;  // Pointer to instrument browser (lower part of tree view) - if it's a nullptr, this object is the instrument browser itself.
	HWND m_hDropWnd = nullptr;
	mpt::mutex m_WatchDirMutex;
	HANDLE m_hSwitchWatchDir = nullptr;
	mpt::PathString m_WatchDir;
	HANDLE m_hWatchDirKillThread = nullptr;
	std::thread m_WatchDirThread;
	ModItem m_itemDrag;
	DWORD m_dwStatus = 0;
	CModDoc *m_selectedDoc = nullptr, *m_dragDoc = nullptr;
	HTREEITEM m_hItemDrag = nullptr, m_hItemDrop = nullptr;
	HTREEITEM m_hInsLib = nullptr, m_hMidiLib = nullptr;
	HTREEITEM m_tiMidi[128];
	HTREEITEM m_tiPerc[128];
	std::vector<HTREEITEM> m_tiDLS;
	std::map<const CModDoc *, ModTreeDocInfo> m_docInfo;

	std::unique_ptr<CDLSBank> m_cachedBank;
	mpt::PathString m_cachedBankName;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
	DWORD m_stringCompareFlags = NORM_IGNORECASE | NORM_IGNOREWIDTH | SORT_DIGITSASNUMBERS;
#else
	DWORD m_stringCompareFlags = NORM_IGNORECASE | NORM_IGNOREWIDTH;
#endif

	// Instrument library
	mpt::PathString m_InstrLibPath;           // Current path to be explored
	mpt::PathString m_InstrLibHighlightPath;  // Folder to highlight in browser after a refresh
	mpt::PathString m_SongFileName;           // Name of open module, without path (== m_InstrLibPath).
	mpt::PathString m_previousPath;           // The folder from which we came from when navigating one folder up
	
	std::vector<const char*> m_modExtensions;                  // Cached in order to avoid querying too often when changing browsed folder
	std::vector<mpt::PathString> m_MediaFoundationExtensions;  // Cached in order to avoid querying too often when changing browsed folder

	struct FileBrowserEntry
	{
		mpt::winstring name;
		uint64 size;
		uint64 modtime;
		uint32 image;
		bool hidden;
	};
	std::vector<FileBrowserEntry> m_fileBrowserEntries;
	mpt::winstring m_filterString;

	enum class LibrarySortOrder
	{
		Name,
		Date,
		Size,
	};

	static LibrarySortOrder m_librarySort;

	int m_redrawLockCount = 0;

	bool m_showAllFiles = false;
	bool m_doLabelEdit = false;

public:
	CModTree(CModTree *pDataTree);
	~CModTree();

	void Init();
	bool InsLibSetFullPath(const mpt::PathString &libPath, const mpt::PathString &songFolder);
	mpt::PathString InsLibGetFullPath(HTREEITEM hItem) const;
	bool SetSoundFile(FileReader &file);
	void RefreshMidiLibrary();
	void RefreshDlsBanks(const bool forceRefresh = false);
	void RefreshInstrumentLibrary();
	void MonitorInstrumentLibrary();
	ModItem GetModItem(HTREEITEM hItem);
	BOOL SetMidiInstrument(UINT nIns, const mpt::PathString &fileName);
	BOOL SetMidiPercussion(UINT nPerc, const mpt::PathString &fileName);
	bool ExecuteItem(HTREEITEM hItem);
	void DeleteTreeItem(HTREEITEM hItem, const bool permanently);
	static void PlayDLSItem(const CDLSBank &dlsBank, const DlsItem &item, ModCommand::NOTE note);
	BOOL PlayItem(HTREEITEM hItem, ModCommand::NOTE nParam, int volume = -1);
	BOOL OpenTreeItem(HTREEITEM hItem);
	BOOL OpenMidiInstrument(DWORD dwItem);
	void SetFullInstrumentLibraryPath(mpt::PathString path);
	void InstrumentLibraryChDir(mpt::PathString dir, bool isSong);
	bool GetDropInfo(DRAGONDROP &dropInfo, mpt::PathString &fullPath);
	void OnOptionsChanged();
	void AddDocument(CModDoc &modDoc);
	void RemoveDocument(const CModDoc &modDoc);
	void UpdateView(ModTreeDocInfo &info, UpdateHint hint);
	void OnUpdate(CModDoc *pModDoc, UpdateHint hint, CObject *pHint);
	bool CanDrop(HTREEITEM hItem, bool bDoDrop);
	void UpdatePlayPos(CModDoc &modDoc, Notification *pNotify);
	bool IsItemExpanded(HTREEITEM hItem);
	void DeleteChildren(HTREEITEM hItem);
	HTREEITEM GetNthChildItem(HTREEITEM hItem, int index) const;
	HTREEITEM GetParentRootItem(HTREEITEM hItem) const;

	bool IsSampleBrowser() const { return m_pDataTree == nullptr; }
	CModTree *GetSampleBrowser() { return IsSampleBrowser() ? this : m_pDataTree; }
	CModTree *GetOtherView();

	void SetInstrumentLibraryFilter(const mpt::winstring &filter);
	void SetInstrumentLibraryFilterSortOrder(LibrarySortOrder sortType);
	void SortInstrumentLibrary();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModTree)
public:
	BOOL PreTranslateMessage(MSG *pMsg) override;
	//}}AFX_VIRTUAL

// Drag & Drop operations
public:
	DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);

protected:
	int ImageToSortOrder(int image) const;
	ModTreeDocInfo *GetDocumentInfoFromItem(HTREEITEM hItem);
	CModDoc *GetDocumentFromItem(HTREEITEM hItem) { ModTreeDocInfo *info = GetDocumentInfoFromItem(hItem); return info ? &info->modDoc : nullptr; }
	ModTreeDocInfo *GetDocumentInfoFromModDoc(CModDoc &modDoc);

	size_t GetDLSBankIndexFromItem(HTREEITEM hItem) const;
	CDLSBank *GetDLSBankFromItem(HTREEITEM hItem) const;

	void InsertOrDupItem(bool insert);
	void OnItemRightClick(HTREEITEM hItem, CPoint pt);

	static bool HasEffectPlugins(const CSoundFile &sndFile);
	static bool AllPluginsBypassed(const CSoundFile &sndFile, bool onlyEffects);
	static void BypassAllPlugins(CSoundFile &sndFile, bool bypass, bool onlyEffects);

	void FillInstrumentLibrary(const TCHAR *selectedItem = nullptr);
	void FilterInstrumentLibrary(mpt::winstring filter, const TCHAR *selectedItem = nullptr);

	HMENU AddLibraryFindAndSortMenus(HMENU hMenu) const;

	void LockRedraw()
	{
		if(!m_redrawLockCount++)
			SetRedraw(FALSE);
	}

	void UnlockRedraw()
	{
		if(!--m_redrawLockCount)
			SetRedraw(TRUE);
	}

protected:
	//{{AFX_MSG(CModTree)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnXButtonUp(UINT nFlags, UINT nButton, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnBeginDrag(HTREEITEM, bool bLeft, LRESULT *pResult);
	afx_msg void OnBeginLDrag(LPNMHDR, LRESULT *pResult);
	afx_msg void OnBeginRDrag(LPNMHDR, LRESULT *pResult);
	afx_msg void OnEndDrag(DWORD dwMask);
	afx_msg void OnItemDblClk(LPNMHDR phdr, LRESULT *pResult);
	afx_msg void OnItemReturn(LPNMHDR, LRESULT *pResult);
	afx_msg void OnItemLeftClick(LPNMHDR pNMHDR, LRESULT *pResult);
	afx_msg void OnItemRightClick(LPNMHDR, LRESULT *pResult);
	afx_msg void OnItemExpanded(LPNMHDR pnmhdr, LRESULT *pResult);
	afx_msg void OnGetDispInfo(LPNMHDR pnmhdr, LRESULT *pResult);
	afx_msg void OnRefreshTree();
	afx_msg void OnExecuteItem();
	afx_msg void OnPlayTreeItem();
	afx_msg void OnDeleteTreeItem();
	afx_msg void OnOpenTreeItem();
	afx_msg void OnMuteTreeItem();
	afx_msg void OnMuteOnlyEffects();
	afx_msg void OnSoloTreeItem();
	afx_msg void OnUnmuteAllTreeItem();
	afx_msg void OnDuplicateTreeItem() { InsertOrDupItem(false); }
	afx_msg void OnInsertTreeItem() { InsertOrDupItem(true); }
	afx_msg void OnSwitchToTreeItem();	// hack for sequence items to avoid double-click action
	afx_msg void OnCloseItem();
	afx_msg void OnRenameItem();
	afx_msg void OnBeginLabelEdit(NMHDR *nmhdr, LRESULT *result);
	afx_msg void OnEndLabelEdit(NMHDR *nmhdr, LRESULT *result);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnKillFocus(CWnd *pNewWnd);
	afx_msg void OnSetFocus(CWnd *pOldWnd);

	afx_msg void OnSetItemPath();
	afx_msg void OnSaveItem();
	afx_msg void OnSaveAll();
	afx_msg void OnReloadItem();
	afx_msg void OnReloadAll();
	afx_msg void OnFindMissing();

	afx_msg void OnAddDlsBank();
	afx_msg void OnImportMidiLib();
	afx_msg void OnExportMidiLib();
	afx_msg void OnSoundBankProperties();
	afx_msg void OnRefreshInstrLib();
	afx_msg void OnShowDirectories();
	afx_msg void OnShowAllFiles();
	afx_msg void OnShowSoundFiles();

	afx_msg void OnGotoInstrumentDir();
	afx_msg void OnGotoSampleDir();

	afx_msg void OnOpenInstrumentLibraryFilter();
	afx_msg void OnSortByName() { SetInstrumentLibraryFilterSortOrder(LibrarySortOrder::Name); }
	afx_msg void OnSortByDate() { SetInstrumentLibraryFilterSortOrder(LibrarySortOrder::Date); }
	afx_msg void OnSortBySize() { SetInstrumentLibraryFilterSortOrder(LibrarySortOrder::Size); }

	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);
	LRESULT OnMidiMsg(WPARAM midiData, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


OPENMPT_NAMESPACE_END
