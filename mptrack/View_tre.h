/*
 * view_tre.h
 * ----------
 * Purpose: Tree view for managing open songs, sound files, file browser, ...
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <vector>
#include <bitset>

OPENMPT_NAMESPACE_BEGIN

class CModDoc;
class CModTree;
class CSoundFile;

#define TREESTATUS_RDRAG			0x01
#define TREESTATUS_LDRAG			0x02
#define TREESTATUS_SINGLEEXPAND		0x04
#define TREESTATUS_DRAGGING			(TREESTATUS_RDRAG|TREESTATUS_LDRAG)

struct ModTreeDocInfo
{
	// Tree state variables
	std::vector<std::vector<HTREEITEM> > tiOrders;
	std::vector<HTREEITEM> tiSequences, tiPatterns;
	CModDoc &modDoc;
	HTREEITEM hSong, hPatterns, hSamples, hInstruments, hComments, hOrders, hEffects;

	// Module information
	ORDERINDEX nOrdSel;
	SEQUENCEINDEX nSeqSel;

	std::bitset<MAX_SAMPLES> samplesPlaying;
	std::bitset<MAX_INSTRUMENTS> instrumentsPlaying;

	ModTreeDocInfo(CModDoc &modDoc);
};


//=============================================
class CModTreeDropTarget: public COleDropTarget
//=============================================
{
protected:
	CModTree *m_pModTree;

public:
	CModTreeDropTarget() { m_pModTree = NULL; }
	BOOL Register(CModTree *pWnd);

public:
	virtual DROPEFFECT OnDragEnter(CWnd *pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver(CWnd *pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual BOOL OnDrop(CWnd *pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
};


OPENMPT_NAMESPACE_END
#include "CTreeCtrl.h"
#include "../common/mptThread.h"
OPENMPT_NAMESPACE_BEGIN


//===============================
class CModTree: public CTreeCtrlW
//===============================
{
protected:

	enum ModItemType
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
		MODITEM_END_SONGITEMS = MODITEM_HDR_INSTRUMENTS,

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

	// Bit mask magic
	enum
	{
		MIDILIB_SHIFT	= 16,
		MIDILIB_MASK	= (1 << MIDILIB_SHIFT) - 1,

		// Must be consistent with CCtrlPatterns::OnActivatePage
		SEQU_SHIFT		= 16,
		SEQU_MASK		= (1 << SEQU_SHIFT) - 1,
		SEQU_INDICATOR	= 0x80000000,

		// Soundbank instrument identification
		DLS_TYPEINST	= 0x40000000,
		DLS_TYPEPERC	= 0x80000000,
		DLS_TYPEMASK	= DLS_TYPEINST | DLS_TYPEPERC,
		DLS_INSTRMASK	= 0x00007FFF,
		DLS_REGIONMASK	= 0x007F0000,	// DLS region
		DLS_REGIONSHIFT	= 16,
		DLS_HIBANKMASK	= 0x3F000000,	// High bits of bank index
		DLS_HIBANKSHIFT	= 24,
	};
	static_assert((ORDERINDEX_INVALID & SEQU_MASK) == ORDERINDEX_INVALID, "ORDERINDEX doesn't fit in GetItemData() parameter");
	static_assert((ORDERINDEX_MAX & SEQU_MASK) == ORDERINDEX_MAX, "ORDERINDEX doesn't fit in GetItemData() parameter");
	static_assert((((SEQUENCEINDEX_INVALID << SEQU_SHIFT) & ~SEQU_INDICATOR) >> SEQU_SHIFT) == SEQUENCEINDEX_INVALID, "SEQUENCEINDEX doesn't fit in GetItemData() parameter");

	struct ModItem
	{
		uint32 val1;
		uint16 type;	// see ModItemType
		uint16 val2;

		ModItem(ModItemType t = MODITEM_NULL, uint32 v1 = 0, uint16 v2 = 0) : type(uint16(t)), val1(v1), val2(v2) { }
		bool IsSongItem() const { return type >= MODITEM_BEGIN_SONGITEMS && type <= MODITEM_END_SONGITEMS; }
	};

	struct DlsItem : public ModItem
	{
		DlsItem(uint32 bank, uint32 itemData) : ModItem(MODITEM_DLSBANK_INSTRUMENT, itemData | ((bank << (DLS_HIBANKSHIFT - 16)) & DLS_HIBANKMASK), (uint16)bank) { }

		uint32 GetBankIndex() const { return val2 | ((val1 & DLS_HIBANKMASK) >> (DLS_HIBANKSHIFT - 16)); }
		uint32 GetRegion() const { return (val1 & DLS_REGIONMASK) >> DLS_REGIONSHIFT; }
		uint32 GetInstr() const { return (val1 & DLS_INSTRMASK); }
		bool IsPercussion() const { return ((val1 & DLS_TYPEMASK) == DLS_TYPEPERC); }
		bool IsInstr() const { return ((val1 & DLS_TYPEMASK) == DLS_TYPEINST); }

		static uint32 EncodeValuePerc(uint8 region, uint16 instr) { return DLS_TYPEPERC | (region << DLS_REGIONSHIFT) | instr; }
		static uint32 EncodeValueInstr(uint8 region, uint16 instr) { return DLS_TYPEINST | (region << DLS_REGIONSHIFT) | instr; }
	};

	static CSoundFile *m_SongFile;	// For browsing samples and instruments inside modules on disk
	CModTreeDropTarget m_DropTarget;
	CModTree *m_pDataTree;	// Pointer to instrument browser (lower part of tree view) - if it's a nullptr, this object is the instrument browser itself.
	HWND m_hDropWnd;
	mpt::mutex m_WatchDirMutex;
	HANDLE m_hSwitchWatchDir;
	mpt::PathString m_WatchDir;
	HANDLE m_hWatchDirKillThread;
	mpt::thread m_WatchDirThread;
	ModItem m_itemDrag;
	DWORD m_dwStatus;
	UINT m_nDocNdx, m_nDragDocNdx;
	HTREEITEM m_hItemDrag, m_hItemDrop;
	HTREEITEM m_hInsLib, m_hMidiLib;
	HTREEITEM m_tiMidi[128];
	HTREEITEM m_tiPerc[128];
	std::vector<HTREEITEM> m_tiDLS;
	std::vector<ModTreeDocInfo *> DocInfo;

	// Instrument library
	mpt::PathString m_InstrLibPath;				// Current path to be explored
	mpt::PathString m_InstrLibHighlightPath;	// Folder to highlight in browser after a refresh
	mpt::PathString m_SongFileName;				// Name of open module, without path (== m_szInstrLibPath).
	bool m_bShowAllFiles;
	std::vector<mpt::PathString> m_MediaFoundationExtensions;	// cached in order to avoid querying too ofter when changing browsed folder
	
	bool doLabelEdit;

public:
	CModTree(CModTree *pDataTree);
	virtual ~CModTree();

// Attributes
public:
	void Init();
	bool InsLibSetFullPath(const mpt::PathString &libPath, const mpt::PathString &songFolder);
	mpt::PathString InsLibGetFullPath(HTREEITEM hItem) const;
	bool SetSoundFile(FileReader &file);
	void RefreshMidiLibrary();
	void RefreshDlsBanks();
	void RefreshInstrumentLibrary();
	void EmptyInstrumentLibrary();
	void FillInstrumentLibrary();
	void MonitorInstrumentLibrary();
	ModItem GetModItem(HTREEITEM hItem);
	BOOL SetMidiInstrument(UINT nIns, const mpt::PathString &fileName);
	BOOL SetMidiPercussion(UINT nPerc, const mpt::PathString &fileName);
	BOOL ExecuteItem(HTREEITEM hItem);
	void DeleteTreeItem(HTREEITEM hItem);
	BOOL PlayItem(HTREEITEM hItem, ModCommand::NOTE nParam);
	BOOL OpenTreeItem(HTREEITEM hItem);
	BOOL OpenMidiInstrument(DWORD dwItem);
	void InstrumentLibraryChDir(mpt::PathString dir, bool isSong);
	bool GetDropInfo(DRAGONDROP &dropInfo, mpt::PathString &fullPath);
	void OnOptionsChanged();
	void AddDocument(CModDoc &modDoc);
	void RemoveDocument(CModDoc &modDoc);
	void UpdateView(ModTreeDocInfo &info, UpdateHint hint);
	void OnUpdate(CModDoc *pModDoc, UpdateHint hint, CObject *pHint);
	bool CanDrop(HTREEITEM hItem, bool bDoDrop);
	void UpdatePlayPos(CModDoc &modDoc, Notification *pNotify);
	bool IsItemExpanded(HTREEITEM hItem);
	void DeleteChildren(HTREEITEM hItem);
	HTREEITEM GetNthChildItem(HTREEITEM hItem, int index);
	HTREEITEM GetParentRootItem(HTREEITEM hItem);

	bool IsSampleBrowser() const { return m_pDataTree == nullptr; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModTree)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Drag & Drop operations
public:
	DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);

protected:
	static int CALLBACK ModTreeInsLibCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	static int CALLBACK ModTreeDrumCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	void ModTreeInsert(const WCHAR *name, int image);
	ModTreeDocInfo *GetDocumentInfoFromItem(HTREEITEM hItem);
	CModDoc *GetDocumentFromItem(HTREEITEM hItem) { ModTreeDocInfo *info = GetDocumentInfoFromItem(hItem); return info ? &info->modDoc : nullptr; }
	ModTreeDocInfo *GetDocumentInfoFromModDoc(CModDoc &modDoc);

	void InsertOrDupItem(bool insert);

	bool IsMediaFoundationExtension(const std::string &ext) const;

// Generated message map functions
protected:
	//{{AFX_MSG(CModTree)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
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
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnRefreshTree();
	afx_msg void OnExecuteItem();
	afx_msg void OnPlayTreeItem();
	afx_msg void OnDeleteTreeItem();
	afx_msg void OnOpenTreeItem();
	afx_msg void OnMuteTreeItem();
	afx_msg void OnSoloTreeItem();
	afx_msg void OnUnmuteAllTreeItem();
	afx_msg void OnDuplicateTreeItem() { InsertOrDupItem(false); }
	afx_msg void OnInsertTreeItem() { InsertOrDupItem(true); }
	afx_msg void OnSwitchToTreeItem();	// hack for sequence items to avoid double-click action
	afx_msg void OnCloseItem();
	afx_msg void OnBeginLabelEdit(NMHDR *nmhdr, LRESULT *result);
	afx_msg void OnEndLabelEdit(NMHDR *nmhdr, LRESULT *result);
	afx_msg void OnDropFiles(HDROP hDropInfo);

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
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);	//rewbs.customKeys
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);	//rewbs.customKeys
	afx_msg void OnSetFocus(CWnd* pOldWnd);		//rewbs.customKeys
};


OPENMPT_NAMESPACE_END
