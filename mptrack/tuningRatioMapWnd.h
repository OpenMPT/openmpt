/*
 * tuningRatioMapWnd.h
 * -------------------
 * Purpose: Alternative sample tuning configuration dialog - ratio map edit control.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../soundlib/tuning.h"

OPENMPT_NAMESPACE_BEGIN

class CTuningDialog;

typedef CTuning::NOTEINDEXTYPE NOTEINDEXTYPE;

//Copied from CNoteMapWnd.
//===============================
class CTuningRatioMapWnd: public CStatic
//===============================
{
	friend class CTuningDialog;
protected:
	const CTuning* m_pTuning;

	CTuningDialog* m_pParent;

	HFONT m_hFont;
	int m_cxFont, m_cyFont;
	COLORREF colorText, colorTextSel;
	NOTEINDEXTYPE m_nNote;

	NOTEINDEXTYPE m_nNoteCentre;


public:
	CTuningRatioMapWnd() : m_cxFont(0),
		m_cyFont(0),
		m_hFont(NULL),
		m_pTuning(NULL),
		m_pParent(NULL),
		m_nNoteCentre(61),
		m_nNote(61) {}

	VOID Init(CTuningDialog* const pParent, CTuning* const pTun) { m_pParent = pParent; m_pTuning = pTun;}

	NOTEINDEXTYPE GetShownCentre() const;

protected:

	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT, CPoint);
	afx_msg void OnLButtonUp(UINT, CPoint);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnKillFocus(CWnd *pNewWnd);
	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END