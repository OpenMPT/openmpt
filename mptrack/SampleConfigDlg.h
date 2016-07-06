/*
 * SampleConfigDlg.h
 * -------------------
 * Purpose: Implementation of the sample/instrument editor settings dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

//===============================================
class COptionsSampleEditor : public CPropertyPage
//===============================================
{
protected:
	CComboBox m_cbnDefaultSampleFormat, m_cbnDefaultVolumeHandling;

public:
	COptionsSampleEditor() : CPropertyPage(IDD_OPTIONS_SAMPLEEDITOR) { }

protected:

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnSetActive();

	void RecalcUndoSize();

	afx_msg void OnHScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* /*pScrollBar*/) { OnSettingsChanged(); }
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnUndoSizeChanged();
	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END
