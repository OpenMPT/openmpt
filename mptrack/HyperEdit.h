//********************************************************************************************
//*	HYPEREDIT.H - Control interface and trivial implementation                               *
//*                                                                                          *
//* This module contains interface code and some typedefs and application defines            *
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

#ifndef AFX_HYPEREDIT_H__28F52ED8_8811_436F_821B_EB02D02A1F88__INCLUDED_
#define AFX_HYPEREDIT_H__28F52ED8_8811_436F_821B_EB02D02A1F88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WHITE_SPACE1   _T(' ')   // Single space
#define WHITE_SPACE2   _T('\r')  // Carriage return
#define WHITE_SPACE3   _T('\n')  // Linefeed/newline
#define WHITE_SPACE4   _T('\t')  // Tab

#define IDT_SELCHANGE 0x6969 

#define IsSelection(p1, p2)( p1 != p2 )

#include <vector>  

using namespace std;

//
// This structure holds the starting offset and length for each token
// located inside the buffer for the control. We use this data inside
// OnPaint and OnMouseMove and OnChange to quickly get the URL or token string
// 

struct _TOKEN_OFFSET{
	WORD iStart;   //
	WORD iLength;
}; typedef vector<_TOKEN_OFFSET> OFFSETS;

// CHyperEdit control interface

class CHyperEdit : public CEdit
{
public:
	CHyperEdit();
	virtual ~CHyperEdit();

	COLORREF GetNormalColor() const { return m_clrNormal; }
	COLORREF GetHoverColor() const { return m_clrHover; }

	void SetLinkColors(COLORREF clrNormal, COLORREF clrHover){ m_clrNormal=clrNormal; m_clrHover=clrHover; }

	// Used with a TIMER to display tooltips (Maybe the webpage TITLE???)
	CString IsHyperlink(CPoint& pt) const;
protected:
	virtual BOOL IsWordHyper(const CString& csToken) const;

	inline BOOL IsWhiteSpace(const CString& csBuff, int iIndex) const;
	
	//{{AFX_VIRTUAL(CHyperEdit)
	virtual void PreSubclassWindow();
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

	afx_msg void OnSelChange(){ DrawHyperlinks(); }

	//{{AFX_MSG(CHyperEdit)
	afx_msg void OnChange(){ DrawHyperlinks(); }
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar){ CEdit::OnHScroll(nSBCode, nPos, pScrollBar); DrawHyperlinks(); }
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar){ CEdit::OnVScroll(nSBCode, nPos, pScrollBar); DrawHyperlinks(); }
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy(){ CEdit::OnDestroy(); KillTimer(m_nTimer); }
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
private:
	// Located in HyperEdit2.cpp
	void DrawHyperlinks();
	void BuildOffsetList(int iCharStart, int iCharFinish);

	// Functions borrowed from Chris Maunder's article
	void SetDefaultCursor();
    HINSTANCE GotoURL(LPCTSTR url, int showcmd);
    void ReportError(int nError);
    LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata);

private:
	UINT m_nTimer;		 
	UINT m_nLineHeight;
	bool m_bHoveringHyperText;

	CFont m_oFont; 		 

	CString m_csLocation; // URL or file to be opened when mouse is released

	HCURSOR m_hHandCursor; 

	OFFSETS m_linkOffsets; // Character offsets for each hyperlink located

	COLORREF m_clrNormal, m_clrHover;

	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
#endif // !defined(AFX_HYPEREDIT_H__28F52ED8_8811_436F_821B_EB02D02A1F88__INCLUDED_)
