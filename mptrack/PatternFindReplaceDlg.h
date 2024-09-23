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

#include "openmpt/all/BuildSettings.hpp"

#include "EffectInfo.h"
#include "PatternCursor.h"
#include "PatternFindReplace.h"
#include "PluginComboBox.h"

OPENMPT_NAMESPACE_BEGIN

/////////////////////////////////////////////////////////////////////////
// Search/Replace

class CFindReplaceTab: public CPropertyPage
{
protected:
	CComboBox m_cbnNote, m_cbnVolCmd, m_cbnVolume, m_cbnCommand, m_cbnParam, m_cbnPCParam;
	PluginComboBox m_cbnInstr;

	CSoundFile &m_sndFile;
	FindReplace &m_settings;
	EffectInfo m_effectInfo;
	ModCommand m_initialValues;
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

		kBeginSpecial = kReplaceInstrumentPlusOne
	};

public:
	CFindReplaceTab(UINT nIDD, bool isReplaceTab, CSoundFile &sf, FindReplace &settings, const ModCommand &initialValues)
		: CPropertyPage(nIDD)
		, m_sndFile(sf)
		, m_settings(settings)
		, m_effectInfo(sf)
		, m_initialValues(initialValues)
		, m_isReplaceTab(isReplaceTab)
	{ }

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	void DoDataExchange(CDataExchange* pDX) override;

	bool IsPCEvent() const;
	
	void UpdateInstrumentList();
	void UpdateVolumeList();
	void UpdateParamList();

	// When a combobox is focussed, check the corresponding checkbox.
	void CheckOnChange(int nIDButton) { CheckDlgButton(nIDButton, BST_CHECKED); CheckReplace(nIDButton); };
	afx_msg void OnNoteChanged();
	afx_msg void OnInstrChanged();
	afx_msg void OnVolCmdChanged();
	afx_msg void OnVolumeChanged();
	afx_msg void OnEffectChanged();
	afx_msg void OnParamChanged();
	afx_msg void OnPCParamChanged();
	// When a checkbox is checked, also check "Replace By".
	afx_msg void OnCheckNote();
	afx_msg void OnCheckInstr();
	afx_msg void OnCheckVolCmd();
	afx_msg void OnCheckVolume();
	afx_msg void OnCheckEffect();
	afx_msg void OnCheckParam();
	// Check "Replace By"
	afx_msg void CheckReplace(int nIDButton);

	afx_msg void OnCheckChannelSearch();

	void RelativeOrMultiplyPrompt(CComboBox &comboBox, FindReplace::ReplaceMode &action, int &value, int range, bool isHex);
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
