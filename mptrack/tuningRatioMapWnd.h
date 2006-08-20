#ifndef TUNINGRATIOMAPWND_H
#define TUNINGRATIOMAPWND_H

#include "../soundlib/tuning_template.h"

class CTuningDialog;

//This is copied from CNoteMapWnd.
//===============================
class CTuningRatioMapWnd: public CStatic
//===============================
{
	friend class CTuningDialog;
protected:
	CTuning* m_pTuning;

	CTuningDialog* m_pParent;

	HFONT m_hFont;
	int m_cxFont, m_cyFont;
	COLORREF colorText, colorTextSel;
	CTuning::STEPTYPE m_nNote;

	CTuning::STEPTYPE m_nNoteCentre;


public:
	CTuningRatioMapWnd() : m_cxFont(0),
		m_cyFont(0),
		m_hFont(NULL),
		m_pTuning(NULL),
		m_pParent(NULL),
		m_nNoteCentre(61),
		m_nNote(0) {}

	VOID Init(CTuningDialog* const pParent, CTuning* const pTun) { m_pParent = pParent; m_pTuning = pTun;}

	CTuning::STEPTYPE GetShownCentre() const;

protected:
	
	afx_msg void OnLButtonDown(UINT, CPoint);
	afx_msg void OnLButtonUp(UINT, CPoint);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnKillFocus(CWnd *pNewWnd);
	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()
};

#endif
