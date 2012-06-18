/*
 * globals.cpp
 * -----------
 * Purpose: Implementation of various views of the tracker interface.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "childfrm.h"
#include "globals.h"
#include "ctrl_gen.h"
#include "ctrl_pat.h"
#include "ctrl_smp.h"
#include "ctrl_ins.h"
#include "ctrl_com.h"
//#include "ctrl_graph.h" //rewbs.graph
#include "globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CModControlDlg

BEGIN_MESSAGE_MAP(CModControlDlg, CDialog)
	//{{AFX_MSG_MAP(CModControlDlg)
	ON_WM_SIZE()
	ON_MESSAGE(WM_MOD_UNLOCKCONTROLS,		OnUnlockControls)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CModControlDlg::CModControlDlg()
//------------------------------
{
	m_pParent = NULL;
	m_pModDoc = NULL;
	m_pSndFile = NULL;
	m_bInitialized = FALSE;
	m_hWndView = NULL;
	m_nLockCount = 0;
}


CModControlDlg::~CModControlDlg()
//-------------------------------
{
	ASSERT(m_hWnd == NULL);
}


BOOL CModControlDlg::OnInitDialog()
//---------------------------------
{
	CDialog::OnInitDialog();
	EnableToolTips(TRUE);
	return TRUE;
}


void CModControlDlg::OnSize(UINT nType, int cx, int cy)
//-----------------------------------------------------
{
	CDialog::OnSize(nType, cx, cy);
	if (((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0))
	{
		RecalcLayout();
	}
}


LRESULT CModControlDlg::OnModCtrlMsg(WPARAM wParam, LPARAM lParam)
//----------------------------------------------------------------
{
	switch(wParam)
	{
	case CTRLMSG_SETVIEWWND:
		m_hWndView = (HWND)lParam;
		break;

	case CTRLMSG_ACTIVATEPAGE:
		OnActivatePage(lParam);
		break;

	case CTRLMSG_DEACTIVATEPAGE:
		OnDeactivatePage();
		break;
	}
	return 0;
}


LRESULT CModControlDlg::SendViewMessage(UINT uMsg, LPARAM lParam) const
//---------------------------------------------------------------------
{
	if (m_hWndView)	return ::SendMessage(m_hWndView, WM_MOD_VIEWMSG, uMsg, lParam);
	return 0;
}


BOOL CModControlDlg::PostViewMessage(UINT uMsg, LPARAM lParam) const
//------------------------------------------------------------------
{
	if (m_hWndView)	return ::PostMessage(m_hWndView, WM_MOD_VIEWMSG, uMsg, lParam);
	return FALSE;
}


int CModControlDlg::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
//------------------------------------------------------------------
{
	int nHit = CDialog::OnToolHitTest(point, pTI);
	if ((nHit >= 0) && (pTI))
	{
		if ((pTI->lpszText == LPSTR_TEXTCALLBACK) && (pTI->hwnd == m_hWnd))
		{
			CFrameWnd *pMDIParent = GetParentFrame();
			if (pMDIParent) pTI->hwnd = pMDIParent->m_hWnd;
		}
	}
	return nHit;
}


BOOL CModControlDlg::OnToolTipText(UINT nID, NMHDR* pNMHDR, LRESULT* pResult)
//---------------------------------------------------------------------------
{
	CChildFrame *pChildFrm = (CChildFrame *)GetParentFrame();
	if (pChildFrm) return pChildFrm->OnToolTipText(nID, pNMHDR, pResult);
	if (pResult) *pResult = 0;
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CModControlView

BOOL CModTabCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
//-----------------------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (!pMainFrm) return FALSE;
	if (!CTabCtrl::Create(dwStyle, rect, pParentWnd, nID)) return FALSE;
	SendMessage(WM_SETFONT, (WPARAM)pMainFrm->GetGUIFont());
	SetImageList(pMainFrm->GetImageList());
	return TRUE;
}


BOOL CModTabCtrl::InsertItem(int nIndex, LPSTR pszText, LPARAM lParam, int iImage)
//--------------------------------------------------------------------------------
{
	TC_ITEM tci;
	tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tci.pszText = pszText;
	tci.lParam = lParam;
	tci.iImage = iImage;
	return CTabCtrl::InsertItem(nIndex, &tci);
}


UINT CModTabCtrl::GetItemData(int nIndex)
//---------------------------------------
{
	TC_ITEM tci;
	tci.mask = TCIF_PARAM;
	tci.lParam = 0;
	if (!GetItem(nIndex, &tci)) return 0;
	return tci.lParam;
}


/////////////////////////////////////////////////////////////////////////////////
// CModControlView

IMPLEMENT_DYNCREATE(CModControlView, CView)

BEGIN_MESSAGE_MAP(CModControlView, CView)
	//{{AFX_MSG_MAP(CModControlView)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TABCTRL1,	OnTabSelchange)
	ON_MESSAGE(WM_MOD_ACTIVATEVIEW,			OnActivateModView)
	ON_MESSAGE(WM_MOD_CTRLMSG,				OnModCtrlMsg)
	ON_MESSAGE(WM_MOD_GETTOOLTIPTEXT,		OnGetToolTipText)
	ON_COMMAND(ID_EDIT_CUT,					OnEditCut)
	ON_COMMAND(ID_EDIT_COPY,				OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE,				OnEditPaste)
	ON_COMMAND(ID_EDIT_MIXPASTE,			OnEditMixPaste)
	ON_COMMAND(ID_EDIT_MIXPASTE_ITSTYLE,	OnEditMixPasteITStyle)
	ON_COMMAND(ID_EDIT_FIND,				OnEditFind)
	ON_COMMAND(ID_EDIT_FINDNEXT,			OnEditFindNext)
	ON_COMMAND(ID_CONTROLTAB,				OnSwitchToView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CModControlView::CModControlView()
//--------------------------------
{
	MemsetZero(m_Pages);
	m_nActiveDlg = -1;
	m_nInstrumentChanged = -1;
	m_hWndView = NULL;
	m_hWndMDI = NULL;
}


BOOL CModControlView::PreCreateWindow(CREATESTRUCT& cs)
//-----------------------------------------------------
{
	return CView::PreCreateWindow(cs);
}


void CModControlView::OnInitialUpdate() // called first time after construct
//-------------------------------------
{
	CRect rect;

	CChildFrame *pParentFrame = (CChildFrame *)GetParentFrame();
	if (pParentFrame) m_hWndView = pParentFrame->GetHwndView();
	GetClientRect(&rect);
	m_TabCtrl.Create(WS_CHILD|WS_VISIBLE|TCS_FOCUSNEVER|TCS_FORCELABELLEFT, rect, this, IDC_TABCTRL1);
	UpdateView(HINT_MODTYPE);
	SetActivePage(0);
}


void CModControlView::OnSize(UINT nType, int cx, int cy)
//------------------------------------------------------
{
	CView::OnSize(nType, cx, cy);
	if (((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0))
	{
		RecalcLayout();
	}
}


void CModControlView::RecalcLayout()
//----------------------------------
{
	CRect rcClient;

	if (m_TabCtrl.m_hWnd == NULL) return;
	GetClientRect(&rcClient);
	if ((m_nActiveDlg >= 0) && (m_nActiveDlg < MAX_PAGES) && (m_Pages[m_nActiveDlg]))
	{
		CWnd *pDlg = m_Pages[m_nActiveDlg];
		CRect rect = rcClient;
		m_TabCtrl.AdjustRect(FALSE, &rect);
		HDWP hdwp = BeginDeferWindowPos(2);
		DeferWindowPos(hdwp, m_TabCtrl.m_hWnd, NULL, rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), SWP_NOZORDER);
		DeferWindowPos(hdwp, pDlg->m_hWnd, NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER);
		EndDeferWindowPos(hdwp);
	} else
	{
		m_TabCtrl.MoveWindow(&rcClient);
	}
}


void CModControlView::OnUpdate(CView*, LPARAM lHint, CObject*pHint)
//-----------------------------------------------------------------
{
	UpdateView(lHint, pHint);
}


void CModControlView::ForceRefresh() 
//---------------------------------
{
	SetActivePage(GetActivePage());
}

int  CModControlView::GetActivePage() 
//-----------------------------------
{
	return m_nActiveDlg;
}

BOOL CModControlView::SetActivePage(int nIndex, LPARAM lParam)
//------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModControlDlg *pDlg = NULL;
	
	
	if (nIndex == -1) nIndex = m_TabCtrl.GetCurSel();

	const UINT nID = m_TabCtrl.GetItemData(nIndex);
	if(nID == 0) return FALSE;

	switch(nID)
	{
		//rewbs.graph
		case IDD_CONTROL_GRAPH:
			nIndex = 5;
			break;
		//end rewbs.graph
		case IDD_CONTROL_COMMENTS:
			nIndex = 4;
			break;
		case IDD_CONTROL_GLOBALS:
			nIndex = 0;
			break;
		case IDD_CONTROL_PATTERNS:
			nIndex = 1;
			break;
		case IDD_CONTROL_SAMPLES:
			nIndex = 2;
			break;
		case IDD_CONTROL_INSTRUMENTS:
			nIndex = 3;
			break;
		default:
			return FALSE;
	}

	if ((nIndex < 0) || (nIndex >= MAX_PAGES) || (!pMainFrm)) return FALSE;

	//rewbs.varWindowSize
	if (m_Pages[m_nActiveDlg])
		*(m_Pages[m_nActiveDlg]->GetSplitPosRef()) = ((CChildFrame *)GetParentFrame())->GetSplitterHeight();

	if (nIndex == m_nActiveDlg)
	{
		pDlg = m_Pages[m_nActiveDlg];
		PostMessage(WM_MOD_CTRLMSG, CTRLMSG_ACTIVATEPAGE, lParam);
		return TRUE;
	}
	if ((m_nActiveDlg >= 0) && (m_nActiveDlg < MAX_PAGES))
	{
		if (m_Pages[m_nActiveDlg])
		{
			OnModCtrlMsg(CTRLMSG_DEACTIVATEPAGE, 0);
			m_Pages[m_nActiveDlg]->ShowWindow(SW_HIDE);
		}
		m_nActiveDlg = -1;
	}
	if (m_Pages[nIndex]) //Ctrl window already created?
	{
		m_nActiveDlg = nIndex;
		pDlg = m_Pages[nIndex];
	} else //Ctrl window is not created yet - creating one.
	{
		switch(nID)
		{
		//rewbs.graph
		case IDD_CONTROL_GRAPH:
			//pDlg = new CCtrlGraph();
			break;
		//end rewbs.graph
		case IDD_CONTROL_COMMENTS:
			pDlg = new CCtrlComments();
			break;
		case IDD_CONTROL_GLOBALS:
			pDlg = new CCtrlGeneral();
			break;
		case IDD_CONTROL_PATTERNS:
			pDlg = new CCtrlPatterns();
			break;
		case IDD_CONTROL_SAMPLES:
			pDlg = new CCtrlSamples();
			break;
		case IDD_CONTROL_INSTRUMENTS:
			pDlg = new CCtrlInstruments();
			break;
		default:
			return FALSE;
		}
		if (!pDlg) return FALSE;
		pDlg->SetDocument(GetDocument(), this);
		pDlg->SetViewWnd(m_hWndView);
		BOOL bStatus = pDlg->Create(nID, this);
		if(bStatus == 0) // Creation failed.
		{
			delete pDlg;
			return FALSE;
		}
		m_nActiveDlg = nIndex;
		m_Pages[nIndex] = pDlg;
	}
	RecalcLayout();
	pMainFrm->SetUserText("");
	pMainFrm->SetInfoText("");
	pMainFrm->SetXInfoText(""); //rewbs.xinfo
	pDlg->ShowWindow(SW_SHOW);
	((CChildFrame *)GetParentFrame())->SetSplitterHeight(*(pDlg->GetSplitPosRef()));	//rewbs.varWindowSize	
	if (m_hWndMDI) ::PostMessage(m_hWndMDI, WM_MOD_CHANGEVIEWCLASS, (WPARAM)lParam, (LPARAM)pDlg);
	return TRUE;
}


void CModControlView::OnDestroy()
//-------------------------------
{
	m_nActiveDlg = -1;
	for (UINT nIndex=0; nIndex<MAX_PAGES; nIndex++)
	{
		CModControlDlg *pDlg = m_Pages[nIndex];
		if (pDlg)
		{
			m_Pages[nIndex] = NULL;
			pDlg->DestroyWindow();
			delete pDlg;
		}
	}
	CView::OnDestroy();
}


void CModControlView::UpdateView(DWORD lHint, CObject *pObject)
//-------------------------------------------------------------
{
	CWnd *pActiveDlg = NULL;
	CModDoc *pDoc = GetDocument();
	if (!pDoc) return;
	// Module type changed: update tabs
	if (lHint & HINT_MODTYPE)
	{
		CSoundFile *pSndFile = pDoc->GetSoundFile();
		UINT nCount = 4;
		UINT nType = pSndFile->GetType();
		UINT mask = 1 | 2 | 4 | 16;

		if (nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			mask |= 8;
			//mask |= 32; //rewbs.graph
			nCount ++;
		}
		if (nCount != (UINT)m_TabCtrl.GetItemCount())
		{
			UINT count = 0;
			if ((m_nActiveDlg >= 0) && (m_nActiveDlg < MAX_PAGES))
			{
				pActiveDlg = m_Pages[m_nActiveDlg];
				if (pActiveDlg) pActiveDlg->ShowWindow(SW_HIDE);
			}
			m_TabCtrl.DeleteAllItems();
			if (mask & 1) m_TabCtrl.InsertItem(count++, "General", IDD_CONTROL_GLOBALS, IMAGE_GENERAL);
			if (mask & 2) m_TabCtrl.InsertItem(count++, "Patterns", IDD_CONTROL_PATTERNS, IMAGE_PATTERNS);
			if (mask & 4) m_TabCtrl.InsertItem(count++, "Samples", IDD_CONTROL_SAMPLES, IMAGE_SAMPLES);
			if (mask & 8) m_TabCtrl.InsertItem(count++, "Instruments", IDD_CONTROL_INSTRUMENTS, IMAGE_INSTRUMENTS);
			if (mask & 32) m_TabCtrl.InsertItem(count++, "Graph", IDD_CONTROL_GRAPH, IMAGE_GRAPH); //rewbs.graph
			if (mask & 16) m_TabCtrl.InsertItem(count++, "Comments", IDD_CONTROL_COMMENTS, IMAGE_COMMENTS);
		}
	}
	// Update child dialogs
	for (UINT nIndex=0; nIndex<MAX_PAGES; nIndex++)
	{
		CModControlDlg *pDlg = m_Pages[nIndex];
		if ((pDlg) && (pObject != pDlg)) pDlg->UpdateView(lHint, pObject);
	}
	// Restore the displayed child dialog
	if (pActiveDlg) pActiveDlg->ShowWindow(SW_SHOW);
}


void CModControlView::OnTabSelchange(NMHDR*, LRESULT* pResult)
//------------------------------------------------------------
{
	SetActivePage(m_TabCtrl.GetCurSel());
	if (pResult) *pResult = 0;
}


LRESULT CModControlView::OnActivateModView(WPARAM nIndex, LPARAM lParam)
//----------------------------------------------------------------------
{
	if (m_TabCtrl.m_hWnd)
	{
		if (nIndex < 100)
		{
			m_TabCtrl.SetCurSel(nIndex);
			SetActivePage(nIndex, lParam);
		} else
		// Might be a dialog id IDD_XXXX
		{
			int nItems = m_TabCtrl.GetItemCount();
			for (int i=0; i<nItems; i++)
			{
				if (m_TabCtrl.GetItemData(i) == nIndex)
				{
					m_TabCtrl.SetCurSel(i);
					SetActivePage(i, lParam);
					break;
				}
			}
		}
	}
	return 0;
}


LRESULT CModControlView::OnModCtrlMsg(WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------
{
	if ((m_nActiveDlg >= 0) && (m_nActiveDlg < MAX_PAGES))
	{
		CModControlDlg *pActiveDlg = m_Pages[m_nActiveDlg];
		if (pActiveDlg)
		{
			switch(wParam)
			{
			case CTRLMSG_SETVIEWWND:
				{
					m_hWndView = (HWND)lParam;
					for (UINT i=0; i<MAX_PAGES; i++)
					{
						if (m_Pages[i]) m_Pages[i]->SetViewWnd(m_hWndView);
					}
				}
				break;
			}
			return pActiveDlg->OnModCtrlMsg(wParam, lParam);
		}
	}
	return 0;
}


LRESULT CModControlView::OnGetToolTipText(WPARAM uId, LPARAM pszText)
//-------------------------------------------------------------------
{
	if ((m_nActiveDlg >= 0) && (m_nActiveDlg < MAX_PAGES))
	{
		CModControlDlg *pActiveDlg = m_Pages[m_nActiveDlg];
		if (pActiveDlg) return (LRESULT)pActiveDlg->GetToolTipText(uId, (LPSTR)pszText);
	}
	return 0;
}


//////////////////////////////////////////////////////////////////
// CModScrollView

IMPLEMENT_SERIAL(CModScrollView, CScrollView, 0)
BEGIN_MESSAGE_MAP(CModScrollView, CScrollView)
	//{{AFX_MSG_MAP(CModScrollView)
	ON_WM_DESTROY()
	ON_WM_MOUSEWHEEL()
	ON_MESSAGE(WM_MOD_VIEWMSG,			OnReceiveModViewMsg)
	ON_MESSAGE(WM_MOD_DRAGONDROPPING,	OnDragonDropping)
	ON_MESSAGE(WM_MOD_UPDATEPOSITION,	OnUpdatePosition)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


LRESULT CModScrollView::SendCtrlMessage(UINT uMsg, LPARAM lParam) const
//---------------------------------------------------------------------
{
	if (m_hWndCtrl)	return ::SendMessage(m_hWndCtrl, WM_MOD_CTRLMSG, uMsg, lParam);
	return 0;
}


BOOL CModScrollView::PostCtrlMessage(UINT uMsg, LPARAM lParam) const
//------------------------------------------------------------------
{
	if (m_hWndCtrl)	return ::PostMessage(m_hWndCtrl, WM_MOD_CTRLMSG, uMsg, lParam);
	return FALSE;
}


LRESULT CModScrollView::OnReceiveModViewMsg(WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------
{
	return OnModViewMsg(wParam, lParam);
}


void CModScrollView::OnUpdate(CView* pView, LPARAM lHint, CObject*pHint)
//----------------------------------------------------------------------
{
	if (pView != this) UpdateView(lHint, pHint);
}


LRESULT CModScrollView::OnModViewMsg(WPARAM wParam, LPARAM lParam)
//----------------------------------------------------------------
{
	switch(wParam)
	{
	case VIEWMSG_SETCTRLWND:
		m_hWndCtrl = (HWND)lParam;
		break;

	case VIEWMSG_SETFOCUS:
	case VIEWMSG_SETACTIVE:
		GetParentFrame()->SetActiveView(this);
		SetFocus();
		break;
	}
	return 0;
}


void CModScrollView::UpdateIndicator(LPCSTR lpszText)
//---------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->SetUserText((lpszText) ? lpszText : "");
}


BOOL CModScrollView::OnMouseWheel(UINT fFlags, short zDelta, CPoint point)
//------------------------------------------------------------------------
{
	// we don't handle anything but scrolling just now
	if (fFlags & (MK_SHIFT | MK_CONTROL)) return FALSE;

	//if the parent is a splitter, it will handle the message
	//if (GetParentSplitter(this, TRUE)) return FALSE;

	// we can't get out of it--perform the scroll ourselves
	return DoMouseWheel(fFlags, zDelta, point);
}


void CModScrollView::OnDestroy()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if ((pMainFrm) && (pModDoc))
	{
		if (pMainFrm->GetFollowSong(pModDoc) == m_hWnd)
		{
			pMainFrm->SetFollowSong(pModDoc, NULL, FALSE);
			pModDoc->SetFollowWnd(NULL, 0);
		}
		if (pMainFrm->GetMidiRecordWnd() == m_hWnd)
		{
			pMainFrm->SetMidiRecordWnd(NULL);
		}
	}
	CScrollView::OnDestroy();
}


LRESULT CModScrollView::OnUpdatePosition(WPARAM, LPARAM lParam)
//-------------------------------------------------------------
{
	MPTNOTIFICATION *pnotify = (MPTNOTIFICATION *)lParam;
	if (pnotify) return OnPlayerNotify(pnotify);
	return 0;
}


////////////////////////////////////////////////////////////////////////////
// 	CModControlBar

BEGIN_MESSAGE_MAP(CModControlBar, CToolBarCtrl)
	ON_MESSAGE(WM_HELPHITTEST,	OnHelpHitTest)
END_MESSAGE_MAP()


CModControlBar::~CModControlBar()
//-------------------------------
{
	if (m_hBarBmp)
	{
		DeleteObject(m_hBarBmp);
		m_hBarBmp = NULL;
	}
}


BOOL CModControlBar::Init(UINT nId)
//---------------------------------
{
	HINSTANCE hInstance = AfxGetInstanceHandle();
	TBADDBITMAP tbab;
	
	SetButtonStructSize(sizeof(TBBUTTON));
	SetBitmapSize(CSize(16, 15));
	SetButtonSize(CSize(27, 24));
	// Add bitmaps
	m_hBarBmp = AfxLoadSysColorBitmap(
					hInstance,
					::FindResource(hInstance, MAKEINTRESOURCE(nId), RT_BITMAP));
	tbab.hInst = NULL;
	tbab.nID = (UINT)m_hBarBmp;
	::SendMessage(m_hWnd, TB_ADDBITMAP, 16, (LPARAM)&tbab);
	UpdateStyle();
	return TRUE;
}


BOOL CModControlBar::AddButton(UINT nID, int iImage, UINT nStyle, UINT nState)
//----------------------------------------------------------------------------
{
	TBBUTTON btn;

	btn.iBitmap = iImage;
	btn.idCommand = nID;
	btn.fsStyle = (BYTE)nStyle;
	btn.fsState = (BYTE)nState;
	btn.dwData = 0;
	btn.iString = 0;
	return AddButtons(1, &btn);
}
	

void CModControlBar::UpdateStyle()
//--------------------------------
{
	if (m_hWnd)
	{
		LONG lStyleOld = GetWindowLong(m_hWnd, GWL_STYLE);
		if (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_FLATBUTTONS)
			lStyleOld |= TBSTYLE_FLAT;
		else
			lStyleOld &= ~TBSTYLE_FLAT;
		lStyleOld |= CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER | TBSTYLE_TOOLTIPS;
		SetWindowLong(m_hWnd, GWL_STYLE, lStyleOld);
		Invalidate();
	}
}


LRESULT CModControlBar::OnHelpHitTest(WPARAM, LPARAM lParam)
//----------------------------------------------------------
{
	TBBUTTON tbbn;
	POINT point;

	point.x = (signed short)(LOWORD(lParam));
	point.y = (signed short)(HIWORD(lParam));
	int ndx = HitTest(&point);
	if ((ndx >= 0) && (GetButton(ndx, &tbbn)))
	{
		return HID_BASE_COMMAND + tbbn.idCommand;
	}
	return 0;
}
