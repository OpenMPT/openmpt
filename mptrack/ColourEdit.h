#pragma once
#include "afxwin.h"

class CColourEdit :
	public CEdit
{
public:
	CColourEdit(void);
	~CColourEdit(void);

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
