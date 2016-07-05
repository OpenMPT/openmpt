/*
 * PatternFindReplaceDlg.h
 * -----------------------
 * Purpose: The find/replace dialog for pattern data.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "EffectInfo.h"
#include "PatternCursor.h"

OPENMPT_NAMESPACE_BEGIN

/////////////////////////////////////////////////////////////////////////
// Search/Replace

struct FindReplace;

//=========================================
class CFindReplaceTab: public CPropertyPage
//=========================================
{
protected:
	CComboBox m_cbnNote, m_cbnInstr, m_cbnVolCmd, m_cbnVolume, m_cbnCommand, m_cbnParam, m_cbnPCParam;

	CSoundFile &m_sndFile;
	FindReplace &m_settings;
	EffectInfo m_effectInfo;
	bool m_isReplaceTab;

	// Special ItemData values
	enum
	{
		kFindAny = INT_MAX - 1,
		kFindRange = INT_MAX - 2,

		kReplaceRelative = INT_MAX - 3,
		kReplaceMultiply = INT_MAX - 4,

		kReplaceNoteMinusOne = INT_MAX - 5,
		kReplaceNotePlusOne = INT_MAX - 6,
		kReplaceNoteMinusOctave = INT_MAX - 7,
		kReplaceNotePlusOctave = INT_MAX - 8,

		kReplaceInstrumentMinusOne = INT_MAX - 5,
		kReplaceInstrumentPlusOne = INT_MAX - 6,
	};

public:
	CFindReplaceTab(UINT nIDD, bool isReplaceTab, CSoundFile &sf, FindReplace &settings)
		: CPropertyPage(nIDD)
		, m_sndFile(sf)
		, m_settings(settings)
		, m_effectInfo(sf)
		, m_isReplaceTab(isReplaceTab)
	{ }

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* pDX);

	bool IsPCEvent() const;
	
	void UpdateInstrumentList();
	void UpdateVolumeList();
	void UpdateParamList();

	// When a combobox is focussed, check the corresponding checkbox.
	void CheckOnChange(int nIDButton) { CheckDlgButton(nIDButton, BST_CHECKED); CheckReplace(nIDButton); };
	afx_msg void OnNoteChanged();
	afx_msg void OnInstrChanged();
	afx_msg void OnVolCmdChanged()	{ CheckOnChange(IDC_CHECK3); UpdateVolumeList(); };
	afx_msg void OnVolumeChanged();
	afx_msg void OnEffectChanged()	{ CheckOnChange(IDC_CHECK5); UpdateParamList(); };
	afx_msg void OnParamChanged();
	afx_msg void OnPCParamChanged();
	// When a checkbox is checked, also check "Replace By".
	afx_msg void OnCheckNote()		{ CheckReplace(IDC_CHECK1); };
	afx_msg void OnCheckInstr()		{ CheckReplace(IDC_CHECK2); };
	afx_msg void OnCheckVolCmd()	{ CheckReplace(IDC_CHECK3); };
	afx_msg void OnCheckVolume()	{ CheckReplace(IDC_CHECK4); };
	afx_msg void OnCheckEffect()	{ CheckReplace(IDC_CHECK5); };
	afx_msg void OnCheckParam()		{ CheckReplace(IDC_CHECK6); };
	// Check "Replace By"
	afx_msg void CheckReplace(int nIDButton)	{ if(m_isReplaceTab && IsDlgButtonChecked(nIDButton)) CheckDlgButton(IDC_CHECK7, BST_CHECKED); };

	afx_msg void OnCheckChannelSearch();

	void RelativeOrMultiplyPrompt(CComboBox &comboBox, FindReplace::ReplaceMode &action, int &value, int range, bool isHex);
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
