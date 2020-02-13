/*
 * ColourEdit.h
 * ------------
 * Purpose: Implementation of a coloured edit UI item.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

OPENMPT_NAMESPACE_BEGIN

class CColourEdit : public CEdit
{
public:
	CColourEdit();
	~CColourEdit();

public:
	void SetTextColor(COLORREF rgb);
	void SetBackColor(COLORREF rgb);

private:
	COLORREF m_crText;
	COLORREF m_crBackGnd;
	CBrush m_brBackGnd;

protected:
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	DECLARE_MESSAGE_MAP()

};

OPENMPT_NAMESPACE_END
