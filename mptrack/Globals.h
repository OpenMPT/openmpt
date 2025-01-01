/*
 * Globals.h
 * ---------
 * Purpose: Implementation of the base classes for the upper and lower half of the MDI child windows.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "DialogBase.h"
#include "Settings.h"
#include "UpdateHints.h"

OPENMPT_NAMESPACE_BEGIN

#define ID_EDIT_MIXPASTE ID_EDIT_PASTE_SPECIAL

class CModControlView;
class CModDoc;
class CSoundFile;
struct DRAGONDROP;
struct Notification;

class CModControlBar: public CToolBarCtrl
{
public:
	BOOL Init(CImageList &icons, CImageList &disabledIcons);
	void UpdateStyle();
	BOOL AddButton(UINT nID, int iImage=0, UINT nStyle=TBSTYLE_BUTTON, UINT nState=TBSTATE_ENABLED);
	void OnDPIChanged();
};


class CModControlDlg : public DialogBase
{
protected:
	CModDoc &m_modDoc;
	CSoundFile &m_sndFile;
	CModControlView &m_parent;
	HWND m_hWndView = nullptr;
	HWND m_lastFocusItem = nullptr;
	int m_nLockCount = 0;
	bool m_initialized = false;

public:
	CModControlDlg(CModControlView &parent, CModDoc &document);
	virtual ~CModControlDlg();
	
public:
	void SetViewWnd(HWND hwndView) { m_hWndView = hwndView; }
	HWND GetViewWnd() const { return m_hWndView; }
	LRESULT SendViewMessage(UINT uMsg, LPARAM lParam = 0) const;
	BOOL PostViewMessage(UINT uMsg, LPARAM lParam = 0) const;
	LRESULT SwitchToView() const;
	void LockControls() { m_nLockCount++; }
	void UnlockControls();
	bool IsLocked() const { return (m_nLockCount > 0); }
	virtual Setting<LONG> &GetSplitPosRef() = 0;

	void SaveLastFocusItem(HWND hwnd);
	void ForgetLastFocusItem() { m_lastFocusItem = nullptr; }
	void RestoreLastFocusItem();

	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditMixPaste();
	afx_msg void OnEditMixPasteITStyle();
	afx_msg void OnEditPasteFlood();
	afx_msg void OnEditPushForwardPaste();
	afx_msg void OnEditFind();
	afx_msg void OnEditFindNext();
	afx_msg void OnSwitchToView();

	//{{AFX_VIRTUAL(CModControlDlg)
	void OnOK() override {}
	void OnCancel() override {}
	void OnDPIChanged() override { RecalcLayout(); }
	virtual void RecalcLayout() = 0;
	virtual void UpdateView(UpdateHint, CObject *) = 0;
	virtual CRuntimeClass *GetAssociatedViewClass() { return nullptr; }
	virtual LRESULT OnModCtrlMsg(WPARAM wParam, LPARAM lParam);
	virtual void OnActivatePage(LPARAM) {}
	virtual void OnDeactivatePage() {}
	//}}AFX_VIRTUAL
	//{{AFX_MSG(CModControlDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnUnlockControls(WPARAM, LPARAM) { if (m_nLockCount > 0) m_nLockCount--; return 0; }
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


class CModTabCtrl: public CTabCtrl
{
public:
	BOOL InsertItem(int nIndex, LPCTSTR pszText, LPARAM lParam=0, int iImage=-1);
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	LPARAM GetItemData(int nIndex);
	void OnDPIChanged();
};


class CModControlView: public CView
{
public:
	// Note: Page IDs are serialized to module window settings!
	enum class Page
	{
		Unknown = -1,
		First = 0,
		Globals = First,
		Patterns,
		Samples,
		Instruments,
		Comments,
		NumPages
	};

protected:
	CModTabCtrl m_TabCtrl;
	std::array<CModControlDlg *, int(Page::NumPages)> m_Pages = {{}};
	Page m_nActiveDlg = Page::Unknown;
	int m_nInstrumentChanged = -1;
	HWND m_hWndView = nullptr, m_hWndMDI = nullptr;

protected: // create from serialization only
	CModControlView() = default;
	DECLARE_DYNCREATE(CModControlView)

public:
	virtual ~CModControlView() = default;
	CModDoc *GetDocument() const noexcept;
	void SampleChanged(SAMPLEINDEX smp);
	void InstrumentChanged(int nInstr = -1) { m_nInstrumentChanged = nInstr; }
	int GetInstrumentChange() const { return m_nInstrumentChanged; }
	void SetMDIParentFrame(HWND hwnd) { m_hWndMDI = hwnd; }
	void ForceRefresh();
	CModControlDlg *GetCurrentControlDlg() const;

protected:
	void RecalcLayout();
	void UpdateView(UpdateHint hint, CObject *pHint = nullptr);
	bool SetActivePage(Page page = Page::Unknown, LPARAM lParam = -1);
public:
	Page GetActivePage() const { return m_nActiveDlg; }

protected:
	//{{AFX_VIRTUAL(CModControlView)
protected:
	void OnInitialUpdate() override;
	void OnDraw(CDC *) override {}
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CModControlView)
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnDPIChangedAfterParent(WPARAM = 0, LPARAM = 0);
	afx_msg void OnDestroy();
	afx_msg void OnTabSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditMixPaste();
	afx_msg void OnEditMixPasteITStyle();
	afx_msg void OnEditFind();
	afx_msg void OnEditFindNext();
	afx_msg void OnSwitchToView();
	afx_msg LRESULT OnActivateModView(WPARAM, LPARAM);
	afx_msg LRESULT OnModCtrlMsg(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// Non-client button attributes
#define NCBTNS_MOUSEOVER		0x01
#define NCBTNS_CHECKED			0x02
#define NCBTNS_DISABLED			0x04
#define NCBTNS_PUSHED			0x08


class CModScrollView: public CScrollView
{
protected:
	HWND m_hWndCtrl = nullptr;
	HWND m_lastFocusItem = nullptr;
	int m_nScrollPosX = 0, m_nScrollPosY = 0;
	int m_nScrollPosXfine = 0, m_nScrollPosYfine = 0;
	int m_dpi = 0;  // Cached DPI settings

public:
	DECLARE_SERIAL(CModScrollView)
	CModScrollView() = default;
	virtual ~CModScrollView() = default;

public:
	CModDoc *GetDocument() const noexcept;
	void SendCtrlCommand(int id) const;
	LRESULT SendCtrlMessage(UINT uMsg, LPARAM lParam = 0) const;
	BOOL PostCtrlMessage(UINT uMsg, LPARAM lParam = 0) const;
	void UpdateIndicator(LPCTSTR lpszText = nullptr);

public:
	//{{AFX_VIRTUAL(CModScrollView)
	void OnInitialUpdate() override;
	BOOL PreTranslateMessage(MSG *pMsg) override;
	void OnDraw(CDC *) override {}
	void OnPrepareDC(CDC *, CPrintInfo *) override {}
	void OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint) override;
	virtual void OnDPIChanged() { Invalidate(); }
	virtual void UpdateView(UpdateHint, CObject *) {}
	virtual LRESULT OnModViewMsg(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnDragonDrop(BOOL, const DRAGONDROP *) { return FALSE; }
	virtual LRESULT OnPlayerNotify(Notification *) { return 0; }
	//}}AFX_VIRTUAL

	CModControlDlg *GetControlDlg() { return static_cast<CModControlView *>(CWnd::FromHandle(m_hWndCtrl))->GetCurrentControlDlg(); }

	void SaveLastFocusItem(HWND hwnd);

protected:
	//{{AFX_MSG(CModScrollView)
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg LRESULT OnReceiveModViewMsg(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnMouseWheel(UINT fFlags, short zDelta, CPoint point);
	afx_msg void OnMouseHWheel(UINT fFlags, short zDelta, CPoint point);
	afx_msg LRESULT OnDragonDropping(WPARAM bDoDrop, LPARAM lParam) { return OnDragonDrop((BOOL)bDoDrop, (const DRAGONDROP *)lParam); }
	afx_msg LRESULT OnUpdatePosition(WPARAM, LPARAM);
	afx_msg LRESULT OnDPIChangedAfterParent(WPARAM, LPARAM);
	//}}AFX_MSG

	// Fixes for 16-bit limitation in MFC's CScrollView and support for fractional mouse wheel movements
	BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE) override;
	BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE) override;
	int SetScrollPos(int nBar, int nPos, BOOL bRedraw = TRUE);
	void SetScrollSizes(int nMapMode, SIZE sizeTotal, const SIZE& sizePage = CScrollView::sizeDefault, const SIZE& sizeLine = CScrollView::sizeDefault);

	BOOL OnGesturePan(CPoint ptFrom, CPoint ptTo) override;

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


OPENMPT_NAMESPACE_END
