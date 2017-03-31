/*
 * ColorConfigDlg.cpp
 * ------------------
 * Purpose: Implementation of the color setup dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

//========================================
class COptionsColors: public CPropertyPage
//========================================
{
protected:
	COLORREF CustomColors[MAX_MODCOLORS];
	UINT m_nColorItem;
	CComboBox m_ComboItem, m_ComboFont, m_ComboPreset;
	CButton m_BtnColor1, m_BtnColor2, m_BtnColor3, m_BtnPreview;
	CSpinButtonCtrl m_ColorSpin;
	CStatic m_TxtColor1, m_TxtColor2, m_TxtColor3;
	MODPLUGDIB *m_pPreviewDib;
	FontSetting patternFont, commentFont;

public:
	COptionsColors():CPropertyPage(IDD_OPTIONS_COLORS) { m_nColorItem = 0; m_pPreviewDib = NULL; }
	~COptionsColors() { if (m_pPreviewDib) delete m_pPreviewDib; m_pPreviewDib = NULL; }
	void SelectColor(COLORREF *);

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnKillActive();
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnSetActive();
	virtual void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnChoosePatternFont();
	afx_msg void OnChooseCommentFont();
	afx_msg void OnUpdateDialog();
	afx_msg void OnDrawItem(int nIdCtl, LPDRAWITEMSTRUCT lpdis);
	afx_msg void OnColorSelChanged();
	afx_msg void OnSettingsChanged();
	afx_msg void OnSelectColor1();
	afx_msg void OnSelectColor2();
	afx_msg void OnSelectColor3();
	afx_msg void OnPresetChange();
	afx_msg void OnLoadColorScheme();
	afx_msg void OnSaveColorScheme();
	afx_msg void OnClearWindowCache();
	afx_msg void OnPreviewChanged();
	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END
