// globals.h : interface of the CViewModTree class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _MODGLOBALS_H_
#define _MODGLOBALS_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#ifndef WM_HELPHITTEST
#define WM_HELPHITTEST		0x366
#endif

#ifndef HID_BASE_COMMAND
#define HID_BASE_COMMAND	0x10000
#endif

#define ID_EDIT_MIXPASTE ID_EDIT_PASTE_SPECIAL		//rewbs.mixPaste

class CModControlView;
class CModControlBar;

//=======================================
class CModControlBar: public CToolBarCtrl
//=======================================
{
protected:
	HBITMAP m_hBarBmp;

public:
	CModControlBar() { m_hBarBmp=NULL; }
	~CModControlBar();

public:
	BOOL Init(UINT nId=IDB_PATTERNS);
	void UpdateStyle();
	BOOL AddButton(UINT nID, int iImage=0, UINT nStyle=TBSTYLE_BUTTON, UINT nState=TBSTATE_ENABLED);
	afx_msg LRESULT OnHelpHitTest(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
};


//==================================
class CModControlDlg: public CDialog
//==================================
{
protected:
	BOOL m_bInitialized;
	CModDoc *m_pModDoc;
	CModControlView *m_pParent;
	CSoundFile *m_pSndFile;
	HWND m_hWndView;
	LONG m_nLockCount;

public:
	CModControlDlg();
	virtual ~CModControlDlg();
	
public:
	VOID SetDocument(CModDoc *pModDoc, CModControlView *parent) { m_pParent = parent; m_pModDoc = pModDoc; m_pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : NULL; }
	VOID SetViewWnd(HWND hwndView) { m_hWndView = hwndView; }
	CModDoc *GetDocument() const { return m_pModDoc; }
	CSoundFile *GetSoundFile() const { return m_pSndFile; }
	HWND GetViewWnd() const { return m_hWndView; }
	LRESULT SendViewMessage(UINT uMsg, LPARAM lParam=0) const;
	BOOL PostViewMessage(UINT uMsg, LPARAM lParam=0) const;
	LRESULT SwitchToView() const { return SendViewMessage(VIEWMSG_SETACTIVE); }
	void LockControls() { m_nLockCount++; }
	void UnlockControls() { PostMessage(WM_MOD_UNLOCKCONTROLS); }
    BOOL IsLocked() const { return (m_nLockCount > 0); }
	virtual LONG* GetSplitPosRef() = 0; 	//rewbs.varWindowSize

protected:
	//{{AFX_VIRTUAL(CModControlDlg)
	public:
	
	afx_msg void OnEditCut() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_CUT, 0); }
	afx_msg void OnEditCopy() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_COPY, 0); }
	afx_msg void OnEditPaste() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_PASTE, 0); }
	afx_msg void OnEditMixPaste() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_PASTE_SPECIAL, 0); }		//rewbs.mixPaste
	afx_msg void OnEditFind() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_FIND, 0); }
	afx_msg void OnEditFindNext() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_FINDNEXT, 0); }
	afx_msg void OnSwitchToView() { if (m_hWndView) ::PostMessage(m_hWndView, WM_MOD_VIEWMSG, VIEWMSG_SETFOCUS, 0); }

	virtual void OnOK() {}
	virtual void OnCancel() {}
	virtual void RecalcLayout() {}
	virtual void UpdateView(DWORD, CObject *) {}
	virtual CRuntimeClass *GetAssociatedViewClass() { return NULL; }
	virtual LRESULT OnModCtrlMsg(WPARAM wParam, LPARAM lParam);
	virtual void OnActivatePage(LPARAM) {}
	virtual void OnDeactivatePage() {}
	virtual BOOL OnInitDialog();
	virtual int OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;
	virtual BOOL GetToolTipText(UINT, LPSTR) { return FALSE; }
	//}}AFX_VIRTUAL
	//{{AFX_MSG(CModControlDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnUnlockControls(WPARAM, LPARAM) { if (m_nLockCount > 0) m_nLockCount--; return 0; }
	afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//================================
class CModTabCtrl: public CTabCtrl
//================================
{
public:
	CModTabCtrl() {}

public:
	BOOL InsertItem(int nIndex, LPSTR pszText, LPARAM lParam=0, int iImage=-1);
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	UINT GetItemData(int nIndex);
};


//=================================
class CModControlView: public CView
//=================================
{
protected:
	enum { MAX_PAGES=5 };

protected:
	CModTabCtrl m_TabCtrl;
	CModControlDlg *m_Pages[MAX_PAGES];
	int m_nActiveDlg, m_nInstrumentChanged;
	HWND m_hWndView, m_hWndMDI;

protected: // create from serialization only
	CModControlView();
	DECLARE_DYNCREATE(CModControlView)

public:
	virtual ~CModControlView() {}
	CModDoc* GetDocument() const { return (CModDoc *)m_pDocument; }
	void InstrumentChanged(int nInstr=-1) { m_nInstrumentChanged = nInstr; }
	int GetInstrumentChange() const { return m_nInstrumentChanged; }
	void SetMDIParentFrame(HWND hwnd) { m_hWndMDI = hwnd; }

protected:
	void RecalcLayout();
	void UpdateView(DWORD dwHintMask=0, CObject *pHint=NULL);
	BOOL SetActivePage(int nIndex=-1, LPARAM lParam=-1);

	//{{AFX_VIRTUAL(CModControlView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnDraw(CDC *) {}
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CModControlView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnDestroy();
	afx_msg void OnTabSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEditCut() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_CUT, 0); }
	afx_msg void OnEditCopy() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_COPY, 0); }
	afx_msg void OnEditPaste() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_PASTE, 0); }
	afx_msg void OnEditMixPaste() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_PASTE_SPECIAL, 0); }		//rewbs.mixPaste
	afx_msg void OnEditFind() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_FIND, 0); }
	afx_msg void OnEditFindNext() { if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_FINDNEXT, 0); }
	afx_msg void OnSwitchToView() { if (m_hWndView) ::PostMessage(m_hWndView, WM_MOD_VIEWMSG, VIEWMSG_SETFOCUS, 0); }
	afx_msg LRESULT OnActivateModView(WPARAM, LPARAM);
	afx_msg LRESULT OnModCtrlMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetToolTipText(WPARAM, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// Non-client button attributes
#define NCBTNS_MOUSEOVER		0x01
#define NCBTNS_CHECKED			0x02
#define NCBTNS_DISABLED			0x04
#define NCBTNS_PUSHED			0x08


//======================================
class CModScrollView: public CScrollView
//======================================
{
protected:
	HWND m_hWndCtrl;

public:
	DECLARE_SERIAL(CModScrollView)
	CModScrollView() { m_hWndCtrl = NULL; }
	virtual ~CModScrollView() {}

public:
	CModDoc* GetDocument() const { return (CModDoc *)m_pDocument; }
	LRESULT SendCtrlMessage(UINT uMsg, LPARAM lParam=0) const;
	BOOL PostCtrlMessage(UINT uMsg, LPARAM lParam=0) const;
	void UpdateIndicator(LPCSTR lpszText=NULL);

public:
	//{{AFX_VIRTUAL(CModScrollView)
	virtual void OnDraw(CDC *) {}
	virtual void OnPrepareDC(CDC*, CPrintInfo*) {}
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void UpdateView(DWORD, CObject *) {}
	virtual LRESULT OnModViewMsg(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnDragonDrop(BOOL, LPDRAGONDROP) { return FALSE; }
	virtual LRESULT OnPlayerNotify(MPTNOTIFICATION *) { return 0; }
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CModScrollView)
	afx_msg void OnDestroy();
	afx_msg LRESULT OnReceiveModViewMsg(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnMouseWheel(UINT fFlags, short zDelta, CPoint point);
	afx_msg LRESULT OnDragonDropping(WPARAM bDoDrop, LPARAM lParam) { return OnDragonDrop((BOOL)bDoDrop, (LPDRAGONDROP)lParam); }
	LRESULT OnUpdatePosition(WPARAM, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif
