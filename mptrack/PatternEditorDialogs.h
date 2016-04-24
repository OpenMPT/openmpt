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

#include "dlg_misc.h"	// for keyboard control
#include "Moddoc.h"		// for SplitKeyboardSettings
#include "EffectInfo.h"
#include "PatternCursor.h"

OPENMPT_NAMESPACE_BEGIN

//=========================================
class CPatternPropertiesDlg: public CDialog
//=========================================
{
protected:
	CModDoc &modDoc;
	TempoSwing m_tempoSwing;
	PATTERNINDEX m_nPattern;

public:
	CPatternPropertiesDlg(CModDoc &modParent, PATTERNINDEX nPat, CWnd *parent=NULL)
		: CDialog(IDD_PATTERN_PROPERTIES, parent)
		, modDoc(modParent)
		, m_nPattern(nPat)
	{ }

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnHalfRowNumber();
	afx_msg void OnDoubleRowNumber();
	afx_msg void OnOverrideSignature();
	afx_msg void OnTempoSwing();
	DECLARE_MESSAGE_MAP()
};


//////////////////////////////////////////////////////////////////////////
// Command Editing


//================================
class CEditCommand: public CDialog
//================================
{
protected:
	CComboBox cbnNote, cbnInstr, cbnVolCmd, cbnCommand, cbnPlugParam;
	CSliderCtrl sldVolParam, sldParam;
	CSoundFile &sndFile;
	const CModSpecifications *oldSpecs;
	EffectInfo effectInfo;
	ModCommand *m;
	ModCommandPos editPos;
	UINT xParam, xMultiplier;
	bool modified;

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
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void OnOK()		{ ShowWindow(SW_HIDE); }
	virtual void OnCancel()	{ ShowWindow(SW_HIDE); }
	virtual BOOL PreTranslateMessage(MSG *pMsg);
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

struct MPTChord;

//================================
class CChordEditor: public CDialog
//================================
{
protected:
	CKeyboardControl m_Keyboard;
	CComboBox m_CbnShortcut, m_CbnBaseNote, m_CbnNote1, m_CbnNote2, m_CbnNote3;

public:
	CChordEditor(CWnd *parent=NULL):CDialog(IDD_CHORDEDIT, parent) {}

protected:
	MPTChord &GetChord();

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	void UpdateKeyboard();
	afx_msg LRESULT OnKeyboardNotify(WPARAM, LPARAM);
	afx_msg void OnChordChanged();
	afx_msg void OnBaseNoteChanged();
	afx_msg void OnNote1Changed();
	afx_msg void OnNote2Changed();
	afx_msg void OnNote3Changed();
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Keyboard Split Settings (pattern editor)

//=========================================
class CSplitKeyboadSettings: public CDialog
//=========================================
{
protected:
	CComboBox m_CbnSplitInstrument, m_CbnSplitNote, m_CbnOctaveModifier, m_CbnSplitVolume;
	CSoundFile &sndFile;

public:
	SplitKeyboardSettings &m_Settings;

	CSplitKeyboadSettings(CWnd *parent, CSoundFile &sf, SplitKeyboardSettings &settings) : CDialog(IDD_KEYBOARD_SPLIT, parent), m_Settings(settings), sndFile(sf) { }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	afx_msg void OnOctaveModifierChanged();

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// Show channel properties from pattern editor

//===========================================
class QuickChannelProperties : public CDialog
//===========================================
{
protected:
	CModDoc *document;
	CHANNELINDEX channel;
	PATTERNINDEX pattern;
	bool visible;
	bool settingsChanged;

	CSliderCtrl volSlider, panSlider;
	CSpinButtonCtrl volSpin, panSpin;
	CEdit nameEdit;

public:
	QuickChannelProperties();
	~QuickChannelProperties();

	void Show(CModDoc *modDoc, CHANNELINDEX chn, PATTERNINDEX ptn, CPoint position);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	void UpdateDisplay();
	void PrepareUndo();

	afx_msg void OnActivate(UINT nState, CWnd *, BOOL);
	afx_msg void OnVolChanged();
	afx_msg void OnPanChanged();
	afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnMuteChanged();
	afx_msg void OnSurroundChanged();
	afx_msg void OnNameChanged();
	afx_msg void OnPrevChannel();
	afx_msg void OnNextChannel();
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);

	BOOL PreTranslateMessage(MSG *pMsg);

	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END
