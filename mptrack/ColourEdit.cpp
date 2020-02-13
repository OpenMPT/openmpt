/*
 * ColourEdit.cpp
 * --------------
 * Purpose: Implementation of a coloured edit UI item.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "ColourEdit.h"


OPENMPT_NAMESPACE_BEGIN


/////////////////////////////////////////////////////////////////////////////
// CColourEdit

CColourEdit::CColourEdit()
{
	m_crText = RGB(0, 0, 0);  //default text color
}

CColourEdit::~CColourEdit()
{
	if(m_brBackGnd.GetSafeHandle())  //delete brush
		m_brBackGnd.DeleteObject();
}


BEGIN_MESSAGE_MAP(CColourEdit, CEdit)
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColourEdit message handlers

HBRUSH CColourEdit::CtlColor(CDC *pDC, UINT nCtlColor)
{
	MPT_UNREFERENCED_PARAMETER(nCtlColor);
	pDC->SetTextColor(m_crText);		//set text color
	pDC->SetBkColor(m_crBackGnd);		//set the text's background color
	return m_brBackGnd;	//return the brush used for background - this sets control background
}

/////////////////////////////////////////////////////////////////////////////
// Implementation

void CColourEdit::SetBackColor(COLORREF rgb)
{
	m_crBackGnd = rgb;					//set background color ref (used for text's background)
	if(m_brBackGnd.GetSafeHandle())  //free brush
		m_brBackGnd.DeleteObject();
	m_brBackGnd.CreateSolidBrush(rgb);  //set brush to new color
	Invalidate(TRUE);					//redraw
}


void CColourEdit::SetTextColor(COLORREF rgb)
{
	m_crText = rgb;    // set text color ref
	Invalidate(TRUE);  // redraw
}


OPENMPT_NAMESPACE_END
