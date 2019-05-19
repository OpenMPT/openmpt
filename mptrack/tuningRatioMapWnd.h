/*
 * tuningRatioMapWnd.h
 * -------------------
 * Purpose: Alternative sample tuning configuration dialog - ratio map edit control.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "../soundlib/tuning.h"

OPENMPT_NAMESPACE_BEGIN

class CTuningDialog;

using NOTEINDEXTYPE = Tuning::NOTEINDEXTYPE;

//Copied from CNoteMapWnd.
class CTuningRatioMapWnd: public CStatic
{
	friend class CTuningDialog;
protected:
	const CTuning* m_pTuning = nullptr;
	CTuningDialog* m_pParent = nullptr;

	int m_cxFont = 0, m_cyFont = 0;
	NOTEINDEXTYPE m_nNote = NOTE_MIDDLEC;

	NOTEINDEXTYPE m_nNoteCentre = NOTE_MIDDLEC;


public:
	void Init(CTuningDialog* const pParent, CTuning* const tuning);

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
