/*
 * ColorConfigDlg.cpp
 * ------------------
 * Purpose: Implementation of the color setup dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

OPENMPT_NAMESPACE_BEGIN

class COptionsColors: public CPropertyPage
{
protected:
	std::array<COLORREF, MAX_MODCOLORS> CustomColors;
	UINT m_nColorItem = 0;
	CComboBox m_ComboItem, m_ComboFont, m_ComboPreset;
	CButton m_BtnColor1, m_BtnColor2, m_BtnColor3, m_BtnPreview;
	CSpinButtonCtrl m_ColorSpin;
	CStatic m_TxtColor1, m_TxtColor2, m_TxtColor3;
	MODPLUGDIB *m_pPreviewDib = nullptr;
	FontSetting patternFont, commentFont;

public:
	COptionsColors():CPropertyPage(IDD_OPTIONS_COLORS) { }
	~COptionsColors() { delete m_pPreviewDib; }
	void SelectColor(COLORREF &color);

protected:
	BOOL OnInitDialog() override;
	BOOL OnKillActive() override;
	void OnOK() override;
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnSetActive() override;
	void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

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
