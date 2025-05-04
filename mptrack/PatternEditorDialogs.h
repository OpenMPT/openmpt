/*
 * PatternEditorDialogs.h
 * ----------------------
 * Purpose: Code for various dialogs that are used in the pattern editor.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "DialogBase.h"
#include "dlg_misc.h"  // for keyboard control
#include "ColorPickerButton.h"
#include "EffectInfo.h"
#include "PatternCursor.h"
#include "PluginComboBox.h"
#include "ResizableDialog.h"
#include "TrackerSettings.h"

OPENMPT_NAMESPACE_BEGIN

class CModDoc;
struct SplitKeyboardSettings;

class CPatternPropertiesDlg : public DialogBase
{
public:
	CPatternPropertiesDlg(CModDoc &modParent, PATTERNINDEX nPat, CWnd *parent = nullptr);

protected:
	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;

	afx_msg void OnHalfRowNumber();
	afx_msg void OnDoubleRowNumber();
	afx_msg void OnOverrideSignature();
	afx_msg void OnTempoSwing();
	afx_msg void OnPatternChanged();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);

	DECLARE_MESSAGE_MAP()

	struct PatternProperties
	{
		std::string name;
		TempoSwing tempoSwing;
		ROWINDEX numRows = 0, rowsPerBeat = 0, rowsPerMeasure = 0;
		ROWINDEX resizeWarningShown = 0;
		bool resizeWarningAtEnd = false;
		bool resizeAtEnd = true, repeatContents = false;
	};

	PatternProperties& GetPatternProperties(PATTERNINDEX pat);
	PatternProperties& GetPatternProperties() { return GetPatternProperties(m_nPattern); }
	void StorePatternProperties();
	void SetCurrentPattern(PATTERNINDEX pat);
	bool ValidatePatternProperties();

	CModDoc &m_modDoc;
	std::map<PATTERNINDEX, PatternProperties> m_properties;
	PATTERNINDEX m_nPattern;
	CSpinButtonCtrl m_spinPattern, m_spinRPB, m_spinRPM;
	CComboBox m_numRows;
	bool m_locked = true;
};


//////////////////////////////////////////////////////////////////////////
// Command Editing


class CEditCommand : public DialogBase
{
protected:
	CComboBox cbnNote, cbnVolCmd, cbnCommand, cbnPlugParam;
	PluginComboBox cbnInstr;
	CSliderCtrl sldVolParam, sldParam;
	CSoundFile &sndFile;
	const CModSpecifications *oldSpecs = nullptr;
	ModCommand *m = nullptr;
	EffectInfo effectInfo;
	PATTERNINDEX editPattern = PATTERNINDEX_INVALID;
	CHANNELINDEX editChannel = CHANNELINDEX_INVALID;
	ROWINDEX editRow = ROWINDEX_INVALID;
	UINT xParam, xMultiplier;
	bool modified = false;

public:
	CEditCommand(CSoundFile &sndFile);

public:
	bool ShowEditWindow(PATTERNINDEX pat, const PatternCursor &cursor, CWnd *parent);

protected:
	void InitAll() { InitNote(); InitVolume(); InitEffect(); InitPlugParam(); }
	void InitNote();
	void InitVolume();
	void InitEffect();
	void InitPlugParam();

	void UpdateVolCmdRange();
	void UpdateVolCmdValue();
	void UpdateEffectRange(bool set);
	void UpdateEffectValue(bool set);

	void PrepareUndo(const char *description);

	//{{AFX_VIRTUAL(CEditCommand)
	void DoDataExchange(CDataExchange *pDX) override;
	void OnOK() override { ShowWindow(SW_HIDE); }
	void OnCancel() override { ShowWindow(SW_HIDE); }
	BOOL PreTranslateMessage(MSG *pMsg) override;
	afx_msg void OnActivate(UINT nState, CWnd *pWndOther, BOOL bMinimized);
	afx_msg void OnClose()	{ ShowWindow(SW_HIDE); }

	afx_msg void OnNoteChanged();
	afx_msg void OnVolCmdChanged();
	afx_msg void OnCommandChanged();
	afx_msg void OnPlugParamChanged();
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Chord Editor

class CChordEditor : public ResizableDialog
{
protected:
	CKeyboardControl m_Keyboard;
	CComboBox m_CbnShortcut, m_CbnBaseNote, m_CbnNote[MPTChord::notesPerChord - 1];
	MPTChords m_chords;
	MPTChord::NoteType m_mouseDownKey = MPTChord::noNote, m_dragKey = MPTChord::noNote;

	static constexpr MPTChord::NoteType CHORD_MIN = -24;
	static constexpr MPTChord::NoteType CHORD_MAX = 24;

public:
	CChordEditor(CWnd *parent = nullptr);

protected:
	MPTChord &GetChord();

	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;

	void UpdateKeyboard();
	afx_msg LRESULT OnKeyboardNotify(WPARAM, LPARAM);
	afx_msg void OnChordChanged();
	afx_msg void OnBaseNoteChanged();
	afx_msg void OnNote1Changed() { OnNoteChanged(0); }
	afx_msg void OnNote2Changed() { OnNoteChanged(1); }
	afx_msg void OnNote3Changed() { OnNoteChanged(2); }
	void OnNoteChanged(int noteIndex);
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Keyboard Split Settings (pattern editor)

class CSplitKeyboardSettings : public DialogBase
{
protected:
	CComboBox m_CbnSplitInstrument, m_CbnSplitNote, m_CbnOctaveModifier, m_CbnSplitVolume;
	CSoundFile &sndFile;

public:
	SplitKeyboardSettings &m_Settings;

	CSplitKeyboardSettings(CWnd *parent, CSoundFile &sf, SplitKeyboardSettings &settings);

protected:
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;
	void OnCancel() override;

	afx_msg void OnOctaveModifierChanged();

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Show channel properties from pattern editor

class QuickChannelProperties : public DialogBase
{
protected:
	CModDoc *m_document = nullptr;
	CHANNELINDEX m_channel = 0;
	bool m_visible = false;
	bool m_settingsChanged = false;
	bool m_settingColor = false;

	ColorPickerButton m_colorBtn, m_colorBtnPrev, m_colorBtnNext;
	CSliderCtrl m_volSlider, m_panSlider;
	CSpinButtonCtrl m_volSpin, m_panSpin;
	CEdit m_nameEdit;

public:
	QuickChannelProperties() = default;
	~QuickChannelProperties();

	void Show(CModDoc *modDoc, CHANNELINDEX chn, CPoint position);
	void UpdateDisplay();
	CHANNELINDEX GetChannel() const { return m_channel; }

protected:
	void DoDataExchange(CDataExchange *pDX) override;

	void PrepareUndo();
	void PickColorFromChannel(CHANNELINDEX channel);

	afx_msg void OnActivate(UINT nState, CWnd *, BOOL);
	afx_msg void OnVolChanged();
	afx_msg void OnPanChanged();
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnMuteChanged();
	afx_msg void OnSurroundChanged();
	afx_msg void OnNameChanged();
	afx_msg void OnPrevChannel();
	afx_msg void OnNextChannel();
	afx_msg void OnChangeColor();
	afx_msg void OnPickPrevColor();
	afx_msg void OnPickNextColor();
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);

	BOOL PreTranslateMessage(MSG *pMsg) override;
	CString GetToolTipText(UINT id, HWND hwnd) const override;

	DECLARE_MESSAGE_MAP();
};


class MetronomeSettingsDlg : public DialogBase
{
public:
	MetronomeSettingsDlg(CWnd *parent = nullptr);

protected:
	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;
	CString GetToolTipText(UINT id, HWND hwnd) const override;

	CString GetVolumeString() const;
	void SetSampleInfo(const mpt::PathString &path, CComboBox &combo, CEdit &edit, CButton &browseButton);
	bool GetSampleInfo(Setting<mpt::PathString> &path, CComboBox &combo, CEdit &edit, CButton &browseButton);
	mpt::PathString BrowseForSample(const mpt::PathString &path);

	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnToggleMetronome();
	afx_msg void OnSampleChanged();
	afx_msg void OnBrowseMeasure();
	afx_msg void OnBrowseBeat();

	DECLARE_MESSAGE_MAP();

protected:
	CSliderCtrl m_volumeSlider;
	CComboBox m_measureCombo, m_beatCombo;
	CEdit m_measureEdit, m_beatEdit;
	CButton m_measureButton, m_beatButton;
};


OPENMPT_NAMESPACE_END
