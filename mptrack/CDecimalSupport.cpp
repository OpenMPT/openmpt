/*
 * CDecimalSupport.cpp
 * -------------------
 * Purpose: Various extensions of the CDecimalSupport implementation.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Snd_defs.h"
#include "CDecimalSupport.h"

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(CNumberEdit, CEdit)
	ON_WM_CHAR()
	ON_MESSAGE(WM_PASTE, &CNumberEdit::OnPaste)
END_MESSAGE_MAP()


void CNumberEdit::SetTempoValue(const TEMPO &t)
{
	SetFixedValue(t.ToDouble(), 4);
}


TEMPO CNumberEdit::GetTempoValue()
{
	double d;
	GetDecimalValue(d);
	return TEMPO(d);
}


void CNumberEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	BOOL bHandled = false;
	CDecimalSupport<CNumberEdit>::OnChar(0, nChar, 0, bHandled);
	if(!bHandled) CEdit::OnChar(nChar , nRepCnt,  nFlags);
}


LPARAM CNumberEdit::OnPaste(WPARAM wParam, LPARAM lParam)
{
	bool bHandled = false;
	CDecimalSupport<CNumberEdit>::OnPaste(0, wParam, lParam, bHandled);
	if(!bHandled)
		return CEdit::DefWindowProc(WM_PASTE, wParam, lParam);
	else
		return 0;
}

OPENMPT_NAMESPACE_END