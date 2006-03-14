//********************************************************************************************
//*	HYPEREDIT.CPP - Custom window message handler(s) implementation                          *
//*                                                                                          *
//* This module contains implementation code for object initialization and default windows   *
//* messages.                                                                                *
//*                                                                                          *
//* Aug.31.04                                                                                *
//*                                                                                          *
//* Copyright PCSpectra 2004 (Free for any purpose, except to sell indivually)               *
//* Website: www.pcspectra.com                                                               *
//*                                                                                          *
//* Notes:                                                                                   *
//* ======																					 *
//* Search module for 'PROGRAMMERS NOTE'                                                     *
//*                                                                                          *
//* History:						                                                         *
//*	========																				 *
//* Mon.dd.yy - None so far     														     *
//********************************************************************************************

#include "stdafx.h"
#include "HyperEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Object construction

CHyperEdit::CHyperEdit()
{
	m_nTimer = 0;

	m_nLineHeight = 0;

	m_hHandCursor = NULL; // No hand cursor 

	// Set default hyperlink colors
	m_clrNormal = RGB(92, 92, 154);
	m_clrHover = RGB(168, 168, 230);

	m_csLocation.Empty();
}

// Object destruction

CHyperEdit::~CHyperEdit()
{
	m_oFont.DeleteObject();	// Delete hyperlink font object
}
		  
BEGIN_MESSAGE_MAP(CHyperEdit, CEdit)
	//{{AFX_MSG_MAP(CHyperEdit)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	ON_WM_MOUSEMOVE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Control initialization

void CHyperEdit::PreSubclassWindow() 
{
	CEdit::PreSubclassWindow();
	
	ASSERT(GetStyle() & ES_MULTILINE); // MUST be in multiline state
	//ASSERT(GetStyle() & WS_VSCROLL); // We need vertical scrollbar
	ASSERT(GetStyle() & ES_AUTOVSCROLL);

	// Initialize font object(s)
	CDC* pDC = GetDC();
	ASSERT(pDC);

	CFont* pTemp = GetFont(); // Get the font dialog controls will be using
	ASSERT(pTemp);

	LOGFONT lf;
	pTemp->GetLogFont(&lf);

	// TODO:
	// Check if there is a system setting that we can check to see if hyperlinks should be 
	// underlined by default or not.
	lf.lfUnderline = TRUE; // Our font needs to be undelrined to distinguish itself as hyper

	// Create our hyperlink font :)
	m_oFont.CreateFontIndirect(&lf);

	SetDefaultCursor(); // Try and initialize our hand cursor

	// Calculate single line height
	m_nLineHeight = pDC->DrawText("Test Line", CRect(0,0,0,0), DT_SINGLELINE|DT_CALCRECT);

	// PROGRAMMERS NOTE:
	// =================
	// If the hyperlinks flicker when changing the selection state
	// of the edit control change the timer value to a lower count
	// 10 is the default and appears to have an almost flicker free
	// transition from selection to hyperlinked colors.
	m_nTimer = SetTimer(IDT_SELCHANGE, 10, NULL);
}

// Override mouse movements

void CHyperEdit::OnMouseMove(UINT nFlags, CPoint point) 
{
	CEdit::OnMouseMove(nFlags, point);						  
	CString csURL = IsHyperlink(point);

	// If not empty, then display hand cursor
	if(!csURL.IsEmpty()){ // We are hovering hypertext
		
		// Get the coordinates of last character in entire buffer
		CPoint pt_lastchar = PosFromChar(GetWindowTextLength()-1);

		// Don't bother changing mouse cursor if it's below last visible character
		if(point.y<=(pt_lastchar.y+m_nLineHeight))		
			::SetCursor(m_hHandCursor);

		
		if (!m_bHoveringHyperText) {	 //only redraw if state has changed
			DrawHyperlinks();			 
			m_bHoveringHyperText = true; //store new state
		}

	} else {			 // We are not hovering hypertext
		if (m_bHoveringHyperText) {		 //only redraw if state has changed
			DrawHyperlinks();
			m_bHoveringHyperText = false; //store new state
		}
	}
}

// Override left mouse button down (Clicking)

void CHyperEdit::OnLButtonDown(UINT nFlags, CPoint point) 
{
	m_csLocation = IsHyperlink(point);

	CEdit::OnLButtonDown(nFlags, point);
}

// Override left mouse button up (Clicking)

void CHyperEdit::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CEdit::OnLButtonUp(nFlags, point);

	int iSelStart=0, iSelFinish=0; 
	GetSel(iSelStart, iSelFinish);

	// If there is a selection, just exit now, we don't open URL's
	if(IsSelection(iSelStart, iSelFinish)) return;

	// If were below the last visible character	exit again, cuz we don't want 
	// to open any URL's that aren't directly clicked on
	// Get the coordinates of last character in entire buffer
	CPoint pt = PosFromChar(GetWindowTextLength()-1);

	// Exit if mouse is below last visible character
	if(point.y>(pt.y+m_nLineHeight)) return; 

	CString csURL = IsHyperlink(point);

	// If not empty, then open browser and show web site
	// only if the URL is the same as one clicked on in OnLButtonDown()
	if(!csURL.IsEmpty() && (m_csLocation==csURL)) 
		GotoURL(csURL, SW_SHOW);
}

// Override low level message handling

LRESULT CHyperEdit::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	// handling OnPaint() didn't work
	if(message==WM_PAINT){
		CEdit::WindowProc(message, wParam, lParam);

		DrawHyperlinks();

		return TRUE;
	}
	
	return CEdit::WindowProc(message, wParam, lParam);
}

// Emulate an OnSelChange() event using a low interval timer (initialized in PreSubclassWindow)

void CHyperEdit::OnTimer(UINT nIDEvent) 
{
	//
	// Emulate a OnSelChange() event
	//

	static int iPrevStart=0, iPrevFinish=0;
	
	DWORD dwSel = GetSel();	

	// Check the previous start/finish of selection range
	// and compare them against the current selection range
	// if there is any difference between them fire off an OnSelChange event
	if(LOWORD(dwSel) != iPrevStart || HIWORD(dwSel) != iPrevFinish)
		OnSelChange();

	// Save current selection state for next call (as previous state)
	iPrevStart = LOWORD(dwSel);
	iPrevFinish = HIWORD(dwSel);

	CEdit::OnTimer(nIDEvent);
}

