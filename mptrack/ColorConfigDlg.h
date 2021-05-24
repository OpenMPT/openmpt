/*
 * ColorConfigDlg.cpp
 * ------------------
 * Purpose: Implementation of the color setup dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "ColorPickerButton.h"

OPENMPT_NAMESPACE_BEGIN

class COptionsColors : public CPropertyPage
{
protected:
	std::array<COLORREF, MAX_MODCOLORS> CustomColors;
	CComboBox m_ComboItem, m_ComboFont, m_ComboPreset;
	ColorPickerButton m_BtnColor[3];
	CButton m_BtnPreview;
	CSpinButtonCtrl m_ColorSpin;
	CStatic m_TxtColor[3];
	MODPLUGDIB *m_pPreviewDib = nullptr;
	FontSetting patternFont, commentFont;
	uint32 m_nColorItem = 0;

public:
	COptionsColors();
	~COptionsColors() { delete m_pPreviewDib; }

protected:
	BOOL OnInitDialog() override;
	BOOL OnKillActive() override;
	void OnOK() override;
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnSetActive() override;
	void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	void SelectColor(int colorIndex);

	afx_msg void OnChoosePatternFont();
	afx_msg void OnChooseCommentFont();
	afx_msg void OnUpdateDialog();
	afx_msg void OnDrawItem(int nIdCtl, LPDRAWITEMSTRUCT lpdis);
	afx_msg void OnColorSelChanged();
	afx_msg void OnSettingsChanged();
	afx_msg void OnSelectColor1() { SelectColor(0); }
	afx_msg void OnSelectColor2() { SelectColor(1); }
	afx_msg void OnSelectColor3() { SelectColor(2); }
	afx_msg void OnPresetChange();
	afx_msg void OnLoadColorScheme();
	afx_msg void OnSaveColorScheme();
	afx_msg void OnClearWindowCache();
	afx_msg void OnPreviewChanged();
	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END
