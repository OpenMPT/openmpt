// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdafx.h"
#include <afxpriv.h>
#include "mptrack.h"
#include "mainfrm.h"
#include "ChildFrm.h"
#include "moddoc.h"
#include "globals.h"
#include "view_gen.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// 

IMPLEMENT_DYNAMIC(CViewExSplitWnd, CSplitterWnd)

CWnd* CViewExSplitWnd::GetActivePane(int*, int*)	// pRow, pCol
//----------------------------------------------
{
	// attempt to use active view of frame window
	CWnd* pView = NULL;
 	CFrameWnd* pFrameWnd = GetParentFrame();
	ASSERT_VALID(pFrameWnd);
	if (pFrameWnd) pView = pFrameWnd->GetActiveView();

	// failing that, use the current focus
	if (pView == NULL)
		pView = GetFocus();

	return pView;
}



/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CChildFrame)
	ON_WM_CLOSE()
	ON_WM_NCACTIVATE()
	ON_MESSAGE(WM_MOD_CHANGEVIEWCLASS,	OnChangeViewClass)
	ON_MESSAGE(WM_MOD_INSTRSELECTED,	OnInstrumentSelected)
	// toolbar "tooltip" notification
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

LONG CChildFrame::glMdiOpenCount = 0;

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
//------------------------
{
	m_szCurrentViewClassName[0] = 0;
	m_hWndCtrl = m_hWndView = NULL;
	m_bMaxWhenClosed = FALSE;
	glMdiOpenCount++;
	RtlZeroMemory(&m_ViewGeneral, sizeof(m_ViewGeneral));
	RtlZeroMemory(&m_ViewPatterns, sizeof(m_ViewPatterns));
	RtlZeroMemory(&m_ViewSamples, sizeof(m_ViewSamples));
	RtlZeroMemory(&m_ViewInstruments, sizeof(m_ViewInstruments));
	RtlZeroMemory(&m_ViewComments, sizeof(m_ViewComments));
}


CChildFrame::~CChildFrame()
//-------------------------
{
	if ((--glMdiOpenCount) == 0)
	{
		CMainFrame::gbMdiMaximize = m_bMaxWhenClosed;
	}
}


BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
//-----------------------------------------------------------------------------
{
	// create a splitter with 1 row, 2 columns
	if (!m_wndSplitter.CreateStatic(this, 2, 1)) return FALSE;

	// add the first splitter pane - the default view in row 0
	int cy = CMainFrame::glCtrlWindowHeight;
	if (cy <= 1) cy = (lpcs->cy*2) / 3;
	if (!m_wndSplitter.CreateView(0, 0, pContext->m_pNewViewClass, CSize(0, cy), pContext)) return FALSE;
	
	// Get 2nd window handle
	CModControlView *pModView;
	if ((pModView = (CModControlView *)m_wndSplitter.GetPane(0, 0)) != NULL)
	{
		m_hWndCtrl = pModView->m_hWnd;
		pModView->SetMDIParentFrame(m_hWnd);
	}

	ChangeViewClass(RUNTIME_CLASS(CViewGlobals), pContext);

	// it all worked, we now have a splitter window which contain two different views
	return TRUE;
}


BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
//-------------------------------------------------
{
	return CMDIChildWnd::PreCreateWindow(cs);
}


void CChildFrame::ActivateFrame(int nCmdShow)
//-------------------------------------------
{
	if ((glMdiOpenCount == 1) && (CMainFrame::gbMdiMaximize) && (nCmdShow == -1))
	{
		nCmdShow = SW_SHOWMAXIMIZED;
	}
	CMDIChildWnd::ActivateFrame(nCmdShow);
}


void CChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
//----------------------------------------------------
{
	// update our parent window first
	GetMDIFrame()->OnUpdateFrameTitle(bAddToTitle);

	if ((GetStyle() & FWS_ADDTOTITLE) == 0)	return;     // leave child window alone!

	CDocument* pDocument = GetActiveDocument();
	if (bAddToTitle)
	{
		TCHAR szText[256+_MAX_PATH];
		if (pDocument == NULL)
			lstrcpy(szText, m_strTitle);
		else
			lstrcpy(szText, pDocument->GetTitle());
		if (pDocument->IsModified()) lstrcat(szText, "*");
		if (m_nWindow > 0)
			wsprintf(szText + lstrlen(szText), _T(":%d"), m_nWindow);

		// set title if changed, but don't remove completely
		AfxSetWindowText(m_hWnd, szText);
	}
}


BOOL CChildFrame::ChangeViewClass(CRuntimeClass* pViewClass, CCreateContext* pContext)
//------------------------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CWnd *pWnd;
	if (!strcmp(pViewClass->m_lpszClassName, m_szCurrentViewClassName)) return TRUE;
	if (m_szCurrentViewClassName[0])
	{
		m_szCurrentViewClassName[0] = 0;
		m_wndSplitter.DeleteView(1, 0);
	}
	if ((m_hWndView) && (pMainFrm))
	{
		if (pMainFrm->GetMidiRecordWnd() == m_hWndView)
		{
			pMainFrm->SetMidiRecordWnd(NULL);
		}
	}
	m_hWndView = NULL;
	if (!m_wndSplitter.CreateView(1, 0, pViewClass, CSize(0, 0), pContext)) return FALSE;
	// Get 2nd window handle
	if ((pWnd = m_wndSplitter.GetPane(1, 0)) != NULL) m_hWndView = pWnd->m_hWnd;
	strcpy(m_szCurrentViewClassName, pViewClass->m_lpszClassName);
	m_wndSplitter.RecalcLayout();
	if ((m_hWndView) && (m_hWndCtrl))
	{
		::PostMessage(m_hWndView, WM_MOD_VIEWMSG, VIEWMSG_SETCTRLWND, (LPARAM)m_hWndCtrl);
		::PostMessage(m_hWndCtrl, WM_MOD_CTRLMSG, CTRLMSG_SETVIEWWND, (LPARAM)m_hWndView);
		pMainFrm->SetMidiRecordWnd(m_hWndView);
	}
	return TRUE;
}


void CChildFrame::SavePosition(BOOL bForce)
//-----------------------------------------
{
	if (m_hWnd)
	{
		CRect rect;
		
		m_bMaxWhenClosed = IsZoomed();
		if (bForce) CMainFrame::gbMdiMaximize = m_bMaxWhenClosed;
		if (!IsIconic())
		{
			CWnd *pWnd = m_wndSplitter.GetPane(0, 0);
			if (pWnd)
			{
				pWnd->GetWindowRect(&rect);
				LONG l = rect.Height();
				if (l > 15) CMainFrame::glCtrlWindowHeight = l;
			}
		}
	}
}


LRESULT CChildFrame::SendViewMessage(UINT uMsg, LPARAM lParam) const
//------------------------------------------------------------------
{
	if (m_hWndView)	return ::SendMessage(m_hWndView, WM_MOD_VIEWMSG, uMsg, lParam);
	return 0;
}


LRESULT CChildFrame::OnInstrumentSelected(WPARAM wParam, LPARAM lParam)
//---------------------------------------------------------------------
{
	CView *pView = GetActiveView();
	CModDoc *pModDoc = NULL;
	if (pView) pModDoc = (CModDoc *)pView->GetDocument();
	if ((m_hWndCtrl) && (pModDoc))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nIns = lParam;

		if ((!wParam) && (pSndFile->m_nInstruments > 0))
		{
			nIns = pModDoc->FindSampleParent(nIns);
		}
		::SendMessage(m_hWndCtrl, WM_MOD_CTRLMSG, CTRLMSG_PAT_SETINSTRUMENT, nIns);
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

void CChildFrame::OnClose()
//-------------------------
{
	SavePosition();
	CMDIChildWnd::OnClose();
}


BOOL CChildFrame::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
//--------------------------------------------------------------------
{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	TCHAR szFullText[256];
	CString strTipText;

	ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);
	szFullText[0] = 0;
	UINT nID = pNMHDR->idFrom;
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = (UINT)::GetDlgCtrlID((HWND)nID);
	}

	if ((nID >= 1000) && (nID < 10000) && (m_hWndCtrl) && (::SendMessage(m_hWndCtrl, WM_MOD_GETTOOLTIPTEXT, nID, (LPARAM)szFullText)))
	{
		strTipText = szFullText;
	} else
	{
		// allow top level routing frame to handle the message
		if (GetRoutingFrame() != NULL) return FALSE;
		if (nID != 0) // will be zero on a separator
		{
			AfxLoadString(nID, szFullText);
			// this is the command id, not the button index
			AfxExtractSubString(strTipText, szFullText, 1, '\n');
		}
	}
	if (pNMHDR->code == TTN_NEEDTEXTA)
		lstrcpyn(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
	else
		_mbstowcsz(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
	*pResult = 0;

	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);

	return TRUE;    // message was handled
}


LRESULT CChildFrame::OnChangeViewClass(WPARAM wParam, LPARAM lParam)
//------------------------------------------------------------------
{
	CModControlDlg *pDlg = (CModControlDlg *)lParam;
	if (pDlg)
	{
		CRuntimeClass *pNewViewClass = pDlg->GetAssociatedViewClass();
		if (pNewViewClass) ChangeViewClass(pNewViewClass);
		::PostMessage(m_hWndCtrl, WM_MOD_CTRLMSG, CTRLMSG_ACTIVATEPAGE, (LPARAM)wParam);
	}
	return 0;
}


BOOL CChildFrame::OnNcActivate(BOOL bActivate)
//--------------------------------------------
{
	if (m_hWndView)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm)
		{
			if (bActivate) pMainFrm->SetMidiRecordWnd(m_hWndView);
		}
	}
	return CMDIChildWnd::OnNcActivate(bActivate);
}


