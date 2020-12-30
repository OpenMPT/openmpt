/*
 * MIDIMacroDialog.h
 * -----------------
 * Purpose: MIDI Macro Configuration Dialog
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "ColourEdit.h"
#include "../common/misc_util.h"
#include "../soundlib/MIDIMacros.h"

OPENMPT_NAMESPACE_BEGIN

class CMidiMacroSetup: public CDialog
{
protected:
	CComboBox m_CbnSFx, m_CbnSFxPreset, m_CbnZxx, m_CbnZxxPreset, m_CbnMacroPlug, m_CbnMacroParam, m_CbnMacroCC;
	CEdit m_EditSFx, m_EditZxx;
	CColourEdit m_EditMacroValue[NUM_MACROS], m_EditMacroType[NUM_MACROS];
	CButton m_EditMacro[NUM_MACROS], m_BtnMacroShowAll[NUM_MACROS];

	CSoundFile &m_SndFile;

public:
	CMidiMacroSetup(CSoundFile &sndFile, CWnd *parent = nullptr) : CDialog(IDD_MIDIMACRO, parent), m_SndFile(sndFile), m_vMidiCfg(sndFile.m_MidiCfg), m_MidiCfg(m_vMidiCfg.value()) { }
private:
	Util::heap_value<MIDIMacroConfig> m_vMidiCfg;
public:
	MIDIMacroConfig & m_MidiCfg;

protected:
	BOOL OnInitDialog() override;
	void DoDataExchange(CDataExchange* pDX) override;

	bool ValidateMacroString(CEdit &wnd, char *lastMacro, bool isParametric);

	void UpdateMacroList(int macro=-1);
	void ToggleBoxes(UINT preset, UINT sfx);
	afx_msg void UpdateDialog();
	afx_msg void OnSetAsDefault();
	afx_msg void OnResetCfg();
	afx_msg void OnMacroHelp();
	afx_msg void OnSFxChanged();
	afx_msg void OnSFxPresetChanged();
	afx_msg void OnZxxPresetChanged();
	afx_msg void OnSFxEditChanged();
	afx_msg void OnZxxEditChanged();
	afx_msg void UpdateZxxSelection();
	afx_msg void OnPlugChanged();
	afx_msg void OnPlugParamChanged();
	afx_msg void OnCCChanged();

	afx_msg void OnViewAllParams(UINT id);
	afx_msg void OnSetSFx(UINT id);
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
