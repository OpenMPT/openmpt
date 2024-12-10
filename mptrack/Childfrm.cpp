/*
 * Childfrm.cpp
 * ------------
 * Purpose: Implementation of the MDI document child windows.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Childfrm.h"
#include "ChannelManagerDlg.h"
#include "Ctrl_ins.h"
#include "Ctrl_pat.h"
#include "Ctrl_smp.h"
#include "Globals.h"
#include "HighDPISupport.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Mptrack.h"
#include "resource.h"
#include "view_com.h"
#include "View_gen.h"
#include "View_ins.h"
#include "View_pat.h"
#include "View_smp.h"
#include "WindowMessages.h"
#include "../common/FileReader.h"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"

#include <sstream>
#include <afxpriv.h>


OPENMPT_NAMESPACE_BEGIN


/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CChildFrame)
	ON_WM_DESTROY()
	ON_WM_NCACTIVATE()
	ON_WM_MDIACTIVATE()
	ON_MESSAGE(WM_DPICHANGED_AFTERPARENT, &CChildFrame::OnDPIChangedAfterParent)
	ON_MESSAGE(WM_MOD_CHANGEVIEWCLASS,    &CChildFrame::OnChangeViewClass)
	ON_MESSAGE(WM_MOD_INSTRSELECTED,      &CChildFrame::OnInstrumentSelected)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CChildFrame *CChildFrame::m_lastActiveFrame = nullptr;
int CChildFrame::glMdiOpenCount = 0;

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
	glMdiOpenCount++;
}


CChildFrame::~CChildFrame()
{
	if ((--glMdiOpenCount) == 0)
	{
		TrackerSettings::Instance().gbMdiMaximize = m_maxWhenClosed;
	}
}


BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	// create a splitter with 2 rows, 1 column
	if (!m_wndSplitter.CreateStatic(this, 2, 1)) return FALSE;

	// add the first splitter pane - the default view in row 0
	int cy = HighDPISupport::ScalePixels(TrackerSettings::Instance().glGeneralWindowHeight, m_hWnd);  //rewbs.varWindowSize - default to general tab.
	if (cy <= 1) cy = (lpcs->cy*2) / 3;
	if (!m_wndSplitter.CreateView(0, 0, pContext->m_pNewViewClass, CSize(0, cy), pContext)) return FALSE;

	// Get 2nd window handle
	CModControlView *pModView;
	if ((pModView = GetModControlView()) != nullptr)
	{
		m_hWndCtrl = pModView->m_hWnd;
		pModView->SetMDIParentFrame(m_hWnd);
	}

	m_dpi = HighDPISupport::GetDpiForWindow(m_hWnd);

	const BOOL bStatus = ChangeViewClass(RUNTIME_CLASS(CViewGlobals), pContext);

	// If it all worked, we now have a splitter window which contain two different views
	return bStatus;
}


void CChildFrame::SetSplitterHeight(int cy)
{
	if (cy <= 1) cy = 188;	//default to 188? why not..
	cy = HighDPISupport::ScalePixels(cy, m_hWnd);
	m_wndSplitter.SetRowInfo(0, cy, 15);
}


LRESULT CChildFrame::OnDPIChangedAfterParent(WPARAM, LPARAM)
{
	auto result = Default();
	if(CModControlView *pModView = GetModControlView())
	{
		if(CModControlDlg *pDlg = pModView->GetCurrentControlDlg())
		{
			SetSplitterHeight(pDlg->GetSplitPosRef());
			m_wndSplitter.RecalcLayout();
		}
	}
	const int newDPI = HighDPISupport::GetDpiForWindow(m_hWnd);
	if(m_dpi)
	{
		// MDI child windows are not resized automatically on DPI change.
		CRect windowRect;
		GetClientRect(windowRect);
		windowRect.right = Util::muldiv(windowRect.right, newDPI, m_dpi);
		windowRect.bottom = Util::muldiv(windowRect.bottom, newDPI, m_dpi);
		HighDPISupport::AdjustWindowRectEx(windowRect, GetStyle(), FALSE, GetExStyle(), newDPI);
		SetWindowPos(nullptr, 0, 0, windowRect.Width(), windowRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
	}
	m_dpi = newDPI;
	return result;
}


BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	return CMDIChildWnd::PreCreateWindow(cs);
}


BOOL CChildFrame::OnNcActivate(BOOL bActivate)
{
	if(bActivate && m_hWndView)
	{
		// Need this in addition to OnMDIActivate when switching from a non-MDI window such as a plugin editor
		CMainFrame::GetMainFrame()->SetMidiRecordWnd(m_hWndView);
	}
	if(m_hWndCtrl)
		::SendMessage(m_hWndCtrl, bActivate ? WM_MOD_MDIACTIVATE : WM_MOD_MDIDEACTIVATE, 0, 0);
	if(m_hWndView)
		::SendMessage(m_hWndView, bActivate ? WM_MOD_MDIACTIVATE : WM_MOD_MDIDEACTIVATE, 0, 0);

	return CMDIChildWnd::OnNcActivate(bActivate);
}


void CChildFrame::OnMDIActivate(BOOL bActivate, CWnd *pActivateWnd, CWnd *pDeactivateWnd)
{
	CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

	if(bActivate)
	{
		MPT_ASSERT(pActivateWnd == this);
		CMainFrame::GetMainFrame()->UpdateEffectKeys(static_cast<CModDoc *>(GetActiveDocument()));
		CMainFrame::GetMainFrame()->SetMidiRecordWnd(m_hWndView);
		m_lastActiveFrame = this;
	}
	if(m_hWndCtrl)
		::SendMessage(m_hWndCtrl, bActivate ? WM_MOD_MDIACTIVATE : WM_MOD_MDIDEACTIVATE, 0, 0);
	if(m_hWndView)
		::SendMessage(m_hWndView, bActivate ? WM_MOD_MDIACTIVATE : WM_MOD_MDIDEACTIVATE, 0, 0);

	// Update channel manager according to active document
	auto instance = CChannelManagerDlg::sharedInstance();
	if(instance != nullptr)
	{
		if(!bActivate && pActivateWnd == nullptr)
			instance->SetDocument(nullptr);
		else if(bActivate)
			instance->SetDocument(static_cast<CModDoc *>(GetActiveDocument()));
	}
}


void CChildFrame::ActivateFrame(int nCmdShow)
{
	if ((glMdiOpenCount == 1) && (TrackerSettings::Instance().gbMdiMaximize) && (nCmdShow == -1))
	{
		nCmdShow = SW_SHOWMAXIMIZED;
	}
	CMDIChildWnd::ActivateFrame(nCmdShow);

	// When song first loads, initialise patternViewState to point to start of song.
	CView *pView = GetActiveView();
	CModDoc *pModDoc = nullptr;
	if (pView) pModDoc = (CModDoc *)pView->GetDocument();
	if ((m_hWndCtrl) && (pModDoc))
	{
		if (m_initialActivation && m_ViewPatterns.nPattern == 0)
		{
			if(!pModDoc->GetSoundFile().Order().empty())
				m_ViewPatterns.nPattern = pModDoc->GetSoundFile().Order()[0];
			m_initialActivation = false;
		}
	}
}


void CChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	// update our parent window first
	GetMDIFrame()->OnUpdateFrameTitle(bAddToTitle);

	if ((GetStyle() & FWS_ADDTOTITLE) == 0)	return;     // leave child window alone!

	CDocument* pDocument = GetActiveDocument();
	if (bAddToTitle)
	{
		CString szText;
		if (pDocument == nullptr)
		{
			szText.Preallocate(m_strTitle.GetLength() + 10);
			szText = m_strTitle;
		} else
		{
			szText.Preallocate(pDocument->GetTitle().GetLength() + 10);
			szText = pDocument->GetTitle();
			if (pDocument->IsModified()) szText += _T("*");
		}
		if (m_nWindow > 0)
			szText.AppendFormat(_T(":%d"), m_nWindow);

		// set title if changed, but don't remove completely
		AfxSetWindowText(m_hWnd, szText);
	}
}


BOOL CChildFrame::ChangeViewClass(CRuntimeClass* pViewClass, CCreateContext* pContext)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CWnd *pWnd;
	if (pViewClass->m_lpszClassName == m_currentViewClassName) return TRUE;
	if(!m_currentViewClassName.empty())
	{
		m_currentViewClassName.clear();
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
	m_currentViewClassName = pViewClass->m_lpszClassName;
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
{
	CModControlView *pModView;
	if ((pModView = GetModControlView()) != nullptr)
	{
		pModView->ForceRefresh();
	}

	return;
}

void CChildFrame::SavePosition(bool force)
{
	if (m_hWnd)
	{
		m_maxWhenClosed = IsZoomed() != FALSE;
		if (force)
			TrackerSettings::Instance().gbMdiMaximize = m_maxWhenClosed;
		if (!IsIconic())
		{
			CWnd *pWnd = m_wndSplitter.GetPane(0, 0);
			if (pWnd)
			{
				CRect rect(0, 0, 0, 0);
				pWnd->GetWindowRect(&rect);
				if(rect.Width() == 0)
					return;
				int l = HighDPISupport::ScalePixelsInv(rect.Height(), m_hWnd);
				//rewbs.varWindowSize - not the nicest piece of code, but we need to distinguish between the views:
				if(CViewGlobals::classCViewGlobals.m_lpszClassName == m_currentViewClassName)
					TrackerSettings::Instance().glGeneralWindowHeight = l;
				else if(CViewPattern::classCViewPattern.m_lpszClassName == m_currentViewClassName)
					TrackerSettings::Instance().glPatternWindowHeight = l;
				else if(CViewSample::classCViewSample.m_lpszClassName == m_currentViewClassName)
					TrackerSettings::Instance().glSampleWindowHeight = l;
				else if(CViewInstrument::classCViewInstrument.m_lpszClassName == m_currentViewClassName)
					TrackerSettings::Instance().glInstrumentWindowHeight = l;
				else if(CViewComments::classCViewComments.m_lpszClassName == m_currentViewClassName)
					TrackerSettings::Instance().glCommentsWindowHeight = l;
			}
		}
	}
}


int CChildFrame::GetSplitterHeight()
{
	if (m_hWnd)
	{
		CRect rect;

		CWnd *pWnd = m_wndSplitter.GetPane(0, 0);
		if (pWnd)
		{
			pWnd->GetWindowRect(&rect);
			return HighDPISupport::ScalePixelsInv(rect.Height(), m_hWnd);
		}
	}
	return 15;	// tidy default
};


LRESULT CChildFrame::SendCtrlMessage(UINT uMsg, LPARAM lParam) const
{
	if(m_hWndCtrl)
		return ::SendMessage(m_hWndCtrl, WM_MOD_CTRLMSG, uMsg, lParam);
	return 0;
}


LRESULT CChildFrame::SendViewMessage(UINT uMsg, LPARAM lParam) const
{
	if(m_hWndView)
		return ::SendMessage(m_hWndView, WM_MOD_VIEWMSG, uMsg, lParam);
	return 0;
}


LRESULT CChildFrame::ActivateView(UINT nId, LPARAM lParam) { return ::SendMessage(m_hWndCtrl, WM_MOD_ACTIVATEVIEW, nId, lParam); }


LRESULT CChildFrame::OnInstrumentSelected(WPARAM wParam, LPARAM lParam)
{
	CView *pView = GetActiveView();
	CModDoc *pModDoc = NULL;
	if (pView) pModDoc = (CModDoc *)pView->GetDocument();
	if ((m_hWndCtrl) && (pModDoc))
	{
		auto nIns = lParam;

		if ((!wParam) && (pModDoc->GetNumInstruments() > 0))
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
// CChildFrame message handlers

void CChildFrame::OnDestroy()
{
	SavePosition();
	if(m_lastActiveFrame == this)
		m_lastActiveFrame = nullptr;
	CMDIChildWnd::OnDestroy();
}


LRESULT CChildFrame::OnChangeViewClass(WPARAM wParam, LPARAM lParam)
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


bool CChildFrame::IsPatternView() const
{
	return CViewPattern::classCViewPattern.m_lpszClassName == m_currentViewClassName;
}


void CChildFrame::SaveAllViewStates()
{
	void *ptr = nullptr;
	if(CViewPattern::classCViewPattern.m_lpszClassName == m_currentViewClassName)
		ptr = &m_ViewPatterns;
	else if(CViewSample::classCViewSample.m_lpszClassName == m_currentViewClassName)
		ptr = &m_ViewSamples;
	else if(CViewInstrument::classCViewInstrument.m_lpszClassName == m_currentViewClassName)
		ptr = &m_ViewInstruments;
	else if(CViewGlobals::classCViewGlobals.m_lpszClassName == m_currentViewClassName)
		ptr = &m_ViewGeneral;
	else if(CViewComments::classCViewComments.m_lpszClassName == m_currentViewClassName)
		ptr = &m_ViewComments;
	::SendMessage(m_hWndView, WM_MOD_VIEWMSG, VIEWMSG_SAVESTATE, reinterpret_cast<LPARAM>(ptr));
}


std::string CChildFrame::SerializeView()
{
	SaveAllViewStates();

	std::ostringstream f(std::ios::out | std::ios::binary);
	// Version
	mpt::IO::WriteVarInt(f, 1u);
	// Current page
	mpt::IO::WriteVarInt(f, static_cast<uint8>(GetModControlView()->GetActivePage()));

	const auto Serialize = [](std::ostringstream &f, CModControlView::Page page, const std::string &s)
	{
		mpt::IO::WriteVarInt(f, static_cast<uint8>(page));
		mpt::IO::WriteVarInt(f, s.size());
		mpt::IO::WriteRaw(f, s.data(), s.size());
	};
	Serialize(f, CModControlView::Page::Patterns, m_ViewPatterns.Serialize());
	Serialize(f, CModControlView::Page::Samples, m_ViewSamples.Serialize());
	Serialize(f, CModControlView::Page::Instruments, m_ViewInstruments.Serialize());

	return std::move(f).str();
}


void CChildFrame::DeserializeView(FileReader &file)
{
	uint32 version, page;
	if(!file.ReadVarInt(version) || version > 1)
		return;
	if(!file.ReadVarInt(page) || page >= static_cast<uint32>(CModControlView::Page::NumPages))
		return;

	UINT pageDlg = 0;
	switch(static_cast<CModControlView::Page>(page))
	{
	case CModControlView::Page::Globals:
		pageDlg = IDD_CONTROL_GLOBALS;
		break;
	case CModControlView::Page::Patterns:
		pageDlg = IDD_CONTROL_PATTERNS;
		if(version == 0)
			m_ViewPatterns.Deserialize(file);
		break;
	case CModControlView::Page::Samples:
		pageDlg = IDD_CONTROL_SAMPLES;
		if(version == 0)
			m_ViewSamples.Deserialize(file);
		break;
	case CModControlView::Page::Instruments:
		pageDlg = IDD_CONTROL_INSTRUMENTS;
		if(version == 0)
			m_ViewInstruments.Deserialize(file);
		break;
	case CModControlView::Page::Comments:
		pageDlg = IDD_CONTROL_COMMENTS;
		break;
	case CModControlView::Page::Unknown:
	case CModControlView::Page::NumPages:
		break;
	}

	// Version 1 extensions: Deserialize all views, not just current view
	while(file.CanRead(3))
	{
		uint32 size = 0;
		file.ReadVarInt(page);
		file.ReadVarInt(size);
		FileReader chunk = file.ReadChunk(size);
		if(page >= static_cast<uint32>(CModControlView::Page::NumPages) || !chunk.IsValid())
			continue;

		switch(static_cast<CModControlView::Page>(page))
		{
		case CModControlView::Page::Globals:     m_ViewGeneral.Deserialize(chunk); break;
		case CModControlView::Page::Patterns:    m_ViewPatterns.Deserialize(chunk); break;
		case CModControlView::Page::Samples:     m_ViewSamples.Deserialize(chunk); break;
		case CModControlView::Page::Instruments: m_ViewInstruments.Deserialize(chunk); break;
		case CModControlView::Page::Comments:    m_ViewComments.Deserialize(chunk); break;
		case CModControlView::Page::Unknown:
		case CModControlView::Page::NumPages:
			break;
		}
	}

	GetModControlView()->PostMessage(WM_MOD_ACTIVATEVIEW, pageDlg, LPARAM(-1));
}


void CChildFrame::ToggleViews()
{
	auto focus = ::GetFocus();
	if(focus == GetHwndView() || ::IsChild(GetHwndView(), focus))
		SendCtrlMessage(CTRLMSG_SETFOCUS);
	else if(focus == GetHwndCtrl() || ::IsChild(GetHwndCtrl(), focus))
		SendViewMessage(VIEWMSG_SETFOCUS);
}


std::string PatternViewState::Serialize() const
{
	std::ostringstream f(std::ios::out | std::ios::binary);
	mpt::IO::WriteVarInt(f, initialized ? nOrder : initialOrder);
	mpt::IO::WriteVarInt(f, visibleColumns.to_ulong());
	return std::move(f).str();
}


void PatternViewState::Deserialize(FileReader &f)
{
	f.ReadVarInt(initialOrder);
	unsigned long columns = 0;
	if(f.ReadVarInt(columns))
		visibleColumns = columns;
}


std::string SampleViewState::Serialize() const
{
	std::ostringstream f(std::ios::out | std::ios::binary);
	mpt::IO::WriteVarInt(f, nSample ? nSample : initialSample);
	return std::move(f).str();
}


void SampleViewState::Deserialize(FileReader &f)
{
	f.ReadVarInt(initialSample);
}


std::string InstrumentViewState::Serialize() const
{
	std::ostringstream f(std::ios::out | std::ios::binary);
	mpt::IO::WriteVarInt(f, instrument ? instrument : initialInstrument);
	return std::move(f).str();
}


void InstrumentViewState::Deserialize(FileReader &f)
{
	f.ReadVarInt(initialInstrument);
}


OPENMPT_NAMESPACE_END
