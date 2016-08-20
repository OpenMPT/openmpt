/*
 * ChildFrm.cpp
 * ------------
 * Purpose: Implementation of tab interface class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include <afxpriv.h>
#include "Mptrack.h"
#include "Mainfrm.h"
#include "ChildFrm.h"
#include "Moddoc.h"
#include "Globals.h"
#include "View_gen.h"
#include "Ctrl_pat.h"
#include "View_pat.h"
#include "Ctrl_smp.h"
#include "View_smp.h"
#include "Ctrl_ins.h"
#include "View_ins.h"
#include "View_com.h"
#include "Childfrm.h"

#include "../common/FileReader.h"
#include "../common/mptIO.h"
#include "../common/mptBufferIO.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


OPENMPT_NAMESPACE_BEGIN


/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CChildFrame)
	ON_WM_DESTROY()
	ON_WM_NCACTIVATE()
	ON_MESSAGE(WM_MOD_CHANGEVIEWCLASS,	OnChangeViewClass)
	ON_MESSAGE(WM_MOD_INSTRSELECTED,	OnInstrumentSelected)
	// toolbar "tooltip" notification
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	//}}AFX_MSG_MAP
	ON_WM_SETFOCUS() //rewbs.customKeysAutoEffects
END_MESSAGE_MAP()

int CChildFrame::glMdiOpenCount = 0;

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
//------------------------
{
	m_bInitialActivation=true; //rewbs.fix3185
	m_szCurrentViewClassName[0] = 0;
	m_hWndCtrl = m_hWndView = NULL;
	m_bMaxWhenClosed = false;
	glMdiOpenCount++;
	RtlZeroMemory(&m_ViewGeneral, sizeof(m_ViewGeneral));
	RtlZeroMemory(&m_ViewPatterns, sizeof(m_ViewPatterns));
	RtlZeroMemory(&m_ViewSamples, sizeof(m_ViewSamples));
	RtlZeroMemory(&m_ViewInstruments, sizeof(m_ViewInstruments));
	RtlZeroMemory(&m_ViewComments, sizeof(m_ViewComments));
	m_ViewPatterns.initialOrder = ORDERINDEX_INVALID;
}


CChildFrame::~CChildFrame()
//-------------------------
{
	if ((--glMdiOpenCount) == 0)
	{
		TrackerSettings::Instance().gbMdiMaximize = m_bMaxWhenClosed;
	}
}


BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
//-----------------------------------------------------------------------------
{
	// create a splitter with 2 rows, 1 column
	if (!m_wndSplitter.CreateStatic(this, 2, 1)) return FALSE;

	// add the first splitter pane - the default view in row 0
	int cy = TrackerSettings::Instance().glGeneralWindowHeight;	//rewbs.varWindowSize - default to general tab.
	if (cy <= 1) cy = (lpcs->cy*2) / 3;
	if (!m_wndSplitter.CreateView(0, 0, pContext->m_pNewViewClass, CSize(0, cy), pContext)) return FALSE;

	// Get 2nd window handle
	CModControlView *pModView;
	if ((pModView = GetModControlView()) != NULL)
	{
		m_hWndCtrl = pModView->m_hWnd;
		pModView->SetMDIParentFrame(m_hWnd);
	}

	const BOOL bStatus = ChangeViewClass(RUNTIME_CLASS(CViewGlobals), pContext);

	// If it all worked, we now have a splitter window which contain two different views
	return bStatus;
}


void CChildFrame::SetSplitterHeight(int cy)
//------------------------------------------
{
	if (cy <= 1) cy = 188;	//default to 188? why not..
	m_wndSplitter.SetRowInfo(0, Util::ScalePixels(cy, m_hWnd), 15);
}


BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
//-------------------------------------------------
{
	return CMDIChildWnd::PreCreateWindow(cs);
}


void CChildFrame::ActivateFrame(int nCmdShow)
//-------------------------------------------
{
	if ((glMdiOpenCount == 1) && (TrackerSettings::Instance().gbMdiMaximize) && (nCmdShow == -1))
	{
		nCmdShow = SW_SHOWMAXIMIZED;
	}
	CMDIChildWnd::ActivateFrame(nCmdShow);


	// When song first loads, initialise patternViewState to point to start of song.
	CView *pView = GetActiveView();
	CModDoc *pModDoc = NULL;
	if (pView) pModDoc = (CModDoc *)pView->GetDocument();
	if ((m_hWndCtrl) && (pModDoc))
	{
		if (m_bInitialActivation && m_ViewPatterns.nPattern==0)
		{
			m_ViewPatterns.nPattern=pModDoc->GetSoundFile()->Order[0];
			m_ViewPatterns.nOrder=0; //just in case (should already be 0)
			m_ViewPatterns.cursor.SetRow(0);   //just in case
			m_bInitialActivation=false;
		}
	}
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
		if (pDocument == nullptr)
		{
			lstrcpy(szText, m_strTitle);
		} else
		{
			lstrcpy(szText, pDocument->GetTitle());
			if (pDocument->IsModified()) lstrcat(szText, "*");
		}
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

void CChildFrame::ForceRefresh()
//------------------------------
{
	CModControlView *pModView;
	if ((pModView = GetModControlView()) != nullptr)
	{
		pModView->ForceRefresh();
	}

	return;
}

void CChildFrame::SavePosition(BOOL bForce)
//-----------------------------------------
{
	if (m_hWnd)
	{
		m_bMaxWhenClosed = IsZoomed() != FALSE;
		if (bForce) TrackerSettings::Instance().gbMdiMaximize = m_bMaxWhenClosed;
		if (!IsIconic())
		{
			CWnd *pWnd = m_wndSplitter.GetPane(0, 0);
			if (pWnd)
			{
				CRect rect(0, 0, 0, 0);
				pWnd->GetWindowRect(&rect);
				if(rect.Width() == 0)
					return;
				int l = Util::ScalePixelsInv(rect.Height(), m_hWnd);
				//rewbs.varWindowSize - not the nicest piece of code, but we need to distinguish between the views:
				if (strcmp(CViewGlobals::classCViewGlobals.m_lpszClassName, m_szCurrentViewClassName) == 0)
					TrackerSettings::Instance().glGeneralWindowHeight = l;
				else if (strcmp(CViewPattern::classCViewPattern.m_lpszClassName, m_szCurrentViewClassName) == 0)
					TrackerSettings::Instance().glPatternWindowHeight = l;
				else if (strcmp(CViewSample::classCViewSample.m_lpszClassName, m_szCurrentViewClassName) == 0)
					TrackerSettings::Instance().glSampleWindowHeight = l;
				else if (strcmp(CViewInstrument::classCViewInstrument.m_lpszClassName, m_szCurrentViewClassName) == 0)
					TrackerSettings::Instance().glInstrumentWindowHeight = l;
				else if (strcmp(CViewComments::classCViewComments.m_lpszClassName, m_szCurrentViewClassName) == 0)
					TrackerSettings::Instance().glCommentsWindowHeight = l;
			}
		}
	}
}


int CChildFrame::GetSplitterHeight()
//----------------------------------
{
	if (m_hWnd)
	{
		CRect rect;

		CWnd *pWnd = m_wndSplitter.GetPane(0, 0);
		if (pWnd)
		{
			pWnd->GetWindowRect(&rect);
			return Util::ScalePixelsInv(rect.Height(), m_hWnd);
		}
	}
	return 15;	// tidy default
};


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
			nIns = pModDoc->FindSampleParent(static_cast<SAMPLEINDEX>(nIns));
			if(nIns == INSTRUMENTINDEX_INVALID)
			{
				nIns = 0;
			}
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

void CChildFrame::OnDestroy()
//---------------------------
{
	SavePosition();
	CMDIChildWnd::OnDestroy();
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
	UINT_PTR nID = pNMHDR->idFrom;
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = (UINT_PTR)::GetDlgCtrlID((HWND)nID);
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

//rewbs.varWindowSize
CHAR* CChildFrame::GetCurrentViewClassName()
{
	return m_szCurrentViewClassName;
}
//end rewbs.varWindowSize

//rewbs.customKeysAutoEffects
//We use this to update effect keys when user changes document, if necessary.
void CChildFrame::OnSetFocus(CWnd* pOldWnd)
{
	CMDIChildWnd::OnSetFocus(pOldWnd);

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm)
	{
		pMainFrm->UpdateEffectKeys();
	}
}
//end rewbs.customKeysAutoEffects


std::string CChildFrame::SerializeView() const
//--------------------------------------------
{
	mpt::ostringstream f(std::ios::out | std::ios::binary);
	// Version
	mpt::IO::WriteVarInt(f, 0u);
	// Current page
	mpt::IO::WriteVarInt(f, static_cast<uint8_t>(GetModControlView()->GetActivePage()));

	CModControlView *view = GetModControlView();
	if (strcmp(CViewPattern::classCViewPattern.m_lpszClassName, m_szCurrentViewClassName) == 0)
	{
		mpt::IO::WriteVarInt(f, (uint32_t)view->SendMessage(WM_MOD_CTRLMSG, CTRLMSG_GETCURRENTORDER));	// Order number
	} else if (strcmp(CViewSample::classCViewSample.m_lpszClassName, m_szCurrentViewClassName) == 0)
	{
		mpt::IO::WriteVarInt(f, (uint32_t)view->SendMessage(WM_MOD_CTRLMSG, CTRLMSG_GETCURRENTINSTRUMENT));	// Sample number
	} else if (strcmp(CViewInstrument::classCViewInstrument.m_lpszClassName, m_szCurrentViewClassName) == 0)
	{
		mpt::IO::WriteVarInt(f, (uint32_t)view->SendMessage(WM_MOD_CTRLMSG, CTRLMSG_GETCURRENTINSTRUMENT));	// Instrument number
	}
	return f.str();
}


void CChildFrame::DeserializeView(FileReader &file)
//-------------------------------------------------
{
	uint32_t version, page;
	if(file.ReadVarInt(version) && version == 0 &&
		file.ReadVarInt(page) && page >= 0 && page < CModControlView::MAX_PAGES)
	{
		UINT pageDlg = 0;
		switch(page)
		{
		case CModControlView::VIEW_GLOBALS:
			pageDlg = IDD_CONTROL_GLOBALS;
			break;
		case CModControlView::VIEW_PATTERNS:
			pageDlg = IDD_CONTROL_PATTERNS;
			file.ReadVarInt(m_ViewPatterns.initialOrder);
			break;
		case CModControlView::VIEW_SAMPLES:
			pageDlg = IDD_CONTROL_SAMPLES;
			file.ReadVarInt(m_ViewSamples.initialSample);
			break;
		case CModControlView::VIEW_INSTRUMENTS:
			pageDlg = IDD_CONTROL_INSTRUMENTS;
			file.ReadVarInt(m_ViewInstruments.initialInstrument);
			break;
		case CModControlView::VIEW_COMMENTS:
			pageDlg = IDD_CONTROL_COMMENTS;
			break;
		}
		GetModControlView()->PostMessage(WM_MOD_ACTIVATEVIEW, pageDlg, (LPARAM)-1);
	}

}

OPENMPT_NAMESPACE_END
